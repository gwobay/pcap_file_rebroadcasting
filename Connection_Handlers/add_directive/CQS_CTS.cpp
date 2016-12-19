#include <QBT/CQS_CTS.h>

#include <MD/MDManager.h>
#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>

namespace hf_md {

  void
  CQS_CTS::init_mcast(const string& line_name, const Parameter& params, bool cqs)
  {
    if(!params.has(line_name)) {
      return;
    }

    CQS_CTS_Handler::channel_info_map_t& cim = m_handler->m_channel_info_map;

    const vector<Parameter>& lines = params[line_name].getVector();
    cim.reserve(cim.size() + lines.size());
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      const string& name = (*l)["name"].getString();
      const string& A_group = (*l)["A_group"].getString();
      int A_port = (*l)["A_port"].getInt();
      const string& A_iface = (*l)["A_iface"].getString();

      size_t context = cim.size();
      
      m_disp->mcast_join(m_handler.get(), "A", context, A_group, A_port, A_iface, "");
      
      if(l->has("B_group"))
      {
        const string& B_group = (*l)["B_group"].getString();
        int B_port = (*l)["B_port"].getInt();
        const string& B_iface = (*l)["B_iface"].getString();
        m_disp->mcast_join(m_handler.get(), "B", context, B_group, B_port, B_iface, "");
      }

      cim.push_back(CQS_CTS_Handler::channel_info_t(name, context, cqs));
      cim.back().init(params);
    }

    m_logger->log_printf(Log::INFO, "%s %s: Lines for %s initialized", "CQS_CTS", m_name.c_str(), line_name.c_str());
  }

  void
  CQS_CTS::init(const string& name,const Parameter& params)
  {
    MDProvider::init(name, params);

    m_disp = m_md.get_dispatcher(params.getDefaultString("dispatcher", name));

    m_handler->set_subscribers(m_md.subscribers_rp());
    m_handler->set_provider(this);
    m_handler->init(name, params);

    init_mcast("cqs_lines", params, true);
    init_mcast("cts_lines", params, false);
  }

  void
  CQS_CTS::start(void)
  {
    if(!is_started) {
      MDProvider::start();
      m_handler->start();
      if(m_disp) {
        m_disp->start(m_handler.get(), "A");
        //m_disp->start(m_handler.get(), "B");
      }
    }
  }

  void
  CQS_CTS::stop(void)
  {
    if(is_started) {
      if(m_disp) {
        m_disp->stop(m_handler.get(), "B");
        m_disp->stop(m_handler.get(), "A");
      }
      m_handler->stop();
      MDProvider::stop();
    }
  }

  void
  CQS_CTS::subscribe_product(const ProductID& id, const ExchangeID& exch,
                            const string& mdSymbol, const string& mdExch)
  {
    m_handler->subscribe_product(id, exch, mdSymbol, mdExch);
  }

  CQS_CTS::CQS_CTS(MDManager& md) :
    MDProvider(md),
    m_disp(0)
  {
    m_handler.reset(new CQS_CTS_Handler(&md.app()));
  }

  CQS_CTS::~CQS_CTS()
  {
    stop();
  }

}
