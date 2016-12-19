#ifndef hf_data_Binary_Data_H
#define hf_data_Binary_Data_H

#include <boost/shared_ptr.hpp>
#include <climits>
#include <map>
#include <Util/enum.h>

namespace hf_core {

  BOOST_ENUM(QBDFStatusType,
             (Invalid)
             (Open)
             (Closed)
             (Halted)
             (Suspended)
             (Auction)
             (OpeningDelay)
             (Resume)
             (NoOpenNoResume)
             (ShortSaleRestrictActivated)
             (ShortSaleRestrictContinued)
             (NoShortSaleRestrictionInEffect)
  );

  BOOST_ENUM(QBDFMsgFmt,
             (Invalid)
             (QuoteSmall)
             (QuoteLarge)
             (Trade)
             (Summary)
             (Imbalance)
             (OrderAddSmall)
             (OrderAddLarge)
             (Status)
             (OpenCloseSummary)
             (OrderReplace)
             (OrderModify)
             (OrderExec)
             (GapMarker)
             (Level)
             (MultiSessionOpenCloseSummary)
             (level_iprice32)
             (level_iprice64)
             (order_add_iprice32)
             (order_replace_iprice32)
             (order_modify_iprice32)
             (order_exec_iprice32)
             (quote_small_iprice32)
             (quote_large_iprice32)
             (trade_iprice32)
             (imbalance_iprice32)
             (status_micros)
             (gap_marker_micros)
	     (trade_augmented)
             (quote_iprice64)
             (trade_iprice64)
             (imbalance_iprice64)
             (order_exec_iprice32_cond)
  );


  struct qbdf_common {
    char     format;
    uint32_t time;
    int32_t  exch_time_delta;
    char     exch;
    uint16_t id;
  } __attribute__((packed));


  struct qbdf_format_header {
    char format;
  } __attribute__((packed));

  struct qbdf_allday_index_item {
    uint8_t   format;  // 0 - uncompressed, 1 - LZO
    uint64_t  offset;
    uint64_t  length;
  }  __attribute__((packed));

  struct qbdf_allday_header {
    char  magic[4];
    qbdf_allday_index_item index[24*60];
  } __attribute__((packed));

  struct qbdf_gap_marker {
    qbdf_format_header hdr;
    uint16_t time;
    uint16_t symbol_id;
    uint64_t context;
  } __attribute__((packed));


  struct qbdf_gap_marker_micros {
    qbdf_common common;
    uint64_t context;
  } __attribute__((packed));


  struct qbdf_quote_price {
    uint16_t time;    // millis since start of this minute, uint16_t is 2 bytes
    uint16_t symbolId;        // 2 byte TAQ specific symbol id, will be mapped to real ID thru API
    float bidPrice;         // use 4 byte float to save space, just like TAQ dvd does
    float askPrice;
    char exch;
    char cond;
    char source;
    char cancelCorrect;
  } __attribute__((packed));


  struct qbdf_quote_size_small {
    uint16_t askSize;
    uint16_t bidSize;
  } __attribute__((packed));


  struct qbdf_quote_size_large {
    uint32_t askSize;
    uint32_t bidSize;
  } __attribute__((packed));


  struct qbdf_quote_large {
    qbdf_format_header hdr;
    qbdf_quote_price qr;
    qbdf_quote_size_large sizes;
  } __attribute__((packed));


  struct qbdf_quote_seq_large {
    qbdf_quote_large lqr;
    uint32_t seq;
  } __attribute__((packed));


  struct qbdf_quote_small {
    qbdf_format_header hdr;
    qbdf_quote_price qr;
    qbdf_quote_size_small sizes;
  } __attribute__((packed));


  struct qbdf_quote_seq_small {
    qbdf_quote_small sqr;
    uint32_t seq;
  } __attribute__((packed));


  struct qbdf_quote_small_iprice32 {
    qbdf_common common;
    char cond;
    uint32_t bid_price;
    uint32_t ask_price;
    uint16_t bid_size;
    uint16_t ask_size;
  } __attribute__((packed));


  struct qbdf_quote_small_iprice32_seq {
    qbdf_quote_small_iprice32 q;
    uint32_t seq;
  } __attribute__((packed));


  struct qbdf_quote_large_iprice32 {
    qbdf_common common;
    char cond;
    uint32_t bid_price;
    uint32_t ask_price;
    uint32_t bid_size;
    uint32_t ask_size;
  } __attribute__((packed));


  struct qbdf_quote_large_iprice32_seq {
    qbdf_quote_large_iprice32 q;
    uint32_t seq;
  } __attribute__((packed));


  struct qbdf_quote_iprice64 {
    qbdf_common common;
    char cond;
    uint64_t bid_price;
    uint64_t ask_price;
    uint32_t bid_size;
    uint32_t ask_size;
  } __attribute__((packed));


  struct qbdf_trade {
    qbdf_format_header hdr;
    uint16_t time;
    uint16_t symbolId;
    float price;
    uint32_t size;
    char exch;
    char stop;
    char correct;
    char cond[4];
    char source;
    char side;
  } __attribute__((packed));


  struct qbdf_trade_seq {
    qbdf_trade tr;
    uint32_t seq;
  } __attribute__((packed));


  struct qbdf_trade_iprice32 {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t side_known = (1 << 1);
    static const uint8_t delta_in_millis = (1 << 3);
    
    qbdf_common common;
    uint8_t flags;
    char correct;
    char cond[4];
    uint32_t size;
    uint32_t price;
  } __attribute__((packed));


  struct qbdf_trade_iprice32_seq {
    qbdf_trade_iprice32 t;
    uint32_t seq;
  } __attribute__((packed));


  struct qbdf_trade_iprice64 {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t side_known = (1 << 1);
    static const uint8_t delta_in_millis = (1 << 3);
    
    qbdf_common common;
    uint8_t flags;
    char correct;
    char cond[4];
    uint32_t size;
    uint64_t price;
  } __attribute__((packed));

 
  struct qbdf_trade_augmented {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t side_known = (1 << 1);
    
    static const uint8_t clean_flag = (1 << 2);
    
    static const uint8_t delta_in_millis = (1 << 3);
    
    static const uint8_t exec_flag_unknown = (0 << 4);
    static const uint8_t exec_flag_exec = (1 << 4);
    static const uint8_t exec_flag_exec_w_price = (2 << 4);
    static const uint8_t exec_flag_trade = (3 << 4);
    
    uint8_t get_exec_flag() const { return (flags >> 4) & 0x03; }
    void    set_exec_flag(char ef) {
      switch(ef) {
      case ' ':  flags |= exec_flag_unknown; break;
      case 'E':  flags |= exec_flag_exec; break;
      case 'P':  flags |= exec_flag_exec_w_price; break;
      case 'T':  flags |= exec_flag_trade; break;
      default: break;
      }
    }
    
    bool    get_clean_flag() const { return flags & clean_flag; }
    void    set_clean_flag(bool c) { if(c) { flags |= clean_flag; } else { flags &= ~clean_flag; } }
    
    qbdf_common common;
    uint8_t flags;
    char correct;
    char trade_status;
    char market_status;
    char cond[4];
    char misc[2];
    uint32_t size;
    uint32_t price;
    uint64_t order_age;    
  } __attribute__((packed));

  struct qbdf_summary {
    qbdf_format_header hdr;
    uint16_t symbolId;
    char exch;
    float bidPrice;
    uint16_t bidSize;
    float askPrice;
    uint16_t askSize;
    char quote_cond;
    char quote_source;
    char quote_cancelCorrect;
    uint32_t quote_time;
    float price;
    uint32_t size;
    char trade_stop;
    char trade_correct;
    char trade_cond[4];
    char trade_source;
    char side;
    uint32_t trade_time;
    uint32_t market_status;
    uint32_t trade_status;
    uint32_t quote_status;
    uint64_t cum_total_volume;
    uint64_t cum_clean_volume;
    uint64_t cum_clean_value;
    uint32_t total_trade_count;
    uint32_t clean_trade_count;
    float high;
    float low;

    qbdf_summary() : symbolId(0), exch(), bidPrice(0), bidSize(0), askPrice(0), askSize(0),
    quote_cond(), quote_cancelCorrect(), quote_time(0), price(0), size(0), trade_stop(),
    trade_correct(), trade_cond(), trade_source(0), side(' '), trade_time(0),
    market_status(), trade_status(), quote_status(), cum_total_volume(0), cum_clean_volume(0),
    cum_clean_value(0), total_trade_count(0), clean_trade_count(0), high(0), low(999999) {;}
  } __attribute__((packed));

  typedef boost::shared_ptr<qbdf_summary> qbdf_summary_rp;

  struct qbdf_summary_cache {
    uint16_t id;
    qbdf_summary_rp cached_by_exch[256];

    qbdf_summary_cache() {}
    qbdf_summary_cache(uint16_t _id) : id(_id) {}
  };

  typedef boost::shared_ptr<qbdf_summary_cache> qbdf_summary_cache_rp;


  struct qbdf_oc_summary {
    qbdf_format_header hdr;
    uint16_t symbolId;
    char exch;
    float open_price;
    uint32_t open_size;
    uint32_t open_time;
    float close_price;
    uint32_t close_size;
    uint32_t close_time;

    qbdf_oc_summary() : symbolId(0), exch(), open_price(0), open_size(0), open_time(0),
    close_price(0), close_size(0), close_time(0) {;}

  } __attribute__((packed));

  typedef boost::shared_ptr<qbdf_oc_summary> qbdf_oc_summary_rp;

  struct qbdf_oc_summary_cache {
    uint16_t id;
    qbdf_oc_summary_rp cached_by_exch[256];

    qbdf_oc_summary_cache() {}
    qbdf_oc_summary_cache(uint16_t _id) : id(_id) {}
  };

  typedef boost::shared_ptr<qbdf_oc_summary_cache> qbdf_oc_summary_cache_rp;


  struct qbdf_oc_summary_multi {
    qbdf_format_header hdr;
    uint16_t symbolId;
    char exch;
    uint32_t session;
    float open_price;
    uint32_t open_size;
    uint32_t open_time;
    float close_price;
    uint32_t close_size;
    uint32_t close_time;

    qbdf_oc_summary_multi() : symbolId(0), exch(), session(),
    open_price(0), open_size(0), open_time(0),
    close_price(0), close_size(0), close_time(0) {;}

  } __attribute__((packed));

  typedef boost::shared_ptr<qbdf_oc_summary_multi> qbdf_oc_summary_multi_rp;

  struct qbdf_oc_summary_multi_cache {
    uint16_t id;
    qbdf_oc_summary_multi_rp cached_by_exch_and_session[256][16];

    qbdf_oc_summary_multi_cache() {}
    qbdf_oc_summary_multi_cache(uint16_t _id) : id(_id) {}
  };

  typedef boost::shared_ptr<qbdf_oc_summary_multi_cache> qbdf_oc_summary_multi_cache_rp;


  struct qbdf_imbalance {
    qbdf_format_header hdr;
    uint16_t time;
    uint16_t symbolId;
    float ref_price;
    float near_price;
    float far_price;
    uint32_t paired_size;
    uint32_t imbalance_size;
    char imbalance_side;
    char cross_type;
    char exch;
  } __attribute__((packed));


  struct qbdf_imbalance_seq {
    qbdf_imbalance ir;
    uint32_t seq;
  } __attribute__((packed));


  struct qbdf_imbalance_iprice32 {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t side_known = (1 << 1);
    static const uint8_t delta_in_millis = (1 << 3);


    qbdf_common common;
    uint8_t flags;
    char cross_type;
    uint32_t paired_size;
    uint32_t imbalance_size;
    uint32_t ref_price;
    uint32_t near_price;
    uint32_t far_price;
  } __attribute__((packed));


  struct qbdf_imbalance_iprice32_seq {
    qbdf_imbalance_iprice32 i;
    uint32_t seq;
  } __attribute__((packed));

  struct qbdf_imbalance_iprice64 {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t side_known = (1 << 1);
    static const uint8_t delta_in_millis = (1 << 3);


    qbdf_common common;
    uint8_t flags;
    char cross_type;
    uint32_t paired_size;
    uint32_t imbalance_size;
    uint64_t ref_price;
    uint64_t near_price;
    uint64_t far_price;
  } __attribute__((packed));


  struct qbdf_status {
    qbdf_format_header hdr;
    uint16_t time;
    uint16_t symbolId;
    char exch;
    uint16_t status_type;
    char status_detail[8];
  } __attribute__((packed));


  struct qbdf_status_seq {
    qbdf_status sr;
    uint32_t seq;
  } __attribute__((packed));


  struct qbdf_status_micros {
    qbdf_common common;
    uint16_t status_type;
    char status_detail[32];
  } __attribute__((packed));


  struct qbdf_order_add_price {
    uint16_t time;
    uint16_t symbol_id;
    uint64_t order_id;
    char exch;
    char side;
    float price;
  } __attribute__((packed));


  struct qbdf_order_add_size_small {
    uint16_t size;
  } __attribute__((packed));

  struct qbdf_order_add_small {
    qbdf_format_header hdr;
    qbdf_order_add_price pr;
    qbdf_order_add_size_small sz;
  } __attribute__((packed));

  struct qbdf_order_add_size_large {
    uint32_t size;
  } __attribute__((packed));


  struct qbdf_order_add_large {
    qbdf_format_header hdr;
    qbdf_order_add_price pr;
    qbdf_order_add_size_large sz;
  } __attribute__((packed));


  struct qbdf_order_replace {
    qbdf_format_header hdr;
    uint16_t time;
    uint16_t symbol_id;
    uint64_t orig_order_id;
    uint64_t new_order_id;
    float price;
    uint32_t size;
  } __attribute__ ((packed));


  struct qbdf_order_modify {
    qbdf_format_header hdr;
    uint16_t time;
    uint16_t symbol_id;
    uint64_t orig_order_id;
    char new_side; // as far as we know, this shouldn't change
    float new_price;
    uint32_t new_size; // new_size = 0 means remove order
  } __attribute__ ((packed));


  struct qbdf_order_exec {
    qbdf_format_header hdr;
    uint16_t time;
    uint16_t symbol_id;
    uint64_t order_id;
    char exch;
    char side;
    float price;
    uint32_t size;
  } __attribute__((packed));


  struct qbdf_level {
    qbdf_format_header hdr;
    uint16_t time;
    uint16_t symbol_id;
    char exch;
    char side;
    float price;
    uint32_t size;
    uint16_t num_orders;
    char trade_status;
    char quote_cond;
    char reason_code;
    uint32_t link_id_1;
    uint32_t link_id_2;
    uint32_t link_id_3;
    uint8_t last_in_group;
  } __attribute__ ((packed));


  struct qbdf_level_iprice32 {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t last_in_group_flag = (1 << 1);
    static const uint8_t executed_flag = (1 << 2);
    static const uint8_t delta_in_millis = (1 << 3);

    qbdf_common common;
    uint8_t flags;
    char trade_cond;
    char quote_cond;
    char reason_code;
    uint16_t num_orders;
    uint32_t size;
    uint32_t price;
  } __attribute__ ((packed));


  struct qbdf_level_iprice64 {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t last_in_group_flag = (1 << 1);
    static const uint8_t executed_flag = (1 << 2);
    static const uint8_t delta_in_millis = (1 << 3);

    qbdf_common common;
    uint8_t flags;
    char trade_cond;
    char quote_cond;
    char reason_code;
    uint16_t num_orders;
    uint32_t size;
    uint64_t price;
  } __attribute__ ((packed));


  struct qbdf_order_add_iprice32 {   // 23 bytes
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t delta_in_millis = (1 << 3);

    qbdf_common common;
    uint8_t     flags;
    uint64_t    order_id;
    uint32_t    size;
    uint32_t    price;
  } __attribute__ ((packed));


  struct qbdf_order_replace_iprice32 {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t delta_in_millis = (1 << 3);

    qbdf_common common;
    uint8_t     flags;
    uint64_t    orig_order_id;
    uint64_t    new_order_id;
    uint32_t    size;
    uint32_t    price;
  } __attribute__ ((packed));


  struct qbdf_order_modify_iprice32 {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t preserve_queue_priority = (1 << 1);
    static const uint8_t delta_in_millis = (1 << 3);

    qbdf_common common;
    uint8_t     flags;
    uint64_t    order_id;
    uint32_t    size;
    uint32_t    price;
  } __attribute__ ((packed));


  struct qbdf_order_exec_iprice32 {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t delta_in_millis = (1 << 3);

    qbdf_common common;
    uint8_t     flags;
    uint8_t     cond;
    uint64_t    order_id;
    uint32_t    size;
    uint32_t    price;
  } __attribute__ ((packed));

  struct qbdf_order_exec_iprice32_cond {
    static const uint8_t side_flag = (1 << 0);
    static const uint8_t delta_in_millis = (1 << 3);

    qbdf_common common;
    uint8_t     flags;
    char        cond[4];
    uint64_t    order_id;
    uint32_t    size;
    uint32_t    price;
  } __attribute__ ((packed));


  typedef std::multimap<int,qbdf_format_header*> SequencedRecordMap;


  template<typename T>
  bool set_delta_in_millis(T& msg, bool is_delta_in_millis)
  {
    if (is_delta_in_millis) {
      msg.flags |= T::delta_in_millis;
    } else {
      msg.flags &= (~T::delta_in_millis);
    }
    return true;
  }

  template<typename T>
  bool get_delta_in_millis(const T& msg)
  {
    if(msg.flags & T::delta_in_millis) {
      return true;
    } else {
      return false;
    }
  }

  template<typename T>
  bool set_side(T& msg, char side)
  {
    if(side == 'B') {
      msg.flags |= T::side_flag;
      return true;
    } else if(side == 'S') {
      msg.flags &= (~T::side_flag);
      return true;
    } else {
      return false;
    }
  }
  
  template<typename T>
  char get_side(const T& msg)
  {
    if(msg.flags & T::side_flag) {
      return 'B';
    } else {
      return 'S';
    }
  }
  
  template< >
  inline bool set_side(qbdf_trade_iprice32& msg, char side)
  {
    if(side == 'B') {
      msg.flags |= qbdf_trade_iprice32::side_flag;
      msg.flags |= qbdf_trade_iprice32::side_known;
    } else if(side == 'S') {
      msg.flags &= (~qbdf_trade_iprice32::side_flag);
      msg.flags |= qbdf_trade_iprice32::side_known;
    } else {
      msg.flags &= (~qbdf_trade_iprice32::side_known);
    }
    return true;
  }

  template< >
  inline bool set_side(qbdf_trade_augmented& msg, char side)
    {
    if(side == 'B') {
      msg.flags |= qbdf_trade_augmented::side_flag;
      msg.flags |= qbdf_trade_augmented::side_known;
    } else if(side == 'S') {
      msg.flags &= (~qbdf_trade_augmented::side_flag);
      msg.flags |= qbdf_trade_augmented::side_known;
    } else {
      msg.flags &= (~qbdf_trade_augmented::side_known);
    }
    return true;
  }


  template< >
  inline bool set_side(qbdf_imbalance_iprice32& msg, char side)
  {
    if(side == 'B') {
      msg.flags |= qbdf_imbalance_iprice32::side_flag;
      msg.flags |= qbdf_imbalance_iprice32::side_known;
    } else if(side == 'S') {
      msg.flags &= (~qbdf_imbalance_iprice32::side_flag);
      msg.flags |= qbdf_imbalance_iprice32::side_known;
    } else {
      msg.flags &= (~qbdf_imbalance_iprice32::side_known);
    }
    return true;
  }

  template<>
  inline char get_side(const qbdf_trade_iprice32& msg)
  {
    if(msg.flags & qbdf_trade_iprice32::side_known) {
      if(msg.flags & qbdf_trade_iprice32::side_flag) {
        return 'B';
      } else {
        return 'S';
      }
    } else {
      return ' ';
    }
  }
  
  template<>
  inline char get_side(const qbdf_trade_augmented& msg)
  {
    if(msg.flags & qbdf_trade_augmented::side_known) {
      if(msg.flags & qbdf_trade_augmented::side_flag) {
        return 'B';
      } else {
        return 'S';
      }
    } else {
      return 'U';
    }
  }

  template<>
  inline char get_side(const qbdf_imbalance_iprice32& msg)
  {
    if(msg.flags & qbdf_imbalance_iprice32::side_known) {
      if(msg.flags & qbdf_imbalance_iprice32::side_flag) {
        return 'B';
      } else {
        return 'S';
      }
    } else {
      return ' ';
    }
  }

  template<typename T>
  bool set_last_in_group(T& msg, bool last_in_group)
  {
    if(last_in_group) {
      msg.flags |= T::last_in_group_flag;
    } else {
      msg.flags &= (~T::last_in_group_flag);
    }
    return true;
  }

  template<typename T>
  uint8_t get_last_in_group(const T& msg)
  {
    if(msg.flags & T::last_in_group_flag) {
      return 1;
    } else {
      return 0;
    }
  }

  template<typename T>
  bool set_executed(T& msg, bool executed)
  {
    if(executed) {
      msg.flags |= T::executed_flag;
    } else {
      msg.flags &= (~T::executed_flag);
    }
    return true;
  }

  template<typename T>
  uint8_t get_executed(const T& msg)
  {
    if(msg.flags & T::executed_flag) {
      return 1;
    } else {
      return 0;
    }
  }

  /****************************************************************************************************************************/
  //MD-like structures for capture
  /****************************************************************************************************************************/


  struct BaseDataCapture {
    //md core event info
    char datatype; //values from 1 to 14 based on SMDSCaptureDataType
    unsigned int gapfillseqno;
    unsigned long long seqno;
    unsigned long long originalseqno;
    struct timeval exchangeTime;
    struct timeval recvTime;
    struct timeval mdrcRecvTime;
    struct timeval recordedTime;

    //product info
    char type; //based on md_core::InstrumentDef::InstrumentType
    char symbol[24];
    char exchange[4];
    char currency[4];

  } __attribute__((packed));

  struct QuoteCapture : BaseDataCapture {
    char quoteCondition;
    bool isNBBO;
    bool slowBid;
    bool slowAsk;
    char status; //MDQuoteStatus
    short numbuyorders;
    short numsellorders;
    double bid;
    double ask;
    char bexchange[9];
    char oexchange[9];
    long bidSize;
    long askSize;
  }__attribute__((packed)) ;

  struct  TradeCapture : BaseDataCapture {
    char orderId[20];
    bool is_valid;
    char orderIDType; //based on u_oid::MDOrderIDType;
    char side; //Orderside
    char crossType[4];
    char  compositeParticipantID[4];
    char tradeQualifier[8];
    double last;
    char status; //based on md_core::MDTrade::MDTradeStatus
    char visibility;//based on  md_core::MDTrade::TradeVisibility
    long lastSize;
    unsigned long long  cumVolume;
    char  subMktID;
    char state; //based on md_core::InstrumentDef::InstrumentMktState
  }__attribute__((packed));

  struct OrderCapture : BaseDataCapture {
    char orderId[20];
    bool is_valid;
    char side;
    long size;
    long oldsize;
    double price;
    double oldprice;
    short status; // based on MDOrder::MDOrderStatus, can be negative
    char display[6]; //display field of order
  }__attribute__((packed));

  struct ImbalanceCapture : BaseDataCapture {
    char imbalance_period; //capture of enum ImbalanceTime
    char side;
    char regulatory; // '1' yes. '0' NO. Nyseonly
    char crosstype[4];
    double price;
    double farprice;
    double nearprice;
    double bp;
    double ap;
    long bsize;
    long asize;
    long size;
    long pairedsize;
    long totalImbalance;
    long marketImbalance;
  }__attribute__((packed));


  struct SecurityStatusCapture : BaseDataCapture {
    char state;
  }__attribute__((packed));

  struct TradingIndAlertCapture: BaseDataCapture {
    char financialStatus;
    char corporateAction;
    char securityStatus;
    char adjustment;
    short  pdenom;
    int  bid;
    int  ask;
    double  bidP;
    double  askP;
  }__attribute__((packed));

  struct TradeDisseminationAlertCapture: BaseDataCapture {
    char financialStatus;
    char corporateAction;
    char securityStatus;
    char tradeDisseminationTime[7];        // Trade dissemination in HHMMSS (if available)
    struct timeval tradeDissemination;       // Trade dissemination in epoch time (if available)
  }__attribute__((packed));

  enum SMDSCaptureDataType  //MD-like
  {
    C_DT_TRADE = 1,             // 0 MDTrade
    C_DT_IMBALANCE,             // 1 MDImblance
    C_DT_BOOK,                // 2 MDOrderBook
    C_DT_QUOTE,               // 3 MDQuote
    C_DT_CUSTOM,                // 4 MDCustom
    C_DT_ORDER,               // 5 MDOrder
    C_DT_ALERT_MKTIMB,          // 6 MDMktImbalanceAlert
    C_DT_ALERT_DELAYHALT,       // 7 MDMktDelayOrTradingHaltAlert
    C_DT_ALERT_TRADEINDICATION,   // 8 MDMktTradingIndicationAlert
    C_DT_ALERT_TRADEDISSEMINATION,  // 9 MDMktTradeDisseminationTimeAlert
    C_DT_ALERT_TRADINGCOLLAR,     // 10 MDMktTradingCollarAlert
    c_DT_ALERT_CICUITBREAKER,     // 11 MDMktCircuitBreakerAlert
    C_DT_PRODUCT,                // 12 MDProductInfo (not strictly a subclass)
    C_DT_SECSTATUS    //13 not defined in md
  };

}

#endif // hf_data_Binary_Data_H
