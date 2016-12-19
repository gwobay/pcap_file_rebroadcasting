#ifndef hf_core_Data_TMX_TL1_Handler_h
#define hf_core_Data_TMX_TL1_Handler_h

#include <Data/Handler.h>
#include <Data/MDBufferQueue.h>
#include <Data/QBDF_Builder.h>
#include <MD/MDSubscriptionAcceptor.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Interface/AlertEvent.h>

#include <Logger/Logger.h>
#include <Util/Time.h>

#include <boost/dynamic_bitset.hpp>

namespace hf_core {
  using namespace hf_tools;
  
  class TMX_TL1_Handler : public Handler {
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

    TMX_TL1_Handler(Application* app);
    virtual ~TMX_TL1_Handler();

  private:
    typedef tbb::spin_mutex mutex_t;
    ProductID product_id_of_symbol(const char* symbol, int len);

    void add_order(size_t context, Time t, uint64_t order_id, ProductID id, char side, uint64_t price, uint32_t shares);
    void reduce_size(size_t context, Time t, uint64_t order_id, uint32_t shares);
    void modify_order(size_t context, Time t, uint64_t order_id, uint32_t shares, uint64_t price);

    inline uint64_t getUniqueOrderId(uint64_t exchangeOrderId, uint16_t productId, uint16_t brokerId)
    {
      uint64_t datePortion = static_cast<uint64_t>(exchangeOrderId / 1000000000ULL);
      hash_map<uint64_t, uint64_t>::iterator it = m_date_id_map.find(datePortion);
      if(it == m_date_id_map.end())
      {
        m_date_id_map[datePortion] = m_next_date_id;
        ++m_next_date_id;
        it = m_date_id_map.find(datePortion);
      }
      const uint64_t dateIndex = it->second;                         // 8 bits for date       (max 255 distinct dates)
      const uint64_t orderNumber = (exchangeOrderId % 1000000000ULL) & 0x3FFFFFFF;     // 30 bits for number    (max 1,073,741,823 order numbers, which is less than the exchange's max of 999,999,999)
      const uint64_t productId64 = static_cast<uint64_t>(productId); // 14 bits for productId (max of 16,383 [~5K in universe as of 20140718])
      const uint64_t brokerId64 = static_cast<uint64_t>(brokerId);   // 12 bits for brokerId  (max of 4,095 [See up to 124 as of 20140718])

      // Can do these for safety:
      if (dateIndex > 255ULL || 
          productId64 > 8191ULL ||
          brokerId64 > 16383ULL)
      {
        std::cout << "Problem: Cannot get unique order ID because inputs violate our constraints: dateIndex=" << dateIndex << " productId=" << productId64 << " brokerId=" << brokerId64 << endl;
        return 0;
      }
      return (dateIndex << 56) | (orderNumber << 26) | (productId64 << 12) | (brokerId64);
    }

    inline uint16_t getBoardLotSize(ProductID productId, MDPrice refPrice)
    {
      hash_map<ProductID, uint16_t>::iterator it = product_id_to_board_lot_size.find(productId);
      if(it != product_id_to_board_lot_size.end())
      {
        return it->second;
      }
      if(refPrice.fprice() < .095)
      {
        return 1000;
      }
      if(refPrice.fprice() < 1.0)
      {
        return 500;
      }
      return 100;
    }

    void publishImbalance(ProductID productId, Time& exchTimestamp, MDPrice& copPrice, uint32_t totalBuySize, uint32_t totalSellSize);

    string m_name;

    Time m_last_alert_time;
    Time_Duration m_alert_frequency;
    AlertSeverity m_alert_severity;

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

    struct MarketStatusState
    {
      MarketStatus groupStatus;
      MarketStatus stockStatusOverride;
      bool hasStockStatusOverride;

      MarketStatusState()
        : hasStockStatusOverride(false)
      { }
    };

    vector<MarketStatus> m_security_status;
    boost::dynamic_bitset<> m_seen_product;

    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;

    MarketStatus m_market_status;
    
    hash_map<uint64_t, uint64_t> m_date_id_map;
    bool m_hist_mode;
    uint64_t m_next_date_id;

    // ToDo: consolidate some of these product ID-keyed maps, perhaps use an array since product IDs are 1,...,n
    hash_map<uint8_t, vector<ProductID> > stock_group_to_product_ids;
    hash_map<ProductID, uint8_t> product_id_to_stock_group;
    hash_map<ProductID, char> product_ids_in_auction_state;
    hash_map<ProductID, uint16_t> product_id_to_board_lot_size;
    hash_map<ProductID, bool> product_id_to_imbalance_state;

    unsigned m_number_of_terms_orders_seen;
    
    LoggerRP m_stock_group_logger;
  };

}

#endif /* ifndef hf_core_Data_TMX_TL1_Handler_h */
