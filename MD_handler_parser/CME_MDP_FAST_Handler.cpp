#include <Data/QBDF_Util.h>
#include <Data/CME_MDP_FAST_Handler.h>
#include <Util/Network_Utils.h>

#include <Exchange/Exchange_Manager.h>
#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <MD/MDProvider.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <boost/lexical_cast.hpp>

#include <stdarg.h>

namespace hf_core {

  static FactoryRegistrar1<std::string, Handler, Application*, CME_MDP_FAST_Handler> r1("CME_MDP_FAST_Handler");

  namespace CME_MDP {
    struct preamble {
      uint32_t seq_no;
      uint8_t  channel;
    } __attribute__ ((packed));
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

  static
  uint64_t
  int_of_symbol(const char* symbol, int len)
  {
    uint64_t r = 0;
    for(int i = 0; i < len; ++i) {
      if(symbol[i] == ' ' || symbol[i] == '\0') {
        break;
      }
      r = (r << 7) + symbol[i];
    }
    return r;
  }

  size_t
  CME_MDP_FAST_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "CME_MDP_FAST_Handler::parse";

    if(len < sizeof(CME_MDP::preamble)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd",
                 where, m_name.c_str(), len, sizeof(CME_MDP::preamble));
      return 0;
    }

    mutex_t::scoped_lock lock(m_mutex);

    if (m_qbdf_builder && !m_use_exch_time) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
    }

    channel_info& ci = m_channel_info_map[context];

    const CME_MDP::preamble& header = reinterpret_cast<const CME_MDP::preamble&>(*buf);
    uint32_t seq_no = ntohl(header.seq_no);
    uint32_t next_seq_no = seq_no + 1;

    parse2(context, buf, len);
    ci.seq_no = next_seq_no;
    return len;
  }


  size_t
  CME_MDP_FAST_Handler::parse2(size_t context, const char* buf, size_t len)
  {
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
    }

    CME_MDP_FAST_Context_Info & context_info = m_context_info[context];
    size_t ret = context_info.handle(context, buf, len);
    return ret;
  }


  size_t
  CME_MDP_FAST_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* packet_buf, size_t packet_len)
  {
    return packet_len;
  }

  void
  CME_MDP_FAST_Handler::reset(const char* msg)
  {
// ToDo: implement
/*
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_qbdf_exch_char, -1);
      }

      m_order_book->clear(false);
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
*/
  }


  void
  CME_MDP_FAST_Handler::reset(size_t context, const char* msg)
  {
// ToDo: implement
/*
    const char* where = "CME_MDP_FAST_Handler::reset";

    if (context != 0) {
      m_logger->log_printf(Log::ERROR, "%s: Bad context (%ld), skipping reset", where, context);
      return;
    }

    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    if(m_subscribers) {
      // nothing for now
    }

    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_qbdf_exch_char, context);
      }
    }
*/
  }


  void
  CME_MDP_FAST_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
					  const string& mdSymbol_,const string& mdExch_)
  {
    mutex_t::scoped_lock lock(m_mutex);

    uint64_t s = int_of_symbol(mdSymbol_.c_str(), mdSymbol_.size());

    if(m_prod_map_global.count(s) > 0) {
      m_logger->log_printf(Log::ERROR, "Mapping %s to %zd COLLIDES", mdSymbol_.c_str(), s);
    }

    m_prod_map_global.insert(make_pair(s, id_));
  }

  void CME_MDP_FAST_Handler::set_up_channels(const Parameter& params)
  {
    const vector<Parameter>& lines = params["cme"].getVector();

    m_context_info.resize(lines.size() + 1);
    m_channel_info_map.resize(lines.size() + 1);

    size_t context = 0;
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      //const string& name = (*l)["name"].getString();

      const string& channel_type_str = (*l)["channel_type"].getString();
      int channel_id = (*l)["channel_id"].getInt();

#ifdef CW_DEBUG
      cout << "set_up_channels: mapping context=" << context << " to channel_type=" << channel_type_str << " channel_id=" << channel_id << endl;
#endif

      CME_MDP_FAST_Context_Info& context_info = m_context_info[context];
      CME_MDP_FAST_Channel_Handler* channel_handler = register_channel(channel_id);
      CME_Channel_Type::Enum channel_type = CME_Channel_Type::from_string(channel_type_str);

      context_info.init(channel_handler, &(m_channel_info_map[context]), channel_type, m_record_only);

      m_channel_info_map[context].context = context;
      m_channel_info_map[context].init(params);
      ++context;
    }
  }

  void
  CME_MDP_FAST_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);

    m_name = name;

    if(exch == ExchangeID::XCME)
    {
      m_qbdf_exch_char_global = 'c';
    }
    else if(exch == ExchangeID::XCBT)
    {
      m_qbdf_exch_char_global = 'b';
    }
    else if(exch == ExchangeID::XNYM)
    {
      m_qbdf_exch_char_global = 'n';
    }
    else if(exch == ExchangeID::XCEC)
    {
      m_qbdf_exch_char_global = 'o';
    }
    else if(exch == ExchangeID::XMGE)
    {
      m_qbdf_exch_char_global = 'm';
    }
    else if(exch == ExchangeID::XOCH)
    {
      m_qbdf_exch_char_global = 'h';
    }
    else
    {
      m_qbdf_exch_char_global = 'c';
    }

    bool build_book = params.getDefaultBool("build_book", true);
    if(build_book) {
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_book->init(params);
      m_implied_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_implied_order_book->init(params);
      m_secondary_order_books.push_back(m_implied_order_book);
    }

    m_exch = exch;

    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules_global = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules_global->init(m_app, Time::today().strftime("%Y%m%d"));
 
    //m_security_status_global.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);

    string write_instrument_defs_file = params.getDefaultString("write_instrument_defs_file", "");

    if(write_instrument_defs_file.size() > 0)
    {
      Logger_Config c;
      c.name = write_instrument_defs_file;
      c.filename = write_instrument_defs_file; // ToDo: Do we need to prepend a dir?
      c.truncate = false;

      m_instrument_logger = m_app->lm()->create_logger(c);
      m_instrument_logger->write_header("symbol,base,maturity,display_factor,min_price_increment,unit_of_measure_qty");
    }

    set_up_channels(params);
  }

  void
  CME_MDP_FAST_Handler::start()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
    if(m_implied_order_book) {
      m_implied_order_book->clear(false);
    }
  }

  void
  CME_MDP_FAST_Handler::stop()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
    if(m_implied_order_book) {
      m_implied_order_book->clear(false);
    }
  }

  CME_MDP_FAST_Handler::CME_MDP_FAST_Handler(Application* app) :
    Handler(app, "CME_MDP_FAST"),
    m_hist_mode(false)
  {
  }

  CME_MDP_FAST_Handler::~CME_MDP_FAST_Handler()
  {
/*
    for(hash_map<int, CME_MDP_FAST_Channel_Handler*>::iterator it = m_channel_handlers.begin(); it != m_channel_handlers.end(); ++it)
    {
      delete it->second;
    }
*/
  }

  void CME_MDP_FAST_Channel_Handler::update_quote_status(ProductID product_id, QuoteStatus quote_status, QBDFStatusType qbdf_status, string qbdf_status_reason)
  {
    QuoteUpdate quote_update;
    quote_update.m_id = product_id;
    quote_update.m_exch = m_quote.m_exch;
    quote_update.m_quote_status = quote_status;

    Time t = Time::current_time(); // ToDo: do we need to do this better?  Not sure
    quote_update.set_time(t);
    quote_update.m_cond = ' '; // ToDo: Do we need to do this better once we start handling implieds?
    m_order_book->fill_quote_update(product_id, quote_update);
    if(m_qbdf_builder)
    { 
      Time receive_time = Time::current_time();
      uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
      int32_t micros_exch_time_delta = 0;
      gen_qbdf_status_micros(m_qbdf_builder, product_id,  micros_since_midnight,
                                 micros_exch_time_delta, m_qbdf_exch_char, qbdf_status.index(),
                                 qbdf_status_reason.c_str());
    }
    else
    {
      if(m_subscribers) {
        m_subscribers->update_subscribers(quote_update);
      }
    }
  }

  void CME_MDP_FAST_Channel_Handler::send_quote_update()
  {
    //m_msu.m_market_status = m_security_status[m_quote.m_id];
    m_msu.m_market_status = MarketStatus::Open;
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);

    // ToDo: only do this if we're not caught up (processing snapshots)
    m_quote.m_quote_status = QuoteStatus::Invalid; 

    if(m_provider) {
      m_provider->check_quote(m_quote);
    }
    m_handler->update_subscribers(m_quote);
  }

  void CME_MDP_FAST_Channel_Handler::send_trade_update()
  {
    m_msu.m_market_status = MarketStatus::Open;
    m_feed_rules->apply_trade_conditions(m_trade, m_msu);

    if(m_provider) {
      m_provider->check_trade(m_trade);
    }
    m_handler->update_subscribers(m_trade);
  }

  ProductID CME_MDP_FAST_Channel_Handler::product_id_of_symbol(string symbol_string)
  {
    if (m_qbdf_builder) {
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string.c_str());
      if (symbol_id == 0) {
        //m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
        //                     "CME_MDP_FAST_Handler::product_id_of_symbol", symbol_string.c_str());
        //m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      return symbol_id;
    }

    uint64_t h = int_of_symbol(symbol_string.c_str(), symbol_string.size());
    hash_map<uint64_t, ProductID>::iterator i = m_prod_map.find(h);
    if(i == m_prod_map.end()) {
      return Product::invalid_product_id;
    } else {
      return i->second;
    }
  }

  CME_MDP_FAST_Channel_Handler::CME_MDP_FAST_Channel_Handler(int channel_id, boost::shared_ptr<MDOrderBookHandler> order_book, 
    boost::shared_ptr<MDOrderBookHandler> implied_order_book, ExchangeID exch, ExchangeRuleRP exch_rule, Feed_Rules_RP feed_rules, 
    boost::shared_ptr<MDSubscriptionAcceptor> subscribers, MDProvider* provider, hash_map<uint64_t, ProductID>& prod_map,
    QBDF_Builder* qbdf_builder, char qbdf_exch_char, LoggerRP instrument_logger, CME_MDP_FAST_Handler* handler)
    : m_channel_id(channel_id),
      m_order_book(order_book),
      m_implied_order_book(implied_order_book),
      m_exch_rule(exch_rule), 
      m_feed_rules(feed_rules),
      m_subscribers(subscribers),
      m_provider(provider),
      m_prod_map(prod_map),
      m_qbdf_builder(qbdf_builder),
      m_qbdf_exch_char(qbdf_exch_char),
      m_expected_incremental_seq_num(0),
      m_finished_handling_instruments(false),
      m_last_seq_num_processed(0),
      m_num_symbols_snapshotted(0),
      m_num_valid_products(0),
      m_is_hist(false),
      m_instrument_logger(instrument_logger),
      m_handler(handler)
  {
    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;

    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';
  }

  void CME_MDP_FAST_Channel_Handler::qbdf_update_level(ProductID product_id, char side, uint32_t shares, MDPrice md_price, bool last_in_group, bool is_snapshot, char quote_condition, bool is_executed)
  {
    if(m_qbdf_builder)
    {
      // ToDo: get receive_time and exch_time_delta properly
      Time receive_time = Time::current_time();
      uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
      int32_t micros_exch_time_delta = 0;
      gen_qbdf_level_iprice64(m_qbdf_builder, product_id,  micros_since_midnight, micros_exch_time_delta, m_qbdf_exch_char, side,
                                          ' ', quote_condition, ' ', 1 /*ToDo: num orders*/, shares, md_price, last_in_group, false);
    }
  }

  template<typename T>
  void CME_MDP_FAST_Channel_Handler::handle_snapshot_msg(T& msg, uint32_t seqNum)
  {
/*
    if(msg.LastMsgSeqNumProcessed() < m_expected_incremental_seq_num)
    {
#ifdef CW_DEBUG
      cout << "Ignoring snapshot seq with LastMsgSeqNumProcessed=" << msg.LastMsgSeqNumProcessed() << " because < m_expected_incremental_seq_num=" << m_expected_incremental_seq_num << endl;
#endif
      return;
    }
*/
    //cout << "Snapshot seqNum=" << seqNum << endl;
    //uint64_t sending_time_units = msg.SendingTime();
    Time quotim = Time::current_time(); //  ToDo: figure out units and convert sending_time_units to quotim
    uint32_t security_id = msg.SecurityID();
    hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(security_id);
    bool outright_inside_changed = false;
    bool implied_inside_changed = false;
    if(it != m_security_definitions.end() && it->second.product_id != Product::invalid_product_id)
    {
      //cout << "Snapshot rpt_seq=" << msg.RptSeq() << endl;
      Security_Definition& sec_def = it->second;
      if(msg.RptSeq() <= sec_def.rpt_seq)
      {
        //cout << "Ignoring snapshot since msg.RptSeq=" << msg.RptSeq() << " < sec_def.rpt_seq=" << sec_def.rpt_seq << endl;
        return;
      }
      clear_book(sec_def);
      sec_def.rpt_seq = msg.RptSeq();
      for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
      {
        double price_raw = msg.MDEntries().entries[i].MDEntryPx().value;
        double price = price_raw * sec_def.display_factor;
        MDPrice md_price = MDPrice::from_fprice_precise(price);
        uint32_t size = msg.MDEntries().entries[i].MDEntrySize().value;
        char quoteCondition = msg.MDEntries().entries[i].QuoteCondition().value.value[0];
        uint32_t priceLevel = msg.MDEntries().entries[i].MDPriceLevel().value;

        boost::shared_ptr<MDOrderBookHandler> order_book;
        if(quoteCondition == 'C')
        {
          continue; // Ignore exchange best
        }
        if(quoteCondition == 'K')
        {
          order_book = m_implied_order_book;
        }
        else
        {
          order_book = m_order_book;
        }
        Book_State& book_state = quoteCondition == 'K' ? sec_def.m_implied_state : sec_def.m_outright_state; 
        bool& inside_changed = (quoteCondition == 'K' ? implied_inside_changed : outright_inside_changed); 
        //cout << "Snapshot price=" << price << " priceLevel=" << priceLevel << endl;
        switch(msg.MDEntries().entries[i].MDEntryType().value[0])
        {
          case '0': // Bid
            {
              //cout << "Snapshot bid level " << priceLevel << ": " << size << " @ " << price << endl;
              inside_changed |= order_book->add_level_ip(quotim, sec_def.product_id, 'B', md_price, size, 1); // ToDo: fill in NumberOfOrders (last parameter)?
              book_state.m_bids[priceLevel - 1].md_price = md_price;
              book_state.m_bids[priceLevel - 1].size = size;
              ++(book_state.m_num_bids);
              if(book_state.m_num_bids > 21)
              {
#ifdef CW_DEBUG
                cout << "m_num_bids for security_id=" << security_id << " > 21" << endl;
#endif
              }
            }
            break;
          case '1': // Offer
            {
              //cout << "Snapshot offer level " << priceLevel << ": " << size << " @ " << price << endl;
              inside_changed |= order_book->add_level_ip(quotim, sec_def.product_id, 'S', md_price, size, 1); // ToDo: fill in NumberOfOrders (last parameter)?
              book_state.m_offers[priceLevel - 1].md_price = md_price;
              book_state.m_offers[priceLevel - 1].size = size;
              ++(book_state.m_num_offers);
            }
            break;
          case '2':
            {
              m_trade.m_id = sec_def.product_id;
              m_trade.m_price = price;
              m_trade.m_size = size;
              m_trade.set_side(0); // ToDo: Use AggressorSide?
              m_trade.m_trade_qualifier[0] = ' '; // ToDo: I don't think this comes through in the message, but make sure
              m_trade.set_time(quotim);
              send_trade_update();
            }
          // Ignoring trades for now
          default:
            break;
        }
      }
      //cout << "Channel " << m_channel_id << " security_id=" << security_id << " applying snapshot " << seqNum << endl;
      if(outright_inside_changed)
      {
         m_quote.set_time(quotim);
         m_quote.m_id = sec_def.product_id;
         m_quote.m_cond = ' '; // ToDo: fill this in, but how do we deal with two different conditions (one for bid one for ask)?
         m_order_book->fill_quote_update(sec_def.product_id, m_quote);
         send_quote_update();
      }
/*
#ifdef CW_DEBUG
      cout << "Finished applying snapshot:" << endl;
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
      for(int i = 0; i < sec_def.m_outright_state.m_num_bids; ++i)
      {
#ifdef CW_DEBUG
        cout << "price=" << sec_def.m_outright_state.m_bids[i].md_price.fprice() << " size=" << sec_def.m_outright_state.m_bids[i].size << endl;
#endif
      }
#ifdef CW_DEBUG
      cout << "Internal offers:" << endl;
#endif
      for(int i = 0; i < sec_def.m_outright_state.m_num_offers; ++i)
      {
#ifdef CW_DEBUG
        cout << "price=" << sec_def.m_outright_state.m_offers[i].md_price.fprice() << " size=" << sec_def.m_outright_state.m_offers[i].size << endl;
#endif
      }
#ifdef CW_DEBUG
      cout << "End finished applyting snapshot" << endl;
#endif
*/
      sec_def.needs_snapshot = false;
      while(!sec_def.incremental_message_queue.Empty() && !sec_def.needs_snapshot)
      {
        IncrementalMessage& msg = *(sec_def.incremental_message_queue.Front());
        apply_incremental_update(msg.security_id, msg.update_action, msg.entry_type, msg.price_level, msg.price, msg.size, msg.trade_volume,
                                 msg.trade_condition, msg.quote_condition, msg.last_in_group, msg.rpt_seq, msg.match_event_indicator);
        sec_def.incremental_message_queue.Pop();
      }
      update_quote_status(sec_def.product_id, QuoteStatus::Normal, QBDFStatusType::Open, "Finished recovering");
    }
  }

  size_t CME_MDP_FAST_Channel_Handler::handle_snapshot(size_t context, uint8_t* buf, size_t len)
  {
      const CME_MDP::preamble& header = reinterpret_cast<const CME_MDP::preamble&>(*buf);
      uint32_t seq_no = ntohl(header.seq_no);
      //cout << "Snapshot msg " << seq_no << " on channel " << m_channel_id << endl;
      buf += sizeof(header);
      uint32_t template_id = m_fast_decoder.Get_Template_Id(buf);
      //cout << "Template id=" << template_id << endl;
      switch(template_id)
      {
        case 114:
          {
            MDSnapshotFullRefresh_114& msg = m_fast_decoder.Parse_MDSnapshotFullRefresh_114(buf);
            handle_snapshot_msg<MDSnapshotFullRefresh_114&>(msg, seq_no);
          }
          break;
        case 118:
          {
            MDSnapshotFullRefresh_118& msg = m_fast_decoder.Parse_MDSnapshotFullRefresh_118(buf);
            handle_snapshot_msg<MDSnapshotFullRefresh_118&>(msg, seq_no);
          }
          break;
      }
      //m_fast_decoder.Reset();
    return len;
  }

  void CME_MDP_FAST_Channel_Handler::apply_security_status(uint32_t security_id, uint32_t security_status)
  {
    hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(security_id);
    if(it != m_security_definitions.end() && it->second.product_id != Product::invalid_product_id)
    {
      Security_Definition& sec_def = it->second;
      apply_security_status(sec_def, security_status);
    }
  }

  void CME_MDP_FAST_Channel_Handler::apply_security_status(Security_Definition& sec_def, uint32_t security_status)
  {
    switch(security_status)
    {
      case 2: // Trading halt
       {
       }
       break;
      case 17: // Ready to trade
       {
       }
       break;
      case 18: // Not available for trading
       {
#ifdef CW_DEBUG
         cout << "Got security status=18 ('Not available for trading') - setting expected seq num to 0" << endl;
#endif
         m_expected_incremental_seq_num = 0;
       }
       break;
      case 20: // Unknown or invalid
       {
       }
       break;
      default:
        {
#ifdef CW_DEBUG
         cout << Time::current_time().to_string() << " Unrecognized security status " << security_status << endl;
#endif
       }
       break;
    }
  }

  template<typename T>
  void CME_MDP_FAST_Channel_Handler::handle_security_status(T& msg)
  {
    FAST_Nullable<uint32_t>& security_id_nullable = msg.SecurityID();
    uint32_t security_status = msg.SecurityTradingStatus().value;
    if(security_id_nullable.has_value)
    {
      uint32_t security_id = security_id_nullable.value;
      //uint32_t rpt_seq = extract(msg.RptSeq());
      apply_security_status(security_id, security_status);
    }
    else
    {
      for(hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.begin(); it != m_security_definitions.end(); ++it)
      {
        Security_Definition& sec_def = it->second;
        if(sec_def.product_id != Product::invalid_product_id)
        {
          apply_security_status(sec_def, security_status);
        } 
      }
    }
  }

#define NO_VALUE 9999999
template<typename T>
inline T extract(T field) { return field; }
template<typename T>
inline T extract(FAST_Nullable<T> field) { return field.has_value ? field.value : NO_VALUE; }
template<>
inline uint32_t extract<uint32_t>(FAST_Nullable<uint32_t> field) { return field.has_value ? field.value : NO_VALUE; }

template<typename T>
inline char extract_char_2(T& field) { return ' '; }
template<typename T>
inline char extract_char_2(FAST_Nullable<T>& field) { return ' '; }
template<>
inline char extract_char_2<FAST_String >(FAST_Nullable<FAST_String>& field) { return field.has_value ? field.value.value[0] : ' '; }
template<>
inline char extract_char_2<FAST_String>(FAST_String& field) { return field.value[0]; }

  void CME_MDP_FAST_Channel_Handler::apply_incremental_update(uint32_t security_id, uint32_t update_action, char entry_type, uint32_t price_level, double price, int32_t size, uint32_t tradeVolume, char tradeCondition, char quoteCondition, bool last_in_group, uint32_t rpt_seq, char match_event_indicator)
  {
    hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(security_id);
    if(it == m_security_definitions.end() || it->second.product_id == Product::invalid_product_id)
    {
      return;
    }
    //cout << "Incremental rpt_seq=" << rpt_seq << endl;
    Security_Definition& sec_def = it->second;
    if(rpt_seq <= sec_def.rpt_seq && entry_type != 'J')
    {
      //cout << Time::current_time().to_string() << " Ignoring incremental update since rtp_seq=" << rpt_seq << " < sec_def.rpt_seq=" << sec_def.rpt_seq << endl;
      return;
    }

    if(sec_def.needs_snapshot)
    {
     // cout << "sec_def needs snapshot so queueing" << endl;
      sec_def.incremental_message_queue.Push(security_id, update_action, entry_type, price_level, price, size, tradeVolume,
                                             tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
      return;
    }
    if(rpt_seq > sec_def.rpt_seq + 1)
    {
#ifdef CW_DEBUG
      cout << "Missed message for security_id=" << security_id << " expecting rpt_seq=" << (sec_def.rpt_seq + 1) << " got " << rpt_seq << endl;
#endif
      // Missed a message.  Queue it up
      if(m_is_hist)
      {
        clear_book(sec_def);
      }
      else
      {
        sec_def.needs_snapshot = true;
        sec_def.incremental_message_queue.Push(security_id, update_action, entry_type, price_level, price, size, tradeVolume,
                                               tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
        sec_def.is_trade_event = false;
        return;
      }
    }
    sec_def.rpt_seq = rpt_seq;

    if(quoteCondition == 'C')
    {
      return;
    }

    boost::shared_ptr<MDOrderBookHandler> order_book;
    uint32_t relevant_max_depth(0);
    if(tradeCondition == 'W' || quoteCondition == 'K')
    {
      order_book = m_implied_order_book;
      relevant_max_depth = sec_def.implied_max_depth;
    }
    else
    {
      order_book = m_order_book;
      relevant_max_depth = sec_def.outright_max_depth;
    }
    Book_State& book_state = quoteCondition == 'K' ? sec_def.m_implied_state : sec_def.m_outright_state;
    Time exch_time = Time::current_time();
    // ToDo: handle num orders?

    //cout << "channel " << m_channel_id << " apply_incremental_update: security_id=" << security_id;
/*
    if(security_id == 131539 && quoteCondition != 'K')
    {
#ifdef CW_DEBUG
      cout << Time::current_time().to_string() << " apply_inc: level=" << price_level << " action=" << update_action << " entry_type=" << entry_type << " price=" << price << " size=" << size << " quoteCondition=" << quoteCondition << endl;
#endif
#ifdef CW_DEBUG
      cout << "BidDepth=" << order_book->book(sec_def.product_id).bid().levels().size() << " AskDepth=" << order_book->book(sec_def.product_id).ask().levels().size() << endl;
#endif
#ifdef CW_DEBUG
      cout << "Bids:" << endl;
#endif
      print_book<MDBidBook>(order_book->book(sec_def.product_id).bid());
#ifdef CW_DEBUG
      cout << "Asks:" << endl;
#endif
      print_book<MDAskBook>(order_book->book(sec_def.product_id).ask());
#ifdef CW_DEBUG
      cout << "Internal bids:" << endl;
#endif
      for(int i = 0; i < sec_def.m_outright_state.m_num_bids; ++i)
      {
#ifdef CW_DEBUG
        cout << "price=" << sec_def.m_outright_state.m_bids[i].md_price.fprice() << " size=" << sec_def.m_outright_state.m_bids[i].size << endl;
#endif
      }
#ifdef CW_DEBUG
      cout << "Internal offers:" << endl;
#endif
      for(int i = 0; i < sec_def.m_outright_state.m_num_offers; ++i)
      {
#ifdef CW_DEBUG
        cout << "price=" << sec_def.m_outright_state.m_offers[i].md_price.fprice() << " size=" << sec_def.m_outright_state.m_offers[i].size << endl;
#endif
      }
    }
*/

    bool inside_changed = false;
    switch(update_action)
    {
      case 0: // New
        {
          switch(entry_type)
          {
            case '0': // Bid
              {
                if(price_level <= (book_state.m_num_bids + 1))
                {
                  MDPrice md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                  inside_changed |= order_book->add_level_ip(exch_time, sec_def.product_id, 'B', md_price, size, 1);
                  MDBidBook& book = order_book->book(sec_def.product_id).bid();
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level(sec_def.product_id, 'B', size, md_price, last_in_group && book.levels().size() <= relevant_max_depth, false, quoteCondition, false);
                  }
                  if(price_level == 0 || price_level > 20)
                  {
#ifdef CW_DEBUG
                    cout << "Weird New Bid" << endl;
#endif
                  }
                  if(book_state.m_num_bids >= static_cast<int32_t>(price_level))
                  {
                    memmove(&(book_state.m_bids[price_level]), &(book_state.m_bids[price_level - 1]), sizeof(Price_Point) * (book_state.m_num_bids - price_level + 1));
                  }
                  book_state.m_bids[price_level - 1].md_price = md_price;
                  book_state.m_bids[price_level - 1].size = size;
                  ++(book_state.m_num_bids);
                }
              }
              break;
            case '1': // Offer
              {
                if(price_level <= (book_state.m_num_offers + 1))
                {
                  MDPrice md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                  inside_changed |= order_book->add_level_ip(exch_time, sec_def.product_id, 'S', md_price, size, 1);
                  MDAskBook& book = order_book->book(sec_def.product_id).ask();
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level(sec_def.product_id, 'S', size, md_price, last_in_group && book.levels().size() <= relevant_max_depth, false, quoteCondition, false);
                  }
                  if(price_level == 0 || price_level > 20)
                  {
#ifdef CW_DEBUG
                    cout << "Weird New Offer" << endl;
#endif
                  }
                  if(book_state.m_num_offers >= static_cast<int32_t>(price_level))
                  {
                    memmove(&(book_state.m_offers[price_level]), &(book_state.m_offers[price_level - 1]), sizeof(Price_Point) * (book_state.m_num_offers - price_level + 1));
                  }
                  book_state.m_offers[price_level - 1].md_price = md_price;
                  book_state.m_offers[price_level - 1].size = size;
                  ++(book_state.m_num_offers);
                }
              }
              break;
            case '2': // Trade
              {
                m_trade.m_id = sec_def.product_id;
                m_trade.m_price = price * sec_def.display_factor;
                m_trade.m_size = size;
                m_trade.set_side(0); // ToDo: Use AggressorSide?
                m_trade.m_trade_qualifier[0] = tradeCondition;
                m_trade.set_time(exch_time);
                send_trade_update();
                //cout << "TRADE price=" << m_trade.m_price << " size=" << m_trade.m_size << endl;
                if(m_qbdf_builder)
                {
                  Time receive_time = Time::current_time();
                  uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
                  int32_t micros_exch_time_delta = 0;
                  MDPrice md_trade_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                  string trade_cond_str(1, tradeCondition);
                  //cout << "Publishing trade cond=" << tradeCondition << " str=" << trade_cond_str << endl;
                  gen_qbdf_trade_micros(m_qbdf_builder, sec_def.product_id, micros_since_midnight, micros_exch_time_delta,
                                    m_qbdf_exch_char, ' ', ' ', trade_cond_str, size, md_trade_price, false);
                }
              }
              break;
            case 'J': // Empty the book
              {
#ifdef CW_DEBUG
                cout << "Got an empty the book update" << endl;
#endif
                clear_book(sec_def);
                sec_def.rpt_seq = 0; 
              }
              break;
            case ' ':
              {
#ifdef CW_DEBUG
                cout << "Got NO_VALUE entry type" << endl;
#endif
              }
              break;
            case '4':
              break;
            case '6':
              {

                m_trade.m_id = sec_def.product_id;
                m_trade.m_price = price * sec_def.display_factor;
                m_trade.m_size = 0;
                m_trade.set_side(0); // ToDo: Use AggressorSide?
                m_trade.m_trade_qualifier[0] = 'S';
                m_trade.set_time(exch_time);
                send_trade_update();
                //cout << "TRADE price=" << m_trade.m_price << " size=" << m_trade.m_size << endl;
                if(m_qbdf_builder)
                {
                  Time receive_time = Time::current_time();
                  uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
                  int32_t micros_exch_time_delta = 0;
                  MDPrice md_trade_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                  string trade_cond_str(1, 'S');
                  //cout << "Publishing trade cond=" << tradeCondition << " str=" << trade_cond_str << endl;
                  gen_qbdf_trade_micros(m_qbdf_builder, sec_def.product_id, micros_since_midnight, micros_exch_time_delta,
                                    m_qbdf_exch_char, ' ', ' ', trade_cond_str, 0, md_trade_price, false);
                }

              }
              break;
            case '7':
            case '8':
            case 'B':
            case 'C':
            case 'E':
            case 'F':
            case 'N':
            case 'O':
            case 'W':
            case 'X':
              break;
            default: // 4=Opening price, 6=Settlement price, 7=Trading session high, price 8=Trading session low price, B=Trading volume, C=Open Inerest,
                     // E=Simulated Sell, F=Simulated Buy, N=Session high bid, O=Session low offer, W=Fixing price, X=Cash note
              {
#ifdef CW_DEBUG
                cout << "Got unknown entry type " << entry_type << " for action=" << update_action << endl;
#endif
              }
              break;
          }
        }
        break;
      case 1: // Change
      case 3: // Overlay
        {
          switch(entry_type)
          {
            case '0': // Bid
              {
                if(book_state.m_num_bids >= static_cast<int>(price_level)) 
                {
                  MDPrice existing_price = book_state.m_bids[price_level - 1].md_price;
                  MDPrice new_md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                  //cout << "Change/Overlay bid existing_price=" << existing_price.fprice()
                  //     << " new_md_price=" << new_md_price.fprice() << endl;
                  if(price_level <= 0 || price_level > 20)
                  {
#ifdef CW_DEBUG
                    cout << "Weird Change/Overlay Bid" << endl;
#endif
                  }
                  if(existing_price != new_md_price)
                  {
                    //cout << "AAAAAA" << endl;
                    inside_changed |= order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, false); // ToDo: last field is "executed" - do we know/care?
                    inside_changed |= order_book->add_level_ip(exch_time, sec_def.product_id, 'B', new_md_price, size, 1);
                    if(m_qbdf_builder)
                    {
                      qbdf_update_level(sec_def.product_id, 'B', 0, existing_price, false, false, quoteCondition, sec_def.is_trade_event);
                      qbdf_update_level(sec_def.product_id, 'B', size, new_md_price, last_in_group, false, quoteCondition, sec_def.is_trade_event);
                    }
                    book_state.m_bids[price_level - 1].md_price = new_md_price;
                    book_state.m_bids[price_level - 1].size = size;
                  }
                  else
                  {
                    //cout << "BBBBBB" << endl;
                    inside_changed |= order_book->modify_level_ip(exch_time, sec_def.product_id, 'B', existing_price, size, 1 /*ToDo: num_orders*/, false); // ToDo: "executed"
                    if(m_qbdf_builder)
                    {
                      qbdf_update_level(sec_def.product_id, 'B', size, existing_price, last_in_group, false, quoteCondition, sec_def.is_trade_event);
                    }
                    book_state.m_bids[price_level - 1].size = size;
                  }
                }
              }
              break;
            case '1': // Offer
              {
                if(book_state.m_num_offers >= static_cast<int>(price_level))
                {
                  MDPrice existing_price = book_state.m_offers[price_level - 1].md_price;
                  MDPrice new_md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                  if(price_level <= 0 || price_level > 20)
                  {
#ifdef CW_DEBUG
                    cout << "Weird Change/Overlay Offer" << endl;
#endif
                  }
                  if(existing_price != new_md_price)
                  {
                    inside_changed |= order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, false); // ToDo: last field is "executed" - do we know/care?
                    inside_changed |= order_book->add_level_ip(exch_time, sec_def.product_id, 'S', new_md_price, size, 1);
                    if(m_qbdf_builder)
                    {
                      qbdf_update_level(sec_def.product_id, 'S', 0, existing_price, false, false, quoteCondition, sec_def.is_trade_event);
                      qbdf_update_level(sec_def.product_id, 'S', size, new_md_price, last_in_group, false, quoteCondition, sec_def.is_trade_event);
                    }
                    book_state.m_offers[price_level - 1].md_price = new_md_price;
                    book_state.m_offers[price_level - 1].size = size;
                  }
                  else
                  {
                    inside_changed |= order_book->modify_level_ip(exch_time, sec_def.product_id, 'S', existing_price, size, 1 /*ToDo: num_orders*/, false); // ToDo: "executed"
                    if(m_qbdf_builder)
                    {
                      qbdf_update_level(sec_def.product_id, 'S', size, existing_price, last_in_group, false, quoteCondition, sec_def.is_trade_event);
                    }
                    book_state.m_offers[price_level - 1].size = size;
                  }
                }
              }
              break;
            case '2': // Trade
              {
#ifdef CW_DEBUG
                cout << "Got change/overlay trade update_action=" << update_action << endl; 
#endif
              }
              break;
            case 'J': // Empty the book
              {
#ifdef CW_DEBUG
                cout << "Got an empty the book update" << endl;
#endif
                clear_book(sec_def);
                sec_def.rpt_seq = 0; 
              }
              break;
            case ' ':
              {
#ifdef CW_DEBUG
                cout << "Got NO_VALUE entry type" << endl;
#endif
              }
              break;
            case '4':
            case '6':
            case '7':
            case '8':
            case 'B':
            case 'C':
            case 'E':
            case 'F':
            case 'N':
            case 'O':
            case 'W':
            case 'X':
              break;
            default: // 4=Opening price, 6=Settlement price, 7=Trading session high, price 8=Trading session low price, B=Trading volume, C=Open Inerest,
                     // E=Simulated Sell, F=Simulated Buy, N=Session high bid, O=Session low offer, W=Fixing price, X=Cash note
              {
#ifdef CW_DEBUG
                cout << "Got unknown entry type " << entry_type << " for action=" << update_action << endl;
#endif
              }
              break;
          }
        }
        break;
      case 2: // Delete
        {
          switch(entry_type)
          {
            case '0': // Bid
              {
                MDPrice existing_price = book_state.m_bids[price_level - 1].md_price;
                if(price_level == 0 || price_level > 20)
                {
#ifdef CW_DEBUG
                  cout << "Weird Delete Bid" << endl;
#endif
                }
                if(book_state.m_num_bids > static_cast<int32_t>(price_level))
                {
                  memmove(&(book_state.m_bids[price_level - 1]), &(book_state.m_bids[price_level]), sizeof(Price_Point) * (book_state.m_num_bids - price_level));
                
                  if(existing_price.iprice64() != 0)
                  {
                    inside_changed |= order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, false); // ToDo: last field is "executed" - do we know/care?
                    if(m_qbdf_builder)
                    {
                      qbdf_update_level(sec_def.product_id, 'B', 0, existing_price, last_in_group, false, quoteCondition, sec_def.is_trade_event);
                    }
                  }
                  --(book_state.m_num_bids);
                }
              }
              break;
            case '1': // Offer
              {
                MDPrice existing_price = book_state.m_offers[price_level - 1].md_price;
                if(price_level == 0 || price_level > 20)
                {
#ifdef CW_DEBUG
                  cout << "Weird Delete Offer" << endl;
#endif
                }
                if(book_state.m_num_offers > static_cast<int32_t>(price_level))
                {
                  memmove(&(book_state.m_offers[price_level - 1]), &(book_state.m_offers[price_level]), sizeof(Price_Point) * (book_state.m_num_offers - price_level));
                  if(existing_price.iprice64() != 0)
                  {
                    inside_changed |= order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, false); // ToDo: last field is "executed" - do we know/care?
                    if(m_qbdf_builder)
                    {
                      qbdf_update_level(sec_def.product_id, 'S', 0, existing_price, last_in_group, false, quoteCondition, sec_def.is_trade_event);
                    }
                  }
                  --(book_state.m_num_offers);
                }
              }
              break;
            case '2': // Trade
              {
#ifdef CW_DEBUG
                cout << "Got delete trade" << endl;
#endif
              }
              break;
            case 'J': // Empty the book
              {
#ifdef CW_DEBUG
                cout << "Got an 'empty the book' message (J) for product id=" << sec_def.product_id << "; clearing book" << endl;
#endif
                clear_book(sec_def);
                sec_def.rpt_seq = 0; 
              }
              break;
            case ' ':
              {
#ifdef CW_DEBUG
                cout << "Got NO_VALUE entry type" << endl;
#endif
              }
              break;
            case '4':
            case '6':
            case '7':
            case '8':
            case 'B':
            case 'C':
            case 'E':
            case 'F':
            case 'N':
            case 'O':
            case 'W':
            case 'X':
              break;
            default: // 4=Opening price, 6=Settlement price, 7=Trading session high, price 8=Trading session low price, B=Trading volume, C=Open Inerest,
                     // E=Simulated Sell, F=Simulated Buy, N=Session high bid, O=Session low offer, W=Fixing price, X=Cash note
              {
#ifdef CW_DEBUG
                cout << "Got unknown entry type " << entry_type << " for action=" << update_action << endl;
#endif
              }
              break;
          }
        }
        break;
      case NO_VALUE:
        {
#ifdef CW_DEBUG
          cout << "Got update_action=NO_VALUE" << endl;
#endif
          // Is this possible?
        }
        break;
      default:
        {
#ifdef CW_DEBUG
          cout << "Got unrecognized update_action " << update_action << endl;
#endif
        }
        break;
    }
    if(book_state.m_num_bids < 0 || book_state.m_num_bids > 20)
    {
#ifdef CW_DEBUG
      cout << "m_num_bids=" << book_state.m_num_bids << " rpt_seq=" << rpt_seq << endl;
#endif
    }
    if(book_state.m_num_offers < 0 || book_state.m_num_offers > 20)
    {
#ifdef CW_DEBUG
      cout << "m_num_offers=" << book_state.m_num_offers << " rpt_seq=" << rpt_seq << " security_id=" << security_id << endl;
#endif
    }
    while(book_state.m_num_bids > static_cast<int32_t>(relevant_max_depth))
    {
      MDPrice price_at_level_to_delete = book_state.m_bids[book_state.m_num_bids - 1].md_price;
      order_book->book(sec_def.product_id).bid().remove_price_level(price_at_level_to_delete);
      if(m_qbdf_builder)
      {
        qbdf_update_level(sec_def.product_id, 'B', 0, price_at_level_to_delete, true, false, quoteCondition, sec_def.is_trade_event);
      }
      --(book_state.m_num_bids);
    }
    while(book_state.m_num_offers > static_cast<int32_t>(relevant_max_depth))
    {
      MDPrice price_at_level_to_delete = book_state.m_offers[book_state.m_num_offers - 1].md_price;
      order_book->book(sec_def.product_id).ask().remove_price_level(price_at_level_to_delete);
      if(m_qbdf_builder)
      {
        qbdf_update_level(sec_def.product_id, 'S', 0, price_at_level_to_delete, true, false, quoteCondition, sec_def.is_trade_event);
      }
      --(book_state.m_num_offers);
    }
    if(inside_changed && quoteCondition != 'K')
    {
       m_quote.set_time(exch_time);
       m_quote.m_id = sec_def.product_id;
       m_quote.m_cond = ' '; // ToDo: fill this in, but how do we deal with two different conditions (one for bid one for ask)?
       order_book->fill_quote_update(sec_def.product_id, m_quote);
       send_quote_update();
    }
    if(book_state.m_num_bids < 0 || book_state.m_num_bids > 20)
    {
#ifdef CW_DEBUG
      cout << "m_num_bids=" << book_state.m_num_bids << " rpt_seq=" << rpt_seq << endl;
#endif
    }
    if(book_state.m_num_offers < 0 || book_state.m_num_offers > 20)
    {
#ifdef CW_DEBUG
      cout << "m_num_offers=" << book_state.m_num_offers << " rpt_seq=" << rpt_seq << endl;
#endif
    }
    
    if(match_event_indicator == '2' || match_event_indicator == '3')
    {
      sec_def.is_trade_event = false;
    }
  }

  void CME_MDP_FAST_Channel_Handler::clear_book(Security_Definition& sec_def)
  {
    //cout << Time::current_time().to_string() << " clear_book product_id=" << sec_def.product_id << " security_id=" << sec_def.security_id << endl; 
    ProductID product_id = sec_def.product_id;
    if(product_id != Product::invalid_product_id)
    {
      m_order_book->unlocked_clear(product_id, true); 
      if(m_qbdf_builder)
      {
        for(int i = 0; i < sec_def.m_outright_state.m_num_bids; ++i)
        {
          Price_Point& bid = sec_def.m_outright_state.m_bids[i];
          bool last_in_group = (i == (sec_def.m_outright_state.m_num_bids - 1));
          qbdf_update_level(product_id, 'B', 0, bid.md_price, last_in_group, false, ' ', false);
        }
        for(int i = 0; i < sec_def.m_outright_state.m_num_offers; ++i)
        {
          Price_Point& offer = sec_def.m_outright_state.m_offers[i];
          bool last_in_group = (i == (sec_def.m_outright_state.m_num_offers - 1));
          qbdf_update_level(product_id, 'S', 0, offer.md_price, last_in_group, false, ' ', false);
        }
      }

      m_implied_order_book->unlocked_clear(product_id, true);
      if(m_qbdf_builder)
      {
        for(int i = 0; i < sec_def.m_implied_state.m_num_bids; ++i)
        {
          Price_Point& bid = sec_def.m_implied_state.m_bids[i];
          bool last_in_group = (i == (sec_def.m_implied_state.m_num_bids - 1));
          qbdf_update_level(product_id, 'B', 0, bid.md_price, last_in_group, false, ' ', false);
        }
        for(int i = 0; i < sec_def.m_implied_state.m_num_offers; ++i)
        {
          Price_Point& offer = sec_def.m_implied_state.m_offers[i];
          bool last_in_group = (i == (sec_def.m_implied_state.m_num_offers - 1));
          qbdf_update_level(product_id, 'S', 0, offer.md_price, last_in_group, false, ' ', false);
        }
      }
      memset(&(sec_def.m_outright_state), 0, sizeof(sec_def.m_outright_state));
      memset(&(sec_def.m_implied_state), 0, sizeof(sec_def.m_implied_state));
    }
  }

  void CME_MDP_FAST_Channel_Handler::clear_books_on_this_channel()
  {
    for(hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.begin(); it != m_security_definitions.end(); ++it)
    {
      Security_Definition& sec_def = it->second;
      clear_book(sec_def);
    }
  }

  void CME_MDP_FAST_Channel_Handler::publish_current_state_to_qbdf_snap()
  {
    for(hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.begin(); it != m_security_definitions.end(); ++it)
    {
      Security_Definition& sec_def = it->second;
      // Publish book state
      Book_State m_outright_state;
      Book_State m_implied_state;
      for(int i = 0; i < sec_def.m_outright_state.m_num_bids; ++i)
      {
        Price_Point& ppt = sec_def.m_outright_state.m_bids[i];
        qbdf_update_level(sec_def.product_id, 'B', ppt.size, ppt.md_price, (i == sec_def.m_outright_state.m_num_bids - 1), false, ' ', false);
      }
      for(int i = 0; i < sec_def.m_outright_state.m_num_offers; ++i)
      {
        Price_Point& ppt = sec_def.m_outright_state.m_offers[i];
        qbdf_update_level(sec_def.product_id, 'S', ppt.size, ppt.md_price, (i == sec_def.m_outright_state.m_num_offers - 1), false, ' ', false); 
      }
      for(int i = 0; i < sec_def.m_implied_state.m_num_bids; ++i)
      {
        Price_Point& ppt = sec_def.m_implied_state.m_bids[i];
        qbdf_update_level(sec_def.product_id, 'B', ppt.size, ppt.md_price, (i == sec_def.m_implied_state.m_num_bids - 1), false, 'K', false);
      }
      for(int i = 0; i < sec_def.m_implied_state.m_num_offers; ++i)
      {
        Price_Point& ppt = sec_def.m_implied_state.m_offers[i];
        qbdf_update_level(sec_def.product_id, 'S', ppt.size, ppt.md_price, (i == sec_def.m_implied_state.m_num_offers - 1), false, 'K', false); 
      }

      Time receive_time = Time::current_time();
      uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
      int32_t micros_exch_time_delta = 0;
      QBDFStatusType qbdf_status = QBDFStatusType::Resume;
      gen_qbdf_status_micros(m_qbdf_builder, sec_def.product_id,  micros_since_midnight,
                             micros_exch_time_delta, m_qbdf_exch_char, qbdf_status.index(),
                             "Processed instrument definition");

    }
  }

  size_t CME_MDP_FAST_Channel_Handler::process_incremental(uint8_t* buf, size_t len, uint32_t template_id, uint32_t seq_num)
  {
    //cout << "process_incremental: seq_num=" << seq_num << endl;
    if(seq_num > m_expected_incremental_seq_num)
    {
#ifdef CW_DEBUG
      cout << Time::current_time().to_string() << " Gap on channel " << m_channel_id << ": expecting " << m_expected_incremental_seq_num << ", got " << seq_num << endl;
#endif
      m_expected_incremental_seq_num = seq_num;
    }
    if(seq_num < m_expected_incremental_seq_num)
    {
      // Ignore
    }
    else
    {
      switch(template_id)
      {
        // ToDo: I think there are only around 3-4 different varieties.  Replace with macros
        case 112:
          {
            MDIncRefresh_112& msg = m_fast_decoder.Parse_MDIncRefresh_112(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = NO_VALUE;
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 113:
          {
            MDIncRefresh_113& msg = m_fast_decoder.Parse_MDIncRefresh_113(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = NO_VALUE;
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 115:
          {
            MDIncRefresh_115& msg = m_fast_decoder.Parse_MDIncRefresh_115(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = ' ';
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, ' ');
            }
          }
          break;
        case 116:
          {
            MDIncRefresh_116& msg = m_fast_decoder.Parse_MDIncRefresh_116(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = NO_VALUE;
              char tradeCondition = ' ';
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, ' ');
            }
          }
          break;
        case 117:
          {
            MDIncRefresh_117& msg = m_fast_decoder.Parse_MDIncRefresh_117(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = NO_VALUE;
              char tradeCondition = ' ';
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, ' ');
            }
          }
          break;
        case 119:
          {
            MDIncRefresh_119& msg = m_fast_decoder.Parse_MDIncRefresh_119(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = NO_VALUE;
              char tradeCondition = ' ';
              char quoteCondition = extract_char_2(msg.MDEntries().entries[i].QuoteCondition());
              bool last_in_group = (i == msg.MDEntries().len - 1);
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, ' ');
            }
          }
          break;
        case 120:
          {
            MDIncRefresh_120& msg = m_fast_decoder.Parse_MDIncRefresh_120(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = NO_VALUE;
              char tradeCondition = ' ';
              char quoteCondition = extract_char_2(msg.MDEntries().entries[i].QuoteCondition());
              bool last_in_group = (i == msg.MDEntries().len - 1);
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, ' ');
            }
          }
          break;
        case 121:
          {
            MDIncRefresh_121& msg = m_fast_decoder.Parse_MDIncRefresh_121(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = NO_VALUE;
              char tradeCondition = ' ';
              char quoteCondition = extract_char_2(msg.MDEntries().entries[i].QuoteCondition());
              bool last_in_group = (i == msg.MDEntries().len - 1);
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, ' ');
            }
          }
          break;
        case 122:
          {
            MDIncRefresh_122& msg = m_fast_decoder.Parse_MDIncRefresh_122(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = NO_VALUE;
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 125:
          {
            MDIncRefresh_125& msg = m_fast_decoder.Parse_MDIncRefresh_125(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = NO_VALUE;
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = NO_VALUE;
              char tradeCondition = ' ';
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, ' ');
            }
          }
          break;
        case 127:
          {
            MDSecurityStatus_127& msg = m_fast_decoder.Parse_MDSecurityStatus_127(buf);
            handle_security_status(msg);
          }
          break;
        case 128:
          {
            //Ignore MDQuoteRequest_128
          }
          break;
        case 129:
          {
            MDIncRefresh_129& msg = m_fast_decoder.Parse_MDIncRefresh_129(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = NO_VALUE;
              char tradeCondition = ' ';
              char quoteCondition = extract_char_2(msg.MDEntries().entries[i].QuoteCondition());
              bool last_in_group = (i == msg.MDEntries().len - 1);
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, ' ');
            }
          }
          break;
        case 130:
          {
            MDSecurityStatus_130& msg = m_fast_decoder.Parse_MDSecurityStatus_130(buf);
            handle_security_status(msg);
          }
          break;
        case 133:
          {
            //Ignore MDHeartbeat_133
          }
          break;
        case 141:
          {
            MDIncRefresh_141& msg = m_fast_decoder.Parse_MDIncRefresh_141(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = NO_VALUE;
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 142:
          {
            MDIncRefresh_142& msg = m_fast_decoder.Parse_MDIncRefresh_142(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = NO_VALUE;
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 143:
          {
            MDIncRefresh_143& msg = m_fast_decoder.Parse_MDIncRefresh_143(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = NO_VALUE;
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = ' ';
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 144:
          {
            MDIncRefresh_144& msg = m_fast_decoder.Parse_MDIncRefresh_144(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = extract_char_2(msg.MDEntries().entries[i].QuoteCondition());
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 145:
          {
            MDIncRefresh_145& msg = m_fast_decoder.Parse_MDIncRefresh_145(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = extract_char_2(msg.MDEntries().entries[i].QuoteCondition());
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 146:
          {
            MDIncRefresh_146& msg = m_fast_decoder.Parse_MDIncRefresh_146(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = extract_char_2(msg.MDEntries().entries[i].QuoteCondition());
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 147:
          {
            MDIncRefresh_147& msg = m_fast_decoder.Parse_MDIncRefresh_147(buf);
            for(uint32_t i = 0; i < msg.MDEntries().len; ++i)
            {
              uint32_t securityId = extract(msg.MDEntries().entries[i].SecurityID());
              uint32_t rpt_seq = extract(msg.MDEntries().entries[i].RptSeq());
              uint32_t updateAction = extract(msg.MDEntries().entries[i].MDUpdateAction());
              char entryType = extract_char_2(msg.MDEntries().entries[i].MDEntryType());
              uint32_t priceLevel = extract(msg.MDEntries().entries[i].MDPriceLevel());
              double price = extract(msg.MDEntries().entries[i].MDEntryPx());
              int32_t size = extract(msg.MDEntries().entries[i].MDEntrySize());
              uint32_t tradeVolume = extract(msg.MDEntries().entries[i].TradeVolume());
              char tradeCondition = extract_char_2(msg.MDEntries().entries[i].TradeCondition());
              char quoteCondition = extract_char_2(msg.MDEntries().entries[i].QuoteCondition());
              bool last_in_group = (i == msg.MDEntries().len - 1);
              char match_event_indicator = (msg.MDEntries().entries[i].MatchEventIndicator().has_value ? msg.MDEntries().entries[i].MatchEventIndicator().value.value[0] : ' ');
              apply_incremental_update(securityId, updateAction, entryType, priceLevel, price, size, tradeVolume, tradeCondition, quoteCondition, last_in_group, rpt_seq, match_event_indicator);
            }
          }
          break;
        case 140:
          {
            // Security Status
          }
          break;
        default:
          {
#ifdef CW_DEBUG
            cout << Time::current_time().to_string() << " Unrecognized incremental template " << template_id << " on channel " << m_channel_id << endl;
#endif
          }
          break;
      }
      ++m_expected_incremental_seq_num;
    }
    //m_fast_decoder.Reset();
    return len;
  }

  size_t CME_MDP_FAST_Channel_Handler::handle_incremental(size_t context, uint8_t* buf, size_t len)
  {
    if(!m_finished_handling_instruments)
    {
      return len;
    }
    const CME_MDP::preamble& header = reinterpret_cast<const CME_MDP::preamble&>(*buf);
    uint32_t seq_no = ntohl(header.seq_no);
    //cout << "Incremental msg " << seq_no << " on channel " << m_channel_id << endl;

    buf += sizeof(header);
    uint32_t template_id = m_fast_decoder.Get_Template_Id(buf); 
    //cout << "Template id=" << template_id << endl;

    return process_incremental(buf, len, template_id, seq_no);
  }

  template<typename T>
  void CME_MDP_FAST_Channel_Handler::handle_instrument_msg(T& msg)
  {
    string symbol(msg.SecurityDesc().value.to_string());
#ifdef CW_DEBUG
    cout << " symbol=" << symbol;
#endif
    uint32_t securityId = msg.SecurityID().value;
#ifdef CW_DEBUG
    cout << " securityId=" << securityId;
#endif
    double displayFactor = msg.DisplayFactor().value;
#ifdef CW_DEBUG
    cout << " displayFactor=" << displayFactor;
#endif
    double minPriceIncrement = msg.MinPriceIncrement().value;
#ifdef CW_DEBUG
    cout << " minPriceIncrement=" << minPriceIncrement;
#endif
    uint32_t totNumReports = msg.TotNumReports().value;
#ifdef CW_DEBUG
    cout << " totNumReports=" << totNumReports;
#endif
    string securityGroup = msg.SecurityGroup().value.to_string();
#ifdef CW_DEBUG
    cout << " securityGroup=" << securityGroup;
#endif
#ifdef CW_DEBUG
    cout << " actual symbol=" << msg.Symbol().value.to_string(); 
#endif
    uint64_t maturityMonthYear = msg.MaturityMonthYear().has_value ? msg.MaturityMonthYear().value : 0;
    //uint32_t contractMultiplier = msg.ContractMultiplier().value;
    //cout << " contractMultiplier=" << contractMultiplier << endl;
    //uint64_t originalContractSize = msg.OriginalContractSize().value;
    //cout << " originalContractSize=" << originalContractSize << endl;
    string unitOfMeasureQtyStr = msg.UnitOfMeasureQty().value.to_string();
#ifdef CW_DEBUG
    cout << " unitOfMeasureQty=" << unitOfMeasureQtyStr << endl;
#endif
    uint32_t unitOfMeasureQty = boost::lexical_cast<uint32_t>(unitOfMeasureQtyStr);

    uint32_t outright_max_depth = 0;
    uint32_t implied_max_depth = 0;
    for(uint32_t i = 0; i < msg.MDFeedTypes().len; ++i)
    {
      string feedType = msg.MDFeedTypes().entries[i].MDFeedType().to_string();
      if(feedType == "GBX")
      {
        outright_max_depth = msg.MDFeedTypes().entries[i].MarketDepth();
      }
      else if(feedType == "GBI")
      {
        implied_max_depth = msg.MDFeedTypes().entries[i].MarketDepth();
      }
      else
      {
#ifdef CW_DEBUG
        cout << "Unrecognized feed type " << feedType << endl;
#endif
      }
    } 
#ifdef CW_DEBUG
    cout << " outright_max_depth=" << outright_max_depth;
#endif
#ifdef CW_DEBUG
    cout << " implied_max_depth=" << implied_max_depth;
#endif
    
/*
    if(securityId != 58738)
    {
      productId = Product::invalid_product_id;
    }
*/
    if(msg.StrikePrice().has_value && msg.StrikePrice().value != 0)
    {
#ifdef CW_DEBUG
       cout << " symbol " << symbol << " is an option with strike price=" << msg.StrikePrice().value << "; ignoring ";
#endif
    }
    else
    {
#ifdef CW_DEBUG
      cout << " symbol " << symbol << " is not an option; processing ";
#endif
      AddInstrumentDefinition(securityId, symbol, maturityMonthYear, displayFactor, minPriceIncrement, unitOfMeasureQty, outright_max_depth,
                              implied_max_depth, totNumReports, securityGroup, true);
    }
  }


  void CME_MDP_FAST_Channel_Handler::AddInstrumentDefinition(uint32_t securityId, string& symbol, unsigned maturityMonthYear, 
                                                          double displayFactor,double minPriceIncrement, uint32_t unitOfMeasureQty, 
                                                          uint32_t outright_max_depth, uint32_t implied_max_depth, uint32_t totNumReports, 
                                                          string& securityGroup, bool publish_status)
  {
    ProductID productId = product_id_of_symbol(symbol);
    //cout << " productId=" << productId << endl;
    hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(securityId);
    if(it == m_security_definitions.end())
    {
      if(productId != Product::invalid_product_id)
      {
        ++m_num_valid_products;
        m_msu.m_id = productId;
        m_msu.m_market_status = MarketStatus::Open; // ToDo: Check instrument definition for current market status
        if(m_subscribers) {
          m_subscribers->update_subscribers(m_msu);
        }
        if(m_qbdf_builder && publish_status)
        {
          Time receive_time = Time::current_time();
          uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
          int32_t micros_exch_time_delta = 0;
          QBDFStatusType qbdf_status = QBDFStatusType::Resume;
          gen_qbdf_status_micros(m_qbdf_builder, productId,  micros_since_midnight,
                                 micros_exch_time_delta, m_qbdf_exch_char, qbdf_status.index(),
                                 "Processed instrument definition");
        }
      }
      m_security_definitions.insert(make_pair(securityId, Security_Definition(securityId, symbol, productId, displayFactor, minPriceIncrement, outright_max_depth, implied_max_depth)));
      if(m_security_definitions.size() == totNumReports)
      {
        m_finished_handling_instruments = true;
#ifdef CW_DEBUG
        cout << "finished handling instruments on channel " << m_channel_id << endl;
#endif
      }

      if(m_instrument_logger.get() != NULL)
      {
        m_instrument_logger->printf("%s,%s,%d,%f,%f,%d\n", 
          symbol.c_str(), securityGroup.c_str(), static_cast<uint32_t>(maturityMonthYear), displayFactor, minPriceIncrement, unitOfMeasureQty);
      }
    }
  }

  size_t CME_MDP_FAST_Channel_Handler::handle_instrument(size_t context, uint8_t* buf, size_t len)
  {
    if(!m_finished_handling_instruments)
    {
      const CME_MDP::preamble& header = reinterpret_cast<const CME_MDP::preamble&>(*buf);

      buf += sizeof(header);
      uint32_t template_id = m_fast_decoder.Get_Template_Id(buf); 
      //cout << "Template id=" << template_id << endl;

      switch(template_id)
      {
        case 123:
          {
            MDSecurityDefinition_123& msg = m_fast_decoder.Parse_MDSecurityDefinition_123(buf);
            handle_instrument_msg<MDSecurityDefinition_123>(msg);
          }
          break;
        case 139:
          {
            MDSecurityDefinition_139& msg = m_fast_decoder.Parse_MDSecurityDefinition_139(buf);
            handle_instrument_msg<MDSecurityDefinition_139>(msg);
          }
          break;
        case 140:
          {
            MDSecurityDefinition_140& msg = m_fast_decoder.Parse_MDSecurityDefinition_140(buf);
            handle_instrument_msg<MDSecurityDefinition_140>(msg);
          }
          break;
        default:
          {
#ifdef CW_DEBUG
            cout << Time::current_time().to_string() << " Unrecognized template id " << template_id << " on channel " << m_channel_id << endl;
#endif
          } 
          break;
      }
      //m_fast_decoder.Reset();
    }
    return len;
  }
}
