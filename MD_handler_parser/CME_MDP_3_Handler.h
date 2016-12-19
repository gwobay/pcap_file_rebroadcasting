#ifndef hf_core_Data_CME_MDP_3_Handler_h
#define hf_core_Data_CME_MDP_3_Handler_h

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

#include <Data/Cme_Mdp_3_Parser.h>
#include <Data/Cme_Mdp_3_Parser_v6.h>

namespace hf_core {
  using namespace hf_tools;

  class MDProvider;

  struct CME_MDP_3_Channel_Type
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
      cout << Time::current_time().to_string() << " - CME_MDP_3_Handler.h: invalid CME_MDP_3_Channel_Type string " << s << endl;
        throw new exception;
      }
    }
  };
//  void CME_MDP_3_Channel_Handler::apply_incremental_update(uint32_t security_id, uint8_t update_action, char entry_type, uint8_t price_level, double price, int32_t size, bool last_in_group, uint32_t rpt_seq)
//
  struct CmeIncrementalMessage
  {
     uint32_t security_id;
     uint8_t update_action;
     char entry_type;
     uint32_t price_level;
     double price;
     int32_t size;
     bool last_in_group;
     uint32_t rpt_seq;
     uint8_t match_event_indicator;
     uint64_t exch_time;

     CmeIncrementalMessage(uint32_t security_id_, uint8_t update_action_, char entry_type_, uint32_t price_level_, double price_, int32_t size_,
                        bool last_in_group_, uint32_t rpt_seq_, uint8_t& match_event_indicator_, uint64_t exch_time_)
       :
       security_id(security_id_),
       update_action(update_action_),
       entry_type(entry_type_),
       price_level(price_level_),
       price(price_),
       size(size_),
       last_in_group(last_in_group_),
       rpt_seq(rpt_seq_),
       match_event_indicator(match_event_indicator_),
       exch_time(exch_time_)
     {

     }
  };

  class CmeIncrementalMessageQueue
  {
  public:
    CmeIncrementalMessageQueue(size_t max_size)
      : m_max_size(max_size)
     {
     }

     void Push(uint32_t security_id_, uint8_t update_action_, char entry_type_, uint32_t price_level_, double price_, int32_t size_,
                        bool last_in_group_, uint32_t rpt_seq_, uint8_t& match_event_indicator_, uint64_t exch_time_)
     {
       CmeIncrementalMessage* msg = new CmeIncrementalMessage(security_id_, update_action_, entry_type_, price_level_, price_, size_, 
                                                              last_in_group_, rpt_seq_, match_event_indicator_, exch_time_);
       m_queue.push(msg);
       if(m_queue.size() > m_max_size)
       {
         cout << "Incremental message queue is overflowing.  Max size=" << m_max_size << endl;
         Pop();
       }
     }

     CmeIncrementalMessage* Front()
     {
       return m_queue.front();
     }

     void Pop()
     {
       CmeIncrementalMessage* front = m_queue.front();
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
    queue<CmeIncrementalMessage*> m_queue;
  };
 
  class CME_MDP_3_Channel_Handler_Base
  {
  public:
    virtual ~CME_MDP_3_Channel_Handler_Base() {}
    virtual size_t handle_incremental(size_t context, uint8_t* buf, size_t len) = 0;
    virtual size_t handle_snapshot(size_t context, uint8_t* buf, size_t len) = 0;
    virtual size_t handle_instrument(size_t context, uint8_t* buf, size_t len) = 0;
    virtual void set_subscribers(boost::shared_ptr<MDSubscriptionAcceptor> s) = 0;
    virtual void set_provider(MDProvider* p) = 0;
    virtual void set_qbdf_builder(QBDF_Builder* qbdf_builder) = 0;
    virtual bool finished_handling_instruments() = 0;
    virtual void apply_incremental_update(uint32_t security_id, uint8_t update_action, char entry_type, uint8_t price_level, double price, int32_t size, bool last_in_group, uint32_t rpt_seq, 
                                  uint8_t match_event_indicator_value, uint64_t exch_time) = 0;
    virtual void apply_daily_statistics(uint32_t security_id, char entry_type, double price, uint32_t rpt_seq, uint8_t match_event_indicator_val, uint64_t exch_time,
                                        uint32_t settle_price_type) = 0;
    virtual QBDF_Builder* get_qbdf_builder() = 0;
    virtual void AddInstrumentDefinition(uint32_t securityId, string& symbol, unsigned maturity_month_year, double displayFactor, double minPriceIncrement,
                                                  double unit_of_measure_qty, uint32_t outright_max_depth, uint32_t implied_max_depth,
                                                  uint32_t totNumReports, string& security_group, bool publish_status, uint32_t market_segment_id,
                                                  double trading_reference_price, int open_interest_qty, int cleared_volume, uint32_t settle_price_type) = 0;
    virtual void publish_current_state_to_qbdf_snap() = 0;
  };

  class CME_MDP_3_Handler;

  template<typename MDP_VERSION> 
  class CME_MDP_3_Channel_Handler : public CME_MDP_3_Channel_Handler_Base
  {
  public:
    CME_MDP_3_Channel_Handler(int channel_id, 
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
                                 bool stop_listening_to_instrument_defs_after_one_loop,
                                 LoggerRP logger,
                                 LoggerRP instrument_logger,
                                 Application* app,
                                 CME_MDP_3_Handler* handler);

    ~CME_MDP_3_Channel_Handler();

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

    bool finished_handling_instruments() { return m_finished_handling_instruments; }

    void apply_incremental_update(uint32_t security_id, uint8_t update_action, char entry_type, uint8_t price_level, double price, int32_t size, bool last_in_group, uint32_t rpt_seq, 
                                  uint8_t match_event_indicator_value, uint64_t exch_time);

    void apply_daily_statistics(uint32_t security_id, char entry_type, double price, uint32_t rpt_seq, uint8_t match_event_indicator_val, uint64_t exch_time,
                                uint32_t settle_price_type);

    QBDF_Builder* get_qbdf_builder()
    {
      return m_qbdf_builder;
    }

    void AddInstrumentDefinition(uint32_t securityId, string& symbol, unsigned maturity_month_year, double displayFactor, double minPriceIncrement,
                                                  double unit_of_measure_qty, uint32_t outright_max_depth, uint32_t implied_max_depth,
                                                  uint32_t totNumReports, string& security_group, bool publish_status, uint32_t market_segment_id,
                                                  double trading_reference_price, int open_interest_qty, int cleared_volume, uint32_t settle_price_type);

    void publish_current_state_to_qbdf_snap();

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
      int64_t sid;
      double display_factor;
      double minimum_tick_increment;
      uint32_t outright_max_depth;
      uint32_t implied_max_depth;
      int channel_id;
      uint32_t rpt_seq;
      bool needs_snapshot;
      CmeIncrementalMessageQueue incremental_message_queue;
      Book_State m_outright_state;
      Book_State m_implied_state;
      int32_t total_volume;
      Time last_unrounded_settlement_price_time;

      Security_Definition(uint32_t security_id_, 
                          string& symbol_,
                          ProductID product_id_,
                          int64_t sid_,
                          double display_factor_,
                          double minimum_tick_increment_,
                          uint32_t outright_max_depth_,
                          uint32_t implied_max_depth_,
                          int channel_id_)
        : security_id(security_id_),
          symbol(symbol_),
          product_id(product_id_),
          sid(sid_),
          display_factor(display_factor_),
          minimum_tick_increment(minimum_tick_increment_),
          outright_max_depth(outright_max_depth_),
          implied_max_depth(implied_max_depth_),
          channel_id(channel_id_),
          rpt_seq(0),
          needs_snapshot(false),
          incremental_message_queue(256*256), // arbitrarily selected, seems like plenty
          total_volume(0),
          last_unrounded_settlement_price_time(0)
      {
        m_outright_state.m_num_bids = 0;
        m_outright_state.m_num_offers = 0;
        m_implied_state.m_num_bids = 0;
        m_implied_state.m_num_offers = 0;
      }
    };

    ProductID product_id_of_symbol(string symbol_string);
    void update_quote_status(ProductID product_id, QuoteStatus quote_status, QBDFStatusType qbdf_status, string qbdf_status_reason);
    void send_quote_update();
    void send_trade_update();

    void apply_security_status(Security_Definition& sec_def, uint32_t security_status);
    template<typename T>
    void handle_security_status(T& msg);
    template<typename T>
    void handle_instrument_msg(T& msg, double unit_of_measure_qty);
    template<typename T>
    void handle_snapshot_msg(T& msg, uint32_t seqNum);

    void process_incremental(uint8_t*& buf_ptr);

    void set_is_trade_event(bool new_value);

    void clear_book(Security_Definition& sec_def);
    void clear_books_on_this_channel();

    template<bool IS_IMPLIED>
    inline void qbdf_update_level(ProductID product_id, char side, uint32_t shares, MDPrice md_price, bool last_in_group, bool is_snapshot, bool is_trade_event, uint64_t exch_time);

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
    LoggerRP m_instrument_logger;
    bool m_finished_handling_instruments;
    bool m_stop_listening_to_instrument_defs_after_one_loop;
    LoggerRP m_logger;
    Application* m_app;
    CME_MDP_3_Handler* m_handler;
    uint32_t m_num_symbols_snapshotted;
    uint32_t m_num_valid_products; //ToDo: put all the vars pertaining to snapshot synchronization into their own little struct to make things clearer?

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;
    QBDFStatusType m_qbdf_status;
    string m_qbdf_status_reason;
    bool m_is_trade_event;

    hash_map<uint32_t, Security_Definition> m_security_definitions;
    //vector<MarketStatus> m_security_status;
  };

  struct CME_MDP_3_Context_Info
  {
    CME_MDP_3_Channel_Handler_Base* m_channel_handler;
    channel_info* m_channel_info;
    CME_MDP_3_Channel_Type::Enum m_channel_type;
    bool m_record_only;

    size_t handle(size_t context, const char* bufC, size_t len)
    {
       uint8_t* buf = reinterpret_cast<uint8_t*>(const_cast<char*>(bufC));
       switch(m_channel_type)
       {
         case CME_MDP_3_Channel_Type::INCREMENTAL:
           {
             if(!m_record_only)
             {
               return m_channel_handler->handle_incremental(context, buf, len);
             }
           }
           break;
         case CME_MDP_3_Channel_Type::SNAPSHOT:
           { 
             if(!m_record_only)
             {
               return m_channel_handler->handle_snapshot(context, buf, len);
             }
           }
           break;
         case CME_MDP_3_Channel_Type::INSTRUMENT:
           { 
             return m_channel_handler->handle_instrument(context, buf, len);
           }
           break;
         default:
           {
             return len;
           }
       }
       return len;
    }

    void init(CME_MDP_3_Channel_Handler_Base* channel_handler_, channel_info* channel_info_, CME_MDP_3_Channel_Type::Enum channel_type_, bool record_only)
    {
      m_channel_handler = channel_handler_;
      m_channel_info = channel_info_;
      m_channel_type = channel_type_;
      m_record_only = record_only;
    }
  };


  class CME_MDP_3_Handler : public Handler {
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
      for(hash_map<int, CME_MDP_3_Channel_Handler_Base*>::iterator it = m_channel_handlers.begin();
          it != m_channel_handlers.end(); ++it)
      {
        it->second->set_subscribers(s);
      }
    }
    void set_provider(MDProvider* p) 
    { 
      m_provider = p; 
      for(hash_map<int, CME_MDP_3_Channel_Handler_Base*>::iterator it = m_channel_handlers.begin();
          it != m_channel_handlers.end(); ++it)
      {
        it->second->set_provider(p);
      }
    }
   
    void set_qbdf_builder(QBDF_Builder* qbdf_builder)
    {
      m_qbdf_builder = qbdf_builder;
      for(hash_map<int, CME_MDP_3_Channel_Handler_Base*>::iterator it = m_channel_handlers.begin();
          it != m_channel_handlers.end(); ++it)
      {
        it->second->set_qbdf_builder(qbdf_builder);
      }
    }
 
    void subscribe_product(ProductID id_,ExchangeID exch_,
                           const string& mdSymbol_,const string& mdExch_);

    CME_MDP_3_Channel_Handler_Base* register_channel(int channel_id, int mdp_version)
    {
      hash_map<int, CME_MDP_3_Channel_Handler_Base*>::iterator it = m_channel_handlers.find(channel_id);
      if(it == m_channel_handlers.end())
      {
        switch(mdp_version)
        {
        case 5:
          {
            boost::shared_ptr<CME_MDP_3_Channel_Handler_Base> channel_handler(new CME_MDP_3_Channel_Handler<CME_MDP_3_v5>(channel_id, m_order_book, m_implied_order_book, m_exch, m_exch_rule_global, m_feed_rules_global, m_subscribers, m_provider, m_prod_map_global, m_qbdf_builder, m_qbdf_exch_char_global, m_stop_listening_to_instrument_defs_after_one_loop, m_logger, m_instrument_logger, m_app, this));
            m_channel_handlers_list.push_back(channel_handler);
            m_channel_handlers[channel_id] = channel_handler.get();
          }
          break;
        case 6:
          {
            boost::shared_ptr<CME_MDP_3_Channel_Handler_Base> channel_handler(new CME_MDP_3_Channel_Handler<CME_MDP_3_v6>(channel_id, m_order_book, m_implied_order_book, m_exch, m_exch_rule_global, m_feed_rules_global, m_subscribers, m_provider, m_prod_map_global, m_qbdf_builder, m_qbdf_exch_char_global, m_stop_listening_to_instrument_defs_after_one_loop, m_logger, m_instrument_logger, m_app, this));
            m_channel_handlers_list.push_back(channel_handler);
            m_channel_handlers[channel_id] = channel_handler.get();
          }
          break;
        default:
          {
            cout << "Unrecognized mdp_version " << mdp_version << " - cannot continue" << endl;
            throw new exception();
          }
          break;
        }
      }
      return m_channel_handlers[channel_id];
    }

    CME_MDP_3_Handler(Application* app);
    virtual ~CME_MDP_3_Handler();

    vector<CME_MDP_3_Context_Info> m_context_info;

    CME_MDP_3_Channel_Handler_Base* my_one_channel_handler()
    {
       if(m_channel_handlers.size() != 1)
       {
         throw new std::exception();
       }
       return m_channel_handlers.begin()->second;
    }

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

    bool m_stop_listening_to_instrument_defs_after_one_loop;
    LoggerRP m_instrument_logger;

    hash_map<int, CME_MDP_3_Channel_Handler_Base*> m_channel_handlers;
    list<boost::shared_ptr<CME_MDP_3_Channel_Handler_Base> > m_channel_handlers_list;  
 
    char m_qbdf_exch_char_global;
    ExchangeID m_exch;
 
    bool m_hist_mode;
  };

}

#endif /* ifndef hf_core_Data_CME_MDP_3_Handler_h */
