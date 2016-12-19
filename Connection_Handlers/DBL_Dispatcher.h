#ifndef hf_md_QBT_DBL_Dispatcher_h
#define hf_md_QBT_DBL_Dispatcher_h

#include <Data/Handler.h>
#include <Util/hash_util.h>
#include <Util/enum.h>

#include <MD/MDManager.h>
#include <vector>
#include <ext/hash_map>

#include <boost/thread.hpp>
#include <tbb/concurrent_queue.h>

#include <netinet/in.h>
#include <dbl.h>

namespace hf_md {
  using namespace __gnu_cxx;
  using namespace std;
  using namespace hf_core;

  class DBL_Dispatcher : public MDDispatcher {
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
    virtual size_t poll_one();
    
    virtual void start(Handler* h, const string& name);
    virtual void stop(Handler* h, const string& name);
    virtual void mcast_join(Handler* h, const string& name, size_t context, const string& group, int port, const string& iface, const string& source, bool bind_only);
    virtual void udp_bind(Handler* handler, const string& name, size_t context, const string& host, int port);
    virtual void add_udp_fd(Handler* handler, const string& name, size_t context, int fd); // will throw exception
    virtual void add_tcp_fd(Handler* handler, const string& name, size_t context, int fd, size_t buf_size); // will throw exception
    
    virtual void start_sync(Handler* h, const string& name);
    virtual void stop_sync(Handler* h, const string& name);
    virtual void join_sync(Handler* h, const string& name);
    virtual void leave_sync(Handler* h, const string& name);
    
    virtual void status(ILogger&) const;
    
    virtual void full_stop();
    
    bool is_started() const { return m_started; };

    DBL_Dispatcher();
    virtual ~DBL_Dispatcher();
    
  private:
    void handle_request(const request_t& req);  // subscribe / unsubscribe
    
    void run_loop();

    struct Context {
      Handler* handler;
      string   name;
      size_t   handler_context;
      bool     mcast;
      bool     bound;
      
      bool    subscribe_on_start;
      bool    subscribed;
      
      in_addr mcast_addr;
      int     port;
      uint32_t source_ip;

      dbl_device_t dev;
      dbl_channel_t chan;
      
      size_t recv_count;
      size_t recv_bytes;
      
      Context(Handler* h, const string& name, size_t handler_context, bool mcast, uint32_t source_ip, dbl_device_t dev, in_addr mcast_addr, int port) :
        handler(h), name(name), handler_context(handler_context), mcast(mcast), bound(false),
        subscribe_on_start(true), subscribed(false), mcast_addr(mcast_addr), port(port), source_ip(source_ip),
        dev(dev), chan(0),
        recv_count(0), recv_bytes(0) { }
    };

    vector<Context> m_contexts;

    bool          m_dbl_dup_to_kernel;
    bool          m_first_error;
    volatile bool m_started;

    dbl_recvmode  m_recvmode;
    
    hash_map<string, dbl_device_t> m_iface_to_dbl_device;
    vector<dbl_device_t> m_dbl_devices;
    
    tbb::concurrent_queue<request_t > m_requests;

    boost::thread m_thread;
    int           m_cpu_bind;

    static const size_t m_buffer_len = 64 * 1024;
    char m_buffer[m_buffer_len];  // Max UDP packet is 64K, 8K is more likely
  };

}

#endif /* ifndef hf_md_QBT_DBL_Dispatcher_h */
