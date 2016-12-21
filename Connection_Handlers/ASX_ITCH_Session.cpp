#include <QBT/ASX_ITCH_Session.h>
#include <QBT/EPoll_Dispatcher.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>

#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>

#include <Logger/Logger_Manager.h>
#include <Util/Network_Utils.h>
#include <netinet/tcp.h>
#include <stdarg.h>

namespace kou_connection {
  
  bool
  ASX_ITCH_Session::init_mcast(const string& line_name, const Parameter& params)
  {
    string where= __func__;

    if(!params.has(line_name)) {
      return false;
    }

    Handler::channel_info_map_t& cim = m_handler->m_channel_info_map;
 
    const vector<Parameter>& lines = params[line_name].getVector();
    cout << where << endl;
    size_t num_contexts = lines.size();
    cout << "num_contexts=" << num_contexts << endl;
    m_handler->set_num_contexts(num_contexts); // ToDo: can get rid of this I think
    cout << "Returned from setting num contexts" << endl;
    size_t context = 1;
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
      //m_disp->mcast_join(m_handler.get(), "A", context, A_group, A_port, A_iface, "");
      m_disp->mcast_join(m_handler.get(), name, context, A_group, A_port, A_iface, "");
      //m_disp->mcast_join(m_handler.get(), "B", context, B_group, B_port, B_iface, "");
      m_disp->mcast_join(m_handler.get(), name, context, B_group, B_port, B_iface, "");

      if(l->has("start")) {
        const string& start = (*l)["start"].getString();
        const string& end = (*l)["end"].getString();
        for(char c = start[0]; c <= end[0]; ++c) {
          m_char_to_channel_info_map.insert(make_pair(c, context));
        }
      }
      cout << "Channel " << name << " has context=" << context << " (multicast)" << endl;
      ++context;
    }

    return true;
  }

  bool
  ASX_ITCH_Session::init_recovery_channel(const string& line_name, const Parameter& params)
  {
    string where= __func__;

    if(!params.has(line_name)) {
      return false;
    }

    Handler::channel_info_map_t& cim = m_handler->m_channel_info_map;
 
    const vector<Parameter>& lines = params[line_name].getVector();
    cout << where << endl;
    size_t num_contexts = lines.size();
    cout << "num_contexts=" << num_contexts << endl;
    size_t context = 1;
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      const string& A_iface = (*l)["A_iface"].getString();
      const string& B_iface = (*l)["B_iface"].getString();
      const string& name = (*l)["name"].getString();

      channel_info* ci;
      channel_info* p=m_handler->m_channel_info_map.data();

      for (vector<channel_info>::iterator ch=m_handler->m_channel_info_map.begin(); ch !=m_handler->m_channel_info_map.end(); ch++, p++){
	      if (p->name != name || p->context != context)
		      continue;
	      ci=p;
	      break;
	}
      if (!ci)
      {
      	m_handler->m_channel_info_map.push_back(channel_info(name, context));
	     ci=&m_handler->m_channel_info_map.back(); 
      ci->init(params);
      ci->seq_no = 0;
      }

      const string& A_group = (*l)["A_group"].getString();
      int A_port = (*l)["A_port"].getInt();
      const string& B_group = (*l)["B_group"].getString();
      int B_port = (*l)["B_port"].getInt();
      fill_sockaddr_in(ci->recovery_addr, A_group.c_str(), A_port);
      ++context;
    }
    return true;
  }

  bool
  ASX_ITCH_Session::init_snapshot_channel(const string& line_name, const Parameter& params)
  {
    string where= __func__;

    if(!params.has(line_name)) {
      return false;
    }

    Handler::channel_info_map_t& cim = m_handler->m_channel_info_map;
 
    const vector<Parameter>& lines = params[line_name].getVector();
    cout << where << endl;
    size_t num_contexts = lines.size();
    cout << "num_contexts=" << num_contexts << endl;
    size_t context = 1;
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      const string& A_iface = (*l)["A_iface"].getString();
      const string& B_iface = (*l)["B_iface"].getString();
      const string& name = (*l)["name"].getString();

      channel_info* ci;
      for (size_t i=0; i< m_handler->m_channel_info_map.size(); i++){
	      if ( m_handler->m_channel_info_map[i].name != name || 
		       m_handler->m_channel_info_map[i].context != context)
	      continue;
	      ci=&m_handler->m_channel_info_map[i];
	      break;
      }
      if (!ci)
      {
      	m_handler->m_channel_info_map.push_back(channel_info(name, context));
	     ci = &(m_handler->m_channel_info_map.back()); 
      	ci->init(params);
      	ci->seq_no = 0;
      }

      const string& A_group = (*l)["A_group"].getString();
      int A_port = (*l)["A_port"].getInt();
      const string& B_group = (*l)["B_group"].getString();
      int B_port = (*l)["B_port"].getInt();
      fill_sockaddr_in(ci->snapshot_addr, A_group.c_str(), A_port);
      ++context;
    }
    return true;
  }

  void
  ASX_ITCH_Session::init(const string& name,const Parameter& params)
{
    MDProvider::init(name, params);
    //if(m_supported_exchanges.size() != 1) {
    //  throw runtime_error("ASX_ITCH_Session Parser supports exactly 1 exchange per Provider");
    //}

    vector<ExchangeID> exchanges;
    for(vector<Supported_Exchange>::const_iterator se = m_supported_exchanges.begin(); se != m_supported_exchanges.end(); ++se) {
      exchanges.push_back(se->exch);
    }

    m_disp = m_md.get_dispatcher(params.getDefaultString("dispatcher", name));
    if(params.has("snapshot_dispatcher")) {
      m_snapshot_disp = m_md.get_dispatcher(params["snapshot_dispatcher"].getString());
    }
    if(params.has("retransmission_dispatcher")) {
      m_retransmission_disp = m_md.get_dispatcher(params["retransmission_dispatcher"].getString());
    }

    cout << "ASX_ITCH_Session::init: creating m_handler" << endl;
    m_handler.reset(new ASX_ITCH_1_1_Handler(&m_md.app()));

    m_handler->set_subscribers(m_md.subscribers_rp());
    m_handler->set_provider(this);
    m_handler->set_dispatcher(m_disp);
    m_handler->init(name, exchanges[0], params);
    m_order_books = m_handler->order_books();

   string lines_name; 
   if(params.has("mcast_lines_name")) { 
	   lines_name = params["mcast_lines_name"].getString(); 
   } else { 
	   lines_name = "mcast_lines"; 
   } 
   init_mcast(lines_name, params);

   if(params.has("recovery_lines_name")) { 
	   lines_name = params["recovery_lines_name"].getString(); 
   } else { 
	   lines_name = "recovery_lines"; 
   } 
   init_recovery_channel(lines_name, params);
   if(params.has("snapshot_lines_name")) { 
	   lines_name = params["snapshot_lines_name"].getString(); 
   } else { 
	   lines_name = "snapshot_lines"; 
   } 
   init_snapshot_channel(lines_name, params);
    //init_mcast("mcast_lines", params);
    //
    m_handler->update_channel_state();
    
  }

  void ASX_ITCH_Session::accept_admin_request(AdminRequest& request_)
  {
    m_admin_handler.accept_admin_request(request_);
  }

  void ASX_ITCH_Session::set_log_skips_admin(AdminRequest& request_, bool enable)
  {
    //m_handler->set_log_skips(enable);
    //request_.printf("ASX_ITCH_Session log_skips = %d\n", m_handler->log_skips());
  }

  void ASX_ITCH_Session::log_skips_admin(AdminRequest& request_)
  {
    //request_.printf("ASX_ITCH_Session log_skips = %d\n", m_handler->log_skips());
  }

  void
  ASX_ITCH_Session::establish_refresh_connection()
  {
    for (size_t i=0; i< m_handler->m_channel_info_map.size(); i++) { 
	    m_handler->establish_refresh_connection(m_handler->m_channel_info_map[i].context);
	    if ( m_handler->m_channel_info_map[i].snapshot_socket < 1) continue;
	    cout << "channel " << m_handler->m_channel_info_map[i].context << " snapshot socket: " <<  m_handler->m_channel_info_map[i].snapshot_socket << endl;
	    m_snapshot_disp->add_tcp_fd(m_handler.get(),  m_handler->m_channel_info_map[i].name,  m_handler->m_channel_info_map[i].context, m_handler->m_channel_info_map[i].snapshot_socket, 2048); 
    }
  }

  void
  ASX_ITCH_Session::establish_retransmission_connection()
  {
    for (size_t i=0; i< m_handler->m_channel_info_map.size(); i++) { 
    m_handler->establish_retransmission_connection(m_handler->m_channel_info_map[i].context);    
	    m_retransmission_disp->add_tcp_fd(m_handler.get(),  m_handler->m_channel_info_map[i].name,  m_handler->m_channel_info_map[i].context, m_handler->m_channel_info_map[i].recovery_socket, 2048); 
    }
  }

  void
  ASX_ITCH_Session::destroy_refresh_connections()
  {
    //const char* where = "ASX_ITCH_Session::destroy_refresh_connections";

    if(m_handler->m_snapshot_socket != -1) {
      m_handler->m_snapshot_socket.close();
    }
  }

  void
  ASX_ITCH_Session::destroy_retransmission_connections()
  {
    //const char* where = "ASX_ITCH_Session::destroy_refresh_connections";

    if(m_handler->m_retransmission_socket != -1) {
      m_handler->m_retransmission_socket.close();
    }
  }

  void
  ASX_ITCH_Session::start(void)
{
    if(!is_started) {
      MDProvider::start();
      m_handler->start();
      if(m_snapshot_disp) {
        establish_refresh_connection();
      }
      for (size_t i=0; i< m_handler->m_channel_info_map.size(); i++) {
	      string name=m_handler->m_channel_info_map[i].name;
	      size_t channel_id=m_handler->m_channel_info_map[i].context;
#ifdef CW_DEBUG
	      cout << " sync "<< name << endl;
#endif
      		if(m_disp) {
			m_handler->set_dispatcher(m_disp);
#ifdef CW_DEBUG
	      cout << " udp ";
#endif
        	m_disp->start(m_handler.get(), name);
	      	}
      	if(m_snapshot_disp) {
		if (m_snapshot_disp != m_disp) {
#ifdef CW_DEBUG
	      cout << " snapshot ";
#endif
		m_handler->set_dispatcher(m_disp);
		//reconnect will only be handled by the handler for snapshot
        	m_snapshot_disp->start(m_handler.get(), name);
		}
      	}
#ifdef CW_DEBUG
	cout << endl;
#endif
      }
      //ready to update the hanndler state when channel info are set
      m_handler->update_channel_state();
      /* send_snapshot_request is moved to update_channel_state
       * so the channel state can be updated promptly and properly 
      for (size_t i=0; i< m_handler->m_channel_info_map.size(); i++) {
	      size_t channel_id=m_handler->m_channel_info_map[i].context;
		m_handler->send_snapshot_request(channel_id);
      }
      */
   }
}

  void
  ASX_ITCH_Session::stop(void)
  {
    if(is_started) {
      if(m_disp) {
        m_disp->stop(m_handler.get(), "B");
        m_disp->stop(m_handler.get(), "A");
      }
      if(m_snapshot_disp) {
        destroy_refresh_connections();
        m_snapshot_disp->stop(m_handler.get(), "refresh");
      }
      if(m_retransmission_disp) {
        destroy_retransmission_connections();
        m_retransmission_disp->stop(m_handler.get(), "retransmission");
      }
    }
    m_handler->stop();
    MDProvider::stop();
  }

  void
  ASX_ITCH_Session::subscribe_product(const ProductID& id, const ExchangeID& exch,
                                     const string& mdSymbol, const string& mdExch)
  {
    const Product& prod = m_md.app().pm()->find_by_id(id);
    
    if(!m_char_to_channel_info_map.empty()) {
      size_t context = m_char_to_channel_info_map[prod.symbol()[0]];
      m_handler->add_context_product_id(context, id);
    }

    m_handler->subscribe_product(id, exch, mdSymbol, mdExch);
  }
  
  bool
  ASX_ITCH_Session::drop_packets(int num_packets)
  {
    if(m_handler) {
      m_handler->drop_packets(num_packets);
      return true;
    }
    return false;
  }
  
  ASX_ITCH_Session::ASX_ITCH_Session(MDManager& md) :
    MDProvider(md),
    m_disp(0),
    m_snapshot_disp(0),
    m_retransmission_disp(0)
  {

  }

  ASX_ITCH_Session::~ASX_ITCH_Session()
  {
    stop();
  }



}
