#ifndef kou_connect_QBT_EPoll_Dispatcher_h
#define kou_connect_QBT_EPoll_Dispatcher_h

#include <Data/Handler.h>
#include <Util/hash_util.h>

#include <MD/MDManager.h>
#include <vector>
#include <ext/hash_map>
#include <boost/thread.hpp>

#include <netinet/in.h>

#include <tbb/spin_mutex.h>

extern "C" {
  struct epoll_event;
}

namespace kou_connect {
  using namespace __gnu_cxx;
  using namespace std;
  using namespace kou_core;
    
  class EPoll_Dispatcher : public MDDispatcher {
  public:
    struct Context {
      Handler* handler;
      size_t   handler_context;
      bool     udp           ;
      bool     mcast      : 1;
      bool     subscribed : 1;
      bool     externally_subscribed : 1;
      
      int      fd;
      
      uint32_t group_addr;
      uint32_t iface_addr;
      int      port;
      uint32_t source_ip;
      
      size_t recv_count;
      size_t recv_bytes;
      
      string   name;
      
      Context(Handler* h, const string& name, size_t handler_context, bool udp, bool mcast, uint32_t source_ip, uint32_t group_addr, uint32_t iface_addr, int port) :
        handler(h), handler_context(handler_context), udp(udp), mcast(mcast),
        subscribed(false), externally_subscribed(false), fd(-1),
        group_addr(group_addr), iface_addr(iface_addr), port(port), source_ip(source_ip),
        recv_count(0), recv_bytes(0), name(name)
      { }
    };
    
    struct TCP_Buffer {
      char*         buffer;
      unsigned int  buffer_len;
      unsigned int  buffer_end;
      
      TCP_Buffer() : buffer(0), buffer_len(0), buffer_end(0) { }
      void init(size_t size) {
        buffer = new char[size];
        buffer_len = size;
        buffer_end = 0;
      };
      
      ~TCP_Buffer() {
        delete [] buffer;
      }
    };
    
    virtual void init(const string& name, MDManager* md, const Parameter& params);
    virtual void run();
    virtual size_t poll_one();
    
    virtual void start(Handler* h, const string& name);
    virtual void stop(Handler* h, const string& name);
    virtual void start_sync(Handler* h, const string& name);
    virtual void stop_sync(Handler* h, const string& name);
    virtual void mcast_join(Handler* handler, const string& name, size_t handler_context, const string& group, int port, const string& iface, const string& source, bool bind_only);
    virtual void udp_bind(Handler* handler, const string& name, size_t context, const string& host, int port);
    
    virtual void add_udp_fd(Handler* handler, const string& name, size_t context, int fd);
    virtual void add_tcp_fd(Handler* handler, const string& name, size_t context, int fd, size_t buf_size);
    
    virtual void status(ILogger&) const;
    
    virtual void full_stop();
    
    // We only want to do one check, so lets just do a general source/netmask deal

    bool is_started() const { return m_started; };

    explicit EPoll_Dispatcher();
    virtual ~EPoll_Dispatcher();

  protected:
    typedef tbb::spin_mutex mutex_t;
    
    void run_loop();
    void add_fd(int fd, size_t context_idx, bool writeable, bool et);
    void remove_fd(int fd);
    void clear();
    
    static const size_t max_events = 256;
    
    volatile bool m_started;
    int m_ep_fd;
    int m_timeout;
    
    vector<Context>      m_contexts;
    vector<size_t>       m_fd_to_context;
    vector<TCP_Buffer*>  m_fd_to_tcp_buffer;
    
    int m_recv_buffer_size;
    
    boost::thread m_thread;
    mutex_t       m_mutex;
    
    boost::scoped_array<epoll_event> m_events;
    
    vector<LoggerRP> m_flush_buffers;
    
    static const size_t m_buffer_len = 64 * 1024;
    char m_buffer[m_buffer_len];  // Max UDP packet is 64K, 8K is more likely
    
    bool  m_first_error;
    int   m_cpu_bind;
    bool  m_timestamp_on;
    bool  m_hw_timestamp_on;
  };

}

#endif /* ifndef kou_connect_QBT_EPoll_Dispatcher_h */
