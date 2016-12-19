#ifndef hf_core_Data_MoldUDP64_h
#define hf_core_Data_MoldUDP64_h

#include <Data/Handler.h>
#include <Data/MDBufferQueue.h>
#include <Data/TVITCH_4_1_Handler.h>
#include <Logger/Logger.h>

#include <string>
#include <queue>
#include <boost/thread.hpp>

#include <netinet/in.h>

namespace hf_core {
  class Application;
  class MDDispatcher;
}

namespace hf_core {
  using namespace hf_tools;

  class MoldUDP64 : public Handler {
  public:

    virtual const string& name() const { return m_name; }

    virtual size_t parse(size_t, const char* buf, size_t len);
    virtual size_t parse2(size_t, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t) { return 0; }
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg, uint64_t expected_seq_no,
                       uint64_t received_seq_no);
    virtual void send_retransmit_request(channel_info& channel_info);
    virtual void disable_reordering() { m_channel_info.has_reordering = false; }
    
    virtual void subscribe_product(ProductID id, ExchangeID exch, const string& mdSymbol,const string& mdExch) { }

    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);
    
    void start();
    void stop();

    void init(const string&, const Parameter&);
    MoldUDP64(Application* app, TVITCH_4_1_Handler* msg_parser);
    virtual ~MoldUDP64();
    
    channel_info m_channel_info;

  private:
    typedef tbb::spin_mutex mutex_t;
    
    string   m_name;
    mutex_t  m_mutex;
    TVITCH_4_1_Handler* m_message_parser;
    char     m_session[10];
  };

}

#endif /* ifndef hf_core_Data_MoldUDP64_h */
