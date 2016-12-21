#ifndef kou_md_QBT_ASX_ITCH_Session_h
#define kou_md_QBT_ASX_ITCH_Session_h

#include <Data/Handler.h>
#include <Data/ASX_ITCH_1_1_Handler.h>
#include <Data/MDBufferQueue.h>

#include <MD/MDProvider.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Interface/AlertEvent.h>

#include <Logger/Logger.h>
#include <Util/Parameter.h>
#include <Util/Time.h>

namespace kou_core {
  class MDDispatcher;
}

namespace kou_connection {
  using namespace std;
  using namespace kou_tools;
  using namespace kou_core;

  class ASX_ITCH_Session : public kou_core::MDProvider, public Event_Acceptor {
  public:
    
    // MDProvider functions
    virtual void init(const string&, const Parameter&);
    virtual void start(void);
    virtual void stop(void);
    virtual void subscribe_product(const ProductID& id_,const ExchangeID& exch_,
				   const string& mdSymbol_,const string& mdExch_);
    virtual bool drop_packets(int num_packets);
    virtual Handler* handler() { return m_handler.get(); }
    
    void set_log_skips_admin(AdminRequest& request_, bool enable);
    void set_log_skips(bool yes){};
    void log_skips_admin(AdminRequest& request_);

    virtual void accept_admin_request(AdminRequest& request_);
    const string& name() const { return m_name; }

    ASX_ITCH_Session(MDManager& md);
    virtual ~ASX_ITCH_Session();
    
  private:   
 
    bool init_mcast(const string& lines, const Parameter& params);
    bool init_snapshot_channel(const string& line_name, const Parameter& params);
    bool init_recovery_channel(const string& line_name, const Parameter& params);
    
    void establish_refresh_connection();
    void establish_retransmission_connection();
    void destroy_refresh_connections();
    void destroy_retransmission_connections();
    
    string m_name;    

    hash_map<string, size_t> m_channel_name_map;
    hash_map<char, size_t> m_char_to_channel_info_map;
    
    boost::scoped_ptr<ASX_ITCH_1_1_Handler> m_handler;
    
    MDDispatcher* m_disp;
    MDDispatcher* m_snapshot_disp;
    MDDispatcher* m_retransmission_disp;
    sockaddr_in  m_snapshot_addr;
    sockaddr_in  m_retransmission_addr;
	void	init_auxiliary(const Parameter&, size_t& curr_context,  Handler::channel_info_map_t& cim);
    size_t m_refresh_tcp_context;
    size_t m_retransmission_context;

    Admin_Request_Handler m_admin_handler;
  };
  
}

#endif /* ifndef kou_md_QBT_ASX_ITCH_Session_h */
