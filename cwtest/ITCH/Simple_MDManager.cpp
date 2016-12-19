#include <ITCH/Simple_MDManager.h>
#include <Util/ObjectFactory.h>
#include <Exchange/Exchange_Manager.h>
#include <Interface/TimerRequest.h>
#include <Interface/TimerUpdate.h>
#include <Event/Disruptor_Queue.h>
#include <MD/MDCache.h>
#include <MD/MDOrderBook.h>
#include <Product/Product_Manager.h>
#include <Interface/SystemStatus.h>
#include <Interface/QuoteUpdate.h>
#include <Interface/QuoteUpdateSubscribe.h>
#include <Logger/Logger_Manager.h>
#include <Event_Hub/Event_Hub.h>
#include <ASIO/IO_Service.h>
#include <Data/Handler.h>
#include <Util/CSV_Parser.h>
#include <Util/Price.h>
#include <app/app.h>

#include <boost/foreach.hpp>

namespace hf_cwtest
{
	using namespace hf_core;

  const string Simple_MDManager::m_name = "Simple_MDManager";
  
  Simple_MDManager::Simple_MDManager(Application* app)
    : m_app(app), MDManager(app)
  {}

  Simple_MDManager::~Simple_MDManager(void)
  {}

MDWatchdogRP Simple_MDManager::watchdog() { return m_mdwatchdog; }
void Simple_MDManager::set_watchdog(MDWatchdogRP w) { 
	m_mdwatchdog = w; 
}

void Simple_MDManager::set_logger(LoggerRP l){
	m_logger = l; 
}

LoggerRP Simple_MDManager::logger(void) { 
	return m_logger;  
}

  void Simple_MDManager::init(const Parameter& params_)
  {
    m_logger=m_app->lm()->get_logger("MD");
    
    m_subscribers=boost::shared_ptr<MDSubscriptionAcceptor>(new MDSubscriptionAcceptor(*m_app));
    
    // init subscription mappings
    if(params_.has("submap")) {
      string submap=params_["submap"].getString();
      m_mappings.init(submap,*m_logger,*m_app);
    }
    
    bool mdcache_client = params_.getDefaultBool("mdcache_client", false);
    if(mdcache_client) {
      m_subscribers->use_composite_update();
    }
    
   /* 
    // init providers
    if (params_.has("providers")) {
      init_providers(params_["providers"]);
      m_app->hub()->subscribe(Event_Type::QuoteUpdateSubscribe, this);
      m_app->hub()->subscribe(Event_Type::TradeUpdateSubscribe, this);
      m_app->hub()->subscribe(Event_Type::SystemStatus, this);
      m_app->hub()->subscribe(Event_Type::TimerRequest, this);
    }
    
    // add admins
    m_admin_domain="md";
    m_admin_handler.add_command(m_admin_domain+"/quote", boost::bind(&Simple_MDManager::print_quote_admin, this, _1),"Display Top Of Book: quote symbol=<sym> (or 'all')");
    m_admin_handler.add_command(m_admin_domain+"/book", boost::bind(&Simple_MDManager::print_book_admin, this, _1),"Display Full Book: book symbol=<sym>");
    m_admin_handler.add_command(m_admin_domain+"/imbalance", boost::bind(&Simple_MDManager::print_imbalance_admin, this, _1),"Display Imbalance data: imbalance symbol=<sym>");
    m_admin_handler.add_command(m_admin_domain+"/update_quote", boost::bind(&Simple_MDManager::update_quote_admin, this, _1),"Update Quote: symbol=<sym> bid=<price> ask=<price> bid_size=<size> ask_size=<size> [exch=<exch>]");
    m_admin_handler.add_command(m_admin_domain+"/set_value", boost::bind(&Simple_MDManager::set_value_admin, this, _1),"Set MDCache values: symbol=<sym> [exch=<exch> (default: primary)] [close_price=<price> size=<size>] [open_price=<price> size=<size>] [last_trade=<price>] [ref_price=<price>] [far_price=<price>] [near_price=<price>] [paired_size=<size>] [imbalance_size=<size>] [imbalance_direction=B|S|O] [indicative_price_flag=I|O|C|H]");
    m_admin_handler.add_command(m_admin_domain+"/read_values", boost::bind(&Simple_MDManager::read_values_admin, this, _1),"Set MDCache values from CSV file: read_values file=NAME [ sid,close_price ]");
    m_admin_handler.add_command(m_admin_domain+"/status", boost::bind(&Simple_MDManager::status_admin, this, _1),"Simple_MDManager status");
    m_admin_handler.add_command(m_admin_domain+"/symbol_status", boost::bind(&Simple_MDManager::symbol_status_admin, this, _1),"Simple_MDManager per-symbol status");
    m_admin_handler.add_command(m_admin_domain+"/start_provider", boost::bind(&Simple_MDManager::provider_change_admin, this, _1, false),"start_provider name=ProviderName [group=<line>]");
    m_admin_handler.add_command(m_admin_domain+"/stop_provider", boost::bind(&Simple_MDManager::provider_change_admin, this, _1, true),"stop_provider name=ProviderName [group=<line>]");
    m_admin_handler.add_command(m_admin_domain+"/enable_exchange", boost::bind(&Simple_MDManager::enable_exchange_admin, this, _1, true), "enable/disable exchange quoting  exchange=EXCH enable");
    m_admin_handler.add_command(m_admin_domain+"/disable_exchange", boost::bind(&Simple_MDManager::enable_exchange_admin, this, _1, false), "enable/disable exchange quoting  exchange=EXCH enable");
    m_admin_handler.add_command(m_admin_domain+"/level2_book", boost::bind(&Simple_MDManager::print_level2_book_admin, this, _1),"Display Full Book: level2_book provider=<prov> symbol=<sym> [merged=<true|false>] [exclude_invalid=<true|false>]");
    m_admin_handler.add_command(m_admin_domain+"/clear_order_pool", boost::bind(&Simple_MDManager::clear_order_pool_admin, this, _1),"Clear OrderBook Pool: clear_order_pool provider=<prov>");
    m_admin_handler.add_command(m_admin_domain+"/clear_level_pool", boost::bind(&Simple_MDManager::clear_level_pool_admin, this, _1),"Clear LevelPool: clear_level_pool provider=<prov>");
    m_admin_handler.add_command(m_admin_domain+"/clear_order_book", boost::bind(&Simple_MDManager::clear_order_book_admin, this, _1),"Clear OrderBook: clear_order_book provider=<prov> [symbol=<sym>] [delete_orders=true]");
    m_admin_handler.add_command(m_admin_domain+"/invalidate_quotes", boost::bind(&Simple_MDManager::invalidate_quote_admin, this, _1),"Invalidate Exchange Quotes: invalidate_quotes exchange=<exch> [symbol=<sym>]");
    m_admin_handler.add_command(m_admin_domain+"/dispatcher_status", boost::bind(&Simple_MDManager::dispatcher_status_admin, this, _1),"Show dispatcher status");
    m_admin_handler.add_command(m_admin_domain+"/shmem_status", boost::bind(&Simple_MDManager::shmem_status_admin, this, _1),"Show shared memory status");
    m_admin_handler.add_command(m_admin_domain+"/drop_packets", boost::bind(&Simple_MDManager::drop_packets_admin, this, _1), "drop_packets name=<provider> [num_packets=<N>] drop N packets from feed to test gap recovery");
    m_admin_handler.add_command(m_admin_domain+"/monitor_activity", boost::bind(&Simple_MDManager::monitor_activity_admin, this, _1), "monitor_activity name=<provider> [set=<True/False>] get or set monitor_activity status -- disable/enable MDWatchdog");
    
   
    m_app->hub()->post(AdminRequestSubscribe(m_admin_domain,this,m_admin_handler.help_strings(m_admin_domain)));
    */
  }

  void Simple_MDManager::start(void)
  {
    BOOST_FOREACH(MDProviderMap_t::value_type& itr, m_providers) {
      if(!itr.second->is_standby()) {
        itr.second->start();
      }
    }
    
    for(MDDispatcherMap_t::const_iterator itr=m_dispatchers.begin(), itr_end = m_dispatchers.end(); itr != itr_end; ++itr) {
      itr->second->run();
    }
  }

  void Simple_MDManager::stop(void)
  {
    BOOST_FOREACH(MDProviderMap_t::value_type& itr, m_providers) {
      itr.second->stop();
    }
  }
  
  void Simple_MDManager::init_providers(const Parameter& params_)
  {
    const Parameter::ParameterDict& dict=params_.getDict();
    BOOST_FOREACH(Parameter::ParameterDict::value_type itr, dict) {
      init_provider(itr.first, itr.second);
    }
  }
  
  void Simple_MDManager::init_provider(const string& name_,const Parameter& params_)
  {
    if (params_.has("classname")) return; //should not handle that type of provider(temporary)

    m_logger->log_printf(Log::INFO,"Simple_MDManager::init_provider provider=%s",name_.c_str());

    string type=params_["type"].getString();

    MDProvider* provider=ObjectFactory1<string,MDProvider,Simple_MDManager&>::allocate(type,*this);
    if (!provider)
      {
	m_logger->log_printf(Log::ERROR,"MDProvider::init_provider: failed to allocate provider with type=%s",
			     type.c_str());
	throw runtime_error("Simple_MDManager::can not allocate MD provider");
      }
    
    if(m_mdwatchdog) {
      provider->set_watchdog_params(m_mdwatchdog->config());
    }
    
    provider->init(name_,params_);
    m_providers.insert(MDProviderMap_t::value_type(name_,MDProviderRP(provider)));
    if(m_mdwatchdog) {
      m_mdwatchdog->add_provider(provider);
    }

    m_logger->log_printf(Log::INFO,"Simple_MDManager::init_provider success provider=%s,type=%s",
			 name_.c_str(),type.c_str());
  }

  void Simple_MDManager::add_provider(const string& name_,const Parameter& params_, MDProviderRP provider_)
  {

    string type=params_["type"].getString();
    if(!provider_.get()) {
      m_logger->log_printf(Log::ERROR,"MDProvider::add_provider: null provider passed with type=%s",type.c_str());
      throw runtime_error("Simple_MDManager::unable to add null MD provider");
    }
    
    provider_->init(name_, params_);
    m_providers.insert(MDProviderMap_t::value_type(name_, provider_));
    if(m_mdwatchdog) {
      m_mdwatchdog->add_provider(provider_.get());
    }
    m_logger->log_printf(Log::INFO,"Simple_MDManager::add_provider success provider=%s,type=%s",
                         name_.c_str(),type.c_str());
    
    bool register_books = true;
    
    if(provider_->handler() && provider_->handler()->record_only()) {
      register_books = false;
    }
    
    if(register_books) {
      for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator itr = provider_->order_books().begin(), i_end = provider_->order_books().end();
          itr != i_end; ++itr) {
        m_logger->log_printf(Log::INFO, "Simple_MDManager::add_provider registering provider=%s orderbook with MDCache exch %s", name_.c_str(), (*itr)->exch().str());
        
        const Exchange& exch = m_app->em()->find_by_primary((*itr)->exch());
        int exch_index = exch.index();
        int max_product_id = m_app->pm()->max_product_id();
        for(int p = 0; p < max_product_id; ++p) {
          const ExchangeBook& eb_c = m_app->mdcache()->get_book(p).exchange(exch_index);
          ExchangeBook& eb = const_cast<ExchangeBook&>(eb_c);
          eb.set_order_book(&(*itr)->book(p));
        }
        m_app->mdcache()->set_order_book_handler(exch_index, itr->get());
      }
      
      for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator itr = provider_->secondary_order_books().begin(), i_end = provider_->secondary_order_books().end();
          itr != i_end; ++itr) {
        m_logger->log_printf(Log::INFO, "Simple_MDManager::add_provider registering provider=%s secondary orderbook with MDCache exch %s", name_.c_str(), (*itr)->exch().str());
        
        const Exchange& exch = m_app->em()->find_by_primary((*itr)->exch());
        int exch_index = exch.index();
        int max_product_id = m_app->pm()->max_product_id();
        for(int p = 0; p < max_product_id; ++p) {
          const ExchangeBook& eb_c = m_app->mdcache()->get_book(p).exchange(exch_index);
          ExchangeBook& eb = const_cast<ExchangeBook&>(eb_c);
          eb.set_secondary_order_book(&(*itr)->book(p));
        }
        m_app->mdcache()->set_secondary_order_book_handler(exch_index, itr->get());
      }
    }
    
  }

  void Simple_MDManager::accept_quote_subscribe(const QuoteUpdateSubscribe& s)
  {
    // store subscriber
    m_subscribers->accept_quote_subscribe(s);
    handle_subscription(s.product(),s.exchange());
  }

  void Simple_MDManager::accept_trade_subscribe(const TradeUpdateSubscribe& s)
  {
    // store subscriber
    m_subscribers->accept_trade_subscribe(s);
    handle_subscription(s.product(),s.exchange());
  }


  void Simple_MDManager::handle_subscription(const ProductID& product_,const ExchangeID& exch_)
  {
    BOOST_FOREACH(MDProviderMap_t::value_type& itr, m_providers) {
      itr.second->handle_subscription(product_,exch_);
    }
  }

  void Simple_MDManager::accept_system_status_update(const SystemStatusUpdate& ss)
  {
    if(ss.status() == SystemStatus::shutdown)
      {
	m_subscribers->stop();

        for(MDProviderMap_t::const_iterator itr=m_providers.begin(), itr_end = m_providers.end(); itr!=itr_end; ++itr) {
          itr->second->stop();
        }

        for(MDDispatcherMap_t::const_iterator d = m_dispatchers.begin(), d_end = m_dispatchers.end(); d != d_end; ++d) {
          d->second->full_stop();
        }
      }
  }
  
  void Simple_MDManager::accept_admin_request(AdminRequest& request_)
  {
    m_admin_handler.accept_admin_request(request_);
  }

  void Simple_MDManager::print_quote_admin(AdminRequest& request_)
  {
    const string& symbol = request_.get_param("symbol");
    if(symbol.empty()) {
      request_.printf("Invalid symbol");
      return;
    }

    if(symbol == "all") {
      request_.set_content_type(HTTP_Headers::MIME_Type::csv);
      request_.printf("time,epoch_time,symbol,sid,bid,ask,bid_size,ask_size,last_price,prim_last_price,volume,clean_volume,open_price,open_volume,close_price,close_volume,quote_status,market_status,adj_close,ref_price\n");
      
      Time cur_time = Time::current_time();
      char cur_time_buf[64];
      Time_Utils::print_time(cur_time_buf, cur_time, Timestamp::MILI);
      
      uint64_t epoch_time = cur_time.total_sec();
      
      Quote quote;
      
      for(Product_Manager::ProductSet_t::const_iterator p = m_app->pm()->active_products().begin(),
	    p_end = m_app->pm()->active_products().end(); p != p_end; ++p) {
        
	const CompositeBook& cq = m_app->mdcache()->get_book((*p)->product_id());
	const Exchange& primary_exch = m_app->em()->find_by_primary((*p)->primary_exch());
	const ExchangeBook& ebook = cq.exchange(primary_exch.index());

	cq.best_quote(quote);
        
	double last = cq.last_trade();
	double prim_last = ebook.last_trade();
	int volume = cq.volume();
	int clean_volume = cq.clean_volume();
	double open_price  = 0.0;
	double close_price = 0.0;
	int open_volume=0.0;
	int close_volume=0.0;
        double adj_close = (*p)->get_double(Product::idx::adj_close, 0.0);
	double ref_price = m_app->mdcache()->mark_price((*p)->product_id());

	if(primary_exch.index() != Exchange::invalid_index) {
	  open_price  = cq.exchange(primary_exch.index()).open_price();
	  close_price = cq.exchange(primary_exch.index()).close_price();
	  open_volume = cq.exchange(primary_exch.index()).open_volume();
	  close_volume  = cq.exchange(primary_exch.index()).close_volume();
	}

	request_.printf("%s,%ld,%s,%ld,%0.*f,%0.*f,%d,%d,%0.*f,%0.*f,%d,%d,%0.*f,%d,%0.*f,%d,%s,%s,%0.*f,%0.*f\n",
			cur_time_buf, epoch_time, (*p)->symbol().c_str(), (*p)->sid(),
			Price::GetMaxNumDecimals(), quote.bid.value(), Price::GetMaxNumDecimals(), quote.ask.value(),
			quote.bid_size,quote.ask_size,
			Price::GetMaxNumDecimals(), last, Price::GetMaxNumDecimals(), prim_last, volume, clean_volume,
			Price::GetMaxNumDecimals(), open_price, open_volume, Price::GetMaxNumDecimals(), close_price, close_volume,
			quote.quote_status.str(), quote.market_status.str(),
                        Price::GetMaxNumDecimals(), adj_close, Price::GetMaxNumDecimals(), ref_price);
      }
      return;
    }


    const Product* product=m_app->pm()->find_by_symbol(symbol);
    if (!product) {
      request_.printf("Symbol %s not found",symbol.c_str());
      return;
    }

    Quote quote;
    m_app->mdcache()->get_book(product->product_id()).best_quote(quote);

    double last=m_app->mdcache()->last_trade(product->product_id());

    request_.printf("BID=%0.4f,ASK=%0.4f,BID_SIZE=%d,ASK_SIZE=%d,LAST=%0.4f,Status=%s,MktStatus=%s",
		    quote.bid.value(),quote.ask.value(),
		    quote.bid_size,quote.ask_size,last,
		    quote.quote_status.str(), quote.market_status.str());
  }

  struct ExchQuote {
    ExchangeID exch;
    Quote      q;

    struct best_bid {
      bool operator()(const ExchQuote& q1, const ExchQuote& q2) const {
	return Quote::best_bid()(q1.q, q2.q);
      }
    };
    struct best_ask {
      bool operator()(const ExchQuote& q1, const ExchQuote& q2) const {
	return Quote::best_ask()(q1.q, q2.q);
      }
    };
  };

  void Simple_MDManager::print_book_admin(AdminRequest& req)
  {
    const string& symbol = req.get_param("symbol");
    if(symbol.empty()) {
      req.printf("Please specify symbol");
      return;
    }

    const Product* product=m_app->pm()->find_by_symbol(symbol);
    if (!product) {
      req.printf("Symbol '%s' not found",symbol.c_str());
      return;
    }

    const CompositeBook& book = m_app->mdcache()->get_book(product->product_id());

    vector<ExchQuote> q(book.size());

    size_t idx = 0;
    for(CompositeBook::q_t::const_iterator i = book.begin(), i_end = book.end();
	i != i_end; ++idx, ++i) {
      q[idx].exch = m_app->em()->find_by_index(idx)->primary_id();
      (*i)->quote(q[idx].q);
    }

    vector<ExchQuote> bids(q);
    vector<ExchQuote> asks(q);

    sort(bids.begin(), bids.end(), ExchQuote::best_bid());
    sort(asks.begin(), asks.end(), ExchQuote::best_ask());

    req.printf("%-12s %-10s %-7s  %10s    %-10s %-7s %-6s %-12s\n",
	       "TIME", "EXCH", "SIZE", "BIDS", "ASKS", "SIZE", "EXCH", "TIME");

    char bid_buf[32];
    char bid_time_buf[32];
    char ask_time_buf[32];
    char vol_time_buf[32];
    char open_time_buf[32];
    char close_time_buf[32];
    for(idx = 0; idx < q.size(); ++idx) {
      snprintf(bid_buf, 32, "$%.*f", Price::GetMaxNumDecimals(), bids[idx].q.bid.value());

      bid_time_buf[0] = '\0';
      ask_time_buf[0] = '\0';
      if(bids[idx].q.bid_time > Time::today()) {
	Time_Utils::print_time(bid_time_buf, bids[idx].q.bid_time, Timestamp::MILI);
      }
      if(asks[idx].q.ask_time > Time::today()) {
	Time_Utils::print_time(ask_time_buf, asks[idx].q.ask_time, Timestamp::MILI);
      }

      req.printf("%-12s %-10s %-7d  %10s   $%-10.*f %-7d %-6s %-12s\n",
		 bid_time_buf, bids[idx].exch.str(), bids[idx].q.bid_size, bid_buf,
		 Price::GetMaxNumDecimals(), asks[idx].q.ask.value(), asks[idx].q.ask_size, asks[idx].exch.str(), ask_time_buf);
    }

    req.printf("\n%-10s %-8s %-12s %-9s %-4s %-5s %-10s %-15s %-15s %-15s %-15s %-10s %-10s %-15s %-10s %-10s %-15s\n",
               "Exchange", "Enabled", "QuoteStatus", "MktStatus", "SSR", "RPI", "LastTrade", "LastTradeTime","Volume", "CleanVolume", "VolumeTime",
               "OpenPrice","OpenVolume","OpenPriceTime","ClosePrice","CloseVolume","ClosePriceTime");
    Time today=Time::today();
    for(idx = 0; idx < q.size(); ++idx) {
      const ExchangeBook& eb = book.exchange(idx);

      bool enabled = this->subscribers().publish_quotes(q[idx].exch);

      bid_time_buf[0] = '\0';
      if(eb.last_trade_time() > today) {
	Time_Utils::print_time(bid_time_buf, eb.last_trade_time(), Timestamp::MILI);
      }

      vol_time_buf[0]='\0';
      if (eb.volume_time() > today) {
        Time_Utils::print_time(vol_time_buf, eb.volume_time(), Timestamp::MILI);
      }
      
      open_time_buf[0]='\0';
      if (eb.open_price_time() > today) {
        Time_Utils::print_time(open_time_buf, eb.open_price_time(), Timestamp::MILI);
      }
      
      close_time_buf[0]='\0';
      if (eb.close_price_time() > today) {
        Time_Utils::print_time(close_time_buf, eb.close_price_time(), Timestamp::MILI);
      }
      
      const char* ssr = "";
      switch(eb.short_restriction_status().index()) {
      case SSRestriction::None: ssr = "None"; break;
      case SSRestriction::Activated: ssr = "Act"; break;
      case SSRestriction::Continued: ssr = "Cont"; break;
      }
      const char* rpi = "";
      switch(eb.retail_price_improvement().index()) {
      case RetailPriceImprovement::None: rpi = "None"; break;
      case RetailPriceImprovement::Bid: rpi = "Bid"; break;
      case RetailPriceImprovement::Offer: rpi = "Offer"; break;
      case RetailPriceImprovement::Both: rpi = "Both"; break;
      }
      
      req.printf("%-10s %-8s %-12s %-9s %-4s %-5s %-10.*f %-15s %-15d %-15d %-15s %-10.*f %-10d %-15s %-10.*f %-10d %-15s\n",
		 q[idx].exch.str(),enabled ? "true" : "false", q[idx].q.quote_status.value(), q[idx].q.market_status.value(),
                 ssr, rpi,
		 Price::GetMaxNumDecimals(), eb.last_trade(), bid_time_buf,eb.volume(),eb.clean_volume(),vol_time_buf,
		 Price::GetMaxNumDecimals(), eb.open_price(), eb.open_volume(),open_time_buf,
                 Price::GetMaxNumDecimals(), eb.close_price(),eb.close_volume(),close_time_buf);
    }
    
    req.printf("%-10s %-8s %-12s %-9s %-4s %-5s %-10.*f %-15s %-15d %-15d\n",
               "Composite", "", "", "", "", "", Price::GetMaxNumDecimals(), book.last_trade(), "", book.volume(), book.clean_volume());
  }

  void Simple_MDManager::print_imbalance_admin(AdminRequest& req)
  {
    const string& symbol = req.get_param("symbol");
    if(symbol.empty()) {
      req.printf("Please specify symbol");
      return;
    }
    
    const Product* product=m_app->pm()->find_by_symbol(symbol);
    if (!product) {
      req.printf("Symbol '%s' not found",symbol.c_str());
      return;
    }
    
    req.set_content_type(HTTP_Headers::MIME_Type::csv);
    
    const CompositeBook& book = m_app->mdcache()->get_book(product->product_id());
    
    req.printf("%s,%s,%s,%s,%s,%s,%s,%s\n", "exchange", "near_price", "far_price", "paired_size", "imbalance_size", "imbalance_direction", "indicative_price_flag", "ref_price");
    
    size_t idx = 0;
    for(CompositeBook::q_t::const_iterator i = book.begin(), i_end = book.end(); i != i_end; ++idx, ++i) {
      const ExchangeBook* eb = *i;
      req.printf("%s,%0.4f,%0.4f,%d,%d,%c,%c,%0.4f\n",
                 eb->exchange_id().str(), eb->near_price(), eb->far_price(), eb->paired_size(), eb->imbalance_size(),
                 eb->imbalance_direction(), eb->indicative_price_flag(), eb->ref_price());
    }
  }
  
  void Simple_MDManager::print_level2_book_admin(AdminRequest& req)
  {
    const string& symbol = req.get_param("symbol");
    if(symbol.empty()) {
      req.printf("Please specify symbol");
      return;
    }
    
    const Product* product=m_app->pm()->find_by_symbol(symbol);
    if (!product) {
      req.printf("Symbol '%s' not found",symbol.c_str());
      return;
    }
    
    int flags = CompositeBook::INCLUDE_INVALID;
    const string& merged_str = req.get_param("merged");
    if(!merged_str.empty() && merged_str != "false") {
      flags |= CompositeBook::MERGED;
    }
    
    const string& exclude_invalid = req.get_param("exclude_invalid");
    if(!exclude_invalid.empty() && exclude_invalid != "false") {
      flags &= ~CompositeBook::INCLUDE_INVALID;
    }
    
    const string& secondary_str = req.get_param("secondary");
    if(!secondary_str.empty() && secondary_str != "false") {
      flags |= CompositeBook::SECONDARY;
    }
    
    MDProviderRP provider;
    const string& provider_name = req.get_param("provider");
    if(!provider_name.empty()) {
      MDProviderMap_t::iterator pitr=m_providers.find(provider_name);
      if (pitr==m_providers.end()) {
        req.printf("No provider with name: %s", provider_name.c_str());
        return;
      }
      provider=(*pitr).second;
    }

    int num_levels = 5;
    
    const string& num_levels_str = req.get_param("num_levels");
    if(!num_levels_str.empty()) {
      num_levels = strtol(num_levels_str.c_str(), 0, 10);
    }
    
    if(provider) {
      const string& disp_orders_str = req.get_param("disp_orders");
      bool disp_orders = !disp_orders_str.empty();
      
      if(flags & CompositeBook::SECONDARY) {
        if(provider->secondary_order_books().empty()) {
          req.printf("Provider does not contain a secondary level2 order book");
          return;
        }
        for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator itr = provider->secondary_order_books().begin(), i_end = provider->secondary_order_books().end();
            itr != i_end; ++itr) {
          (*itr)->disp_book(req, product->product_id(), num_levels, disp_orders);
        }
      } else {
        if(provider->order_books().empty()) {
          req.printf("Provider does not contain a level2 order book");
          return;
        }
        for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator itr = provider->order_books().begin(), i_end = provider->order_books().end();
            itr != i_end; ++itr) {
          (*itr)->disp_book(req, product->product_id(), num_levels, disp_orders);
        }
      }
    } else {
      list<MDExchPriceLevel> bid_levels, ask_levels;
      const CompositeBook& book = m_app->mdcache()->get_book(product->product_id());
      book.get_price_levels(num_levels, bid_levels, ask_levels, flags);
      MDOrderBookHandler::print_levels(req, bid_levels, ask_levels);
    }

    return;
  }
  
  void Simple_MDManager::clear_order_pool_admin(AdminRequest& req)
  {
    const string& provider_name = req.get_param("provider");
    if(provider_name.empty()) {
      req.printf("Please specify provider");
      return;
    }

    MDProviderMap_t::iterator pitr=m_providers.find(provider_name);
    if (pitr==m_providers.end()) {
      req.printf("No provider with name: %s", provider_name.c_str());
      return;
    }
    MDProviderRP provider=(*pitr).second;
    
    if(provider->order_books().empty()) {
      req.printf("Provider does not contain a level2 order book");
      return;
    }
    
    for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator itr = provider->order_books().begin(), i_end = provider->order_books().end();
        itr != i_end; ++itr) {
      (*itr)->clear_order_pool();
    }
    
    req.printf("OrderBook pool cleared\n");
  }
  
  void Simple_MDManager::clear_level_pool_admin(AdminRequest& req)
  {
    const string& provider_name = req.get_param("provider");
    if(provider_name.empty()) {
      req.printf("Please specify provider");
      return;
    }
    
    MDProviderMap_t::iterator pitr=m_providers.find(provider_name);
    if (pitr==m_providers.end()) {
      req.printf("No provider with name: %s", provider_name.c_str());
      return;
    }
    MDProviderRP provider=(*pitr).second;
    
    if(provider->order_books().empty()) {
      req.printf("Provider does not contain a level2 order book");
      return;
    }
    
    for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator itr = provider->order_books().begin(), i_end = provider->order_books().end();
        itr != i_end; ++itr) {
      (*itr)->clear_level_pool();
    }
    
    req.printf("Level pool cleared\n");
  }
  
  void Simple_MDManager::clear_order_book_admin(AdminRequest& req)
  {
    const string& provider_name = req.get_param("provider");
    if(provider_name.empty()) {
      req.printf("Please specify provider");
      return;
    }
    
    MDProviderMap_t::iterator pitr=m_providers.find(provider_name);
    if (pitr==m_providers.end()) {
      req.printf("No provider with name: %s", provider_name.c_str());
      return;
    }
    MDProviderRP provider=(*pitr).second;
    
    if(provider->order_books().empty()) {
      req.printf("Provider does not contain a level2 order book");
      return;
    }
    
    bool delete_orders = false;
    const string& delete_orders_str = req.get_param("delete_orders");
    delete_orders = (delete_orders_str == "true");
    
    const string& symbol = req.get_param("symbol");
    
    for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator itr = provider->order_books().begin(), i_end = provider->order_books().end();
        itr != i_end; ++itr) {
      if(symbol.empty()) {
        (*itr)->clear(delete_orders);
        this->subscribers().post_invalid_quote((*itr)->exch());
      } else {
        const Product* product = m_app->pm()->find_by_symbol(symbol);
        if(product) {
          (*itr)->clear(product->product_id(), delete_orders);
          this->subscribers().post_invalid_quote(product->product_id(), (*itr)->exch());
        } else {
          req.printf("symbol %s not found", symbol.c_str());
          return;
        }
      }
    }    
    
    req.printf("OrderBook cleared\n");
  }
  
  void Simple_MDManager::invalidate_quote_admin(AdminRequest& request_)
  {
    const string& exch_name = request_.get_param("exchange");
    if(exch_name.empty()) {
      request_.printf("Please specify exchange");
      return;
    }
    
    boost::optional<ExchangeID> exch = ExchangeID::get_by_name(exch_name.c_str());
    if(!exch) {
      request_.printf("No exchange with name: %s\n Known exchanges:", exch_name.c_str());
      for(ExchangeID::const_iterator e = ExchangeID::begin(); e != ExchangeID::end(); ++e) {
	request_.printf(" %s", e->str());
      }
      return;
    }
    
    const string& symbol = request_.get_param("symbol");
    
    if(symbol.empty()) {
      this->subscribers().post_invalid_quote(*exch);
      request_.printf("Exchange %s all quotes invalidated", exch->str());
    } else {
      const Product* product = m_app->pm()->find_by_symbol(symbol);
      if(product) {
        this->subscribers().post_invalid_quote(product->product_id(), *exch);
        request_.printf("Exchange %s symbol %s quotes invalidated", exch->str(), symbol.c_str());
      } else {
        request_.printf("symbol %s not found", symbol.c_str());
      }
    }
  }
  
  void Simple_MDManager::update_quote_admin(AdminRequest& req)
  {
    string symbol = req.get_param("symbol");
    
    const Product* product=m_app->pm()->find_by_symbol(symbol);
    if (!product) {
      req.printf("Symbol '%s' not found",symbol.c_str());
      return;
    }
    
    double bid = atof(req.get_param("bid").c_str());
    double ask = atof(req.get_param("ask").c_str());
    int ask_size = atoi(req.get_param("ask_size").c_str());
    int bid_size = atoi(req.get_param("bid_size").c_str());
    
    if (bid<=0 || ask<=0 || bid_size<=0 || ask_size<=0) {
      req.printf("Invalid quote data, must provide bid, ask, ask_size, bid_size.  Optionally exch");
    }
    
    ExchangeID exch=product->primary_exch();
    string exch_str = req.get_param("exch");
    if(!exch_str.empty()) {
      boost::optional<ExchangeID> p=ExchangeID::get_by_name(exch_str.c_str());
      if (p) {
        exch=*p;
      } else {
        req.printf("Exchange '%s' not found", exch_str.c_str());
        return;
      }
    }
    
    QuoteUpdate update(Time::current_time(),product->product_id());;
    update.m_ask=ask;
    update.m_bid=bid;
    update.m_ask_size=ask_size;
    update.m_bid_size=bid_size;
    update.m_exch=exch;
    subscribers().update_subscribers(update);
  }
  
  void Simple_MDManager::read_values_admin(AdminRequest& req) {
    const string& filename = req.get_param("file");
    if(filename.empty()) {
      req.printf("File not specified");
      return;
    }

    try  {
      CSV_Parser parser(filename);
      int sid_idx = parser.column_index("sid");
      int close_price_idx = parser.column_index("close_price");
      int close_size_idx = parser.column_index("close_size");
      int open_price_idx = parser.column_index("open_price");
      int open_size_idx = parser.column_index("open_size");
      int exch_idx = parser.column_index("exchange");

      if (sid_idx < 0) {
	req.printf("No sid column in file=%s",filename.c_str());
	return;
      }
      
      int close_trades=0;
      int open_trades=0;
      while(CSV_Parser::OK == parser.read_row()) {

	char* end = 0;
	Product::sid_t sid = strtoll(parser.get_column(sid_idx), &end, 10);
	const Product* product=m_app->pm()->find_by_sid(sid);

	ExchangeID eid=product->primary_exch();
	if (exch_idx>=0) {
	  boost::optional<ExchangeID> p=ExchangeID::get_by_name(parser.get_column(exch_idx));
	  if (p) eid=*p;
	  else {
	    req.printf("Invalid exchange for product: %s, skipping\n",product->symbol().c_str());
	    continue;
	  }
	}

	if (close_price_idx>=0) {
	  double close_price = atof(parser.get_column(close_price_idx));
	  if(close_price < 0) {
	    req.printf("Invalid close price for symbol=%s, skipping\n",product->symbol().c_str());
	    continue;
	  }
	  int close_size=0;
	  if (close_size_idx>=0) {
	    close_size_idx = atoi(parser.get_column(close_size_idx));
	  }
	  TradeUpdate t;
	  t.set_time(Time::current_time());
	  t.m_id = product->product_id();
	  t.m_price = close_price;
	  t.m_size = close_size;
	  t.m_exch = eid;
	  t.m_trade_status = TradeStatus::CloseTrade;
	  t.m_market_status = MarketStatus::Closed;
	  t.m_is_clean = true;
	  subscribers().update_subscribers(t);
	  close_trades++;
	}

	if (open_price_idx>=0) {
	  double open_price = atof(parser.get_column(open_price_idx));
	  if(open_price < 0) {
	    req.printf("Invalid open price for symbol=%s, skipping\n",product->symbol().c_str());
	    continue;
	  }
	  int open_size=0;
	  if (open_size_idx>=0) {
	    open_size_idx = atoi(parser.get_column(open_size_idx));
	  }
	  TradeUpdate t;
	  t.set_time(Time::current_time());
	  t.m_id = product->product_id();
	  t.m_price = open_price;
	  t.m_size = open_size;
	  t.m_exch = eid;
	  t.m_trade_status = TradeStatus::OpenTrade;
	  t.m_market_status = MarketStatus::Open;
	  t.m_is_clean = true;
	  subscribers().update_subscribers(t);
	  open_trades++;
	}
      }
      req.printf("Processed close_trades=%d,open_trades=%d",close_trades,open_trades);
    }
    catch (runtime_error& error_) {
      req.printf("Error processing file: %s",filename.c_str());
    }
    
  }
  
  void Simple_MDManager::set_value_admin(AdminRequest& req)
  {
    string symbol = req.get_param("symbol");
    
    const Product* product=m_app->pm()->find_by_symbol(symbol);
    if (!product) {
      req.printf("Symbol '%s' not found\n",symbol.c_str());
      return;
    }    
    
    ExchangeID exch_id = product->primary_exch();
    if(req.has_param("exch")) {
      string exch_str = req.get_param("exch");
      boost::optional<ExchangeID> p=ExchangeID::get_by_name(exch_str.c_str());
      if (p) {
        exch_id = *p;
      } else {
        req.printf("Exchange '%s' not found\n", exch_str.c_str());
        return;
      }
    }

    const Exchange& exch = m_app->em()->find_by_primary(exch_id);
    
    if(exch.index() == -1) {
      req.printf("Exchange '%s' not active\n", exch.name().c_str());
      return;
    }
    
    CompositeBook& cb = const_cast<CompositeBook&>(m_app->mdcache()->get_book(product->product_id()));
    ExchangeBook& eb = const_cast<ExchangeBook&>(cb.exchange(exch.index()));
    
    if(req.has_param("close_price")) {
      double close_price = atof(req.get_param("close_price").c_str());
      if(close_price < 0) {
        req.printf("Invalid close price < 0\n");
        return;
      }
      int size = atof(req.get_param("size").c_str());
      
      TradeUpdate t;
      t.set_time(Time::current_time());
      t.m_id = product->product_id();
      t.m_price = close_price;
      t.m_size = size;
      t.m_exch = exch_id;
      t.m_trade_status = TradeStatus::CloseTrade;
      t.m_market_status = MarketStatus::Closed;
      t.m_is_clean = true;
      
      subscribers().update_subscribers(t);
    }
    
    if(req.has_param("open_price")) {
      double open_price = atof(req.get_param("open_price").c_str());
      if(open_price < 0) {
        req.printf("Invalid open price < 0\n");
        return;
      }
      int size = atof(req.get_param("size").c_str());
      
      TradeUpdate t;
      t.set_time(Time::current_time());
      t.m_id = product->product_id();
      t.m_price = open_price;
      t.m_size = size;
      t.m_exch = exch_id;
      t.m_trade_status = TradeStatus::OpenTrade;
      t.m_market_status = MarketStatus::Open;
      t.m_is_clean = true;
      
      subscribers().update_subscribers(t);
    }
    
    if(req.has_param("last_trade")) {
      double last_trade = atof(req.get_param("last_trade").c_str());
      if(last_trade < 0) {
        req.printf("Invalid open price < 0\n");
        return;
      }
      eb.update_last_trade(last_trade);
      req.printf("last_trade set to %0.4f\n", last_trade);
    }
    
    if(req.has_param("ref_price")) {
      double ref_price = atof(req.get_param("ref_price").c_str());
      eb.set_ref_price(ref_price);
      req.printf("ref_price set to %0.4f\n", ref_price);
    }
    
    if(req.has_param("far_price")) {
      double far_price = atof(req.get_param("far_price").c_str());
      eb.set_far_price(far_price);
      req.printf("far_price set to %0.4f\n", far_price);
    }
    
    if(req.has_param("near_price")) {
      double near_price = atof(req.get_param("near_price").c_str());
      eb.set_near_price(near_price);
      req.printf("near_price set to %0.4f\n", near_price);
    }
    
    if(req.has_param("paired_size")) {
      int paired_size = atoi(req.get_param("paired_size").c_str());
      eb.set_paired_size(paired_size);
      req.printf("paired_size set to %d\n", paired_size);
    }
    
    if(req.has_param("imbalance_size")) {
      int imbalance_size = atoi(req.get_param("imbalance_size").c_str());
      eb.set_imbalance_size(imbalance_size);
      req.printf("imbalance_size set to %d\n", imbalance_size);
    }
    
    if(req.has_param("imbalance_direction")) {
      string dir = req.get_param("imbalance_direction");
      if(!dir.empty()) {
        char d = dir[0];
        eb.set_imbalance_direction(d);
        req.printf("imbalance_direction set to %c\n", d);
      }
    }
    
    if(req.has_param("indicative_price_flag")) {
      string flag = req.get_param("indicative_price_flag");
      if(!flag.empty()) {
        char f = flag[0];
        eb.set_indicative_price_flag(f);
        req.printf("indicative_price_flag set to %c\n", f);
      }
    }
    
    
    if(req.has_param("max_spread")) {
      
    }
    
    if(req.has_param("alert_spread")) {
      
    }
    
    if(req.has_param("max_cross")) {
      
    }
    
    if(req.has_param("max_trade_delta")) {
      
    }
    
  }
  
  
  struct exch_data {
    const MDProvider* provider;
    bool quotes, trades;
    exch_data() : provider(0), quotes(0), trades(0) { }
    exch_data(const MDProvider* p, bool q, bool t) : provider(p), quotes(q), trades(t) { }
  };

  void Simple_MDManager::status_admin(AdminRequest& request_)
  {
    multimap<ExchangeID, exch_data> exchanges;

    request_.printf("Providers: \n");
    const char* pr_str_hdr = "%-10s %-10s %-10s %-10s %-20s %-10s %s\n";
    const char* pr_str = "%-10s %-10s %-10s %-10s %-20d %-10s %s\n";
    request_.printf(pr_str_hdr, "Name", "Type", "Service", "Source", "Num Subscriptions", "Started", "Status");
    MDProviderMap_t::const_iterator itr=m_providers.begin();
    MDProviderMap_t::const_iterator itr_end=m_providers.end();
    for(;itr!=itr_end;++itr)
      {
        string msg;
	const MDProvider& p=*(*itr).second;

        if(p.order_books().empty()) {
          request_.printf(pr_str,
                          p.name().c_str(),p.type().c_str(),p.service().c_str(),p.source().c_str(),
                          p.num_products(), (p.enabled()?"true" : "false"), msg.c_str() );  
        } else {
          for(vector<boost::shared_ptr<MDOrderBookHandler> >::const_iterator book_itr = p.order_books().begin(), i_end = p.order_books().end();
              book_itr != i_end; ++book_itr) {
            msg = (*book_itr)->status_msg();
            
            request_.printf(pr_str,
                            p.name().c_str(),p.type().c_str(),p.service().c_str(),p.source().c_str(),
                            p.num_products(), (p.enabled()?"true" : "false"), msg.c_str() );  
          }
        }
        
        for(vector<boost::shared_ptr<MDOrderBookHandler> >::const_iterator book_itr = p.secondary_order_books().begin(), i_end = p.secondary_order_books().end();
            book_itr != i_end; ++book_itr) {
          msg = (*book_itr)->status_msg();
          request_.printf(pr_str, " secondary", "", "", "",
                          p.num_products(), (p.enabled()?"true" : "false"), msg.c_str() );
        }
        
	for(vector<Supported_Exchange>::const_iterator e = p.supported_exchanges().begin(), e_end = p.supported_exchanges().end();
	    e != e_end; ++e) {
	  exchanges.insert(make_pair(e->exch, exch_data(&p, true, true)));
	}
      }
    
    char last_update[32];
    request_.printf("\nExchanges:\n");
    const char* ex_str = "%-10s %-16s %-15s %-15s %-15s %-15s\n";
    request_.printf(ex_str, "Name", "Quotes_Enabled", "Provider", "Last_Update", "Quotes", "Trades");
    ExchangeID prev_ex;
    for(map<ExchangeID, exch_data>::const_iterator e = exchanges.begin(), e_end = exchanges.end(); e != e_end; ++e) {
      const exch_data& ed = e->second;
      bool enabled = subscribers().publish_quotes(e->first);
      if(ed.provider->stats().last_update_time.is_set()) {
	Time_Utils::print_time(last_update, ed.provider->stats().last_update_time, Timestamp::MILI);
      } else {
	strcpy(last_update, "-");
      }
      request_.printf(ex_str,
		      e->first != prev_ex ? e->first.str() : "",
		      enabled ? "true" : "false",
		      ed.provider->name().c_str(),
		      last_update,
		      ed.quotes ? "true" : "false",
		      ed.trades ? "true" : "false");
      prev_ex = e->first;
    }

  }

  void Simple_MDManager::dispatcher_status_admin(AdminRequest& request_)
  {
    for(MDDispatcherMap_t::const_iterator d = m_dispatchers.begin(), d_end = m_dispatchers.end(); d != d_end; ++d) {
      d->second->status(request_);
      request_.puts("\n");
    }
  }
  
  
  void Simple_MDManager::shmem_status_admin(AdminRequest& req)
  {
    boost::shared_ptr<bip::managed_shared_memory> shmem = m_app->mdcache()->shmem();
    if(!shmem) {
      req.puts("No Shared Memory configured");
    } else {
      size_t size = shmem->get_size();
      size_t free_mem = shmem->get_free_memory();
      bool sanity = shmem->check_sanity();

      size_t num_named = shmem->get_num_named_objects();
      size_t num_unique = shmem->get_num_unique_objects();
      
      req.printf("Total memory size:     %ld\n", size);
      req.printf("Memory free:           %ld\n", free_mem);
      req.printf("Sanity check returned: %s\n", sanity ? "true" : "error");
      req.printf("MDCache shmem version  %d\n", MDCache::shmem_version);
      req.printf("Num named objects:     %ld\n", num_named);
      req.printf("Num unique objects:    %ld\n", num_unique);
      
      if(m_app->mdcache()->event_queue()) {
        req.printf("Event Queue seqno:     %ld\n", m_app->mdcache()->event_queue()->cursor());
      }
      if(m_app->mdcache()->high_traffic_event_queue()) {
        req.printf("HT Event Queue seqno:  %ld\n", m_app->mdcache()->high_traffic_event_queue()->cursor());
      }
      
    }
  }

  void Simple_MDManager::symbol_status_admin(AdminRequest& req)
  {
    req.set_content_type(HTTP_Headers::MIME_Type::csv);

    int max_exch_index = m_app->em()->max_index();

    req.puts("symbol,sid");
    for(int e_idx = 0; e_idx < max_exch_index; ++e_idx) {
      const char* en = m_app->em()->find_by_index(e_idx)->name().c_str();
      req.printf(",%s_bid,%s_bid_size,%s_bid_time,%s_ask,%s_ask_size,%s_ask_time,%s_trade,%s_trade_time,%s_volume,%s_clean_volume",
		 en, en, en, en, en, en, en, en, en, en);
    }
    req.puts("\n");

    char bid_time[32];
    char ask_time[32];
    char trade_time[32];

    Quote q;

    
    for(Product_Manager::ProductSet_t::const_iterator p = m_app->pm()->active_products().begin(),
	  p_end = m_app->pm()->active_products().end(); p != p_end; ++p) {
      const CompositeBook& cq = m_app->mdcache()->get_book((*p)->product_id());
      req.printf("%s,%ld", (*p)->symbol().c_str(), (*p)->sid());
      for(int e_idx = 0; e_idx < max_exch_index; ++e_idx) {
	const ExchangeBook& eb = cq.exchange(e_idx);

	eb.quote(q);

	bid_time[0] = '\0';
	ask_time[0] = '\0';
	trade_time[0] = '\0';
	if(q.bid_time > Time::today()) {
	  Time_Utils::print_time(bid_time, q.bid_time, Timestamp::MILI);
	}
	if(q.ask_time > Time::today()) {
	  Time_Utils::print_time(ask_time, q.ask_time, Timestamp::MILI);
	}
	if(eb.last_trade_time() > Time::today()) {
	  Time_Utils::print_time(trade_time, eb.last_trade_time(), Timestamp::MILI);
	}

	req.printf(",%0.4f,%d,%s,%0.4f,%d,%s,%0.4f,%s,%d,%d",
		   q.bid.value(), q.bid_size, bid_time,
		   q.ask.value(), q.ask_size, ask_time,
		   eb.last_trade(), trade_time,eb.volume(),eb.clean_volume());
      }
      req.puts("\n");
    }

  }

  void Simple_MDManager::recovery_status_admin(AdminRequest& req)
  {
    req.printf("Providers: \n");
    const char* pr_str_hdr = "%-20s %-10s %-10s %-10s %-10s %-10s\n";
    const char* pr_str = "%-20s %-10d %-10d %-10s %-10s %-10d %-10s %s\n";
    
    req.printf(pr_str_hdr, "name", "context", "seq_no", "last_update", "recovery", "snapshot");
    
    char last_update_str[32];
    
    BOOST_FOREACH(MDProviderMap_t::value_type& itr, m_providers) {
      MDProvider& provider = *itr.second;
      const Handler* handler = provider.handler();
      
      if(handler) {
        BOOST_FOREACH(const channel_info& ci, handler->m_channel_info_map) {
          Time_Utils::print_time(last_update_str, ci.last_update, Timestamp::MILI);
          string ri = ci.recovery_socket.remote_endpoint();
          string si = ci.snapshot_socket.remote_endpoint();
          
          req.printf(pr_str, provider.name().c_str(), ci.context, ci.seq_no, last_update_str, ri.c_str(), si.c_str());
        }
        req.printf("\n");
      }
    }
  }


  void Simple_MDManager::provider_change_admin(AdminRequest& request_, bool do_stop_)
  {
    string name = request_.get_param("name");
    if(name.empty()) {
      request_.printf("Invalid provider name");
      return;
    }
    
    MDProviderMap_t::iterator pitr=m_providers.find(name);
    if (pitr==m_providers.end()) {
      request_.printf("No provider with name: %s",name.c_str());
      return;
    }
    
    string group = request_.get_param("group");
    
    MDProviderRP provider=(*pitr).second;
    
    if(group.empty()) {
      if (do_stop_) provider->stop();
      else provider->start();
    } else {
      Handler* h = provider->handler();
      if(!h) {
        request_.printf("Provider '%s' does not have an associated dispatch handler", name.c_str());
        return;
      }
      
      for(MDDispatcherMap_t::const_iterator d = m_dispatchers.begin(), d_end = m_dispatchers.end(); d != d_end; ++d) {
        if(do_stop_) {
          d->second->stop(h, group);
        } else {
          d->second->start(h, group);
        }
      }
    }


    request_.printf("Provider: name=%s, type=%s, service=%s, source=%s, started=%s",
		    provider->name().c_str(),provider->type().c_str(),provider->service().c_str(),provider->source().c_str(),
		    (provider->enabled()?"Started":"Not Started") );
  }

  void Simple_MDManager::enable_exchange_admin(AdminRequest& request_, bool enable)
  {
    const string& exch_name = request_.get_param("exchange");
    if(exch_name.empty()) {
      request_.printf("Please specify exchange");
      return;
    }

    boost::optional<ExchangeID> exch = ExchangeID::get_by_name(exch_name.c_str());
    if(!exch) {
      request_.printf("No exchange with name: %s\n Known exchanges:", exch_name.c_str());
      for(ExchangeID::const_iterator e = ExchangeID::begin(); e != ExchangeID::end(); ++e) {
	request_.printf(" %s", e->str());
      }
      return;
    }

    this->subscribers().set_publish_quotes(*exch, enable);
    if(!enable) {
      this->subscribers().post_invalid_quote(*exch);
    }

    request_.printf("Exchange %s %s", exch->str(), enable ? "enabled" : "disabled");
  }

  void Simple_MDManager::set_security_status_admin(AdminRequest& req)
  {
    const string& symbol = req.get_param("symbol");
    if(symbol.empty()) {
      req.printf("Please specify symbol");
      return;
    }

    const Product* p = m_app->pm()->find_by_symbol(symbol);
    if(!p) {
      req.printf("Product symbol '%s' not found", symbol.c_str());
      return;
    }


    const string& mkt_status = req.get_param("market_status");
    if(mkt_status.empty()) {
      req.printf("Please specify market_status");
      return;
    }

    boost::optional<MarketStatus> ms = MarketStatus::get_by_name(mkt_status.c_str());
    if(!ms) {
      req.printf("No MarketStatus of type '%s' known\n Known MarketStatus values:", mkt_status.c_str());
      for(MarketStatus::const_iterator i = MarketStatus::begin(); i != MarketStatus::end(); ++i) {
        req.printf(" %s", i->str());
      }
      return;
    }

    const CompositeBook& cq = m_app->mdcache()->get_book(p->product_id());
    const Exchange& primary_exch = m_app->em()->find_by_primary(p->primary_exch());

    if(primary_exch.index() == -1) {
      req.printf("Primary exchange %s of %s is not an active exchange, unable to change status", primary_exch.name().c_str(), p->symbol().c_str());
      return;
    }

    ExchangeBook& eb = const_cast<ExchangeBook&>(cq.exchange(primary_exch.index()));
    eb.set_market_status(*ms);

    req.printf("MarketStatus on product %s changed to %s", p->symbol().c_str(), (*ms).str());
  }

  void
  Simple_MDManager::drop_packets_admin(AdminRequest& req)
  {
    string name = req.get_param("name");
    if (name.empty()) {
      req.printf("Invalid provider name");
      return;
    }
    
    MDProviderMap_t::iterator pitr=m_providers.find(name);
    if (pitr==m_providers.end()) {
      req.printf("No provider with name: %s",name.c_str());
      return;
    }
    
    MDProviderRP provider=(*pitr).second;
    
    int num_packets = 10;
    string num_packets_str = req.get_param("num_packets");
    if(!num_packets_str.empty()) {
      num_packets = strtol(num_packets_str.c_str(), 0, 10);
    }
    
    bool done = provider->drop_packets(num_packets);
    
    if(done) {
      req.printf("Provider: name=%s told to drop %d packets", provider->name().c_str(), num_packets);
    } else {
      req.printf("Provider: name=%s does not support drop packets request", provider->name().c_str());
    }
    
  }

  void
  Simple_MDManager::monitor_activity_admin(AdminRequest& req)
  {
    string name = req.get_param("name");
    if (name.empty()) {
      req.printf("Invalid provider name");
      return;
    }
    
    MDProviderMap_t::iterator pitr=m_providers.find(name);
    if (pitr==m_providers.end()) {
      req.printf("No provider with name: %s",name.c_str());
      return;
    }
    
    MDProviderRP provider=(*pitr).second;

    if(req.has_param("set")) {
      bool ma = req.get_bool_param("set");
      provider->set_monitor_activity(ma);
      req.printf("SET provider: %s  monitor_activity: %s\n", name.c_str(), provider->monitor_activity() ? "true" : "false");
    } else {
      req.printf("provider: %s  monitor_activity: %s\n", name.c_str(), provider->monitor_activity() ? "true" : "false");
    }    
  }
  
  
  
  
  void
  Simple_MDManager::add_dispatcher(const string& name, const Parameter& params, MDDispatcherRP dispatcher)
  {
    m_dispatchers.insert(make_pair(name, dispatcher));
    dispatcher->init(name, this, params);
  }

  MDDispatcher*
  Simple_MDManager::get_dispatcher(const string& name)
  {
    MDDispatcherMap_t::iterator i = m_dispatchers.find(name);
    if(i == m_dispatchers.end()) {
      return 0;
    }
    return i->second.get();
  }

  MDProvider*
  Simple_MDManager::get_provider(const string& name)
  {
    MDProviderMap_t::iterator i = m_providers.find(name);
    if(i == m_providers.end()) {
      return 0;
    }
    return i->second.get();
  }

  void Simple_MDManager::show_limits_admin(AdminRequest& req)
  {

  }

  void Simple_MDManager::set_limits_admin(AdminRequest& req)
  {

  }

  Feed_Rules_RP Simple_MDManager::get_feed_rules(const string& feed_rules_name)
  {
    m_feed_rules_by_name_iter = m_feed_rules_by_name.find(feed_rules_name);
    if (m_feed_rules_by_name_iter == m_feed_rules_by_name.end()) {
      Feed_Rules_RP feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
      feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
      m_feed_rules_by_name[feed_rules_name] = feed_rules;
      return feed_rules;
    } else {
      return m_feed_rules_by_name_iter->second;
    }
  }

  Feed_Rules_RP Simple_MDManager::get_feed_rules(const ExchangeID& feed_rules_exch)
  {
    m_feed_rules_by_exch_iter = m_feed_rules_by_exch.find(feed_rules_exch);
    if (m_feed_rules_by_exch_iter == m_feed_rules_by_exch.end()) {
      Feed_Rules_RP feed_rules = ObjectFactory<ExchangeID,Feed_Rules>::allocate(feed_rules_exch);
      feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
      m_feed_rules_by_exch[feed_rules_exch] = feed_rules;
      return feed_rules;
    } else {
      return m_feed_rules_by_exch_iter->second;
    }
  }

}

