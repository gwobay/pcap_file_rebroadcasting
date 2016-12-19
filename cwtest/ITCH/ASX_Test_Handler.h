#ifndef hf_cwtest_ITCHEST_ASX_Test_Handler_H
#define hf_cwtest_ITCHEST_ASX_Test_Handler_H

#include <ITCH/Snapshot_Handler.h>
#include <Data/Handler.h>
#include <vector>
#include <set>
#include <boost/function.hpp>
#include <app/app.h>
#include <Logger/Logger.h>
#include <Util/tbb_hash_util.h>
#include <tbb/concurrent_hash_map.h>
#include <map>


namespace hf_cwtest {

	using namespace hf_tools;
	using namespace hf_core;


  class ASX_Test_Handler : public Snapshot_Handler {
  public:

      virtual const string& name() const {return my_name;};
      virtual size_t parse(size_t context, const char* buf, size_t len);
      virtual size_t parse2(size_t context, const char* buf, size_t len) {return 0;}; 
      virtual void reset(const char* msg) {};
      virtual void reset(size_t context, const char* msg) { reset(msg); }
      virtual void reset(size_t context, const char* msg, uint64_t expected_seq_no, uint64_t received_seq_no){};

      virtual void subscribe_product(ProductID id, ExchangeID exch, const string& mdSymbol,const string& mdExch) {};

    virtual void fill_message_list(size_t context, const char* buf, size_t len);
    virtual void confirm_login(snapshot_session::channel_info& wk_channel, const char* buf);
	virtual void send_data(snapshot_session::channel_info& wk_channel, size_t   last_snapshot_seqno);
	virtual void send_accept(snapshot_session::channel_info& wk_channel);
	virtual void send_reject(snapshot_session::channel_info& wk_channel);
	virtual void send_debug(snapshot_session::channel_info& wk_channel);
	virtual void send_heartbeat(snapshot_session::channel_info& wk_channel);
	virtual void send_end_snapshot(snapshot_session::channel_info& wk_channel);
    virtual void check_time_out();
    virtual size_t process_client_data(int fd, const char* data, size_t l);
    virtual size_t read(size_t channel_id, const char* data, size_t len);
    virtual void to_write(int fd);
    virtual void disconnect(snapshot_session::channel_info& wk_channel);
    virtual bool disconnect(int context);
    virtual void set_up_channels(){};
    virtual void sync_udp_channel(string name, size_t context, int  id);
    virtual void sync_tcp_channel(string name, size_t context, int  id);
    virtual void load_user_info(const string& filename);
    virtual void init(const string&, const Parameter&);
    virtual void init(const string& data_file_name); //load
    virtual void init();
    virtual void add_user(string username, string password);
    virtual void load_test_data(const char* pMM, size_t len); 
    virtual void load_test_data(const string& data_file_name); 
    static void dump_message(const char* block);

    void enable_test_mode(){m_test_mode=true;}
    void set_udp_writer(size_t channel, UDP_READER aWriter);
    void inline update_channel_seqno_time(snapshot_session::channel_info& channel, size_t last_seqno);
    void verify_outbound_packet(const char* buf);
    void send_next_packet(snapshot_session::channel_info& wch);
    void check_write_event();
    void set_packet_size(size_t k){m_packet_size=k;}
    void start_multicast();
     void start_snapshot_session(snapshot_session::channel_info& wch);
    virtual bool is_done();
    virtual void start();
    virtual void stop();

    //void set_realm(const string& realm) { m_realm = realm; }
    //void set_alert_frequency(Time_Duration f) { m_alert_frequency = f; }
    
    ASX_Test_Handler(Application* app);
    ASX_Test_Handler(Application* app, const string& handler_type);
    virtual ~ASX_Test_Handler();

  private:
    string my_name;
    string m_test_session;
    bool m_test_mode;
    bool m_verify_message;

    size_t m_broadcast_packet_max;
    size_t m_drop_packet_frequency;
    size_t m_next_drop_seqno;
    size_t m_mcast_interval;
    size_t m_recovery_interval;
    size_t m_first_seqno;
    size_t m_tcp_interval;
    size_t m_packet_size;
    snapshot_session::channel_info* get_channel_info(int fd);
    snapshot_session::channel_info* get_channel_by_id(int id);
    void send_snapshot_message(snapshot_session::channel_info& wk_channel);
    size_t create_packet(snapshot_session::channel_info& wk_channel, size_t b, size_t& count);
     size_t send_to_udp_fd(int fd, const char* buffer, size_t len, int fg, struct sockaddr* addr, size_t addr_size);
    void send_recovery_packet(snapshot_session::channel_info& wk_channel);
    void create_recovery_list(snapshot_session::channel_info& wk_channel);
    map<string, string>   m_user_info;
    char packet1[3000];
    size_t packet1_len;
    /*
    vector<boost::shared_ptr<snapshot_server::channel_info> > m_channels;
    
    asx_test::SoupBinTcpLogin m_soup_Login;
    asx_test::SoupBinTcpAccept m_soup_Accept;
    asx_test::SoupBinTcpReject m_soup_Reject;
    asx_test::SoupBinTcpData m_soup_Data;
    asx_test::SoupBinTcpLogout m_soup_Logout;

    struct User {
      string password;
      string username;
    };
    
    Application* m_app;
    
    map<string, User>   m_user_info;
        
    string m_realm;
    string    m_user_info_file;
    
    Time m_last_alert_time;
    Time_Duration m_alert_frequency;

    Snapshot_SessionRP m_session;
    

    LoggerRP m_logger;
    */
  };

  typedef boost::intrusive_ptr<ASX_Test_Handler> ASX_Test_HandlerRP;

}

#endif /* ifndef hf_core_CWTEST_ASX_Test_Handler_H */
