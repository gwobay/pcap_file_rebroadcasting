#include <Data/QBDF_Util.h>
#include <Data/TVITCH_5_Handler.h>
#include <Util/Network_Utils.h>

#include <Exchange/Exchange_Manager.h>
#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <MD/MDProvider.h>
#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>
#include <stdint.h>


#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)


namespace hf_core {

  static FactoryRegistrar1<std::string, Handler, Application*, TVITCH_5_Handler> r1("TVITCH_5_Handler");
  
  namespace tvitch_5 {
    
    struct nanos_t {
      uint32_t h;
      uint16_t l;
    } __attribute__ ((packed));
    
    struct message_block_t {
      uint16_t message_length;
    } __attribute__ ((packed));
    
    struct downstream_header_t {
      char      session[10];
      uint64_t  seq_num;
      uint16_t  message_count;
    } __attribute__ ((packed));
    
    struct request_packet_t {
      char      session[10];
      uint64_t  seq_num;
      uint16_t  message_count;
    } __attribute__ ((packed));
    
    struct common_message {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
    } __attribute__ ((packed));
    
    struct system_event_message {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      char     event_code;
    } __attribute__ ((packed));

    struct stock_directory {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      char     stock[8];
      char     market_category;
      char     financial_status;
      uint32_t round_lot_size;
      char     round_lots_only;
      char     issue_classification;
      char     issue_sub_type[2];
      char     authenticity;
      char     short_sale_threshold_indicator;
      char     ipo_flag;
      char     luld_reference_price_tier;
      char     etp_flag;
      uint32_t etp_leverage_factor;
      char     inverse_indicator;
    } __attribute__ ((packed));

    struct stock_trading_action {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      char     stock[8];
      char     trading_state;
      char     reserved;
      char     reason[4];
    } __attribute__ ((packed));

    struct reg_sho_restriction {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      char     stock[8];
      char     reg_sho_action;
    } __attribute__ ((packed));

    struct market_participant_position {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      char     mpid[4];
      char     stock[8];
      char     primary_market_maker;
      char     market_maker_mode;
      char     market_participant_state;
    } __attribute__ ((packed));
    
    struct mwcb_decline_level {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t level1;
      uint64_t level2;
      uint64_t level3;
    } __attribute__ ((packed));

    struct mwcb_status {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      char     breached_level;
    } __attribute__ ((packed));
    
    struct ipo_quoting_period_update {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      char     stock[8];
      uint32_t ipo_release_time;
      char     ipo_release_qualifier;
      uint32_t ipo_price;
    } __attribute__ ((packed));
    
    struct add_order_no_mpid {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint32_t price;
    } __attribute__ ((packed));

    struct add_order_mpid {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint32_t price;
      char     attribution[4];
    } __attribute__ ((packed));

    struct order_executed {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t order_id;
      uint32_t executed_shares;
      uint64_t match_number;
    } __attribute__ ((packed));

    struct order_executed_with_price {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t order_id;
      uint32_t executed_shares;
      uint64_t match_number;
      char     printable;
      uint32_t execution_price;
    } __attribute__ ((packed));

    struct order_cancel_message {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t order_id;
      uint32_t canceled_shares;
    } __attribute__ ((packed));

    struct order_delete_message {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t order_id;
    } __attribute__ ((packed));

    struct order_replace_message {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t orig_order_id;
      uint64_t new_order_id;
      uint32_t shares;
      uint32_t price;
    } __attribute__ ((packed));
    
    struct trade_message {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t order_ref_number;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint32_t price;
      uint64_t match_number;
    } __attribute__ ((packed));

    struct cross_message {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t shares;
      char     stock[8];
      uint32_t cross_price;
      uint64_t match_number;
      char     cross_type;
    } __attribute__ ((packed));

    struct broken_trade_message {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t match_number;
    } __attribute__ ((packed));

    struct imbalance_message {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      uint64_t paired_shares;
      uint64_t imbalance_shares;
      char     imbalance_direction;
      char     stock[8];
      uint32_t far_price;
      uint32_t near_price;
      uint32_t ref_price;
      char     cross_type;
      char     price_variation_indicator;
    } __attribute__ ((packed));

    struct retail_price_improvement {
      char     message_type;
      uint16_t stock_locate;
      uint16_t tracking_number;
      nanos_t  nanoseconds;
      char     stock[8];
      char     interest_flag;
    } __attribute__ ((packed));
    
  }
  
  
  ProductID
  TVITCH_5_Handler::product_id_of_symbol(const char* symbol)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol, 8);
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);
      
      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string);
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol [%s]",
                             "TVITCH_5_Handler::product_id_of_symbol", symbol_string.c_str());
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      return symbol_id;
    }
    
    ProductID id = m_prod_map.find8(symbol);
    return id;
  }
  
  size_t
  TVITCH_5_Handler::parse(size_t context, const char* buffer, size_t len)
  {
    const tvitch_5::downstream_header_t* header = reinterpret_cast<const tvitch_5::downstream_header_t*>(buffer);
    
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    uint64_t next_seq_num = seq_num + message_count;
    
    if(unlikely(m_drop_packets)) {
      --m_drop_packets;
      return 0;
    }
    
    if(next_seq_num <= m_channel_info.seq_no) {
      return 0;
    }
    
    mutex_t::scoped_lock lock;
    if(context != 0) {
      lock.acquire(m_mutex);
    }
    
    if(unlikely(!m_channel_info.last_update.is_set())) {
      memcpy(m_session, header->session, 10);
      m_logger->log_printf(Log::INFO, "TVITCH_5_Handler: %s Session = %10s", m_name.c_str(), m_session);
      m_channel_info.seq_no = seq_num;
    }
    /*
    if(unlikely(strncmp(m_session, header->session, 10))) {
      return 0;
    }
    */
    
    if(next_seq_num <= m_channel_info.seq_no) {
      return 0;
    } else if(seq_num > m_channel_info.seq_no) {
      if(m_channel_info.begin_recovery(this, buffer, len, seq_num, next_seq_num)) {
        return 0;
      }
    }
    
    size_t parsed_len = TVITCH_5_Handler::parse2(context, buffer, len);
    
    m_channel_info.seq_no = next_seq_num;
    m_channel_info.last_update = Time::currentish_time();
    
    if(!m_channel_info.queue.empty()) {
      m_channel_info.process_recovery(this);
    }
    
    return parsed_len;
  }

  size_t
  TVITCH_5_Handler::parse2(size_t context, const char* buffer, size_t len)
  {
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buffer, len);
      if(m_record_only) {
        return len;
      }
    }
    
    const tvitch_5::downstream_header_t* header = reinterpret_cast<const tvitch_5::downstream_header_t*>(buffer);
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    
    size_t parsed_len = sizeof(tvitch_5::downstream_header_t);
    const char* block = buffer + sizeof(tvitch_5::downstream_header_t);
    
    if(seq_num < m_channel_info.seq_no && message_count > 0) {
      //m_logger->log_printf(Log::INFO, "TVITCH_5_Handler::parse2: %s received partially-new packet seq_num=%zd expected_seq_num=%zd msg_count=%d",
      //                     m_name.c_str(), seq_num, m_channel_info.seq_no, message_count);
      while(seq_num < m_channel_info.seq_no && message_count > 0) {
        // a rare case, this message contains some things we've already seen and some
        // things we haven't, skip what we've already seen
        uint16_t message_length = ntohs(reinterpret_cast<const tvitch_5::message_block_t*>(block)->message_length);
        parsed_len += 2 + message_length;
        if (unlikely(parsed_len > len)) {
          m_logger->log_printf(Log::ERROR, "TVITCH_5_Handler::parse: tried to more bytes than len (%zd)", len);
          return 0;
        }
        block += 2 + message_length;
        --message_count;
        ++seq_num;
      }
    }
    
    for( ; message_count > 0; --message_count) {
      uint16_t message_length = ntohs(reinterpret_cast<const tvitch_5::message_block_t*>(block)->message_length);
      parsed_len += 2 + message_length;
      if (unlikely(parsed_len > len)) {
        m_logger->log_printf(Log::ERROR, "TVITCH_5_Handler::parse: tried to more bytes than len (%zd)", len);
        return 0;
      }
      size_t parser_result = parse_message(0, block+2, message_length);
      if (unlikely(parser_result != message_length)) {
        m_logger->log_printf(Log::ERROR, "TVITCH_5_Handler::parse: ERROR message parser returned %zu bytes parsed, expected %u\n",
                             parser_result, message_length);
        return 0;
      }
      block += 2 + message_length;
    }
    
    if(unlikely(parsed_len != len)) {
      m_logger->log_printf(Log::ERROR, "TVITCH_5_Handler::parse: Received buffer of size %zd but parsed %zd, message_count=%d",
                           len, parsed_len, (int) ntohs(header->message_count));
      return 0;
    }
    
    return parsed_len;
  }
  

  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  if(unlikely(len != sizeof(type))) {                                   \
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), len, sizeof(type)); \
    return len;                                                         \
  }                                                                     \
  
  // Common check to insure that we looked up the product id
#define PRODUCT_CHECK()                                         \
  if(unlikely(id == Product::invalid_product_id)) {             \
    id = product_id_of_symbol(msg.stock);                       \
    if(id == Product::invalid_product_id) {                     \
      return len;                                               \
    } else {                                                    \
      m_stock_to_product_id[msg.stock_locate] = id;             \
    }                                                           \
  }
  
  static inline
  uint64_t nanos_to_time(const tvitch_5::nanos_t& n) {
    uint64_t h = ntohl(n.h);
    uint64_t l = ntohs(n.h);
    return (h << 16 | l);
  }
  
  size_t
  TVITCH_5_Handler::parse_message(size_t, const char* buf, size_t len)
  {
    static const char* where = "TVITCH_5_Handler::parse_message";
    
    if(unlikely(len == 0)) {
      m_logger->log_printf(Log::ERROR, "%s %s called with len=0", where, m_name.c_str());
      return len;
    }
    
    const tvitch_5::common_message* common = reinterpret_cast<const tvitch_5::common_message*>(buf);
    ProductID id = m_stock_to_product_id[common->stock_locate];
    uint64_t ns = nanos_to_time(common->nanoseconds);
    Time t = m_midnight + Time_Duration(ns);
    
    if(m_qbdf_builder) {
      m_micros_exch_time = t.total_usec() - Time::today().total_usec();
      if(!m_hist_mode) {
        m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
        if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
          m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
          m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                               m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
          m_logger->sync_flush();
        }
        m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
      } else {  // hist mode

        m_micros_recv_time = m_micros_exch_time;
        m_micros_exch_time_delta = 0;
      }
    }
    
    switch(buf[0]) {
    case 'A':
      {
        SIZE_CHECK(tvitch_5::add_order_no_mpid);
        const tvitch_5::add_order_no_mpid& msg = reinterpret_cast<const tvitch_5::add_order_no_mpid&>(*buf);
        PRODUCT_CHECK();
        
        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.shares);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));
        
        if(likely(m_order_book)) {
          bool inside_changed = m_order_book->add_order_ip(t, order_id, id, msg.side, price, shares);
          
          if(!m_qbdf_builder) {
            if(inside_changed) {
              m_quote.set_time(t);
              m_quote.m_id = id;
              send_quote_update();
            }
          } else {
            if(!m_exclude_level_2) {
              gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                        m_micros_exch_time_delta, m_exch_char,
                                        msg.side, shares, price);
            }
          }
        }
      }
      break;
    case 'B':
      {
        //SIZE_CHECK(tvitch_5::broken_trade_message);
        //const tvitch_5::broken_trade_message& msg = reinterpret_cast<const tvitch_5::broken_trade_message&>(*buf);
      }
      break;
    case 'C':
      {
        SIZE_CHECK(tvitch_5::order_executed_with_price);
        const tvitch_5::order_executed_with_price& msg = reinterpret_cast<const tvitch_5::order_executed_with_price&>(*buf);
        
        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.executed_shares);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.execution_price));
        
        if(likely(m_order_book)) {
          bool order_found = false;
          bool inside_changed = false;
          uint32_t new_size = 0;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
            
            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              if(unlikely(id == -1)) {
                id = order->product_id;
                m_stock_to_product_id[msg.stock_locate] = id;
              }
              new_size = order->size - shares;
              order_found = true;
              inside_changed = order->is_top_level();
              m_trade.m_id = id;
              m_trade.set_side(order->flip_side());
              
              MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
              if(new_size > 0) {
                m_order_book->unlocked_modify_order(t, order, new_size, shares);
              } else {
                m_order_book->unlocked_remove_order(t, order, shares);
              }
            }
          }
          
          if(!m_qbdf_builder) {
            if(order_found && msg.printable != 'N') {
              m_trade.set_time(t);
              m_trade.m_price = price.fprice();
              m_trade.m_size = shares;
              m_trade.clear_flags();
              m_trade.set_exec_flag(TradeUpdate::exec_flag_exec_w_price);
              
              if (!m_qbdf_builder) {
                send_trade_update();
              }
            }
            if(inside_changed) {
              m_quote.set_time(t);
              m_quote.m_id = m_trade.m_id;
              if (!m_qbdf_builder) {
                send_quote_update();
              }
            }
            if(m_order_book->orderbook_update()) {
              m_order_book->post_orderbook_update();
            }
          } else {
            if(order_found) {
              if(msg.printable == 'Y') {
                gen_qbdf_order_exec_micros(m_qbdf_builder, id, order_id,
                                           m_micros_recv_time, m_micros_exch_time_delta,
                                           m_exch_char, m_trade.side(), msg.printable,
                                           shares, price);
              } else {
                if(!m_exclude_level_2) {
                  gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id,
                                               m_micros_recv_time, m_micros_exch_time_delta,
                                               m_exch_char, m_trade.side(), new_size, price);
                }
              }
            }
          }
        }
      }
      break;
    case 'D':
      {
        SIZE_CHECK(tvitch_5::order_delete_message);
        const tvitch_5::order_delete_message& msg = reinterpret_cast<const tvitch_5::order_delete_message&>(*buf);
        
        uint64_t order_id = ntohll(msg.order_id);
        
        if(likely(m_order_book)) {
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
            
            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              if(unlikely(id == -1)) {
                id = order->product_id;
                m_stock_to_product_id[msg.stock_locate] = id;
              }
              inside_changed = order->is_top_level();
              m_quote.m_id = id;
              MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
              m_order_book->unlocked_remove_order(t, order);
            }
          }
          
          if(!m_qbdf_builder) {
            if(inside_changed) {
              m_quote.set_time(t);
              send_quote_update();
            }
            if(m_order_book->orderbook_update()) {
              m_order_book->post_orderbook_update();
            }
          } else {
            if(!m_exclude_level_2) {
              gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id,
                                           m_micros_recv_time, m_micros_exch_time_delta,
                                           m_exch_char, ' ', 0, MDPrice());
            }
          }
        }
      }
      break;
    case 'E':
      {
        SIZE_CHECK(tvitch_5::order_executed);
        const tvitch_5::order_executed& msg = reinterpret_cast<const tvitch_5::order_executed&>(*buf);
        
        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.executed_shares);
        
        if(m_order_book) {
          bool order_found = false;
          bool inside_changed = false;
          MDPrice order_price;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
            
            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              if(unlikely(id == -1)) {
                id = order->product_id;
                m_stock_to_product_id[msg.stock_locate] = id;
              }
              order_found = true;
              m_trade.m_id = id;
              m_trade.m_price = order->price_as_double();
              m_trade.set_side(order->flip_side());
              
              order_price = order->price;
              
              uint32_t new_size = order->size - shares;
              inside_changed = order->is_top_level();
              
              MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
              if(new_size > 0) {
                m_order_book->unlocked_modify_order(t, order, new_size, shares);
              } else {
                m_order_book->unlocked_remove_order(t, order, shares);
              }
            }
          }
          
          if(!m_qbdf_builder) {
            if(order_found) {
              m_trade.set_time(t);
              m_trade.m_size = shares;
              m_trade.clear_flags();
              m_trade.set_exec_flag(TradeUpdate::exec_flag_exec);
              
              send_trade_update();
              
              if(inside_changed) {
                m_quote.set_time(t);
                m_quote.m_id = id;
                send_quote_update();
              }
              if(m_order_book->orderbook_update()) {
                m_order_book->post_orderbook_update();
              }
            }
          } else {
            if(!m_exclude_level_2 && id != -1) {
              gen_qbdf_order_exec_micros(m_qbdf_builder, id, order_id,
                                         m_micros_recv_time, m_micros_exch_time_delta,
                                         m_exch_char, m_trade.side(), ' ',
                                         shares, order_price);
            }
          }
        }
      }
      break;
    case 'F':
      {
        SIZE_CHECK(tvitch_5::add_order_mpid);
        const tvitch_5::add_order_mpid& msg = reinterpret_cast<const tvitch_5::add_order_mpid&>(*buf);
        PRODUCT_CHECK();
        
        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.shares);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));
        
        if(likely(m_order_book)) {
          bool inside_changed = m_order_book->add_order_ip(t, order_id, id, msg.side, price, shares);
          
          if(!m_qbdf_builder) {
            if(inside_changed) {
              m_quote.set_time(t);
              m_quote.m_id = id;
              if (!m_qbdf_builder) {
                send_quote_update();
              }
            }
          } else {
	    if(!m_exclude_level_2) {
	      gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
					m_micros_exch_time_delta, m_exch_char,
					msg.side, shares, price);
	    }
	  }
        }
      }
      break;
    case 'H':
      {
        SIZE_CHECK(tvitch_5::stock_trading_action);
        const tvitch_5::stock_trading_action& msg = reinterpret_cast<const tvitch_5::stock_trading_action&>(*buf);
        PRODUCT_CHECK();
        
        m_msu.set_time(t);
        m_msu.m_id = id;
        
        switch(msg.trading_state) {
        case 'H':
          m_msu.m_market_status = MarketStatus::Halted;
          m_qbdf_status = QBDFStatusType::Halted;
          break;
        case 'R':
          m_msu.m_market_status = MarketStatus::Auction;
          m_qbdf_status = QBDFStatusType::Auction;
          break;
        case 'Q':
          m_msu.m_market_status = MarketStatus::Auction;
          m_qbdf_status = QBDFStatusType::Auction;
          break;
        case 'T':
          m_msu.m_market_status = MarketStatus::Open;
          m_qbdf_status = QBDFStatusType::Open;
          break;
        case 'V':
          m_msu.m_market_status = MarketStatus::Halted;
          m_qbdf_status = QBDFStatusType::Halted;
          break;
        default:
          m_msu.m_market_status = MarketStatus::Halted;
          m_qbdf_status = QBDFStatusType::Halted;
          break;
        }
        
        if(!m_qbdf_builder) {
          m_msu.m_reason.assign(msg.reason, 4);
          m_security_status[id] = m_msu.m_market_status;
          if(m_subscribers) {
            m_subscribers->update_subscribers(m_msu);
          }
          m_msu.m_reason.clear();
        } else {
	  if(!m_exclude_status) {
	    m_qbdf_status_reason = string("NASDAQ Trading State ") + msg.trading_state;
	    gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time,
				   m_micros_exch_time_delta, m_exch_char,
				   m_qbdf_status.index(), m_qbdf_status_reason);
	  }          
        }
      }
      break;
    case 'I':
      {
        SIZE_CHECK(tvitch_5::imbalance_message);
        const tvitch_5::imbalance_message& msg = reinterpret_cast<const tvitch_5::imbalance_message&>(*buf);
        PRODUCT_CHECK();
        
        m_auction.set_time(t);
        m_auction.m_id = id;
        
        MDPrice ref_price = MDPrice::from_iprice32(ntohl(msg.ref_price));
        MDPrice near_price = MDPrice::from_iprice32(ntohl(msg.near_price));
        MDPrice far_price = MDPrice::from_iprice32(ntohl(msg.far_price));
        
        m_auction.m_ref_price = ref_price.fprice();
        m_auction.m_far_price = far_price.fprice();
        m_auction.m_near_price = near_price.fprice();
        m_auction.m_paired_size = ntohll(msg.paired_shares);
        m_auction.m_imbalance_size = ntohll(msg.imbalance_shares);
        m_auction.m_imbalance_direction = msg.imbalance_direction;
        m_auction.m_indicative_price_flag = msg.cross_type;
        
        if(!m_qbdf_builder) {
          if(m_subscribers) {
            m_subscribers->update_subscribers(m_auction);
          }
        } else {
	  if(!m_exclude_imbalance) {
	    gen_qbdf_imbalance_micros(m_qbdf_builder, id, m_micros_recv_time,
				      m_micros_exch_time_delta, m_exch_char,
				      ref_price, near_price, far_price,
				      m_auction.m_paired_size, m_auction.m_imbalance_size,
				      msg.imbalance_direction, msg.cross_type);
	  }
	}
      }
      break;
    case 'K':
      {
        SIZE_CHECK(tvitch_5::ipo_quoting_period_update);
        const tvitch_5::ipo_quoting_period_update& msg = reinterpret_cast<const tvitch_5::ipo_quoting_period_update&>(*buf);
        PRODUCT_CHECK();
      }
      break;
    case 'L':
      {
        //SIZE_CHECK(tvitch_5::market_participant_position);
      }
      break;
    case 'N':
      {
        //SIZE_CHECK(tvitch_5::retail_price_improvement);
        //const tvitch_5::retail_price_improvement& msg = reinterpret_cast<const tvitch_5::retail_price_improvement&>(*buf);
      }
      break;
    case 'P':
      {
        SIZE_CHECK(tvitch_5::trade_message);
        const tvitch_5::trade_message& msg = reinterpret_cast<const tvitch_5::trade_message&>(*buf);
        PRODUCT_CHECK();
        
        m_trade.set_time(t);
        m_trade.m_id = id;
        
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));
        m_trade.m_price = price.fprice();
        m_trade.m_size = ntohl(msg.shares);
        char side_c = ' ';
        if(!m_trade_has_no_side) {
          side_c = msg.side == 'B' ? 'S' : 'B';
        }
        m_trade.set_side(side_c);
        m_trade.clear_flags();
        m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
        
        if(!m_qbdf_builder) {        
          send_trade_update();
        } else {
          gen_qbdf_trade_micros(m_qbdf_builder, id,
                                m_micros_recv_time, m_micros_exch_time_delta,
                                m_exch_char, side_c, '0', "    ",
                                ntohl(msg.shares), price);
        }
      }
      break;
    case 'Q':
      {
        SIZE_CHECK(tvitch_5::cross_message);
        const tvitch_5::cross_message& msg = reinterpret_cast<const tvitch_5::cross_message&>(*buf);
        PRODUCT_CHECK();
        
        m_trade.set_time(t);
        m_trade.m_id = id;
        m_msu.m_id = id;
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.cross_price));
        m_trade.m_price = price.fprice();
        m_trade.m_size = ntohll(msg.shares);
        m_trade.set_side(0);
        m_trade.clear_flags();
        m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
        
        bool send_msu = true;
        switch(msg.cross_type) {
        case 'O':
          m_trade.m_trade_status = TradeStatus::OpenTrade;
          m_msu.m_market_status = MarketStatus::Open;
          break;
        case 'C':
          m_trade.m_trade_status = TradeStatus::CloseTrade;
          m_msu.m_market_status = MarketStatus::Closed;
          break;
        case 'H':
          m_trade.m_trade_status = TradeStatus::Cross;
          m_msu.m_market_status = MarketStatus::Open;
          break;
        case 'I':
        default:
          m_trade.m_trade_status = TradeStatus::Cross;
          send_msu = false;
        break;
        }
        
        if(!m_qbdf_builder) {
          if(m_provider) {
            m_provider->check_trade(m_trade);
          }
          if(m_subscribers) {
            if(send_msu) {
              m_subscribers->update_subscribers(m_msu);
            }
            m_trade.m_is_clean = true;
            m_trade.m_size_adjustment_to_cum = m_trade.m_size;
            update_subscribers(m_trade);
          }
        } else {
          gen_qbdf_cross_trade_micros(m_qbdf_builder, id,
                                      m_micros_recv_time, m_micros_exch_time_delta,
                                      m_exch_char, ' ', ' ', msg.cross_type,
                                      ntohll(msg.shares), price);
        }
      }
      break;
    case 'R':
      {
        SIZE_CHECK(tvitch_5::stock_directory);
        const tvitch_5::stock_directory& msg = reinterpret_cast<const tvitch_5::stock_directory&>(*buf);
        PRODUCT_CHECK();
      }
      break;
    case 'S':
      {
        SIZE_CHECK(tvitch_5::system_event_message);
        const tvitch_5::system_event_message& msg = reinterpret_cast<const tvitch_5::system_event_message&>(*buf);
        
        if(msg.event_code == 'A') {
          send_alert("%s %s received system event message 'A' -- Emergency Market Condition - Halt", where, m_name.c_str());
        } else if(msg.event_code == 'R') {
          send_alert("%s %s received system event message 'A' -- Emergency Market Condition - Quote Only Period", where, m_name.c_str());
        } else if(msg.event_code == 'B') {
          send_alert("%s %s received system event message 'A' -- Emergency Market Condition - Resume", where, m_name.c_str());
        }
      }
      break;
    case 'U':
      {
        SIZE_CHECK(tvitch_5::order_replace_message);
        const tvitch_5::order_replace_message& msg = reinterpret_cast<const tvitch_5::order_replace_message&>(*buf);
        
        uint64_t orig_order_id = ntohll(msg.orig_order_id);
        uint64_t new_order_id = ntohll(msg.new_order_id);
        uint32_t shares = ntohl(msg.shares);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));
        char    orig_side = '\0';
        
        if(likely(m_order_book)) {
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
            
            MDOrder* orig_order = m_order_book->unlocked_find_order(orig_order_id);
            if(orig_order) {
              if(unlikely(id == -1)) {
                id = orig_order->product_id;
                m_stock_to_product_id[msg.stock_locate] = id;
              }
              orig_side = orig_order->side();
              MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
              inside_changed = m_order_book->unlocked_replace_order_ip(t, orig_order, orig_side,
                                                                       price, shares, new_order_id, false);
            }
          }
          
          if(orig_side) { // Make sure we found the order
            if(!m_qbdf_builder) {
              if(inside_changed) {
                m_quote.set_time(t);
                m_quote.m_id = id;
                send_quote_update();
              }
              if(m_order_book->orderbook_update()) {
                m_order_book->post_orderbook_update();
              }
            } else {
              gen_qbdf_order_replace_micros(m_qbdf_builder, id, orig_order_id,
                                            m_micros_recv_time, m_micros_exch_time_delta,
                                            m_exch_char, orig_side, new_order_id,
                                            shares, price);
            }
          }
        }
      }
      break;
    case 'V':
      {
        SIZE_CHECK(tvitch_5::mwcb_decline_level);
        //const tvitch_5::mwcb_decline_level& msg = reinterpret_cast<const tvitch_5::mwcb_decline_level&>(*buf);
      }
      break;
    case 'W':
      {
        SIZE_CHECK(tvitch_5::mwcb_status);
        const tvitch_5::mwcb_status& msg = reinterpret_cast<const tvitch_5::mwcb_status&>(*buf);
        send_alert("MWCB level %c breached", msg.breached_level);
      }
     case 'X':
      {
        SIZE_CHECK(tvitch_5::order_cancel_message);
        const tvitch_5::order_cancel_message& msg = reinterpret_cast<const tvitch_5::order_cancel_message&>(*buf);
        
        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.canceled_shares);
        
        if(likely(m_order_book)) {
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
            
            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              if(unlikely(id == -1)) {
                id = order->product_id;
                m_stock_to_product_id[msg.stock_locate] = id;
              }
              
              uint32_t new_size = order->size - shares;
              inside_changed = order->is_top_level();
              
              if(m_qbdf_builder) {
		if (!m_exclude_level_2) {
		  if (new_size > 0) {
		    gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id,
						 m_micros_recv_time, m_micros_exch_time_delta,
						 m_exch_char, order->side(), new_size,
						 order->price);
		  } else {
		    gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id,
						 m_micros_recv_time, m_micros_exch_time_delta,
						 m_exch_char, order->side(), 0, MDPrice());
		  }
		}
              }

              MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
              if(new_size > 0) {
                m_order_book->unlocked_modify_order(t, order, new_size);
              } else {
                m_order_book->unlocked_remove_order(t, order);
              }
            }
          }
          
          if(!m_qbdf_builder) {
            if(inside_changed) {
              m_quote.m_id = id;
              m_quote.set_time(t);
              send_quote_update();
            }
            if(m_order_book->orderbook_update()) {
              m_order_book->post_orderbook_update();
            }
          }
        }
      }
      break;
    case 'Y':
      {
        SIZE_CHECK(tvitch_5::reg_sho_restriction);
        const tvitch_5::reg_sho_restriction& msg = reinterpret_cast<const tvitch_5::reg_sho_restriction&>(*buf);

        ProductID id = product_id_of_symbol(msg.stock);
        if(id != Product::invalid_product_id) {
          SSRestriction rest;
          //const char* action;
          switch(msg.reg_sho_action) {
          case '0':
            //action = "No Short Sale Restriction";
            rest=SSRestriction::None;
            m_qbdf_status = QBDFStatusType::NoShortSaleRestrictionInEffect;
            break;
          case '1':
            //action = "Short Sale Restriction Activated";
            rest=SSRestriction::Activated;
            m_qbdf_status = QBDFStatusType::ShortSaleRestrictActivated;
            break;
          case '2':
            //action = "Short Sale Restriction Continued";
            rest=SSRestriction::Continued;
            m_qbdf_status = QBDFStatusType::ShortSaleRestrictContinued;
            break;
          default:
            //action = "Unknown!";
            break;
          }
          
          //const Product& prod = m_app->pm()->find_by_id(id);
          //send_alert(AlertSeverity::LOW, "NASDAQ SSR symbol=%s %c %s", prod.symbol().c_str(), msg.reg_sho_action, action);
          
          if (rest!=SSRestriction::Unknown) {
            if(!m_qbdf_builder) {
              m_msu.m_id = id;
              m_msu.m_ssr=rest;
              m_msu.m_market_status = m_security_status[id];
              if(m_subscribers) {
                m_subscribers->update_subscribers(m_msu);
              }
              m_msu.m_ssr=SSRestriction::Unknown;
            } else {
              m_qbdf_status_reason = string("NASDAQ SSR Action ") + msg.reg_sho_action;
              gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time,
                                     m_micros_exch_time_delta, m_exch_char,
                                     m_qbdf_status.index(), m_qbdf_status_reason);
            }
          }
        }
      }
      break;
    default:
      send_alert("%s %s called with unknown message type '%c'", where, m_name.c_str(), buf[0]);
      break;
    };
    
    if (m_qbdf_builder && m_hist_mode) {
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                             m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }

    return len;
  }

  size_t 
  TVITCH_5_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
  {
    const tvitch_5::downstream_header_t* header = reinterpret_cast<const tvitch_5::downstream_header_t*>(buf);
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    
    log.printf(" seq_num:%lu  message_count:%u\n", seq_num, message_count);
    
    const char* block = buf + sizeof(tvitch_5::downstream_header_t);
    
    for(uint16_t i = 0; i < message_count; ++i) {
      uint16_t message_length = ntohs(reinterpret_cast<const tvitch_5::message_block_t*>(block)->message_length);
      dump_message(log, filter, context, block + 2, len);
      block += 2 + message_length;
    }
    
    return len;
  }
  
  size_t
  TVITCH_5_Handler::dump_message(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
  {
    const tvitch_5::common_message* common = reinterpret_cast<const tvitch_5::common_message*>(buf);
    uint64_t ns = nanos_to_time(common->nanoseconds);
    
    Time_Duration ts(ns);
    
    char time_buf[32];
    Time_Utils::print_time(time_buf, ts, Timestamp::MILI);
    
    log.printf("  %c   timestamp %s", buf[0], time_buf);
    
    
    switch(buf[0]) {
    case 'A':
      {
        const tvitch_5::add_order_no_mpid& msg = reinterpret_cast<const tvitch_5::add_order_no_mpid&>(*buf);
        
        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.shares);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));
        
        log.printf("  add_order order_id:%lu side:%c shares:%u stock:%.8s price:%0.4f",
                   order_id, msg.side, shares, msg.stock, price.fprice());
      }
      break;
    case 'H':
      {
        
      }
      break;
    case 'K':
      {
        const tvitch_5::ipo_quoting_period_update& msg = reinterpret_cast<const tvitch_5::ipo_quoting_period_update&>(*buf);
        
        log.printf("  ipo_quoting_period_update  stock:%.8s", msg.stock);
      }
      break;
    case 'N':
      {
        const tvitch_5::retail_price_improvement& msg = reinterpret_cast<const tvitch_5::retail_price_improvement&>(*buf);
        log.printf("  retail_price_improvement stock:%.8s interest_flag:%c",
                   msg.stock, msg.interest_flag);
      }
      break;
    case 'P':
      {
        const tvitch_5::trade_message& msg = reinterpret_cast<const tvitch_5::trade_message&>(*buf);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));

        log.printf("  trade order_ref_num:%lu  side:%c  shares:%u  stock:%.8s  price:%0.4f  match_number:%lu",
                   ntohll(msg.order_ref_number), msg.side, ntohl(msg.shares), msg.stock, price.fprice(), ntohll(msg.match_number));
        
      }
      break;
    case 'R':
      {
        //const tvitch_5::stock_directory& msg = reinterpret_cast<const tvitch_5::stock_directory&>(*buf);
      }
      break;
    case 'S':
      {
        const tvitch_5::system_event_message& msg = reinterpret_cast<const tvitch_5::system_event_message&>(*buf);
        log.printf("  system_event event_code:%c", msg.event_code);
      }
      break;
    case 'V':
      {
        const tvitch_5::mwcb_decline_level& msg = reinterpret_cast<const tvitch_5::mwcb_decline_level&>(*buf);
        
        IPrice64 l1 = IPrice64::from_iprice64(ntohll(msg.level1));
        IPrice64 l2 = IPrice64::from_iprice64(ntohll(msg.level2));
        IPrice64 l3 = IPrice64::from_iprice64(ntohll(msg.level3));
        
        log.printf("  mwcb_decline_level  level_1=%0.4f  level_2=%0.4f  level_3=%0.4f", l1.fprice(), l2.fprice(), l3.fprice());
      }
      break;
    case 'W':
      {
        const tvitch_5::mwcb_status& msg = reinterpret_cast<const tvitch_5::mwcb_status&>(*buf);
        log.printf("  mwcb_status level %c breached", msg.breached_level);
      }
      break;
    default:
      log.printf("  message_type:%c not supported", buf[0]);
      break;
    }
    log.printf("\n");
    return len;
  }
  
  void
  TVITCH_5_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    if(m_order_book) {
      m_order_book->clear(false);
      if(m_qbdf_builder && m_micros_recv_time > 0) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_char, -1);
      }
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }
  
  void
  TVITCH_5_Handler::reset(size_t context, const char* msg, uint64_t expected_seq_no, uint64_t received_seq_no)
  {
    Handler::reset(context, msg, expected_seq_no, received_seq_no);
  }
  
  
  void
  TVITCH_5_Handler::send_quote_update()
  {
    ProductID id = m_quote.m_id;
    m_order_book->fill_quote_update(id, m_quote);
    m_msu.m_market_status = m_security_status[id];
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);
    if(m_msu.m_market_status != m_security_status[id]) {
      m_msu.m_id = id;
      m_security_status[id] = m_msu.m_market_status;
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
  TVITCH_5_Handler::send_trade_update()
  {
    ProductID id = m_trade.m_id;
    m_msu.m_market_status = m_security_status[id];
    m_feed_rules->apply_trade_conditions(m_trade, m_msu);
    if(m_msu.m_market_status != m_security_status[id]) {
      m_msu.m_id = id;
      m_security_status[id] = m_msu.m_market_status;
      if(m_subscribers) {
        m_subscribers->update_subscribers(m_msu);
      }
    }
    if(m_provider) {
      m_provider->check_trade(m_trade);
    }
    update_subscribers(m_trade);
  }

  void
  TVITCH_5_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);
    m_name = name;
    
    m_midnight = Time::today();
    
    bool build_book = params.getDefaultBool("build_book", true);
    if(build_book) {
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_books.push_back(m_order_book);
      m_order_book->init(params);
    }
    
    string iso_date = Time::current_time().to_iso_date();
    if(iso_date >= "20140714") {
      m_trade_has_no_side = true;
      m_logger->log_printf(Log::INFO, "Date %s >= 20140714, trade_has_no_side enabled", iso_date.c_str());
    } else {
      m_trade_has_no_side = false;
      m_logger->log_printf(Log::INFO, "Date %s <  20140714, trade_has_no_side disabled", iso_date.c_str());
    }
    
    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;

    if (exch == ExchangeID::XBOS) {
      m_exch_char = 'B';
    } else {
      m_exch_char = 'Q';
    }

    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';
    
    m_stock_to_product_id.resize(65536, -1);
    
    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);
    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);
  }

  void
  TVITCH_5_Handler::start()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  TVITCH_5_Handler::stop()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }
  
  void
  TVITCH_5_Handler::send_retransmit_request(channel_info& channel_info)
  {
    if(channel_info.recovery_socket == -1) {
      return;
    }

    uint64_t from_seq_no = channel_info.seq_no;
    uint16_t message_count = channel_info.queue.top().seq_num - from_seq_no;

    send_alert("TVITCH_5_Handler::send_retransmit_request %s from_seq_no=%zd, message_count=%d", m_name.c_str(), from_seq_no, (int) message_count);

    tvitch_5::request_packet_t req;
    
    memcpy(req.session, m_session, 10);
    req.seq_num = htonll(from_seq_no);
    req.message_count = htons(message_count);

    int ret = send(channel_info.recovery_socket, &req, sizeof(req), MSG_DONTWAIT);
    if(ret < 0) {
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert("TVITCH_5_Handler::send_retransmit_request %s sendto returned %d - %s", m_name.c_str(), errno, err_msg);
    }
  }
  
  void
  TVITCH_5_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
                                        const string& mdSymbol_,const string& mdExch_)
  {
    mutex_t::scoped_lock lock(m_mutex);
    
    char symbol[9];
    size_t offset = 0;
    if(mdExch_.empty()) {
      offset = snprintf(symbol, 9, "%s", mdSymbol_.c_str());
    } else {
      offset = snprintf(symbol, 9, "%s.%s", mdSymbol_.c_str(), mdExch_.c_str());
    }
    memset(symbol+offset, ' ', 9 - offset);
    
    m_prod_map.insert8(symbol, id_);
  }

  TVITCH_5_Handler::TVITCH_5_Handler(Application* app) :
    Handler(app, "TVITCH_5"),
    m_prod_map(262000),
    m_micros_exch_time_marker(0),
    m_hist_mode(false)
  {
  }

  TVITCH_5_Handler::~TVITCH_5_Handler()
  {
  }

}
