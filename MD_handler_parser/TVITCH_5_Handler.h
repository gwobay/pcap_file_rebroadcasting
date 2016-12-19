#ifndef hf_core_Data_TVITCH_5_Handler_h
#define hf_core_Data_TVITCH_5_Handler_h

#include <Data/Handler.h>
#include <Data/QBDF_Builder.h>
#include <MD/MDOrderBook.h>
#include <MD/MDSubscriptionAcceptor.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Logger/Logger.h>
#include <Util/Time.h>
#include <Util/Symbol_Lookup.h>

namespace hf_core {
  using namespace hf_tools;

  class TVITCH_5_Handler : public Handler {
  public:

    // Handler functions
    virtual const string& name() const { return m_name; }
    
    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t, const char* buf, size_t len);
    size_t  parse_message(size_t, const char* buf, size_t len);
        
    virtual size_t read(size_t, const char*, size_t) { return 0; }
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg, uint64_t expected_seq_no, uint64_t received_seq_no);
    virtual void send_retransmit_request(channel_info& channel_info);
    virtual void disable_reordering() { m_channel_info.has_reordering = false; } 
    
    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);
    static size_t dump_message(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);

    void init(const string&, ExchangeID exch, const Parameter&);
    virtual void start();
    virtual void stop();
    virtual void enable_hist_mode() { m_hist_mode = true; }

    void subscribe_product(ProductID id_,ExchangeID exch_,
                           const string& mdSymbol_,const string& mdExch_);

    TVITCH_5_Handler(Application* app);
    virtual ~TVITCH_5_Handler();

  private:
    typedef tbb::spin_mutex mutex_t;
    ProductID product_id_of_symbol(const char* symbol);

    void send_quote_update(); // fill out time & id and this does the rest
    void send_trade_update(); // fill out time, id, shares and price

    string       m_name;

  public:
    channel_info m_channel_info;

  private:
    mutable mutex_t m_mutex;
    Symbol_Lookup8  m_prod_map;
    
    Time m_midnight;
    uint64_t m_micros_exch_time_marker;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;
    QBDFStatusType m_qbdf_status;
    string m_qbdf_status_reason;
    char m_exch_char;
    bool m_trade_has_no_side;
    
    vector<MarketStatus> m_security_status;
    
    vector<ProductID>    m_stock_to_product_id;
    
    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;
    
    MarketStatus m_market_status;
    
    boost::shared_ptr<MDOrderBookHandler>     m_order_book;
    
    bool m_hist_mode;
    
    char m_session[10];
    
  };

}

#endif /* ifndef hf_core_Data_TVITCH_5_Handler_h */