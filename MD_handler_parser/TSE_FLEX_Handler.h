#ifndef hf_core_Data_TSE_FLEX_Handler_h
#define hf_core_Data_TSE_FLEX_Handler_h

#include <Data/Handler.h>
#include <Data/MDBufferQueue.h>
#include <Data/QBDF_Builder.h>
#include <MD/MDSubscriptionAcceptor.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Interface/AlertEvent.h>

#include <Logger/Logger.h>
#include <Util/Time.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  
  struct TradeState {
    uint16_t id;
    char exch;
    uint64_t recv_time;
    uint64_t exch_time;
    char side;
    uint32_t size;
    MDPrice price;
  };

  struct bp_data_struct {
    ProductID id;
    uint16_t  stock_code;
    string    applicable_date;
    uint8_t   tick_size_table;
    MDPrice   base_price;
    MDPrice   limit_max_price;
    MDPrice   limit_min_price;

  bp_data_struct(ProductID i, uint16_t sc, string ad, uint8_t tst, MDPrice bp, MDPrice lp_max, MDPrice lp_min) :
    id(i), stock_code(sc), applicable_date(ad), tick_size_table(tst), base_price(bp), limit_max_price(lp_max), limit_min_price(lp_min)
    {}
  };

  struct ii_data_struct {
    ProductID id;
    uint16_t  stock_code;
    string    applicable_date;
    uint32_t  trading_unit;

  ii_data_struct(ProductID i, uint16_t sc, string ad, uint32_t tu) : id(i), stock_code(sc), applicable_date(ad), trading_unit(tu)
    {}
  };


  class TSE_FLEX_Handler : public Handler {
  public:

    virtual const string& name() const { return m_name; }

    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t context, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t) { return 0; }
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg);

    virtual void init(const string&, ExchangeID, const Parameter&);
    virtual void start(void);
    virtual void stop(void);

    boost::shared_ptr<MDOrderBookHandler> order_book() { return m_order_book; }
    boost::shared_ptr<MDOrderBookHandler> auction_order_book() { return m_auction_order_book; }

    void subscribe_product(ProductID id_,ExchangeID exch_,
                           const string& mdSymbol_,const string& mdExch_);

    ProductID product_id_of_symbol(int symbol);
    
    void write_morning_closes();
    void write_afternoon_closes();
    void write_special_quotes();
    void write_bp_data();
    void write_ii_data();

    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);

    TSE_FLEX_Handler(Application* app);
    virtual ~TSE_FLEX_Handler();

    typedef channel_info channel_info_t;
    typedef vector<channel_info_t> channel_info_map_t;
    channel_info_map_t m_channel_info_map;

  private:
    typedef tbb::spin_mutex mutex_t;

    string m_name;
    mutable mutex_t m_mutex;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;

    vector<MarketStatus> m_security_status;

    vector<int> m_exch_lot_size;

    Feed_Rules_RP m_feed_rules;

    boost::shared_ptr<MDOrderBookHandler>     m_order_book;
    boost::shared_ptr<MDOrderBookHandler>     m_auction_order_book;
    
    LoggerRP m_quote_dump;
    vector<MDPrice> m_current_price;
    vector<uint64_t> m_trading_volume;
    Time m_midnight;
    uint64_t m_micros_midnight;
    uint64_t m_micros_morning_close;
    uint64_t m_micros_afternoon_close;
    bool m_use_record_time;

    map<int, ProductID> m_symbol_to_id;

    bool m_morning_closes_written;
    bool m_afternoon_closes_written;
    vector<TradeState> m_morning_close_data;
    vector<TradeState> m_afternoon_close_data;
    vector<TradeState> m_special_quote_data;
    vector<bp_data_struct> m_bp_data;
    vector<ii_data_struct> m_ii_data;

    string m_root_output_path;
    string m_date;
    bool   m_is_standard_feed;
  };

}

#endif /* ifndef hf_core_Data_TSE_FLEX_Handler_h */
