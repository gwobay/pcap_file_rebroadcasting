#pragma once

#include <boost/shared_ptr.hpp>
#include <Util/Parameter.h>
#include <Logger/Logger.h>
#include <Event/Event_Acceptor.h>
#include <Exchange/Feed_Rules.h>
#include <Admin/Admin_Request_Handler.h>
#include <MD/MDManager.h>
#include <MD/MDDispatcher.h>
#include <MD/MDSubscriptionMapping.h>
#include <MD/MDSubscriptionAcceptor.h>
#include <MD/MDProvider.h>
#include <MD/MDWatchdog.h>
#include <Data/Handler.h>
#include <Util/ObjectFactory.h>
#include <ASIO/App_Service.h>
#include <app/app.h>
#include <app/App_Run_Loop.h>
#include <ext/hash_map>


namespace hf_cwtest
{
  using namespace std;
  using namespace hf_core;
  using namespace hf_tools;

  //class hf_core::Application;
  //class hf_core::Handler;
  //class hf_core::MDManager;
  //class hf_core::App_Run_Loop;

  class Simple_MDManager: public hf_core::MDManager
  {
  public:

    Simple_MDManager(Application* app);
    ~Simple_MDManager(void);

    void init(const Parameter&);
    void start(void);
    void stop(void);
    
    virtual const string& name() const { return m_name; }
    virtual void accept_admin_request(AdminRequest&);
    virtual void accept_quote_subscribe(const QuoteUpdateSubscribe& s);
    virtual void accept_trade_subscribe(const TradeUpdateSubscribe& s);
    virtual void accept_system_status_update(const SystemStatusUpdate&);
    
    MDSubscriptionAcceptor& subscribers(void) { return *m_subscribers; }

    boost::shared_ptr<MDSubscriptionAcceptor> subscribers_rp(void) { return m_subscribers; }

    const MDSubscriptionMapping& submap(void) { return m_mappings; }

    Application& app(void) const { return *m_app; }

    MDWatchdogRP watchdog();// { return m_mdwatchdog; }
    void set_watchdog(MDWatchdogRP w);// { m_mdwatchdog = w; }
    void set_logger(LoggerRP l);

    LoggerRP logger(void);// { return m_logger; }

    void add_provider(const string& name_,const Parameter& params_, MDProviderRP provider_);

    void add_dispatcher(const string& name, const Parameter& params, MDDispatcherRP dispatcher);
    MDDispatcher* get_dispatcher(const string& name);

    MDProvider* get_provider(const string& name);
    Feed_Rules_RP get_feed_rules(const string& feed_rules_name);
    Feed_Rules_RP get_feed_rules(const ExchangeID& feed_rules_exch);

    Admin_Request_Handler& admin_handler() { return m_admin_handler; };
  private:
    static const string m_name;
    
    typedef __gnu_cxx::hash_map<string,MDProviderRP> MDProviderMap_t;
    typedef __gnu_cxx::hash_map<string,MDDispatcherRP> MDDispatcherMap_t;
    
    struct Timer_Desc {
      string name;
      bool   fast;
      
      Time start_time;
      Time end_time;
      
      vector<Event_AcceptorRP> acceptors;
    };
    
    void init_providers(const Parameter& params_);
    void init_provider(const string&,const Parameter& params_);

    // admin functions
    void print_quote_admin(AdminRequest& request_);
    void print_book_admin(AdminRequest& request_);
    void update_quote_admin(AdminRequest& request_);
    void set_value_admin(AdminRequest& request_);
    void read_values_admin(AdminRequest& request_);
    void status_admin(AdminRequest& request_);
    void symbol_status_admin(AdminRequest& request_);
    void recovery_status_admin(AdminRequest& request_);
    void provider_change_admin(AdminRequest& request_,bool);
    void enable_exchange_admin(AdminRequest& request_,bool);
    void set_security_status_admin(AdminRequest& req);
    void show_limits_admin(AdminRequest& req);
    void set_limits_admin(AdminRequest& req);
    void print_imbalance_admin(AdminRequest& req);
    void print_level2_book_admin(AdminRequest& req);
    void clear_order_pool_admin(AdminRequest& req);
    void clear_level_pool_admin(AdminRequest& req);
    void clear_order_book_admin(AdminRequest& req);
    void invalidate_quote_admin(AdminRequest& req);
    void dispatcher_status_admin(AdminRequest& req);
    void shmem_status_admin(AdminRequest& req);
    void drop_packets_admin(AdminRequest& req);
    void monitor_activity_admin(AdminRequest& req);
    
    void handle_subscription(const ProductID& product_,const ExchangeID& exch_);

    void post_timer_td(Time t, const Timer_Desc& td);
    
    Application* m_app;
    
    MDSubscriptionMapping m_mappings;
    MDDispatcherMap_t     m_dispatchers;
    MDProviderMap_t       m_providers;
        
    boost::shared_ptr<MDSubscriptionAcceptor> m_subscribers;

    MDWatchdogRP m_mdwatchdog;

    Admin_Request_Handler m_admin_handler;
    string m_admin_domain;
    LoggerRP m_logger;

    map<string,Feed_Rules_RP> m_feed_rules_by_name;
    map<string,Feed_Rules_RP>::iterator m_feed_rules_by_name_iter;
    map<ExchangeID,Feed_Rules_RP> m_feed_rules_by_exch;
    map<ExchangeID,Feed_Rules_RP>::iterator m_feed_rules_by_exch_iter;
  };

}
