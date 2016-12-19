#pragma once

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

namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  using namespace hf_core;

  class MDProvider;

  class NYSE_XDP_Integrated_Handler : public Handler {
  public:
    
    typedef vector<uint64_t> order_id_vector_t;

    struct symbol_info {
      int8_t    price_scale_code;
      uint8_t   matching_engine;
      symbol_info() :
        price_scale_code(-1), matching_engine(0)
      { }
    };
        
    void set_num_contexts(size_t nc) {
      m_context_to_product_ids.resize(nc);
      //m_context_to_security_info_map.resize(nc);
    }
    
    // Handler functions
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t context, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t);
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg);
    
    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len);

    virtual void init(const string& name, ExchangeID exch, const Parameter& params);
    virtual void start();
    virtual void stop();
    virtual void send_retransmit_request(channel_info& channel_info);
    
    virtual void add_context_product_id(size_t context, ProductID id);
    virtual void subscribe_product(ProductID id_,ExchangeID exch_, const string& mdSymbol_,const string& mdExch_);
    
    void send_quote_update();
    void send_trade_update();
    
    NYSE_XDP_Integrated_Handler(Application* app);
    virtual ~NYSE_XDP_Integrated_Handler();
    
    static inline MDPrice get_int_price(uint32_t price, int price_scale_code)
    {
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

    void send_heartbeat_response(channel_info& channel_info);
    void handle_retran_msg(channel_info& channel_info, const char* buf, size_t len);
    
  protected:
    struct eqstr{
      bool operator()(const char* s1, const char* s2) const {
        return strcmp(s1, s2) == 0;
      }
    };
    ProductID product_id_of_symbol_index(uint32_t symbol_index);
    
    typedef tbb::spin_mutex mutex_t;
    typedef hash_map<uint32_t, symbol_info> symbol_info_map_t;
    
    Symbol_Lookup8       m_symbol_to_product_id;
    vector<symbol_info>  m_symbol_info_vector;
    vector<Time>         m_time_references;
        
    string m_name;
    string m_source_id;

    vector<vector<ProductID> > m_context_to_product_ids;

    Time_Duration m_max_send_source_difference;
    uint64_t m_source_time_seconds;

    mutable mutex_t m_mutex;

    int  m_lot_size;
    int  m_nyse_product_id;

    int m_req_seq_num;

    bool m_use_old_imbalance;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;

    vector<MarketStatus> m_security_status;
    
    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;

    MarketStatus m_market_status;
    
    boost::shared_ptr<MDOrderBookHandler>     m_order_book;

    symbol_info_map_t    m_symbol_info_map;
    uint64_t m_micros_midnight;
    char m_exch_char;
    
  };

  namespace NYSE_XDP_Integrated_structs {
    struct packet_header {
      uint16_t pkt_size;
      uint8_t  delivery_flag;
      uint8_t  number_msgs;
      uint32_t seq_num;
      uint32_t send_time;
      uint32_t send_time_ns;
    };

    struct message_header {
      uint16_t msg_size;
      uint16_t msg_type;
    }  __attribute__ ((packed));

// Control Messages

    // Msg Type 1
    struct sequence_number_reset {
      uint32_t source_time;
      uint32_t source_time_ns;
      uint8_t  product_id;
      uint8_t  channel_id;
    } __attribute__ ((packed));

    struct source_time_reference_message {
      uint32_t id;
      uint32_t symbol_seq_num;
      uint32_t time_reference;
    } __attribute__ ((packed));

// Client Request Messages
    // Msg Type 12
    struct heartbeat_response_message {
      char     source_id[10];
    } __attribute__ ((packed));

    // Msg Type 10
    struct retransmission_request_message {
      uint32_t begin_seq_num;
      uint32_t end_seq_num;
      char     source_id[10];
      uint8_t  product_id;
      uint8_t  channel_id;
    } __attribute__ ((packed));

    // Msg Type 13
    struct symbol_index_mapping_request {
      uint32_t symbol_index;
      char     source_id[10];
      uint8_t  product_id;
      uint8_t  channel_id;
      uint8_t  retransmit_method;
    } __attribute__ ((packed));

// Response Messages

    // Msg type 11
    struct request_response_message {
      uint32_t request_seq_num;
      uint32_t begin_seq_num;
      uint32_t end_seq_num;
      char     source_id[10];
      uint8_t  product_id;
      uint8_t  channel_id;
      char     status;

    } __attribute__ ((packed));

    // Msg Type 3
    struct symbol_index_mapping_message {
      uint32_t symbol_index;
      char     symbol[11];
      char     filler;
      uint16_t market_id;
      uint8_t  system_id;
      char     exchange_code;
      uint8_t  price_scale_code;
      char     security_type;
      uint16_t UOT;
      uint32_t prev_close_price;
      uint32_t prev_close_volume;
      uint8_t  price_resolution;
      char     round_lot;
      char     bloomberg_company[12];
      char     bloomberg_security_id[20];
      char     bloomberg_symbol[30];
    } __attribute__ ((packed));

    struct message_unavailable_message {
      uint32_t begin_seq_num;
      uint32_t end_seq_num;
      uint8_t  product_id;
      uint8_t  channel_id;
    } __attribute__ ((packed));
    
    // Msg Type 32
    struct symbol_clear_message {
      uint32_t source_time;
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t next_symbol_seq_num;
    } __attribute__ ((packed));
    
    // Msg Type 33
    struct trading_session_change_message {
      uint32_t source_time;
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint8_t  trading_session;
    } __attribute__ ((packed));

    // Msg Type 34
    struct security_status_message {
      uint32_t source_time;
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      char     security_status;
      char     halt_condition;
    } __attribute__ ((packed));

    // Msg Type 35
    struct refresh_header {
      uint16_t current_refresh_pkt;
      uint16_t total_refresh_pkts;
      uint32_t last_seq_num;
      uint32_t last_symbol_seq_num;
    } __attribute__ ((packed));
    
    // Msg Type 100
    struct add_order_message {
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint64_t order_id;
      uint32_t price;
      uint32_t volume;
      char     side;
      char     firm_id[5];
      uint8_t  num_parity_splits;
    } __attribute__ ((packed));

    // Msg Type 101
    struct modify_order_message {
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint64_t order_id;
      uint32_t price;
      uint32_t volume;
      uint8_t  position_change;
      uint8_t  prev_price_parity_splits;
      uint8_t  new_price_parity_splits;
    } __attribute__ ((packed));

    // Msg Type 104
    struct replace_order_message {
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint64_t order_id;
      uint64_t new_order_id;
      uint32_t price;
      uint32_t volume;
      uint8_t  prev_price_parity_splits;
      uint8_t  new_price_parity_splits;
    } __attribute__ ((packed));

    // Msg Type 102
    struct delete_order_message {
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint64_t order_id;
      uint8_t  num_parity_splits;
    } __attribute__ ((packed));

    // Msg Type 103
    struct execution_message {
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint64_t order_id;
      uint32_t trade_id;
      uint32_t price;
      uint32_t volume;
      uint8_t  printable_flag;
      uint8_t  num_parity_splits;
    } __attribute__ ((packed));

    // Msg Type 110
    struct non_displayed_trade_message {
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint32_t trade_id;
      uint32_t price;
      uint32_t volume;
      uint8_t  printable_flag;
      uint32_t db_exec_id;
    } __attribute__ ((packed));
    
    // Msg Type 112
    struct trade_cancel_message {
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint32_t trade_id;
    } __attribute__ ((packed));
    
    // Msg Type 111
    struct cross_trade_message {
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint32_t cross_id;
      uint32_t price;
      uint32_t volume;
      uint8_t  cross_type;
    } __attribute__ ((packed));

    // Msg Type 113
    struct cross_correction_message {
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint32_t cross_id;
      uint32_t volume;
    } __attribute__ ((packed));
    
    // Msg Type 105
    struct imbalance_message {
      uint32_t source_time;
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint32_t reference_price;
      uint32_t paired_qty;
      int32_t  total_imbalance_qty;
      int32_t  market_imbalance_qty;
      uint16_t auction_time;
      char     auction_type;
      char     imbalance_side;
      uint32_t continuous_book_clearing_price;
      uint32_t closing_only_clearing_price;
      uint32_t ssr_filing_price;
    } __attribute__ ((packed));

    // Msg Type 106
    struct add_order_refresh_message {
      uint32_t source_time;
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t symbol_seq_num;
      uint64_t order_id;
      uint32_t price;
      uint32_t volume;
      char     side;
      char     firm_id[5];
      uint8_t  num_parity_splits;
    } __attribute__ ((packed));
    
    struct stock_summary_message {
      uint32_t source_time;
      uint32_t source_time_ns;
      uint32_t symbol_index;
      uint32_t high_price;
      uint32_t low_price;
      uint32_t open;
      uint32_t close;
      uint32_t total_volume;
    } __attribute__ ((packed));

  }

}
