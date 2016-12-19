#include <app/app.h>
#include <Util/ObjectFactory.h>
#include <Data/QRec.h>
#include <LLIO/EPoll_Dispatcher.h>
#include <MD/MDPrice.h>
#include <Product/Product_Manager.h>

#include <Admin/Admin_Request_Handler.h>
#include <Event_Hub/Event_Hub.h>
#include <Interface/AdminRequest.h>

#include <Logger/Logger_Manager.h>

#include <LLIO/MsgPack.h>
#include <ITCH/Snapshot_Handler.h>
#include <ITCH/Snapshot_Session.h>


namespace hf_cwtest {
		using namespace hf_tools;
		using namespace hf_core;

static FactoryRegistrar<string,Session,Snapshot_Session> r("Snapshot_Session");
static FactoryRegistrar<string,TCP_Session,Snapshot_Session> r2("Snapshot_Session");
  
  static const uint8_t max_supported_version = 1;

  Snapshot_Session_Config::Snapshot_Session_Config() :
    alert_frequency(seconds(5)),
	m_file_name("asx_users.lst"),
    version(0)
  {
    heartbeat_interval = seconds(30);
  }

  Snapshot_Session_Config::Snapshot_Session_Config(string filename):
	Snapshot_Session_Config(){
		m_file_name=filename; 
  }
  
  Snapshot_Session_Config::~Snapshot_Session_Config()
  {
  }

  int
  Snapshot_Session::handle_fd_event(int fd, int event_mask)
  {
    TCP_SESSION_HANDLE_FD_CONNECT();
    
    ++m_handle_reads;
    
    Time now = Time::current_time();
    m_recv_heartbeat_deadline = now + m_heartbeat_interval + m_heartbeat_interval;
    int ret;
    ssize_t min_len=(ssize_t)sizeof(itch_session_packet::SoupBinTcpBeat);
    do {
      ret = m_conn.handle_fd_event(fd, event_mask);
      
      if(m_conn.read_buffer_len() < min_len){
        break;
      }
      
      itch_session_packet::SoupBinTcpBeat* h = reinterpret_cast<itch_session_packet::SoupBinTcpBeat*>(m_conn.read_buffer_begin());
      
      while(m_conn.read_buffer_len() >= min_len &&
			      m_conn.read_buffer_len() >= h->packet_length) {
        ++m_handle_messages;
        
	if(h->packet_type != 'R'){
		//log_in_msg(now, m_conn.read_buffer_begin(), h->len);
        }
        bool res = handle_message(fd, m_conn.read_buffer_begin());
        if(unlikely(!res)) {
          return 1;
        }
        m_conn.consume(2+h->packet_length);
        h = reinterpret_cast<itch_session_packet::SoupBinTcpBeat*>(m_conn.read_buffer_begin());
      }
      
      if(m_conn.commit()) {
        ++m_partial_reads;
      }
    } while(ret == 0);
    
    TCP_SESSION_HANDLE_FD_DISCONNECT();
    
    return m_conn.return_value();
  }
  
  bool
  Snapshot_Session::handle_message(int fd, char* msg)
  {
	  if (!m_handler) return false;
	  m_handler->process_client_data(fd, msg, 0);
    //printf("Snapshot_Session::handle_message\n");

    return true;
  }
  
  bool
  Snapshot_Session::handle_message(char* msg)
  {
	 if (!m_handler) return false;
           m_handler->process_client_data(0, msg, 0);
	  return true;
  }

  template<typename T>
  void
  print_enum(int value, char* buf, int n)
  {
    if(value <= T::size) {
      T t = (typename T::domain)(value);
      strncpy(buf, t.str(), n);
    } else {
      snprintf(buf, n, "(%d)", value);
    }
  }
    
  
  int
  static
  dump_extensions(ILogger& log, const char* msg, size_t len)
  {
    uint16_t extension_offset = ((const QMessage::heartbeat*) msg)->extension_offset;
    
    if(!extension_offset) {
      log.printf("  (no extensions)");
      return 0;
    }
    
    uint8_t num_extensions = msg[extension_offset];

    log.printf("\n   %d extensions\n", num_extensions);
    
    const char* buf = msg + extension_offset + 1;
    //for(int ext = 0; ext < num_extensions; ++ext) {
    buf = MsgPack::dump(buf, msg+len, log, 3, num_extensions*2);
    //buf = MsgPack::dump(buf, msg+len, log, 3);
    //}
    
    return num_extensions;
  }
  
  bool
  Snapshot_Session::handle_timer_event(LLIO::Timer* timeNow )
  {
           if (!m_handler) return false;
           m_handler->check_time_out();
    return true;
  }
    
  void
  Snapshot_Session::handle_connect()
  {
    TCP_Session::handle_connect();
    //send_login();
    //m_need_outgoing_login = true;
    Time now = Time::current_time();
    m_send_heartbeat_deadline = now + m_heartbeat_interval;
    m_recv_heartbeat_deadline = now + m_heartbeat_interval + m_heartbeat_interval;
  }

  void
  Snapshot_Session::handle_disconnect() {
    boost::shared_ptr<Session_Acceptor> acceptor = m_session_acceptor.lock();
    if (acceptor) {
      acceptor->disconnected(m_session_config.name);
    }
  }

  void
  Snapshot_Session::accept(int fd)
  {
	  //PENDING TO-DO
	  //this mightbe a session accepting mutiple connections
	  //with service providing mutiple access points
    m_conn.fd = fd;
    m_conn.set_nodelay();
    m_conn.set_nonblocking();
    
    //m_need_incoming_login = true;
    //m_should_reconnect = false;

    Time now = Time::current_time();
    m_send_heartbeat_deadline = now + m_config.heartbeat_interval;
    m_recv_heartbeat_deadline = now + m_config.heartbeat_interval;
    //m_recv_login_deadline = now + m_config.heartbeat_interval;
    
    m_disp->register_IO_Object(this, m_conn, true, true, true);
    setup_timer("Snapshot_Session");//m_conn.remote_endpoint());
  }

  Snapshot_Session::Snapshot_Session(LLIO::EPoll_Dispatcher* disp, Snapshot_Handler* handler) :
    TCP_Session(),
    //m_need_incoming_login(true),
    //m_need_outgoing_login(false),
	//m_should_reconnect(false),
	  //m_app(),
	  m_config(),
	  m_disp(disp),
	  m_handler(handler)
  {
    m_timer_definition.name = "Snapshot_Session ";// + m_config.name();
    m_timer_definition.period = seconds(1);
  }
 
  Snapshot_Session::Snapshot_Session() :
    TCP_Session()
    //m_need_incoming_login(false),
    //m_need_outgoing_login(false)
  {
  }

  void Snapshot_Session::initialize(Application* app, const Parameter& param_)
  {
    TCP_Session::initialize(app, param_);

    m_timer_definition.name = "Snapshot_Session ";// + m_config.name;
    m_timer_definition.period = seconds(1);

    // TODO: GET_CONFIG_FIELD(m_config, name, getString)
    // build the user list from configuration file
    /*
    if(param_.has("username")) { m_config.username = param_["username"].getString(); }
    if(param_.has("resend_wait_time")) { m_config.resend_wait_time = param_["resend_wait_time"].getTimeDuration(); }
    if(param_.has("alert_frequency")) { m_config.alert_frequency = param_["alert_frequency"].getTimeDuration(); }
    if(param_.has("buffer_size")) { m_config.buffer_size = param_["buffer_size"].getInt(); }
    if(param_.has("gap_fill")) { m_config.gap_fill = param_["gap_fill"].getBool(); }
    if(param_.has("resend_data")) { m_config.resend_data = param_["resend_data"].getBool(); }
    if(param_.has("log_heartbeats")) { m_config.log_heartbeats = param_["log_heartbeats"].getBool(); }
    if(param_.has("version")) { m_config.version = static_cast<uint8_t>(param_["version"].getInt()); }
    */
  } 
  
  Snapshot_Session::~Snapshot_Session()
  {
  }
  
}
