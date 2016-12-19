#include <QBT/NYSE_XDP_Common.h>

#include <QBT/NYSE_Common.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>

#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>

#include <Logger/Logger_Manager.h>
#include <Util/Network_Utils.h>
#include <netinet/tcp.h>
#include <stdarg.h>

namespace hf_md {
  
  bool
  NYSE_XDP_Common::init_mcast(const string& line_name, const Parameter& params)
  {
    //const char* where = "NYSE_XDP_Common::init_mcast";

    if(!params.has(line_name)) {
      return false;
    }

    Handler::channel_info_map_t& cim = m_parser->m_channel_info_map;
    
    const vector<Parameter>& lines = params[line_name].getVector();
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      const string& name = (*l)["name"].getString();
      const string& A_group = (*l)["A_group"].getString();
      int A_port = (*l)["A_port"].getInt();
      const string& A_iface = (*l)["A_iface"].getString();

      const string& B_group = (*l)["B_group"].getString();
      int B_port = (*l)["B_port"].getInt();
      const string& B_iface = (*l)["B_iface"].getString();

      //const string& p_source = (*l)["primary_source"].getString();

      size_t context = cim.size();
      cim.push_back(channel_info(name, context));
      channel_info& ci = cim.back();
      ci.init(params);

      m_disp->mcast_join(m_parser.get(), "A", context, A_group, A_port, A_iface, "");
      m_disp->mcast_join(m_parser.get(), "B", context, B_group, B_port, B_iface, "");

      if(l->has("A_retrans_group") && m_recov_disp) {
        ci.has_recovery = true;

        const string& A_retrans_group = (*l)["A_retrans_group"].getString();
        int A_retrans_port = (*l)["A_retrans_port"].getInt();

        const string& B_retrans_group = (*l)["B_retrans_group"].getString();
        int B_retrans_port = (*l)["B_retrans_port"].getInt();

        m_disp->mcast_join(m_parser.get(), "A", context, A_retrans_group, A_retrans_port, A_iface, "");
        m_disp->mcast_join(m_parser.get(), "B", context, B_retrans_group, B_retrans_port, B_iface, "");

        const string& retrans_ip = (*l)["retrans_ip"].getString();
        int retrans_port = (*l)["retrans_port"].getInt();
	
        memset(&ci.recovery_addr, 0, sizeof(ci.recovery_addr));
        fill_sockaddr_in(ci.recovery_addr, retrans_ip.c_str(), retrans_port);
      }
      
      if(l->has("start")) {
        const string& start = (*l)["start"].getString();
        const string& end = (*l)["end"].getString();
        for(char c = start[0]; c <= end[0]; ++c) {
          m_char_to_channel_info_map.insert(make_pair(c, context));
        }
      }

      // Interval group always has a context of +1 to the main group
      if(l->has("A_interval_group")) {
        size_t context = cim.size();
        cim.push_back(channel_info(name+"_int", context));

        const string& A_interval_group = (*l)["A_interval_group"].getString();
        int A_interval_port = (*l)["A_interval_port"].getInt();

        const string& B_interval_group = (*l)["B_interval_group"].getString();
        int B_interval_port = (*l)["B_interval_port"].getInt();

        m_disp->mcast_join(m_parser.get(), "A", context, A_interval_group, A_interval_port, A_iface, "");
        m_disp->mcast_join(m_parser.get(), "B", context, B_interval_group, B_interval_port, B_iface, "");
      }
    }
    
    m_parser->set_num_contexts(cim.size());
    
    m_logger->log_printf(Log::INFO, "%s %s: Lines for %s initialized", "NYSE_XDP_Common", m_name.c_str(), line_name.c_str());

    return true;
  }

  void
  NYSE_XDP_Common::init(const string& name,const Parameter& params)
  {
    MDProvider::init(name, params);
    if(m_supported_exchanges.size() != 1) {
      throw runtime_error("NYSE_XDP_Common Parser supports exactly 1 exchange per Provider");
    }

    ExchangeID exch = m_supported_exchanges[0].exch;

    m_disp = m_md.get_dispatcher(params.getDefaultString("dispatcher", name));
    if(params.has("recovery_dispatcher")) {
      m_recov_disp = m_md.get_dispatcher(params["recovery_dispatcher"].getString());
    }

    bool has_nyse = params.has("nyse_bbo") || params.has("nyse_trades")  || params.has("nyse_imbalances")  || params.has("nyse_alerts") || params.has("nyse_openbook");
    bool has_arca = params.has("arca_trades") || params.has("arca_book");

    if(has_nyse && has_arca) {
      throw runtime_error("NYSE_XDP_Common Parser has both NYSE and ARCA feeds shared, one parser can only have 1 parser type");
    }
    if(!(has_nyse || has_arca)) {
      throw runtime_error("NYSE_XDP_Common Parser has valid feed type configured");
    }

    if(has_nyse) {
      NYSE_XDP_Handler* nh = new NYSE_XDP_Handler(&m_md.app());
      m_parser.reset(nh);
      
      // This is used by the NYSETRADES feed handler to be able to determine the side of a trade given the link id
      if(params.has("linkid_provider")) {
        string oh = params["linkid_provider"].getString();
	
        MDProvider* obh = m_md.get_provider(oh);
        if(!obh) {
          throw runtime_error("NYSE_Common linkid_provider '" + oh + "' not found");
        }
	
        NYSE_Common* nc = dynamic_cast<NYSE_Common*>(obh);
        if(!nc) {
          throw runtime_error("NYSE_Common linkid_provider '" + oh + "' required to be of type NYSE_Common");
        }
	
        nh->set_linkid_source(nc->m_parser.get());
      }
      
    } else if(has_arca) {
      m_parser.reset(new NYSE_XDP_Handler(&m_md.app()));
    }

    m_parser->set_subscribers(m_md.subscribers_rp());
    m_parser->set_provider(this);
    m_parser->init(name, exch, params);
    m_order_books = m_parser->order_books();

    init_mcast("nyse_bbo", params);
    init_mcast("nyse_trades", params);
    init_mcast("nyse_imbalances", params);
    init_mcast("nyse_alerts", params);
    init_mcast("nyse_openbook", params);
    init_mcast("arca_trades", params);
    init_mcast("arca_book", params);
  }

  void
  NYSE_XDP_Common::establish_retransmission_connections()
  {
    const char* where = "NYSE_XDP_Common::establish_retransmission_connections";

    Handler::channel_info_map_t& cim = m_parser->m_channel_info_map;

    for(Handler::channel_info_map_t::iterator c = cim.begin(); c != cim.end(); ++c) {
      channel_info& ci = *c;
      if(ci.has_recovery) {
        int ret = ci.recovery_socket.connect_tcp(ci.recovery_addr);
        if(ret == 0) {
          m_parser->send_alert("%s: %s %s Unable to connect to retransmission host", where, m_name.c_str(), ci.name.c_str());
          continue;
        }
        ci.recovery_socket.set_nodelay();
        
        size_t context = c - cim.begin();
        m_recov_disp->add_tcp_fd(m_parser.get(), "retrans", context, ci.recovery_socket, 2048);

        if(fcntl(ci.recovery_socket, F_SETFL, O_NONBLOCK) < 0) {
          throw errno_exception(where + m_name + " " + " fcntl(O_NONBLOCK) failed ", errno);
        }

        m_parser->send_heartbeat_response(ci);
      }
    }
  }

  void
  NYSE_XDP_Common::destroy_retransmission_connections()
  {
    //const char* where = "NYSE_XDP_Common::destroy_retransmission_connections";

    Handler::channel_info_map_t& cim = m_parser->m_channel_info_map;

    for(Handler::channel_info_map_t::iterator c = cim.begin(); c != cim.end(); ++c) {
      channel_info& ci = *c;
      if(ci.recovery_socket != -1) {
        ci.recovery_socket.close();
      }
    }
  }

  void
  NYSE_XDP_Common::start(void)
  {
    if(!is_started) {
      MDProvider::start();
      m_parser->start();
      if(m_disp) {
        m_disp->start(m_parser.get(), "A");
        m_disp->start(m_parser.get(), "B");
      }
      if(m_recov_disp) {
        establish_retransmission_connections();
        m_recov_disp->start(m_parser.get(), "retrans");
      }
    }
  }

  void
  NYSE_XDP_Common::stop(void)
  {
    if(is_started) {
      if(m_disp) {
        m_disp->stop(m_parser.get(), "B");
        m_disp->stop(m_parser.get(), "A");
      }
      if(m_recov_disp) {
        destroy_retransmission_connections();
        m_recov_disp->stop(m_parser.get(), "retrans");
      }
    }
    m_parser->stop();
    MDProvider::stop();
  }

  void
  NYSE_XDP_Common::subscribe_product(const ProductID& id, const ExchangeID& exch,
                                     const string& mdSymbol, const string& mdExch)
  {
    const Product& prod = m_md.app().pm()->find_by_id(id);
    
    if(!m_char_to_channel_info_map.empty()) {
      size_t context = m_char_to_channel_info_map[prod.symbol()[0]];
      m_parser->add_context_product_id(context, id);
    }

    m_parser->subscribe_product(id, exch, mdSymbol, mdExch);
  }
  
  bool
  NYSE_XDP_Common::drop_packets(int num_packets)
  {
    if(m_parser) {
      m_parser->drop_packets(num_packets);
      return true;
    }
    return false;
  }
  
  NYSE_XDP_Common::NYSE_XDP_Common(MDManager& md) :
    MDProvider(md),
    m_disp(0),
    m_recov_disp(0)
  {

  }

  NYSE_XDP_Common::~NYSE_XDP_Common()
  {
    stop();
  }



}
