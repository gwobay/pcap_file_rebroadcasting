#pragma once

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
#include <queue>

#include <boost/dynamic_bitset.hpp>

#include <Data/Cme_Fast_Decoder.h>

namespace hf_core {
  using namespace hf_tools;

  class MDProvider;

  struct CME_Channel_Type
  {
    enum Enum
    {
      UNKNOWN,
      INCREMENTAL,
      SNAPSHOT,
      INSTRUMENT
    };

    static Enum from_string(string s)
    {
      if(s == "INCREMENTAL")
      {
        return INCREMENTAL;
      }
      else if(s == "SNAPSHOT")
      {
        return SNAPSHOT;
      }
      else if(s == "INSTRUMENT")
      {
        return INSTRUMENT;
      }
      else
      {
      cout << Time::current_time().to_string() << " - CME_MDP_FAST_Handler.h: invalid CME_Channel_Type string " << s << endl;
        throw new exception;
      }
    }
  };

  struct IncrementalMessage
  {
     uint32_t security_id;
     uint32_t update_action;
     char entry_type;
     uint32_t price_level;
     double price;
     int32_t size;
     uint32_t trade_volume;
     char trade_condition;
     char quote_condition;
     bool last_in_group;
     uint32_t rpt_seq;
     char match_event_indicator;

     IncrementalMessage(uint32_t security_id_, uint32_t update_action_, char entry_type_, uint32_t price_level_, double price_, int32_t size_, uint32_t trade_volume_,
                        char trade_condition_, char quote_condition_, bool last_in_group_, uint32_t rpt_seq_, char match_event_indicator_)
       :
       security_id(security_id_),
       update_action(update_action_),
       entry_type(entry_type_),
       price_level(price_level_),
       price(price_),
       size(size_),
       trade_volume(trade_volume_),
       trade_condition(trade_condition_),
       quote_condition(quote_condition_),
       last_in_group(last_in_group_),
       rpt_seq(rpt_seq_),
       match_event_indicator(match_event_indicator_)
     {

     }
  };

  class IncrementalMessageQueue
  {
  public:
    IncrementalMessageQueue(size_t max_size)
      : m_max_size(max_size)
     {
     }

     void Push(uint32_t security_id_, uint32_t update_action_, char entry_type_, uint32_t price_level_, double price_, int32_t size_, uint32_t trade_volume_,
                        char trade_condition_, char quote_condition_, bool last_in_group_, uint32_t rpt_seq_, char match_event_indicator_)
     {
       m_queue.push(new IncrementalMessage(security_id_, update_action_, entry_type_, price_level_, price_, size_, trade_volume_,
                                           trade_condition_, quote_condition_, last_in_group_, rpt_seq_, match_event_indicator_));
       if(m_queue.size() > m_max_size)
       {
         cout << "Incremental message queue is overflowing.  Max size=" << m_max_size << endl;
         Pop();
       }
     }

     IncrementalMessage* Front()
     {
       return m_queue.front();
     }

     void Pop()
     {
       IncrementalMessage* front = m_queue.front();
       delete front;
       m_queue.pop();
     }

     bool Empty()
     {
       return m_queue.empty();
     }

     void Clear()
     {
       while(!m_queue.empty())
       {
         m_queue.pop();
       }
     }

  private:
    size_t m_max_size;
    queue<IncrementalMessage*> m_queue;
  };

  class CME_MDP_FAST_Handler;

  class CME_MDP_FAST_Channel_Handler
  {
  public:
    CME_MDP_FAST_Channel_Handler(int channel_id, 
                                 boost::shared_ptr<MDOrderBookHandler> order_book, 
                                 boost::shared_ptr<MDOrderBookHandler> implied_order_book, 
                                 ExchangeID exch,
                                 ExchangeRuleRP exch_rule,
                                 Feed_Rules_RP feed_rules,
                                 boost::shared_ptr<MDSubscriptionAcceptor> subscribers,
                                 MDProvider*                               provider,
                                 hash_map<uint64_t, ProductID>& prod_map,
                                 QBDF_Builder* qbdf_builder,
                                 char qbdf_exch_char,
                                 LoggerRP instrument_logger,
                                 CME_MDP_FAST_Handler* handler);

    size_t handle_incremental(size_t context, uint8_t* buf, size_t len);

    size_t handle_snapshot(size_t context, uint8_t* buf, size_t len);

    size_t handle_instrument(size_t context, uint8_t* buf, size_t len);

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

    QBDF_Builder* get_qbdf_builder()
    {
      return m_qbdf_builder;
    }

    void AddInstrumentDefinition(uint32_t securityId, string& symbol, unsigned maturityMonthYear, double displayFactor, 
                                                  double minPriceIncrement, uint32_t unit_of_measure_qty, uint32_t outright_max_depth, 
                                                  uint32_t implied_max_depth, uint32_t totNumReports, string& security_group, bool publish_status); 
    void apply_incremental_update(uint32_t security_id, uint32_t update_action, char entry_type, uint32_t price_level, double price, int32_t size, uint32_t tradeVolume, char tradeCondition, char quoteCondition, bool last_in_group, uint32_t rpt_seq, char match_event_indicator);

    void apply_security_status(uint32_t security_id, uint32_t security_status);

    void publish_current_state_to_qbdf_snap();

    void set_is_hist(bool is_hist) { m_is_hist = is_hist; }

    int m_channel_id;
  private:
    struct Price_Point
    {
      MDPrice md_price;
      uint32_t size;
    };

    struct Book_State
    {
      Book_State()
        : m_num_bids(0), m_num_offers(0)
      { }

      Price_Point m_bids[25];
      Price_Point m_offers[25];
      int m_num_bids;
      int m_num_offers;
    };

    struct Security_Definition
    {
      uint32_t security_id;
      string symbol;
      ProductID product_id;
      double display_factor;
      double minimum_tick_increment;
      uint32_t outright_max_depth;
      uint32_t implied_max_depth;
      uint32_t rpt_seq;
      bool needs_snapshot;
      IncrementalMessageQueue incremental_message_queue;
      Book_State m_outright_state;
      Book_State m_implied_state;
      bool is_trade_event;

      Security_Definition(uint32_t security_id_, 
                          string& symbol_,
                          ProductID product_id_,
                          double display_factor_,
                          double minimum_tick_increment_,
                          uint32_t outright_max_depth_,
                          uint32_t implied_max_depth_)
        : security_id(security_id_),
          symbol(symbol_),
          product_id(product_id_),
          display_factor(display_factor_),
          minimum_tick_increment(minimum_tick_increment_),
          outright_max_depth(outright_max_depth_),
          implied_max_depth(implied_max_depth_),
          rpt_seq(0),
          needs_snapshot(false),
          incremental_message_queue(256*256), // arbitrarily selected, seems like plenty
          is_trade_event(false)
      {
      }
    };

    ProductID product_id_of_symbol(string symbol_string);
    void update_quote_status(ProductID product_id, QuoteStatus quote_status, QBDFStatusType qbdf_status, string qbdf_status_reason);
    void send_quote_update();
    void send_trade_update();

    void apply_security_status(Security_Definition& sec_def, uint32_t security_status);
    template<typename T>
    inline void handle_security_status(T& msg);
    template<typename T>
    inline void handle_instrument_msg(T& msg);
    template<typename T>
    inline void handle_snapshot_msg(T& msg, uint32_t seqNum);

    size_t process_incremental(uint8_t* buf, size_t len, uint32_t template_id, uint32_t seq_num);

    void clear_book(Security_Definition& sec_def);
    void clear_books_on_this_channel();

    inline void qbdf_update_level(ProductID product_id, char side, uint32_t shares, MDPrice md_price, bool last_in_group, bool is_snapshot, char quote_condition, bool is_executed);

    boost::shared_ptr<MDOrderBookHandler> m_order_book;
    boost::shared_ptr<MDOrderBookHandler> m_implied_order_book;
    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;
    boost::shared_ptr<MDSubscriptionAcceptor> m_subscribers;
    MDProvider*                               m_provider;
    hash_map<uint64_t, ProductID>& m_prod_map;
    QBDF_Builder* m_qbdf_builder;
    char m_qbdf_exch_char;
    uint32_t m_expected_incremental_seq_num;
    bool m_finished_handling_instruments;
    uint32_t m_last_seq_num_processed;
    uint32_t m_num_symbols_snapshotted;
    uint32_t m_num_valid_products; //ToDo: put all the vars pertaining to snapshot synchronization into their own little struct to make things clearer?
    bool m_is_hist;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;
    QBDFStatusType m_qbdf_status;
    string m_qbdf_status_reason;
    LoggerRP m_instrument_logger;
    CME_MDP_FAST_Handler* m_handler;

    Cme_Fast_Decoder m_fast_decoder;

    hash_map<uint32_t, Security_Definition> m_security_definitions;
    //vector<MarketStatus> m_security_status;
  };

  struct CME_MDP_FAST_Context_Info
  {
    CME_MDP_FAST_Channel_Handler* m_channel_handler;
    channel_info* m_channel_info;
    CME_Channel_Type::Enum m_channel_type;
    bool m_record_only;

    size_t handle(size_t context, const char* bufC, size_t len)
    {
       // Ignoring non index future channels for now

/*
       if(m_channel_handler->m_channel_id != 9)
       {
         return len;
       }
*/
       uint8_t* buf = reinterpret_cast<uint8_t*>(const_cast<char*>(bufC));
       switch(m_channel_type)
       {
         case CME_Channel_Type::INCREMENTAL:
           { 
             if(m_record_only) {
               return len;
             }
             return m_channel_handler->handle_incremental(context, buf, len);
           }
           break;
         case CME_Channel_Type::SNAPSHOT:
           { 
             if(m_record_only) {
               return len;
             }
             return m_channel_handler->handle_snapshot(context, buf, len);
           }
           break;
         case CME_Channel_Type::INSTRUMENT:
           { 
             return m_channel_handler->handle_instrument(context, buf, len);
           }
           break;
         default:
           {
             return len;
           }
       }
    }

    void init(CME_MDP_FAST_Channel_Handler* channel_handler_, channel_info* channel_info_, CME_Channel_Type::Enum channel_type_, bool record_only)
    {
      m_channel_handler = channel_handler_;
      m_channel_info = channel_info_;
      m_channel_type = channel_type_;
      m_record_only = record_only;
    }
  };

  class CME_MDP_FAST_Handler : public Handler {
  public:
    // Handler functions
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t) { return 0; }
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg);

    virtual bool sets_up_own_contexts() { return true; }

    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buffer, size_t len);

    void init(const string&, ExchangeID exch, const Parameter&);
    virtual void start();
    virtual void stop();

    boost::shared_ptr<MDOrderBookHandler> order_book() { return m_order_book; }

    //void set_order_book(boost::shared_ptr<MDOrderBookHandler> ob) { m_order_book = ob; }
    virtual void enable_hist_mode() { m_hist_mode = true; }
    void set_subscribers(boost::shared_ptr<MDSubscriptionAcceptor> s) 
    {  
      m_subscribers = s;
      for(hash_map<int, CME_MDP_FAST_Channel_Handler*>::iterator it = m_channel_handlers.begin();
          it != m_channel_handlers.end(); ++it)
      {
        it->second->set_subscribers(s);
      }
    }
    void set_provider(MDProvider* p) 
    { 
      m_provider = p; 
      for(hash_map<int, CME_MDP_FAST_Channel_Handler*>::iterator it = m_channel_handlers.begin();
          it != m_channel_handlers.end(); ++it)
      {
        it->second->set_provider(p);
      }
    }
   
    void set_qbdf_builder(QBDF_Builder* qbdf_builder)
    {
      m_qbdf_builder = qbdf_builder;
      for(hash_map<int, CME_MDP_FAST_Channel_Handler*>::iterator it = m_channel_handlers.begin();
          it != m_channel_handlers.end(); ++it)
      {
        it->second->set_qbdf_builder(qbdf_builder);
      }
    }

    CME_MDP_FAST_Channel_Handler* my_one_channel_handler()
    {
       if(m_channel_handlers.size() != 1)
       {
         throw new std::exception();
       }
       return m_channel_handlers.begin()->second;
    } 
 
    void subscribe_product(ProductID id_,ExchangeID exch_,
                           const string& mdSymbol_,const string& mdExch_);

    CME_MDP_FAST_Channel_Handler* register_channel(int channel_id)
    {
      hash_map<int, CME_MDP_FAST_Channel_Handler*>::iterator it = m_channel_handlers.find(channel_id);
      if(it == m_channel_handlers.end())
      {
        m_channel_handlers[channel_id] = new CME_MDP_FAST_Channel_Handler(channel_id, m_order_book, m_implied_order_book, m_exch, m_exch_rule_global, m_feed_rules_global, m_subscribers, m_provider, m_prod_map_global, m_qbdf_builder, m_qbdf_exch_char_global, m_instrument_logger, this);
      }
      return m_channel_handlers[channel_id];
    }

    CME_MDP_FAST_Handler(Application* app);
    virtual ~CME_MDP_FAST_Handler();

    vector<CME_MDP_FAST_Context_Info> m_context_info;

  private:
    typedef tbb::spin_mutex mutex_t;

    void set_up_channels(const Parameter& params);

    void add_order(size_t context, Time t, uint64_t order_id, ProductID id, char side, uint64_t price, uint32_t shares);
    void reduce_size(size_t context, Time t, uint64_t order_id, uint32_t shares);
    void modify_order(size_t context, Time t, uint64_t order_id, uint32_t shares, uint64_t price);

    string m_name;

    mutable mutex_t m_mutex;
    hash_map<uint64_t, ProductID> m_prod_map_global;

    Time m_midnight;

    boost::dynamic_bitset<> m_seen_product;

    ExchangeRuleRP m_exch_rule_global;
    Feed_Rules_RP m_feed_rules_global;

    MarketStatus m_market_status;

    boost::shared_ptr<MDOrderBookHandler>     m_order_book;
    boost::shared_ptr<MDOrderBookHandler>     m_implied_order_book;

    hash_map<int, CME_MDP_FAST_Channel_Handler*> m_channel_handlers;
   
    char m_qbdf_exch_char_global;
    ExchangeID m_exch;
 
    bool m_hist_mode;
    LoggerRP m_instrument_logger;
  };

}
