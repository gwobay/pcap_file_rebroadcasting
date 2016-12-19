#pragma once

#include <Data/QBDF_Util.h>
#include <Data/Handler.h>
#include <MD/Quote.h>
#include <MD/MDSubscriptionAcceptor.h>
#include <MD/MDProvider.h>
#include <Data/Eurex_Message_Types.h>
#include <boost/foreach.hpp>

namespace hf_core {

  namespace Eurex {
    struct MessageHeader {
      uint16_t BodyLen;
      Eurex::EnumMessageType::MessageType TemplateID:16;
      uint32_t MsgSeqNum;
    } __attribute__((packed));

    struct OrderIDKey {
      uint64_t eurex_time_priority;
       int64_t security_id;
          char side;

      OrderIDKey(uint64_t ord_id, int64_t sec_id, char s) :
        eurex_time_priority(ord_id), security_id(sec_id), side(s)
      { }


      bool operator==( const OrderIDKey& x ) const
      {
        return (eurex_time_priority == x.eurex_time_priority && security_id == x.security_id && side == x.side);
      }

    };
    typedef hash_map<OrderIDKey, uint64_t> order_id_hash_table;
  }
}

namespace __gnu_cxx
{
  template<> struct hash< hf_core::Eurex::OrderIDKey >
  {
    std::size_t operator()( const hf_core::Eurex::OrderIDKey& x ) const
    {
      std::size_t seed = 0;
      boost::hash_combine(seed, x.eurex_time_priority);
      boost::hash_combine(seed, x.security_id);
      boost::hash_combine(seed, x.side);

      return seed;
    }
  };
}

namespace hf_core {



  class Eurex_Handler : public Handler {

  public:
    virtual void reset(const char* msg);
    virtual size_t read(size_t context, const char* buf, size_t len) { return 0; }
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t context, const char* buf, size_t len);
    virtual void subscribe_product(hf_core::ProductID, hf_core::ExchangeID, const string&, const string&);

    Eurex_Handler(Application* app);
    virtual ~Eurex_Handler() { }

    void set_ref_data_mode() {
      // m_fast_decoder.reset(new Eurex::Eurex_FAST_Decoder11());
      m_ref_data_mode = true;
    }

    bool get_ref_data_mode() { return m_ref_data_mode; }

    void init(const string& name, ExchangeID exch, const Parameter& params);
    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);

  private:
    std::string m_name;
    const char m_qbdf_exch_char;

    bool m_ref_data_mode;
    // boost::shared_ptr<Eurex::Eurex_FAST_Decoder11> m_fast_decoder;

    size_t parse_ref_data(size_t context, uint8_t *buf, size_t len);

    bool m_last_packet_complete; // whether the CompletionIndicator of the last PacketHeader was 1 (=complete)
                                 // if we are in the middle of a "snapshot cycle" then this will be 0

    uint64_t m_transact_time;
    Time m_midnight;
    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    MarketStatusUpdate m_msu;
    ExchangeID m_exch;

    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;
    vector<MarketStatus> m_security_status;

    boost::shared_ptr<MDOrderBookHandler> m_order_book;

    ProductID product_id_of_symbol(int64_t symbol);

    hash_map<int64_t, ProductID> m_eurex_id_to_product_id;

    Eurex::order_id_hash_table m_order_id_hash;
    uint64_t m_order_id;
    uint64_t get_unique_order_id(uint64_t eurex_time_priority, int64_t security_id, char side);
    void set_times(uint64_t exch_time_nanos);

    int32_t m_market_segment_id;
    std::map<int32_t, std::set<ProductID> > m_MarketSegmentID_ProductID; // TODO: populate me

    void send_quote_update();
    void send_trade_update();
    void modify_order(size_t context, uint64_t exch_time_nanos, uint64_t security_id, Eurex::EnumSide::Side s, uint64_t existing_time_prio, uint64_t new_time_prio, int64_t px, int32_t shares, channel_info& ci);
  };
}
