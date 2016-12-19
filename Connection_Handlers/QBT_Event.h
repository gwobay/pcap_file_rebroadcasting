#ifndef hf_md_QBT_QBT_Event_h
#define hf_md_QBT_QBT_Event_h

#include <MD/MDProvider.h>
#include <MD/QBT_Event_Receiver.h>

#include <Logger/Logger.h>

namespace hf_core {
  class Application;
}

namespace hf_md {
  using namespace hf_tools;
  using namespace hf_core;
  
  class QBT_Event : public MDProvider {
  public:
    
    class QBT_Event_Acceptor : public Event_Acceptor {
    public:
      virtual const string& name() const { return m_name; }
      
      virtual void accept_quote_update(const QuoteUpdate&);
      virtual void accept_trade_update(const TradeUpdate&);
      virtual void accept_market_status_update(const MarketStatusUpdate&);
      virtual void accept_auction_update(const AuctionUpdate&);
      virtual void accept_alert_event(const AlertEvent&);
      virtual void accept_orderbook_update(const OrderBookUpdate&);
      
      QBT_Event_Acceptor(QBT_Event* evt, MDManager* md) : m_evt(evt), m_md(md) { }
      
    private:
      string m_name;
      
      QBT_Event* m_evt;
      MDManager* m_md;
    };
    
    virtual void init(const string&, const Parameter&);
    virtual void start(void);
    virtual void stop(void);
    virtual void subscribe_product(const ProductID& id_,const ExchangeID& exch_,
				   const string& mdSymbol_,const string& mdExch_);

    virtual void inactivity_notification();
    
    QBT_Event(MDManager& mgr);
    virtual ~QBT_Event();
    
  private:
    friend class QBT_Event_Acceptor;
    
    boost::intrusive_ptr<QBT_Event_Acceptor> m_acceptor;
    boost::intrusive_ptr<QBT_Event_Receiver> m_receiver;
    
    volatile bool m_started;
    
    bool     m_first_msg;
  };
  
}

#endif /* ifndef hf_md_QBT_QBT_Event_h */
