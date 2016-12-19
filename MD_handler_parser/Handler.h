#pragma once

#include <Data/QRec.h>
#include <Data/MDBufferQueue.h>
#include <Data/QBDF_Builder.h>
#include <Interface/AlertEvent.h>
#include <Logger/Logger.h>
#include <Util/Parameter.h>
#include <Util/Time.h>
#include <MD/MDSubscriptionAcceptor.h>
#include <MD/MDProvider.h>

#include <string>

namespace hf_core  {
  using namespace std;
  using namespace hf_tools;

  class Application;
  struct channel_info;
  class MDDispatcher;
  
  class Handler {
  public:
    
    Application* app() { return m_app; }
    
    virtual void init(const string& name, ExchangeID eid, const Parameter& p) { init(name, p); }
    virtual void init(const string& name, const vector<ExchangeID>& supported_exchanges, const Parameter& p);
    virtual void init(const string&, const Parameter&);
    
    virtual const string& name() const = 0;
    virtual size_t parse(size_t context, const char* buf, size_t len) = 0;   // Generally responsible for seq_ordering, stage 1 parse
    virtual size_t parse2(size_t context, const char* buf, size_t len) = 0;  // 2nd stage of parsing
    virtual size_t read(size_t context, const char* buf, size_t len) = 0;    // TCP stream
    virtual void reset(const char* msg) = 0;
    virtual void reset(size_t context, const char* msg) { reset(msg); }
    virtual void reset(size_t context, const char* msg, uint64_t expected_seq_no, uint64_t received_seq_no);
    
    virtual void subscribe_product(ProductID id, ExchangeID exch, const string& mdSymbol,const string& mdExch) = 0;
    
    // there is a basic implementation in the base class, but the sub-class may
    // want to override it e.g., to create a TCP connection for recovery.
    virtual void start();
    virtual bool is_done();
    virtual void stop();
    
    virtual void send_retransmit_request(channel_info& b) { }

    channel_info* get_channel_info(string name, size_t context);
    virtual void sync_udp_channel(string name, size_t context, int  recovery_socket){}
    virtual void sync_tcp_channel(string name, size_t context, int  recovery_socket){}
    virtual void update_channel_state(){} //used for separation of channel info and state, see ASX_ITCH
    virtual void to_write(int fd){} //check for send event explicitly
                                    //may not necessary if use read(., .. len=0) instead
    virtual bool sets_up_own_channel_contexts() { return false; }   
 
    vector<boost::shared_ptr<MDOrderBookHandler> > order_books()           { return m_order_books; }
    vector<boost::shared_ptr<MDOrderBookHandler> > secondary_order_books() { return m_secondary_order_books; }
    
    virtual void set_qbdf_builder(QBDF_Builder* qb) { m_qbdf_builder = qb; }
    void set_subscribers(boost::shared_ptr<MDSubscriptionAcceptor> s) { m_subscribers = s;}
    void set_provider(MDProvider* p) { m_provider = p; }
    void set_dispatcher(MDDispatcher* d) { m_dispatcher = d; }
    MDDispatcher* dispatcher() const { return m_dispatcher; }
    
    void send_alert(const char* fmt, ...);
    void send_alert_forced(const char* fmt, ...);
    void send_alert(AlertSeverity sev, const char* fmt, ...);
    
    void clear_order_books(bool delete_orders);
    
    Logger& logger() { return *m_logger; }
    LoggerRP recorder() { return m_recorder; }
    
    bool    record_only() const { return m_record_only; }

    void drop_packets(int num_packets);  // should cause handler to drop N packets to test GAP recovery
    void set_ms_latest(uint64_t l) { m_ms_latest = l; }

    virtual bool check_order_id(uint64_t order_id);

    typedef vector<channel_info> channel_info_map_t;
    channel_info_map_t m_channel_info_map;
    
    inline void set_hw_recv_time(Time t) { m_hw_recv_time = t; }
    
    void compare_hw_time();
    
    Handler(Application* app, const string& handler_type);
    virtual ~Handler() {}

    MDProvider* provider() { return m_provider; }

    inline void update_subscribers(QuoteUpdate& quote_update)
    {
      if(m_subscribers) {
        if(m_provider->collect_latency_stats()) {
           m_provider->stats_collector().current_timestamps().after_handler_parse = Time::current_time();
           quote_update.before_mdcache_commit_time = Time();
        }
        quote_update.line_time = m_hw_recv_time;
        m_subscribers->update_subscribers(quote_update);
        if(m_provider->collect_latency_stats() && quote_update.before_mdcache_commit_time != Time()) {
           m_provider->stats_collector().current_timestamps().before_commit = quote_update.before_mdcache_commit_time;
           m_provider->stats_collector().current_timestamps().after_commit = Time::current_time();
        }
      }
    }
 
    inline void update_subscribers(TradeUpdate& trade_update)
    {
      if(m_subscribers) {
        if(m_provider->collect_latency_stats()) {
           m_provider->stats_collector().current_timestamps().after_handler_parse = Time::current_time();
           trade_update.before_mdcache_commit_time = Time();
        }
        trade_update.line_time = m_hw_recv_time;
        m_subscribers->update_subscribers(trade_update);
        if(m_provider->collect_latency_stats() && trade_update.before_mdcache_commit_time != Time()) {
           m_provider->stats_collector().current_timestamps().before_commit = trade_update.before_mdcache_commit_time;
           m_provider->stats_collector().current_timestamps().after_commit = Time::current_time();
        }
      }
    }

  protected:
    
    Time  m_hw_recv_time;
    
    Application* m_app;
    string m_handler_type;
    
    Time m_last_alert_time;
    Time_Duration m_alert_frequency;
    AlertSeverity m_alert_severity;
    
    LoggerRP m_logger;
    LoggerRP m_recorder;
    
    QBDF_Builder* m_qbdf_builder;
    uint64_t m_ms_latest;
    uint64_t m_ms_since_midnight;
    uint64_t m_last_sec_reported;
    uint64_t m_last_min_reported;
    
    uint64_t m_micros_latest;
    uint64_t m_micros_since_midnight;
    uint64_t m_micros_recv_time;
    uint64_t m_micros_exch_time;
    int32_t m_micros_record_time_delta;
    int32_t m_micros_exch_time_delta;
    
    int m_drop_packets;
    
    bool m_record_only       : 1;
    bool m_exclude_level_2   : 1;
    bool m_exclude_imbalance : 1;
    bool m_exclude_quotes    : 1;
    bool m_exclude_status    : 1;
    bool m_exclude_trades    : 1;
    
    bool m_use_micros        : 1;
    bool m_use_exch_time     : 1;
    
    set<uint64_t> m_search_orders;
    
    boost::shared_ptr<MDSubscriptionAcceptor>      m_subscribers;
    MDProvider*                                    m_provider;
    MDDispatcher*                                  m_dispatcher;
    
    vector<boost::shared_ptr<MDOrderBookHandler> > m_order_books;
    vector<boost::shared_ptr<MDOrderBookHandler> > m_secondary_order_books;
  };
  
  void parse_qrec_file(Handler* handler, Logger* logger, FILE* fp, size_t max_context);

}
