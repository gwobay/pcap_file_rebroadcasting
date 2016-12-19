#pragma once

namespace hf_core {

namespace Eurex {

namespace EnumMessageType { // emulating C++11 scoped enums
enum MessageType {
  AddComplexInstrument = 13400 ,
  AuctionBBO = 13500 ,
  AuctionClearingPrice = 13501 ,
  CrossRequest = 13502 ,
  ExecutionSummary = 13202 ,
  FullOrderExecution = 13104 ,
  Heartbeat = 13001 ,
  InstrumentStateChange = 13301 ,
  InstrumentSummary = 13601 ,
  OrderAdd = 13100 ,
  OrderDelete = 13102 ,
  OrderMassDelete = 13103 ,
  OrderModify = 13101 ,
  OrderModifySamePrio = 13106 ,
  PacketHeader = 13003 ,
  PartialOrderExecution = 13105 ,
  ProductStateChange = 13300 ,
  ProductSummary = 13600 ,
  QuoteRequest = 13503 ,
  SnapshotOrder = 13602 ,
  TopOfBook = 13504 ,
  TradeReport = 13201 ,
  TradeReversal = 13200
};
}

namespace EnumAggressorSide { // emulating C++11 scoped enums
enum AggressorSide {
  Buy = 1 ,
  Sell = 2
};
}
namespace EnumApplSeqResetIndicator { // emulating C++11 scoped enums
enum ApplSeqResetIndicator {
  NoReset = 0 ,
  Reset = 1
};
}
namespace EnumCompletionIndicator { // emulating C++11 scoped enums
enum CompletionIndicator {
  Incomplete = 0 ,
  Complete = 1
};
}
namespace EnumFastMarketIndicator { // emulating C++11 scoped enums
enum FastMarketIndicator {
  No = 0 ,
  Yes = 1
};
}
namespace EnumImpliedMarketIndicator { // emulating C++11 scoped enums
enum ImpliedMarketIndicator {
  NotImplied = 0 ,
  ImpliedInOut = 3
};
}
namespace EnumLegSide { // emulating C++11 scoped enums
enum LegSide {
  Buy = 1 ,
  Sell = 2
};
}
namespace EnumMDEntryType { // emulating C++11 scoped enums
enum MDEntryType {
  Trade = 2 ,
  OpeningPrice = 4 ,
  ClosingPrice = 5 ,
  HighPrice = 7 ,
  LowPrice = 8 ,
  TradeVolume = 66 ,
  PreviousClosingPrice = 101 ,
  OpeningAuction = 200 ,
  IntradayAuction = 201 ,
  CircuitBreakerAuction = 202 ,
  ClosingAuction = 203
};
}
namespace EnumMDReportEvent { // emulating C++11 scoped enums
enum MDReportEvent {
  ScopeDefinition = 0
};
}
namespace EnumMDUpdateAction { // emulating C++11 scoped enums
enum MDUpdateAction {
  New = 0 ,
  Change = 1 ,
  Delete = 2 ,
  Overlay = 5
};
}
namespace EnumMarketDataType { // emulating C++11 scoped enums
enum MarketDataType {
  OrderBookMaintenance = 1 ,
  OrderBookExecution = 2 ,
  TradeReversal = 3 ,
  TradeReport = 4 ,
  AuctionBBO = 5 ,
  AuctionClearingPrice = 6 ,
  CrossTradeAnnouncement = 7 ,
  QuoteRequest = 8 ,
  MarketSegmentSnapshot = 9 ,
  SingleInstrumentSnapshot = 10 ,
  OrderBookSnapshot = 11 ,
  MatchEvent = 12 ,
  TopOfBook = 13
};
}
namespace EnumMatchSubType { // emulating C++11 scoped enums
enum MatchSubType {
  OpeningAuction = 1 ,
  ClosingAuction = 2 ,
  IntradayAuction = 3 ,
  CircuitBreakerAuction = 4
};
}
namespace EnumMatchType { // emulating C++11 scoped enums
enum MatchType {
  ConfirmedTradeReport = 3 ,
  CrossAuction = 5 ,
  CallAuction = 7
};
}
namespace EnumNoMarketSegments { // emulating C++11 scoped enums
enum NoMarketSegments {
  One = 1
};
}
namespace EnumProductComplex { // emulating C++11 scoped enums
enum ProductComplex {
  FuturesSpread = 5 ,
  InterProductSpread = 6 ,
  StandardFuturesStrategy = 7 ,
  PackAndBundle = 8 ,
  Strip = 9
};
}
namespace EnumSecurityStatus { // emulating C++11 scoped enums
enum SecurityStatus {
  Active = 1 ,
  Inactive = 2 ,
  Expired = 4 ,
  Suspended = 9
};
}
namespace EnumSecurityTradingStatus { // emulating C++11 scoped enums
enum SecurityTradingStatus {
  Closed = 200 ,
  Restricted = 201 ,
  Book = 202 ,
  Continuous = 203 ,
  OpeningAuction = 204 ,
  OpeningAuctionFreeze = 205 ,
  IntradayAuction = 206 ,
  IntradayAuctionFreeze = 207 ,
  CircuitBreakerAuction = 208 ,
  CircuitBreakerAuctionFreeze = 209 ,
  ClosingAuction = 210 ,
  ClosingAuctionFreeze = 211
};
}
namespace EnumSide { // emulating C++11 scoped enums
enum Side {
  Buy = 1 ,
  Sell = 2
};
}
namespace EnumTradSesEvent { // emulating C++11 scoped enums
enum TradSesEvent {
  TBD = 0 ,
  StatusChange = 3
};
}
namespace EnumTradSesStatus { // emulating C++11 scoped enums
enum TradSesStatus {
  Halted = 1 ,
  Open = 2 ,
  Closed = 3
};
}
namespace EnumTradeCondition { // emulating C++11 scoped enums
enum TradeCondition {
  ImpliedTrade = 1
};
}
namespace EnumTradingSessionID { // emulating C++11 scoped enums
enum TradingSessionID {
  Day = 1 ,
  Morning = 3 ,
  Evening = 5 ,
  Holiday = 7
};
}
namespace EnumTradingSessionSubID { // emulating C++11 scoped enums
enum TradingSessionSubID {
  PreTrading = 1 ,
  Trading = 3 ,
  Closing = 4 ,
  PostTrading = 5 ,
  Quiescent = 7
};
}
struct AddComplexInstrument {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t TransactTime;
   int32_t SecuritySubType;
  EnumProductComplex::ProductComplex ProductComplex:8;
  EnumImpliedMarketIndicator::ImpliedMarketIndicator ImpliedMarketIndicator:8;
   uint8_t NoLegs;
      char Pad1;
   int32_t LegSymbol;
      char Pad4[4];
   int64_t LegSecurityID;
   int32_t LegRatioQty;
  EnumLegSide::LegSide LegSide:8;
      char Pad3[3];
} __attribute__((packed));

struct AuctionBBO {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TransactTime;
   int64_t SecurityID;
   int64_t BidPx;
   int64_t OfferPx;
} __attribute__((packed));

struct AuctionClearingPrice {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TransactTime;
   int64_t SecurityID;
   int64_t LastPx;
} __attribute__((packed));

struct CrossRequest {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
   int32_t LastQty;
      char Pad4[4];
  uint64_t TransactTime;
} __attribute__((packed));

struct ExecutionSummary {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t AggressorTimestamp;
  uint64_t RequestTime;
  uint64_t ExecID;
   int32_t LastQty;
  EnumAggressorSide::AggressorSide AggressorSide:8;
  EnumTradeCondition::TradeCondition TradeCondition:8;
      char Pad2[2];
   int64_t LastPx;
   int32_t RestingHiddenQty;
      char Pad4[4];
} __attribute__((packed));

// version 1.0, containing fewer fields than earlier versions
struct ExecutionSummary10 {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t AggressorTimestamp;
  uint64_t ExecID;
   int32_t LastQty;
  EnumAggressorSide::AggressorSide AggressorSide:8;
  EnumTradeCondition::TradeCondition TradeCondition:8;
      char Pad2[2];
   int64_t LastPx;
} __attribute__((packed));


struct FullOrderExecution {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  EnumSide::Side Side:8;
      char Pad7[7];
   int64_t Price;
  uint64_t TrdRegTSTimePriority;
   int64_t SecurityID;
  uint32_t TrdMatchID;
   int32_t LastQty;
   int64_t LastPx;
} __attribute__((packed));

struct Heartbeat {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint32_t LastMsgSeqNumProcessed;
      char Pad4[4];
} __attribute__((packed));

struct InstrumentStateChange {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  EnumSecurityStatus::SecurityStatus SecurityStatus:8;
  EnumSecurityTradingStatus::SecurityTradingStatus SecurityTradingStatus:8;
  EnumFastMarketIndicator::FastMarketIndicator FastMarketIndicator:8;
      char Pad5[5];
  uint64_t TransactTime;
} __attribute__((packed));

struct InstrumentSummary {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t LastUpdateTime;
  uint64_t TrdRegTSExecutionTime;
  uint16_t TotNoOrders;
  EnumSecurityStatus::SecurityStatus SecurityStatus:8;
  EnumSecurityTradingStatus::SecurityTradingStatus SecurityTradingStatus:8;
  EnumFastMarketIndicator::FastMarketIndicator FastMarketIndicator:8;
   uint8_t NoMDEntries;
      char Pad2[2];
   int64_t MDEntryPx;
   int32_t MDEntrySize;
  EnumMDEntryType::MDEntryType MDEntryType:8;
      char Pad3[3];
} __attribute__((packed));

struct OrderAdd {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TrdRegTSTimeIn;
   int64_t SecurityID;
  uint64_t TrdRegTSTimePriority;
   int32_t DisplayQty;
  EnumSide::Side Side:8;
      char Pad3[3];
   int64_t Price;
} __attribute__((packed));

struct OrderDelete {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TrdRegTSTimeIn;
  uint64_t TransactTime;
   int64_t SecurityID;
  uint64_t TrdRegTSTimePriority;
   int32_t DisplayQty;
  EnumSide::Side Side:8;
      char Pad3[3];
   int64_t Price;
} __attribute__((packed));

struct OrderDelete10 {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TrdRegTSTimeIn;
   int64_t SecurityID;
  uint64_t TrdRegTSTimePriority;
   int32_t DisplayQty;
  EnumSide::Side Side:8;
      char Pad3[3];
   int64_t Price;
} __attribute__((packed));

struct OrderMassDelete {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t TransactTime;
} __attribute__((packed));

struct OrderMassDelete10 {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t TransactTime;
} __attribute__((packed));

struct OrderModify {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TrdRegTSTimeIn;
  uint64_t TrdRegTSPrevTimePriority;
   int64_t PrevPrice;
   int32_t PrevDisplayQty;
      char Pad4[4];
   int64_t SecurityID;
  uint64_t TrdRegTSTimePriority;
   int32_t DisplayQty;
  EnumSide::Side Side:8;
      char Pad3[3];
   int64_t Price;
} __attribute__((packed));

struct OrderModifySamePrio {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TrdRegTSTimeIn;
  uint64_t TransactTime;
   int32_t PrevDisplayQty;
      char Pad4[4];
   int64_t SecurityID;
  uint64_t TrdRegTSTimePriority;
   int32_t DisplayQty;
  EnumSide::Side Side:8;
      char Pad3[3];
   int64_t Price;
} __attribute__((packed));

struct PacketHeader {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint32_t ApplSeqNum;
   int32_t MarketSegmentID;
   uint8_t PartitionID;
  EnumCompletionIndicator::CompletionIndicator CompletionIndicator:8;
  EnumApplSeqResetIndicator::ApplSeqResetIndicator ApplSeqResetIndicator:8;
      char Pad5[5];
  uint64_t TransactTime;
} __attribute__((packed));

struct PartialOrderExecution {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  EnumSide::Side Side:8;
      char Pad7[7];
   int64_t Price;
  uint64_t TrdRegTSTimePriority;
   int64_t SecurityID;
  uint32_t TrdMatchID;
   int32_t LastQty;
   int64_t LastPx;
} __attribute__((packed));

struct ProductStateChange {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  EnumTradingSessionID::TradingSessionID TradingSessionID:8;
  EnumTradingSessionSubID::TradingSessionSubID TradingSessionSubID:8;
  EnumTradSesStatus::TradSesStatus TradSesStatus:8;
  EnumFastMarketIndicator::FastMarketIndicator FastMarketIndicator:8;
      char Pad4[4];
  uint64_t TransactTime;
} __attribute__((packed));

struct ProductSummary {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint32_t LastMsgSeqNumProcessed;
  EnumTradingSessionID::TradingSessionID TradingSessionID:8;
  EnumTradingSessionSubID::TradingSessionSubID TradingSessionSubID:8;
  EnumTradSesStatus::TradSesStatus TradSesStatus:8;
  EnumFastMarketIndicator::FastMarketIndicator FastMarketIndicator:8;
} __attribute__((packed));

struct QuoteRequest {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
   int32_t LastQty;
  EnumSide::Side Side:8;
      char Pad3[3];
  uint64_t TransactTime;
} __attribute__((packed));

struct SnapshotOrder {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TrdRegTSTimePriority;
   int32_t DisplayQty;
  EnumSide::Side Side:8;
      char Pad3[3];
   int64_t Price;
} __attribute__((packed));

struct TopOfBook {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TransactTime;
   int64_t SecurityID;
   int64_t BidPx;
   int64_t OfferPx;
} __attribute__((packed));

struct TradeReport {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t TransactTime;
  uint32_t TrdMatchID;
   int32_t LastQty;
   int64_t LastPx;
  EnumMatchType::MatchType MatchType:8;
  EnumMatchSubType::MatchSubType MatchSubType:8;
      char Pad6[6];
} __attribute__((packed));

struct TradeReversal {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t TransactTime;
  uint32_t TrdMatchID;
   int32_t LastQty;
   int64_t LastPx;
  uint64_t TrdRegTSExecutionTime;
   uint8_t NoMDEntries;
      char Pad7[7];
   int64_t MDEntryPx;
   int32_t MDEntrySize;
  EnumMDEntryType::MDEntryType MDEntryType:8;
      char Pad3[3];
} __attribute__((packed));


}

}
