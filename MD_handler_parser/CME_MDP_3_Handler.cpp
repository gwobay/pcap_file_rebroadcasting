#include <Data/QBDF_Util.h>
#include <Data/CME_MDP_3_Handler.h>
#include <Util/Network_Utils.h>

#include <Exchange/Exchange_Manager.h>
#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <MD/MDProvider.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>
#include <Interface/TimerRequest.h>
#include <Interface/OrderBookUpdate.h>

#include <stdarg.h>

#define MD_ENTRY_TYPE_TRADE 'T'

//using namespace CME_MDP_3;

namespace hf_core {

  static FactoryRegistrar1<std::string, Handler, Application*, CME_MDP_3_Handler> r1("CME_MDP_3_Handler");

  namespace CME_MDP_3_Common
  {
     struct packet_header {
     uint32_t seq_no;
      uint64_t sending_time;
    } __attribute__ ((packed));

    struct message_header
    {
      uint16_t msg_size;
      uint16_t block_length;
      uint16_t template_id;
      uint16_t schema_id;
      uint16_t version;
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
  CME_MDP_3_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "CME_MDP_3_Handler::parse";

    if(len < sizeof(CME_MDP_3_Common::packet_header)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd",
                 where, m_name.c_str(), len, sizeof(CME_MDP_3_Common::packet_header));
      return 0;
    }

    mutex_t::scoped_lock lock(m_mutex);

    if (m_qbdf_builder && !m_use_exch_time) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
    }

    channel_info& ci = m_channel_info_map[context];

    const CME_MDP_3_Common::packet_header& header = reinterpret_cast<const CME_MDP_3_Common::packet_header&>(*buf);
    uint32_t seq_no = header.seq_no;
    uint32_t next_seq_no = seq_no + 1;

    parse2(context, buf, len);
    ci.seq_no = next_seq_no;
    return len;
  }


  size_t
  CME_MDP_3_Handler::parse2(size_t context, const char* buf, size_t len)
  {
    CME_MDP_3_Context_Info & context_info = m_context_info[context];
    const bool finished_instruments = context_info.m_channel_handler->finished_handling_instruments();
    {
      context_info.handle(context, buf, len);
      
      if(m_recorder)
        {
          if (context_info.m_channel_type != CME_MDP_3_Channel_Type::INSTRUMENT || !finished_instruments)
            {
              Logger::mutex_t::scoped_lock l(m_recorder->mutex());
              record_header h(Time::current_time(), context, len);
              m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
              m_recorder->unlocked_write(buf, len);
            }
        }
      
      return len;
      //return ret;
    }
  }


  size_t
  CME_MDP_3_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* packet_buf, size_t packet_len)
  {
    return packet_len;
  }

  void
  CME_MDP_3_Handler::reset(const char* msg)
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
  CME_MDP_3_Handler::reset(size_t context, const char* msg)
  {
// ToDo: implement
/*
    const char* where = "CME_MDP_3_Handler::reset";

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
  CME_MDP_3_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
					  const string& mdSymbol_,const string& mdExch_)
  {
    mutex_t::scoped_lock lock(m_mutex);

    uint64_t s = int_of_symbol(mdSymbol_.c_str(), mdSymbol_.size());

    if(m_prod_map_global.count(s) > 0) {
      m_logger->log_printf(Log::ERROR, "Mapping %s to %zd COLLIDES", mdSymbol_.c_str(), s);
    }

    m_prod_map_global.insert(make_pair(s, id_));
  }

  void CME_MDP_3_Handler::set_up_channels(const Parameter& params)
  {
    const vector<Parameter>& lines = params["cme"].getVector();

    m_context_info.resize(lines.size() + 1);
    m_channel_info_map.resize(lines.size() + 1);

    hash_map<int, int> channel_id_to_version;
    const vector<Parameter>& channel_versions = params["channel_versions"].getVector();
    for(vector<Parameter>::const_iterator it = channel_versions.begin(); it != channel_versions.end(); ++it)
    {
      string date_str = (*it)["date"].getString();
      Time date(date_str, "%Y%m%d");
      if (Time::today() >= date)
      {
        const vector<Parameter>& versions = (*it)["versions"].getVector();
        for(vector<Parameter>::const_iterator version_it = versions.begin(); version_it != versions.end(); ++version_it)
        {
          int version = (*version_it)["version"].getInt();
          const vector<int>& channels = (*version_it)["channels"].getIntVector();
          for(vector<int>::const_iterator channel_it = channels.begin(); channel_it != channels.end(); ++channel_it)
          {
            int channel_id = (*channel_it);
            channel_id_to_version[channel_id] = version;
          }
        }
      }
    }

    size_t context = 0;
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      //const string& name = (*l)["name"].getString();

      const string& channel_type_str = (*l)["channel_type"].getString();
      int channel_id = (*l)["channel_id"].getInt();

      int mdp_version = channel_id_to_version[channel_id];

      m_logger->log_printf(Log::INFO, "CME_MDP_3_Handler set_up_channels: mapping context=%lu to channel type %s channel_id=%d mdP_version=%d",
                           context, channel_type_str.c_str(), channel_id, mdp_version);

      CME_MDP_3_Context_Info& context_info = m_context_info[context];
      CME_MDP_3_Channel_Handler_Base* channel_handler = register_channel(channel_id, mdp_version);
      CME_MDP_3_Channel_Type::Enum channel_type = CME_MDP_3_Channel_Type::from_string(channel_type_str);

      context_info.init(channel_handler, &(m_channel_info_map[context]), channel_type, m_record_only);

      m_channel_info_map[context].context = context;
      m_channel_info_map[context].init(params);
      ++context;
    }
  }

  void
  CME_MDP_3_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
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
      m_order_book->init(params, false);
      m_implied_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_implied_order_book->init(params, true);
      m_order_books.push_back(m_order_book);
      m_secondary_order_books.push_back(m_implied_order_book);
    }

    m_exch = exch;

    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules_global = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules_global->init(m_app, Time::today().strftime("%Y%m%d"));

    //m_security_status_global.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);

    m_stop_listening_to_instrument_defs_after_one_loop = params.getDefaultBool("stop_listening_to_instrument_defs_after_one_loop", true);


    string write_instrument_defs_file = params.getDefaultString("write_instrument_defs_file", "");

    if(write_instrument_defs_file.size() > 0)
    {
      Logger_Config c;
      c.name = write_instrument_defs_file;
      c.filename = write_instrument_defs_file; // ToDo: Do we need to prepend a dir?
      c.truncate = false;

      m_instrument_logger =  m_app->lm()->find_logger(write_instrument_defs_file);
      if(m_instrument_logger.get() == NULL)
      {
        m_instrument_logger = m_app->lm()->create_logger(c);
      }
      m_instrument_logger->write_header("symbol,symbol_base,maturity,display_factor,min_price_increment,unit_of_measure_qty,sid,market_segment_id,trading_reference_price,open_interest_qty,cleared_volume,settle_price_type");
    }

    set_up_channels(params);
  }

  void
  CME_MDP_3_Handler::start()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
    if(m_implied_order_book) {
      m_implied_order_book->clear(false);
    }
  }

  void
  CME_MDP_3_Handler::stop()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
    if(m_implied_order_book) {
      m_implied_order_book->clear(false);
    }
  }

  CME_MDP_3_Handler::CME_MDP_3_Handler(Application* app) :
    Handler(app, "CME_MDP_3"),
    m_hist_mode(false)
  {
  }

  CME_MDP_3_Handler::~CME_MDP_3_Handler()
  {
/*
    for(hash_map<int, CME_MDP_3_Channel_Handler*>::iterator it = m_channel_handlers.begin(); it != m_channel_handlers.end(); ++it)
    {
      delete it->second;
    }
*/
  }


  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::update_quote_status(ProductID product_id, QuoteStatus quote_status, QBDFStatusType qbdf_status, string qbdf_status_reason)
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
      m_handler->update_subscribers(quote_update);
    }
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::send_quote_update()
  {

    //m_msu.m_market_status = m_security_status[m_quote.m_id];
    m_msu.m_market_status = MarketStatus::Open;
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);

    // ToDo: only do this if we're not caught up (processing snapshots)
    //m_quote.m_quote_status = QuoteStatus::Invalid; 

    if(m_provider) {
      m_provider->check_quote(m_quote);
    }
    m_handler->update_subscribers(m_quote);
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::send_trade_update()
  {
    m_msu.m_market_status = MarketStatus::Open;
    m_feed_rules->apply_trade_conditions(m_trade, m_msu);

    if(m_provider) {
      m_provider->check_trade(m_trade);
    }
    m_handler->update_subscribers(m_trade);
  }

  template<typename T> inline double cme_to_double_7(T& v)
  {
    return static_cast<double>(v.mantissa) * 0.0000001;
  }

  template<typename T> inline double cme_to_double_7_nullable(T& v)
  {
    return static_cast<double>(v.mantissa.value) * 0.0000001;
  }

  template<typename MDP_VERSION>
  ProductID CME_MDP_3_Channel_Handler<MDP_VERSION>::product_id_of_symbol(string symbol_string)
  {
    if (m_qbdf_builder) {
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string.c_str());
      if (symbol_id == 0) {
        //m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
        //                     "CME_MDP_3_Handler::product_id_of_symbol", symbol_string.c_str());
        //m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      return symbol_id;
    }

    uint64_t h = int_of_symbol(symbol_string.c_str(), symbol_string.size());
    typename hash_map<uint64_t, ProductID>::iterator i = m_prod_map.find(h);
    if(i == m_prod_map.end()) {
      return Product::invalid_product_id;
    } else {
      return i->second;
    }
  }

  template<typename MDP_VERSION>
  CME_MDP_3_Channel_Handler<MDP_VERSION>::CME_MDP_3_Channel_Handler(int channel_id, boost::shared_ptr<MDOrderBookHandler> order_book, boost::shared_ptr<MDOrderBookHandler> implied_order_book, ExchangeID exch,
    ExchangeRuleRP exch_rule, Feed_Rules_RP feed_rules, boost::shared_ptr<MDSubscriptionAcceptor> subscribers, MDProvider* provider, hash_map<uint64_t, ProductID>& prod_map,
    QBDF_Builder* qbdf_builder, char qbdf_exch_char, bool stop_listening_to_instrument_defs_after_one_loop, LoggerRP logger, LoggerRP instrument_logger, Application* app, 
    CME_MDP_3_Handler* handler)
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
      m_instrument_logger(instrument_logger),
      m_finished_handling_instruments(false),
      m_stop_listening_to_instrument_defs_after_one_loop(stop_listening_to_instrument_defs_after_one_loop),
      m_logger(logger),
      m_app(app),
      m_handler(handler),
      m_num_symbols_snapshotted(0),
      m_num_valid_products(0),
      m_is_trade_event(false)
  {
    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;


    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';
  }

  template<typename MDP_VERSION>
  template<bool IS_IMPLIED>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::qbdf_update_level(ProductID product_id, char side, uint32_t shares, MDPrice md_price, bool last_in_group, bool is_snapshot, bool is_executed, uint64_t exch_timestamp)
  {
    //if(m_qbdf_builder && !is_snapshot)
    if(m_qbdf_builder)
    {
      // ToDo: get receive_time and exch_time_delta properly
      Time receive_time = Time::current_time();
      uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
      uint64_t exch_micros_since_midnight = (exch_timestamp / 1000) - Time::today().total_usec();
      int32_t micros_exch_time_delta = exch_micros_since_midnight - micros_since_midnight;
      const char quoteCondition = IS_IMPLIED ? 'K' : ' ';
      gen_qbdf_level_iprice64(m_qbdf_builder, product_id,  micros_since_midnight, micros_exch_time_delta, m_qbdf_exch_char, side,
                                          ' ', quoteCondition, ' ', 1 /*ToDo: num orders*/, shares, md_price, last_in_group, is_executed);
    }
  }

  template<typename MDP_VERSION>
  template<typename T>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::handle_snapshot_msg(T& msg, uint32_t seqNum)
  {
    Time quotim = Time::current_time(); //  ToDo: figure out units and convert sending_time_units to quotim
    uint32_t security_id = msg.security_id;
    typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(security_id);
    bool outright_inside_changed = false;
    uint64_t exch_time = msg.transact_time;
    if(it != m_security_definitions.end() && it->second.product_id != Product::invalid_product_id)
    {
      Security_Definition& sec_def = it->second;
      uint32_t rpt_seq = msg.rpt_seq;
      if(rpt_seq <= sec_def.rpt_seq)
      {
        return;
      }
      clear_book(sec_def);
      sec_def.rpt_seq = rpt_seq;
      for(int i = 0; i < static_cast<int>(msg.no_md_entries().numInGroup); ++i)
      {
        typename T::md_entries_t& entry = msg.md_entries(i);

        double price_raw = cme_to_double_7_nullable(entry.md_entry_px); // ToDo: check if price is null?
        double price = price_raw * sec_def.display_factor;
        MDPrice md_price = MDPrice::from_fprice_precise(price);
        int32_t size = entry.md_entry_size.value; // ToDo: check if size is null?
        uint32_t priceLevel = static_cast<uint32_t>(entry.md_price_level.value); // ToDo: check if price_level is null?

        switch(entry.md_entry_type.value)
        {
          case MDP_VERSION::MDEntryType::Bid:
            {
              outright_inside_changed |= m_order_book->add_level_ip(quotim, sec_def.product_id, 'B', md_price, size, 1); // ToDo: fill in NumberOfOrders (last parameter)?
              if(m_order_book->orderbook_update()) {
                m_order_book->post_orderbook_update();
              }
              if(m_qbdf_builder)
              {
                bool last_in_group = (i == static_cast<int>(msg.no_md_entries().numInGroup) - 1);
                qbdf_update_level<false>(sec_def.product_id, 'B', size, md_price, last_in_group, true, false, exch_time);
              }
              sec_def.m_outright_state.m_bids[priceLevel - 1].md_price = md_price;
              sec_def.m_outright_state.m_bids[priceLevel - 1].size = size;
              ++(sec_def.m_outright_state.m_num_bids);
              if(sec_def.m_outright_state.m_num_bids > 21)
              {
              }
            }
            break;
          case MDP_VERSION::MDEntryType::Offer:
            {
              outright_inside_changed |= m_order_book->add_level_ip(quotim, sec_def.product_id, 'S', md_price, size, 1); // ToDo: fill in NumberOfOrders (last parameter)?
              if(m_order_book->orderbook_update()) {
                m_order_book->post_orderbook_update();
              }
              if(m_qbdf_builder)
              {
                bool last_in_group = (i == static_cast<int>(msg.no_md_entries().numInGroup) - 1);
                qbdf_update_level<false>(sec_def.product_id, 'S', size, md_price, last_in_group, true, false, exch_time);
              }
              sec_def.m_outright_state.m_offers[priceLevel - 1].md_price = md_price;
              sec_def.m_outright_state.m_offers[priceLevel - 1].size = size;
              ++(sec_def.m_outright_state.m_num_offers);
            }
            break;
          case MDP_VERSION::MDEntryType::Trade:
            {
              m_trade.m_id = sec_def.product_id;
              m_trade.m_price = price;
              m_trade.m_size = size;
              m_trade.set_side(0); // ToDo: Use AggressorSide?
              m_trade.m_trade_qualifier[0] = ' '; // ToDo: I don't think this comes through in the message, but make sure
              m_trade.set_time(quotim);
              send_trade_update();
            }
            break;
          case MDP_VERSION::MDEntryType::ImpliedBid:
            {
              m_implied_order_book->add_level_ip(quotim, sec_def.product_id, 'B', md_price, size, 1); // ToDo: fill in NumberOfOrders (last parameter)?
              if(m_qbdf_builder)
              {
                bool last_in_group = (i == static_cast<int>(msg.no_md_entries().numInGroup) - 1);
                qbdf_update_level<true>(sec_def.product_id, 'B', size, md_price, last_in_group, true, false, exch_time);
              }
              sec_def.m_implied_state.m_bids[priceLevel - 1].md_price = md_price;
              sec_def.m_implied_state.m_bids[priceLevel - 1].size = size;
              ++(sec_def.m_implied_state.m_num_bids);
              if(sec_def.m_implied_state.m_num_bids > 21)
              {
              }
            }
            break;
          case MDP_VERSION::MDEntryType::ImpliedOffer:
            {
              m_implied_order_book->add_level_ip(quotim, sec_def.product_id, 'S', md_price, size, 1); // ToDo: fill in NumberOfOrders (last parameter)?
              if(m_qbdf_builder)
              {
                bool last_in_group = (i == static_cast<int>(msg.no_md_entries().numInGroup) - 1);
                qbdf_update_level<true>(sec_def.product_id, 'S', size, md_price, last_in_group, true, false, exch_time);
              }
              sec_def.m_implied_state.m_offers[priceLevel - 1].md_price = md_price;
              sec_def.m_implied_state.m_offers[priceLevel - 1].size = size;
              ++(sec_def.m_implied_state.m_num_offers);
            }
            break;
          case CME_MDP_3_v5::MDEntryType::EmptyBook:
            {
              // ToDo?
            }
            break;
          case CME_MDP_3_v5::MDEntryType::OpeningPrice:
            {
              // ToDo
            }
            break;
          case MDP_VERSION::MDEntryType::SettlementPrice:
            {
            }
            break;
          case MDP_VERSION::MDEntryType::ThresholdLimitsandPriceBandVariation:
            {
              // ToDo
            }
            break;
/*
          case MDP_VERSION::MDEntryType::TradeVolume:
            {
              if(size > sec_def.total_volume)
              {
                sec_def.total_volume = size;
#ifdef CW_DEBUG
                cout << Time::current_time().to_string() << " security_id=" << security_id << " setting total_volume=" << size << endl;
#endif
              }
            }
            break;
*/
          case MDP_VERSION::MDEntryType::ElectronicVolume:
            {
              if(size > sec_def.total_volume)
              {
                sec_def.total_volume = size;
              }
            }
            break;
          default:
            {
              // Don't care about: TradingSessionHighPrice, TradingSessionLowPrice, TradeVolume (I think?), SessionHighBid, SessionLowOffer, FixingPrice, ElectronicVolume
            }
            break;
        }
      }
      if(outright_inside_changed)
      {
         m_quote.set_time(quotim);
         m_quote.m_id = sec_def.product_id;
         m_quote.m_cond = ' '; // ToDo: fill this in, but how do we deal with two different conditions (one for bid one for ask)?
         m_order_book->fill_quote_update(sec_def.product_id, m_quote);
         send_quote_update();
/*
         if(security_id == 803678)
         {
#ifdef CW_DEBUG
           cout << Time::current_time().to_string() << " bid=" << m_quote.m_bid << " ask=" << m_quote.m_ask << endl;
#endif
#ifdef CW_DEBUG
           cout << "    outright_state.bid=" << sec_def.m_outright_state.m_bids[0].md_price.fprice() << " outright_state.ask=" << sec_def.m_outright_state.m_offers[0].md_price.fprice() << endl;
#endif
#ifdef CW_DEBUG
           cout << "    implied_state.bid=" << sec_def.m_implied_state.m_bids[0].md_price.fprice() << " implied_state.ask=" << sec_def.m_implied_state.m_offers[0].md_price.fprice() << endl;
#endif

         }
*/
      }
      sec_def.needs_snapshot = false;
      while(!sec_def.incremental_message_queue.Empty() && !sec_def.needs_snapshot)
      {
        CmeIncrementalMessage& msg = *(sec_def.incremental_message_queue.Front());
        apply_incremental_update(msg.security_id, msg.update_action, msg.entry_type, msg.price_level, msg.price, msg.size, 
                                 msg.last_in_group, msg.rpt_seq, msg.match_event_indicator, msg.exch_time);
        sec_def.incremental_message_queue.Pop();
      }
      update_quote_status(sec_def.product_id, QuoteStatus::Normal, QBDFStatusType::Open, "Finished recovering");
    } 
  }

  template<typename MDP_VERSION>
  size_t CME_MDP_3_Channel_Handler<MDP_VERSION>::handle_snapshot(size_t context, uint8_t* buf, size_t len)
  {
    const CME_MDP_3_Common::packet_header& packet_header = reinterpret_cast<const CME_MDP_3_Common::packet_header&>(*buf);
    uint32_t seq_no = packet_header.seq_no;
    uint8_t* buf_ptr = buf + sizeof(CME_MDP_3_Common::packet_header);
    uint8_t* end_ptr = buf + len;
    while(buf_ptr < end_ptr)
    {
      const CME_MDP_3_Common::message_header& message_header = reinterpret_cast<const CME_MDP_3_Common::message_header&>(*buf_ptr);
      uint16_t msg_size = message_header.msg_size;
      uint16_t template_id = message_header.template_id;
      buf_ptr += sizeof(CME_MDP_3_Common::message_header);
      switch(template_id)
      {
        case 38:
          {
            typename MDP_VERSION::SnapshotFullRefresh38& msg = reinterpret_cast<typename MDP_VERSION::SnapshotFullRefresh38&>(*buf_ptr);
            handle_snapshot_msg(msg, seq_no);
          }
          break;
        default:
          {
            m_logger->log_printf(Log::WARN, "CME_MDP_3_Channel_Handler<MDP_VERSION>::handle_incremental::Unrecognized template_id %d", template_id);
          }
          break;
      }
      buf_ptr += (msg_size - sizeof(CME_MDP_3_Common::message_header));;
    }
    return len;
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::apply_security_status(Security_Definition& sec_def, uint32_t security_status)
  {
    // ToDo
  }

  template<typename MDP_VERSION>
  template<typename T>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::handle_security_status(T& msg)
  {
   // ToDo
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::clear_book(Security_Definition& sec_def)
  {
    ProductID product_id = sec_def.product_id;
    if(product_id != Product::invalid_product_id)
    {
      m_order_book->clear(product_id, true); 
      if(m_order_book->orderbook_update() && m_order_book->orderbook_update()->product_id != Product::invalid_product_id) {
        m_order_book->post_orderbook_update();
      } 
      uint64_t exch_time = Time::current_time().total_usec() * 1000;
      if(m_qbdf_builder)
      {
        for(int i = 0; i < sec_def.m_outright_state.m_num_bids; ++i)
        {
          Price_Point& bid = sec_def.m_outright_state.m_bids[i];
          bool last_in_group = (i == (sec_def.m_outright_state.m_num_bids - 1));
          qbdf_update_level<false>(product_id, 'B', 0, bid.md_price, last_in_group, false, false, exch_time);
        }
        for(int i = 0; i < sec_def.m_outright_state.m_num_offers; ++i)
        {
          Price_Point& offer = sec_def.m_outright_state.m_offers[i];
          bool last_in_group = (i == (sec_def.m_outright_state.m_num_offers - 1));
          qbdf_update_level<false>(product_id, 'S', 0, offer.md_price, last_in_group, false, false, exch_time);
        }
      }
      sec_def.m_outright_state.m_num_bids = 0;
      sec_def.m_outright_state.m_num_offers = 0
;
      m_implied_order_book->unlocked_clear(product_id, true); 
      if(m_qbdf_builder)
      {
        for(int i = 0; i < sec_def.m_implied_state.m_num_bids; ++i)
        {
          Price_Point& bid = sec_def.m_implied_state.m_bids[i];
          bool last_in_group = (i == (sec_def.m_implied_state.m_num_bids - 1));
          qbdf_update_level<true>(product_id, 'B', 0, bid.md_price, last_in_group, false, false, exch_time);
        }
        for(int i = 0; i < sec_def.m_implied_state.m_num_offers; ++i)
        {
          Price_Point& offer = sec_def.m_implied_state.m_offers[i];
          bool last_in_group = (i == (sec_def.m_implied_state.m_num_offers - 1));
          qbdf_update_level<true>(product_id, 'S', 0, offer.md_price, last_in_group, false, false, exch_time);
        }
      }
      sec_def.m_implied_state.m_num_bids = 0;
      sec_def.m_implied_state.m_num_offers = 0;
    }
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::clear_books_on_this_channel()
  {
    for(typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.begin(); it != m_security_definitions.end(); ++it)
    {
      Security_Definition& sec_def = it->second;
      if(sec_def.product_id != Product::invalid_product_id)
      {
        clear_book(sec_def);
      }
    }
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::publish_current_state_to_qbdf_snap()
  {
    uint64_t exch_time = Time::current_time().total_usec() * 1000;
    for(typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.begin(); it != m_security_definitions.end(); ++it)
    {
      Security_Definition& sec_def = it->second;
      // Publish book state
      Book_State m_outright_state;
      Book_State m_implied_state;
      for(int i = 0; i < sec_def.m_outright_state.m_num_bids; ++i)
      {
        Price_Point& ppt = sec_def.m_outright_state.m_bids[i];
        qbdf_update_level<false>(sec_def.product_id, 'B', ppt.size, ppt.md_price, (i == sec_def.m_outright_state.m_num_bids - 1), false, false, exch_time);
      }
      for(int i = 0; i < sec_def.m_outright_state.m_num_offers; ++i)
      {
        Price_Point& ppt = sec_def.m_outright_state.m_offers[i];
        qbdf_update_level<false>(sec_def.product_id, 'S', ppt.size, ppt.md_price, (i == sec_def.m_outright_state.m_num_offers - 1), false, false, exch_time); 
      }
      for(int i = 0; i < sec_def.m_implied_state.m_num_bids; ++i)
      {
        Price_Point& ppt = sec_def.m_implied_state.m_bids[i];
        qbdf_update_level<true>(sec_def.product_id, 'B', ppt.size, ppt.md_price, (i == sec_def.m_implied_state.m_num_bids - 1), false, false, exch_time);
      }
      for(int i = 0; i < sec_def.m_implied_state.m_num_offers; ++i)
      {
        Price_Point& ppt = sec_def.m_implied_state.m_offers[i];
        qbdf_update_level<true>(sec_def.product_id, 'S', ppt.size, ppt.md_price, (i == sec_def.m_implied_state.m_num_offers - 1), false, false, exch_time); 
      }

      Time receive_time = Time::current_time();
      uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
      uint64_t exch_micros_since_midnight = (exch_time / 1000) - Time::today().total_usec();
      int32_t micros_exch_time_delta = exch_micros_since_midnight - micros_since_midnight;
      QBDFStatusType qbdf_status = QBDFStatusType::Resume;
      gen_qbdf_status_micros(m_qbdf_builder, sec_def.product_id,  micros_since_midnight,
                             micros_exch_time_delta, m_qbdf_exch_char, qbdf_status.index(),
                             "Processed instrument definition");
    }
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::set_is_trade_event(bool new_value)
  {
    m_is_trade_event = new_value;
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::apply_daily_statistics(uint32_t security_id, char entry_type, double price, uint32_t rpt_seq, uint8_t match_event_indicator_val, uint64_t exch_time_ns, uint32_t settle_price_type)
  {
    typename MDP_VERSION::MatchEventIndicator* match_event_indicator = reinterpret_cast<typename MDP_VERSION::MatchEventIndicator*>(&(match_event_indicator_val));

    Time exch_time = Time(exch_time_ns);
    //cout << "STATISTICS33: md_entry_px=" << cme_to_double_7_nullable(entry.md_entry_px) << " md_entry_size=" << entry.md_entry_size.value 
    //     << " security_id=" << entry.security_id << " trading_reference_date=" << entry.trading_reference_date.value
    //     /*<< " settl_price_type=" << entry.settl_price_type.value*/ << " md_update_action=" << entry.md_update_action.value << " md_entry_type=" << entry.md_entry_type.value << endl;
    
    typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(security_id);
    if(it != m_security_definitions.end())
    {
      Security_Definition& sec_def = it->second;
      if(sec_def.product_id != Product::invalid_product_id)
      {
        //cout << "typename MDP_VERSION::MDIncrementalRefreshDailyStatistics33: rpt_seq=" << rpt_seq << " sec_def.rpt_seq=" << sec_def.rpt_seq << endl;
        if(rpt_seq > sec_def.rpt_seq)
        {
          if(sec_def.needs_snapshot)
          {
            //cout << "typename MDP_VERSION::MDIncrementalRefreshDailyStatistics33: sec_def needs snapshot so ignoring" << endl;
          }
          else if(rpt_seq > sec_def.rpt_seq + 1)
          {
            //cout << "typename MDP_VERSION::MDIncrementalRefreshDailyStatistics33: missed message for security_id=" << security_id << " expecting rpt_seq=" << (sec_def.rpt_seq + 1) << " got " << rpt_seq << endl;
            sec_def.needs_snapshot = true;
          }
          else
          {
            sec_def.rpt_seq = rpt_seq;
  
            switch(entry_type)
            {
              case MDP_VERSION::MDEntryTypeDailyStatistics::SettlementPrice:
              {
                int32_t size = 0; //entry.md_entry_size.value;
  /*
                if(security_id == 326771)
                {
#ifdef CW_DEBUG
                  cout.precision(9);
#endif
#ifdef CW_DEBUG
                  cout << "SettlementPrice: price=" << price << endl;
#endif
                }
  */
                bool rounded = (settle_price_type & (0x1 << 2));
                //cout << Time::current_time().to_string() << ",symbol=" << sec_def.symbol << ",settlement_price=" << price << ",settle_price_type=" << settle_price_type << ",rounded=" << rounded << endl;
                if(!rounded || sec_def.last_unrounded_settlement_price_time < Time::currentish_time() - hours(1)) { 
                  m_trade.m_id = sec_def.product_id;
                  m_trade.m_price = price * sec_def.display_factor;
                  m_trade.m_size = size;
                  m_trade.set_side(0); // ToDo: Use AggressorSide?
                  m_trade.m_trade_qualifier[0] = 'S';
                  char tradeCondition = 'S'; 
                  m_trade.set_time(exch_time);
                  send_trade_update();
                  set_is_trade_event(true);
                  if(m_qbdf_builder)
                  {
                    Time receive_time = Time::current_time();
                    uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
                    uint64_t exch_micros_since_midnight = (exch_time_ns / 1000) - Time::today().total_usec();
                    int32_t micros_exch_time_delta = exch_micros_since_midnight - micros_since_midnight;
                    MDPrice md_trade_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                    gen_qbdf_trade_micros(m_qbdf_builder, sec_def.product_id, micros_since_midnight, micros_exch_time_delta,
                                      m_qbdf_exch_char, ' ', ' ', string(1, tradeCondition), size, md_trade_price, false);
                  }
                  if(!rounded) {
                    sec_def.last_unrounded_settlement_price_time = Time::currentish_time();
                  }
                }
              }
              break;
              default:
              {
              }
              break;
            }
            if(match_event_indicator->EndOfEvent())
            {
              set_is_trade_event(false);
            }
          }
        }
      }
      else
      {
        if(rpt_seq > sec_def.rpt_seq)
        {
          sec_def.rpt_seq = rpt_seq;
        }
      }
    }
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::apply_incremental_update(uint32_t security_id, uint8_t update_action, char entry_type, uint8_t price_level, double price, int32_t size, bool last_in_group, uint32_t rpt_seq, uint8_t match_event_indicator_val, uint64_t exch_time_ns)
  {
    typename MDP_VERSION::MatchEventIndicator* match_event_indicator = reinterpret_cast<typename MDP_VERSION::MatchEventIndicator*>(&(match_event_indicator_val));

    typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(security_id);
    if(it == m_security_definitions.end())
    {
      //cout << "Sec def not found" << endl;
      return;
    }
    Security_Definition& sec_def = it->second;
    if(it->second.product_id == Product::invalid_product_id)
    {
      if(rpt_seq > sec_def.rpt_seq)
      {
        ///in acout << "product not found" << endl;
        if(match_event_indicator->EndOfEvent())
        {
          set_is_trade_event(false);
        }
        sec_def.rpt_seq = rpt_seq;
      }
      return;
    }
    if(rpt_seq <= sec_def.rpt_seq)// && )
    {
      return;
    }
    if(sec_def.needs_snapshot)
    {
      m_logger->log_printf(Log::INFO, "sec_def needs snapshot so queueing");
      sec_def.incremental_message_queue.Push(security_id, update_action, entry_type, price_level, price, size, last_in_group, rpt_seq, match_event_indicator_val, exch_time_ns);
      return;
    }

    if(rpt_seq > sec_def.rpt_seq + 1)
    {
      m_logger->log_printf(Log::WARN, "Missed message for security_id=%d expecting rpt_seq=%d got %d", security_id, (sec_def.rpt_seq + 1), rpt_seq);
      // Missed a message.  Queue it up
      sec_def.needs_snapshot = true;
      set_is_trade_event(false);
      sec_def.incremental_message_queue.Push(security_id, update_action, entry_type, price_level, price, size, last_in_group, rpt_seq, match_event_indicator_val, exch_time_ns);
      return;
    }
    sec_def.rpt_seq = rpt_seq;

    Time exch_time(exch_time_ns);

/*
#ifdef CW_DEBUG
    cout << "security_id=" << security_id << " update_action=" << (int)update_action << " entry_type=" << entry_type << " price_level=" << (int)price_level << " price=" << price << " size=" << size << " rpt_seq=" << rpt_seq << endl;
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
        cout << "price=" << sec_def.m_outright_state.m_offers[i].md_price.fprice() << " size=" << sec_def.m_outright_state.m_offers[i].size  << endl;
#endif
      }
*/

    bool inside_changed = false;
    switch(update_action)
    {
      case MDP_VERSION::MDUpdateAction::New:
        {
          switch(entry_type)
          {
            case MDP_VERSION::MDEntryTypeBook::Bid:
              {
                MDPrice md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                inside_changed |= m_order_book->add_level_ip(exch_time, sec_def.product_id, 'B', md_price, size, 1);
                if(m_order_book->orderbook_update()) {
                  m_order_book->post_orderbook_update();
                }
                MDBidBook& book = m_order_book->book(sec_def.product_id).bid();
                if(m_qbdf_builder)
                {
                  qbdf_update_level<false>(sec_def.product_id, 'B', size, md_price, last_in_group && book.levels().size() <= sec_def.outright_max_depth, false, false, exch_time_ns);
                }
                if(sec_def.m_outright_state.m_num_bids >= price_level)
                {
                  memmove(&(sec_def.m_outright_state.m_bids[price_level]), &(sec_def.m_outright_state.m_bids[price_level - 1]), sizeof(Price_Point) * (sec_def.m_outright_state.m_num_bids - price_level + 1));
                }
                sec_def.m_outright_state.m_bids[price_level - 1].md_price = md_price;
                sec_def.m_outright_state.m_bids[price_level - 1].size = size;
                ++(sec_def.m_outright_state.m_num_bids);
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::Offer:
              {
                MDPrice md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                inside_changed |= m_order_book->add_level_ip(exch_time, sec_def.product_id, 'S', md_price, size, 1);
                if(m_order_book->orderbook_update()) {
                  m_order_book->post_orderbook_update();
                }
                MDAskBook& book = m_order_book->book(sec_def.product_id).ask();
                if(m_qbdf_builder)
                {
                  qbdf_update_level<false>(sec_def.product_id, 'S', size, md_price, last_in_group && book.levels().size() <= sec_def.outright_max_depth, false, false, exch_time_ns);
                }
                if(sec_def.m_outright_state.m_num_offers >= price_level)
                {
                  memmove(&(sec_def.m_outright_state.m_offers[price_level]), &(sec_def.m_outright_state.m_offers[price_level - 1]), sizeof(Price_Point) * (sec_def.m_outright_state.m_num_offers - price_level + 1));
                }
                sec_def.m_outright_state.m_offers[price_level - 1].md_price = md_price;
                sec_def.m_outright_state.m_offers[price_level - 1].size = size;
                ++(sec_def.m_outright_state.m_num_offers);
              }
              break;
            case MD_ENTRY_TYPE_TRADE:
              {
                m_trade.m_id = sec_def.product_id;
                m_trade.m_price = price * sec_def.display_factor;
                m_trade.m_size = size;
                m_trade.set_side(0); // ToDo: Use AggressorSide?
                m_trade.m_trade_qualifier[0] = ' '; 
                char tradeCondition = ' '; 
                if(match_event_indicator->LastTradeMsg()) {
                  m_trade.m_trade_qualifier[0] = 'L';
                  tradeCondition = 'L';
                }
                m_trade.set_time(exch_time);
                send_trade_update();
                set_is_trade_event(true);
                if(m_qbdf_builder)
                {
                  Time receive_time = Time::current_time();
                  uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
                  uint64_t exch_micros_since_midnight = (exch_time_ns / 1000) - Time::today().total_usec();
                  int32_t micros_exch_time_delta = exch_micros_since_midnight - micros_since_midnight;
                  MDPrice md_trade_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                  gen_qbdf_trade_micros(m_qbdf_builder, sec_def.product_id, micros_since_midnight, micros_exch_time_delta,
                                    m_qbdf_exch_char, ' ', ' ', string(1, tradeCondition), size, md_trade_price, false);
                }
                sec_def.total_volume += size;
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::BookReset: 
              {
                m_logger->log_printf(Log::WARN, "Got a BookReset msg for product id=%d ; clearing book", sec_def.product_id);
                clear_book(sec_def);
              }
              break;
            case ' ':
              {
                m_logger->log_printf(Log::WARN, "Got a NO_VALUE entry_type");
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedBid:
              {
                MDPrice md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                inside_changed |= m_implied_order_book->add_level_ip(exch_time, sec_def.product_id, 'B', md_price, size, 1);
                MDBidBook& book = m_implied_order_book->book(sec_def.product_id).bid();
                if(m_qbdf_builder)
                {
                  qbdf_update_level<true>(sec_def.product_id, 'B', size, md_price, last_in_group && book.levels().size() <= sec_def.implied_max_depth, false, false, exch_time_ns);
                }
                if(sec_def.m_implied_state.m_num_bids >= price_level)
                {
                  memmove(&(sec_def.m_implied_state.m_bids[price_level]), &(sec_def.m_implied_state.m_bids[price_level - 1]), sizeof(Price_Point) * (sec_def.m_implied_state.m_num_bids - price_level + 1));
                }
                sec_def.m_implied_state.m_bids[price_level - 1].md_price = md_price;
                sec_def.m_implied_state.m_bids[price_level - 1].size = size;
                ++(sec_def.m_implied_state.m_num_bids);
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedOffer:
              {
                MDPrice md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                inside_changed |= m_implied_order_book->add_level_ip(exch_time, sec_def.product_id, 'S', md_price, size, 1);
                MDAskBook& book = m_implied_order_book->book(sec_def.product_id).ask();
                if(m_qbdf_builder)
                {
                  qbdf_update_level<true>(sec_def.product_id, 'S', size, md_price, last_in_group && book.levels().size() <= sec_def.implied_max_depth, false, false, exch_time_ns);
                }
                if(sec_def.m_implied_state.m_num_offers >= price_level)
                {
                  memmove(&(sec_def.m_implied_state.m_offers[price_level]), &(sec_def.m_implied_state.m_offers[price_level - 1]), sizeof(Price_Point) * (sec_def.m_implied_state.m_num_offers - price_level + 1));
                }
                sec_def.m_implied_state.m_offers[price_level - 1].md_price = md_price;
                sec_def.m_implied_state.m_offers[price_level - 1].size = size;
                ++(sec_def.m_implied_state.m_num_offers);
              }
              break;
            case CME_MDP_3_v5::MDEntryType::OpeningPrice:
            //case CME_MDP_3_v6::MDEntryType::OpenPrice:
            case MDP_VERSION::MDEntryType::SettlementPrice:
            {
              break;
            }
            case MDP_VERSION::MDEntryType::TradingSessionHighPrice:
            case MDP_VERSION::MDEntryType::TradingSessionLowPrice:
            case CME_MDP_3_v5::MDEntryType::TradeVolume:
            //case CME_MDP_3_v6::MDEntryType::ClearedVolume:
            case MDP_VERSION::MDEntryType::OpenInterest:
            case MDP_VERSION::MDEntryType::SessionHighBid:
            case MDP_VERSION::MDEntryType::SessionLowOffer:
            case MDP_VERSION::MDEntryType::FixingPrice:
            case MDP_VERSION::MDEntryType::ElectronicVolume:
            case MDP_VERSION::MDEntryType::ThresholdLimitsandPriceBandVariation:
              {
                //Ignore
              }
              break;
            default:
              {
                m_logger->log_printf(Log::WARN, "CME got unknown entry type %d for action %d (NEW)", entry_type, update_action);
              }
              break;
          }
        }
        break;
      case MDP_VERSION::MDUpdateAction::Change:
      case MDP_VERSION::MDUpdateAction::Overlay:
        {
          switch(entry_type)
          {
            case MDP_VERSION::MDEntryTypeBook::Bid:
              {
                MDPrice existing_price = sec_def.m_outright_state.m_bids[price_level - 1].md_price;
                MDPrice new_md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                if(existing_price != new_md_price)
                {
                  inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, false);
                  inside_changed |= m_order_book->add_level_ip(exch_time, sec_def.product_id, 'B', new_md_price, size, 1);
                  if(m_order_book->orderbook_update()) {
                    m_order_book->post_orderbook_update();
                  }
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<false>(sec_def.product_id, 'B', 0, existing_price, false, false, false, exch_time_ns);
                    qbdf_update_level<false>(sec_def.product_id, 'B', size, new_md_price, last_in_group, false, false, exch_time_ns);
                  }
                  sec_def.m_outright_state.m_bids[price_level - 1].md_price = new_md_price;
                  sec_def.m_outright_state.m_bids[price_level - 1].size = size;
                }
                else
                {
                  inside_changed |= m_order_book->modify_level_ip(exch_time, sec_def.product_id, 'B', existing_price, size, 1 /*ToDo: num_orders*/, m_is_trade_event);
                  if(m_order_book->orderbook_update()) {
                    m_order_book->post_orderbook_update();
                  }
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<false>(sec_def.product_id, 'B', size, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                  sec_def.m_outright_state.m_bids[price_level - 1].size = size;
                }
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::Offer:
              {
                MDPrice existing_price = sec_def.m_outright_state.m_offers[price_level - 1].md_price;
                MDPrice new_md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                if(existing_price != new_md_price)
                {
                  inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, false);
                  inside_changed |= m_order_book->add_level_ip(exch_time, sec_def.product_id, 'S', new_md_price, size, 1);
                  if(m_order_book->orderbook_update()) {
                    m_order_book->post_orderbook_update();
                  }
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<false>(sec_def.product_id, 'S', 0, existing_price, false, false, false, exch_time_ns);
                    qbdf_update_level<false>(sec_def.product_id, 'S', size, new_md_price, last_in_group, false, false, exch_time_ns);
                  }
                  sec_def.m_outright_state.m_offers[price_level - 1].md_price = new_md_price;
                  sec_def.m_outright_state.m_offers[price_level - 1].size = size;
                }
                else
                {
                  inside_changed |= m_order_book->modify_level_ip(exch_time, sec_def.product_id, 'S', existing_price, size, 1 /*ToDo: num_orders*/, m_is_trade_event);
                  if(m_order_book->orderbook_update()) {
                    m_order_book->post_orderbook_update();
                  }
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<false>(sec_def.product_id, 'S', size, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                  sec_def.m_outright_state.m_offers[price_level - 1].size = size;
                }
              }
              break;
            case MD_ENTRY_TYPE_TRADE:
              {
                m_logger->log_printf(Log::WARN, "CME Got change/overlay trade update_action=%d", update_action);
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::BookReset:
              {
		m_logger->log_printf(Log::WARN, "CME Got a BookReset msg for product id=%d; clearing book", sec_def.product_id);
                clear_book(sec_def);
              }
              break;
            case ' ':
              {
		m_logger->log_printf(Log::WARN, "CME Got NO_VALUE entry type");
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedBid:
              {
                MDPrice existing_price = sec_def.m_implied_state.m_bids[price_level - 1].md_price;
                MDPrice new_md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                if(existing_price != new_md_price)
                {
                  m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, false);
                  m_implied_order_book->add_level_ip(exch_time, sec_def.product_id, 'B', new_md_price, size, 1);
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<true>(sec_def.product_id, 'B', 0, existing_price, false, false, false, exch_time_ns);
                    qbdf_update_level<true>(sec_def.product_id, 'B', size, new_md_price, last_in_group, false, false, exch_time_ns);
                  }
                  sec_def.m_implied_state.m_bids[price_level - 1].md_price = new_md_price;
                  sec_def.m_implied_state.m_bids[price_level - 1].size = size;
                }
                else
                {
                  m_implied_order_book->modify_level_ip(exch_time, sec_def.product_id, 'B', existing_price, size, 1 /*ToDo: num_orders*/, m_is_trade_event);
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<true>(sec_def.product_id, 'B', size, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                  sec_def.m_implied_state.m_bids[price_level - 1].size = size;
                }
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedOffer:
              {
                MDPrice existing_price = sec_def.m_implied_state.m_offers[price_level - 1].md_price;
                MDPrice new_md_price = MDPrice::from_fprice_precise(price * sec_def.display_factor);
                if(existing_price != new_md_price)
                {
                  m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, false);
                  m_implied_order_book->add_level_ip(exch_time, sec_def.product_id, 'S', new_md_price, size, 1);
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<true>(sec_def.product_id, 'S', 0, existing_price, false, false, false, exch_time_ns);
                    qbdf_update_level<true>(sec_def.product_id, 'S', size, new_md_price, last_in_group, false, false, exch_time_ns);
                  }
                  sec_def.m_implied_state.m_offers[price_level - 1].md_price = new_md_price;
                  sec_def.m_implied_state.m_offers[price_level - 1].size = size;
                }
                else
                {
                  m_implied_order_book->modify_level_ip(exch_time, sec_def.product_id, 'S', existing_price, size, 1 /*ToDo: num_orders*/, m_is_trade_event); 
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<true>(sec_def.product_id, 'S', size, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                  sec_def.m_implied_state.m_offers[price_level - 1].size = size;
                }
              }
              break;
            default:
              {
		m_logger->log_printf(Log::WARN, "CME Got unknown entry type %d for action=%d (CHANGE/OVERLAY)", entry_type, update_action);
              }
              break;
          }
        }
        break;
      case MDP_VERSION::MDUpdateAction::Delete:
        {
          switch(entry_type)
          {
            case MDP_VERSION::MDEntryTypeBook::Bid:
              {
                MDPrice existing_price = sec_def.m_outright_state.m_bids[price_level - 1].md_price;
                inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, m_is_trade_event);
                if(m_order_book->orderbook_update()) {
                  m_order_book->post_orderbook_update();
                }
                if(m_qbdf_builder)
                {
                  qbdf_update_level<false>(sec_def.product_id, 'B', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                }
                if(sec_def.m_outright_state.m_num_bids > price_level)
                {
                  memmove(&(sec_def.m_outright_state.m_bids[price_level - 1]), &(sec_def.m_outright_state.m_bids[price_level]), sizeof(Price_Point) * (sec_def.m_outright_state.m_num_bids - price_level));
                }
                --(sec_def.m_outright_state.m_num_bids);
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::Offer:
              {
                MDPrice existing_price = sec_def.m_outright_state.m_offers[price_level - 1].md_price;
                inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, m_is_trade_event); 
                if(m_order_book->orderbook_update()) {
                  m_order_book->post_orderbook_update();
                }
                if(m_qbdf_builder)
                {
                  qbdf_update_level<false>(sec_def.product_id, 'S', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                }
                if(sec_def.m_outright_state.m_num_offers > price_level)
                {
                  memmove(&(sec_def.m_outright_state.m_offers[price_level - 1]), &(sec_def.m_outright_state.m_offers[price_level]), sizeof(Price_Point) * (sec_def.m_outright_state.m_num_offers - price_level));
                }
                --(sec_def.m_outright_state.m_num_offers);
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::BookReset: 
              {
		m_logger->log_printf(Log::WARN, "Got an 'empty the book' message (J) for product id=%d; clearing book", sec_def.product_id);
                clear_book(sec_def);
              }
              break;
            case MD_ENTRY_TYPE_TRADE:
              {
		m_logger->log_printf(Log::WARN, "Got delete trade");
              }
              break;
            case ' ':
              {
		m_logger->log_printf(Log::WARN, "Got NO_VALUE entry type");
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedBid:
              {
                MDPrice existing_price = sec_def.m_implied_state.m_bids[price_level - 1].md_price;
                m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, m_is_trade_event);
                if(m_qbdf_builder)
                {
                  qbdf_update_level<true>(sec_def.product_id, 'B', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                }
                if(sec_def.m_implied_state.m_num_bids > price_level)
                {
                  memmove(&(sec_def.m_implied_state.m_bids[price_level - 1]), &(sec_def.m_implied_state.m_bids[price_level]), sizeof(Price_Point) * (sec_def.m_implied_state.m_num_bids - price_level));
                }
                --(sec_def.m_implied_state.m_num_bids);
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedOffer:
              {
                MDPrice existing_price = sec_def.m_implied_state.m_offers[price_level - 1].md_price;
                m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, m_is_trade_event); 
                if(m_qbdf_builder)
                {
                  qbdf_update_level<true>(sec_def.product_id, 'S', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                }
                if(sec_def.m_implied_state.m_num_offers > price_level)
                {
                  memmove(&(sec_def.m_implied_state.m_offers[price_level - 1]), &(sec_def.m_implied_state.m_offers[price_level]), sizeof(Price_Point) * (sec_def.m_implied_state.m_num_offers - price_level));
                }
                --(sec_def.m_implied_state.m_num_offers);
              }
              break;
            default: 
              {
		m_logger->log_printf(Log::WARN, "Got unknown entry type %d for action=%d (DELETE)", entry_type, update_action);
              }
              break;
          }
        }
        break;
      case MDP_VERSION::MDUpdateAction::DeleteFrom:
        {
          switch(entry_type)
          {
            case MDP_VERSION::MDEntryTypeBook::Bid:
              {
                MDBidBook& book = m_order_book->book(sec_def.product_id).bid();
                size_t original_num_levels = book.levels().size();
                for(size_t level = original_num_levels; level >= price_level; --level)
                {
                  MDPrice existing_price = sec_def.m_outright_state.m_bids[level - 1].md_price;
                  inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, m_is_trade_event); 
                  if(m_order_book->orderbook_update()) {
                    m_order_book->post_orderbook_update();
                  }
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<false>(sec_def.product_id, 'B', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                  --(sec_def.m_outright_state.m_num_bids);
                }
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::Offer:
              {
                MDAskBook& book = m_order_book->book(sec_def.product_id).ask();
                size_t original_num_levels = book.levels().size();
                for(size_t level = original_num_levels; level >= price_level; --level)
                {
                  MDPrice existing_price = sec_def.m_outright_state.m_offers[level - 1].md_price;
                  inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, m_is_trade_event);
                  if(m_order_book->orderbook_update()) {
                    m_order_book->post_orderbook_update();
                  }
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<false>(sec_def.product_id, 'S', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                  --(sec_def.m_outright_state.m_num_offers);
                }
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::BookReset:
              {
		m_logger->log_printf(Log::WARN, "CME Got an 'empty the book' message (J) for product id=%d; clearing book", sec_def.product_id);
                clear_book(sec_def);
              }
              break;
            case ' ':
              {
		m_logger->log_printf(Log::WARN, "CME Got NO_VALUE entry type");
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedBid:
              {
                MDBidBook& book = m_implied_order_book->book(sec_def.product_id).bid();
                size_t original_num_levels = book.levels().size();
                for(size_t level = original_num_levels; level >= price_level; --level)
                {
                  MDPrice existing_price = sec_def.m_implied_state.m_bids[level - 1].md_price;
                  m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, m_is_trade_event); 
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<true>(sec_def.product_id, 'B', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                  --(sec_def.m_implied_state.m_num_bids);
                }
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedOffer:
              {
                MDAskBook& book = m_implied_order_book->book(sec_def.product_id).ask();
                size_t original_num_levels = book.levels().size();
                for(size_t level = original_num_levels; level >= price_level; --level)
                {
                  MDPrice existing_price = sec_def.m_implied_state.m_offers[level - 1].md_price;
                  m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, m_is_trade_event);
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<true>(sec_def.product_id, 'S', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                  --(sec_def.m_implied_state.m_num_offers);
                }
              }
              break;
            default:
              {
		m_logger->log_printf(Log::WARN, "CME Got unknown entry type %d for action=%d (DELETE_FROM)", entry_type, update_action);
              }
              break;
          }
        }
        break;
      case MDP_VERSION::MDUpdateAction::DeleteThru:
        {
          switch(entry_type)
          {
            case MDP_VERSION::MDEntryTypeBook::Bid:
              {
                for(size_t level = 0; level < price_level; ++level)
                {
                  MDPrice existing_price = sec_def.m_outright_state.m_bids[level].md_price;
                  inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, m_is_trade_event);
                  if(m_order_book->orderbook_update()) {
                    m_order_book->post_orderbook_update();
                  }
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<false>(sec_def.product_id, 'B', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                }
                size_t num_remaining_levels = sec_def.m_outright_state.m_num_bids - price_level;
                if(price_level < sec_def.m_outright_state.m_num_bids)
                {
                  memmove(&(sec_def.m_outright_state.m_bids[0]), &(sec_def.m_outright_state.m_bids[price_level]), sizeof(Price_Point) * num_remaining_levels);
                }
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::Offer:
              {
                for(size_t level = 0; level < price_level; ++level)
                {
                  MDPrice existing_price = sec_def.m_outright_state.m_offers[level].md_price;
                  inside_changed |= m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, m_is_trade_event);
                  if(m_order_book->orderbook_update()) {
                    m_order_book->post_orderbook_update();
                  }
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<false>(sec_def.product_id, 'S', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                }
                size_t num_remaining_levels = sec_def.m_outright_state.m_num_offers - price_level;
                if(price_level < sec_def.m_outright_state.m_num_offers)
                {
                  memmove(&(sec_def.m_outright_state.m_offers[0]), &(sec_def.m_outright_state.m_offers[price_level]), sizeof(Price_Point) * num_remaining_levels);
                }
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::BookReset:
              {
		m_logger->log_printf(Log::WARN, "CME Got an 'empty the book' message (J) for product id=%d; clearing book", sec_def.product_id);
                clear_book(sec_def);
              }
              break;
            case ' ':
              {
		m_logger->log_printf(Log::WARN, "CME Got NO_VALUE entry type");
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedBid:
              {
                for(size_t level = 0; level < price_level; ++level)
                {
                  MDPrice existing_price = sec_def.m_implied_state.m_bids[level].md_price;
                  m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', existing_price, m_is_trade_event);
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<true>(sec_def.product_id, 'B', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                }
                size_t num_remaining_levels = sec_def.m_implied_state.m_num_bids - price_level;
                if(price_level < sec_def.m_implied_state.m_num_bids)
                {
                  memmove(&(sec_def.m_implied_state.m_bids[0]), &(sec_def.m_implied_state.m_bids[price_level]), sizeof(Price_Point) * num_remaining_levels);
                }
              }
              break;
            case MDP_VERSION::MDEntryTypeBook::ImpliedOffer:
              {
                for(size_t level = 0; level < price_level; ++level)
                {
                  MDPrice existing_price = sec_def.m_implied_state.m_offers[level].md_price;
                  m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', existing_price, m_is_trade_event);
                  if(m_qbdf_builder)
                  {
                    qbdf_update_level<true>(sec_def.product_id, 'S', 0, existing_price, last_in_group, false, m_is_trade_event, exch_time_ns);
                  }
                }
                size_t num_remaining_levels = sec_def.m_implied_state.m_num_offers - price_level;
                if(price_level < sec_def.m_implied_state.m_num_offers)
                {
                  memmove(&(sec_def.m_implied_state.m_offers[0]), &(sec_def.m_implied_state.m_offers[price_level]), sizeof(Price_Point) * num_remaining_levels);
                }
              }
              break;
            default:
              {
		m_logger->log_printf(Log::WARN, "CME Got unknown entry type %d for action=%d (DELETE_THRU)", entry_type, update_action);
              }
              break;
          }
        }
        break;

      default:
        {
	  m_logger->log_printf(Log::WARN, "CME Got unrecognized update action %d", update_action);
        }
        break;
    }
    if(inside_changed)
    {
       m_quote.set_time(exch_time);
       m_quote.m_id = sec_def.product_id;
       m_quote.m_cond = ' '; // ToDo: fill this in, but how do we deal with two different conditions (one for bid one for ask)?
       m_order_book->fill_quote_update(sec_def.product_id, m_quote);
       send_quote_update();
/*
       if(security_id == 803678)
       {
#ifdef CW_DEBUG
         cout << Time::current_time().to_string() << " bid=" << m_quote.m_bid << " ask=" << m_quote.m_ask << endl;
#endif
#ifdef CW_DEBUG
         cout << "    outright_state.bid=" << sec_def.m_outright_state.m_bids[0].md_price.fprice() << " outright_state.ask=" << sec_def.m_outright_state.m_offers[0].md_price.fprice() << endl;
#endif
#ifdef CW_DEBUG
         cout << "    implied_state.bid=" << sec_def.m_implied_state.m_bids[0].md_price.fprice() << " implied_state.ask=" << sec_def.m_implied_state.m_offers[0].md_price.fprice() << endl;
#endif
       }
*/
    }
    while(sec_def.m_outright_state.m_num_bids > static_cast<int32_t>(sec_def.outright_max_depth)) {
      MDPrice price_at_level_to_delete = sec_def.m_outright_state.m_bids[sec_def.m_outright_state.m_num_bids - 1].md_price;
      m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', price_at_level_to_delete, false);
      if(m_order_book->orderbook_update()) {
        m_order_book->post_orderbook_update();
      }
      if(m_qbdf_builder) {
        qbdf_update_level<false>(sec_def.product_id, 'B', 0, price_at_level_to_delete, true, false, false, exch_time_ns);
      }
      --(sec_def.m_outright_state.m_num_bids);
    }
    while(sec_def.m_outright_state.m_num_offers > static_cast<int32_t>(sec_def.outright_max_depth)) {
      MDPrice price_at_level_to_delete = sec_def.m_outright_state.m_offers[sec_def.m_outright_state.m_num_offers - 1].md_price;
      m_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', price_at_level_to_delete, false);
      if(m_order_book->orderbook_update()) {
        m_order_book->post_orderbook_update();
      }
      if(m_qbdf_builder) {
        qbdf_update_level<false>(sec_def.product_id, 'S', 0, price_at_level_to_delete, true, false, false, exch_time_ns);
      }
      --(sec_def.m_outright_state.m_num_offers);
    }
  
    while(sec_def.m_implied_state.m_num_bids > static_cast<int32_t>(sec_def.implied_max_depth)) {
      MDPrice price_at_level_to_delete = sec_def.m_implied_state.m_bids[sec_def.m_implied_state.m_num_bids - 1].md_price;
      m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'B', price_at_level_to_delete, false);
      if(m_qbdf_builder) {
        qbdf_update_level<true>(sec_def.product_id, 'B', 0, price_at_level_to_delete, true, false, false, exch_time_ns);
      }
      --(sec_def.m_implied_state.m_num_bids);
    }
    while(sec_def.m_implied_state.m_num_offers > static_cast<int32_t>(sec_def.implied_max_depth)) {
      MDPrice price_at_level_to_delete = sec_def.m_implied_state.m_offers[sec_def.m_implied_state.m_num_offers - 1].md_price;
      m_implied_order_book->remove_level_ip(exch_time, sec_def.product_id, 'S', price_at_level_to_delete, false);
      if(m_qbdf_builder) {
        qbdf_update_level<true>(sec_def.product_id, 'S', 0, price_at_level_to_delete, true, false, false, exch_time_ns);
      }
      --(sec_def.m_implied_state.m_num_offers);
    }
    if(match_event_indicator->EndOfEvent())
    {
      set_is_trade_event(false);
    }
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::process_incremental(uint8_t*& buf_ptr)
  {
    const CME_MDP_3_Common::message_header& message_header = reinterpret_cast<const CME_MDP_3_Common::message_header&>(*buf_ptr);
    uint16_t msg_size = message_header.msg_size;
    uint16_t template_id = message_header.template_id;
    buf_ptr += sizeof(CME_MDP_3_Common::message_header);
    switch(template_id)
    {
      case 4: // ChannelReset
        {
          typename MDP_VERSION::ChannelReset4& msg = reinterpret_cast<typename MDP_VERSION::ChannelReset4&>(*buf_ptr);
          hash_map<int, bool> channels_to_reset;
          for(int i = 0; i < static_cast<int>(msg.no_md_entries().numInGroup); ++i)
          {
            typename MDP_VERSION::ChannelReset4_MDEntries_Entry& entry = msg.md_entries(i);
            int channel_id = static_cast<int>(entry.appl_id);
	    m_logger->log_printf(Log::WARN, "CME got ChannelReset for channel %d", channel_id);
            channels_to_reset[channel_id] = true;
          }

          for(typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.begin(); it != m_security_definitions.end(); ++it)
          {
            Security_Definition& sec_def = it->second;
            if(sec_def.product_id != Product::invalid_product_id)
            {
              if(channels_to_reset.find(sec_def.channel_id) != channels_to_reset.end())
              {
                clear_book(sec_def);
                sec_def.rpt_seq = 0;
              }
            }
            else
            {
              sec_def.rpt_seq = 0;
            }
          }
        }
        break;
      case 12: // AdminHeartbeat
        {
          //cout << Time::current_time().to_string() << " - AdminHeartbeat" << endl;
        }
        break;
      case 30: // SecurityStatus
        {
	  typename MDP_VERSION::SecurityStatus30& msg = reinterpret_cast<typename MDP_VERSION::SecurityStatus30&>(*buf_ptr);
	  m_logger->log_printf(Log::INFO, "CME SECURITY_STATUS: security_id=%d security_trading_status=%d halt_reason=%d security_trading_event=%d", 
              msg.security_id.value, static_cast<int>(msg.security_trading_status.value.value), static_cast<int>(msg.halt_reason.value), static_cast<int>(msg.security_trading_event.value));
          //typename MDP_VERSION::MatchEventIndicator& match_event_indicator = msg.match_event_indicator;
          //cout << endl;
          //if(match_event_indicator.EndOfEvent())
          //{
          //  set_is_trade_event(false);
          //}
        }
        break;
      case 32:
        {
          typename MDP_VERSION::MDIncrementalRefreshBook32& msg = reinterpret_cast<typename MDP_VERSION::MDIncrementalRefreshBook32&>(*buf_ptr);
          typename MDP_VERSION::MatchEventIndicator& match_event_indicator = msg.match_event_indicator;
          uint64_t exch_time = msg.transact_time;
          for(int i = 0; i < static_cast<int>(msg.no_md_entries().numInGroup); ++i)
          {
            typename MDP_VERSION::MDIncrementalRefreshBook32_MDEntries_Entry& entry = msg.md_entries(i);
            int32_t security_id = static_cast<uint32_t>(entry.security_id);
            uint32_t rpt_seq = entry.rpt_seq;
            uint8_t price_level = entry.md_price_level;
            uint8_t update_action = entry.md_update_action.value;
            char entry_type = entry.md_entry_type.value;
            double price = cme_to_double_7_nullable(entry.md_entry_px);
            int32_t size = entry.md_entry_size.value;
            bool last_in_group = (i == static_cast<int>(msg.no_md_entries().numInGroup) - 1);
            apply_incremental_update(security_id, update_action, entry_type, price_level, price, size, last_in_group, rpt_seq, match_event_indicator.value, exch_time);
          }
        }
        break;
      case 33: // typename MDP_VERSION::MDIncrementalRefreshDailyStatistics33
        {
          typename MDP_VERSION::MDIncrementalRefreshDailyStatistics33& msg = reinterpret_cast<typename MDP_VERSION::MDIncrementalRefreshDailyStatistics33&>(*buf_ptr);
          typename MDP_VERSION::MatchEventIndicator& match_event_indicator = msg.match_event_indicator;
          uint64_t exch_time_ns = msg.transact_time;
          for(int i = 0; i < static_cast<int>(msg.no_md_entries().numInGroup); ++i)
          {
            typename MDP_VERSION::MDIncrementalRefreshDailyStatistics33_MDEntries_Entry& entry = msg.md_entries(i);
            int32_t security_id = static_cast<uint32_t>(entry.security_id);
            uint32_t rpt_seq = entry.rpt_seq;
            char entry_type = entry.md_entry_type.value;
            double price = cme_to_double_7_nullable(entry.md_entry_px);
            uint32_t settle_price_type = static_cast<uint32_t>(*reinterpret_cast<uint8_t*>(&(entry.settl_price_type)));

            apply_daily_statistics(security_id, entry_type, price, rpt_seq, match_event_indicator.value, exch_time_ns, settle_price_type);
          }
          if(match_event_indicator.EndOfEvent())
          {
            set_is_trade_event(false);
          }
        }
        break;
      case 34: // typename MDP_VERSION::MDIncrementalRefreshLimitsBanding34
        {
          typename MDP_VERSION::MDIncrementalRefreshLimitsBanding34& msg = reinterpret_cast<typename MDP_VERSION::MDIncrementalRefreshLimitsBanding34&>(*buf_ptr);
          typename MDP_VERSION::MatchEventIndicator& match_event_indicator = msg.match_event_indicator;
          for(int i = 0; i < static_cast<int>(msg.no_md_entries().numInGroup); ++i)
          {
            typename MDP_VERSION::MDIncrementalRefreshLimitsBanding34_MDEntries_Entry& entry = msg.md_entries(i);
            int32_t security_id = static_cast<uint32_t>(entry.security_id);
            uint32_t rpt_seq = entry.rpt_seq;

            //cout << "BANDING34: high_limit_price=" << cme_to_double_7_nullable(entry.high_limit_price) << " low_limit_price=" << cme_to_double_7_nullable(entry.low_limit_price)
            //     << " max_price_variation=" << cme_to_double_7_nullable(entry.max_price_variation)
            //     << " security_id=" << entry.security_id << endl;

            typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(security_id);
            if(it != m_security_definitions.end())
            {
              Security_Definition& sec_def = it->second;
              if(sec_def.product_id != Product::invalid_product_id)
              {
                 if(rpt_seq > sec_def.rpt_seq)
                 {
                  if(sec_def.needs_snapshot)
                  {
                    //cout << "typename MDP_VERSION::MDIncrementalRefreshLimitsBanding34: sec_def needs snapshot so ignoring" << endl;
                  }
                  else if(rpt_seq > sec_def.rpt_seq + 1)
                  {
                    //cout << "typename MDP_VERSION::MDIncrementalRefreshLimitsBanding34: missed message for security_id=" << security_id << " expecting rpt_seq=" << (sec_def.rpt_seq + 1) << " got " << rpt_seq << endl;
                    sec_def.needs_snapshot = true;
                  }
                  else
                  {
                    sec_def.rpt_seq = rpt_seq;
                    if(match_event_indicator.EndOfEvent())
                    {
                      set_is_trade_event(false);
                    }
                  }
                }
              }
              else
              {
                if(rpt_seq > sec_def.rpt_seq)
                {
                  sec_def.rpt_seq = rpt_seq;
                  if(match_event_indicator.EndOfEvent())
                  {
                    set_is_trade_event(false);
                  }
                }
              }
            }
          }
        }
        break;
      case 35: // typename MDP_VERSION::MDIncrementalRefreshSessionStatistics35
        {
          typename MDP_VERSION::MDIncrementalRefreshSessionStatistics35& msg = reinterpret_cast<typename MDP_VERSION::MDIncrementalRefreshSessionStatistics35&>(*buf_ptr);
          typename MDP_VERSION::MatchEventIndicator& match_event_indicator = msg.match_event_indicator;
          for(int i = 0; i < static_cast<int>(msg.no_md_entries().numInGroup); ++i)
          {
            typename MDP_VERSION::MDIncrementalRefreshSessionStatistics35_MDEntries_Entry& entry = msg.md_entries(i);
            int32_t security_id = static_cast<uint32_t>(entry.security_id);
            uint32_t rpt_seq = entry.rpt_seq;

            //cout << "STATISTICS35: md_entry_px=" << cme_to_double_7(entry.md_entry_px) ;
            //cout << " security_id=" << entry.security_id << " open_close_settl_flag=" << entry.open_close_settl_flag.value.value ;
            //cout << " md_update_action=" << entry.md_update_action.value << " md_entry_type=" << entry.md_entry_type.value << endl;

            typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(security_id);
            if(it != m_security_definitions.end())
            {
              Security_Definition& sec_def = it->second;
              if(sec_def.product_id != Product::invalid_product_id)
              {
                //cout << "typename MDP_VERSION::MDIncrementalRefreshSessionStatistics35: rpt_seq=" << rpt_seq << " sec_def.rpt_seq=" << sec_def.rpt_seq << endl;
                if(rpt_seq > sec_def.rpt_seq)
                {
                  if(sec_def.needs_snapshot)
                  {
                    //cout << "typename MDP_VERSION::MDIncrementalRefreshSessionStatistics35: sec_def needs snapshot so ignoring" << endl;
                  }
                  else if(rpt_seq > sec_def.rpt_seq + 1)
                  {
                    //cout << "typename MDP_VERSION::MDIncrementalRefreshSessionStatistics35: missed message for security_id=" << security_id << " expecting rpt_seq=" << (sec_def.rpt_seq + 1) << " got " << rpt_seq << endl;
                    sec_def.needs_snapshot = true;
                  }
                  else
                  {
                    sec_def.rpt_seq = rpt_seq;
                    if(match_event_indicator.EndOfEvent())
                    {
                      set_is_trade_event(false);
                    }
                  }
                }
              }
              else
              {
                if(rpt_seq > sec_def.rpt_seq)
                {
                  sec_def.rpt_seq = rpt_seq;
                  if(match_event_indicator.EndOfEvent())
                  {
                    set_is_trade_event(false);
                  }
                }
              }
            }
          }
        }
        break;
      case 36: 
        {
          typename MDP_VERSION::MDIncrementalRefreshTrade36& msg = reinterpret_cast<typename MDP_VERSION::MDIncrementalRefreshTrade36&>(*buf_ptr);
          typename MDP_VERSION::MatchEventIndicator& match_event_indicator = msg.match_event_indicator;
          uint64_t exch_time = msg.transact_time;
          for(int i = 0; i < static_cast<int>(msg.no_md_entries().numInGroup); ++i)
          {
            typename MDP_VERSION::MDIncrementalRefreshTrade36_MDEntries_Entry& entry = msg.md_entries(i);
            int32_t security_id = static_cast<uint32_t>(entry.security_id);
            uint32_t rpt_seq = entry.rpt_seq;
            uint8_t update_action = entry.md_update_action.value;
            double price = cme_to_double_7(entry.md_entry_px);
            int32_t size = entry.md_entry_size;
            bool last_in_group = (i == static_cast<int>(msg.no_md_entries().numInGroup) - 1);
            //cout <<  Time::current_time().to_string() << " typename MDP_VERSION::MDIncrementalRefreshTrade36 security_id=" << security_id << " rpt_seq=" << rpt_seq << " price=" << price << " size=" << size << endl;
            apply_incremental_update(security_id, update_action, MD_ENTRY_TYPE_TRADE, 0, price, size, last_in_group, rpt_seq, match_event_indicator.value, exch_time);
          }
        }
        break;
      case 37: // typename MDP_VERSION::MDIncrementalRefreshVolume37
        {
          typename MDP_VERSION::MDIncrementalRefreshVolume37& msg = reinterpret_cast<typename MDP_VERSION::MDIncrementalRefreshVolume37&>(*buf_ptr);
          typename MDP_VERSION::MatchEventIndicator& match_event_indicator = msg.match_event_indicator;
          uint64_t exch_time_ns = msg.transact_time;
          for(int i = 0; i < static_cast<int>(msg.no_md_entries().numInGroup); ++i)
          {
            typename MDP_VERSION::MDIncrementalRefreshVolume37_MDEntries_Entry& entry = msg.md_entries(i);
            int32_t security_id = static_cast<uint32_t>(entry.security_id);
            uint32_t rpt_seq = entry.rpt_seq;
            typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(security_id);
            if(it != m_security_definitions.end())
            {
              Security_Definition& sec_def = it->second;
              if(sec_def.product_id != Product::invalid_product_id)
              {
                if(rpt_seq > sec_def.rpt_seq)
                {
                  if(sec_def.needs_snapshot)
                  {
                    //cout << "typename MDP_VERSION::MDIncrementalRefreshVolume37: sec_def needs snapshot so ignoring" << endl;
                  }
                  else if(rpt_seq > sec_def.rpt_seq + 1)
                  {
                    //cout << "typename MDP_VERSION::MDIncrementalRefreshVolume37: missed message for security_id=" << security_id << " expecting rpt_seq=" << (sec_def.rpt_seq + 1) << " got " << rpt_seq << endl;
                    sec_def.needs_snapshot = true;
                  }
                  else
                  {
                    //cout << "typename MDP_VERSION::MDIncrementalRefreshVolume37: price=" << cme_to_double_7(entry.
                    sec_def.rpt_seq = rpt_seq;
                    //cout << Time::current_time().to_string() << " typename MDP_VERSION::MDIncrementalRefreshVolume37: security_id=" << security_id << " md_entry_size=" << entry.md_entry_size << " md_update_action=" << static_cast<int>(entry.md_update_action.value);
                    if (entry.md_entry_size > sec_def.total_volume)
                    {
                      int32_t implied_trade_size = entry.md_entry_size - sec_def.total_volume;
                      //cout << " Looks like implied trade size=" << implied_trade_size << endl;
  
                      m_trade.m_id = sec_def.product_id;
                      m_trade.m_price = 0.0; // Where to get price?
                      m_trade.m_size = implied_trade_size;
                      m_trade.set_side(0); // ToDo: Use AggressorSide?
                      m_trade.m_trade_qualifier[0] = '1';
                      m_trade.set_time(Time::current_time());
                      send_trade_update();
                      set_is_trade_event(true);
                      if(m_qbdf_builder)
                      {
                        Time receive_time = Time::current_time();
                        uint64_t micros_since_midnight = receive_time.total_usec() - Time::today().total_usec();
                        uint64_t exch_micros_since_midnight = (exch_time_ns / 1000) - Time::today().total_usec();
                        int32_t micros_exch_time_delta = exch_micros_since_midnight - micros_since_midnight;
                        MDPrice md_trade_price = MDPrice::from_fprice_precise(0.0);
                        gen_qbdf_trade_micros(m_qbdf_builder, sec_def.product_id, micros_since_midnight, micros_exch_time_delta,
                                          m_qbdf_exch_char, ' ', ' ', string("1"), implied_trade_size, md_trade_price, false);
                      }
  
                      sec_def.total_volume = entry.md_entry_size;
                    }
                    if(match_event_indicator.EndOfEvent())
                    {
                      set_is_trade_event(false);
                    }
                    //cout << endl;
                  }
                }
              }
              else
              {
                if(rpt_seq > sec_def.rpt_seq)
                {
                  sec_def.rpt_seq = rpt_seq;
                  if(match_event_indicator.EndOfEvent())
                  {
                    set_is_trade_event(false);
                  }
                }
              }
            }
          }
          if(match_event_indicator.EndOfEvent())
          {
            set_is_trade_event(false);
          }
        }
        break;
      case 39: // QuoteRequest
        {

        }
        break;
      case 42: // typename MDP_VERSION::MDIncrementalRefreshTradeSummary42
        {
          typename MDP_VERSION::MDIncrementalRefreshTradeSummary42& msg = reinterpret_cast<typename MDP_VERSION::MDIncrementalRefreshTradeSummary42&>(*buf_ptr);
          typename MDP_VERSION::MatchEventIndicator& match_event_indicator = msg.match_event_indicator;
          uint64_t exch_time = msg.transact_time;
          for(int i = 0; i < static_cast<int>(msg.no_md_entries().numInGroup); ++i)
          {
            typename MDP_VERSION::MDIncrementalRefreshTradeSummary42_MDEntries_Entry& entry = msg.md_entries(i);
            int32_t security_id = static_cast<uint32_t>(entry.security_id);
            uint32_t rpt_seq = entry.rpt_seq;
            uint8_t update_action = entry.md_update_action.value;
            double price = cme_to_double_7(entry.md_entry_px);
            int32_t size = entry.md_entry_size;
            bool last_in_group = (i == static_cast<int>(msg.no_md_entries().numInGroup) - 1);
            //cout << Time::current_time().to_string() << " TradeSummary32: security_id=" << security_id << " update_action=" << update_action << " price=" << price << " size=" << size << endl;
            apply_incremental_update(security_id, update_action, MD_ENTRY_TYPE_TRADE, 0, price, size, last_in_group, rpt_seq, match_event_indicator.value, exch_time);
          }
        }
        break;
      default:
        {
	  m_logger->log_printf(Log::WARN, "CME handle_incremental::Unrecognized template_id %d", template_id);
        }
        break;
    }
    buf_ptr += (msg_size - sizeof(CME_MDP_3_Common::message_header));;
  }

  template<typename MDP_VERSION>
  size_t CME_MDP_3_Channel_Handler<MDP_VERSION>::handle_incremental(size_t context, uint8_t* buf, size_t len)
  {
    //cout << Time::current_time().to_string() << " handle_incremental" << endl;
    //const CME_MDP_3_Common::packet_header& packet_header = reinterpret_cast<const CME_MDP_3_Common::packet_header&>(*buf);
    //uint32_t seq_no = ntohl(packet_header.seq_no);
    uint8_t* buf_ptr = buf + sizeof(CME_MDP_3_Common::packet_header);
    uint8_t* end_ptr = buf + len;
    while(buf_ptr < end_ptr)
    {
      process_incremental(buf_ptr);
    }
    return len;
  }

  template<typename T>
  bool try_get_min_price_increment(T& msg, double& val)
  {
    val = cme_to_double_7(msg.min_price_increment);
    return true;
  }

  template<>
  bool try_get_min_price_increment<CME_MDP_3_v5::MDInstrumentDefinitionOption41>(CME_MDP_3_v5::MDInstrumentDefinitionOption41& msg, double& val)
  {
    val = cme_to_double_7_nullable(msg.min_price_increment);
    return msg.min_price_increment.mantissa.has_value();
  }

  template<>
  bool try_get_min_price_increment<CME_MDP_3_v6::MDInstrumentDefinitionOption41>(CME_MDP_3_v6::MDInstrumentDefinitionOption41& msg, double& val)
  {
    val = cme_to_double_7_nullable(msg.min_price_increment);
    return msg.min_price_increment.mantissa.has_value();
  }

  template<typename MDP_VERSION>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::AddInstrumentDefinition(uint32_t securityId, string& symbol, unsigned maturity_month_year, double displayFactor, 
                                                          double minPriceIncrement, double unit_of_measure_qty, uint32_t outright_max_depth, 
                                                          uint32_t implied_max_depth, uint32_t totNumReports, string& security_group, bool publish_status,
                                                          uint32_t market_segment_id, double trading_reference_price, int open_interest_qty, int cleared_volume,
                                                          uint32_t settle_price_type) 
  {
    ProductID productId;
    bool invalid_symbol = false;

    /*
    if(symbol != "GCQ6")
    {
      productId = Product::invalid_product_id;
    }
    else */if(symbol.find("-") == string::npos && symbol.find(":") == string::npos && symbol.find(",") == string::npos)
    {
      productId = product_id_of_symbol(symbol);
    }
    else
    {
      productId = Product::invalid_product_id;
      invalid_symbol = true;
    }
    //cout << " productId=" << productId << endl;

    //cout << "symbol=" << symbol << ",settle_price_type=" << settle_price_type << endl;

    int64_t sid = 0;
    if(m_instrument_logger.get() != NULL)
    {
      const Product* product=  m_app->pm()->find_by_symbol(symbol);
      if(product)
      {
        sid = product->sid();
      }
      
      if(!invalid_symbol)
      {
        m_instrument_logger->printf("%s,%s,%d,%f,%f,%f,%ld,%d,%f,%d,%d,%d\n",
          symbol.c_str(), security_group.c_str(), static_cast<uint32_t>(maturity_month_year), displayFactor, minPriceIncrement, unit_of_measure_qty, sid, market_segment_id, trading_reference_price,
          open_interest_qty, cleared_volume, settle_price_type);
      }
    }

    typename hash_map<uint32_t, Security_Definition>::iterator it = m_security_definitions.find(securityId);
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
      m_security_definitions.insert(make_pair(securityId, Security_Definition(securityId, symbol, productId, sid, displayFactor, minPriceIncrement, outright_max_depth, implied_max_depth, m_channel_id)));
      //cout << "m_security_definitions.size()=" << m_security_definitions.size() << " totNumReports=" << totNumReports << endl;
      if(m_security_definitions.size() == totNumReports)
      {
        if(m_stop_listening_to_instrument_defs_after_one_loop)
        {
          m_finished_handling_instruments = true;
	  m_logger->log_printf(Log::INFO, "CME finished handling %d instruments on channel %d", totNumReports, m_channel_id);
        }
      }
    }
  }

  template<typename MDP_VERSION>
  template<typename T>
  void CME_MDP_3_Channel_Handler<MDP_VERSION>::handle_instrument_msg(T& msg, double unit_of_measure_qty)
  {
    string symbol(msg.symbol);
    uint32_t securityId = msg.security_id;
    //cout << "symbol=" << symbol << " securityId=" << securityId << endl;
    double displayFactor = cme_to_double_7(msg.display_factor);
    //cout << " displayFactor=" << displayFactor;
    double minPriceIncrement(0.0);
    if(!try_get_min_price_increment<T>(msg, minPriceIncrement))
    {
      //cout << "No min_price_increment.  This is a VTT symbol.  Using display factor, arbitrarily" << endl;
      minPriceIncrement = displayFactor;
    }
    //cout << " minPriceIncrement=" << minPriceIncrement;
    if(!msg.tot_num_reports.has_value())
    {
      //cout << "Warning: tot_num_reports has no value" << endl;
    }
    uint32_t totNumReports = msg.tot_num_reports.value;
    //cout << " totNumReports=" << totNumReports;

    //cout << " no_events().blockLength=" << msg.no_events().blockLength;
    //cout << " no_events().numInGroup=" << static_cast<int>(msg.no_events().numInGroup);
    //cout << " no_md_feed_types().blockLength=" << msg.no_md_feed_types().blockLength;
    //cout << " no_md_feed_types().numInGroup=" << static_cast<int>(msg.no_md_feed_types().numInGroup);
    uint32_t outright_max_depth = 0;
    uint32_t implied_max_depth = 0;
    for(int i = 0; i < static_cast<int>(msg.no_md_feed_types().numInGroup); ++i)
    {
      typename T::md_feed_types_t& entry = msg.md_feed_types(i);
      string feedType(entry.md_feed_type, sizeof(entry.md_feed_type));
      if(feedType == "GBX")
      {
        outright_max_depth = static_cast<uint32_t>(entry.market_depth);
      }
      else if(feedType == "GBI")
      {
        implied_max_depth = static_cast<uint32_t>(entry.market_depth);
      }
      else
      {
	m_logger->log_printf(Log::WARN, "CME Unrecognized feedType '%s' on channel_id=%d", feedType.c_str(), m_channel_id);
      }
    }
    //cout << " outright_max_depth=" << outright_max_depth;
    //cout << " implied_max_depth=" << implied_max_depth;

    //for(int i = 0; i < static_cast<int>(msg.no_events().numInGroup); ++i)
    //{
    //  typename T::events_t& event = msg.events(i);
    //  //cout << " events[" << i << "].event_type=" << static_cast<uint32_t>(event.event_type.value);
    //  //cout << " eventts[" << i << "].event_time=" << event.event_time;
    //}

    string security_group(msg.security_group);
    //cout << " security_group='" << security_group << "'";
    uint32_t market_segment_id = static_cast<uint32_t>(msg.market_segment_id);
    //cout << " market_segment_id=" << static_cast<uint32_t>(msg.market_segment_id);


    unsigned maturity_month_year = (static_cast<unsigned>(msg.maturity_month_year.year.value) * 100) + static_cast<unsigned>(msg.maturity_month_year.month.value);

    double trading_reference_price = 0.0;
    if(msg.trading_reference_price.mantissa.has_value())
    {
      trading_reference_price = cme_to_double_7_nullable(msg.trading_reference_price) * displayFactor;
    }

    int open_interest_qty = 0;
    if(msg.open_interest_qty.has_value()) {
      open_interest_qty = msg.open_interest_qty.value;
    }

    int cleared_volume = 0;
    if(msg.cleared_volume.has_value()) {
      cleared_volume = msg.cleared_volume.value;
    }

    uint32_t settle_price_type = static_cast<uint32_t>(*reinterpret_cast<uint8_t*>(&(msg.settl_price_type)));

    AddInstrumentDefinition(securityId, symbol, maturity_month_year, displayFactor, minPriceIncrement, unit_of_measure_qty, outright_max_depth, 
                            implied_max_depth, totNumReports, security_group, true, market_segment_id, trading_reference_price, open_interest_qty, cleared_volume,
                            settle_price_type);
  }

  template<typename MDP_VERSION>
  size_t CME_MDP_3_Channel_Handler<MDP_VERSION>::handle_instrument(size_t context, uint8_t* buf, size_t len)
  {
    if(!m_finished_handling_instruments)
    {
      //const CME_MDP_3_Common::packet_header& packet_header = reinterpret_cast<const CME_MDP_3_Common::packet_header&>(*buf);
      //uint32_t seq_no = ntohl(packet_header.seq_no);
      uint8_t* buf_ptr = buf + sizeof(CME_MDP_3_Common::packet_header);
      uint8_t* end_ptr = buf + len;
      int count = 0;
      while(buf_ptr < end_ptr)
      {
        //cout << "msg" << endl;
        const CME_MDP_3_Common::message_header& message_header = reinterpret_cast<const CME_MDP_3_Common::message_header&>(*buf_ptr);
        uint16_t msg_size = message_header.msg_size;
        uint16_t template_id = message_header.template_id;
        //cout << "count=" << count << " msg_size=" << msg_size << " template_id=" << template_id << endl;
        ++count;
        buf_ptr += sizeof(CME_MDP_3_Common::message_header);
        switch(template_id)
        {
          case 27:
            {
              typename MDP_VERSION::MDInstrumentDefinitionFuture27& instrument_def = reinterpret_cast<typename MDP_VERSION::MDInstrumentDefinitionFuture27&>(*buf_ptr);
              double unit_of_measure_qty = cme_to_double_7_nullable(instrument_def.unit_of_measure_qty); 
              handle_instrument_msg<typename MDP_VERSION::MDInstrumentDefinitionFuture27>(instrument_def, unit_of_measure_qty);
            }
          break;
          case 29:
            {
              typename MDP_VERSION::MDInstrumentDefinitionSpread29& instrument_def = reinterpret_cast<typename MDP_VERSION::MDInstrumentDefinitionSpread29&>(*buf_ptr);
              double unit_of_measure_qty = 0.0; 
              handle_instrument_msg<typename MDP_VERSION::MDInstrumentDefinitionSpread29>(instrument_def, unit_of_measure_qty);
            }
          break;
          case 41:
            {
              typename MDP_VERSION::MDInstrumentDefinitionOption41& instrument_def = reinterpret_cast<typename MDP_VERSION::MDInstrumentDefinitionOption41&>(*buf_ptr);
              double unit_of_measure_qty = cme_to_double_7_nullable(instrument_def.unit_of_measure_qty); 
              handle_instrument_msg<typename MDP_VERSION::MDInstrumentDefinitionOption41>(instrument_def, unit_of_measure_qty);
            }
          break;
          default:
            {
	      m_logger->log_printf(Log::WARN, "CME handle_instrument::Unrecognized template_id %d", template_id);
            }
            break;
        }
        buf_ptr += (msg_size - sizeof(CME_MDP_3_Common::message_header));;
      }
    }
    return len;
  }

  template<typename MDP_VERSION>
  CME_MDP_3_Channel_Handler<MDP_VERSION>::~CME_MDP_3_Channel_Handler()
  {
  }
}
