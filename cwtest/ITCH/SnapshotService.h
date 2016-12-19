#ifndef hf_cwtest_ITCH_SnapshotService_h
#define hf_cwtest_ITCH_SnapshotService_h

//#include <Util/intrusive_ptr_base.h>
//#include <app/app.h>
#include <Util/Parameter.h>

#include <LLIO/EPoll_Dispatcher.h>
#include <HTTP/HTTP_Connection.h>

#include <ITCH/Snapshot_Session.h>
#include <ITCH/ASX_Test_Handler.h>

#include <Event/Event_Acceptor.h>

#include <tbb/concurrent_hash_map.h>

namespace hf_cwtest {

	using namespace hf_core;
	using namespace hf_tools;
  

  struct SnapshotService_Config { 
	  int port; 
	  //this is a multiple port service so this port will be a dummy
	  //those ports will be opened in the Session opened by this service
	  string realm; 
	  vector<string> user_files; 
	  string user_info_file_name;
	  //this user information will also be stored under Session 
	  Time_Duration alert_frequency; 
	  SnapshotService_Config(int pport=0):port(pport), alert_frequency(seconds(5)){}; 
  };

  class SnapshotService// : public Event_Acceptor
  {
  public:
    
    void init();
    void build_down_stream_reservoir(const string& fileName);
    
    void start();
    void stop();
    
    // Implement Event_Acceptor events we care about
    virtual const string& name() const { return m_name; }
    
    SnapshotService();
    SnapshotService(Application* app,  const Parameter& params);
    SnapshotService(Application* app, const SnapshotService_Config& config, const Parameter& params);

    ~SnapshotService();
    
  private:
    static const string m_name;
    int m_port;
    Application  m_app;
    Parameter m_param;

    boost::shared_ptr<LLIO::EPoll_Dispatcher> m_disp;
    
    boost::intrusive_ptr<Snapshot_Session> create_snapshot_session();
    
    SnapshotService_Config m_config;
    
    boost::shared_ptr<ASX_Test_Handler> m_handler;
    
    LLIO::EPoll_TCP_AcceptorRP m_acceptor;
    
    tbb::concurrent_hash_map<string, Snapshot_SessionRP> m_connections;
    
    LoggerRP m_logger;
  };

}

#endif /* ifndef hf_cwtest_ITCH_SnapshotService_h */
