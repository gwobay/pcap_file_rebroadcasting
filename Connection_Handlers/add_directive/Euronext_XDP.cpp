#include <QBT/Euronext_XDP.h>

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
  Euronext_XDP::init_mcast(const string& line_name, const Parameter& params)
  {
    //const char* where = "Euronext_XDP::init_mcast";

    if(!params.has(line_name)) {
      return false;
    }

    Handler::channel_info_map_t& cim = m_parser->m_channel_info_map;
 
    const vector<Parameter>& lines = params[line_name].getVector();
#ifdef CW_DEBUG
#ifdef CW_DEBUG
    cout << "Euronext_XDP::init_mcast" << endl;
#endif
#endif
    size_t num_contexts = lines.size();
#ifdef CW_DEBUG
#ifdef CW_DEBUG
    cout << "num_contexts=" << num_contexts << endl;
#endif
#endif
    m_parser->set_num_contexts(num_contexts); // ToDo: can get rid of this I think
#ifdef CW_DEBUG
#ifdef CW_DEBUG
    cout << "Returned from setting num contexts" << endl;
#endif
#endif
    size_t context = 0;
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      const string& A_iface = (*l)["A_iface"].getString();
      const string& B_iface = (*l)["B_iface"].getString();
      const string& name = (*l)["name"].getString();

      //const string& p_source = (*l)["primary_source"].getString();

      cim.push_back(channel_info(name, context));
      channel_info& ci = cim.back();
      ci.init(params);
      ci.seq_no = 0;

      //cim.push_back(channel_info(name, context+1));
      //channel_info& ci_snapshot = cim.back();
      //ci_snapshot.init(params);
      //ci_snapshot.seq_no = 0;

      const string& A_group = (*l)["A_group"].getString();
      int A_port = (*l)["A_port"].getInt();
      const string& B_group = (*l)["B_group"].getString();
      int B_port = (*l)["B_port"].getInt();
      m_disp->mcast_join(m_parser.get(), "A", context, A_group, A_port, A_iface, "");
      m_disp->mcast_join(m_parser.get(), "B", context, B_group, B_port, B_iface, "");

      if(l->has("start")) {
        const string& start = (*l)["start"].getString();
        const string& end = (*l)["end"].getString();
        for(char c = start[0]; c <= end[0]; ++c) {
          m_char_to_channel_info_map.insert(make_pair(c, context));
        }
      }
#ifdef CW_DEBUG
#ifdef CW_DEBUG
      cout << "Channel " << name << " has context=" << context << " (multicast)" << endl;
#endif
#endif
      ++context;
    }

    const string& refresh_ip = params["refresh_ip"].getString();
    int refresh_port = params["refresh_port"].getInt();	
    memset(&m_parser->m_snapshot_addr, 0, sizeof(m_parser->m_snapshot_addr));
#ifdef CW_DEBUG
#ifdef CW_DEBUG
    cout << "About to fill_sockaddr_in for " << refresh_ip.c_str() << " : " << refresh_port << endl;
#endif
#endif
    fill_sockaddr_in(m_parser->m_snapshot_addr, refresh_ip.c_str(), refresh_port);
    m_refresh_tcp_context = context;
    ++context;
    
    const string& retransmission_ip = params["retransmission_ip"].getString();
    int retransmission_port = params["retransmission_port"].getInt();	
    memset(&m_parser->m_retransmission_addr, 0, sizeof(m_parser->m_retransmission_addr));
#ifdef CW_DEBUG
#ifdef CW_DEBUG
    cout << "About to fill_sockaddr_in for " << retransmission_ip.c_str() << " : " << retransmission_port << endl;
#endif
#endif
    fill_sockaddr_in(m_parser->m_retransmission_addr, retransmission_ip.c_str(), retransmission_port);
    m_retransmission_tcp_context = context;
    ++context;
  
    int tcp_heartbeat_response_service_id = params["tcp_heartbeat_response_service_id"].getInt(); 
    m_parser->SetTcpContexts(m_refresh_tcp_context, m_retransmission_tcp_context);
    m_parser->SetTcpHeartbeatResponseServiceId(static_cast<uint16_t>(tcp_heartbeat_response_service_id));
 
    m_logger->log_printf(Log::INFO, "%s %s: Lines for %s initialized", "Euronext_XDP", m_name.c_str(), line_name.c_str());

    int cur_context = 0;
    for(vector<channel_info>::iterator it = cim.begin(); it != cim.end(); ++it)
    {
#ifdef CW_DEBUG
#ifdef CW_DEBUG
      cout << " setting context" << cur_context << " at " << &(*it) << " seq num to zero" << endl;
#endif
#endif
      it->seq_no = 0;
      ++cur_context;
    }

    return true;
  }

  void
  Euronext_XDP::init(const string& name,const Parameter& params)
  {
    MDProvider::init(name, params);
    //if(m_supported_exchanges.size() != 1) {
    //  throw runtime_error("Euronext_XDP Parser supports exactly 1 exchange per Provider");
    //}

    vector<ExchangeID> exchanges;
    for(vector<Supported_Exchange>::const_iterator se = m_supported_exchanges.begin(); se != m_supported_exchanges.end(); ++se) {
      exchanges.push_back(se->exch);
    }

    m_disp = m_md.get_dispatcher(params.getDefaultString("dispatcher", name));
    if(params.has("refresh_dispatcher")) {
      m_snapshot_disp = m_md.get_dispatcher(params["refresh_dispatcher"].getString());
    }
    if(params.has("retransmission_dispatcher")) {
      m_retransmission_disp = m_md.get_dispatcher(params["retransmission_dispatcher"].getString());
    }

#ifdef CW_DEBUG
#ifdef CW_DEBUG
    cout << "Euronext_XDP::init: creating m_parser" << endl;
#endif
#endif
    m_parser.reset(new Euronext_XDP_Handler(&m_md.app()));

    m_parser->set_subscribers(m_md.subscribers_rp());
    m_parser->set_provider(this);
    m_parser->init(name, exchanges, params);
    m_order_books = m_parser->order_books();

    
    m_admin_handler.add_command("euronext_xdp/enable_log_skips", boost::bind(&Euronext_XDP::set_log_skips_admin, this, _1, true),"Enable logging of why we're not processing incremental messages");
    m_admin_handler.add_command("euronext_xdp/disable_log_skips", boost::bind(&Euronext_XDP::set_log_skips_admin, this, _1, false),"Disable logging of why we're not processing incremental messages");
    m_admin_handler.add_command("euronext_xdp/show_log_skips", boost::bind(&Euronext_XDP::log_skips_admin, this, _1),"Are we logging skips (why we're not processing incremental messages)?");
    m_md.app().hub()->post(AdminRequestSubscribe("euronext_xdp",this,m_admin_handler.help_strings("euronext_xdp")));

    init_mcast("mcast_lines", params);
  }

  void Euronext_XDP::accept_admin_request(AdminRequest& request_)
  {
    m_admin_handler.accept_admin_request(request_);
  }

  void Euronext_XDP::set_log_skips_admin(AdminRequest& request_, bool enable)
  {
    m_parser->set_log_skips(enable);
    request_.printf("Euronext_XDP log_skips = %d\n", m_parser->log_skips());
  }

  void Euronext_XDP::log_skips_admin(AdminRequest& request_)
  {
    request_.printf("Euronext_XDP log_skips = %d\n", m_parser->log_skips());
  }

  void
  Euronext_XDP::establish_refresh_connection()
  {
    m_parser->establish_refresh_connection();    
    m_snapshot_disp->add_tcp_fd(m_parser.get(), "refresh", m_refresh_tcp_context, m_parser->m_snapshot_socket, 2048);
  }

  void
  Euronext_XDP::establish_retransmission_connection()
  {
    m_parser->establish_retransmission_connection();    
    m_retransmission_disp->add_tcp_fd(m_parser.get(), "retransmission", m_retransmission_tcp_context, m_parser->m_retransmission_socket, 2048);
  }

  void
  Euronext_XDP::destroy_refresh_connections()
  {
    //const char* where = "Euronext_XDP::destroy_refresh_connections";

    if(m_parser->m_snapshot_socket != -1) {
      m_parser->m_snapshot_socket.close();
    }
  }

  void
  Euronext_XDP::destroy_retransmission_connections()
  {
    //const char* where = "Euronext_XDP::destroy_refresh_connections";

    if(m_parser->m_retransmission_socket != -1) {
      m_parser->m_retransmission_socket.close();
    }
  }

  void
  Euronext_XDP::start(void)
  {
    if(!is_started) {
      MDProvider::start();
      m_parser->start();
      if(m_disp) {
        m_disp->start(m_parser.get(), "A");
        m_disp->start(m_parser.get(), "B");
      }
      if(m_snapshot_disp) {
        establish_refresh_connection();
        m_snapshot_disp->start(m_parser.get(), "refresh");
      }
      if(m_retransmission_disp) {
        establish_retransmission_connection();
        m_retransmission_disp->start(m_parser.get(), "retransmission");
      }
    }
  }

  void
  Euronext_XDP::stop(void)
  {
    if(is_started) {
      if(m_disp) {
        m_disp->stop(m_parser.get(), "B");
        m_disp->stop(m_parser.get(), "A");
      }
      if(m_snapshot_disp) {
        destroy_refresh_connections();
        m_snapshot_disp->stop(m_parser.get(), "refresh");
      }
      if(m_retransmission_disp) {
        destroy_retransmission_connections();
        m_retransmission_disp->stop(m_parser.get(), "retransmission");
      }
    }
    m_parser->stop();
    MDProvider::stop();
  }

  void
  Euronext_XDP::subscribe_product(const ProductID& id, const ExchangeID& exch,
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
  Euronext_XDP::drop_packets(int num_packets)
  {
    if(m_parser) {
      m_parser->drop_packets(num_packets);
      return true;
    }
    return false;
  }
  
  Euronext_XDP::Euronext_XDP(MDManager& md) :
    MDProvider(md),
    m_disp(0),
    m_snapshot_disp(0),
    m_retransmission_disp(0)
  {

  }

  Euronext_XDP::~Euronext_XDP()
  {
    stop();
  }



}
