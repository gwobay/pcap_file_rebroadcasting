#ifndef hf_md_QBT_MDMulticast_Subscriber_h
#define hf_md_QBT_MDMulticast_Subscriber_h

#include <Data/Handler.h>
#include <Data/MDBufferQueue.h>

#include <MD/MDProvider.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Interface/AlertEvent.h>

#include <Logger/Logger.h>
#include <Util/Parameter.h>
#include <Util/Time.h>

namespace hf_core {
  class MDDispatcher;
}

namespace hf_md {
  using namespace std;
  using namespace hf_tools;
  using namespace hf_core;
  
  class MDMulticast_Subscriber : public hf_core::MDProvider {
  public:
    // MDProvider functions
    virtual void init(const string&, const Parameter&);
    virtual void start(void);
    virtual void stop(void);
    virtual void subscribe_product(const ProductID& id_,const ExchangeID& exch_,
				   const string& mdSymbol_,const string& mdExch_);
    virtual bool drop_packets(int num_packets);
    virtual Handler* handler() { return m_handler.get(); }
    
    MDMulticast_Subscriber(MDManager& md);
    virtual ~MDMulticast_Subscriber();
    
  private:
    
    bool init_mcast(const string& lines, const Parameter& params);
    
    boost::scoped_ptr<Handler> m_handler;
    
    MDDispatcher* m_disp;
    MDDispatcher* m_recov_disp;
  };
  
}

#endif /* ifndef hf_md_QBT_MDMulticast_Subscriber_h */
