#ifndef hf_md_QBT_OpenOnload_Dispatcher_h
#define hf_md_QBT_OpenOnload_Dispatcher_h

#include <QBT/EPoll_Dispatcher.h>

#include <tbb/concurrent_queue.h>

struct onload_zc_recv_args;

namespace hf_md {
  using namespace __gnu_cxx;
  using namespace std;
  using namespace hf_core;
  
  class OpenOnload_Dispatcher : public EPoll_Dispatcher {
  public:
    
    struct request_t {
      BOOST_ENUM(Command,
                 (start)
                 (stop)
                 (join)
                 (leave));
      
      Handler* handler;
      string   name;
      Command  command;
      
      request_t() : handler(0) { };
      request_t(Handler* h, const string& n, Command command) : handler(h), name(n), command(command) { }
    };
    
    virtual void init(const string& name, MDManager* md, const Parameter& params);
    virtual void run();
    virtual size_t poll_one(onload_zc_recv_args* zc_args);
    
    virtual void start(Handler* h, const string& name);
    virtual void stop(Handler* h, const string& name);

    virtual void start_sync(Handler* h, const string& name);
    virtual void stop_sync(Handler* h, const string& name);
    
    virtual void status(ILogger&) const;
        
    // We only want to do one check, so lets just do a general source/netmask deal
    
    bool is_started() const { return m_started; };
    
    explicit OpenOnload_Dispatcher();
    virtual ~OpenOnload_Dispatcher();
    
  protected:
    typedef tbb::spin_mutex mutex_t;

    tbb::concurrent_queue<request_t > m_requests;
    
    void run_loop();
    void handle_request(const request_t& req);

    bool     m_onload_set_spin;
  };

}

#endif /* ifndef hf_md_QBT_OpenOnload_Dispatcher_h */
