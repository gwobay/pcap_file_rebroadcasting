#ifndef hf_core_Data_CQS_CTS_Handler_h
#define hf_core_Data_CQS_CTS_Handler_h

#include <Data/Handler.h>
#include <Data/MDBufferQueue.h>
#include <Data/QBDF_Builder.h>
#include <MD/MDSubscriptionAcceptor.h>

#include <Exchange/ExchangeRule.h>
#include <Exchange/Feed_Rules.h>

#include <Interface/AlertEvent.h>

#include <Logger/Logger.h>
#include <Util/Time.h>

#include <boost/scoped_ptr.hpp>

#include <ext/hash_set>

namespace hf_core {

  using namespace hf_tools;
  using namespace hf_core;

  class MDProvider;

  class CQS_CTS_Handler : public Handler {
  public:

    //Handler functions
    virtual const string& name() const { return m_name; }

    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t, const char* buf, size_t len);
    virtual size_t read(size_t, const char*, size_t) { return 0; }
    virtual void reset(const char* msg);
    virtual void reset(size_t context, const char* msg);

    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);

    virtual void init(const string&,const Parameter&);
    virtual void start(void);
    virtual void stop(void);
    virtual void subscribe_product(ProductID id_,ExchangeID exch_,
				   const string& mdSymbol_,const string& mdExch_);
    
    CQS_CTS_Handler(Application* app);
    virtual ~CQS_CTS_Handler();

    struct channel_info_t : public channel_info {
      bool cqs;

      channel_info_t(const string& name, size_t context, bool cqs) :
        channel_info(name, context),
        cqs(cqs)
      { }
    };

    typedef vector<channel_info_t> channel_info_map_t;
    channel_info_map_t m_channel_info_map;

  private:
    void send_quote_update();
    void send_trade_update();

    struct eqstr{
      bool operator()(const char* s1, const char* s2) const {
        return strcmp(s1, s2) == 0;
      }
    };

    struct symbol_info_t {
      ProductID id;
      ExchangeID exch;

      symbol_info_t(ProductID i, ExchangeID e) : id(i), exch(e) { }
    };

    typedef tbb::spin_mutex mutex_t;
    typedef hash_map<const char*, symbol_info_t, __gnu_cxx::hash<const char *>, eqstr> symbol_map_t;

    symbol_info_t product_id_of_symbol(char participant_id, const char* symbol, int len);

    string m_name;

    mutable mutex_t m_mutex;

    vector<symbol_map_t*> m_symbol_maps; // indexed by participant ID

    set<string> m_symbols;

    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_msu;

    vector<MarketStatus> m_security_status;

    vector<int> m_exch_lot_size;

    Feed_Rules_RP m_feed_rules;
  };

}

#endif /* ifndef hf_core_Data_CQS_CTS_Handler_h */
