#ifndef hf_md_QBT_TSE_FLEX_FULL_h
#define hf_md_QBT_TSE_FLEX_FULL_h

#include <Data/Handler.h>
#include <Data/TSE_FLEX_Handler.h>
#include <MD/MDProvider.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Interface/AlertEvent.h>

#include <Logger/Logger.h>
#include <Util/Time.h>

namespace hf_core {
  class Application;
  class MDDispatcher;
}

namespace hf_md {
  using namespace hf_tools;
  using namespace hf_core;

  class TSE_FLEX_Full : public hf_core::MDProvider {
  public:
    
    // MDProvider functions
    virtual void init(const string&,const Parameter&);
    virtual void start(void);
    virtual void stop(void);
    virtual void subscribe_product(const ProductID& id_,const ExchangeID& exch_,
				   const string& mdSymbol_,const string& mdExch_);
    virtual Handler* handler() { return m_tse_parser.get(); }
    
    TSE_FLEX_Full(MDManager& md);
    virtual ~TSE_FLEX_Full();
    
  private:
    void init_mcast(const string& line_name, const Parameter& params);
    
    MDDispatcher* m_dispatcher;
    MDDispatcher* m_recovery_dispatcher;
    
    Time_Duration m_recovery_timeout;
    
    boost::scoped_ptr<TSE_FLEX_Handler> m_tse_parser;
  };
  
}

#endif /* ifndef hf_md_QBT_TSE_FLEX_FULL_h */
