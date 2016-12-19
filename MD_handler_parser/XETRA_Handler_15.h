#ifndef hf_core_Data_XETRA_Handler_15_h
#define hf_core_Data_XETRA_Handler_15_h

#include <Data/Handler.h>
#include <Data/MDBufferQueue.h>
#include <Data/QBDF_Builder.h>
#include <MD/MDOrderBook.h>
#include <MD/MDSubscriptionAcceptor.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Interface/AlertEvent.h>

#include <Logger/Logger.h>
#include <Util/Time.h>
#include <Util/Symbol_Lookup.h>

#include <boost/dynamic_bitset.hpp>

#include <Data/Xetra_Fast_Decoder_15.h>
#include <set>

#define MAX_XETRA_ISIX 30000

namespace hf_core {
  using namespace hf_tools;

  class MDProvider;

  struct XETRA_Channel_Type_15
  {
    enum Enum
    {
      UNKNOWN,
      DELTA,
      SNAPSHOT,
      INSTRUMENT,
      TRADES
    };
    
    static const char* to_string(Enum e) {
      switch(e) {
      case UNKNOWN: return "UNKNOWN";
      case DELTA: return "DELTA";
      case SNAPSHOT: return "SNAPSHOT";
      case INSTRUMENT: return "INSTRUMENT";
      case TRADES: return "TRADES";
      default: return "!INVALID!";
      }
      return "!INVALID!";
    }
    
    static Enum from_string(string s)
    {
      if(s == "DELTA")
      {
        return DELTA;
      }
      else if(s == "SNAPSHOT")
      {
        return SNAPSHOT;
      }
      else if(s == "INSTRUMENT")
      {
        return INSTRUMENT;
      }
      else if(s == "TRADES")
      {
        return TRADES;
      }
      else
      {
      cout << Time::current_time().to_string() << " - XETRA_Handler_15.h: invalid XETRA_Channel_Type_15 string " << s << endl;
        throw new exception;
      }
    }
  };

  struct DeltaMessage15
  {
     uint8_t* buf;
     size_t len;
     uint32_t template_id;
     uint32_t seq_num;

     void clean_up()
     {
       free(buf);
     }

     DeltaMessage15(uint8_t* buf_, size_t len_, uint32_t template_id_, uint32_t seq_num_) :
       len(len_),
       template_id(template_id_),
       seq_num(seq_num_)
     {
       buf = reinterpret_cast<uint8_t*>(malloc(len_));
       memcpy(buf, buf_, len_);
     }

     ~DeltaMessage15()
     {
       free(buf);
     }
  };

  class DeltaMessage15Queue
  {
  public:
    DeltaMessage15Queue()
      : m_max_size(0)
    {
    }

    DeltaMessage15Queue(size_t max_size)
      : m_max_size(max_size)
     {
     }

     void Set_Max_Size(size_t max_size)
     {
       m_max_size = max_size;
     }

     size_t Size()
     {
       return m_queue.size();
     }

     void Push(uint8_t* buf, size_t len, uint32_t template_id, uint32_t seq_num, bool & overflowing)
     {
       m_queue.push(new DeltaMessage15(buf, len, template_id, seq_num));
       if(m_queue.size() > m_max_size)
       {
         overflowing = true;
         Pop();
       }
       else
       {
         overflowing = false;
       }
     }

     DeltaMessage15* Front()
     {
       return m_queue.front();
     }

     void Pop()
     {
       DeltaMessage15* front = m_queue.front();
       delete front;
       m_queue.pop();
     }

     bool Empty()
     {
       return m_queue.empty();
     }

     void Clear()
     {
       while(!Empty())
       {
         Pop();
       }
     }

     size_t MaxSize()
     {
       return m_max_size;
     }

  private:
    size_t m_max_size;
    queue<DeltaMessage15*> m_queue;
  };

  struct Price_Point_15
  {
    Price_Point_15()
      : is_market_order(false)
    {
    }

    MDPrice md_price;
    uint32_t size;
    bool is_market_order;
  };


  struct Xetra_Security_Definition_15
  {
    Xetra_Security_Definition_15()
      : product_id(Product::invalid_product_id),
        needs_snapshot(false),
        expected_seq_num(0),
        max_depth(0),
        last_match_id_number_seen(0),
        last_midpoint_match_id_number_seen(0),
        isix(0),
        m_num_bids(0),
        m_num_offers(0),
        m_has_market_bid(false),
        m_has_market_offer(false)
    {
    }

    void Initialize(ProductID productId, uint32_t maxDepth, ExchangeID exchange_id, uint32_t isix_)
    {
      product_id = productId;
      max_depth = maxDepth;
      delta_message_queue.Set_Max_Size(1024); // Probably don't need that much
      auction_update.m_id = productId;
      auction_update.m_exch = exchange_id;
      isix = isix_;
    }
 
    ProductID product_id;
    bool needs_snapshot;
    DeltaMessage15Queue delta_message_queue;
    uint32_t expected_seq_num;
    uint32_t max_depth;
    AuctionUpdate auction_update;
    set<uint32_t> potential_missed_match_ids;
    uint32_t last_match_id_number_seen;
    uint32_t last_midpoint_match_id_number_seen;
    uint32_t isix;
    Price_Point_15 m_bids[61]; // 3 * max max_depth + 1
    Price_Point_15 m_offers[61];
    int m_num_bids;
    int m_num_offers;
    bool m_has_market_bid;
    bool m_has_market_offer;
  };

  class XETRA_Handler_15;

  class XETRA_CHannel_Handler_15
  {
  public:
    XETRA_CHannel_Handler_15(int channel_id,
                                 boost::shared_ptr<MDOrderBookHandler> order_book,
                                 ExchangeID exch,
                                 ExchangeRuleRP exch_rule,
                                 Feed_Rules_RP feed_rules,
                                 boost::shared_ptr<MDSubscriptionAcceptor> subscribers,
                                 MDProvider*                               provider,
                                 hash_map<string, ProductID>& prod_map,
                                 Xetra_Security_Definition_15* security_definitions_by_isix,
                                 QBDF_Builder* qbdf_builder,
                                 char qbdf_exch_char,
                                 vector<MarketStatus>& security_status,
                                 LoggerRP logger, XETRA_Handler_15* handler);

    void qbdf_update_level(ProductID product_id, char side, uint32_t shares, MDPrice md_price, uint32_t num_orders, bool last_in_group, Time& exch_time);
    void clear_book(Xetra_Security_Definition_15& sec_def, Time& exch_time);

    template<typename T> void show_snapshot(T& msg, Xetra_Security_Definition_15& sec_def);
    template<typename T> void apply_snapshot(T& msg, Xetra_Security_Definition_15& sec_def);
    template<typename T> void process_delta(T& delta, Xetra_Security_Definition_15& sec_def);

    void handle_state_update(uint32_t state, Xetra_Security_Definition_15& sec_def);

    size_t handle_delta(size_t context, uint8_t* buf, size_t len);

    size_t handle_snapshot(size_t context, uint8_t* buf, size_t len);

    size_t handle_trades(size_t context, uint8_t* buf, size_t len);

    void set_subscribers(boost::shared_ptr<MDSubscriptionAcceptor> s)
    {
      m_subscribers = s;
    }
    void set_provider(MDProvider* p)
    {
      m_provider = p;
    }

    void set_qbdf_builder(QBDF_Builder* qbdf_builder)
    {
      m_qbdf_builder = qbdf_builder;
    }

    int m_channel_id;

  private:

    ProductID product_id_of_symbol(string& symbol_string);
    void send_quote_update(Xetra_Security_Definition_15& sec_def);
    void send_trade_update();
    void send_auction_update(AuctionUpdate& auctionUpdate);

    boost::shared_ptr<MDOrderBookHandler> m_order_book;
    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;
    boost::shared_ptr<MDSubscriptionAcceptor> m_subscribers;
    MDProvider*                               m_provider;
    hash_map<string, ProductID>& m_prod_map;
    Xetra_Security_Definition_15* m_security_definitions_by_isix;
    QBDF_Builder* m_qbdf_builder;
    char m_qbdf_exch_char;
    vector<MarketStatus>& m_security_status;
    LoggerRP m_logger;
    XETRA_Handler_15* m_handler;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;
    QBDFStatusType m_qbdf_status;
    string m_qbdf_status_reason;

    xetra_15::Xetra_Fast_Decoder m_fast_decoder;
  };


  struct XETRA_Context_Info_15
  {
    XETRA_CHannel_Handler_15* m_channel_handler;
    channel_info* m_channel_info;
    XETRA_Channel_Type_15::Enum m_channel_type;

    size_t handle(size_t context, const char* bufC, size_t len)
    {
       uint8_t* buf = reinterpret_cast<uint8_t*>(const_cast<char*>(bufC));
       switch(m_channel_type)
       {
         case XETRA_Channel_Type_15::DELTA:
           {
             return m_channel_handler->handle_delta(context, buf, len);
           }
           break;
         case XETRA_Channel_Type_15::SNAPSHOT:
           {
             return m_channel_handler->handle_snapshot(context, buf, len);
           }
           break;
         case XETRA_Channel_Type_15::INSTRUMENT:
           {
             cout << "XETRA_Context_Info_15::handle: channel type=instrument.  Shouldn't happen since instruments are handled by the XETRA_Handler_15 directly" << endl;
             throw new std::exception();
           }
           break;
         case XETRA_Channel_Type_15::TRADES:
           {
             return m_channel_handler->handle_trades(context, buf, len);
           }
           break;
         default:
           {
             cout << "Unrecognized channel type" << endl;
             return len;
           }
       }
    }

    void init(XETRA_CHannel_Handler_15* channel_handler_, channel_info* channel_info_, XETRA_Channel_Type_15::Enum channel_type_)
    {
      m_channel_handler = channel_handler_;
      m_channel_info = channel_info_;
      m_channel_type = channel_type_;
    }
  };



  class XETRA_Handler_15 : public Handler {
  public:
    // Handler functions
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t) { return 0; }
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg);

    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buffer, size_t len);
    
    void init(const string&, ExchangeID exch, const Parameter&);
    virtual void start();
    virtual void stop();
    
    void subscribe_product(ProductID id_,ExchangeID exch_,
                           const string& mdSymbol_,const string& mdExch_);

    virtual bool sets_up_own_contexts() { return true; }

    XETRA_Handler_15(Application* app);
    virtual ~XETRA_Handler_15();

  private:
    typedef tbb::spin_mutex mutex_t;

    XETRA_CHannel_Handler_15* register_channel(int channel_id)
    {
      hash_map<int, XETRA_CHannel_Handler_15*>::iterator it = m_channel_handlers.find(channel_id);
      if(it == m_channel_handlers.end())
      {
        m_channel_handlers[channel_id] = new XETRA_CHannel_Handler_15(channel_id, m_order_book, m_exch, m_exch_rule_global, m_feed_rules_global, m_subscribers, m_provider, m_prod_map_global, m_security_definitions_by_isix, m_qbdf_builder, m_qbdf_exch_char_global, m_security_status, m_logger, this);
      }
      return m_channel_handlers[channel_id];
    }

    void set_up_channels(const Parameter& params);

    vector<XETRA_Context_Info_15> m_context_info;

    string m_name;
    
    mutable mutex_t m_mutex;
    hash_map<string, ProductID> m_prod_map_global;
    
    size_t handle_instrument(uint8_t* buf, size_t len);
    xetra_15::Xetra_Fast_Decoder m_fast_instrument_decoder;
    bool m_finished_handling_instruments;
    bool m_received_start_ref_data;
    uint32_t m_instrument_expected_seq_num;
    uint32_t m_instrument_expected_packet_seq_num;
    Xetra_Security_Definition_15 m_security_definitions_by_isix[MAX_XETRA_ISIX];

    Time m_midnight;
    
    ExchangeRuleRP m_exch_rule_global;
    Feed_Rules_RP m_feed_rules_global;

    MarketStatus m_market_status;
    
    boost::shared_ptr<MDOrderBookHandler>     m_order_book;
   
    hash_map<int, XETRA_CHannel_Handler_15*> m_channel_handlers; 
    hash_map<ProductID, uint32_t> m_products_seen;

    char m_qbdf_exch_char_global;
    ExchangeID m_exch;

    vector<MarketStatus> m_security_status;
  };

}

#endif /* ifndef hf_core_Data_XETRA_Handler_15_h */
