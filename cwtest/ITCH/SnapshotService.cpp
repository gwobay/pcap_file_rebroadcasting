#include <ITCH/SnapshotService.h>
#include <ITCH/Snapshot_Handler.h>
#include <ITCH/ASX_Test_Handler.h>


#include <Logger/Logger.h>
#include <Logger/Logger_Manager.h>

#include <Event_Hub/Event_Hub.h>
#include <Util/intrusive_ptr_base.h>

#include <boost/intrusive_ptr.hpp>
#include <sstream>

namespace hf_cwtest {

	using namespace hf_tools;
	using namespace hf_core;
  
  const string SnapshotService::m_name = "SnapshotService";
  
  /* maybe we need this later on
  SnapshotService_Config::SnapshotService_Config(int sPort=9966, const string& user_info_file="asx_user_info") :
    port(sPort),
	user_info_file_name(user_info_file),
    alert_frequency(seconds(5))
  {
  }
*/
  void
  SnapshotService::init()
  {
	  //Logger_Manager lm; 
    //m_logger = lm()->get_logger("Snapshot_Service");

    m_handler->init();

    //m_app->hub()->subscribe(Event_Type::AdminRequestSubscribe, this);

    //m_handler->subscribe_for_path("/admin", this);
  }
  
  void
  SnapshotService::start()
  {
    m_logger->log_printf(Log::LOG, "SnapshotService started on port %d", m_config.port);
    m_acceptor->start(m_config.port);
    m_disp->start();
  }

  void
  SnapshotService::stop()
  {
    m_disp->stop();
  }
  
  boost::intrusive_ptr<Snapshot_Session>
  SnapshotService::create_snapshot_session()
  {
    boost::intrusive_ptr<Snapshot_Session> s(new Snapshot_Session(m_disp.get(), m_handler.get()));
    return s;
  }
  
  /*
  void
  SnapshotService::set_identifier(const string& id)
  {
    m_handler->set_identifier(id);
  }
  */
  
  SnapshotService::SnapshotService(Application* app, 
		  const SnapshotService_Config& config, 
		  const Parameter& params) :
    m_config(config),
	m_disp(new LLIO::EPoll_Dispatcher("Snapshot Service", params)),
	m_handler(new ASX_Test_Handler(app))
  {
    //m_disp.reset(new LLIO::EPoll_Dispatcher("Snapshot Service", params)),
    //m_handler.reset(new ASX_Test_Handler(&m_app));
    m_handler->set_realm(config.realm);
    m_handler->load_user_info(config.user_info_file_name);
    m_handler->set_alert_frequency(config.alert_frequency);
    
    m_acceptor.reset(new LLIO::EPoll_TCP_Acceptor("Snapshot Service Acceptor", m_disp.get(), boost::bind(&SnapshotService::create_snapshot_session, this)));
    
  }
  
  SnapshotService::SnapshotService(Application* app, const Parameter& params) :
          m_config(0) {SnapshotService(app, m_config, params);}
  
  SnapshotService::SnapshotService():
          //SnapshotService(&m_app, m_config, m_param),
	  m_app(),
	  m_config(0), 
          m_param(){
	m_disp.reset(new LLIO::EPoll_Dispatcher("Snapshot Service", m_param));
	//m_handler(new ASX_Test_Handler(app)),
      m_handler.reset(new ASX_Test_Handler(&m_app));
      m_handler->set_realm(m_config.realm);
      m_handler->load_user_info(m_config.user_info_file_name);
      m_handler->set_alert_frequency(m_config.alert_frequency);

      m_acceptor.reset(new LLIO::EPoll_TCP_Acceptor("Snapshot Service Acceptor", m_disp.get(), boost::bind(&SnapshotService::create_snapshot_session, this)));
  }
  
  
  SnapshotService::~SnapshotService()
  {
    
  }

}
