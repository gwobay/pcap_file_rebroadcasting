#ifndef hf_core_Data_CHIX_MMD_Handler_h
#define hf_core_Data_CHIX_MMD_Handler_h

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

  class CHIX_MMD_Handler : public Handler {
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
    
    virtual void enable_hist_mode() { m_hist_mode = true; }
    
    void subscribe_product(ProductID id_,ExchangeID exch_,
                           const string& mdSymbol_,const string& mdExch_);

    CHIX_MMD_Handler(Application* app);
    virtual ~CHIX_MMD_Handler();

  private:
    typedef tbb::spin_mutex mutex_t;
    ProductID product_id_of_symbol(const char* symbol, int len);

    void add_order(size_t context, Time t, uint64_t order_id, ProductID id, char side, uint64_t price, uint32_t shares);
    void reduce_size(size_t context, Time t, uint64_t order_id, uint32_t shares);
    void modify_order(size_t context, Time t, uint64_t order_id, uint32_t shares, uint64_t price);

    void send_quote_update(); // fill out time & id and this does the rest
    void send_trade_update(); // fill out time, id, shares and price

    string m_name;

    mutable mutex_t m_mutex;
    hash_map<uint64_t, ProductID> m_prod_map;

    Time m_midnight;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;
    QBDFStatusType m_qbdf_status;
    string m_qbdf_status_reason;
    char m_qbdf_exch_char;

    vector<MarketStatus> m_security_status;
    boost::dynamic_bitset<> m_seen_product;

    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;

    MarketStatus m_market_status;

    boost::shared_ptr<MDOrderBookHandler>     m_order_book;
    
    bool m_hist_mode;
  };

}

#endif /* ifndef hf_core_Data_CHIX_MMD_Handler_h */
