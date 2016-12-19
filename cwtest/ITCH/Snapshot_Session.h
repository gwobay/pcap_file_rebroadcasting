#ifndef hf_core_Session_Snapshot_Session_h
#define hf_core_Session_Snapshot_Session_h

#include <ITCH/Snapshot_Handler.h>
#include <Session/TCP_Session.h>
#include <Session/Session_Acceptor.h>

#include <LLIO/EPoll_Dispatcher.h>
#include <LLIO/QMessage.h>
#include <app/Config.h>

#include <Logger/LoggerEnum.h>

#include <Interface/AlertEvent.h>
#include <Interface/Order.h>
#include <Interface/OrderResponse.h>
//#include <ITCH/ASX_Test_Handler.h>
#include <Util/Time.h>

#include <boost/enable_shared_from_this.hpp>


namespace hf_cwtest {
  using namespace hf_tools;
  using namespace hf_core;
  
  class Snapshot_Handler;

  class Snapshot_Session_Config : public TCP_Session_Config
  {
  public:
      Time_Duration heartbeat_interval;
      Time_Duration alert_frequency;
	struct user_info{
    		string username;
    		string password;
    		Time_Duration heartbeat_interval;
        		Time_Duration alert_frequency;
	  };
	  vector<struct user_info> m_users;

	  vector<int> m_listen_ports;

	  
	string&  name(){return m_file_name;}
	string&  operator()(){return m_file_name;}
    int socket_buffer_size;
    
    //bool gap_fill;
    //bool resend_data;
    //bool log_heartbeats;
    
    uint8_t version;
    
    Snapshot_Session_Config();
    Snapshot_Session_Config(string filename);
    
    virtual ~Snapshot_Session_Config();
  private:
    string m_file_name;
  };
  
  
  class Snapshot_Session : public TCP_Session
  {
  public:
    
    // TCP Session methods
    virtual const char*   type() const { return "Snapshot_Session"; }
    virtual int  handle_fd_event(int fd, int event_mask);
    //fd will be used to locate the channel info in handler
    virtual bool handle_timer_event(LLIO::Timer* t);
    virtual void handle_connect();
    virtual void handle_disconnect();
    //virtual void handle_status_msg(ILogger&,bool detailed) const;
    //virtual void register_admins(Admin_Request_Handler& admin_handler, const string& base_path);

    // Session_Acceptor 
    void accept(int fd);   // fd data will be saved in the handler's channel_info

    
    // Snapshot_Session methods
    virtual bool  handle_message(char* buf);
    virtual bool  handle_message(int fd, char* buf);
    //static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);
    
    void set_session_acceptor(boost::weak_ptr<Session_Acceptor> sa) {
	    m_session_acceptor = sa;
    }
      
    Snapshot_Session();

    Snapshot_Session(Application* app, const Snapshot_Session_Config& config);

    Snapshot_Session(LLIO::EPoll_Dispatcher* disp,
		    Snapshot_Handler* handler);

    virtual void initialize(Application* app, const Parameter& param_);
    virtual ~Snapshot_Session();
    
  protected:
    //boost::shared_ptr<Snapshot_Handler> m_handler;
    boost::shared_ptr<Snapshot_Handler> m_handler;

    boost::shared_ptr<LLIO::EPoll_Dispatcher> m_disp;
    // protocol
    //void handle_login(const char* msg);
    
    // admins
    //void handle_send_message(AdminRequest& req);
    
    Snapshot_Session_Config m_config;
    
    // Timer updates frequently enough (every second by default) to check for pending 
    uint8_t          m_version;
    
    boost::weak_ptr<Session_Acceptor> m_session_acceptor;
  };
  
  typedef boost::intrusive_ptr<Snapshot_Session> Snapshot_SessionRP;

}

#endif /* ifndef hf_core_Session_Snapshot_Session_h */
