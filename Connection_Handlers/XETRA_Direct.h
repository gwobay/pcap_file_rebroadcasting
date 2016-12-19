#ifndef hf_md_QBT_XETRA_Direct_h
#define hf_md_QBT_XETRA_Direct_h

#include <Data/Handler.h>
#include <Data/XETRA_Handler.h>
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

  class XETRA_Direct : public hf_core::MDProvider {
  public:

    // MDProvider functions
    virtual void init(const string&,const Parameter&);
    virtual void start(void);
    virtual void stop(void);
    virtual void subscribe_product(const ProductID& id_,const ExchangeID& exch_,
				   const string& mdSymbol_,const string& mdExch_);
    virtual Handler* handler() { return m_parser.get(); }

    XETRA_Direct(MDManager& md);
    virtual ~XETRA_Direct();

  private:
    void init_mcast(const string& line_name, const Parameter& params);

    MDDispatcher* m_dispatcher;
    MDDispatcher* m_recovery_dispatcher;

    Time_Duration m_recovery_timeout;

    boost::scoped_ptr<XETRA_Handler> m_parser;
  };

}

#endif /* ifndef hf_md_QBT_XETRA_Direct_h */
