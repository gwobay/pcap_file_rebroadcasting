#ifndef hf_cwtest_CWTEST_Snapshot_Handler_H
#define hf_cwtest_CWTEST_Snapshot_Handler_H

#include <LLIO/EPoll_Dispatcher.h>
#include <Logger/Logger.h>
#include <Util/Parameter.h>
#include <Util/Time.h>
#include <ITCH/Snapshot_Session.h>
#include <ITCH/Simple_Dispatcher.h>
#include <Data/Handler.h>
#include <vector>
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/function.hpp>
#include <Util/enum.h>
#include <app/app.h>
#include <Logger/Logger.h>
#include <Util/tbb_hash_util.h>
#include <tbb/concurrent_hash_map.h>
#include <map>


namespace hf_cwtest {
         using namespace hf_core;
         using namespace hf_tools;

	 class Snapshot_Session;

  namespace itch_session_packet {
    
    struct message_block_t {
      uint16_t message_length;
    } __attribute__ ((packed));
    
    struct downstream_header_t {
      char      session[10];
      uint64_t  seq_num;
      uint16_t  message_count;
    } __attribute__ ((packed));
    
    struct timestamp_message {
      char     message_type;
      uint32_t seconds;
    }  __attribute__ ((packed));

    struct order_book_directory {
      char     message_type;
      uint32_t nanoseconds;
      uint32_t order_book_id;
      char     symbol[32];
      char     long_name[32];
      char     isin[12];
      uint8_t  financial_product;
      char     currency[3];
      uint16_t num_decimals_in_price;
      uint16_t num_decimals_in_nominal;
      uint32_t odd_lot_size;
      uint32_t round_lot_size;
      uint32_t block_lot_size;
      uint64_t nominal_value;
    }  __attribute__ ((packed));

    struct combination_order_book_directory {
      char     message_type;
      uint32_t nanoseconds;
      uint32_t order_book_id;
      char     symbol[32];
      char     long_name[32];
      char     isin[12];
      uint8_t  financial_product;
      char     currency[3];
      uint16_t num_decimals_in_price;
      uint16_t num_decimals_in_nominal;
      uint32_t odd_lot_size;
      uint32_t round_lot_size;
      uint32_t block_lot_size;
      uint64_t nominal_value;
      char     leg_1_symbol[32];
      char     leg_1_side;
      uint32_t leg_1_ratio;
      char     leg_2_symbol[32];
      char     leg_2_side;
      uint32_t leg_2_ratio;
      char     leg_3_symbol[32];
      char     leg_3_side;
      uint32_t leg_3_ratio;
      char     leg_4_symbol[32];
      char     leg_4_side;
      uint32_t leg_4_ratio;
    }  __attribute__ ((packed));

    struct tick_size_message {
      char     message_type;
      uint32_t nanoseconds;
      uint32_t order_book_id;
      uint64_t tick_size;
      int32_t  price_from;
      int32_t  price_to;
    }  __attribute__ ((packed));

    struct system_event_message {
      char     message_type;//'S'
      uint32_t nanoseconds;
      char     event_code; //'O', 'C'
    } __attribute__ ((packed));

    struct order_book_state_message {
      char     message_type;
      uint32_t nanoseconds;
      uint32_t order_book_id;
      char     state_name[20];
    } __attribute__ ((packed));

    struct add_order_no_mpid {
      char     message_type; //'A'
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t order_book_id;
      char     side;
      uint32_t order_book_posn;
      uint64_t quantity;
      int32_t  price;
      uint16_t exchange_order_type;
      uint8_t  lot_type;
    } __attribute__ ((packed));

    struct add_order_mpid {
      char     message_type; //'F'
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t order_book_id;
      char     side;
      uint32_t order_book_posn;
      uint64_t quantity;
      int32_t  price;
      uint16_t exchange_order_type;
      uint8_t  lot_type;
      char     participant_id[7];
    } __attribute__ ((packed));

    struct order_executed {
      char     message_type;//'E'
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t order_book_id;
      char     side;
      uint64_t executed_quantity;
      uint64_t match_id_part_1;
      uint32_t match_id_part_2; //total 12 digits for match id
      char     participant_id_owner[7];
      char     participant_id_counterparty[7];
    } __attribute__ ((packed));

    struct order_executed_with_price {
      char     message_type; //'C'
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t order_book_id;
      char     side;
      uint64_t executed_quantity;
      uint64_t match_id_part_1;
      uint32_t match_id_part_2;
      char     participant_id_owner[7];
      char     participant_id_counterparty[7];
      uint32_t execution_price; //spec : trade price
      char     cross_trade;
      char     printable;
    } __attribute__ ((packed));

    struct order_delete_message {
      char     message_type; //'D'
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t order_book_id;
      char     side;
    } __attribute__ ((packed));

    struct order_replace_message {
      char     message_type; //'U'
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t order_book_id;
      char     side;
      uint32_t new_order_book_posn;
      uint64_t quantity;
      int32_t  price;
      uint16_t exchange_order_type;
    } __attribute__ ((packed));

    struct trade_message {
      char     message_type; //'P'
      uint32_t nanoseconds;
      uint64_t match_id_part_1;
      uint32_t match_id_part_2;
      char     side;
      uint64_t quantity;
      uint32_t order_book_id;
      int32_t  trade_price;
      char     participant_id_owner[7];
      char     participant_id_counterparty[7];
      char     printable;
      char     cross_trade;
    } __attribute__ ((packed));

    //Auction Messages
    struct equilibrium_price_message {
      char     message_type;
      uint32_t nanoseconds;
      uint32_t order_book_id;
      uint64_t bid_quantity;
      uint64_t ask_quantity;
      int32_t  equilibrium_price;
      int32_t  best_bid_price;
      int32_t  best_ask_price;
      uint64_t best_bid_quantity;
      uint64_t best_ask_quantity;
    } __attribute__ ((packed));

    //RPI message is not used in ASX
    struct retail_price_improvement {
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     interest_flag;
    } __attribute__ ((packed));

    //SoupBinTcp
    struct SoupBinTcpLogin {
      uint16_t packet_length;
      char     packet_type; //'L'
      char     username[6];
      char     password[10];
      char     session[10]; //empty
      char     seqno[20]; //left aligned space filled
    } __attribute__ ((packed));

    struct SoupBinTcpAccept {
      uint16_t packet_length;
      char     packet_type; //'A'
      char     session[10]; //put in m_current_session
      char     seqno[20]; //left padded space filled
    } __attribute__ ((packed));

    struct SoupBinTcpLogOut {
      uint16_t packet_length;
      char     packet_type; //'O' 
    } __attribute__ ((packed));

    struct SoupBinTcpBeat {
      uint16_t packet_length;
      char     packet_type; //'H' from server; 'R' from client
    } __attribute__ ((packed));

    struct SoupBinTcpSessionEnd {
      uint16_t packet_length;
      char     packet_type; //'Z' 
    } __attribute__ ((packed));

    struct SoupBinTcpDebug {
      uint16_t packet_length;
      char     packet_type; //'+' 
      char*    text;
    } __attribute__ ((packed));

    struct SoupBinTcpData {
      uint16_t packet_length;
      char     packet_type; //'S' 
      char*    message;
    } __attribute__ ((packed));

    struct SoupBinTcpEndSnapshot {
      uint16_t packet_length;
      char     packet_type; //'S' 
      char     message_type; //'G' 
      char     itch_seqno[20];
    } __attribute__ ((packed));

    struct SoupBinTcpReject {
      uint16_t packet_length;
      char     packet_type; //'J' 
      char     reject_code; //A or S
    } __attribute__ ((packed));

  }

  namespace snapshot_session {

  enum channel_state_types{
		  listening,
		  connected,
		  got_logon,
		  accepted,
		  sending_msg,
		  end_snapshot,
		  disconnected,
		  reset};

  struct channel_info{
	  string username;
	  size_t channel_id;
	  vector<boost::shared_array<char> > m_outbound_msg_list;
	  vector<size_t> pack_seqno_list; //from input file, msgs were bundled and same bundled messages will be sent out in the same release
	  vector<size_t> dropped_pack_seqno_list; //from input file, msgs were bundled and same bundled messages will be sent out in the same release
	  vector<boost::shared_array<char> > m_inbound_msg_list;
	  size_t client_first_seqno;
	  int port;
	  int fd;
	  int mcast_fd;
	  int udp_send_fd;
	  int udp_read_fd;
	  UDP_READER udp_writer;
//for broadcasting
		size_t next_mcast_packet;
	  Time next_mcast_time;
//for recovery
	  int tcp_send_fd;
	  int tcp_read_fd;
	  queue<int> recovery_packet_seqno_list;
	  char one_pack_buffer[56*1000];
	  	size_t next_recovery_packet;
	  	size_t m_next_drop_seqno;
		int recovery_end_seqno;
		Time recovery_next_send_time;
//for snapshot
	  socket_fd snapshot_skt;
	  	size_t next_out_seqno;
	  	Time snapshot_next_send_time;
	  	int snapshot_last_seqno;
	  size_t next_packet_seqno;
	  size_t last_out_seqno;
	  size_t last_in_seqno;
	  size_t snapshot_seqno_1;
	  Time_Duration heartbeat_interval;
	  Time last_sent_time;
	  Time last_recv_time;
	bool final_message_completed;
          struct channel_state{
		  size_t listening:1;
		  size_t connected:1;
		  size_t got_logon:1;
		  size_t accepted:1;
		  size_t sending_msg:1;
		  size_t end_snapshot:1;
		  size_t disconnected:1;
		  channel_state():
		  	listening(1),
		  	connected(0),
		  	got_logon(0),
		  	accepted(0),
		  	sending_msg(0),
		  end_snapshot(0),
		  	disconnected(0) {}
	  };
	  struct channel_state m_channel_state;
  void update_channel_state(channel_state_types st){
	  switch(st){
		  case channel_state_types::listening: 
			  m_channel_state.listening=1; break;
		  case channel_state_types::connected: 
			  m_channel_state.connected=1; break;
		  case channel_state_types::sending_msg: 
			  m_channel_state.sending_msg=1; break;
		  case channel_state_types::accepted: 
			  m_channel_state.accepted=1; break;
		  case channel_state_types::got_logon: 
			  m_channel_state.got_logon=1; break;
		  case channel_state_types::end_snapshot: 
			  m_channel_state.end_snapshot=1; break;
		  case channel_state_types::disconnected: 
			  m_channel_state.disconnected=1; break;
		  default: break;
	  }
  }
  }; //end of channel_info

  }

  class Snapshot_Handler : public Handler {
  public:
       virtual const string& name(){ return class_name;}
       virtual size_t parse(size_t context, const char* buf, size_t len){ return 0;}
      virtual size_t parse2(size_t context, const char* buf, size_t len){ return 0;}
      virtual size_t read(size_t context, const char* buf, size_t len) {return 0;}
      virtual void to_write(int fd){};
      virtual void reset(const char* msg){}
       virtual void subscribe_product(ProductID id, ExchangeID exch, const string& mdSymbol,const string& mdExch){}

    virtual void fill_message_list(size_t context, const char* buf, size_t len){}
    virtual void confirm_login(snapshot_session::channel_info& wk_channel, const char* buf){}
    virtual void send_data(snapshot_session::channel_info& wk_channel, size_t last_snapshot_seqno){}
    virtual void send_accept(snapshot_session::channel_info& wk_channel){}
    virtual void send_reject(snapshot_session::channel_info& wk_channel){}
    virtual void send_debug(snapshot_session::channel_info& wk_channel){}
    virtual void send_heartbeat(snapshot_session::channel_info& wk_channel){}
    virtual void send_end_snapshot(snapshot_session::channel_info& wk_channel){}
virtual size_t process_client_data(int fd, const char* data, size_t l){return 0;}

    virtual void disconnect(snapshot_session::channel_info& wk_channel){}
    virtual bool disconnect(int context){return true;}
    virtual void set_up_channels(){}
    virtual void update_channel_state(){}

    virtual void init(const string&, const Parameter&){}
    virtual void init(){}

    virtual bool is_done(){return false;}
    virtual void start(){}
    virtual void stop(){}

    virtual void check_time_out(){}
    void set_realm(const string& realm) { m_realm = realm; }
    void set_alert_frequency(Time_Duration f) { m_alert_frequency = f; }
    virtual void load_user_info(const string& filename){}

    Snapshot_Handler(Application* app);
    Snapshot_Handler(Application* app, const string& handler_type);
    virtual ~Snapshot_Handler(){}
    
    vector<boost::shared_ptr<snapshot_session::channel_info> > m_channels;
    virtual void sync_udp_channel(string name, size_t context, int  recovery_fd){}
    virtual void sync_tcp_channel(string name, size_t context, int  snapshot_fd){}
  private:
    string class_name;
  protected:
    
    itch_session_packet::SoupBinTcpLogin m_soup_Login;
    itch_session_packet::SoupBinTcpAccept m_soup_Accept;
    itch_session_packet::SoupBinTcpReject m_soup_Reject;
    itch_session_packet::SoupBinTcpData m_soup_Data;
    itch_session_packet::SoupBinTcpLogOut m_soup_LogOut;
    itch_session_packet::SoupBinTcpDebug m_soup_Debug;
    itch_session_packet::SoupBinTcpBeat m_soup_Heartbeat;

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

    boost::shared_ptr<Snapshot_Session> m_session;
    
    LoggerRP m_logger;
  };

  typedef boost::intrusive_ptr<Snapshot_Handler> Snapshot_HandlerRP;
  typedef boost::shared_ptr<snapshot_session::channel_info> channel_infoRP;

}

#endif /* ifndef hf_cwtest_CWTEST_Snapshot_Handler_H */
