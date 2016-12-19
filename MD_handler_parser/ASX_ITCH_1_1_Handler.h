#ifndef hf_core_Data_ASX_ITCH_1_1_Handler_h
#define hf_core_Data_ASX_ITCH_1_1_Handler_h

#include <Data/Handler.h>
#include <Data/MoldUDP64.h>
#include <Data/QBDF_Builder.h>
#include <MD/MDOrderBook.h>
#include <MD/MDSubscriptionAcceptor.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Logger/Logger.h>
#include <Util/hash_util.h>
#include <Util/Network_Utils.h>
#include <Util/Time.h>
#include <Util/Symbol_Lookup.h>

#include <boost/interprocess/containers/set.hpp>
#include <boost/scoped_array.hpp>
#include <boost/functional/hash.hpp>

#include <zlib.h>

namespace hf_core {

  namespace asx_itch_1_1 {
    
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

    struct SoupBinTcpReject {
      uint16_t packet_length;
      char     packet_type; //'J' 
      char     reject_code; //A or S
    } __attribute__ ((packed));
  }
  
  struct asx_order_id_hash_key {
    uint64_t asx_order_id;
    uint32_t asx_order_book_id;
    char     side;
    
    asx_order_id_hash_key(uint64_t oid, uint32_t obid, char s)
      : asx_order_id(oid), asx_order_book_id(obid), side(s) {}
    
    bool operator==( const asx_order_id_hash_key& x ) const
    {
      if (asx_order_id == x.asx_order_id && asx_order_book_id == x.asx_order_book_id && side == x.side) {
	return true;
      }
      return false;
    }
  };
  
  struct asx_match_id {
    uint64_t part_1;
    uint32_t part_2;
    
    asx_match_id(uint64_t p1, uint32_t p2) : part_1(p1), part_2(p2)
    {}
    
    asx_match_id() : part_1(-1), part_2(-1)
    {}
    
    string to_string()
    {
      ostringstream oss;
      oss << part_1 << part_2;
      return oss.str();
    }
    
    bool operator<(const asx_match_id& x) const
    {
      if (part_1 == x.part_1) {
	return part_2 < x.part_2;
      } else {
	return part_1 < x.part_1;
      }
    }
    
    bool operator==(const asx_match_id& x) const
    {
      return (part_1 == x.part_1 && part_2 == x.part_2);
    }
  };
  /*
    bool operator<(const asx_match_id& l, const asx_match_id& r)
    {
      if (l.part_1 == r.part_1) {
	return l.part_2 < r.part_2;
      } else {
	return l.part_1 < r.part_1;
      }
    }

    bool operator==(const asx_match_id& l, const asx_match_id& r)
    {
      return (l.part_1 == r.part_1 && l.part_2 == r.part_2);
    }
  */
}

namespace __gnu_cxx
{
  template<> struct hash< hf_core::asx_order_id_hash_key >
  {
    std::size_t operator()( const hf_core::asx_order_id_hash_key& x ) const
    {
      std::size_t seed = 0;
      boost::hash_combine(seed, x.asx_order_id);
      boost::hash_combine(seed, x.asx_order_book_id);
      boost::hash_combine(seed, x.side);

      return seed;
    }
  };
};

namespace hf_core {
  using namespace hf_tools;

  typedef hash_map<asx_order_id_hash_key,uint64_t> asx_order_id_hash_t;

  class MDProvider;

  class ASX_ITCH_1_1_Handler : public Handler {
  public:

    struct symbol_info {
      ProductID product_id;
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
      uint32_t price_multiplier;
      symbol_info() :
      product_id(Product::invalid_product_id)
      { }
    };
    
    inline symbol_info* get_symbol_info(uint32_t order_book_id) {
      symbol_info_map_iter_t sim_iter = m_symbol_info_map.find(order_book_id);
      if (sim_iter != m_symbol_info_map.end())
	return &sim_iter->second;
      
      m_symbol_info_map[order_book_id] = symbol_info();
      return &m_symbol_info_map[order_book_id];
    }

    socket_fd m_snapshot_socket; 
    sockaddr_in m_snapshot_addr; 
    socket_fd m_retransmission_socket; 
    sockaddr_in m_retransmission_addr;
    //virtual void init(const string& name, const vector<ExchangeID>& exchanges, const Parameter& params);  //commented out for only single exchange //from Euronext_XDP
    vector<channel_info> m_retransmission_channels;
    vector<channel_info> m_snapshot_channels;
    virtual bool sets_up_own_contexts() { return true;  }
    void set_up_channels(const Parameter& params);
    //void get_channel_info(string name, size_t context);
    virtual void sync_udp_channel(string name, size_t context, int  recovery_socket);
    virtual void sync_tcp_channel(string name, size_t context, int  snapshot_socket);
    void set_num_contexts(size_t nc);
    void add_context_product_id(size_t context, ProductID id);
    void set_channel_ID(size_t context, uint16_t channel_ID, const string& channel_type_str);

	//void SetTcpHeartbeatResponseServiceId(uint16_t channel_ID);

	void SetTcpContexts(size_t refresh_tcp_context, size_t retransmission_tcp_context) ;

    // Handler functions
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len);
    void process_queued_messages(size_t last_msg_seq_num, size_t context);
    virtual size_t parse2(size_t, const char* buf, size_t len);
    virtual size_t parse_message(size_t, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t);// { return 0; }
    virtual void execute_tcp_data(size_t, const char*, size_t);
    virtual size_t parse_tcp_data(size_t, const char*, size_t);
    //virtual size_t process_tcp_data(size_t, const char*, size_t);
    //virtual void reset_tcp_channel();
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg);
    virtual ProductID product_id_of_symbol_index(uint32_t symbol_index);
    
    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);
    static size_t dump_message(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);

    static size_t dump_refdata_headers(ILogger& log);
    static size_t dump_refdata(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len);

    virtual void update_channel_state();
    virtual void flush_all_loggers();
    virtual void to_write(int fd){}
    void init(const string&, ExchangeID exch, const Parameter&);
    static void set_Glogger(LoggerRP& alg);
    virtual void start();
    virtual bool is_done(){return Handler::is_done();}
    virtual void stop();
    
    void send_retransmit_request(channel_info& channel_info);
    //bool send_retransmission_request(uint16_t channel_ID, uint32_t begin_seq_no, uint32_t end_seq_no);
    void establish_refresh_connection(size_t channel_id);
    bool establish_refresh_connection(channel_info* wk_channel);
    void establish_retransmission_connection(size_t channel_id);
    //void send_retransmission_heartbeat_response(uint16_t channel_ID);
    virtual bool send_snapshot_request(size_t channel_id);
    //void send_snapshot_heartbeat_response(uint16_t channel_ID);
    void handle_retran_msg(channel_info& channel_info, const char* buf, size_t len);
    virtual void enable_hist_mode() { m_hist_mode = true; }
    
    static size_t dump_sym_dir_data_header(ILogger& log);
    static size_t dump_sym_dir_data(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len);

    virtual void enable_test_mode() { m_test_mode = true; }
    
    size_t get_itch_packet_length(const char* buffer, size_t len, size_t& cc);
    
    void subscribe_product(ProductID id_,ExchangeID exch_,
                           const string& mdSymbol_,const string& mdExch_);

    void send_quote_update(MDOrderBookHandler* order_book);
    void send_quote_update(); // fill out time & id and this does the rest
    void send_trade_update(); // fill out time, id, shares and price

    ASX_ITCH_1_1_Handler(Application* app);
    virtual ~ASX_ITCH_1_1_Handler();

  private:
    typedef tbb::spin_mutex mutex_t;
    ProductID get_product_id(uint32_t order_book_id, const string& symbol);
    uint64_t get_unique_order_id(uint64_t asx_order_id, uint32_t order_book_id, char side);

    //typedef vector<symbol_info> symbol_info_map_t;
    typedef map<uint32_t,symbol_info> symbol_info_map_t;
    typedef map<uint32_t,symbol_info>::iterator symbol_info_map_iter_t;

    symbol_info_map_t m_symbol_info_map;

    string m_name;
    static LoggerRP G_logger;
    void print_message(const char* buf);

    mutable mutex_t m_mutex;
    Symbol_Lookup8  m_prod_map;

    Time m_midnight;
    Time m_timestamp;
    Time m_new_auction_date;
    uint64_t m_micros_exch_time_marker;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;
    QBDFStatusType m_qbdf_status;
    string m_qbdf_status_reason;
    char m_exch_char;

    vector<MarketStatus> m_security_status;

    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;

    MarketStatus m_market_status;
    
    boost::shared_ptr<MDOrderBookHandler>     m_order_book;
    
    bool m_test_mode;
    bool m_hist_mode;

    asx_order_id_hash_t m_order_id_hash;
    uint64_t m_order_id;
    vector<set<asx_match_id> > m_match_id_lookup;

    //for connection FROM Euronext_XDP
    vector<ExchangeID>       m_supported_exchanges;
    vector<char>             m_qbdf_exchange_chars;
    
    uint64_t m_micros_midnight;
    Time_Duration m_heartbeat_interval;

    int m_num_reference_groups_remaining;
    bool m_sent_reference_refresh_requests;

    struct Queued_Message_Header
    {
      size_t len;
      size_t context;
    };

    struct MessageQueue
    {
      size_t m_offset;
      char m_buffer[30000000];

      MessageQueue()
        : m_offset(0)
      {
      }

      void Enqueue(size_t len, size_t context, const char* buf)
      {
        Queued_Message_Header* queued_msg_header = reinterpret_cast<Queued_Message_Header*>(&(m_buffer[m_offset]));
        queued_msg_header->len = len;
        queued_msg_header->context = context;
        m_offset += sizeof(Queued_Message_Header);
        memcpy(&(m_buffer[m_offset]), buf, len);
        m_offset += len;
      }

      bool WouldOverflow(size_t len)
      {
        return m_offset + sizeof(Queued_Message_Header) + len >= sizeof(m_buffer);
      }

      bool HasMore(size_t cur_queue_offset) 
      {
        return cur_queue_offset < m_offset;
      }

      const char* GetPayloadAt(size_t& cur_queue_offset, size_t& cur_len, size_t& cur_context)
      {
        Queued_Message_Header* queued_msg_header = reinterpret_cast<Queued_Message_Header*>(&(m_buffer[cur_queue_offset]));
        cur_len = queued_msg_header->len;
        cur_context = queued_msg_header->context;
        cur_queue_offset += sizeof(Queued_Message_Header);
        const char* ret = &(m_buffer[cur_queue_offset]);
        cur_queue_offset += cur_len;
        return ret;
      }
 
      void Reset()
      {
        m_offset = 0;
      } 
    };


    struct Channel_State
    {
      uint16_t m_channel_ID;
      bool m_has_sent_refresh_request;
      bool m_in_refresh_state;
      bool m_refresh_rejected; //true indicate don't try again
      size_t m_refresh_start_seqno;
      bool m_received_refresh_start;
      bool m_in_retransmit_state;
      uint32_t m_highest_retrans_seq_num_needed;
      int m_num_refresh_tries_remaining;
      size_t m_incremental_context;
      size_t m_recovery_seqno;
      size_t m_recovery_end_seqno;
      //when created the m_in_refresh_state is true
      //but the m_snapshot_timeout_time=0
      //
      Time m_snapshot_timeout_time;
      //if m_snapshot_timeout_time != 0 
      //the channel is in m_in_refresh_state
      //if a m_snapshot_timeout_time is time_outed
      //treat as snapshot_reject is received
      //m_refresh_rejected will be set
#ifdef USE_MY_Q
      MessageQueue m_refresh_queue;
      MessageQueue m_retransmission_queue;
#else
      MDBufferQueue m_refresh_queue;
      MDBufferQueue m_retransmission_queue;
#endif

      Channel_State() :
          m_channel_ID(0),
         m_has_sent_refresh_request(false),
	m_refresh_start_seqno(1),
          m_in_refresh_state(true),
          m_received_refresh_start(false),
          m_in_retransmit_state(false),
	  m_refresh_rejected(false),
          m_num_refresh_tries_remaining(2)
      {
      }
    };

  BOOST_ENUM_VALUES(ASX_Channel_Type, int,
                     (INCREMENTAL)(1)
		     (REFRESH)(2)
                     (REFERENCE)(3)
                   )
	  /*
    struct Redundant_This
    {
      Channel_State* service_state;
      ASX_Channel_Type channel_type;

      Redundant_This() { }

      Redundant_This(Channel_State* service_state_, bool is_refresh_, bool is_reference_)
        : service_state(service_state_)
      {
        if(is_refresh_) { channel_type = ASX_Channel_Type::REFRESH; }
        else if(is_reference_) { channel_type = ASX_Channel_Type::REFERENCE; }
        else { channel_type = ASX_Channel_Type::INCREMENTAL; }
      }
    };
    */

    //hash_map<uint16_t, Channel_State> m_channel_ID_to_state;
    //hash_map<size_t, Redundant_This> m_context_to_state;

    struct asx_channel {
	    char session[11];
	    channel_info* m_channel_info;
	    //ILogger m_logger;
	    LoggerRP m_loggerRP;
	    Channel_State m_channel_state;
	    // for test only mode --------
	    int udp_recovery_send_fd;
	    int udp_recovery_read_fd;
	    //------------------------------
	    size_t snapshot_msg_received;
	    Time next_heartbeat_time;
	    //channel_info operator() (){return m_channel_info;}
	    operator channel_info(){return *m_channel_info;}
	    asx_channel(): m_channel_state(){};
    };
    vector<boost::shared_ptr<struct asx_channel> > m_asx_channels;
    asx_channel* get_asx_channel(size_t channel_id);
    Logger *m_current_logger;
    string m_channel_log_base;

    void try_connect_snapshot(asx_channel* asxCh);
    void process_MDQ(asx_channel* );
    void process_snapshot_reject(asx_channel*);
    bool send_retransmit_request(channel_info& ch, int64_t from, uint16_t amt);
    bool send_retransmit_request(asx_channel* asxCh, int64_t from, uint16_t amt);

    z_stream m_z_stream;
    boost::scoped_array<char>  m_decompressed_buf;
    
    //symbol_order_sessions_t m_symbol_order_sessions;
    vector< boost::interprocess::set<uint64_t> > m_orders_to_delete_at_open;
    vector< boost::interprocess::set<uint64_t> > m_orders_to_delete_at_close;
  
    uint16_t m_tcp_heartbeat_response_channel_ID; 
    size_t m_refresh_tcp_context; 
    size_t m_retransmission_tcp_context; 
    bool m_supports_snapshots; 
    string m_symbol_mapping_file;
    vector<vector<ProductID> > m_context_to_product_ids;
    //tcp objects
    //SoupBinTcp
    asx_itch_1_1::SoupBinTcpLogin m_soup_login;
    asx_itch_1_1::SoupBinTcpLogOut m_soup_logout;
    asx_itch_1_1::SoupBinTcpBeat m_soup_heartbeat;
    Time m_last_beat_time;
    Time m_last_tcp_rcv_time;
    char m_tcp_session[10];
    uint16_t m_tcp_count;
    uint16_t m_tcp_expected_total;
    bool m_tcp_is_on;
    bool m_tcp_got_session_end;
    asx_itch_1_1::SoupBinTcpDebug m_soup_debug;

    static channel_info empty_channel;
    channel_info* get_channel_info(size_t context);
    bool send_snapshot_login(size_t channel_id);
    bool send_snapshot_login(asx_channel* asxCh);
    bool send_tcp_logout(size_t channel_id);
    bool send_tcp_heartbeat(asx_channel* asxCh);

    // following lines are added for local inline functions

size_t parseTIME_MESSAGE_T(size_t context, const char* buf, size_t len);
size_t parseEND_OF_BUSINESS_TRADE_DATE_S(size_t context, const char* buf, size_t len);
size_t parseEQUITY_SYMBOL_DIRECTORY_R(size_t context, const char* buf, size_t len);
size_t parseFUTURE_SYMBOL_DIRECTORY_f(size_t context, const char* buf, size_t len);
size_t parseOPTION_SYMBOL_DIRECTORY_h(size_t context, const char* buf, size_t len);
size_t parseCOMBINATION_SYMBOL_DIRECTORY_M(size_t context, const char* buf, size_t len);
size_t parseBUNDLES_SYMBOL_DIRECTORY_m(size_t context, const char* buf, size_t len);
size_t parseTICK_SIZE_TABLE_L(size_t context, const char* buf, size_t len);
size_t parseORDER_BOOK_STATE_O(size_t context, const char* buf, size_t len);
size_t parseORDER_ADDED_A(size_t context, const char* buf, size_t len);
size_t parseORDER_ADDED_WITH_PARTICIPANT_ID_F(size_t context, const char* buf, size_t len);
size_t parseORDER_VOLUME_CANCELLED_X(size_t context, const char* buf, size_t len);
size_t parseORDER_REPLACED_U(size_t context, const char* buf, size_t len);
size_t parseORDER_DELETED_D(size_t context, const char* buf, size_t len);
size_t parseORDER_EXECUTED_E(size_t context, const char* buf, size_t len);
size_t parseAUCTION_ORDER_EXECUTED_C(size_t context, const char* buf, size_t len);
size_t parseCOMBINATION_EXECUTED_e(size_t context, const char* buf, size_t len);
size_t parseIMPLIED_ORDER_ADDED_j(size_t context, const char* buf, size_t len);
size_t parseIMPLIED_ORDER_REPLACED_l(size_t context, const char* buf, size_t len);
size_t parseIMPLIED_ORDER_DELETED_k(size_t context, const char* buf, size_t len);
size_t parseTRADE_EXECUTED_P(size_t context, const char* buf, size_t len);
size_t parseCOMBINATION_TRADE_EXECUTED_p(size_t context, const char* buf, size_t len);
size_t parseTRADE_REPORT_Q(size_t context, const char* buf, size_t len);
size_t parseTRADE_CANCELLATION_B(size_t context, const char* buf, size_t len);
size_t parseEQUILIBRIUM_PRICE_AUCTION_INFO_Z(size_t context, const char* buf, size_t len);
size_t parseOPEN_HIGH_LOW_LAST_TRADE_ADJUSTMENT_t(size_t context, const char* buf, size_t len);
size_t parseMARKET_SETTLEMENT_Y(size_t context, const char* buf, size_t len);
size_t parseTEXT_MESSAGE_x(size_t context, const char* buf, size_t len);
size_t parseREQUEST_FOR_QUOTE_q(size_t context, const char* buf, size_t len);
size_t parseANOMALOUS_ORDER_THRESHOLD_PUBLISH_W(size_t context, const char* buf, size_t len);
size_t parseVOLUME_AND_OPEN_INTEREST_V(size_t context, const char* buf, size_t len);
size_t parseSNAPSHOT_COMPLETE_G(size_t context, const char* buf, size_t len);
  };

}

#endif /* ifndef hf_core_Data_ASX_ITCH_1_1_Handler_h */
