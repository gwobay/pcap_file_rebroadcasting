#pragma once

#include <ITCH/Simple_Dispatcher.h>
#include <onload/extensions.h>
#include <onload/extensions_zc.h>

struct onload_zc_recv_args;

namespace hf_cwtest {
  using namespace __gnu_cxx;
  using namespace std;
  using namespace hf_core;
  //using namespace hf_md;
  
  //class TestOnLoad_Dispatcher : public EPoll_Dispatcher {
  class TestOnLoad_Dispatcher : public Simple_Dispatcher {
  public:
    
    virtual void init(const string& name, MDManager* md, const Parameter& params);
    virtual void run();
    virtual size_t poll_one(onload_zc_recv_args* zc_args);
    
    virtual void start(Handler* h, const string& name);
    virtual void stop(Handler* h, const string& name);

    virtual void start_sync(Handler* h, const string& name);
    virtual void stop_sync(Handler* h, const string& name);
    
    virtual void status(ILogger&) const;
        
    static onload_zc_callback_rc
    onload_callback(struct onload_zc_recv_args* args, int flags);
    // We only want to do one check, so lets just do a general source/netmask deal
    
    bool is_started() const { return m_started; };
    
    explicit TestOnLoad_Dispatcher();
    virtual ~TestOnLoad_Dispatcher();
    
  protected:
    typedef tbb::spin_mutex mutex_t;
    void run_loop();
   char m_udp_buffer[0x0000FFFF]; 
   uint32_t pUdpBuffer_offset;
    bool     m_onload_set_spin;
  };

}

