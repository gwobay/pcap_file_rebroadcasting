// based on euronext_cash_markets_xdp_client_specification_v3.23.pdf
#include <Data/QBDF_Util.h>
#include <Data/Euronext_XDP_Handler.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>

#include <MD/MDProvider.h>

#include <Logger/Logger_Manager.h>

#include <Util/CSV_Parser.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <stdarg.h>

#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)

#define EURONEXT_LOGV(X) if(false) { X };

namespace hf_core {
  
  static FactoryRegistrar1<std::string, Handler, Application*, Euronext_XDP_Handler> r1("Euronext_XDP_Handler");
  
  using namespace Euronext_XDP_structs;
  
#define ORDER_ID(obj)                                                   \
  obj.order_id + (((obj.symbol_index * 100ULL) + obj.gtc_indicator) * 4294967296ULL);
 
  void Euronext_XDP_Handler::set_num_contexts(size_t nc) {
      m_context_to_product_ids.resize(nc);
  }
 
#define ZLIB_BUF_LEN 16*1024
 
  static size_t
  decompress_buffer_one_shot(const char* buf, int len, char* decompressed_buf)
  {
    z_stream m_z_stream;
    m_z_stream.zalloc = Z_NULL;
    m_z_stream.zfree = Z_NULL;
    m_z_stream.opaque = Z_NULL;
    m_z_stream.avail_in = 0;
    m_z_stream.next_in = Z_NULL;
    
    int ret = inflateInit(&m_z_stream);
    if(ret != 0) {
      throw runtime_error("Unable to turn on zlib de-compression");
    }
    
    m_z_stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(buf));
    m_z_stream.avail_in = len;
    m_z_stream.next_out = reinterpret_cast<Bytef*>(decompressed_buf);
    m_z_stream.avail_out = ZLIB_BUF_LEN;
    
    int inflateRet = inflate(&m_z_stream, Z_SYNC_FLUSH);
    EURONEXT_LOGV(cout << "decompress_buffer_one_shot: inflateRet=" << inflateRet << endl;)
    
    inflateEnd(&m_z_stream);

    return ZLIB_BUF_LEN - m_z_stream.avail_out;
  }

  void 
  Euronext_XDP_Handler::read_symbol_mapping_from_file()
  {
    m_logger->log_printf(Log::INFO, "Euronext_XDP_Handler::read_symbol_mapping_from_file: Reading symbol mapping from %s", m_symbol_mapping_file.c_str());
    CSV_Parser parser(m_symbol_mapping_file);
  
    int symbol_index_idx = parser.column_index("symbol_index");
    int mic_idx = parser.column_index("mic");
    int sid_idx = parser.column_index("sid");
  
    while(CSV_Parser::OK == parser.read_row()) {
      int symbol_index = strtol(parser.get_column(symbol_index_idx), 0, 10);
      uint64_t sid = boost::lexical_cast<uint64_t>(parser.get_column(sid_idx)); 
      string mic = parser.get_column(mic_idx);
 
      ProductID product_id = Product::invalid_product_id;
      if (m_qbdf_builder) {
        stringstream ss;
        ss << symbol_index;
        product_id = m_qbdf_builder->process_taq_symbol(ss.str());
        if (product_id == 0) {
          m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol index [%d]",
                               "Euronext_Handler::product_id_of_symbol", symbol_index);
          m_logger->sync_flush();
        }
      }
      else
      {
        product_id = m_app->pm()->product_id_by_sid(sid);
      }
      symbol_info* si = get_symbol_info(symbol_index);
      si->product_id = product_id;
      si->exchange_code = ' ';
      
      boost::optional<ExchangeID> eid = ExchangeID::get_by_name(mic.c_str());
      if(eid) {
        si->exch_id = *eid;
        
        vector<ExchangeID>::const_iterator book_itr = find(m_exchanges.begin(), m_exchanges.end(), *eid);
        if(book_itr != m_exchanges.end()) {
          si->book_idx = (book_itr - m_exchanges.begin());
        }
        if (m_qbdf_builder) {
          char exch_char = m_qbdf_builder->get_qbdf_exch_by_hf_exch_id(*eid);
          si->exchange_code = exch_char;
          
          if(m_qbdf_exchange_chars.end() == find(m_qbdf_exchange_chars.begin(), m_qbdf_exchange_chars.end(), exch_char)) {
            m_qbdf_exchange_chars.push_back(exch_char);
          }
        }
      }
    }
  }

  size_t
  Euronext_XDP_Handler::parse(size_t context, const char* buf, size_t len)
  {
    static const char* where = "Euronext_XDP_Handler::parse";

    if(len < sizeof(packet_header)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd", where, m_name.c_str(), len, sizeof(message_header));
      return 0;
    }
    
    if(m_drop_packets) {
      --m_drop_packets;
      return len;
    }
    
    if (m_qbdf_builder) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                             m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }
    
    channel_info& ci = m_channel_info_map[context];

    const packet_header& packet = reinterpret_cast<const packet_header&>(*buf);
    
    uint16_t pkt_size = ntohs(packet.pkt_size);
    uint16_t pkt_type = ntohs(packet.packet_type);
    uint8_t delivery_flag = packet.delivery_flag;
    uint32_t seq_no = ntohl(packet.seq_num);
    uint32_t next_seq_no = seq_no + 1;
  
/* 
    if(seq_no % 2000 == 0 || seq_no % 2015 == 0) 
    { 
#ifdef CW_DEBUG
      cout << " Simulating gap" << endl; 
#endif
      return len; 
    }
 */
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::parse: context=" << context << " pkt_type=" << pkt_type << " delivery_flag=" << static_cast<int>(delivery_flag) << endl;)

    if(pkt_size > len) {
      send_alert("%s: %s Buffer of size %zd smaller than packet length %d", where, m_name.c_str(), len, pkt_size);
      return 0;
    }

    Channel_State& channel_state = m_context_to_state.find(context)->second; 
    Service_State& service_state = *(channel_state.service_state); 
    EURONEXT_LOGV(cout << "channel_state.channel_type=" << channel_state.channel_type << " m_num_reference_groups_remaining=" << m_num_reference_groups_remaining << endl;)

    if(!m_sent_reference_refresh_requests)
    {
      for(hash_map<size_t, Channel_State>::iterator it = m_context_to_state.begin(); it != m_context_to_state.end(); ++it)
      {
        Channel_State& cur_channel_state = it->second;
        Service_State& cur_service_state = *(cur_channel_state.service_state);
        if(cur_channel_state.channel_type == EuronextChannelType::REFERENCE)
        {
          m_logger->log_printf(Log::INFO, "EuronextXDP 1. About to send_snapshot_request for reference data with service id=%d", cur_service_state.m_service_id);
          if(!m_qbdf_builder && m_supports_snapshots && !send_snapshot_request(cur_service_state.m_service_id))
          {
            m_logger->log_printf(Log::WARN, "Euronext_XDP error when sending reference data refresh request");
            throw new exception();
          }
          if(!m_supports_snapshots)
          {
            read_symbol_mapping_from_file();
            m_num_reference_groups_remaining = 0;
            clear_all_books();
          }
        }
      }
      m_sent_reference_refresh_requests = true;
    }

    if(channel_state.channel_type == EuronextChannelType::REFERENCE)
    {
      if(m_num_reference_groups_remaining <= 0)
      {
        return len;
      }
      if(seq_no < ci.seq_no) {
          return len; // duplicate
      }
      Euronext_XDP_Handler::parse2(context, buf, len);
      ci.seq_no = next_seq_no;
    }
    else if(channel_state.channel_type == EuronextChannelType::REFRESH)
    {
      if(!service_state.m_in_refresh_state)
      {
        return len;
      }
      if(m_num_reference_groups_remaining > 0)
      {
        return len;
      }
      if(seq_no < ci.seq_no) {
          return len; // duplicate
      }
      Euronext_XDP_Handler::parse2(context, buf, len);
      ci.seq_no = next_seq_no;
    }
    else
    {
      if(m_num_reference_groups_remaining > 0)
      {
        return len;
      }
      if(pkt_type == 501) { // market data packet
        // Normal path
      
        EURONEXT_LOGV(cout << "service_state.m_in_retransmit_state=" << service_state.m_in_retransmit_state << endl;)
        if(service_state.m_in_retransmit_state)
        {
          if(service_state.m_retransmission_queue.WouldOverflow(len))
          { //ToDo: try to detect this earlier (e.g. if we get 100 messages after our seq num but only needed to retransmit 1, it's probably a bust anyway)
            send_alert("Euronext_XDP_Handler: Overran retransmit message buffer waiting for refresh to finish. context=%d", context);
            clear_all_books(); 
            service_state.m_in_retransmit_state = false;
          }
          else
          {
            service_state.m_retransmission_queue.Enqueue(len, context, buf);
            EURONEXT_LOGV(cout << Time::current_time().to_string() << " In retransmit state; queueing seq_no=" << seq_no << endl;)
          }
          return len;
        }

        EURONEXT_LOGV(cout << "parse: Normal path seq_no=" << seq_no << " ci.seq_no=" << ci.seq_no << " context=" << context << " &ci=" << &ci << endl;)
        if(seq_no < ci.seq_no) {
          return len; // duplicate
        } else if(seq_no > ci.seq_no && (ci.seq_no != 0)) {

          service_state.m_highest_retrans_seq_num_needed = seq_no - 1;
          if(!m_qbdf_builder && send_retransmission_request(service_state.m_service_id, ci.seq_no, seq_no - 1))
          {
            service_state.m_in_retransmit_state = true;
            service_state.m_retransmission_queue.Enqueue(len, context, buf);
            m_logger->log_printf(Log::WARN, "Euronext_XDP Missed message. Expecting %lu got %d. Queueing", ci.seq_no, seq_no);
            return len;
          }
          else
          {
            m_logger->log_printf(Log::WARN, "Euronext_XDP retransmit send failed, so clearing books. Context=%lu", context);
            if(!m_qbdf_builder)
            {
              send_alert("Euronext_XDP_Handler: retransmit send failed, so clearing books context=%d", context);
            }
            clear_all_books(); 
          }
        }
      } else if(pkt_type == 1) {
        // Sequence number reset
        send_alert_forced("%s: %s received sequence reset message orig=%d  new=%d", where, m_name.c_str(), seq_no, next_seq_no);
        ci.clear();
        //ToDo: clear books, make sure expected seq num is zero?
        return len;
      }

      if(!service_state.m_has_sent_refresh_request)
      {
        if(!m_qbdf_builder)
        {
          m_logger->log_printf(Log::INFO, "Euronext_XDP 4. About to send refresh request with service_id=%d", service_state.m_service_id);
          //::sleep(1);
          if(m_supports_snapshots && send_snapshot_request(service_state.m_service_id))
          {
            service_state.m_snapshot_timeout_time = Time::current_time() + seconds(m_snapshot_timeout_seconds);
          }
          else
          {
            clear_all_books(); 
            service_state.m_in_refresh_state = false;
          }
        }
        else if(!m_supports_snapshots)
        {
          clear_all_books(); 
          service_state.m_in_refresh_state = false;
        }
        service_state.m_has_sent_refresh_request = true;
      }

      EURONEXT_LOGV(cout << "service_state.m_in_refresh_state=" << service_state.m_in_refresh_state << endl;)
      if(service_state.m_in_refresh_state)
      {
        if(Time::current_time() > service_state.m_snapshot_timeout_time)
        {
          m_logger->log_printf(Log::WARN, "Euronext_XDP Snapshot timed out for service_id=%d num tries remaining=%d", 
                               service_state.m_service_id, service_state.m_num_refresh_tries_remaining);
          if(service_state.m_num_refresh_tries_remaining <= 0)
          {
            clear_all_books(); 
            service_state.m_in_refresh_state = false;
          }
          else
          {
            --(service_state.m_num_refresh_tries_remaining);
            service_state.m_has_sent_refresh_request = false;
          }
        }
        else if(service_state.m_refresh_queue.WouldOverflow(len))
        {
          send_alert("Euronext_XDP_Handler: Overran queued message buffer waiting for refresh to finish. context=%d", context);
          clear_all_books(); 
          service_state.m_in_refresh_state = false;
        }
        else
        {
          service_state.m_refresh_queue.Enqueue(len, context, buf);
          return len;
        }
      }
      Euronext_XDP_Handler::parse2(context, buf, len);

      ci.seq_no = next_seq_no;
      EURONEXT_LOGV(cout << "Setting ci.seq_no=" << ci.seq_no << endl;)
      ci.last_update = Time::currentish_time();

      //if(!ci.queue.empty()) {
      //  ci.process_recovery(this);
      //}
    } 

    if(pkt_type == 2) {
      EURONEXT_LOGV(cout << "In parse service_id=" << service_state.m_service_id << " context=" << context << endl;)
      //send_snapshot_heartbeat_response(service_state.m_service_id);
      return len;
    } 
    
    return len;
  }

  // we need to fit the order_id into 64 bits
  static inline uint64_t
  get_order_id(const order_update& msg, ProductID product_id)
  {
    uint64_t order_id = (uint64_t) ntohl(msg.order_id);
    uint64_t date = (uint64_t) ntohl(msg.order_date);

    uint64_t id = order_id;

    uint64_t year = (date / 10000) % 8; // so it fits in 3 bits. we are assuming no order
                                        // stays on the book for 8 years.
    uint64_t month = (date % 10000) / 100; // 4 bits suffices
    uint64_t day = date  % 100; // 5 bits
    uint64_t stock_id = product_id; // technically could lose precision, but there's
                                    // enough room for numbers we actually encounter,
                                    // which are all under 600k so far
                                    // (remaining 20 bits ~ 10e6)

    id |= day << 32;
    id |= month << (32 + 5);
    id |= year <<  (32 + 5 + 4);
    id |= stock_id <<  (32 + 5 + 4 + 3);

    return id;
  }

#define GET_TIME(msg)                                                   \
  Time(Time::today().ticks() +					\
       (Time_Constants::ticks_per_mili * (uint64_t)ntohl(msg.source_time)) + \
       (Time_Constants::ticks_per_micro*(uint64_t)ntohs(msg.source_time_micro_secs)));
 
  void
  Euronext_XDP_Handler::process_queued_messages(size_t last_msg_seq_num, size_t context)
  {
    Service_State& service_state = *(m_context_to_state.find(context)->second.service_state);
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::process_queued_messages context=" << context << " service_state.m_refresh_queue.m_offset=" << service_state.m_refresh_queue.m_offset << endl;)

    size_t cur_queue_offset = 0;
    while(service_state.m_refresh_queue.HasMore(cur_queue_offset))
    {
      size_t cur_len;
      size_t cur_context;
      const char* buf = service_state.m_refresh_queue.GetPayloadAt(cur_queue_offset, cur_len, cur_context);
      //Queued_Message_Header* queued_msg_header = reinterpret_cast<Queued_Message_Header*>(&(service_state.m_queued_message_buffer[service_state.m_queued_message_offset]));
      const packet_header& packet = reinterpret_cast<const packet_header&>(*buf);
      uint32_t seq_no = ntohl(packet.seq_num); 
      if(seq_no > last_msg_seq_num)
      {
        m_channel_info_map[cur_context].seq_no = seq_no + 1;
        parse2(cur_context, buf, cur_len);
      } // Else it was already included in the snapshot
    }
    service_state.m_refresh_queue.Reset();
    service_state.m_in_refresh_state = false;
    m_logger->log_printf(Log::INFO, "Euronext_XDP Snapshot process_queued_messages - done");
  }
 
  size_t
  Euronext_XDP_Handler::parse2(size_t context, const char* buf, size_t len)
  {   
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::parse2" << endl;)

    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
    }
    const char* buf_end = buf + len;
    
    const packet_header& packet = reinterpret_cast<const packet_header&>(*buf);
    uint8_t  num_msgs = packet.number_msgs;

    uint8_t delivery_flag = packet.delivery_flag;
    
    buf += sizeof(packet_header);
    
    if(delivery_flag == 17 && num_msgs > 0) 
    {
      // must decompress
      EURONEXT_LOGV(cout << "Euronext_XDP_Handler::parse2 - delivery_flag=17 so decompressing buffer. num_msgs=" << static_cast<int>(num_msgs) << endl;)
      //unsigned new_len = decompress_buffer(buf, len-sizeof(packet_header));
      unsigned new_len = decompress_buffer_one_shot(buf, len-sizeof(packet_header), m_decompressed_buf.get());
      buf = m_decompressed_buf.get();
      buf_end = buf + new_len;

/*
      uint32_t seq_no = ntohl(packet.seq_num);
      Channel_State& channel_state = m_context_to_state.find(context)->second;
      if(channel_state.channel_type == EuronextChannelType::REFRESH)
      {
#ifdef CW_DEBUG
        cout << "REFRESH seq_no=" << seq_no << endl;
#endif
      }
      if(channel_state.channel_type == EuronextChannelType::REFERENCE)
      {
#ifdef CW_DEBUG
        cout << "REFERENCE seq_no=" << seq_no << endl;
#endif
      }
*/
    }
    
    Channel_State& channel_state = m_context_to_state.find(context)->second;
    Service_State& service_state = *(channel_state.service_state); 

    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::parse2 - about to start iterating over num_msgs=" << num_msgs << endl;)
    for(int msg_idx = 0; msg_idx < num_msgs; ++msg_idx) {
      const message_header& header = reinterpret_cast<const message_header&>(*buf);
      
      uint16_t msg_size = ntohs(header.msg_size);
      uint16_t msg_type = ntohs(header.msg_type);
      EURONEXT_LOGV(cout << "   msg_type=" << msg_type << endl;)
      
      const char* msg_buf = buf;

      buf += msg_size + 2; // +2 because the size does not include the uint16_t that stores the size itself

      if(buf > buf_end) {
	send_alert("Euronext_XDP_Handler: %s overran buffer length, len=%zd, msg_type=%d, msg_size=%d", m_name.c_str(), len, msg_type, msg_size);
	break;
      }
      
      if(m_recorder) {
        if(m_record_only) {
          if(msg_type != 550 && msg_type != 551 && msg_type != 580 && msg_type != 581) {
            continue;
          }
        }
      }
      if(channel_state.channel_type == EuronextChannelType::REFRESH && msg_type != 580 && !service_state.m_received_refresh_start)
      {
        continue;
      }
      switch(msg_type) {
      case 140: // Quote
        {
          //const quote_message& msg = reinterpret_cast<const quote_message&>(*msg_buf);
          // Ignore BBO quotes
        }
        break;
      case 230: // order_update
        {
	  const order_update& msg = reinterpret_cast<const order_update&>(*msg_buf);
          
          uint32_t symbol_index = ntohl(msg.symbol_index);
          const symbol_info* si = get_symbol_info(symbol_index);
          
          MDOrderBookHandler* order_book = (si->book_idx != -1) ? m_order_books[si->book_idx].get() : 0;
         
 
          if(order_book) {
            ProductID id = product_id_of_symbol_index(symbol_index);
            if(id != Product::invalid_product_id) {
              
              Time t = GET_TIME(msg);
              m_micros_exch_time = t.total_usec() - m_micros_midnight; //m_today.total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              
              
              bool inside_changed = false;
              
              char action_type = msg.action_type;  // A, D, F, M, Y
              switch(action_type) {
              case 'A':
                {
                  // TODO: to maintain a consistent book, we must time-prioritize the orders based
                  // on OrderPriorityDate, OrderPriorityTime, OrderPriorityMicroSecs (maybe)
                  MDPrice price = get_int_price(ntohl(msg.price), msg.price_scale_code);
                  if (!price.blank()) {
                    // concatenate the product id, order_date and order_id to get an unique identifier (for this security)
                    uint64_t order_id = get_order_id(msg, id);
                    uint32_t shares =   ntohl(msg.volume);
                    char side = msg.side;
                    inside_changed = order_book->add_order_ip(t, order_id, id, side, price, shares);
                    
                    if (m_qbdf_builder) {
                      gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                                m_micros_exch_time_delta, si->exchange_code, side, shares, price);
                    }
                  }
                }
                break;
              case 'M':
                {
                  MDPrice price = get_int_price(ntohl(msg.price), msg.price_scale_code);
                  if (!price.blank()) {
                    // concatenate the order_date and order_id to get an unique identifier
                    uint64_t order_id = get_order_id(msg, id);
                    uint32_t shares =   ntohl(msg.volume);
                    char side = msg.side;
                    if(m_qbdf_builder) { // && !m_exclude_level_2)
                      gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                                   m_micros_exch_time_delta, si->exchange_code, side, shares, price);
                    }
                    
                    {
                      MDOrderBookHandler::mutex_t::scoped_lock lock(order_book->m_mutex);
                      MDOrder* order = order_book->unlocked_find_order(order_id);
                      if (order) {
                        MDOrderBook::mutex_t::scoped_lock book_lock(order_book->book(id).m_mutex, true); // maybe
                        inside_changed = order_book->unlocked_modify_order_ip(t, order, side, price, shares);
                      }
                    }
                  }
                }
                break;
              case 'D':
                {
                  uint64_t order_id = get_order_id(msg, id);
                  if(m_qbdf_builder) { // && !m_exclude_level_2  //??
                    gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                                 m_micros_exch_time_delta, si->exchange_code, ' ', 0, MDPrice());
                  }
                  
                  {
                    MDOrderBookHandler::mutex_t::scoped_lock lock(order_book->m_mutex);
                    MDOrder* order = order_book->unlocked_find_order(order_id);
                    if (order) {
                      MDOrderBook::mutex_t::scoped_lock book_lock(order_book->book(id).m_mutex, true); // maybe
                      inside_changed = order_book->unlocked_remove_order(t, order);
                    }
                  }
                  
                }
                break;
              case 'Y': // need to handle this in the morning to get the book into a consistent state.
                {
                  // "Retransmission of all orders for the given instrument"
                  // but it's just a single message. possibly comes between two
                  // messages of type 231?
                  //MDPrice price = get_int_price(ntohl(msg.price), msg.price_scale_code);
                }
                break;
              case 'F':
                {
                  if(m_qbdf_builder) {
                    gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, id, si->exchange_code, context);
                  }
                  order_book->clear(id, true);
                  inside_changed = true;
                  
                  // TODO: acquire lock? -- and use MDOrderBookHandler::unlocked_clear(id, true) ??
                  if(msg.side == 0x0) {
                    // clear both sides of the book
                  } else if(msg.side == 'B') {
                    // clear buys. not sure if this function exists yet.
                  } else if(msg.side == 'S') {
                    // clear sells. not sure if this function exists yet.
                  }
                }
                break;
              }
              if(inside_changed) {
                m_quote.set_time(t);
                m_quote.m_id = id;
                m_quote.m_exch = si->exch_id;
                order_book->fill_quote_update(id, m_quote);
                send_quote_update(order_book);
              }
            }
          }
        }
        break;
      case 231: // order_retransmission
        {
          // whether we are receiving messages of type Y ??
        }
        break;
      case 240: // trade
        {
          const trade_full_message& msg = reinterpret_cast<const trade_full_message&>(*msg_buf);
          uint8_t trade_cond3 = msg.trade_condition[2];
          if (trade_cond3 != '4' && trade_cond3 != '7') {
            uint32_t symbol_index = ntohl(msg.symbol_index);
            ProductID id = product_id_of_symbol_index(symbol_index);
            if(id != Product::invalid_product_id) {
              const symbol_info* si = get_symbol_info(symbol_index);
              
              Time t = GET_TIME(msg);
              if(t.is_set()) {
                m_trade.set_time(t);
              }
              MDPrice price = get_int_price(ntohl(msg.price), msg.price_scale_code);
              if (!price.blank()) {
                uint32_t shares = ntohl(msg.volume);
                char side = ' '; // unknown
                
                if (!m_qbdf_builder) {
                  m_trade.m_id = id;
                  EURONEXT_LOGV(cout << "About to send_trade_update symbol_index=" << symbol_index << " si->exch_id=" << si->exch_id << endl;)
                  m_trade.m_exch = si->exch_id;
                  m_trade.m_price = price.fprice();
                  m_trade.m_size = shares;
                  m_trade.set_side(side);
                  m_trade.clear_flags();
                  memcpy(m_trade.m_trade_qualifier, msg.trade_condition, 4);
                  m_trade.m_trade_qualifier[3] = msg.opening_trade_indicator;
                  m_trade.m_correct = 0;
                  send_trade_update();
                } else {
                  m_micros_exch_time = t.total_usec() - m_micros_midnight; //m_today.total_usec();
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                  
                  char cond[] = {' ', ' ', ' ', ' '};
                  cond[1] = msg.trade_condition[1];
                  cond[2] = msg.trade_condition[2];
		  // BMC 20150323: last char of condition codes not used, so use it for
		  // opening trade indicator
		  cond[3] = msg.opening_trade_indicator;
                  
                  gen_qbdf_trade_micros(m_qbdf_builder, id, m_micros_recv_time,
                                        m_micros_exch_time_delta, si->exchange_code, side, '0',
                                        string(cond, 4), shares, price);
                                        // 1, 2 b/c the first and last trade condition bytes
                                        // are not used and generall contain null bytes
                                        // but the function is expecting a string of length 4
                } 
              }
            }
          }
        }
        break;
      case 245: // Auction summary
        {
          /*
          const auction_summary_message& msg = reinterpret_cast<const auction_summary_message&>(*msg_buf);
          uint32_t symbol_index = ntohl(msg.symbol_index);
          ProductID id = product_id_of_symbol_index(symbol_index);
          if(id != Product::invalid_product_id) {
            Time t = GET_TIME(msg);
            MDPrice price = get_int_price(ntohl(msg.price), msg.price_scale_code);
            }
          */
        }
        break;
      case 505:
        {
	  const stock_state_change& msg = reinterpret_cast<const stock_state_change&>(*msg_buf);

          if(m_qbdf_builder) {

          } else {
            ProductID id = product_id_of_symbol_index(ntohl(msg.symbol_index));
            if(id != Product::invalid_product_id) {
              m_msu.m_id = id;

              /** e.g. from BATS/PITCH
              switch(msg.halt_status) {
              case 'A': m_msu.m_market_status = MarketStatus::Auction; break;
              case 'H': m_msu.m_market_status = MarketStatus::Halted; break;
              case 'Q': m_msu.m_market_status = MarketStatus::Auction; break;
              case 'S': m_msu.m_market_status = MarketStatus::Suspended; break;
              case 'T': m_msu.m_market_status = MarketStatus::Open; break;
              default:
                break;
              }
              m_security_status[id] = m_msu.m_market_status;
              if(m_subscribers) {
                m_subscribers->update_subscribers(m_msu);
              }
              */
            }
          }
        }
        break;
      case 516:
        {
        }
        break;
      case 530: // Indicative Matching Price Message
        {
          const indicative_matching_price_message& msg = reinterpret_cast<const indicative_matching_price_message&>(*msg_buf);
          
          uint32_t symbol_index = ntohl(msg.symbol_index);
          ProductID id = product_id_of_symbol_index(symbol_index);
          if(id != Product::invalid_product_id) {
            Time t = GET_TIME(msg);
            m_micros_exch_time = t.total_usec() - m_micros_midnight; //m_today.total_usec();
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            
            const symbol_info* si = get_symbol_info(symbol_index);
            
            MDPrice near_price = get_int_price(ntohl(msg.im_price), msg.price_scale_code);
            
            m_auction.m_id = id;
            m_auction.m_exch = si->exch_id;
            m_auction.m_ref_price = 0.0;
            m_auction.m_far_price = 0.0;
            m_auction.m_paired_size = ntohl(msg.im_volume);
            m_auction.m_near_price = near_price.fprice();
            m_auction.m_indicative_price_flag = ' ';
            
            if(ntohs(msg.msg_size) == 42) {
              m_auction.m_imbalance_size = ntohl(msg.imbalance_volume);
              switch(msg.imbalance_volume_side) {
              case '1': m_auction.m_imbalance_direction = 'B'; break;
              case '2': m_auction.m_imbalance_direction = 'S'; break;
              default:  m_auction.m_imbalance_direction = ' '; break;
              }
            } else {
              m_auction.m_imbalance_size = 0;
              m_auction.m_imbalance_direction = ' ';
            }
            

            if(!m_qbdf_builder) {
              if(m_subscribers) {
                m_subscribers->update_subscribers(m_auction);
              }
            } else {
              if(!m_exclude_imbalance) {
                MDPrice ref_price, far_price;
                gen_qbdf_imbalance_micros(m_qbdf_builder, id, m_micros_recv_time,
                                          m_micros_exch_time_delta, si->exchange_code,
                                          ref_price, near_price, far_price,
                                          m_auction.m_paired_size, m_auction.m_imbalance_size,
                                          ' ', ' ');
              }
            }
          }
        }
        break;
      case 550:
        {
          m_logger->log_printf(Log::INFO, "Euronext_XDP 2. Start reference refresh message 550");
        }
        break;
      case 551:
        {
          --m_num_reference_groups_remaining; // ToDo: make sure we really got the whole thing
          m_logger->log_printf(Log::INFO, "Euronext_XDP 3. End reference refresh message 551 2. Start reference refresh message 550=%d", m_num_reference_groups_remaining);
          if(!m_recorder || !m_record_only)
          {
            clear_all_books(); 
          }
        }
        break;
      case 553:
        {
         EURONEXT_LOGV(cout << " Got reference data message 553" << endl;)
         const reference_data_message& msg = reinterpret_cast<const reference_data_message&>(*msg_buf);
         int symbol_index = ntohl(msg.symbol_index);
         string instrument_name = string(msg.instrument_name); 
         string isin = string(msg.instrument_code, sizeof(msg.instrument_code));
         string mnemo = strlen(msg.mnemo) > sizeof(msg.mnemo) ? string(msg.mnemo, sizeof(msg.mnemo)) : string(msg.mnemo);
         string trading_code = string(msg.trading_code);
         char instrument_category = msg.instrument_category;
         string mic = string(msg.mic, sizeof(msg.mic));
         string cfi = string(msg.cfi, sizeof(msg.cfi));
         string market_feed_code = string(msg.market_feed_code, sizeof(msg.market_feed_code));

         if(cfi[0] == 'E')
         {
           string symbol = mnemo + "." + mic;

           EURONEXT_LOGV(cout << " symbol_index=" << symbol_index << " instrument_name='" << instrument_name << "' isin='" << isin
                << "' mnemo='" << mnemo << "' trading_code='" << trading_code << "' instrument_category='" << instrument_category 
                << "' mic='" << mic << "' cfi='" << cfi << "' symbol='" << symbol << "' system_id=" << ntohl(msg.system_id)
                << " type_of_market_admission=" << msg.type_of_market_admission << " market_feed_code='" << market_feed_code
                << endl;)

           ProductID product_id = Product::invalid_product_id;
           if(m_qbdf_builder)
           {
              product_id = m_qbdf_builder->process_taq_symbol(symbol.c_str());
           }
           else
           {
             hash_map<string, ProductID>::iterator it = m_subscription_symbol_to_product_id.find(symbol);
             if(it != m_subscription_symbol_to_product_id.end())
             {
               product_id = it->second;
             }
           }
           if(product_id != Product::invalid_product_id)
           {
             EURONEXT_LOGV(cout << "Mapping symbol_index=" << symbol_index << " to product_id=" << product_id << " symbol=" << symbol << endl;)
             boost::optional<ExchangeID> eid = ExchangeID::get_by_name(mic.c_str());
             EURONEXT_LOGV(cout << "m_exchanges.size()=" << m_exchanges.size() << endl;)
             if(eid) {
               EURONEXT_LOGV(cout << " *eid=" << *eid << endl;)
               if (m_qbdf_builder) {
                 symbol_info* si = &m_symbol_info_map[symbol_index];
                 char exch_char = m_qbdf_builder->get_qbdf_exch_by_hf_exch_id(*eid);
                 si->exchange_code = exch_char;
            
                 if(m_qbdf_exchange_chars.end() == find(m_qbdf_exchange_chars.begin(), m_qbdf_exchange_chars.end(), exch_char)) {
                   m_qbdf_exchange_chars.push_back(exch_char);
                 }
               }
               EURONEXT_LOGV(cout << "m_exchanges[0]=" << m_exchanges[0] << endl;)
               vector<ExchangeID>::const_iterator book_itr = find(m_exchanges.begin(), m_exchanges.end(), *eid);
               if(book_itr != m_exchanges.end()) {
                 symbol_info* si = &m_symbol_info_map[symbol_index];
                 int book_index = (book_itr - m_exchanges.begin());
                 if(static_cast<unsigned>(book_index) >= m_order_books.size()) { book_index = -1; }
                 si->product_id = product_id;
                 si->book_idx = book_index;
                 si->exch_id = *eid;
                 EURONEXT_LOGV(cout << " setting book_idx=" << si->book_idx << " exch_id=" << si->exch_id << endl;)
               }
             }
           }
         }
        }
        break;
      case 580: //Start Refresh Message
        {
          if(channel_state.channel_type == EuronextChannelType::REFRESH)
          {
            m_logger->log_printf(Log::INFO, "Euronext_XDP 5. Start refresh message 580 context=%lu service_id=%d", context, service_state.m_service_id);
            //const start_refresh_message& msg = reinterpret_cast<const start_refresh_message&>(*msg_buf);
            service_state.m_received_refresh_start = true;
          }
        }
        break;
      case 581: // End Refresh Message
        {
          if(channel_state.channel_type == EuronextChannelType::REFRESH && service_state.m_received_refresh_start)
          {
            m_logger->log_printf(Log::INFO, "Euronext_XDP 6. End refresh message 581 service_id=%d", service_state.m_service_id);
            const end_refresh_message& msg = reinterpret_cast<const end_refresh_message&>(*msg_buf);
            size_t last_msg_seq_num = ntohl(msg.last_seq_num);
            process_queued_messages(last_msg_seq_num, context);
            service_state.m_in_refresh_state = false;
          }
        }
        break;
      default:
        {
          // m_logger->log_printf(Log::INFO, "Received unknown msg_type %d", (int)msg_type);
        }
	break;
      }

    }

    EURONEXT_LOGV(cout << "parse2 - about to return len=" << len << endl;)
    return len;
  }


#undef  SIZE_CHECK  
#define SIZE_CHECK(type)                                                \
  if(unlikely(ntohs(msg.msg_size)+2 != sizeof(type))) {                 \
    log.printf("ERROR --  called with " #type " len=%u, expecting %lu", (unsigned)ntohs(msg.msg_size)+2, sizeof(type)); \
  }                                                                     \
 
#define GET_TIME_DURATION(msg) \
  Time_Duration((Time_Constants::ticks_per_mili * (uint64_t)ntohl(msg.source_time)) + \
                (Time_Constants::ticks_per_micro*(uint64_t)ntohs(msg.source_time_micro_secs)));
  
  size_t
  Euronext_XDP_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len)
  {
    const packet_header& packet = reinterpret_cast<const packet_header&>(*buf);
    
    uint8_t  num_msgs = packet.number_msgs;
    Time send_time = Time(Time_Constants::ticks_per_mili * (uint64_t)ntohl(packet.send_time));
    
    char time_buf[32];
    Time_Utils::print_time(time_buf, send_time, Timestamp::MICRO);
    
    log.printf(" \"send_time\":%s, \"seq_num\":%u, \"delivery_flag\":%u, \"entries\":%u \"packet_type\":%u", time_buf, ntohl(packet.seq_num), packet.delivery_flag, num_msgs, ntohs(packet.packet_type));
    
    buf += sizeof(packet_header);
  
    char decompressed_buf[ZLIB_BUF_LEN];
    if(packet.delivery_flag == 17) {
      if(packet_len <= sizeof(packet_header)) {
        log.printf(" }\n");
        return packet_len;
      }
      // must decompress
      unsigned new_len = decompress_buffer_one_shot(buf, packet_len-sizeof(packet_header), decompressed_buf);
      
      log.printf(", \"decompressed_len\":%u", new_len);
      
      buf = decompressed_buf;
    }
    log.printf(" }\n");
    
    for(int msg_idx = 0; msg_idx < num_msgs; ++msg_idx) {
      const message_header& header = reinterpret_cast<const message_header&>(*buf);
      
      uint16_t msg_size = ntohs(header.msg_size);
      uint16_t msg_type = ntohs(header.msg_type);
      
      log.printf(" { \"msg_type_id\":%d, \"msg_size\":%d", msg_type, msg_size);
      
      const char* msg_buf = buf;
      buf += 2 + msg_size;
      switch(msg_type) {
        /*
        */
      case 140: // quote
        {
          const quote_message& msg = reinterpret_cast<const quote_message&>(*msg_buf);
          SIZE_CHECK(quote_message);
          
          uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
          
          MDPrice ask_price = get_int_price(ntohl(msg.ask_price), msg.price_scale_code);
          MDPrice bid_price = get_int_price(ntohl(msg.bid_price), msg.price_scale_code);
          
          log.printf("  quote  source_time:%s symbol_index:%u source_seq_num:%u  system_id:%u  ask_price:%0.4f  ask_size:%u  bid_price:%0.4f  bid_size:%u  quote_number:%d",
                     time_buf, symbol_index, ntohl(msg.source_seq_num), ntohl(msg.system_id),
                     ask_price.fprice(), ntohl(msg.ask_size), bid_price.fprice(), ntohl(msg.bid_size), msg.quote_number);
        }
        break;
      case 230: // order_update
        {
          const order_update& msg = reinterpret_cast<const order_update&>(*msg_buf);
          SIZE_CHECK(order_update);

          uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
          
          MDPrice price = get_int_price(ntohl(msg.price), msg.price_scale_code);
          
          log.printf("  order_update  source_time:%s symbol_index:%u source_seq_num:%u  system_id:%u  order_id:%u  price:%0.4f  size:%u  side:%c  order_type:%c  action_type:%c order_date:%u",
                     time_buf, symbol_index, ntohl(msg.source_seq_num), ntohl(msg.system_id), ntohl(msg.order_id), 
                     price.fprice(), ntohl(msg.volume), msg.side, msg.order_type ?  msg.order_type : '0', msg.action_type, ntohl(msg.order_date));
          
        }
        break;
      case 231:
        {
          const order_retransmission_delimiter& msg = reinterpret_cast<const order_retransmission_delimiter&>(*msg_buf);
          SIZE_CHECK(order_retransmission_delimiter);
          
          Time_Duration source_time = Time_Duration(Time_Constants::ticks_per_mili * (uint64_t)ntohl(msg.source_time));
          Time_Utils::print_time(time_buf, source_time, Timestamp::MILI);

          log.printf("  order_retransmission_delimiter  source_time:%s  source_seq_num:%u  trading_engine_id:%.2s  instance_id:%d  retransmission_indicator:%c",
                     time_buf, ntohl(msg.source_seq_num), msg.trading_engine_id, msg.instance_id, msg.retransmission_indicator);

        }
        break;
      case 240: // trade_full
        {
          const trade_full_message& msg = reinterpret_cast<const trade_full_message&>(*msg_buf);
          SIZE_CHECK(trade_full_message);
          
          uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
          
          MDPrice price = get_int_price(ntohl(msg.price), msg.price_scale_code);

          log.printf("  trade_full  source_time:%s symbol_index:%u source_seq_num:%u  system_id:%u  price:%0.4f  size:%u trade_cond2:%c trade_cond3:%c  opening:%c",
                     time_buf, symbol_index, ntohl(msg.source_seq_num), ntohl(msg.system_id),
                     price.fprice(), ntohl(msg.volume), msg.trade_condition[1], msg.trade_condition[2],
                     msg.opening_trade_indicator);
          
          
        }
        break;
      case 530: // Indicative Matching Price Message
        {
          const indicative_matching_price_message& msg = reinterpret_cast<const indicative_matching_price_message&>(*msg_buf);
          //SIZE_CHECK(indicative_matching_price_message);
          
          uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
          
          MDPrice im_price = get_int_price(ntohl(msg.im_price), msg.price_scale_code);
          
          log.printf("  imbalance  source_time:%s symbol_index:%u source_seq_num:%u  system_id:%u  im_price:%0.4f  im_volume:%u",
                     time_buf, symbol_index, ntohl(msg.source_seq_num), ntohl(msg.system_id), im_price.fprice(), ntohl(msg.im_volume));
          if(ntohs(msg.msg_size) == 42) {
            log.printf("  imbalance_volume_side:%c  imbalance_volume:%u", msg.imbalance_volume_side, ntohl(msg.imbalance_volume));
          }
        }
        break;
      case 537: // Collar
        {
          const collar_message& msg = reinterpret_cast<const collar_message&>(*msg_buf);
          SIZE_CHECK(collar_message);
          
          uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
          
          MDPrice high = get_int_price(ntohl(msg.high_collar), msg.high_scale_code);
          MDPrice low = get_int_price(ntohl(msg.low_collar), msg.low_scale_code);
          
          log.printf("  collar  source_time:%s symbol_index:%u source_seq_num:%u  system_id:%u  high_collar:%0.4f  low_collar:%0.4f",
                     time_buf, symbol_index, ntohl(msg.source_seq_num), ntohl(msg.system_id),
                     high.fprice(), low.fprice());
        }
        break;
      case 550:
        {
          log.printf("  start_reference_data");
        }
        break;
      case 551:
        {
          log.printf("  end_reference_data");
        }
        break;
      case 553:
        {
          EURONEXT_LOGV(cout << " Got reference data message 553" << endl;)
          const reference_data_message& msg = reinterpret_cast<const reference_data_message&>(*msg_buf);
          SIZE_CHECK(reference_data_message);
          
          uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
         
          string mnemo = strlen(msg.mnemo) > sizeof(msg.mnemo) ? string(msg.mnemo, sizeof(msg.mnemo)) : string(msg.mnemo);
 
          log.printf(", \"msg_type\":\"reference_data\", \"symbol_index\":%u, \"ISIN\":\"%.12s\", \"instrument_name\":\"%.18s\", \"mic\":\"%.4s\", \"financial_market_code\":\"%.3s\", \"mic_list\":\"%.24s\", \"lot_size\":%lu, \"MNEMO\":\"%s\"",
                     symbol_index, msg.instrument_code, msg.instrument_name, msg.mic,
                     msg.financial_market_code, msg.mic_list, ntohll(msg.lot_size), mnemo.c_str());
        }
        break;
      case 221: // Cancel or Bust
	{
	  const trade_cancel_message& msg = reinterpret_cast<const trade_cancel_message&>(*msg_buf);

	  uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
	  Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
	  log.printf ( " trade_cancel source_time:%s symbol_index:%u source_seq_num:%u original_trade_id:%u system_id:%u",
		       time_buf, symbol_index, ntohl(msg.source_seq_num), ntohl(msg.original_trade_id_number), ntohl(msg.system_id));
	}
	break ;
      case 241:
        {
          const price_update_message& msg = reinterpret_cast<const price_update_message&>(*msg_buf);
	  uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
	  Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);

          MDPrice price = get_int_price(ntohl(msg.price), msg.price_scale_code);

	  log.printf ( " price_update source_time:%s symbol_index:%u source_seq_num:%u price:%0.4f type_of_price:%.2s",
		       time_buf, symbol_index, ntohl(msg.source_seq_num), price.fprice(), msg.type_of_price);

        }
        break;
      case 245:
        {
          const auction_summary_message& msg = reinterpret_cast<const auction_summary_message&>(*msg_buf);
	  uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
	  Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);

          MDPrice price = get_int_price(ntohl(msg.last_price), msg.price_scale_code);
          uint32_t cumulative_quantity = ntohl(msg.cumulative_quantity);

	  log.printf( " auction_summary source_time:%s symbol_index:%u source_seq_num:%u price:%0.4f cumulative_quantity:%u instrument_valuation_price:%c",
		       time_buf, symbol_index, ntohl(msg.source_seq_num), price.fprice(), cumulative_quantity, msg.instrument_valuation_price);

        }
        break;
      default : // we don't know the messsage type yet.
	log.printf ( " msg_type %d: not implemented ", msg_type );
        break;
      }
      
      log.printf(" },\n");
    }
    
    return packet_len;
  }

  size_t
  Euronext_XDP_Handler::dump_refdata_headers(ILogger& log)
  {
    log.printf("instrument_code,symbol_index,instrument_name,trading_code,mic,financial_market_code,mic_list,lot_size,fix_price_tick,type_of_market_admission,issuing_country_code,event_date,instrument_group,instrument_category,type_of_instrument,mnemo\n");
    return 0;
  }

  size_t
  Euronext_XDP_Handler::dump_refdata(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len)
  {
    const packet_header& packet = reinterpret_cast<const packet_header&>(*buf);
    
    uint8_t  num_msgs = packet.number_msgs;
    Time_Duration send_time = Time_Duration(Time_Constants::ticks_per_mili * (uint64_t)ntohl(packet.send_time));
    
    char time_buf[32];
    Time_Utils::print_time(time_buf, send_time, Timestamp::MICRO);
    
    buf += sizeof(packet_header);
    
    char decompressed_buf[ZLIB_BUF_LEN];
    if(packet.delivery_flag == 17) {
      if(packet_len <= sizeof(packet_header)) {
        return packet_len;
      }
      // must decompress
      //unsigned new_len =
      decompress_buffer_one_shot(buf, packet_len-sizeof(packet_header), decompressed_buf);
      buf = decompressed_buf;
    }
    
    for(int msg_idx = 0; msg_idx < num_msgs; ++msg_idx) {
      const message_header& header = reinterpret_cast<const message_header&>(*buf);
      
      uint16_t msg_size = ntohs(header.msg_size);
      uint16_t msg_type = ntohs(header.msg_type);
      
      const char* msg_buf = buf;
      buf += 2 + msg_size;
      switch(msg_type) {
      case 553:
        {
          const reference_data_message& msg = reinterpret_cast<const reference_data_message&>(*msg_buf);
          SIZE_CHECK(reference_data_message);
          
          uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
          
          char instrument_name[18];
          for(int i = 0; i < 18; ++i) {
            char c = msg.instrument_name[i];
            if(!(c == ',' || c == '\'' || c == '"' || c == '\n')) {
              instrument_name[i] = c;
            } else {
              instrument_name[i] = '_';
            }
          }
          
          string mnemo = strlen(msg.mnemo) > sizeof(msg.mnemo) ? string(msg.mnemo, sizeof(msg.mnemo)) : string(msg.mnemo);
 
          log.printf("%.12s,%u,%.18s,%.12s,%.4s,%.3s,%.24s,%lu,%u,%c,%.3s,%.8s,%.3s,%c,%hu,%s\n",
                     msg.instrument_code, symbol_index, instrument_name, msg.trading_code,
                     msg.mic, msg.financial_market_code, msg.mic_list,
                     ntohll(msg.lot_size), ntohl(msg.fix_price_tick),
                     msg.type_of_market_admission, msg.issuing_country_code, msg.event_date, msg.instrument_group_code, msg.instrument_category,
                     ntohs(msg.type_of_instrument), mnemo.c_str());
          
        }
        break;
      }
    }
    return packet_len;
  }


  void
  Euronext_XDP_Handler::init(const string& name, const vector<ExchangeID>& exchanges, const Parameter& params)
  {
    m_supported_exchanges = exchanges;

    //const char* where = "Euronext_XDP_Handler::init";

    Handler::init(name, params);
    
    m_name = name;
   
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::init build_book=" << params.getDefaultBool("build_book", false) << endl;)
    if (params.getDefaultBool("build_book", false)) {
      int prealloc_orders = params.getDefaultInt("prealloc_orders", 50000);
      for(vector<ExchangeID>::const_iterator eid = exchanges.begin(); eid != exchanges.end(); ++eid) {
        boost::shared_ptr<MDOrderBookHandler> ob(new MDOrderBookHandler(m_app, *eid, m_logger));
        ob->init(params);
        ob->prealloc_order_pool(prealloc_orders);
        EURONEXT_LOGV(cout << "Euronext_XDP_Handler::init - m_order_books.push_back ob for *eid=" << *eid << endl;)
        m_order_books.push_back(ob);
      }
    }
  
    for(vector<ExchangeID>::const_iterator it = exchanges.begin(); it != exchanges.end(); ++it)
    {
      m_exchanges.push_back(*it);
    } 
    
    string snapshot_start_date_str = params["snapshot_start_date"].getString();
    Time  snapshot_start_date(snapshot_start_date_str, "%Y%m%d");
    m_supports_snapshots = (Time::today() >= snapshot_start_date);
    if(!m_supports_snapshots)
    {
      m_symbol_mapping_file = params["symbol_mapping"].getString();
    }
 
    m_source_id = params.getDefaultString("source_id", "");
    m_snapshot_timeout_seconds = params.getDefaultInt("snapshot_timeout_seconds", 120);
    
    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';
    
    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);
    
    if (m_qbdf_builder) {
      int64_t tz_offset = m_qbdf_builder->get_bin_tz_adjust() * Time_Constants::micros_per_second;
      m_micros_midnight = Time::today().total_usec() + tz_offset;
    }
    //m_today           = Time::today();
    //m_today           = m_today - hours(1);
    // Feed is in CET timezone, so we place the offset here
    //m_micros_midnight = Time::today().ticks() / Time_Constants::ticks_per_micro;
    
    m_lot_size = params.getDefaultInt("lot_size", m_lot_size);
    
    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);
    
    m_max_send_source_difference = params.getDefaultTimeDuration("max_send_source_difference", m_max_send_source_difference);

    set_up_channels(params);
  }

  void 
  Euronext_XDP_Handler::set_up_channels(const Parameter& params)
  {
    const vector<Parameter>& lines = params["mcast_lines"].getVector();
    size_t context = 0;
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      
      const string& channel_type = (*l)["channel_type"].getString();
      int service_id_int = (*l)["service_id"].getInt();
      //cout << "service_id_int=" << service_id_int << endl;
      uint16_t service_id = static_cast<uint16_t>(service_id_int);
      set_service_id(context, service_id, channel_type);
      ++context;
    }
  }

  void
  Euronext_XDP_Handler::start()
  {
    clear_all_books(); 
  }

  void
  Euronext_XDP_Handler::stop()
  {
    clear_all_books(); 
  }

  void
  Euronext_XDP_Handler::send_snapshot_heartbeat_response(uint16_t service_id)
  {
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::send_snapshot_heartbeat_response: service_id=" << service_id << " m_snapshot_socket.connected()=" << m_snapshot_socket.connected() << endl;)
    if(!m_snapshot_socket.connected()) {
      return;
    }

    struct {
      packet_header  pkt_header;
      heartbeat_response_message payload;
    } __attribute__ ((packed)) resp;

    ++m_req_seq_num;

    //Time t = Time::current_time();
    //uint32_t send_time = t.sec();

    memset(&resp, 0, sizeof(resp));
    resp.pkt_header.pkt_size = htons(sizeof(resp));
    resp.pkt_header.packet_type = ntohs(22);
    resp.pkt_header.delivery_flag = 0;
    resp.pkt_header.number_msgs = 1;
    resp.pkt_header.seq_num = 0;//m_req_seq_num;
    resp.pkt_header.send_time = 0;//htonl(send_time);
    resp.pkt_header.service_id = htons(service_id);
    resp.pkt_header.delivery_flag = 0;
    strncpy(resp.payload.source_id, m_source_id.c_str(), m_source_id.size());

     EURONEXT_LOGV(cout << "Rseponse payload source id=" << string(resp.payload.source_id) << endl;)

    if(m_snapshot_socket.connected()) {
      EURONEXT_LOGV(cout << "Euronext_XDP_Handler::send_snapshot_heartbeat_response - Sending " << sizeof(resp) << " bytes" << endl;)
      m_logger->log_printf(Log::INFO, "Euronext_XDP_Handler::send_snapshot_heartbeat_response - Sending %lu bytes", sizeof(resp));
      if(send(m_snapshot_socket, &resp, sizeof(resp), 0) < 0) {
        int send_err = errno;
        m_logger->log_printf(Log::WARN, "Euronext_XDP_Handler::send_snapshot_heartbeat_response send got negative response, closing. Errno=%d", send_err);
        m_snapshot_socket.close();
        send_alert("%s error sending snapshot heartbeat response, closing socket", m_name.c_str());
      }
    }
  }

  void
  Euronext_XDP_Handler::send_retransmission_heartbeat_response(uint16_t service_id)
  {
     EURONEXT_LOGV(cout << "Euronext_XDP_Handler::send_retransmission_heartbeat_response: service_id=" << service_id << " m_retransmission_socket.connected()=" << m_retransmission_socket.connected() << endl; )
    if(!m_retransmission_socket.connected()) {
      return;
    }

    struct {
      packet_header  pkt_header;
      heartbeat_response_message payload;
    } __attribute__ ((packed)) resp;

    ++m_req_seq_num;

    //Time t = Time::current_time();
    //uint32_t send_time = t.sec();

    memset(&resp, 0, sizeof(resp));
    resp.pkt_header.pkt_size = htons(sizeof(resp));
    resp.pkt_header.packet_type = ntohs(24);
    resp.pkt_header.delivery_flag = 0;
    resp.pkt_header.number_msgs = 1;
    resp.pkt_header.seq_num = 0;//m_req_seq_num;
    resp.pkt_header.send_time = 0;//htonl(send_time);
    resp.pkt_header.service_id = htons(service_id);
    resp.pkt_header.delivery_flag = 0;
    strncpy(resp.payload.source_id, m_source_id.c_str(), m_source_id.size());

     EURONEXT_LOGV(cout << "Rseponse payload source id=" << string(resp.payload.source_id) << endl;)

    if(m_retransmission_socket.connected()) {
       EURONEXT_LOGV(cout << "Euronext_XDP_Handler::send_retransmission_heartbeat_response - Sending " << sizeof(resp) << " bytes" << endl;)
      if(send(m_retransmission_socket, &resp, sizeof(resp), 0) < 0) {
        int send_err = errno;
        m_logger->log_printf(Log::WARN, "Euronext_XDP_Handler::send_retransmission_heartbeat_response send got negative response, closing. Errno=%d", send_err);
        m_retransmission_socket.close();
        send_alert("%s error sending retransmission heartbeat response, closing socket", m_name.c_str());
      }
    }
  }

  bool 
  Euronext_XDP_Handler::send_retransmission_request(uint16_t service_id, uint32_t begin_seq_no, uint32_t end_seq_no)
  {
     EURONEXT_LOGV(cout << "Euronext_XDP_Handler::send_retransmission_request: m_retransmission_socket.connected()=" << m_retransmission_socket.connected() << endl; )
    if(!m_retransmission_socket.connected()) {
      return false;
    }

    struct {
      packet_header  pkt_header;
      uint32_t begin_seq_num;
      uint32_t end_seq_num;
      char source_id[20];
    } __attribute__ ((packed)) req;


    Time t = Time::current_time();
    uint32_t send_time = t.sec();

    memset(&req, 0, sizeof(req));
    req.pkt_header.pkt_size = htons(sizeof(req));
    req.pkt_header.packet_type = ntohs(20);
    req.pkt_header.delivery_flag = 11;
    req.pkt_header.number_msgs = 1;
    req.pkt_header.seq_num = htonl(begin_seq_no);//m_req_seq_num;
    req.pkt_header.send_time = htonl(send_time);
    req.pkt_header.service_id = htons(service_id);
    req.begin_seq_num = htonl(begin_seq_no);
    req.end_seq_num = htonl(end_seq_no);
    strncpy(req.source_id, m_source_id.c_str(), m_source_id.size());

    EURONEXT_LOGV(cout << "Request payload source id=" << string(req.source_id) << endl;)
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::send_retransmission_request - begin_seq_no=" << begin_seq_no << " end_seq_no=" << end_seq_no << " service_id=" << service_id << endl;)
    m_logger->log_printf(Log::INFO, "Euronext_XDP_Handler  7. Euronext_XDP_Handler::send_retransmission_request - Sending %lu bytes begin_seq_no=%d end_seq_no=%d", sizeof(req), begin_seq_no, end_seq_no);
    if(send(m_retransmission_socket, &req, sizeof(req), 0) < 0) {
      int send_err = errno;
      m_logger->log_printf(Log::WARN, "Euronext_XDP_Handler::send_retransmission_request send got negative response, closing. Errno=%d", send_err);
      m_retransmission_socket.close();
      send_alert("%s error sending retransmission request, closing socket", m_name.c_str());
      return false;
    }
    return true;
  }

  bool
  Euronext_XDP_Handler::send_snapshot_request(uint16_t service_id)
  {
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::send_snapshot_request" << endl;)

    //if(service_id == 301) { service_id = 201; }

    if(!m_snapshot_socket.connected()) {
      m_logger->log_printf(Log::INFO, "service_id=%d sent refresh request called but socket =- -1", service_id);
      return false;
    }

    struct {
      packet_header  pkt_header;
      refresh_request_message payload;
    } __attribute__((packed)) req;

    ++m_refresh_req_seq_num;

    Time t = Time::current_time();
    uint32_t send_time = t.sec();
    memset(&req, 0, sizeof(req));
    req.pkt_header.pkt_size = htons(sizeof(req));
    req.pkt_header.packet_type = ntohs(22);
    req.pkt_header.seq_num = ntohl(m_refresh_req_seq_num);
    req.pkt_header.send_time = htonl(send_time);
    req.pkt_header.service_id = htons(service_id);
    // delivery_flag is ignored
    req.pkt_header.number_msgs = 1;

    strncpy(req.payload.source_id, m_source_id.c_str(), m_source_id.size());

    int ret = 0;
    ret = ::send(m_snapshot_socket, &req, sizeof(req), MSG_DONTWAIT);
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::send_snapshot_request - sizeof(req)=" << sizeof(req) << " ret=" << ret << " service_id=" << service_id << endl;)

    if(ret < 0) {
      m_logger->log_printf(Log::WARN, "Euronext_XDP_Handler::send_snapshot_request send got negative=%d", ret);
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert("Euronext_XDP_Handler::send_refresh_request %s sendto returned %d - %s", m_name.c_str(), errno, err_msg);
      return false;
    }
    return true;
  }
  
  size_t
  Euronext_XDP_Handler::read(size_t context, const char* buf, size_t len)
  {
    if(len < 2) {
      return 0;
    }
    //cout << "Euronext_XDP_Handler::read TCP len=" << len << endl;
    
    //const char* where = "Euronext_XDP_Handler::handle_retran_msg";
    const packet_header& pkt_header = reinterpret_cast<const packet_header&>(*buf);
    
    uint16_t pkt_size = ntohs(pkt_header.pkt_size); // ntohs here?
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::read pkt_size=" << pkt_size << endl;)
    if(len < pkt_size) {
      //cout << Time::current_time().to_string() << "WARN got TCP read with len=" << len << " < pkt_size=" << pkt_size << endl;
      return 0;
    }

    uint16_t pkt_type = ntohs(pkt_header.packet_type);
    //uint8_t delivery_flag = pkt_header.delivery_flag;
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::read - Got packet header with type=" << pkt_type << endl;)

    uint16_t service_id = ntohs(pkt_header.service_id);
    if(pkt_type == 23)
    {
      const refresh_response_message& msg = reinterpret_cast<const refresh_response_message&>(*(buf + sizeof(pkt_header)));
      string source_id(msg.source_id, 10);
//      if(source_id == m_source_id)
//      {
        char status = msg.status;
        EURONEXT_LOGV(cout << "    status=" << status << endl;)
        if(status != 'A')
        {
          char reject_reason_code = msg.reject_reason + '0';
          send_alert("Euronext_XDP_Handler: Received Refresh Reject on context=%d. status=%c reject_reason code=%c", context, status, reject_reason_code);
          m_logger->log_printf(Log::WARN, "Euronext_XDP_Handler got refresh reject with reason=%c", reject_reason_code);
          process_queued_messages(0, context);
          for(hash_map<uint16_t, Service_State>::iterator it = m_service_id_to_state.begin(); it != m_service_id_to_state.end(); ++it)
          {
            Service_State& service_state = it->second;
            service_state.m_in_refresh_state = false;
          }
        }
//      }
    }
    else if(pkt_type == 10)
    {
      const retransmission_response_message& msg = reinterpret_cast<const retransmission_response_message&>(*(buf + sizeof(pkt_header)));
      string source_id(msg.source_id, 10);
      char status = msg.status;
      EURONEXT_LOGV(cout << "    retransmission response status=" << status << endl;)
      if(status != 'A')
      {
        char reject_reason_code = msg.reject_reason + '0';
        m_logger->log_printf(Log::WARN, "Euronext_XDP_Handler got retransmission reject with reason=%c. Have to clear all books", reject_reason_code);

        Service_State& service_state = m_service_id_to_state.find(service_id)->second;

        clear_all_books(); 

        service_state.m_retransmission_queue.Reset();
        service_state.m_in_retransmit_state = false;
      }
    }
    else if(pkt_type == 2)
    {
      EURONEXT_LOGV(cout << "In read service_id=" << service_id << " context=" << context << endl;)
      EURONEXT_LOGV(cout << "Using service_id=" << service_id << endl;)
      if(context == m_refresh_tcp_context)
      {
        if(service_id == 0)
        {
          for(hash_map<uint16_t, Service_State>::iterator it = m_service_id_to_state.begin(); it != m_service_id_to_state.end(); ++it)
          {
            uint16_t cur_service_id = it->first;
            send_snapshot_heartbeat_response(cur_service_id);
          }
        }
        else
        {
          send_snapshot_heartbeat_response(service_id);
          //cout << "    AAA" << endl;
        }
      }
      else if(context == m_retransmission_tcp_context)
      {
        if(service_id == 0)
        {
          service_id = m_tcp_heartbeat_response_service_id;
        }
        send_retransmission_heartbeat_response(service_id);
      }
    }
    else
    {
      EURONEXT_LOGV(cout << "Euronext_XDP_Handler::read service_id=" << service_id << endl;)
      Service_State& service_state = m_service_id_to_state.find(service_id)->second;
      EURONEXT_LOGV(cout << "  service_state.m_incremental_context=" << service_state.m_incremental_context << endl;)
      parse2(service_state.m_incremental_context, buf, len);
      if(ntohl(pkt_header.seq_num) == service_state.m_highest_retrans_seq_num_needed)
      {
        m_channel_info_map[service_state.m_incremental_context].seq_no = ntohl(pkt_header.seq_num) + 1;
        EURONEXT_LOGV(cout << "Euronext_XDP_Handler::read - setting seq_no=" <<  ntohl(pkt_header.seq_num) + 1 << " for context=" << service_state.m_incremental_context << endl;)
        size_t cur_queue_offset = 0;
        while(service_state.m_retransmission_queue.HasMore(cur_queue_offset))
        {
          size_t cur_len;
          size_t cur_context;
          const char* buf_ptr = service_state.m_retransmission_queue.GetPayloadAt(cur_queue_offset, cur_len, cur_context);
          const packet_header& packet = reinterpret_cast<const packet_header&>(*buf_ptr);
          uint32_t seq_no = ntohl(packet.seq_num);
          // ToDo: protect against missed queued messages
          uint32_t expected_seq_no = m_channel_info_map[cur_context].seq_no;
          if(seq_no > expected_seq_no)
          {
            m_logger->log_printf(Log::WARN, "Euronext_XDP_Handler::read WARN got missed message during queued messages (expecting %d got %d)", expected_seq_no, seq_no);

            if(false || !send_retransmission_request(service_state.m_service_id, expected_seq_no, seq_no - 1)) // This happened too many times?
            {
              m_logger->log_printf(Log::WARN, "Euronext_XDP_Handler Aborting retransmission and clearing books. context=%lu", cur_context);
              clear_all_books();
              break;
            }
            else
            {
              service_state.m_highest_retrans_seq_num_needed = seq_no - 1;
              // Should we move the queue over so we don't replay duplicates?
              return pkt_size;
            }
          }
          else if(seq_no == m_channel_info_map[cur_context].seq_no)
          {
            parse2(cur_context, buf_ptr, cur_len);
            EURONEXT_LOGV(cout << " REPLAY MSGS QUEUED DURING RETRANSMIT: seq_no=" << seq_no << endl;)
            m_channel_info_map[cur_context].seq_no = seq_no + 1;
          }
        }
        service_state.m_retransmission_queue.Reset();
        service_state.m_in_retransmit_state = false;
        m_logger->log_printf(Log::INFO, "Euronext_XDP 8. Done processing retransmission");
      }
    }
 
    return pkt_size;
  }
 
  void
  Euronext_XDP_Handler::clear_all_books()
  {
    m_logger->log_printf(Log::WARN, "Euronext_XDP_Handler::clear_all_books");
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    clear_order_books(false);
  }
 
  void
  Euronext_XDP_Handler::add_context_product_id(size_t context, ProductID id)
  {
    if(m_context_to_product_ids.size() <= context) {
      m_context_to_product_ids.resize(context+1);
    }
    m_context_to_product_ids[context].push_back(id);
  }

  void
  Euronext_XDP_Handler::subscribe_product(ProductID id, ExchangeID exch,
                                          const string& mdSymbol, const string& mdExch)
  {
    Product& product = m_app->pm()->find_by_id(id);
    EURONEXT_LOGV(cout << "subscribe_product called with id=" << id << " mdSymbol='" << mdSymbol << "' mdExch=" << mdExch << " product.symbol()=" << product.symbol() << endl;)
    mutex_t::scoped_lock lock(m_mutex);
    m_subscription_symbol_to_product_id[product.symbol()] = id;
  }

  void
  Euronext_XDP_Handler::reset(size_t context, const char* msg)
  {
    if(context == 3) { // quote context
      reset(msg);
      return;
    }
    
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    
  }

  void
  Euronext_XDP_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
   
    clear_all_books(); 
    
    if(m_qbdf_builder) {
      BOOST_FOREACH(char exch_char, m_qbdf_exchange_chars) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, exch_char, -1);
      }
    }
    
    if(m_subscribers) {
      BOOST_FOREACH(ExchangeID eid, m_supported_exchanges) {
        m_subscribers->post_invalid_quote(eid);
      }
    }
  }

  void
  Euronext_XDP_Handler::send_quote_update(MDOrderBookHandler* order_book)
  {
    order_book->fill_quote_update(m_quote.m_id, m_quote);
    m_msu.m_market_status = m_security_status[m_quote.m_id];
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);

    if(m_msu.m_market_status != m_security_status[m_quote.m_id]) {
      m_msu.m_id = m_quote.m_id;
      m_msu.m_exch = m_quote.m_exch;
      m_security_status[m_quote.m_id] = m_msu.m_market_status;
      if(m_subscribers) {
        m_subscribers->update_subscribers(m_msu);
      }
    }
    if(m_provider) {
      m_provider->check_quote(m_quote);
    }
    update_subscribers(m_quote);
  }

  void
  Euronext_XDP_Handler::send_trade_update()
  {
    m_msu.m_market_status = m_security_status[m_trade.m_id];
    m_feed_rules->apply_trade_conditions(m_trade, m_msu);
    if(m_msu.m_market_status != m_security_status[m_trade.m_id]) {
      m_msu.m_id = m_trade.m_id;
      m_security_status[m_trade.m_id] = m_msu.m_market_status;
      m_msu.m_exch = m_trade.m_exch;
      if(m_subscribers) {
        m_subscribers->update_subscribers(m_msu);
      }
    }
    if(m_provider) {
      m_provider->check_trade(m_trade);
    }
    update_subscribers(m_trade);
  }
  
  ProductID
  Euronext_XDP_Handler::product_id_of_symbol_index(uint32_t symbol_index)
  {
    hash_map<uint32_t, symbol_info>::iterator it = m_symbol_info_map.find(symbol_index);
    if(it != m_symbol_info_map.end())
    {
      return it->second.product_id;
    }
    return Product::invalid_product_id;
/*
*/
  }


  Euronext_XDP_Handler::Euronext_XDP_Handler(Application* app) :
    Handler(app, "Euronext_XDP_Handler"),
    m_max_send_source_difference(msec(10000)),
    m_lot_size(100),
    m_req_seq_num(0),
    m_refresh_req_seq_num(0),
    m_micros_midnight(0),
    m_num_reference_groups_remaining(0),
    m_sent_reference_refresh_requests(false)
  {
    EURONEXT_LOGV(cout << "Euronext_XDP_Handler::Euronext_XDP_Handler" << endl;)
    m_symbol_info_map.resize(1048576); // 2**20
    m_orders_to_delete_at_open.resize(65536);
    m_orders_to_delete_at_close.resize(65536);

    /* allocate inflate state */
    {
      m_z_stream.zalloc = Z_NULL;
      m_z_stream.zfree = Z_NULL;
      m_z_stream.opaque = Z_NULL;
      m_z_stream.avail_in = 0;
      m_z_stream.next_in = Z_NULL;
      int ret = inflateInit(&m_z_stream);
      if(ret != 0) {
        throw runtime_error("Unable to turn on zlib de-compression");
      }
      m_decompressed_buf.reset(new char[ZLIB_BUF_LEN]);
    }
  }

  Euronext_XDP_Handler::~Euronext_XDP_Handler()
  {
    inflateEnd(&m_z_stream);
  }
}
