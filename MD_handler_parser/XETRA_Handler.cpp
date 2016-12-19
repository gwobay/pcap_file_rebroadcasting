#include <Data/QBDF_Util.h>
#include <Data/XETRA_Handler.h>
#include <Util/Network_Utils.h>

#include <Exchange/Exchange_Manager.h>
#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <MD/MDProvider.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>

namespace hf_core {
  
  static FactoryRegistrar1<std::string, Handler, Application*, XETRA_Handler> r1("XETRA_Handler");
  
  namespace xetra {
    
    struct packet_header {
      uint8_t  presence_map;
      uint8_t  template_id;
      uint8_t  sender_comp_id;
      uint8_t  packet_seq_num_len;
      uint32_t packet_seq_num;
      uint8_t  sending_time[6];
      uint8_t  performance_indicator[4];
    } __attribute__ ((packed));
  }
  
  inline
  uint64_t decode_bytevector(const char* vec)
  {
    int len = *vec & 0x7F;
    ++vec;
    uint64_t v = 0;
    for(int i = len; i > 0; --i, ++vec) {
      v = (v << 8) + *(uint8_t*)vec;
    }
    return v;
  }
  

#define LOGV(x)       \
  if(false) {         \
    x;                \
  } 
 
  inline static ProductID product_id_of_symbol(QBDF_Builder* qbdf_builder, hash_map<string, ProductID>& prod_map, string& symbol, LoggerRP& logger)
  {
    if (qbdf_builder) {
      size_t whitespace_posn = symbol.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol = symbol.substr(0, whitespace_posn+1);
      
      ProductID symbol_id = qbdf_builder->process_taq_symbol(symbol.c_str());
      if (symbol_id == 0) {
        //m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
        //                     "XETRA_Handler::product_id_of_symbol", symbol.c_str());
        //m_logger->sync_flush();
        logger->log_printf(Log::WARN, "XETRA: unable to create or find id for symbol %s", symbol.c_str());
        return Product::invalid_product_id;
      }
      
      return symbol_id;
    }
    
    hash_map<string, ProductID>::const_iterator i = prod_map.find(symbol);
    if(i != prod_map.end()) {
      return i->second;
    } else {
      return -1;
    }
  }


  void XETRA_Channel_Handler::send_quote_update(Xetra_Security_Definition& sec_def)
  {
    m_msu.m_market_status = m_security_status[m_quote.m_id];
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);

    LOGV(cout << Time::current_time().to_string() << " isix: " << sec_def.isix << " quote:" << m_quote.m_bid_size << " " << m_quote.m_bid << " " << m_quote.m_ask << " " << m_quote.m_ask_size << endl);

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
    m_handler->update_subscribers(m_quote);
  }

  void XETRA_Channel_Handler::send_trade_update()
  {
    LOGV(cout << Time::current_time().to_string() << "trade price=" << m_trade.m_price << " size=" << m_trade.m_size << endl);
   
    m_msu.m_market_status = m_security_status[m_trade.m_id]; 
    m_feed_rules->apply_trade_conditions(m_trade, m_msu);

    if(m_msu.m_market_status != m_security_status[m_trade.m_id]) {
      m_msu.m_id = m_trade.m_id;
      m_security_status[m_trade.m_id] = m_msu.m_market_status;
      if(m_subscribers) {
        m_subscribers->update_subscribers(m_msu);
      }
    }

    if(m_provider) {
      m_provider->check_trade(m_trade);
    }
    m_handler->update_subscribers(m_trade);
  }

  void XETRA_Channel_Handler::send_auction_update(AuctionUpdate& auctionUpdate)
  {
    auctionUpdate.set_time(Time::current_time());
    if(m_qbdf_builder)
    {
      uint64_t usSinceMidnight = Time::current_time().total_usec() - Time::today().total_usec();
      //gen_qbdf_imbalance(m_qbdf_builder, &auctionUpdate, usSinceMidnight, 'A');
      int32_t exch_time_delta = 0;
      char indicativePriceFlag = ' ';
      bool suppress64 = false;
      MDPrice emptyPrice;
      //cout << Time::current_time().to_string() << " m_ref_price=" << auctionUpdate.m_ref_price << endl;
      gen_qbdf_imbalance_micros(m_qbdf_builder, auctionUpdate.m_id, usSinceMidnight, exch_time_delta, m_qbdf_exch_char,
        emptyPrice, // ref_price
        MDPrice::from_fprice(auctionUpdate.m_near_price),// near_price
        emptyPrice, // far_price
        auctionUpdate.m_paired_size,
        auctionUpdate.m_imbalance_size, auctionUpdate.m_imbalance_direction, indicativePriceFlag, suppress64);
    }
    if(m_subscribers) {
      m_subscribers->update_subscribers(auctionUpdate);
    }
    // ToDo: clear
  }

  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  if(len != sizeof(type)) {                                             \
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), len, sizeof(type)); \
    return len;                                                         \
  }                                                                     \
  
  size_t
  XETRA_Handler::parse(size_t context, const char* buf, size_t len)
  {
    //cout << Time::current_time().to_string() << " parse: context=" << context << endl;
    const char* where = "XETRA_Handler::parse";

    if(len < sizeof(xetra::packet_header)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd", where, m_name.c_str(), len, sizeof(xetra::packet_header));
      return 0;
    }
    
    mutex_t::scoped_lock lock(m_mutex);

    if (m_qbdf_builder) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                             m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }
    
    const xetra::packet_header& header = reinterpret_cast<const xetra::packet_header&>(*buf);
    
    channel_info& ci = m_channel_info_map[context];
    
    uint32_t seq_no = ntohl(header.packet_seq_num);
    uint32_t next_seq_no = ntohl(header.packet_seq_num);    
   
    if(seq_no == 0) {
      parse2(context, buf, len);
      return len;
    }
    
    //if(seq_no > ci.seq_no) {
    // Please note that this datagram sequence number cannot be used to detect data
    // loss on the application level.  The receiver application has to use the sequence
    // numbers inside the messages for this purpose
    //if(ci.begin_recovery(this, buf, len, seq_no, next_seq_no)) {
    //  return len;
    //}
    //} else

// Commenting out 201100123:
/*
    if(next_seq_no <= ci.seq_no) {
      // duplicate
      return len;
    }
*/  
    parse2(context, buf, len);
    ci.seq_no = next_seq_no;
    ci.last_update = Time::currentish_time();
    
    //if(!ci.queue.empty()) {
    //  ci.process_recovery(this);
    //}
    
    return len;
  }
  
  
  size_t
  XETRA_Handler::parse2(size_t context, const char* packet_buf, size_t packet_len)
  {
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, packet_len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(packet_buf, packet_len);
    }
    
    if(m_record_only) {
      return packet_len;
    }
  
    XETRA_Context_Info& context_info = m_context_info[context];
    
    try {
      if(context_info.m_channel_type == XETRA_Channel_Type::INSTRUMENT) {
        handle_instrument(reinterpret_cast<uint8_t*>(const_cast<char*>(packet_buf)), packet_len);
      } else {
        if(!m_finished_handling_instruments) {
          return packet_len;
        }
        context_info.handle(context, packet_buf, packet_len);
      }
    } catch(const std::exception& e) {
      m_logger->log_printf(Log::ERROR, "XETRA_Handler::parse2 Caught exception on context %lu Channel_Type %s  -- %s", context, XETRA_Channel_Type::to_string(context_info.m_channel_type), e.what());
    } catch(...) {
      m_logger->log_printf(Log::ERROR, "XETRA_Handler::parse2 Caught exception on context %lu Channel_Type %s", context, XETRA_Channel_Type::to_string(context_info.m_channel_type));
    }
    
    return packet_len;
  }

  size_t
  XETRA_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* packet_buf, size_t packet_len)
  {
    Xetra_Fast_Decoder fast_decoder;
    
    uint8_t* buf_ptr = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(packet_buf));
    uint8_t* end_ptr = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(packet_buf)) + packet_len;
    
    fast_decoder.Parse_PacketHeader(buf_ptr);
    buf_ptr += 2; // Get past the reset message

    while(buf_ptr < end_ptr) {
      uint32_t template_id = fast_decoder.Get_Template_Id(buf_ptr);
      
      log.printf("\n template_id = %d\n", template_id);
      
      
      switch(template_id) {
      case 2:
        log.printf("Beacon\n");
        buf_ptr = end_ptr;
        break;

      case 6:
        {
          //InsideMarketSnapshotInformation& snapshot = m_fast_decoder.Parse_InsideMarketSnapshotInformation(buf_ptr);
          //uint32_t isix = snapshot.isix();
          

        }
        break;

        
      }
    }
    
    
    return packet_len;
  }

  void
  XETRA_Handler::reset(const char* msg)
  {
    // ToDo:
    /*
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_char, -1);
      }

      m_order_book->clear(false);
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }*/
  }

  void
  XETRA_Handler::reset(size_t context, const char* msg)
  {
    // ToDo:
    /*
    const char* where = "XETRA_Handler::reset";

    if (context >= m_channel_info_map.size()) {
      m_logger->log_printf(Log::ERROR, "%s: Bad context (%ld), skipping reset", where, context);
      return;
    }

    channel_info& ci = m_channel_info_map[context];

    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    if(m_subscribers) {
      for(vector<ProductID>::const_iterator p = ci.clear_products.begin(), p_end = ci.clear_products.end();
          p != p_end; ++p) {
        m_subscribers->post_invalid_quote(*p, m_quote.m_exch);
      }
    }

    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_char, context);
      }

      for(vector<ProductID>::const_iterator p = ci.clear_products.begin(), p_end = ci.clear_products.end();
          p != p_end; ++p) {
        m_order_book->clear(*p, false);
        m_seen_product.reset(*p);
      }
    }

    m_logger->log_printf(Log::WARN, "XETRA_Handler::reset %s called with %zd products", m_name.c_str(), ci.clear_products.size());

    ci.clear_products.clear();
    */
  }

  void
  XETRA_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);

    m_name = name;

    m_midnight = Time::today();

    if (exch == ExchangeID::XETR) {
      m_qbdf_exch_char_global = 'X';
    }else {
      m_logger->log_printf(Log::ERROR, "XETRA_Handler::init: Unrecognized ExchangeID");
      m_qbdf_exch_char_global = ' ';
    }

    bool build_book = params.getDefaultBool("build_book", true);
    if(build_book) {
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_books.push_back(m_order_book);
      m_order_book->init(params);
    }
   
    m_exch = exch;
 
    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules_global = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules_global->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);
  
    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown); 
 
    set_up_channels(params);
  }

  void
  XETRA_Handler::start()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  XETRA_Handler::stop()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  XETRA_Handler::subscribe_product(ProductID id_,ExchangeID exch_, const string& mdSymbol_,const string& mdExch_)
  {
    mutex_t::scoped_lock lock(m_mutex);
    
    string symbol = mdSymbol_;
    if(!mdExch_.empty()) {
      symbol += ".";
      symbol += mdExch_;
    }
    
    m_prod_map_global[symbol] = id_;
  }

  XETRA_Handler::XETRA_Handler(Application* app) :
    Handler(app, "XETRA"),
    m_finished_handling_instruments(false),
    m_received_start_ref_data(false),
    m_instrument_expected_seq_num(1),
    m_instrument_expected_packet_seq_num(0)
  {
  }

  XETRA_Handler::~XETRA_Handler()
  {
  }

  void XETRA_Handler::set_up_channels(const Parameter& params)
  {
    const vector<Parameter>& lines = params["mcast_lines"].getVector();

    m_context_info.resize(lines.size() + 1);
    m_channel_info_map.resize(lines.size() + 1);

    size_t context = 0;
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      //const string& name = (*l)["name"].getString();

      const string& channel_type_str = (*l)["channel_type"].getString();
      int channel_id = (*l)["channel_id"].getInt();

      m_logger->log_printf(Log::INFO, "set_up_channels: mapping context=%d to channel_type=%s channel_id=%d", static_cast<int>(context), channel_type_str.c_str(), channel_id);

      XETRA_Context_Info& context_info = m_context_info[context];
      XETRA_Channel_Handler* channel_handler = register_channel(channel_id);
      XETRA_Channel_Type::Enum channel_type = XETRA_Channel_Type::from_string(channel_type_str);

      context_info.init(channel_handler, &(m_channel_info_map[context]), channel_type); // ToDo: Initialize with shared instrument parameters here

      m_channel_info_map[context].context = context;
      m_channel_info_map[context].init(params);
      ++context;
    }
  }

  XETRA_Channel_Handler::XETRA_Channel_Handler(int channel_id,
                                 boost::shared_ptr<MDOrderBookHandler> order_book,
                                 ExchangeID exch,
                                 ExchangeRuleRP exch_rule,
                                 Feed_Rules_RP feed_rules,
                                 boost::shared_ptr<MDSubscriptionAcceptor> subscribers,
                                 MDProvider*                               provider,
                                 hash_map<string, ProductID>& prod_map,
                                 Xetra_Security_Definition* security_definitions_by_isix,
                                 QBDF_Builder* qbdf_builder,
                                 char qbdf_exch_char,
                                 vector<MarketStatus>& security_status,
                                 LoggerRP logger, XETRA_Handler* handler)
    : m_channel_id(channel_id),
      m_order_book(order_book),
      m_exch_rule(exch_rule),
      m_feed_rules(feed_rules),
      m_subscribers(subscribers),
      m_provider(provider),
      m_prod_map(prod_map),
      m_security_definitions_by_isix(security_definitions_by_isix),
      m_qbdf_builder(qbdf_builder),
      m_qbdf_exch_char(qbdf_exch_char),
      m_security_status(security_status),
      m_logger(logger),
      m_handler(handler)
  {
    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;
    
    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';
  }

  template<typename T>
  void print_book(T& book)
  {
    size_t level = 1;
    for(typename T::levels_t::const_iterator it = book.levels().begin(); it != book.levels().end(); ++it)
    {
      const MDPriceLevel& price_level = *it;
#ifdef CW_DEBUG
      cout << level << ": " << price_level.price_as_double() << " " << price_level.total_shares << endl;
#endif
      ++level;
    }
  }

  uint64_t adjust_to_london_time(uint64_t entry_time)
  {
    return entry_time - (60*60*1000000ull);
  }

  template<typename T>
  void XETRA_Channel_Handler::process_delta(T& delta, Xetra_Security_Definition& sec_def)
  {
    //cout << "process_delta ****************PACKET************************" << endl;
    FAST_Sequence<InsideMarketDeltaInformation_EntriesDepth_Element, 100>& entriesDepth = delta.EntriesDepth();
    bool inside_changed = false;
    Time exch_time = Time::today() + Time_Duration(static_cast<int64_t>(adjust_to_london_time(delta.entryTime())) * 1000);
    bool published_qbdf = false;
    bool published_last_in_group = false; 
    //cout << "entriesDepth.len=" << entriesDepth.len << endl;
    for(size_t i = 0; i < entriesDepth.len; ++i)
    {
      InsideMarketDeltaInformation_EntriesDepth_Element& entry = entriesDepth.entries[i];
      uint32_t entryType = entry.entryType();
      double price = entry.entryPrc();
      double qty = entry.entryQty();
      uint32_t price_level = entry.entryPrcLvl();
      uint32_t action = entry.updateAction();
      uint32_t num_orders = entry.numOrders();
      bool last_in_group = (i == entriesDepth.len - 1);

/*
#ifdef CW_DEBUG
      cout << "********MSG***********" << endl;
#endif
#ifdef CW_DEBUG
      cout << Time::current_time().to_string() << " EntriesDepth[" << i << "]: level=" << price_level << " action=" << action << " entryType=" << entryType << " price=" << price << " qty=" << qty << endl;
#endif
#ifdef CW_DEBUG
      cout << "sec_def.m_num_bids=" << sec_def.m_num_bids << " sec_def.m_num_offers=" << sec_def.m_num_offers << endl;
#endif
#ifdef CW_DEBUG
      cout << "orderBookBidsSize=" <<  m_order_book->book(sec_def.product_id).bid().levels().size() << " orderBookAsksSize=" << m_order_book->book(sec_def.product_id).ask().levels().size() << endl;
#endif
      if(m_order_book->book(sec_def.product_id).bid().levels().size() != sec_def.m_num_bids)
      {
#ifdef CW_DEBUG
        cout << "RRRRRR" << endl;
#endif
      }
      if(m_order_book->book(sec_def.product_id).ask().levels().size() != sec_def.m_num_offers)
      {
#ifdef CW_DEBUG
        cout << "TTTTTT" << endl;
#endif
      }
#ifdef CW_DEBUG
      cout << "sec_Def.max_depth=" << sec_def.max_depth << endl;
#endif
#ifdef CW_DEBUG
      cout << "has_market_bid=" << sec_def.m_has_market_bid << " has_market_offer=" << sec_def.m_has_market_offer << endl;
#endif

#ifdef CW_DEBUG
      cout << "BidDepth=" << m_order_book->book(sec_def.product_id).bid().levels().size() << " AskDepth=" << m_order_book->book(sec_def.product_id).ask().levels().size() << endl;
#endif
#ifdef CW_DEBUG
      cout << "Bids:" << endl;
#endif
      print_book<MDBidBook>(m_order_book->book(sec_def.product_id).bid());
#ifdef CW_DEBUG
      cout << "Asks:" << endl;
#endif
      print_book<MDAskBook>(m_order_book->book(sec_def.product_id).ask());
#ifdef CW_DEBUG
      cout << "Internal bids:" << endl;
#endif
      for(int i = 0; i < sec_def.m_num_bids; ++i)
      {
#ifdef CW_DEBUG
        cout << "price=" << sec_def.m_bids[i].md_price.fprice() << " size=" << sec_def.m_bids[i].size <<  " is_market=" << sec_def.m_bids[i].is_market_order << endl;
#endif
      }
#ifdef CW_DEBUG
      cout << "Internal offers:" << endl;
#endif
      for(int i = 0; i < sec_def.m_num_offers; ++i)
      {
#ifdef CW_DEBUG
        cout << "price=" << sec_def.m_offers[i].md_price.fprice() << " size=" << sec_def.m_offers[i].size << " is_market=" << sec_def.m_offers[i].is_market_order << endl;
#endif
      }
*/

      switch(entryType)
      {
        case 3: // EMPTY_BOOK
          {
            m_logger->log_printf(Log::WARN, "XETRA got empty book for isiex=%d", sec_def.isix);
            clear_book(sec_def, exch_time); 
          }
          break;
        case 1: // ASK  
          {
            switch(action)
            {
              case 1: // CRE (New)
                {
                  MDPrice md_price = MDPrice::from_fprice(price);
                  inside_changed |= m_order_book->add_level_ip(exch_time, sec_def.product_id, 'S', md_price, qty, num_orders);
                  MDAskBook& book = m_order_book->book(sec_def.product_id).ask();
                  if(m_qbdf_builder)
                  {
                    published_qbdf = true;
                    published_last_in_group |= (last_in_group && book.levels().size() <= sec_def.max_depth);
                    qbdf_update_level(sec_def.product_id, 'S', qty, md_price, num_orders, last_in_group && book.levels().size() <= sec_def.max_depth, exch_time);
                  }
                  if(sec_def.m_num_offers >= static_cast<int>(price_level))
                  {
                    memmove(&(sec_def.m_offers[price_level]), &(sec_def.m_offers[price_level - 1]), sizeof(Price_Point) * (sec_def.m_num_offers - price_level + 1));
                  }
                  sec_def.m_offers[price_level - 1].md_price = md_price;
                  sec_def.m_offers[price_level - 1].size = qty;
                  sec_def.m_offers[price_level - 1].is_market_order = false;
                  ++(sec_def.m_num_offers);
                  if(sec_def.m_num_offers > static_cast<int>(sec_def.max_depth)*3) {
                    m_logger->log_printf(Log::WARN, "XETRA Got offer CRE delta with bringing num_offers > max_depth*3 for isix=%d. This is erroneous so we'll get a snapshot", sec_def.isix);
                    clear_book(sec_def, exch_time);
                    sec_def.expected_seq_num = 1;
                    sec_def.needs_snapshot = true;
                  }
                }
                break;
              case 2: // CHG (Change)
                {
                  if(static_cast<int>(price_level) > sec_def.m_num_offers)
                  {
                    m_logger->log_printf(Log::WARN, "XETRA Got offer CHG delta with price_level > num_offers for isix=%d. This is erroneous so we'll get a snapshot", sec_def.isix);
                    sec_def.needs_snapshot = true;
                    return;
                  }
                  if(!sec_def.m_offers[price_level - 1].is_market_order)
                  {
                    MDPrice existing_price = sec_def.m_offers[price_level - 1].md_price;
                    MDPrice new_md_price = MDPrice::from_fprice(price);
                    //cout << "Change/Overlay ask existing_price=" << existing_price.fprice()
                    if(existing_price != new_md_price)
                    {
                      inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, false); // ToDo: last field is "executed" - do we know/care?
                      inside_changed |= m_order_book->add_level_ip(exch_time, sec_def.product_id, 'S', new_md_price, qty, num_orders);
                      if(m_qbdf_builder)
                      {
                        qbdf_update_level(sec_def.product_id, 'S', 0, existing_price, 0, false, exch_time);
                        qbdf_update_level(sec_def.product_id, 'S', qty, new_md_price, num_orders, last_in_group, exch_time);
                        published_qbdf = true;
                        published_last_in_group |= last_in_group;
                      }
                      sec_def.m_offers[price_level - 1].md_price = new_md_price;
                      sec_def.m_offers[price_level - 1].size = qty;
                    }
                    else
                    {
                      inside_changed |= m_order_book->modify_level_ip(exch_time, sec_def.product_id, 'S', existing_price, qty, num_orders, false); // ToDo: "executed" 
                      if(m_qbdf_builder)
                      {
                        qbdf_update_level(sec_def.product_id, 'S', qty, existing_price, num_orders, last_in_group, exch_time);
                        published_qbdf = true;
                        published_last_in_group |= last_in_group;
                      }
                      sec_def.m_offers[price_level - 1].size = qty;
                    }
                  }
                }
                break;
              case 3: // DEL (Delete)
                {
                  if(!sec_def.m_offers[price_level - 1].is_market_order)
                  {
                    MDPrice existing_price = sec_def.m_offers[price_level - 1].md_price;
                    inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, false);
                    if(m_qbdf_builder)
                    {
                      qbdf_update_level(sec_def.product_id, 'S', 0, existing_price, 0, last_in_group, exch_time);
                      published_qbdf = true;
                      published_last_in_group |= last_in_group;
                    }
                  }
                  else
                  {
                    sec_def.m_offers[price_level - 1].is_market_order = false;
                    sec_def.m_has_market_offer = false;
                  }
                  if(sec_def.m_num_offers > static_cast<size_t>(price_level))
                  {
                    memmove(&(sec_def.m_offers[price_level - 1]), &(sec_def.m_offers[price_level]), sizeof(Price_Point) * (sec_def.m_num_offers - price_level));
                  }
                  --(sec_def.m_num_offers);
                }
                break;
              case 4: // DEL_FRM (Delete From. Delete from level X to maximum levels) 
                {
                  size_t original_num_levels = sec_def.m_num_offers;;
                  for(size_t level = original_num_levels; level >= price_level; --level)
                  {
                    if(!sec_def.m_offers[level - 1].is_market_order)
                    {
                      MDPrice existing_price = sec_def.m_offers[level - 1].md_price;
                      inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, false);
                      if(m_qbdf_builder)
                      {
                        qbdf_update_level(sec_def.product_id, 'S', 0, existing_price, 0, last_in_group, exch_time);
                        published_qbdf = true;
                        published_last_in_group |= last_in_group;
                      }
                    }
                    else
                    {
                      sec_def.m_offers[level - 1].is_market_order = false;
                      sec_def.m_has_market_offer = false;
                    }
                    --(sec_def.m_num_offers);
                  }
                }
                break;
              case 5: // DEL_THR ( Delete Through. Delete from level 1 to level X)
                {
                  for(size_t level = 0; level < price_level; ++level)
                  {
                    if(!sec_def.m_offers[level].is_market_order)
                    {
                      MDPrice existing_price = sec_def.m_offers[level].md_price;
                      inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, false); // ToDo: last field is "executed" - do we know/care?
                      if(m_qbdf_builder)
                      {
                        qbdf_update_level(sec_def.product_id, 'S', 0, existing_price, 0, last_in_group, exch_time);
                        published_qbdf = true;
                        published_last_in_group |= last_in_group;
                      }
                    }
                    else
                    {
                      sec_def.m_offers[level].is_market_order = false;
                      sec_def.m_has_market_offer = false;
                    }
                  }
                  size_t num_remaining_levels = sec_def.m_num_offers - price_level;
                  if(static_cast<int>(price_level) < sec_def.m_num_offers)
                  {
                    memmove(&(sec_def.m_offers[0]), &(sec_def.m_offers[price_level]), sizeof(Price_Point) * num_remaining_levels);
                  }
                  sec_def.m_num_offers = num_remaining_levels;
                }
                break;
              default:
                {
                  m_logger->log_printf(Log::WARN, "XETRA process_delta: unrecognized depth action type %d for entry %lu", action, i);
                }
                break;
            }
          }
          break;
        case 2: //  BID
          {
            switch(action)
            {
              case 1: // CRE (New)
                {
                  MDPrice md_price = MDPrice::from_fprice(price);
                  inside_changed |= m_order_book->add_level_ip(exch_time, sec_def.product_id, 'B', md_price, qty, num_orders);
                  MDBidBook& book = m_order_book->book(sec_def.product_id).bid();
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level(sec_def.product_id, 'B', qty, md_price, num_orders, last_in_group && book.levels().size() <= sec_def.max_depth, exch_time);
                    published_qbdf = true;
                    published_last_in_group |= (last_in_group && book.levels().size() <= sec_def.max_depth);
                  }
                  if(sec_def.m_num_bids >= static_cast<int>(price_level))
                  {
                    memmove(&(sec_def.m_bids[price_level]), &(sec_def.m_bids[price_level - 1]), sizeof(Price_Point) * (sec_def.m_num_bids - price_level + 1));
                  }
                  sec_def.m_bids[price_level - 1].md_price = md_price;
                  sec_def.m_bids[price_level - 1].size = qty;
                  sec_def.m_bids[price_level - 1].is_market_order = false;
                  ++(sec_def.m_num_bids);
                  if(sec_def.m_num_bids > static_cast<int>(sec_def.max_depth)*3) {
                    m_logger->log_printf(Log::WARN, "XETRA Got offer CRE delta with bringing num_bids > max_depth*3 for isix=%d. This is erroneous so we'll get a snapshot", sec_def.isix);
                    clear_book(sec_def, exch_time);
                    sec_def.expected_seq_num = 1;
                    sec_def.needs_snapshot = true;
                  }
                }
                break;
              case 2: // CHG (Change)
                {
                  if(static_cast<int>(price_level) > sec_def.m_num_bids)
                  {
                    m_logger->log_printf(Log::WARN, "XETRA Got offer CHG delta with price_level > num_bids for isix=%d. This is erroneous so we'll get a snapshot", sec_def.isix);
                    sec_def.needs_snapshot = true;
                    return;
                  }
                  if(!sec_def.m_bids[price_level - 1].is_market_order)
                  {
                    MDPrice existing_price = sec_def.m_bids[price_level - 1].md_price;
                    MDPrice new_md_price = MDPrice::from_fprice(price);
                    //cout << "Change/Overlay bid existing_price=" << existing_price.fprice()
                    if(existing_price != new_md_price)
                    {
                      inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, false); // ToDo: last field is "executed" - do we know/care?
                      inside_changed |= m_order_book->add_level_ip(exch_time, sec_def.product_id, 'B', new_md_price, qty, num_orders);
                      if(m_qbdf_builder)
                      {
                        qbdf_update_level(sec_def.product_id, 'B', 0, existing_price, 0, false, exch_time);
                        qbdf_update_level(sec_def.product_id, 'B', qty, new_md_price, num_orders, last_in_group, exch_time);
                        published_qbdf = true;
                        published_last_in_group |= last_in_group;
                      }
                      sec_def.m_bids[price_level - 1].md_price = new_md_price;
                      sec_def.m_bids[price_level - 1].size = qty;
                    }
                    else
                    {
                      inside_changed |= m_order_book->modify_level_ip(exch_time, sec_def.product_id, 'B', existing_price, qty, num_orders, false); // ToDo: "executed"
                      if(m_qbdf_builder)
                      {
                        qbdf_update_level(sec_def.product_id, 'B', qty, existing_price, num_orders, last_in_group, exch_time);
                        published_qbdf = true;
                        published_last_in_group |= last_in_group;
                      }
                      sec_def.m_bids[price_level - 1].size = qty;
                    }
                  }
                }
                break;
              case 3: // DEL (Delete)
                {
                  if(!sec_def.m_bids[price_level - 1].is_market_order)
                  {
                    MDPrice existing_price = sec_def.m_bids[price_level - 1].md_price;
                    inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, false);
                    if(m_qbdf_builder)
                    {
                      qbdf_update_level(sec_def.product_id, 'B', 0, existing_price, 0, last_in_group, exch_time);
                      published_qbdf = true;
                      published_last_in_group |= last_in_group;
                    }
                  }
                  else
                  {
                    sec_def.m_bids[price_level - 1].is_market_order = false;
                    sec_def.m_has_market_bid = false;
                  }
                  if(sec_def.m_num_bids > static_cast<int>(price_level))
                  {
                    memmove(&(sec_def.m_bids[price_level - 1]), &(sec_def.m_bids[price_level]), sizeof(Price_Point) * (sec_def.m_num_bids - price_level));
                  }
                  --(sec_def.m_num_bids);
                }
                break;
              case 4: // DEL_FRM (Delete From. Delete from level X to maximum levels)
                {
                  size_t original_num_levels = sec_def.m_num_bids;
                  for(size_t level = original_num_levels; level >= price_level; --level)
                  {
                    if(!sec_def.m_bids[level - 1].is_market_order)
                    {
                      MDPrice existing_price = sec_def.m_bids[level - 1].md_price;
                      inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, false);
                      if(m_qbdf_builder)
                      {
                        qbdf_update_level(sec_def.product_id, 'B', 0, existing_price, 0, last_in_group, exch_time);
                        published_qbdf = true;
                        published_last_in_group |= last_in_group;
                      }
                    }
                    else
                    {
                      sec_def.m_bids[level - 1].is_market_order = false;
                      sec_def.m_has_market_bid = false;
                    }
                    --(sec_def.m_num_bids);
                  }
                }
                break;
              case 5: // DEL_THR ( Delete Through. Delete from level 1 to level X)
                {
                  for(size_t level = 0; level < price_level; ++level)
                  {
                    if(!sec_def.m_bids[level].is_market_order)
                    {
                      MDPrice existing_price = sec_def.m_bids[level].md_price;
                      inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, false); // ToDo: last field is "executed" - do we know/care?
                      if(m_qbdf_builder)
                      {
                        qbdf_update_level(sec_def.product_id, 'B', 0, existing_price, 0, last_in_group, exch_time);
                        published_qbdf = true;
                        published_last_in_group |= last_in_group;
                      }
                    }
                    else
                    {
                      sec_def.m_bids[level].is_market_order = false;
                      sec_def.m_has_market_bid = false;
                    }
                  }
                  size_t num_remaining_levels = sec_def.m_num_bids - price_level;
                  if(static_cast<int>(price_level) < sec_def.m_num_bids)
                  {
                    memmove(&(sec_def.m_bids[0]), &(sec_def.m_bids[price_level]), sizeof(Price_Point) * num_remaining_levels);
                  }
                  sec_def.m_num_bids = num_remaining_levels;
                }
                break;
              default:
                {
                  m_logger->log_printf(Log::WARN, "XETRA process_delta: unrecognized depth action type %d for entry %lu", action, i);
                }
                break;
            }
          }
          break;
        case 23: // ASK_PRC_MARKET:
          {
            switch(action)
            {
              case 1: //NEW
              {
                if(sec_def.m_num_offers > 0)
                {
                  memmove(&(sec_def.m_offers[1]), &(sec_def.m_offers[0]), sizeof(Price_Point) * sec_def.m_num_offers);
                }
                sec_def.m_offers[0].is_market_order = true;
                ++(sec_def.m_num_offers);
                sec_def.m_has_market_offer = true;
              }
              break;
              case 2: //CHG
              {
                //Ignore
              }
              break;
              default:
              {
                m_logger->log_printf(Log::WARN, "XETRA Got unexpected update action %d for entry type %d", action, entryType);
              }
              break;
            }
          }
          break;
        case 24: // BID_PRC_MARKET:
          {
            switch(action)
            {
              case 1: //NEW
              {
                if(sec_def.m_num_bids > 0)
                {
                  memmove(&(sec_def.m_bids[1]), &(sec_def.m_bids[0]), sizeof(Price_Point) * sec_def.m_num_bids);
                }
                sec_def.m_bids[0].is_market_order = true;
                ++(sec_def.m_num_bids);
                sec_def.m_has_market_bid = true;
              }
              break;
              case 2: //CHG
              {
                //Ignore
              }
              break;
              default:
              {
                m_logger->log_printf(Log::WARN, "XETRA Got unexpected update action %d for entry type %d", action, entryType);
              }
              break;
            }
          }
          break;
        default:
          {
            m_logger->log_printf(Log::WARN, "XETRA process_delta: unrecognized depth action type %d for entry %lu", action, i);
          }
          break;
      }
    }
    
    if(sec_def.m_num_bids > 41) {
      sec_def.expected_seq_num = 1;
      throw runtime_error("sec_def.m_num_bids > 41");
    }
    if(sec_def.m_num_offers > 41) {
      sec_def.expected_seq_num = 1;
      throw runtime_error("sec_def.m_num_offers > 41");
    }
    
    // Remove any levels over the max depth
    while(m_order_book->book(sec_def.product_id).bid().levels().size() > sec_def.max_depth)
    {
      MDPrice price_at_level_to_delete = sec_def.m_bids[sec_def.m_num_bids - 1].md_price;
      m_order_book->book(sec_def.product_id).bid().remove_price_level(price_at_level_to_delete);
      if(m_qbdf_builder)
      {
        qbdf_update_level(sec_def.product_id, 'B', 0, price_at_level_to_delete, 0, true, exch_time);
        published_qbdf = true;
        published_last_in_group = true;
      }
      --(sec_def.m_num_bids);
    }
    while(m_order_book->book(sec_def.product_id).ask().levels().size() > sec_def.max_depth)
    {
      MDPrice price_at_level_to_delete = sec_def.m_offers[sec_def.m_num_offers - 1].md_price;
      m_order_book->book(sec_def.product_id).ask().remove_price_level(price_at_level_to_delete);
      if(m_qbdf_builder)
      {
        qbdf_update_level(sec_def.product_id, 'S', 0, price_at_level_to_delete, 0, true, exch_time);
        published_qbdf = true;
        published_last_in_group = true;
      }
      --(sec_def.m_num_offers);
    }

    uint32_t instrStatus = delta.instrStatus();
    handle_state_update(instrStatus, sec_def);

    if(inside_changed)
    {
       m_quote.set_time(exch_time);
       m_quote.m_id = sec_def.product_id;
       m_quote.m_cond = ' '; // ToDo: fill this in, but how do we deal with two different conditions (one for bid one for ask)?
       m_order_book->fill_quote_update(sec_def.product_id, m_quote);
       if(m_quote.m_bid >= m_quote.m_ask)
       {
         m_logger->log_printf(Log::WARN, "XETRA Crossed book for isix=%d bid=%f ask=%f", sec_def.isix, m_quote.m_bid, m_quote.m_ask);
       }
       send_quote_update(sec_def);
    }

    if(published_qbdf && ! published_last_in_group)
    {
      MDPrice zeroPrice = MDPrice::from_iprice64(0);
      Time currentTime = Time::current_time();
      qbdf_update_level(sec_def.product_id, 'B', 0, zeroPrice, 0, true, currentTime);
    }

    FAST_Sequence<InsideMarketDeltaInformation_EntriesTrade_Element, 100>& trades = delta.EntriesTrade();
//    if(trades.len > 0)
//    {
//      cout << Time::current_time().to_string() << " trades.len=" << trades.len << endl;
//    }
    bool tradeGap = delta.trdgapIndicator().has_value && delta.trdgapIndicator().value.value[0] == 'Y';
    if(tradeGap)
    {
      m_logger->log_printf(Log::INFO, "Delta Trade Gap Indicator: '%s'", delta.trdgapIndicator().value.to_string().c_str());
    }
    for(int j = trades.len - 1; j >= 0; --j)
    {
      //cout << "processing trades[" << j << "]" << endl;
      InsideMarketDeltaInformation_EntriesTrade_Element& trade = trades.entries[j];
      uint32_t entryType = trade.entryType();
      switch(entryType)
      {
        // Do we need to treat these two any differently?
        case 26: // TRADE_PRC_LIST
        case 4: // LAST_TRD_PRC
        case 11: // MPO_PRC
        //case 10: // BEST_PRC
          {
            FAST_Sequence<InsideMarketDeltaInformation_EntriesTrade_Element_EntriesTradePrices_Element, 50>& tradePrices = trade.EntriesTradePrices();
            //cout << "Got trade prices. len=" << tradePrices.len << endl;
            for(int i = tradePrices.len - 1; i >= 0; --i)
            {
              InsideMarketDeltaInformation_EntriesTrade_Element_EntriesTradePrices_Element& entry = tradePrices.entries[i];
              char priceTypeCode = entry.priceTypeCod().value[0]; // ToDo: use this to identify open/close trades. 'O' = Opening Auction 'F' = Closing Auction.  There are other auctions too but we probably don't care about those.  V=Volatility interruption in continuous trading.  L=Liquidity Interruption
              double price = entry.entryPrc();
              double qty = entry.entryQty().value;
              uint64_t entryTime = adjust_to_london_time(entry.entryTime());
              Time exch_time = Time::today() + Time_Duration(static_cast<int64_t>(entryTime) * 1000); 
              uint32_t actionCode = entry.actnCod();
              switch(actionCode)
              {
                case 4: // Add
                {
                  uint32_t& last_match_id = entryType == 11 ? sec_def.last_midpoint_match_id_number_seen : sec_def.last_match_id_number_seen;
		  uint32_t tranMtchIdNo = entry.tranMtchIdNo();
                  //cout << "Got trade add price=" << price << " qty=" << qty << endl;
                  //if (entryType == 10) { cout << "BEST_PRC!" << endl; } 
                  //cout << Time::current_time().to_string() << " deltaTrade last_match_id=" << last_match_id << " tranMtchIdNo=" << tranMtchIdNo << " price=" << price << " qty=" << qty << endl;
                  if(last_match_id < tranMtchIdNo)
                  {
                    m_trade.m_id = sec_def.product_id;
                    m_trade.m_price = price;
                    m_trade.m_size = qty;
                    m_trade.set_side(0); // ToDo: Use AggressorSide?
                    //cout << Time::current_time().to_string() << ": priceTypeCode=" << priceTypeCode << endl;
                    //cout << Time::current_time().to_string() << ": Trade  isix=" << sec_def.isix << " tranMtchIdNo=" << entry.tranMtchIdNo() << endl;
                    m_trade.m_trade_qualifier[0] = priceTypeCode;
                    m_trade.set_time(exch_time);
                    send_trade_update();
                    if(m_qbdf_builder)
                    {
                      Time receive_time = Time::current_time();
                      uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
                      uint64_t exch_micros_since_midnight = exch_time.total_usec() - Time::today().total_usec();
                      int32_t micros_exch_time_delta = micros_since_midnight - exch_micros_since_midnight;
                      MDPrice md_trade_price = MDPrice::from_fprice(price);
                      string trade_qualifier_str("    ");
                      trade_qualifier_str[0] = priceTypeCode;
                      gen_qbdf_trade_micros(m_qbdf_builder, sec_def.product_id, micros_since_midnight, micros_exch_time_delta,
                                        m_qbdf_exch_char, ' ', ' ', trade_qualifier_str, qty, md_trade_price);
                    }
                    if(tradeGap)
                    {
                      for(uint32_t m = last_match_id + 1; m < tranMtchIdNo; ++m)
                      {
                        m_logger->log_printf(Log::INFO, "TradeGap for isix=%d; inserting potential missed match id %d", sec_def.isix, m);
                        sec_def.potential_missed_match_ids.insert(m);
                      }
                      tradeGap = false;
                    }
                    last_match_id = tranMtchIdNo;
                  }
                }
                break;
                case 5: // Update
                {
                  // ToDo: apply trade correction
                  m_logger->log_printf(Log::WARN, "XETRA Got trade correction");
                }
                break;
                case 6: // Delete
                {
                  // ToDo: apply trade cancel
                  m_logger->log_printf(Log::WARN, "XETRA Got trade bust");
                }
                break;
                default:
                {
                  m_logger->log_printf(Log::WARN, "XETRA Got unrecognized trade actnCod %d", actionCode);
                }
                break; 
              }
            }
          }
          break;
        case 10: // BEST_PRC
          {
          }
          break;
        default:
          {
            m_logger->log_printf(Log::WARN, "XETRA Got process_delta: unrecognized trade entry type %d for trade", entryType);
          }
          break;
      }
    }

    if(delta.auctionGroup().has_value)
    {
      bool moiInterruption = false;
      if(delta.auctionGroup().value.moiInd().has_value)
      {
        switch(delta.auctionGroup().value.moiInd().value.value[0])
        {
          case 'M':
          case 'X':
            {
              moiInterruption = true;
              // ToDo: notify?
            }
            break;
          case 'P':
            {
              // Potential MOI interruption
            }
            break;
          default:
            {
            }
            break;
        }
      }

      bool volInterruption = false;
      if(delta.auctionGroup().value.volInd().has_value)
      {
        switch(delta.auctionGroup().value.volInd().value.value[0])
        {
          case 'E':
          case 'F':
          case 'V':
          case 'L':
          case 'A':
          case 'I':
          case 'B':
            {
              volInterruption = true;
              // ToDo: notify?
            }
            break;
          case 'P':
            {
              // Potential volatility interruption
            }
            break;
          default:
            {
            }
            break;
        }
      }

      if(moiInterruption == false && volInterruption == false) // Ignore these
      {
        AuctionUpdate& auction_update = sec_def.auction_update;
        FAST_Sequence<InsideMarketDeltaInformation_auctionGroup_Group_EntriesAuction_Element, 50>& entries = delta.auctionGroup().value.EntriesAuction();
        for(size_t i = 0; i < entries.len; ++i)
        {
          InsideMarketDeltaInformation_auctionGroup_Group_EntriesAuction_Element& entry = entries.entries[i];
          LOGV(cout << Time::current_time().to_string() << " got auction entryType=" << entry.entryType() << " entry.entryPrc=" << entry.entryPrc().value << " entry.entryQty=" << entry.entryQty().value << endl);
          switch(entry.entryType())
          {
            case 12: //POTENTIAL_AUCTION_PRICE
              {
                if(entry.entryPrc().has_value)
                {
                  auction_update.m_near_price = entry.entryPrc().value;
                  auction_update.m_paired_size = entry.entryQty().value;
                }
                else
                {
                  auction_update.m_near_price = 0.0;
                }
              }
              break;
            case 15: // SURPLUS_BID
              {
                if(entry.entryQty().has_value)
                {
                  auction_update.m_imbalance_size = entry.entryQty().value;
                  auction_update.m_imbalance_direction = 'B';
                }
                else
                {
                  auction_update.m_imbalance_size = 0;
                  auction_update.m_imbalance_direction = ' ';
                }
              }
              break;
            case 16: // SURPLUS_ASK
              {
                if(entry.entryQty().has_value)
                {
                  auction_update.m_imbalance_size = entry.entryQty().value;
                  auction_update.m_imbalance_direction = 'S';
                }
                else
                {
                  auction_update.m_imbalance_size = 0;
                  auction_update.m_imbalance_direction = ' ';
                }
              }
              break;
            case 14: // MATCHING RANGE BID
              {
                // Not sure how to handle these
              }
              break;
            case 13: // MATCHING RANGE ASK
              {
                
              }
              break;
            case 27: // DELT_PTNL_PRC
              {
                auction_update.m_near_price = 0.0;
                auction_update.m_paired_size = 0;
              }
              break;
            case 28: // DELT_SRP
              {
                auction_update.m_imbalance_size = 0;
                auction_update.m_imbalance_direction = ' ';
              }
              break;
            default:
              {
                m_logger->log_printf(Log::WARN, "Unrecognized auction entry type %d", entry.entryType());
              }
              break;
          }
        }
        send_auction_update(auction_update);
      }
    }
  }

/*
                    (Unknown)("unknown")
                    (Invalid)("invalid")
                    (Open)("open")
                    (Closed)("closed")
                    (Halted)("halted")
                    (Suspended)("suspended")
                    (Auction)("auction")
 * */

  void XETRA_Channel_Handler::handle_state_update(uint32_t state, Xetra_Security_Definition& sec_def)
  {
    MarketStatus ms = m_security_status[sec_def.product_id];
    switch(state)
    {
      case 0: ms = MarketStatus::Unknown; break; // Start
      case 1: ms = MarketStatus::Unknown; break; // Pre Trading
      case 2: ms = MarketStatus::Unknown; break; // Pre-call
      case 3: break; // Crossing period
      case 4: break; //Closing crossing period
      case 5: ms = MarketStatus::Auction; break; // Opening Auction Call
      case 6: ms = MarketStatus::Auction; break; // Intraday Auction Call
      case 7: ms = MarketStatus::Auction; break; // Closing Auction Call
      case 8: break; // End auction call
      case 9: ms = MarketStatus::Auction; break; // Auction Call
      case 10: ms = MarketStatus::Auction; break; // Opening Auction IPO Call
      case 11: ms = MarketStatus::Suspended; break; // Opening Auction IP Freeze
      case 12: ms = MarketStatus::Auction; break; // Intraday Auction IPO Call
      case 13: ms = MarketStatus::Suspended; break; // Intraday Auction IPO Freeze
      case 14: break; // IPO
      case 15: ms = MarketStatus::Suspended; break; // Quote-Driven IPO Freeze
      case 16: break; // Opening Auction Pre-Orderbook Balancing
      case 17: break; // Intra Day Auction Pre-Orderbook Balancing
      case 18: break; // Closing Auction Pre-Orderbook Balancing
      case 19: break; // End-of-day Auction Pre-Orderbook Balancing
      case 20: break; // Pre-Orderbook Balancing of quote driver auction
      case 21: break; // Opening Auction Orderbook Balancing
      case 22: break; // Intra Day Auction Orderbook Balancing
      case 23: break; // Closing Auction Orderbook Balancing
      case 24: break; // End-of-day Auction Orderbook Balancing
      case 25: break; // Orderbook Balancing
      case 26: ms = MarketStatus::Open; break; // Continuous Trading
      case 27: break; // In Between Auctions
      case 28: break; // Post Trading (should we be setting to Closed?)
      case 29: ms = MarketStatus::Closed; break; // End Of Trading
      case 30: ms = MarketStatus::Halted; break; // Halt
      case 31: ms = MarketStatus::Suspended; break; // Suspend
      case 32: ms = MarketStatus::Halted; break; // Volatility Interruption
      case 35: break; // Add 
      case 36: break; // Delete
      case 38: break; // Call Unfreeze
      case 39: break; // Continuous Auction Pre-Call
      case 40: break; // Continuous Auction Call
      case 41: break; // Continuous Auction Freeze
      default: m_logger->log_printf(Log::WARN, "Got unknown state %d", state);
    }
    MarketStatus existing_ms = m_security_status[sec_def.product_id];

    if(ms != existing_ms) 
    {
      m_msu.m_market_status = ms;
      m_msu.m_id = sec_def.product_id;
      m_msu.m_exch = m_quote.m_exch;
      m_security_status[sec_def.product_id] = ms;
      if(m_subscribers) 
      {
        m_subscribers->update_subscribers(m_msu);
      }
      LOGV(cout << Time::current_time().to_string() << " handle_status_update: product=" << sec_def.product_id << " state=" << state << " status->" << ms << endl);
    }   
  }

  size_t XETRA_Channel_Handler::handle_delta(size_t context, uint8_t* buf, size_t len)
  {
    //cout << "handle delta; context=" << context << " packet seq num=" << ntohl(reinterpret_cast<xetra::packet_header&>(*buf).packet_seq_num) << endl;
    uint8_t* buf_ptr = buf;
    uint8_t* end_ptr = buf + len;

    m_fast_decoder.Parse_PacketHeader(buf_ptr);

    //cout << "QQQQQQQ delta packet QQQQQQQQ" << endl;

    buf_ptr += 2; // Get past the reset message
    while(buf_ptr < end_ptr)
    {
      uint32_t template_id = m_fast_decoder.Get_Template_Id(buf_ptr);
      switch(template_id)
      {
        case 2:
          {
            //cout << Time::current_time().to_string() << " Got beacon msg" << endl;
            m_fast_decoder.Reset();
            return len;
          }
          break; 
        case 7:
          {
            InsideMarketDeltaInformation& delta = m_fast_decoder.Parse_InsideMarketDeltaInformation(buf_ptr);
            uint32_t isix = delta.isix();
/*
            if(isix != 1154)
            {
              continue;
            }
*/
            //cout << "Delta: isix=" << isix << " srcId=" << delta.srcId() << " seqNum=" << delta.seqNum() << " expected_seq_num=" << m_security_definitions_by_isix[isix].expected_seq_num << endl;
            LOGV(cout << "delta isix=" << isix << endl);
            LOGV(cout << "delta srcId=" << delta.srcId() << endl);
            LOGV(cout << "delta seqNum=" << delta.seqNum() << endl);
            LOGV(cout << "delta entryTime=" << adjust_to_london_time(delta.entryTime()) << endl);
            if(isix < MAX_XETRA_ISIX)
            {
              Xetra_Security_Definition& sec_def = m_security_definitions_by_isix[isix];
              if(sec_def.product_id != Product::invalid_product_id)
              {
                //cout << "sec_def.delta_message_queue.Size()=" << sec_def.delta_message_queue.Size() << endl;
                LOGV(cout << Time::current_time().to_string() << " Found valid product. isix=" << isix << " needs_snapshot=" << sec_def.needs_snapshot << " exp_seq_num=" << sec_def.expected_seq_num << " delta.seqNum=" << delta.seqNum() << endl);
                // We care about this product
                //cout << Time::current_time().to_string() << " delta.seqNum()=" << delta.seqNum() << endl;
                if(delta.seqNum() == 1 && sec_def.expected_seq_num > 2)
                {
                  m_logger->log_printf(Log::WARN, "XETRA_HANDLER:: Likely sequence number reset. isix=%d seqNum=1 expected=%d. Will get snapshot.", isix, sec_def.expected_seq_num);
                  /*
                  sec_def.needs_snapshot = true;
                  sec_def.expected_seq_num = 0;
                  sec_def.delta_message_queue.Clear();
                  bool garbage = false;
                  sec_def.delta_message_queue.Push(buf, len, template_id, delta.seqNum(), garbage);
*/
                  Time exch_time = Time::current_time();
                  clear_book(sec_def, exch_time);
                  process_delta<InsideMarketDeltaInformation>(delta, sec_def);
                  sec_def.expected_seq_num = 2;
                }
                else if(sec_def.needs_snapshot)
                {
                  bool overflowing = false;
                  sec_def.delta_message_queue.Push(buf, len, template_id, delta.seqNum(), overflowing);
                  if(overflowing)
                  {
                    m_logger->log_printf(Log::INFO, "XETRA_HANDLER::incremental message queue is overflowing for isix %d. Max size=%d", 
                      static_cast<int>(sec_def.isix), static_cast<int>(sec_def.delta_message_queue.MaxSize()));
                  }
                }
                else if((delta.seqNum() > sec_def.expected_seq_num && sec_def.expected_seq_num > 2))
                {
                  //cout << "Gap: delta.seqNum()=" << delta.seqNum() << " sec_def.expected_seq_num=" << sec_def.expected_seq_num << endl;
                  if (sec_def.expected_seq_num > 1)
                  {
                    m_logger->log_printf(Log::WARN, "XETRA_Handler::gap for isix=%d: expecting %d; got %d", sec_def.isix, sec_def.expected_seq_num, delta.seqNum());
                    Time exch_time = Time::current_time();
                    clear_book(sec_def, exch_time);
                  }
                  sec_def.needs_snapshot = true;
                  bool overflowing = false;
                  sec_def.delta_message_queue.Push(buf, len, template_id, delta.seqNum(), overflowing);
                  if(overflowing)
                  {
                    m_logger->log_printf(Log::INFO, "XETRA_HANDLER::incremental message queue is overflowing for isix %d. Max size=%d",
                      static_cast<int>(sec_def.isix), static_cast<int>(sec_def.delta_message_queue.MaxSize()));
                  }
                }
                else if(delta.seqNum() > 20 && delta.seqNum() < sec_def.expected_seq_num - 20)
                {
                m_logger->log_printf(Log::WARN, "Likely sequence number reset. isix=%d seqNum=1 expected=%d. Will get snapshot", isix, sec_def.expected_seq_num);
                  if (sec_def.expected_seq_num > 1)
                  {
                    m_logger->log_printf(Log::WARN, "XETRA_Handler::much lower seq num than expected for isix=%d: expecting %d; got %d", sec_def.isix, sec_def.expected_seq_num, delta.seqNum());
                    Time exch_time = Time::current_time();
                    clear_book(sec_def, exch_time);
                  }
                  sec_def.expected_seq_num = delta.seqNum() - 1;
                  sec_def.needs_snapshot = true;
                  bool overflowing = false;
                  sec_def.delta_message_queue.Push(buf, len, template_id, delta.seqNum(), overflowing);
                  if(overflowing)
                  {
                    m_logger->log_printf(Log::INFO, "XETRA_HANDLER::incremental message queue is overflowing for isix %d. Max size=%d",
                      static_cast<int>(sec_def.isix), static_cast<int>(sec_def.delta_message_queue.MaxSize()));
                  }
                }
                else if(delta.seqNum() == sec_def.expected_seq_num)
                {
                  //cout << "delta.seqNum=" << delta.seqNum() << "exp=" << sec_def.expected_seq_num << endl;
                  process_delta<InsideMarketDeltaInformation>(delta, sec_def);
                  ++(sec_def.expected_seq_num);
                }
                // else it's a duplicate; ignore
              }
              else
              {
                LOGV(cout << "handle_delta: Don't care about isix=" << isix << endl);
              }
            }
            else
            {
              m_logger->log_printf(Log::WARN, "Isix %d exceeds max %d", isix, MAX_XETRA_ISIX);
            }
            m_fast_decoder.Reset();
            if(buf_ptr < end_ptr)
            {
              //cout << Time::current_time().to_string() << " More messages buf_ptr=" << (size_t)buf_ptr << " end_ptr=" << (size_t)end_ptr << endl;
              m_logger->log_printf(Log::INFO, "XETRA: more messages");
              return len;
              
              skip_field_mandatory(buf_ptr); // Skip to the next message
              skip_field_mandatory(buf_ptr);
            }
            //m_fast_decoder.Reset();
            //return len;
          }
          break;
        case 13: 
          {
            InstrumentStateMessage& instrument_state = m_fast_decoder.Parse_InstrumentStateMessage(buf_ptr);
            uint32_t isix = instrument_state.isix();
            if(isix < MAX_XETRA_ISIX)
            {
              Xetra_Security_Definition& sec_def = m_security_definitions_by_isix[isix];
              if(sec_def.product_id != Product::invalid_product_id)
              {
                uint32_t state = instrument_state.state();
                m_logger->log_printf(Log::INFO, "XETRA: Got InstrumentStateMessage isix=%d state=%d", isix, state);

                handle_state_update(state, sec_def);
              }
            }
            m_fast_decoder.Reset();
            if(buf_ptr < end_ptr)
            {
              //cout << Time::current_time().to_string() << " More messages buf_ptr=" << (size_t)buf_ptr << " end_ptr=" << (size_t)end_ptr << endl;
              skip_field_mandatory(buf_ptr); // Skip to the next message
              skip_field_mandatory(buf_ptr);
            }
          }
          break;
        // ToDo: handle instrument state change
        // ToDo: clear auction status when auctions end (open)
        // ToDo: handle system state change
        default:
          {
            m_logger->log_printf(Log::WARN, "XETRA Got handle_delta: unrecognized template_id %d", template_id);
            m_fast_decoder.Reset();
            return len;
          }
      }
    }
    m_fast_decoder.Reset();
    return len;
  }

  void XETRA_Channel_Handler::qbdf_update_level(ProductID product_id, char side, uint32_t shares, MDPrice md_price, uint32_t num_orders, bool last_in_group, Time& exch_time)
  {
    if(m_qbdf_builder)
    {
      LOGV(cout << Time::current_time().to_string() << " last_in_group=" << last_in_group << endl);
      Time receive_time = Time::current_time();
      uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
      //cout << "receive_time=" << receive_time.to_string() << " exch_time=" << exch_time.to_string() << endl;
      int32_t micros_exch_time_delta = exch_time.total_usec() - receive_time.total_usec();
      //cout << "micros_exch_time_delta=" << micros_exch_time_delta << endl;
      gen_qbdf_level_iprice64(m_qbdf_builder, product_id,  micros_since_midnight, micros_exch_time_delta, m_qbdf_exch_char, side,
                                          ' ', ' ', ' ', num_orders, shares, md_price, last_in_group, false);
    }
  }

  void XETRA_Channel_Handler::clear_book(Xetra_Security_Definition& sec_def, Time& exch_time)
  {
    ProductID product_id = sec_def.product_id;
    if(product_id != Product::invalid_product_id)
    {
      m_order_book->clear(product_id, true);
      if(m_qbdf_builder)
      {
        for(int i = 0; i < sec_def.m_num_bids; ++i)
        {
          Price_Point& bid = sec_def.m_bids[i];
          bool last_in_group = (i == (sec_def.m_num_bids - 1)) && (sec_def.m_num_offers == 0);
          qbdf_update_level(product_id, 'B', 0, bid.md_price, 0, last_in_group, exch_time);
        }
        for(int i = 0; i < sec_def.m_num_offers; ++i)
        {
          Price_Point& offer = sec_def.m_offers[i];
          bool last_in_group = (i == (sec_def.m_num_offers - 1));
          qbdf_update_level(product_id, 'S', 0, offer.md_price, 0, last_in_group, exch_time);
        }
      }
      memset(sec_def.m_bids, 0, sizeof(sec_def.m_bids));
      memset(sec_def.m_offers, 0, sizeof(sec_def.m_offers));
      sec_def.m_num_bids = 0;
      sec_def.m_num_offers = 0;
      sec_def.m_has_market_bid = false;
      sec_def.m_has_market_offer = false;
    }
  }

  template<typename T>
  void XETRA_Channel_Handler::show_snapshot(T& msg, Xetra_Security_Definition& sec_def)
  {
#ifdef CW_DEBUG
    cout << Time::current_time().to_string() << " show_snapshot:" << endl;
#endif
    FAST_Sequence<InsideMarketSnapshotInformation_EntriesDepth_Element, 50>& entriesDepth = msg.EntriesDepth();
    for(size_t i = 0; i < entriesDepth.len; ++i)
    {
      InsideMarketSnapshotInformation_EntriesDepth_Element& entry = entriesDepth.entries[i];
      switch(entry.entryType())
      {
        case 1: // ASK
        case 2: // BID
          {
            char side = entry.entryType() == 2 ? 'B' : 'S';
            double price = entry.entryPrc();
            double qty = entry.entryQty();
#ifdef CW_DEBUG
            cout << "side=" << side << " price=" << price << " qty=" << qty << endl;
#endif
          }
          break;
        default:
          {
          }
          break;
      }
    }
  }

  template<typename T>
  void XETRA_Channel_Handler::apply_snapshot(T& msg, Xetra_Security_Definition& sec_def)
  {
    uint64_t entryTime = adjust_to_london_time(msg.entryTime());
    Time quotim = Time::today() + Time_Duration(static_cast<int64_t>(entryTime) * 1000); // 

    clear_book(sec_def, quotim);

    FAST_Sequence<InsideMarketSnapshotInformation_EntriesAtp_Element, 50>& entriesAtp = msg.EntriesAtp(); // ToDo: add in typedefs so we can refer generally to T's entries atp type
    for(size_t i = 0; i < entriesAtp.len; ++i)
    {
      InsideMarketSnapshotInformation_EntriesAtp_Element& entry = entriesAtp.entries[i];
      switch(entry.entryType())
      {
        case 6: //LAST_AUCTION
          {
            // ToDo: process
            //double price = entry.entryPrc();
            //double qty = entry.entryQty();
            //cout << "Snapshot: got last auction trade price=" << price << " qty=" << qty << endl;
          }
          break;
        case 4: //LAST_TRADE
          {
/*
            double price = entry.entryPrc();
            double qty = entry.entryQty();
#ifdef CW_DEBUG
            cout << Time::current_time().to_string() << " Snapshot: got last trade price=" << price << " qty=" << qty << endl;
#endif
            m_trade.m_id = sec_def.product_id;
            m_trade.m_price = price;
            m_trade.m_size = qty;
            m_trade.set_side(0); // ToDo: Use AggressorSide?
            m_trade.m_trade_qualifier[0] = 'C';
            m_trade.set_time(quotim);
            send_trade_update();
            
            if(m_qbdf_builder)
            {
              Time receive_time = Time::current_time();
              uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
              int32_t micros_exch_time_delta = 0;
              MDPrice md_trade_price = MDPrice::from_fprice(price);
              string trade_qualifier_str("    ");
              trade_qualifier_str[0] = m_trade.m_trade_qualifier[0];;
              gen_qbdf_trade_micros(m_qbdf_builder, sec_def.product_id, micros_since_midnight, micros_exch_time_delta,
                                    m_qbdf_exch_char, ' ', ' ', trade_qualifier_str, qty, md_trade_price);
            }
*/
          }
          break;
        case 9: //SUBSCR_PRC
          {
          }
          break;
        case 11: //LAST_MIDPOINT
          {
          }
          break;
        case 10: //LAST_BEST
          {
          }
          break;
        case 25: //BUBA_PRC
          {
          }
          break;
        default:
          {
            m_logger->log_printf(Log::WARN, "XETRA Got apply_snapshot: unrecognized trade_entry %d for entry %lu", entry.entryType(), i);
          }
          break;
      }
    }
    FAST_Sequence<InsideMarketSnapshotInformation_EntriesDepth_Element, 50>& entriesDepth = msg.EntriesDepth();
    bool inside_changed = false;
    bool published_qbdf = false;
    bool published_last_in_group = false;
    //cout << "snapshot entriesDepth.len=" << entriesDepth.len << endl;
    for(size_t i = 0; i < entriesDepth.len; ++i)
    {
      InsideMarketSnapshotInformation_EntriesDepth_Element& entry = entriesDepth.entries[i];
      switch(entry.entryType())
      {
        case 1: // ASK
        case 2: // BID
          {
            char side = entry.entryType() == 2 ? 'B' : 'S';
            double price = entry.entryPrc();
            MDPrice md_price = MDPrice::from_fprice(price);
            double qty = entry.entryQty();
            uint32_t level = entry.entryPrcLvl();
            uint32_t num_orders = entry.numOrders();
            inside_changed |= m_order_book->add_level_ip(quotim, sec_def.product_id, side, md_price, qty, num_orders);
            //cout << Time::current_time().to_string() << " Snapshot: got " << side << " entry price=" << price << " qty=" << qty << endl;
            if(m_qbdf_builder)
            {
              bool last_in_group = (i == entriesDepth.len - 1);
              published_qbdf = true;
              published_last_in_group |= last_in_group;
              qbdf_update_level(sec_def.product_id, side, qty, md_price, num_orders, last_in_group, quotim);
            }
            if(entry.entryType() == 2)
            {
              sec_def.m_bids[level - 1].md_price = md_price;
              sec_def.m_bids[level - 1].size = qty;
              sec_def.m_bids[level - 1].is_market_order = false;
              ++(sec_def.m_num_bids);
            }
            else
            {
              sec_def.m_offers[level - 1].md_price = md_price;
              sec_def.m_offers[level - 1].size = qty;
              sec_def.m_offers[level - 1].is_market_order = false;
              ++(sec_def.m_num_offers);
            }
          }
          break;
        case 24: // BID_PRC_MARKET
          {
            sec_def.m_bids[0].is_market_order = false;
            if(sec_def.m_num_bids > 0)
            {
              memmove(&(sec_def.m_bids[1]), &(sec_def.m_bids[0]), sizeof(Price_Point) * sec_def.m_num_bids);
            }
            ++(sec_def.m_num_bids);
            sec_def.m_bids[0].is_market_order = true;
            sec_def.m_has_market_bid = true;
          }
          break;
        case 23: //ASK_PRC_MARKET
          {
            sec_def.m_offers[0].is_market_order = false;
            if(sec_def.m_num_offers > 0)
            {
              memmove(&(sec_def.m_offers[1]), &(sec_def.m_offers[0]), sizeof(Price_Point) * sec_def.m_num_offers);
            }
            ++(sec_def.m_num_offers);
            sec_def.m_offers[0].is_market_order = true;
            sec_def.m_has_market_offer = true;
          }
     
     break;
        case 0: //EMPTY_BOOK
          {
            m_logger->log_printf(Log::WARN, "XETRA: EMPTY_BOOK in snapshot");
          }
          break;
        default:
          {
            m_logger->log_printf(Log::WARN, "XETRA Got apply_snapshot: unrecognized depth entry %d for entry %lu", entry.entryType(), i);
          }
          break;
      }
    }

    if(inside_changed)
    {
      m_quote.set_time(quotim);
      m_quote.m_id = sec_def.product_id;
      m_quote.m_cond = ' '; // ToDo: fill this in
      m_order_book->fill_quote_update(sec_def.product_id, m_quote);
      if(m_quote.m_bid >= m_quote.m_ask)
      {
        m_logger->log_printf(Log::WARN, "XETRA: (snapshot) Crossed book for isix=%d bid=%f ask=%f", sec_def.isix, m_quote.m_bid, m_quote.m_ask);
      }
      send_quote_update(sec_def);
    }

    if(published_qbdf && ! published_last_in_group)
    {
      MDPrice zeroPrice = MDPrice::from_iprice64(0);
      Time currentTime = Time::current_time();
      qbdf_update_level(sec_def.product_id, 'B', 0, zeroPrice, 0, true, currentTime);
    }

    uint32_t instrStatus = msg.instrStatus();
    handle_state_update(instrStatus, sec_def); 
  }

  size_t XETRA_Channel_Handler::handle_snapshot(size_t context, uint8_t* buf, size_t len)
  {
    //cout << "handle snapshot; context=" << context << endl;
    uint8_t* buf_ptr = buf;
    uint8_t* end_ptr = buf + len;

    m_fast_decoder.Parse_PacketHeader(buf_ptr);

    buf_ptr += 2; // Get past the reset message
    while(buf_ptr < end_ptr)
    {
      uint32_t template_id = m_fast_decoder.Get_Template_Id(buf_ptr);
      switch(template_id)
      { 
        case 6:
          {
            InsideMarketSnapshotInformation& snapshot = m_fast_decoder.Parse_InsideMarketSnapshotInformation(buf_ptr);
            uint32_t isix = snapshot.isix();
/*
            if(isix != 1154)
            {
              m_fast_decoder.Reset();
              if(buf_ptr < end_ptr)
              {
                skip_field_mandatory(buf_ptr);
                skip_field_mandatory(buf_ptr);
              }
              continue;
            }
*/
            LOGV(cout << "snapshot isix=" << isix << endl);
            //cout << Time::current_time().to_string() << " snapshot isix=" << isix << endl;
            if(isix < MAX_XETRA_ISIX)
            {
              Xetra_Security_Definition& sec_def = m_security_definitions_by_isix[isix];
              if(sec_def.product_id != Product::invalid_product_id)
              { 
                // ToDo: check if the last seq num for this snapshot exceeds the actual last seq num we saw.  If it does, note that we missed a message and do the steps below
                uint32_t consolSeqNum = snapshot.consolSeqNum();
                //cout << "consoleSeqNum=" << consolSeqNum << " sec_def.expected_seq_num=" << sec_def.expected_seq_num << endl;
                //show_snapshot<InsideMarketSnapshotInformation>(snapshot, sec_def);
                if(consolSeqNum >= sec_def.expected_seq_num)
                {
                  if(!(sec_def.needs_snapshot))
                  {
                    //cout << "Got snapshot with consolSeqNum=" << consolSeqNum << " >= delta expected seq num=" << sec_def.expected_seq_num << " for isix=" << isix << endl;
                  }
                  // 1. apply the snapshot
                  apply_snapshot<InsideMarketSnapshotInformation>(snapshot, sec_def);
                  // 2. mark this product as not needing snapshot
                  //cout << "Applying snapshot for isix=" << isix << endl;
                  sec_def.needs_snapshot = false;
                  sec_def.expected_seq_num = consolSeqNum + 1;
                  // 3. drain the delta message queue for this symbol
                  m_fast_decoder.Reset();
                  //cout << "Draining queue..." << endl;
                  while(!sec_def.delta_message_queue.Empty())
                  {
                    DeltaMessage* delta_msg = sec_def.delta_message_queue.Front();
                    uint8_t* delta_buf_ptr = delta_msg->buf;
                    delta_buf_ptr += sizeof(xetra::packet_header);
                    delta_buf_ptr += 2; // Get past the reset message
                    switch(delta_msg->template_id)
                    {
                      case 7:
                        {
                          InsideMarketDeltaInformation& delta = m_fast_decoder.Parse_InsideMarketDeltaInformation(delta_buf_ptr);
                          //cout << "Draining delta queue. delta seqNum=" << delta.seqNum() << " expected=" << sec_def.expected_seq_num << endl;
                          if(delta.seqNum() == sec_def.expected_seq_num)
                          {
                            process_delta<InsideMarketDeltaInformation>(delta, sec_def);
                            sec_def.expected_seq_num = delta.seqNum() + 1;
                          }
                          else if(delta.seqNum() > sec_def.expected_seq_num)
                          {
                            m_logger->log_printf(Log::WARN, "XETRA: delta.seqNum() > sec_def.expected_seq_num.  Will have to get snapshots again");
                          }
                          else
                          {
                            //cout << "Ignoring queued delta msg since seqNum < expected" << endl;
                          }
                          m_fast_decoder.Reset();
                        }
                        break;
                      default:
                        {
                            m_logger->log_printf(Log::WARN, "Unrecognized delta message template id=%d found when draining delta msg queue for isix=%d after snapshot", delta_msg->template_id, isix);
                        }
                        break;
                    }
                    sec_def.delta_message_queue.Pop();
                  }
                }
                else if(sec_def.needs_snapshot)
                {
                  //cout << "Need snapshot for isix=" << isix << " but got consolSeqNum=" << consolSeqNum << " < expected_seq_num=" << sec_def.expected_seq_num << endl; 
                }
              }
            }
            else
            {
              m_logger->log_printf(Log::WARN, "got isix %d > MAX_XETRA_ISIX %d", isix, MAX_XETRA_ISIX);
            }
            //if(buf_ptr < end_ptr)
            //{
            //  cout << "More messages buf_ptr=" << (size_t)buf_ptr << " end_ptr=" << (size_t)end_ptr << endl;
            //}
            m_fast_decoder.Reset();
            if(buf_ptr < end_ptr)
            {
              skip_field_mandatory(buf_ptr);
              skip_field_mandatory(buf_ptr);
            }
            //return len;
          }
          break;
        default:
          {
            m_logger->log_printf(Log::WARN, "XETRA Got handle_snapshot: unrecognized template_id %d", template_id);
            m_fast_decoder.Reset();
            return len;
          }
      }
    }
    m_fast_decoder.Reset();
    return len;
  }

  size_t XETRA_Handler::handle_instrument(uint8_t* buf, size_t len)
  {
    uint8_t* buf_ptr = buf;
    uint8_t* end_ptr = buf + len;

    const xetra::packet_header& packet_header = reinterpret_cast<const xetra::packet_header&>(*buf);
    uint32_t packet_seq_num = ntohl(packet_header.packet_seq_num);
    if(packet_seq_num < m_instrument_expected_packet_seq_num)
    {
      //cout << "BBBB packet_header.packet_seq_num=" << packet_seq_num << " m_instrument_expected_packet_seq_num=" << m_instrument_expected_packet_seq_num << endl;
      if(m_instrument_expected_packet_seq_num > 10 &&  packet_seq_num == 1)
      {
        m_logger->log_printf(Log::WARN, "XETRA_Handler::handle_instrument: packet_seq_num = 1");
      }
      else
      {
        return len;
      }
    }
    m_instrument_expected_packet_seq_num = packet_seq_num + 1;

    m_fast_instrument_decoder.Parse_PacketHeader(buf_ptr);

    buf_ptr += 2; // Get past the reset message
    while(buf_ptr < end_ptr)
    {
      uint32_t template_id = m_fast_instrument_decoder.Get_Template_Id(buf_ptr);
      LOGV(cout << "instrument got template id " << template_id << endl);
      switch(template_id)
      { 
        case 3:
          {
            if(m_received_start_ref_data && !m_finished_handling_instruments)
            {
              InstrumentReferenceData& instrument_reference_data = m_fast_instrument_decoder.Parse_InstrumentReferenceData(buf_ptr);
              LOGV(cout << "seqNum=" << instrument_reference_data.seqNum() << endl);
              if(instrument_reference_data.seqNum() != m_instrument_expected_seq_num)
              {
                m_received_start_ref_data = false;
                m_instrument_expected_seq_num = 1;
                m_logger->log_printf(Log::WARN, "Gap on instrument channel. Expecting %d got %d", m_instrument_expected_seq_num, instrument_reference_data.seqNum());
                return len;
                // ToDo: just make sure we don't add any instruments to our maps twice due to gap + reset on instrument channel
              }
              ++m_instrument_expected_seq_num;
              //cout << "buf_ptr=" << (unsigned)buf_ptr << " end_ptr=" << (unsigned)end_ptr << endl;
              LOGV(cout << "srcId=" << instrument_reference_data.srcId() << endl);
              LOGV(cout << "exchId=" << instrument_reference_data.exchId().to_string() << endl);
              LOGV(cout << "instGrp=" << instrument_reference_data.instGrp().to_string() << endl);

              size_t max_depth = 0;
              FAST_Sequence<InstrumentReferenceData_MDFeedTypes_Element, 50>& feedTypes = instrument_reference_data.MDFeedTypes();
              for(size_t i = 0; i < feedTypes.len; ++i)
              {
                InstrumentReferenceData_MDFeedTypes_Element& entry = feedTypes.entries[i];
                FAST_Nullable<uint32_t >& mktDepth = entry.mktDepth();
                if(mktDepth.has_value)
                {
                   if(entry.streamType().value[0] == '2') // Delta stream for EnBS
                   {
                     max_depth = mktDepth.value;
                     LOGV(cout << "max_depth=" << max_depth << endl);
                   }
                }
                LOGV(cout << "port=" << entry.port() << endl);
                LOGV(cout << "inetAddr=" << entry.inetAddr() << endl);
                LOGV(cout << "FeedTypes[" << i << "]: streamType=" << entry.streamType().value[0] << " port=" << entry.port() << " inetAddr=" << entry.inetAddr() << endl);
              }

              FAST_Sequence<InstrumentReferenceData_SecListGroup_Element, 50>& secList = instrument_reference_data.SecListGroup();
              for(size_t i = 0; i < secList.len; ++i)
              {
                InstrumentReferenceData_SecListGroup_Element& secDef = secList.entries[i];
                LOGV(cout << "****** Instrument def ******" << endl);
                LOGV(cout << "  isin=" << secDef.isin().to_string() << endl);
                uint32_t isix = secDef.isix();
		LOGV(cout << "  isix=" << isix << endl);
                LOGV(cout << "  instMnem=" << secDef.instMnem().value.to_string() << endl);
                LOGV(cout << "  instShtNam=" << secDef.instShtNam().value.to_string() << endl);
                LOGV(cout << "  homeMkt=" << secDef.homeMkt().to_string() << endl);
                LOGV(cout << "  refMkt=" << secDef.refMkt().to_string() << endl);
                LOGV(cout << "  reportMarket=" << secDef.reportMarket().to_string() << endl);
                string symbol = secDef.instMnem().value.to_string() + "." + secDef.refMkt().to_string(); // ToDo: do we need to add any suffix or anything?
                LOGV(cout << "  symbol=" << symbol << endl);
                LOGV(cout << endl);
                LOGV(cout << "INST_GRP_TO_CHANNEL:" << instrument_reference_data.instGrp().to_string() << "," << symbol << endl);
                if(isix > MAX_XETRA_ISIX)
                {
                  LOGV(cout << "XETRA_Channel_Handler::handle_instrument: got isix " << isix << " which exceeds MAX_XETRA_ISIX " << MAX_XETRA_ISIX << endl);
                }
                Xetra_Security_Definition& sec_def = m_security_definitions_by_isix[isix];
                if(sec_def.product_id == Product::invalid_product_id) // Don't initialize this twice, which could happen if this channel gaps and we have to reread from the beginning
                {
                  ProductID product_id = product_id_of_symbol(m_qbdf_builder, m_prod_map_global, symbol, m_logger);
                  hash_map<ProductID, uint32_t>::iterator dupIt = m_products_seen.find(product_id);
                  if(dupIt == m_products_seen.end())
                  {
                    m_products_seen[product_id] = isix;
                    if(product_id != Product::invalid_product_id)
                    {
                      LOGV(cout << "product_id=" << product_id << endl);
                      LOGV(cout << "Got valid product id for symbol " << symbol << endl);
                      ExchangeID exchange_id = ExchangeID::XETR;
                      sec_def.Initialize(product_id, max_depth, exchange_id, isix);

                      if(m_qbdf_builder)
                      {
                        Time receive_time = Time::current_time();
                        uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
                        int32_t micros_exch_time_delta = 0;
                        QBDFStatusType qbdf_status = QBDFStatusType::Resume;
                        gen_qbdf_status_micros(m_qbdf_builder, product_id,  micros_since_midnight,
                                   micros_exch_time_delta, m_qbdf_exch_char_global, qbdf_status.index(),
                                   "Processed instrument definition");
                      }
                    }
                  }
                  else
                  {
                    m_logger->log_printf(Log::WARN, "duplicate product for isix %d and %d", isix, dupIt->second);
                  }
                }
              }
            }
            // ToDo: should we actually keep processing here?
            m_fast_instrument_decoder.Reset();
            return len;
          }
          break;
        case 130:
          {
            m_received_start_ref_data = true;
            LOGV(cout << "Received start ref data" << endl);
            m_fast_instrument_decoder.Reset();
            return len;
          }
          break;
        case 131:
          {
            if(m_received_start_ref_data)
            {
              if(!m_finished_handling_instruments)
              {
                m_logger->log_printf(Log::INFO, "Finished handling instrument defs");
                m_finished_handling_instruments = true;
              }
            }
            m_fast_instrument_decoder.Reset();
            return len;
          }
          break;
        case 4: // MaintenanceReferenceData
          {
            m_fast_instrument_decoder.Reset();
            return len;
          }
          break;
        case 128: // Start service
          {
            m_fast_instrument_decoder.Reset();
            return len;
          }
          break;
        case 129: // End service
          {
            m_fast_instrument_decoder.Reset();
            return len;
          }
          break;
        case 132: // Start mtn data
          {
            m_fast_instrument_decoder.Reset();
            return len;
          }
          break;
        case 133: // End mtn data
          {
            m_fast_instrument_decoder.Reset();
            return len;
          }
          break;
        default:
          {
            m_logger->log_printf(Log::WARN, "XETRA Got handle_instrument: unrecognized template_id %d", template_id);
            m_fast_instrument_decoder.Reset();
            return len;
          }
      }
    }
    m_fast_instrument_decoder.Reset();
    return len;
  }

  size_t XETRA_Channel_Handler::handle_trades(size_t context, uint8_t* buf, size_t len)
  {
    uint8_t* buf_ptr = buf;
    uint8_t* end_ptr = buf + len;

    int msgCount = 0;
    m_fast_decoder.Parse_PacketHeader(buf_ptr);
    buf_ptr += 2; // Get past the reset message
    while(buf_ptr < end_ptr)
    {
      ++msgCount;
      uint32_t template_id = m_fast_decoder.Get_Template_Id(buf_ptr);
      switch(template_id)
      {
        case 9:
          {
            while(buf_ptr < end_ptr)
            {
              AllTradePrice& msg = m_fast_decoder.Parse_AllTradePrice(buf_ptr);
              uint32_t isix = msg.isix();
/*
              if(msg.isix() != 1154)
              {
                continue;
              }
*/
              if (isix < MAX_XETRA_ISIX)
              {
                Xetra_Security_Definition& sec_def = m_security_definitions_by_isix[isix];
                if(sec_def.product_id != Product::invalid_product_id) // Don't initialize this twice, which could happen if this channel gaps and we have to reread from the beginning
                { 
                  uint32_t entryType = msg.entryType();

                  switch(entryType)
                  {
                    case 26: // TRADE_PRC_LIST
                    case 4: // LAST_TRD_PRC
                    case 11: // MPO_PRC
                    //case 10: // BEST_PRC
                    {
                    uint32_t tranMtchIdNo = msg.tranMtchIdNo();
                      //cout << Time::current_time().to_string() << " Got AllTrade for isix " << msg.isix() << " seqNum=" << msg.seqNum() << " entryPrc=" << msg.entryPrc() << " entryQty=" << msg.entryQty().value << " entryType=" << msg.entryType() << " tranMtchIdNo=" << tranMtchIdNo << endl;
                      //cout << " tranMtchIdNo=" << msg.tranMtchIdNo() << " priceTypCod=" << msg.priceTypeCod().value[0] << " actnCod=" << msg.actnCod() <<  endl;
                      uint64_t entryTime = adjust_to_london_time(msg.timeStamp());
                      Time exch_time = Time::today() + Time_Duration(static_cast<int64_t>(entryTime) * 1000);                   
   
                      bool applyTrade(false);
                      set<uint32_t>::iterator it = sec_def.potential_missed_match_ids.find(tranMtchIdNo);
                      if(it != sec_def.potential_missed_match_ids.end())
                      {
                        sec_def.potential_missed_match_ids.erase(it);
                        applyTrade = true;
                      }
  
                      uint32_t& last_match_id = entryType == 11 ? sec_def.last_midpoint_match_id_number_seen : sec_def.last_match_id_number_seen;
                      //cout << Time::current_time().to_string() << " AllTrades: last_match_id=" << last_match_id << " tranMtchIdNo=" << tranMtchIdNo << " msgCount=" << msgCount << " price=" << msg.entryPrc() << " qty=" << msg.entryQty().value << " priceTypeCod=" << msg.priceTypeCod().value[0] << endl;
                      if(last_match_id < tranMtchIdNo)
                      {
                        //cout << "Going to apply a missed trade because last_match_id=" << last_match_id << 
                        //        " < tranMtchIdNo=" << tranMtchIdNo << endl;
                        if(msg.actnCod() == 4)
                        {
                          last_match_id = tranMtchIdNo;
                        }
                        applyTrade = true;
                      }
  
                      if(applyTrade)
                      {
                        if(msg.actnCod() == 4) // Add
                        {
                          //cout << "AllTrade: got trade we haven't seen before; processing. isix=" << isix << " tranMtchIdNo=" << tranMtchIdNo << endl;
                          // ToDo: process it like a normal trade 
                          m_trade.m_id = sec_def.product_id;
                          m_trade.m_price = msg.entryPrc();
                          m_trade.m_size = msg.entryQty().value;
                          m_trade.set_side(0); // ToDo: Use AggressorSide?
                          m_trade.m_trade_qualifier[0] = msg.priceTypeCod().value[0];
                          m_trade.set_time(exch_time);
                          send_trade_update();
                          if(m_qbdf_builder)
                          {
                            Time receive_time = Time::current_time();
                            uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
                            uint64_t exch_micros_since_midnight = exch_time.total_usec() - Time::today().total_usec();
                            int32_t micros_exch_time_delta = micros_since_midnight - exch_micros_since_midnight;
                            MDPrice md_trade_price = MDPrice::from_fprice(m_trade.m_price);
                            string trade_qualifier_str("    ");
                            trade_qualifier_str[0] = msg.priceTypeCod().value[0];
                            gen_qbdf_trade_micros(m_qbdf_builder, sec_def.product_id, micros_since_midnight, micros_exch_time_delta,
                                           m_qbdf_exch_char, ' ', ' ', trade_qualifier_str, m_trade.m_size, md_trade_price);
                          }
                        }
                      }
                    }
                    break;
                  case 10: // BEST_PRC
                    {
                    }
                    break;
                  default:
                    {
                      m_logger->log_printf(Log::WARN, "AllTrades: Got unhandled entryType=%d", entryType);
                    }
                    break;
                  }
                }
              }
            }
          }
          break;
        default:
          {
            m_logger->log_printf(Log::WARN, "XETRA AllTrades: unrecognized template_id %d", template_id);
            m_fast_decoder.Reset();
            return len;
          }
          break;
      }
    }
    m_fast_decoder.Reset();
    return len;
  }

}
