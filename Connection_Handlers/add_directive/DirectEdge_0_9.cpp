#include <QBT/DirectEdge_0_9.h>
#include <Util/Network_Utils.h>

#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>

namespace hf_md {

  void
  DirectEdge_0_9::start()
  {
    if(!is_started) {
      MDProvider::start();
      m_edge_parser->start();
      m_dispatcher->start(m_edge_parser.get(), "A");
      m_dispatcher->start(m_edge_parser.get(), "B");
    }
  }

  void
  DirectEdge_0_9::stop()
  {
    m_dispatcher->stop(m_edge_parser.get(), "B");
    m_dispatcher->stop(m_edge_parser.get(), "A");
    m_edge_parser->stop();
    MDProvider::stop();
  }

  void
  DirectEdge_0_9::init_mcast(const string& line_name, const Parameter& params)
  {
    if(!params.has(line_name)) {
      return;
    }

    const vector<Parameter>& lines = params[line_name].getVector();

    int max_unit = 0;
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      int unit = (*l)["unit"].getInt();
      max_unit = std::max(unit, max_unit);
    }

    m_edge_parser->m_channel_info_map.resize(max_unit + 1);

    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      //const string& name = (*l)["name"].getString();

      int unit = (*l)["unit"].getInt();

      const string& A_group = (*l)["A_group"].getString();
      int A_port = (*l)["A_port"].getInt();
      const string& A_iface = (*l)["A_iface"].getString();

      size_t context = unit;
      m_dispatcher->mcast_join(m_edge_parser.get(), "A", context, A_group, A_port, A_iface, "");

      if ((*l).has("B_group")) {
        const string& B_group = (*l)["B_group"].getString();
        int B_port = (*l)["B_port"].getInt();
        const string& B_iface = (*l)["B_iface"].getString();
        m_dispatcher->mcast_join(m_edge_parser.get(), "B", context, B_group, B_port, B_iface, "");
      }

      m_edge_parser->m_channel_info_map[unit].context = context;
      m_edge_parser->m_channel_info_map[unit].init(params);
    }

  }


  void
  DirectEdge_0_9::init(const string& name,const Parameter& params)
  {
    MDProvider::init(name, params);

    if(m_supported_exchanges.size() != 1) {
      throw runtime_error("DirectEDGE Parser supports exactly 1 exchange per Provider");
    }
    ExchangeID exch = m_supported_exchanges[0].exch;

    m_edge_parser.reset(new DirectEdge_0_9_Handler(&m_md.app()));
    m_edge_parser->set_subscribers(m_md.subscribers_rp());
    m_edge_parser->set_provider(this);
    m_edge_parser->init(name, exch, params);

    m_order_books = m_edge_parser->order_books();

    m_dispatcher = m_md.get_dispatcher(params.getDefaultString("dispatcher", name));
    if(params.has("recovery_dispatcher")) {
      m_recovery_dispatcher = m_md.get_dispatcher(params.getDefaultString("recovery_dispatcher", name));
    }

    init_mcast("edge", params);
  }

  void
  DirectEdge_0_9::subscribe_product(const ProductID& id_,const ExchangeID& exch_,
                                    const string& mdSymbol_,const string& mdExch_)
  {
    if(m_edge_parser) {
      m_edge_parser->subscribe_product(id_, exch_, mdSymbol_, mdExch_);
    }
  }

  DirectEdge_0_9::DirectEdge_0_9(MDManager& md) :
    MDProvider(md),
    m_dispatcher(0),
    m_recovery_dispatcher(0)
  {
  }

  DirectEdge_0_9::~DirectEdge_0_9()
  {
    stop();
  }


}
