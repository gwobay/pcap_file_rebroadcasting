#include <QBT/TMX_TL1.h>
#include <Util/Network_Utils.h>

#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>

namespace hf_md {

  void
  TMX_TL1::start()
  {
    if(!is_started) {
      MDProvider::start();
      m_tmxtl1_parser->start();
      m_dispatcher->start(m_tmxtl1_parser.get(), "A");
      m_dispatcher->start(m_tmxtl1_parser.get(), "B");
    }
  }

  void
  TMX_TL1::stop()
  {
    m_dispatcher->stop(m_tmxtl1_parser.get(), "A");
    m_dispatcher->stop(m_tmxtl1_parser.get(), "B");
    m_tmxtl1_parser->stop();
    MDProvider::stop();
  }

  void
  TMX_TL1::init_mcast(const string& line_name, const Parameter& params)
  {
    if(!params.has(line_name)) {
      return;
    }

    const vector<Parameter>& lines = params[line_name].getVector();

    int max_unit = lines.size();
    //m_tmxtl1_parser->m_channel_info_map.resize(max_unit + 1);

    size_t context = 0;

    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      //const string& name = (*l)["name"].getString();

      const string& A_group = (*l)["A_group"].getString();
      int A_port = (*l)["A_port"].getInt();
      const string& A_iface = (*l)["A_iface"].getString();

      //const string& p_source = (*l)["primary_source"].getString();

      m_dispatcher->mcast_join(m_tmxtl1_parser.get(), "A", context, A_group, A_port, A_iface, "");

      if ((*l).has("B_group")) {
        const string& B_group = (*l)["B_group"].getString();
        int B_port = (*l)["B_port"].getInt();
        const string& B_iface = (*l)["B_iface"].getString();
        m_dispatcher->mcast_join(m_tmxtl1_parser.get(), "B", context, B_group, B_port, B_iface, "");
      }

      //m_tmxtl1_parser->m_channel_info_map[context].context = context;
      //m_tmxtl1_parser->m_channel_info_map[context].init(params);

      ++context;
    }

   for(size_t i = 0; i < m_tmxtl1_parser->m_channel_info_map.size(); ++i)
   {
     m_tmxtl1_parser->m_channel_info_map[i].init(params);
   }

  }


  void
  TMX_TL1::init(const string& name,const Parameter& params)
  {
    MDProvider::init(name, params);

    if(m_supported_exchanges.size() != 1) {
      throw runtime_error("Chi-X Japan MMD Parser supports exactly 1 exchange per Provider");
    }

    ExchangeID exch = m_supported_exchanges[0].exch;

    m_tmxtl1_parser.reset(new TMX_TL1_Handler(&m_md.app()));
    m_tmxtl1_parser->set_subscribers(m_md.subscribers_rp());
    m_tmxtl1_parser->set_provider(this);
    m_tmxtl1_parser->init(name, exch, params);

    m_dispatcher = m_md.get_dispatcher(params.getDefaultString("dispatcher", name));
    if(params.has("recovery_dispatcher")) {
      m_recovery_dispatcher = m_md.get_dispatcher(params.getDefaultString("recovery_dispatcher", name));
    }

    m_recovery_timeout = params.getDefaultTimeDuration("recovery_timeout", msec(100));

    init_mcast("tl1_lines", params);
  }

  void
  TMX_TL1::subscribe_product(const ProductID& id_,const ExchangeID& exch_,
                                const string& mdSymbol_,const string& mdExch_)
  {
    if(m_tmxtl1_parser) {
      m_tmxtl1_parser->subscribe_product(id_, exch_, mdSymbol_, mdExch_);
    }
  }

  TMX_TL1::TMX_TL1(MDManager& md) :
    MDProvider(md),
    m_dispatcher(0),
    m_recovery_dispatcher(0)
  {
  }

  TMX_TL1::~TMX_TL1()
  {
    stop();
  }


}
