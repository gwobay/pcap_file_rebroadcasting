#ifndef hf_core_Data_LSE_Millennium_Handler_h
#define hf_core_Data_LSE_Millennium_Handler_h

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

#include <boost/dynamic_bitset.hpp>

namespace hf_core {
  using namespace hf_tools;

  class MDProvider;

  class LSE_Millennium_Handler : public Handler {
  public:
    // Handler functions
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t);
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg);
    
    virtual void send_retransmit_request(channel_info& b);

    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);
    
    void init(const string&, ExchangeID exch, const Parameter&);
    virtual void start();
    virtual void stop();
    
    void subscribe_product(ProductID id_,ExchangeID exch_,
                           const string& mdSymbol_,const string& mdExch_);

    LSE_Millennium_Handler(Application* app);
    virtual ~LSE_Millennium_Handler();

  private:
    typedef tbb::spin_mutex mutex_t;

    ProductID product_id_of_symbol(uint32_t instrument_id);

    void add_order(size_t context, Time t, uint64_t order_id, ProductID id, char side, MDPrice price, uint32_t shares);
    void modify_order(size_t context, Time t, uint64_t order_id, uint32_t shares, MDPrice price);

    void send_quote_update(); // fill out time & id and this does the rest
    void send_trade_update(); // fill out time, id, shares and price

    string m_name;

    mutable mutex_t m_mutex;
    hash_map<uint32_t, ProductID> m_prod_map;

    Time m_midnight;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;

    vector<MarketStatus> m_security_status;
    boost::dynamic_bitset<> m_seen_product;
    
    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;

    MarketStatus m_market_status;

    boost::shared_ptr<MDOrderBookHandler>     m_order_book;
    
    uint64_t m_micros_exch_time_marker;
  };

}

#endif /* ifndef hf_core_Data_LSE_Millennium_Handler_h */
