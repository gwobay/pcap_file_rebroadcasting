#include <QBT/QBT_Event.h>
#include <MD/MDManager.h>

#include <app/app.h>
#include <Event_Hub/Event_Hub.h>
#include <Interface/AlertEvent.h>
#include <Product/Product_Manager.h>
#include <Util/except.h>

#include <MD/QBT_Event_Publisher.h>

#include <unistd.h>

#include <stdarg.h>
#include <syscall.h>

namespace hf_md {
  
  using namespace hf_tools;
  
  using namespace hf_core::QBT_Event_structs;
  
  void QBT_Event::QBT_Event_Acceptor::accept_quote_update(const QuoteUpdate& qu)
  {
    m_evt->check_quote(const_cast<QuoteUpdate&>(qu));
    m_md->subscribers().update_subscribers(const_cast<QuoteUpdate&>(qu));
  }

  void QBT_Event::QBT_Event_Acceptor::accept_trade_update(const TradeUpdate& tu)
  {
    m_evt->check_trade(const_cast<TradeUpdate&>(tu));
    m_md->subscribers().update_subscribers(const_cast<TradeUpdate&>(tu));
  }

  void QBT_Event::QBT_Event_Acceptor::accept_market_status_update(const MarketStatusUpdate& msu)
  {
    m_md->subscribers().update_subscribers(const_cast<MarketStatusUpdate&>(msu));
  }
  
  void QBT_Event::QBT_Event_Acceptor::accept_auction_update(const AuctionUpdate& au)
  {
    m_evt->check_auction(const_cast<AuctionUpdate&>(au));
    m_md->subscribers().update_subscribers(const_cast<AuctionUpdate&>(au));
  }
  
  void QBT_Event::QBT_Event_Acceptor::accept_alert_event(const AlertEvent& alert)
  {
    
  }
  
  void QBT_Event::QBT_Event_Acceptor::accept_orderbook_update(const OrderBookUpdate& qu)
  {
    
  }
  
  
  void
  QBT_Event::init(const string& name, const Parameter& params)
  {
    MDProvider::init(name, params);
    m_receiver->init(name, params);
    m_receiver->subscribe_all();
  }
  
  void 
  QBT_Event::subscribe_product(const ProductID& id_,const ExchangeID& exch_,
                               const string& mdSymbol_,const string& mdExch_)
  {
    m_receiver->subscribe_product(id_);
  }
  
  void
  QBT_Event::inactivity_notification()
  {
    
  }
  
  void
  QBT_Event::start()
  {
    //const char* where = "QBT_Event:start ";
    if(!is_started) {
      MDProvider::start();
      m_receiver->start();
    }
  }
  
  void
  QBT_Event::stop()
  {
    if(m_started) {
      m_started = false;
      MDProvider::stop();
      m_receiver->stop();
    }
  }
  
  QBT_Event::QBT_Event(MDManager& mgr) :
    MDProvider(mgr),
    m_receiver(new QBT_Event_Receiver(&mgr.app())),
    m_started(false)
  {
    m_acceptor.reset(new QBT_Event_Acceptor(this, &mgr));
    m_receiver->set_acceptor(m_acceptor.get());
  }
  
  QBT_Event::~QBT_Event()
  {
    stop();
  }
  
}
