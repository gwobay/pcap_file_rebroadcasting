#ifndef hf_md_Data_NYSE_Handler_h
#define hf_md_Data_NYSE_Handler_h

#include <Data/Handler.h>
#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>
#include <Interface/AlertEvent.h>
#include <Logger/Logger.h>
#include <MD/MDOrderBook.h>
#include <MD/MDSubscriptionAcceptor.h>
#include <Util/Parameter.h>
#include <Util/Time.h>


namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  using namespace hf_core;

  class MDProvider;
  class NYSE_XDP_Handler;

  class NYSE_Handler : public Handler {
  public:

    struct linkid_to_side {
      uint32_t link_id;
      char     side;

      linkid_to_side(uint32_t link_id, char side) : link_id(link_id), side(side) { }
    };

    // Handler functions
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t context, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t);
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg);
    virtual ProductID product_id_of_symbol(const char* symbol);
    virtual ProductID product_id_of_symbol_index(uint16_t symbol_index);
    
    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);
    
    virtual void init(const string& name, ExchangeID exch, const Parameter& params);
    virtual void start();
    virtual void stop();
    virtual void send_retransmit_request(channel_info& channel_info);
    
    virtual void add_context_product_id(size_t context, ProductID id);
    virtual void subscribe_product(ProductID id_,ExchangeID exch_,
                                   const string& mdSymbol_,const string& mdExch_);

    void send_quote_update(char trading_status);
    void send_trade_update();
    void set_openbook_tickers(const map<uint16_t,string>& openbook_tickers) { m_openbook_tickers = openbook_tickers; }
    void set_link_record_map(const link_record_map_t& link_record_map) { m_link_record_map = link_record_map; }


    NYSE_Handler(Application* app);
    virtual ~NYSE_Handler();

    static inline double get_price_scale_mult(char price_scale_code)
    {
      switch(price_scale_code) {
      case 0: return 1.0;
      case 1: return 0.1;
      case 2: return 0.01;
      case 3: return 0.001;
      case 4: return 0.0001;
      case 5: return 0.00001;  // should be overkill at this point
      case 6: return 0.000001;
      case 7: return 0.0000001;;
      default:
        return 1.0/pow(10, price_scale_code);
      }
    }

    static inline MDPrice get_int_price(uint32_t iPrice, char price_scale_code)
    {
      uint64_t price = ntohl(iPrice);
      switch(price_scale_code) {
      case 0: price *= 100000000; break;
      case 1: price *= 10000000; break;
      case 2: price *= 1000000; break;
      case 3: price *= 100000; break;
      case 4: price *= 10000; break;
      case 5: price *= 1000; break;
      case 6: price *= 100; break;
      case 7: price *= 10; break;
      case 8: break;
      case 9: price /= 10; break;
      case 10:price /= 100; break;
      default:
        price = 0;
      }
      return MDPrice::from_iprice64(price);
    }

    void send_heartbeat_response(channel_info& channel_info);
    void handle_retran_msg(channel_info& channel_info, const char* buf, size_t len);

    void set_linkid_source(NYSE_Handler* nh);

  protected:
    struct eqstr {
      bool operator()(const char* s1, const char* s2) const {
        return strcmp(s1, s2) == 0;
      }
    };
    
    typedef tbb::spin_mutex mutex_t;
    typedef hash_map<const char*, ProductID, __gnu_cxx::hash<const char *>, eqstr> symbol_to_product_id_t;
    typedef vector<ProductID> security_index_to_product_id_t;
    
    string m_name;
    string m_source_id;

    symbol_to_product_id_t m_symbol_to_product_id;
    set<string> m_symbols;
    security_index_to_product_id_t m_security_index_to_product_id;
    vector<vector<ProductID> > m_context_to_product_ids;

    Time_Duration m_max_send_source_difference;

    mutable mutex_t m_mutex;

    Time m_midnight;
    int  m_lot_size;
    int  m_nyse_product_id;

    bool m_use_old_imbalance;
    
    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;
    QBDFStatusType m_qbdf_status;
    string m_qbdf_status_reason;

    vector<MarketStatus> m_security_status;

    boost::shared_ptr< vector< deque<linkid_to_side> > > m_linkid_queue;  // by productid
    size_t m_max_linkid_queue_size;

    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;

    MarketStatus m_market_status;

    boost::shared_ptr<MDOrderBookHandler>     m_order_book;
    
    map<uint16_t,string> m_openbook_tickers;
    link_record_map_t m_link_record_map;
    bool m_read_link_records;
    bool m_write_link_records;

    friend class NYSE_XDP_Handler;
  };

  namespace NYSE_structs {
    struct message_header {
      uint16_t msg_size;
      uint16_t msg_type;
      uint32_t seq_num;
      uint32_t send_time;
      uint8_t  product_id;
      uint8_t  retrans_flag;
      uint8_t  num_body_entries;
      uint8_t  link_flag;
    }  __attribute__ ((packed));

    struct sequence_reset_message {
      uint32_t new_seq_no;
    }  __attribute__ ((packed));

    struct bbo_message {
      uint32_t source_time;
      uint32_t filler;
      uint32_t ask_price_numerator;
      uint32_t ask_size;
      uint32_t bid_price_numerator;
      uint32_t bid_size;
      char     price_scale_code;
      char     exchange_id;
      char     security_type;
      char     quote_condition;
      char     symbol[16];
    }  __attribute__ ((packed));

    struct trade_message {
      uint32_t source_time;
      uint32_t link_id;
      uint32_t filler;
      uint32_t price_numerator;
      uint32_t volume;
      uint32_t source_seq_num;
      char     source_session_id;
      char     price_scale_code;
      char     exchange_id;
      char     security_type;
      char     trade_condition[4];
      char     symbol[16];
    }  __attribute__ ((packed));

    struct trade_cancel_message {
      uint32_t source_time;
      uint32_t source_seq_num;
      uint32_t original_ref_num;
      char     source_session_id;
      char     exchange_id;
      char     security_type;
      char     filler;
      char     symbol[16];
    } __attribute__ ((packed));

    struct trade_correction_message {
      uint32_t source_time;
      uint32_t filler1;
      uint32_t filler2;
      uint32_t price_numerator;
      uint32_t volume;
      uint32_t source_seq_num;
      uint32_t original_ref_num;
      char     source_session_id;
      char     price_scale_code;
      char     exchange_id;
      char     security_type;
      char     corrected_trade_condition[4];
      char     symbol[16];
    } __attribute__ ((packed));

    struct full_price_point {
      uint32_t price_numerator;
      uint32_t volume;
      uint16_t num_orders;
      char     side;
      char     filler;
    }  __attribute__ ((packed));

    struct full_update_message {
      uint16_t msg_size;
      uint16_t security_index;
      uint32_t source_time;
      uint16_t source_time_micro;
      uint32_t symbol_seq_num;
      uint8_t  source_session_id;
      char     symbol[11];
      uint8_t  price_scale_code;
      char     quote_condition;
      char     trading_status;
      char     filler;
      uint16_t mpv;
    }  __attribute__ ((packed));

    struct delta_price_point {
      uint32_t price_numerator;
      uint32_t volume;
      uint32_t change_qty;
      uint16_t num_orders;
      char     side;
      char     reason_code;
      uint32_t linkid_1;
      uint32_t linkid_2;
      uint32_t linkid_3;
    }  __attribute__ ((packed));

    struct delta_update_message {
      uint16_t msg_size;
      uint16_t security_index;
      uint32_t source_time;
      uint16_t source_time_micro;
      uint32_t symbol_seq_num;
      uint8_t  source_session_id;
      char     quote_condition;
      char     trading_status;
      uint8_t  price_scale_code;
    }  __attribute__ ((packed));

    struct open_imbalance_message {
      char     symbol[11];
      char     stock_open_indicator;
      char     imbalance_side;
      char     price_scale_code;
      uint32_t reference_price_numerator;
      uint32_t imbalance_quantity;
      uint32_t paired_quantity;
      uint32_t clearing_price_numerator;
      uint32_t source_time;
      uint32_t ssrfiling_price_numerator;
    } __attribute__ ((packed));

    struct old_open_imbalance_message {
      char     symbol[11];
      char     stock_open_indicator;
      char     imbalance_side;
      char     price_scale_code;
      uint32_t reference_price_numerator;
      uint32_t imbalance_quantity;
      uint32_t paired_quantity;
      uint32_t clearing_price_numerator;
      uint32_t source_time;
    } __attribute__ ((packed));

    struct close_imbalance_message {
      char     symbol[11];
      char     regulatory_imbalance_indicator;
      char     imbalance_side;
      char     price_scale_code;
      uint32_t reference_price_numerator;
      uint32_t imbalance_quantity;
      uint32_t paired_quantity;
      uint32_t continuous_clearing_price_numerator;
      uint32_t closing_clearing_price_numerator;
      uint32_t source_time;
    } __attribute__ ((packed));

    struct security_info_message {
      uint32_t source_time;
      char     symbol[11];
      char     security_type;
      uint16_t filler;
      uint16_t minimum_price_variation;
      char     post;
      uint16_t panel;
      char     ticket_desgination;
      char     IPO_flag;
      char     country_code[3];
      uint16_t unit_of_trade;
      char     price_scale_code;
      char     LRP_price_scape_code;
      uint16_t LRP;
      char     bankruptcy_flag;
      char     financial_status;
      char     ex_distribution_flag;
      char     ex_rights_flag;
      char     ex_dividend_flag;
      char     ex_divident_amt_price_scale_code;
      uint32_t ex_divident_amt;
      char     ex_divident_date[5];
      char     special_div_flag;
      char     stock_split;
      char     rule_19C3;
      char     ITS_eligible;
    } __attribute__ ((packed));

    struct delay_halt_message {
      uint32_t source_time;
      char     symbol[11];
      char     security_status;
      char     halt_condition;
    } __attribute__ ((packed));

    struct circuit_breaker_message {
      uint32_t event_time;
      char     status;
      char     url[128];
    } __attribute__ ((packed));

    struct short_sale_restriction_message {
      uint32_t source_time;
      char     symbol[11];
      char     security_status;
      char     short_sale_restriction_indicator;
      char     triggering_exchange_id;
      uint32_t short_sale_trigger_time;
      uint32_t trade_price;
      char     price_scale_code;
      uint32_t trade_volume;
    } __attribute__ ((packed));

    struct retail_price_improvement {
      uint32_t  source_time;
      char      symbol[11];
      char      rpi_indicator;
    } __attribute__ ((packed));

    struct heartbeat_response_message {
      message_header header;
      char           source_id[20];
    } __attribute__ ((packed));

    struct retransmission_request_message {
      message_header header;
      uint32_t       begin_seq_num;
      uint32_t       end_seq_num;
      char           source_id[20];
    } __attribute__ ((packed));

    struct retransmission_response_message {
      uint32_t  source_seq_num;
      char      source_id[20];
      char      status;
      char      reject_reason;
      char      filler[2];
    } __attribute__ ((packed));

    struct message_unavailable {
      uint32_t  begin_seq_num;
      uint32_t  end_seq_num;
    } __attribute__ ((packed));

  }


}

#endif /* ifndef hf_md_Data_NYSE_Handler_h */
