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

#include <boost/dynamic_bitset.hpp>

namespace hf_core {
  using namespace hf_tools;

  class NXT_Book_Handler;

  inline static Time_Duration
  convert_timestamp(uint32_t t)
  {
    return Time_Duration(uint64_t(t) * 25UL * Time_Constants::ticks_per_micro);
  }

  inline static uint64_t
  convert_micros(uint32_t t)
  {
    return uint64_t(t) * 25UL;
  }


  namespace nxt_book {
    struct publisher_info {
      uint8_t sender_id;
      uint8_t priority;
      uint8_t major_data_version;
      uint8_t minor_data_version;
      uint32_t tcp_service_ip;
      uint16_t tcp_service_port;

      bool operator<(const publisher_info& t) const {
        return priority < t.priority;
      }
    };

    struct multicast_packet_header_common {
      uint8_t   protocol_version;
      uint8_t   sender_id;
      uint16_t  reserved01;
      uint32_t  sequence_number;
    } __attribute__ ((packed));

    struct multicast_packet_header_v01 {
      uint8_t   protocol_version;
      uint8_t   sender_id;
      uint8_t   fragment_indicator;
      uint8_t   message_count;
      uint32_t  sequence_number;

      void dump(ILogger& log, Dump_Filters& filter) const {
        log.printf(" packet version:%d  sender_id:%d  fragment_indicator:%d  message_count:%d  sequence_number:%d\n",
                   protocol_version, sender_id, fragment_indicator, message_count, sequence_number);
      }
    } __attribute__ ((packed));

    struct multicast_packet_header_v06 {
      uint8_t   protocol_version;
      uint8_t   sender_id;
      uint16_t  packet_size;
      uint32_t  sequence_number;
      char      packet_type;

      void dump(ILogger& log, Dump_Filters& filter) const {
        log.printf(" packet version:%d  sender_id:%d  packet_size:%d  sequence_number:%d  packet_type:%c\n",
                   protocol_version, sender_id, packet_size, sequence_number, packet_type);
      }
    } __attribute__ ((packed));


    struct header_only {
      uint8_t   message_type;
      uint32_t  message_size;
    } __attribute__ ((packed));

    struct header_w_symbol_id_only {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  symbol_id;
    } __attribute__ ((packed));

    struct header_w_symbol_seq {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  symbol_id;
      uint32_t  symbol_seq_number;
    } __attribute__ ((packed));

    struct heartbeat_v01 {
      uint8_t   message_type;
      uint32_t  message_size;
      uint8_t   priority;
      uint8_t   major_data_version;
      uint8_t   minor_data_version;
      uint16_t  heartbeat_interval;
      uint32_t  tcp_service_ip;
      uint16_t  tcp_service_port;

      void dump(ILogger& log, Dump_Filters& filter) const {
        union { uint8_t quad[4]; uint32_t i; }  host_i;
        host_i.i = tcp_service_ip;
        char host[32];
        snprintf(host, 32, "%d.%d.%d.%d", host_i.quad[0], host_i.quad[1], host_i.quad[2], host_i.quad[3]);
        log.printf("  heartbeat  priority:%d  major:%d  minor:%d  interval:%d  tcp_service_ip:%s  tcp_service_port:%d\n",
                   priority, major_data_version, minor_data_version, heartbeat_interval, host, tcp_service_port);
      }
    } __attribute__ ((packed));


    struct heartbeat {
      uint8_t   priority;
      uint8_t   major_data_version;
      uint8_t   minor_data_version;
      uint16_t  heartbeat_interval;
      uint32_t  tcp_service_ip;
      uint16_t  tcp_service_port;

      void dump(ILogger& log, Dump_Filters& filter) const {
        union { uint8_t quad[4]; uint32_t i; }  host_i;
        host_i.i = tcp_service_ip;
        char host[32];
        snprintf(host, 32, "%d.%d.%d.%d", host_i.quad[0], host_i.quad[1], host_i.quad[2], host_i.quad[3]);
        log.printf("  heartbeat  priority:%d  major:%d  minor:%d  interval:%d  tcp_service_ip:%s  tcp_service_port:%d\n",
                   priority, major_data_version, minor_data_version, heartbeat_interval, host, tcp_service_port);
      }

    } __attribute__ ((packed));


    struct data_packet {
      uint8_t   fragment_indicator;
      uint8_t   message_count;
    } __attribute__ ((packed));

    struct client_hello {
      uint8_t   message_type;
      uint32_t  message_size;
      char      client_id[15];
    } __attribute__ ((packed));

    struct get_symbol_id {
      uint8_t   message_type;
      uint32_t  message_size;
      char      symbol[31];

      void dump(ILogger& log, Dump_Filters& filter) const {
        log.printf("get_symbol_id  symbol:%.31s\n", symbol);
      }
    } __attribute__ ((packed));

    struct subscription {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  symbol_id;
      uint8_t   md_type;
      char      client_id[15];
      uint32_t  client_ip;
      char      password[11];
    } __attribute__ ((packed));

    struct packet_recovery_request {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  packet_sequence_number;
      uint16_t  packet_count;
    } __attribute__ ((packed));

    struct packet_recovery_response {
      uint8_t   message_type;
      uint32_t  message_size;
      uint16_t  packet_count;
    } __attribute__ ((packed));

    struct notification {
      uint8_t   message_type;
      uint32_t  message_size;
      uint8_t   notification;
    } __attribute__ ((packed));

    struct symbol_id {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  symbol_id;
      uint8_t   status;
      char      symbol[31];

      void dump(ILogger& log, Dump_Filters& filter) const {
        log.printf("symbol_id  symbol:%.31s  symbol_id:%u  status:%d\n", symbol, symbol_id, status);
      }
    } __attribute__ ((packed));

    struct price_book {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  symbol_id;
      uint32_t  symbol_seq_number;
      uint8_t   symbol_status;
      uint16_t  pl_count;
    } __attribute__ ((packed));

    struct price_book_bid_ask {
      uint8_t   side;
      double    price;
      uint32_t  size;
      uint32_t  timestamp;
      uint16_t  pl_order_count;
      uint16_t  attached_order_count;
    } __attribute__ ((packed));

    struct attached_order {
      uint64_t order_id;
      uint32_t order_size;
      uint32_t order_timestamp;
    } __attribute__ ((packed));

    struct trade {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  symbol_id;
      uint32_t  symbol_seq_number;
      uint8_t   symbol_status;
      double    price;
      double    high_price;
      double    low_price;
      uint64_t  trade_id;
      uint64_t  order_id;

      struct TradeFlags
      {
        unsigned OffExchange :1;
        unsigned OffBook :1;
        unsigned Hidden :1;
        unsigned Matched :1;
        unsigned UndefinedAuction :1;
        unsigned OpeningAuction :1;
        unsigned ClosingAuction :1;
        unsigned IntradayAuction :1;
        unsigned UnscheduledAuction :1;
        unsigned Cross :1;
        unsigned Dark :1;
        unsigned Cancel :1;
        unsigned OTC :1;
        unsigned AtMarketClose :1;
        unsigned OutOfMainSession :1;
        unsigned TradeThroughExempt :1;
        unsigned Acquisition :1;
        unsigned Bunched :1;
        unsigned Cash :1;
        unsigned Distribution :1;
        unsigned IntermarketSweep :1;
        unsigned BunchedSold :1;
        unsigned OddLot :1;
        unsigned PriceVariation :1;
        unsigned Rule127_155 :1;
        unsigned SoldLast :1;
        unsigned NextDay :1;
        unsigned MCOpen :1;
        unsigned OpeningPrint :1;
        unsigned MCClose :1;
        unsigned ClosingPrint :1;
        unsigned CorrectedConsClose :1;
        unsigned PriorRefPrice :1;
        unsigned Seller :1;
        unsigned Split :1;
        unsigned FormT :1;
        unsigned ExtOutOfSeq :1;
        unsigned StockOption :1;
        unsigned AvgPrice :1;
        unsigned YellowFlag :1;
        unsigned SoldOutOfSeq :1;
        unsigned StoppedStock :1;
        unsigned DerivPriced :1;
        unsigned MCReOpen :1;
        unsigned AutoExec :1;
        unsigned Unused :19;

        void print(ILogger& log) const;
      } __attribute__((packed))  trade_flags;

      uint32_t  cumulative_volume;
      uint32_t  trade_size;
      uint32_t  trade_timestamp;
      uint8_t   side;
    } __attribute__ ((packed));

    struct imbalance {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  symbol_id;
      uint32_t  symbol_seq_number;
      uint8_t   symbol_status;
      uint64_t  imbalance_volume;
      uint64_t  match_volume;
      double    ref_price;
      double    near_price;
      double    far_price;
      uint32_t  timestamp;
      uint8_t   imbalance_status;
      char      variance;

      void dump(ILogger& log, Dump_Filters& filter) const {
        char ts[64];
        Time_Utils::print_time(ts, convert_timestamp(timestamp), Timestamp::MICRO);
        log.printf("  imbalance  symbol_id:%d  symbol_seq_number:%u  symbol_status:%d  imbalance_volume:%lu  match_volume:%lu  ref_price:%0.4f  near_price:%0.4f  far_price:%0.4f  timestamp:%s  imbalance_status:%u  variance:%u\n",
                   symbol_id, symbol_seq_number, symbol_status, imbalance_volume, match_volume, ref_price, near_price, far_price, ts, imbalance_status, variance);
      }

    } __attribute__ ((packed));

    struct trading_status {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  symbol_id;
      uint32_t  symbol_seq_number;
      uint8_t   symbol_status;
      uint8_t   trading_phase;
      uint32_t  short_sale_restriction : 1;
      uint32_t  padding : 31;
      uint32_t  timestamp;

      void dump(ILogger& log, Dump_Filters& filter) const;

    } __attribute__ ((packed));

    struct status_message {
      uint8_t   message_type;
      uint32_t  message_size;
      uint32_t  symbol_id;
      uint32_t  symbol_seq_number;
      uint8_t   symbol_status;

      void dump(ILogger& log, Dump_Filters& filter) const;
    } __attribute__ ((packed));
  }

  class NXT_Book_Shared_TCP_Handler: public Handler {
  public:
    struct pending_write {
      string data;
      int already_written;

      pending_write(const char* buf, size_t len, int already_written) :
        data(buf + already_written, len - already_written),
        already_written(0)
      {
      }
    };
    
    struct publisher_t : public nxt_book::publisher_info {
      publisher_t(const nxt_book::publisher_info p) :
        nxt_book::publisher_info(p),
        connection_state(0),
        writes_pending(false)
      {
      }
      
      uint8_t connection_state;
      // 0 - not attempted
      // 1 - connecting
      // 2 - connected
      // 3 - error

      socket_fd tcp_fd;
      
      hash_map<string,   NXT_Book_Handler*> symbol_to_handler;
      hash_map<uint32_t, NXT_Book_Handler*> symbol_id_to_handler;

      bool                  writes_pending;
      deque<pending_write>  pending_writes;
    };
    
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len) { return 0; }
    virtual size_t parse2(size_t, const char* buf, size_t len) { return 0;}
    virtual void reset(const char* msg) { }
    virtual void reset(size_t context, const char* msg) { }
    virtual void subscribe_product(ProductID id_, ExchangeID exch_, const string& mdSymbol_,const string& mdExch_) { }
    
    virtual void send_retransmit_request(channel_info& b);

    void init(const string&, const Parameter&);
    virtual void start() { }
    virtual void stop() { }
    
    int  establish_connection(nxt_book::publisher_info& publisher);
    bool request_symbol_id(NXT_Book_Handler* h, int context, const string& symbol);
    
    virtual size_t read(size_t, const char*, size_t);
    
    NXT_Book_Shared_TCP_Handler(Application* app);
    virtual ~NXT_Book_Shared_TCP_Handler();
    
  private:
    bool send_msg(NXT_Book_Handler* h, publisher_t& pub, const void* msg, size_t len);
    void subscribe(NXT_Book_Handler* h, publisher_t& pub, uint32_t symbol_id);
    
    typedef tbb::spin_mutex mutex_t;
    
    string          m_name;
    string          m_client_id;
    string          m_password;
    uint32_t        m_client_ip;
    mutex_t         m_mutex;
    
    deque<publisher_t>  m_publishers;
  };
  
  
  class NXT_Book_Handler : public Handler {
  public:
    struct subscription_t {
      ProductID  id;
      uint32_t   nxt_symbol_id;
      int        book_idx;
      ExchangeID exch;
      char       exch_char;
      
      subscription_t() : id(-1), nxt_symbol_id(-1), book_idx(-1), exch_char(' ') { }
    };
    
    // Handler functions
    virtual const string& name() const { return m_name; }
    virtual size_t parse(size_t context, const char* buf, size_t len);
    virtual size_t parse2(size_t, const char* buf, size_t len);
    virtual void reset(const char* msg);
    
    virtual void send_retransmit_request(channel_info& b);
    void create_qbdf_channel_info();
    
    virtual size_t read(size_t, const char*, size_t) { return 0; }

    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);
    static size_t dump_message(ILogger& log, Dump_Filters& filter, size_t context, const char* buf);
    
    void init(const string&, const vector<ExchangeID>& supported_exchanges, const Parameter&);
    virtual void start();
    virtual void stop();
    
    void reselect_publisher();
    
    void subscribe_product(ProductID id_, ExchangeID exch_, const string& mdSymbol_,const string& mdExch_);
    
    // Can be called by NXT_Book_Shared_TCP_Handler
    void handle_message(size_t context, const char* buf);
    
    NXT_Book_Handler(Application* app);
    virtual ~NXT_Book_Handler();

  private:
    typedef tbb::spin_mutex mutex_t;

    // The first NXT_Book_Handler to be initilaized will set this up
    //  for everyone else
    static NXT_Book_Shared_TCP_Handler*       m_tcp_handler;
    
    void look_for_heartbeat(size_t context, const char* buf, size_t len);
    void handle_heartbeat(size_t context, const char* buf, size_t len, uint8_t sender_id);
    
    void send_quote_update(); // fill out time & id and this does the rest
    void send_trade_update(); // fill out time, id, shares and price

    string m_name;
    string m_client_id;
    string m_password;
    
    mutable mutex_t m_mutex;
    
    Time m_midnight;

    QuoteUpdate        m_quote;
    TradeUpdate        m_trade;
    AuctionUpdate      m_auction;
    MarketStatusUpdate m_msu;

    vector<MarketStatus> m_security_status;
    
    bool m_not_logged_in        : 1;
    
    map<string, subscription_t>           m_subscriptions;
    hash_map<uint32_t, subscription_t>   m_nxt_subscriptions;
    
    //map<string, ExchangeID>        m_mic_to_exchangeid;
    
    vector<nxt_book::publisher_info>  m_nxt_publishers;
    uint8_t                           m_chosen_sender_id;
    uint8_t                           m_other_heartbeat;
    
    ExchangeRuleRP m_exch_rule;
    Feed_Rules_RP m_feed_rules;

    MarketStatus m_market_status;
        
    vector<char>             m_fragment_buffer;
    boost::dynamic_bitset<>  m_waiting_snapshot;
    deque<MDBufferQueue>     m_symbol_seq;
    
    vector<ExchangeID>       m_supported_exchanges;
    vector<char>             m_qbdf_exchange_chars;

    int64_t m_tz_offset;
    int64_t m_micros_recv_qbdf_time;
  };
  
}
