#include <QBT/MDP_FAST.h>
#include <Util/Network_Utils.h>

#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>

namespace hf_md {

  void
  MDP_FAST::start()
  {
    if(!is_started) {
      MDProvider::start();
      m_mdp_parser->start();
      m_dispatcher->start(m_mdp_parser.get(), "A");
      m_dispatcher->start(m_mdp_parser.get(), "B");
    }
  }

  void
  MDP_FAST::stop()
  {
    m_dispatcher->stop(m_mdp_parser.get(), "A");
    m_dispatcher->stop(m_mdp_parser.get(), "B");
    m_mdp_parser->stop();
    MDProvider::stop();
  }

  void
  MDP_FAST::init_mcast(const string& line_name, const Parameter& params)
  {
    if(!params.has(line_name)) {
      return;
    }

    const vector<Parameter>& lines = params[line_name].getVector();

    size_t context = 0;
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      //const string& name = (*l)["name"].getString();

      const string& A_group = (*l)["A_group"].getString();
      int A_port = (*l)["A_port"].getInt();
      const string& A_iface = (*l)["A_iface"].getString();

      m_dispatcher->mcast_join(m_mdp_parser.get(), "A", context, A_group, A_port, A_iface, "");

      if ((*l).has("B_group")) {
        const string& B_group = (*l)["B_group"].getString();
        int B_port = (*l)["B_port"].getInt();
        const string& B_iface = (*l)["B_iface"].getString();
        m_dispatcher->mcast_join(m_mdp_parser.get(), "B", context, B_group, B_port, B_iface, "");
      }

      ++context;
    }
  }


  void
  MDP_FAST::init(const string& name,const Parameter& params)
  {
    MDProvider::init(name, params);

    if(m_supported_exchanges.size() != 1) {
      throw runtime_error("CME MDP FIX/FAST Parser supports exactly 1 exchange per Provider");
    }

    ExchangeID exch = m_supported_exchanges[0].exch;

    m_mdp_parser.reset(new CME_MDP_FAST_Handler(&m_md.app()));
    m_mdp_parser->set_subscribers(m_md.subscribers_rp());
    m_mdp_parser->set_provider(this);
    m_mdp_parser->init(name, exch, params);

    m_order_books = m_mdp_parser->order_books();
    //m_secondary_order_book = m_mdp_parser->auction_order_book();

    m_dispatcher = m_md.get_dispatcher(params.getDefaultString("dispatcher", name));
    if(params.has("recovery_dispatcher")) {
      m_recovery_dispatcher = m_md.get_dispatcher(params.getDefaultString("recovery_dispatcher", name));
    }

    m_recovery_timeout = params.getDefaultTimeDuration("recovery_timeout", msec(100));

    init_mcast("cme", params);
  }

  void
  MDP_FAST::subscribe_product(const ProductID& id_,const ExchangeID& exch_,
                                const string& mdSymbol_,const string& mdExch_)
  {
    if(m_mdp_parser) {
      m_mdp_parser->subscribe_product(id_, exch_, mdSymbol_, mdExch_);
    }
  }

  MDP_FAST::MDP_FAST(MDManager& md) :
    MDProvider(md),
    m_dispatcher(0),
    m_recovery_dispatcher(0)
  {
  }

  MDP_FAST::~MDP_FAST()
  {
    stop();
  }


}
