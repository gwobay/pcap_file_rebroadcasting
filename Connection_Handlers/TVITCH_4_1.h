#ifndef hf_md_QBT_TVITCH_4_1_h
#define hf_md_QBT_TVITCH_4_1_h

#include <Data/Handler.h>
#include <Data/MoldUDP64.h>
#include <Data/TVITCH_4_1_Handler.h>
#include <MD/MDProvider.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Interface/AlertEvent.h>

#include <Logger/Logger.h>
#include <Util/Time.h>

namespace hf_md {
  using namespace hf_tools;
  using namespace hf_core;

  class TVITCH_4_1 : public hf_core::MDProvider {
  public:
    
    // MDProvider functions
    virtual void init(const string&,const Parameter&);
    virtual void start(void);
    virtual void stop(void);
    virtual void subscribe_product(const ProductID& id_,const ExchangeID& exch_,
				   const string& mdSymbol_,const string& mdExch_);
    virtual bool drop_packets(int num_packets);
    
    virtual Handler* handler() { return m_itch_parser.get(); }
    
    TVITCH_4_1(MDManager& md);
    virtual ~TVITCH_4_1();
    
  private:
    
    boost::scoped_ptr<MoldUDP64> m_mold_parser;
    boost::scoped_ptr<TVITCH_4_1_Handler> m_itch_parser;
    
    sockaddr_in  m_recovery_addr;
    
    vector<string> m_lines;
    
    MDDispatcher* m_dispatcher;
    MDDispatcher* m_recovery_dispatcher;
  };
  
}

#endif /* ifndef hf_md_QBT_TVITCH_4_1_h */
