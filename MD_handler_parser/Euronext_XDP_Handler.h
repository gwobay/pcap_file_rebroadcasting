#ifndef hf_md_Data_Euronext_XDP_Handler_h
#define hf_md_Data_Euronext_XDP_Handler_h

#include <Data/Handler.h>
#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>
#include <Interface/AlertEvent.h>
#include <Logger/Logger.h>
#include <MD/MDOrderBook.h>
#include <MD/MDSubscriptionAcceptor.h>
#include <Util/Parameter.h>
#include <Util/Time.h>

#include <boost/interprocess/containers/set.hpp>
#include <boost/scoped_array.hpp>

#include <zlib.h>


namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  using namespace hf_core;

  class MDProvider;

  class Euronext_XDP_Handler : public Handler {
  public:
    /*
    struct order_session {
      uint64_t order_id;
      uint8_t  trade_session;

      order_session(uint64_t oid, uint8_t ts) : order_id(oid), trade_session(ts) {}
    };
    */

    //typedef vector<order_session> order_sessions_t;
    //typedef vector<order_sessions_t> symbol_order_sessions_t;

    typedef vector<uint64_t> order_id_vector_t;

    struct symbol_info {
      ProductID  product_id;
      ExchangeID exch_id;
      int        book_idx;
      char       exchange_code;

      symbol_info() :
        product_id(Product::invalid_product_id), book_idx(-1), exchange_code(' ')
      { }
    };
    
    inline symbol_info* get_symbol_info(uint32_t symbol_index) {
      // used to take "size_t context" as first argument
      //symbol_info_map_t& info_map = m_context_to_security_info_map[context];
      return &m_symbol_info_map[symbol_index];
    }

    virtual bool sets_up_own_contexts() { return true; }
    void set_up_channels(const Parameter& params);

    void set_num_contexts(size_t nc);

    void set_service_id(size_t context, uint16_t service_id, const string& channel_type_str)
    {
      cout << "set_service_id context=" << context << " service_id=" << service_id << "channel_type_str=" << channel_type_str << endl;
      hash_map<uint16_t, Service_State>::iterator it = m_service_id_to_state.find(service_id);
      if(it == m_service_id_to_state.end())
      {
        m_service_id_to_state[service_id] = Service_State();
        it = m_service_id_to_state.find(service_id);
      }
      Service_State& service_state = it->second;
      bool is_refresh = (channel_type_str == "refresh");
      bool is_reference = (channel_type_str == "reference");
      if(!is_refresh && !is_reference)
      {
        service_state.m_incremental_context = context;
      }
      m_context_to_state[context] = Channel_State(&service_state, is_refresh, is_reference);
      service_state.m_service_id = service_id;
      if(is_reference)
      {
        ++m_num_reference_groups_remaining;
      }
      if(m_qbdf_builder)
      {
        service_state.m_num_refresh_tries_remaining = 0;
      }
    }

    // Handler functions
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len);
    void process_queued_messages(size_t last_msg_seq_num, size_t context);
    virtual size_t parse2(size_t context, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t);
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg);
    virtual ProductID product_id_of_symbol_index(uint32_t symbol_index);

    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len);
    static size_t dump_refdata_headers(ILogger& log);
    static size_t dump_refdata(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len);

    virtual void init(const string& name, const vector<ExchangeID>& exchanges, const Parameter& params);
    virtual void start();
    virtual void stop();
    virtual bool send_snapshot_request(uint16_t service_id);
    
    virtual void add_context_product_id(size_t context, ProductID id);
    virtual void subscribe_product(ProductID id_,ExchangeID exch_,
                                   const string& mdSymbol_,const string& mdExch_);

    void send_quote_update(MDOrderBookHandler* order_book);
    void send_trade_update();
    
    Euronext_XDP_Handler(Application* app);
    virtual ~Euronext_XDP_Handler();
    
    // TODO: put ntohl() inside here for reduced code redundancy (calling it everywhere this is called)
    static inline MDPrice get_int_price(uint32_t price, uint8_t price_scale_code)
    {
      // uint32_t price_local = ntohs(price);
      switch(price_scale_code) {
      case 0: price *= 10000; break;
      case 1: price *= 1000; break;
      case 2: price *= 100; break;
      case 3: price *= 10; break;
      case 4: break;
      case 5: price /= 10; break;
      case 6: price /= 100; break;
      case 7: price /= 1000; break;
      case 8: price /= 10000; break;
      case 9: price /= 100000; break;
      case 10: price /= 1000000; break;
      default:
        price = 0;
      }
      return MDPrice::from_iprice32(price);
    }

    bool send_retransmission_request(uint16_t service_id, uint32_t begin_seq_no, uint32_t end_seq_no);
    void send_snapshot_heartbeat_response(uint16_t service_id);
    void send_retransmission_heartbeat_response(uint16_t service_id);
    void handle_retran_msg(channel_info& channel_info, const char* buf, size_t len);
    
    socket_fd m_snapshot_socket;
    sockaddr_in m_snapshot_addr;

    socket_fd m_retransmission_socket;
    sockaddr_in m_retransmission_addr;

    void SetTcpHeartbeatResponseServiceId(uint16_t service_id)
    {
      m_tcp_heartbeat_response_service_id = service_id;
    }
    
    void SetTcpContexts(size_t refresh_tcp_context, size_t retransmission_tcp_context)
    {
      m_refresh_tcp_context = refresh_tcp_context;
      m_retransmission_tcp_context = retransmission_tcp_context;
    }

  protected:
    struct eqstr{
      bool operator()(const char* s1, const char* s2) const {
        return strcmp(s1, s2) == 0;
      }
    };
  
    void read_symbol_mapping_from_file();
 
    void clear_all_books();
 
    typedef tbb::spin_mutex mutex_t;
    typedef hash_map<uint32_t, symbol_info> symbol_info_map_t;

    hash_map<string, ProductID> m_subscription_symbol_to_product_id;

    symbol_info_map_t m_symbol_info_map;

    string m_name;
    string m_source_id;
    int m_snapshot_timeout_seconds;
    bool m_continue_on_refresh_failure;

    vector<vector<ProductID> > m_context_to_product_ids;

    Time_Duration m_max_send_source_difference;
    uint64_t m_source_time_seconds;

    mutable mutex_t m_mutex;

    int  m_lot_size;

    int m_req_seq_num;
    int m_refresh_req_seq_num;

    bool m_use_old_imbalance;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;

    vector<MarketStatus> m_security_status;
    
    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;

    MarketStatus m_market_status;
    
    vector<ExchangeID>       m_supported_exchanges;
    vector<char>             m_qbdf_exchange_chars;
    
    uint64_t m_micros_midnight;

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


    struct Service_State
    {
      bool m_has_sent_refresh_request;
      bool m_in_refresh_state;
      bool m_received_refresh_start;
      bool m_in_retransmit_state;
      uint32_t m_highest_retrans_seq_num_needed;
      uint16_t m_service_id;
      int m_num_refresh_tries_remaining;
      size_t m_incremental_context;
      Time m_snapshot_timeout_time;
      MessageQueue m_refresh_queue;
      MessageQueue m_retransmission_queue;

      Service_State()
        : m_has_sent_refresh_request(false),
          m_in_refresh_state(true),
          m_received_refresh_start(false),
          m_in_retransmit_state(false),
          m_service_id(0),
          m_num_refresh_tries_remaining(2)
      {
      }
    };

  BOOST_ENUM_VALUES(EuronextChannelType, int,
                     (INCREMENTAL)(1)
		     (REFRESH)(2)
                     (REFERENCE)(3)
                   )
    struct Channel_State
    {
      Service_State* service_state;
      EuronextChannelType channel_type;

      Channel_State() { }

      Channel_State(Service_State* service_state_, bool is_refresh_, bool is_reference_)
        : service_state(service_state_)
      {
        if(is_refresh_) { channel_type = EuronextChannelType::REFRESH; }
        else if(is_reference_) { channel_type = EuronextChannelType::REFERENCE; }
        else { channel_type = EuronextChannelType::INCREMENTAL; }
      }
    };

    hash_map<uint16_t, Service_State> m_service_id_to_state;
    hash_map<size_t, Channel_State> m_context_to_state;
 
    z_stream m_z_stream;
    boost::scoped_array<char>  m_decompressed_buf;
    
    //symbol_order_sessions_t m_symbol_order_sessions;
    vector< boost::interprocess::set<uint64_t> > m_orders_to_delete_at_open;
    vector< boost::interprocess::set<uint64_t> > m_orders_to_delete_at_close;
  
    vector<ExchangeID> m_exchanges;
    uint16_t m_tcp_heartbeat_response_service_id;
    size_t m_refresh_tcp_context;
    size_t m_retransmission_tcp_context;
    bool m_supports_snapshots;
    string m_symbol_mapping_file;
  };

  namespace Euronext_XDP_structs {
    struct packet_header {
      uint16_t pkt_size;
      uint16_t packet_type;
      uint32_t seq_num;
      uint32_t send_time;
      uint16_t service_id;
      uint8_t  delivery_flag;
      uint8_t  number_msgs;
    }  __attribute__ ((packed));
    
    struct message_header {
      uint16_t msg_size;
      uint16_t msg_type;
    }  __attribute__ ((packed));

// Control Messages

    // Msg Type 1
    struct sequence_number_reset {
      uint32_t next_seq_number;
    } __attribute__ ((packed));

    struct source_time_reference_message {
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint32_t time_reference;
    } __attribute__ ((packed));

// Client Request Messages
    // Msg Type 12
    struct heartbeat_response_message {
      char     source_id[20];
    } __attribute__ ((packed));

    // Msg Type 10
    struct retransmission_request_message {
      uint32_t begin_seq_num;
      uint32_t end_seq_num;
      char     source_id[20];
    } __attribute__ ((packed));


    struct retransmission_response_message {
      uint32_t source_seq_num;
      char     source_id[20];
      uint8_t  status;
      uint8_t  reject_reason;
      char     filler[2];
    } __attribute__ ((packed));

    struct refresh_request_message {
      char     source_id[20];
    } __attribute__ ((packed));


    struct refresh_response_message {
      uint32_t source_seq_num;
      char     source_id[20];
      uint8_t  status;
      uint8_t  reject_reason;
      char     filler[2];
    } __attribute__ ((packed));
    
    // Msg Type 580
    struct start_refresh_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t last_seq_num;
    } __attribute__ ((packed));


    // Msg Type 581
    struct end_refresh_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t last_seq_num;
    } __attribute__ ((packed));

    
// Response Messages
    
    // Msg Type 505
    struct stock_state_change {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint32_t source_time;
      uint32_t system_id;
      uint16_t source_time_micros;
      char     filler1[2];
      char     start_date_halting[8];
      char     start_time_halting[6];
      char     prog_opening_time[6];
      char     order_entry_rejection;
      char     instrument_state;
      char     instrument_trading_status;
      char     halt_reason;
      char     action_affecting_state;
      char     instrument_state_tcs;
      char     period_side;
      char     filler2;
    } __attribute__ ((packed));

    // Msg Type 516
    struct class_state_change {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t source_seq_num;
      uint32_t source_time;
      uint32_t system_id;
      uint16_t source_time_micros;
      char     instrument_group_code[3];
      char     filler1;
      char     session_type;
      char     filler2;
      char     class_state[4];
    } __attribute__ ((packed));


    // Msg Type 523
    struct mail_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t source_seq_num;
      uint32_t source_time;
      uint32_t system_id;
      uint16_t source_time_micros;
      char     instrument_group_code[3];
      char     filler1;
      char     priority_indicator;
      char     type_of_message;
      char     address_type;
      char     title[80];
      char     text[854];
      char     filler[2];
    } __attribute__ ((packed));
    
    // Msg Type 524
    struct request_for_size_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t source_seq_num;
      uint32_t source_time;
      char     side;
      uint32_t volume;
      uint32_t rfsid;
    } __attribute__ ((packed));

    // Msg Type 550
    struct start_reference_data_message {
      uint16_t msg_size;
      uint16_t msg_type;
      char     indicator;
      char     filler[3];
    } __attribute__ ((packed));

    // Msg Type 551
    struct end_reference_data_message {
      uint16_t msg_size;
      uint16_t msg_type;
      char     indicator;
      char     filler[3];
    } __attribute__ ((packed));

    struct reference_data_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t source_seq_num;
      uint32_t source_time;
      uint32_t last_adj_price;
      uint32_t system_id;
      uint32_t prev_volume_traded;
      uint32_t fix_price_tick;
      uint16_t source_time_micro_secs;
      uint16_t stock_exchange_code;
      uint16_t type_of_instrument;
      char     event_date[8];
      char     instrument_name[18];
      char     period_indicator;
      char     type_of_market_admission;
      char     issuing_country_code[3];
      char     trading_currency[3];
      char     instrument_group_code[3];
      char     instrument_category;
      char     instrument_code[12];
      char     date_of_last_trade[8];
      char     underlying_repo_isin_code[12];
      char     repo_expiry_date[8];
      char     first_settlement_date[8];
      char     type_of_derivates;
      char     bic_depositary[11];
      char     icb[4];
      char     mic[4];
      char     underlying_wisin_code[12];
      char     depositary_list[25];
      char     main_depositary[5];
      char     type_of_corporate_event[2];
      char     time_lag_euronext_utc[5];
      char     time_lag_mifid_reg_utc[5];
      char     cfi[6];
      char     quantity_notiation[4];
      char     index_set_of_var_price_tick[2];
      char     market_feed_code[2];
      char     mic_list[24];
      char     industry_code[4];
      char     filler1[100];
      char     financial_market_code[3];
      char     us_indicators[7];
      char     filler2[2];
      uint64_t prev_day_capital_traded;
      uint64_t nom_mkt_price;
      uint64_t lot_size;
      uint64_t number_instrument_circ;
      uint64_t shares_out;
      uint64_t auth_shares;
      char     filler3[3];
      char     repo_indicator;
      uint8_t  last_adj_price_scale_code;
      uint8_t  type_of_unit_exp;
      uint8_t  market_indicator;
      uint8_t  prev_day_capital_traded_scale_code;
      uint8_t  tax_code;
      uint8_t  nom_mkdt_price_scale_code;
      uint8_t  lot_size_scale_code;
      uint8_t  fix_price_tick_scale_code;
      char     mnemo[5];
      char     trading_code[12];
      char     filler4[3];
      uint32_t strike_price;
      char     strike_currency[3];
      uint8_t  strike_scale_code;
      uint32_t currency_coef;
      uint8_t  currency_coef_scale_code;
      uint8_t  trading_currency_indicator;
      uint8_t  strike_currency_indicator;
      char     filler5[9];
    } __attribute__ ((packed));

    
    // Msg Type 221
    struct trade_cancel_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t source_time;
      uint32_t source_seq_num;
      uint32_t original_trade_id_number;
      uint32_t system_id;
      uint16_t source_time_micro_secs;
      char     filler[2];
    } __attribute__ ((packed));

    // Msg Type 240
    struct trade_full_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t source_time;
      uint32_t trade_id_number;
      uint32_t quote_link_id; // not used
      uint32_t source_seq_num;
      uint32_t price;
      uint32_t volume;
      uint32_t cumulative_quantity;
      uint32_t highest_price;
      uint32_t lowest_price;
      int32_t  variation_last_price;
      uint32_t system_id;
      uint16_t source_time_micro_secs;
      char     filler[2];
      char     trade_condition[4]; // positions 0 and 3 are unused
      char     tick_direction;
      char     opening_trade_indicator;
      uint8_t  variation_scale_code;
      uint8_t  price_scale_code;
    } __attribute__ ((packed));

    // Msg Type 241
    struct price_update_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t source_time;
      uint32_t source_seq_num;
      uint32_t price;
      uint32_t highest_price;
      uint32_t lowest_price;
      uint32_t variation_last_price;
      uint32_t system_id;
      uint16_t source_time_micro_secs;
      char     type_of_price[2];
      char     filler[2];
      uint8_t  price_scale_code;
      uint8_t  variation_scale_code;
    } __attribute__ ((packed));

    // Msg Type 245
    struct auction_summary_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t source_time;
      uint32_t source_seq_num;
      uint32_t first_price;
      uint32_t last_price;
      uint32_t highest_price;
      uint32_t lowest_price;
      uint32_t cumulative_quantity;
      uint32_t variation;
      uint32_t system_id;
      uint16_t source_time_micro_secs;
      uint8_t  type_of_last_price[2];
      char     tick_direction;
      char     instrument_valuation_price;
      uint8_t  price_scale_code;
      uint8_t  variation_scale_code;
    } __attribute__ ((packed));

    // Msg Type 140
    struct  quote_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t source_seq_num;
      uint32_t source_time;
      uint32_t quote_link_id; // not used
      uint32_t ask_price;
      uint32_t ask_size;
      uint32_t bid_price;
      uint32_t bid_size;
      uint32_t system_id;
      uint16_t num_ask_orders;
      uint16_t num_bid_orders;
      uint16_t source_time_micro_secs;
      uint8_t  type_of_ask_price;
      uint8_t  type_of_bid_price;
      char     quote_condition;
      uint8_t  quote_number;
      uint8_t  price_scale_code;
      char     filler;
    } __attribute__ ((packed));

    // Msg Type 530
    struct  indicative_matching_price_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t source_seq_num;
      uint32_t source_time;
      uint32_t im_price;
      uint32_t variation;
      uint32_t system_id;
      uint16_t source_time_micro_secs;
      uint8_t  price_scale_code;
      uint8_t  variation_scale_code;
      uint32_t im_volume;
      char     imbalance_volume_side;  // Only present in version 3.26 and up
      char     filler[3];              // Only present in version 3.26 and up
      uint32_t imbalance_volume;       // Only present in version 3.26 and up
    } __attribute__ ((packed));
    
    // Msg Type 537
    struct  collar_message {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t source_seq_num;
      uint32_t source_time;
      uint32_t high_collar;
      uint32_t low_collar;
      uint32_t system_id;
      uint16_t source_time_micro_secs;
      uint8_t  high_scale_code;
      uint8_t  low_scale_code;
    } __attribute__ ((packed));
    

    // Msg Type 230
    struct  order_update {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t symbol_index;
      uint32_t source_time;
      uint32_t source_seq_num;
      uint32_t price;
      uint32_t aggregated_volume;
      uint32_t volume;
      uint32_t link_id;  // not used
      uint32_t order_id;
      uint32_t system_id;
      uint16_t source_time_micro_secs;
      uint16_t number_orders;
      char     side;
      char     order_type;
      char     action_type;
      uint8_t  price_scale_code;
      uint32_t order_date;
      uint32_t order_priorty_date;
      uint32_t order_priority_time;
      uint16_t order_priority_micro_seconds;
      char     filler[2];
    } __attribute__ ((packed));


    // Msg Type 231
    struct  order_retransmission_delimiter {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t source_time;
      uint32_t source_seq_num;
      char     trading_engine_id[2];
      uint8_t  instance_id;
      char     retransmission_indicator;
    } __attribute__ ((packed));

    // Msg Type 539
    struct session_timetable {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t source_seq_num;
      uint32_t source_time;
      uint32_t system_id;
      uint16_t source_time_micros;
      char     instrument_group_code[3];
      char     session_type;
      char     time_pre_opening1[6];
      char     time_opening1[6];
      char     time_closing1[6];
      char     time_pre_opening2[6];
      char     time_opening2[6];
      char     time_closing2[6];
      char     time_pre_opening3[6];
      char     time_opening3[6];
      char     time_closing3[6];
      char     eod_time[6];
      char     filler[2];
    } __attribute__ ((packed));
    

  }


}

#endif /* ifndef hf_md_Data_Euronext_XDP_Handler_h */
