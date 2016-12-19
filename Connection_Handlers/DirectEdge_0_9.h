#ifndef hf_md_QBT_DirectEdge_0_9_h
#define hf_md_QBT_DirectEdge_0_9_h

#include <Data/Handler.h>
#include <Data/DirectEdge_0_9_Handler.h>
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

  class DirectEdge_0_9 : public hf_core::MDProvider {
  public:
    
    // MDProvider functions
    virtual void init(const string&,const Parameter&);
    virtual void start(void);
    virtual void stop(void);
    virtual void subscribe_product(const ProductID& id_,const ExchangeID& exch_,
				   const string& mdSymbol_,const string& mdExch_);
    virtual Handler* handler() { return m_edge_parser.get(); }
    
    DirectEdge_0_9(MDManager& md);
    virtual ~DirectEdge_0_9();
    
  private:
    void init_mcast(const string& line_name, const Parameter& params);
    
    MDDispatcher* m_dispatcher;
    MDDispatcher* m_recovery_dispatcher;
        
    boost::scoped_ptr<DirectEdge_0_9_Handler> m_edge_parser;
  };
  
}

#endif /* ifndef hf_md_QBT_DirectEdge_0_9_h */
