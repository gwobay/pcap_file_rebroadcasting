#ifndef hf_md_QBT_UTP_h
#define hf_md_QBT_UTP_h

#include <MD/MDProvider.h>
#include <Data/UTP_Handler.h>

#include <Logger/Logger.h>
#include <Util/Time.h>

#include <boost/scoped_ptr.hpp>

namespace hf_core {
  class MDDispatcher;
}

namespace hf_md {

  using namespace hf_tools;
  using namespace hf_core;


  class UTP : public hf_core::MDProvider {
  public:
    
    // MDProvider functions
    virtual void init(const string&,const Parameter&);
    virtual void start(void);
    virtual void stop(void);
    virtual void subscribe_product(const ProductID& id_,const ExchangeID& exch_,
				   const string& mdSymbol_,const string& mdExch_);
    virtual Handler* handler() { return m_handler.get(); }
    
    UTP(MDManager& md);
    virtual ~UTP();
    
    void init_mcast(const string& line_name, const Parameter& params);
    
  private:
    
    MDDispatcher* m_disp;
    boost::scoped_ptr<UTP_Handler> m_handler;
    
  };
  
}

#endif /* ifndef hf_md_QBT_UTP_h */
