#include <Data/FAST_Decoder.h>


struct Cme_Fast_Decoder_GlobalDictionary
{
  StringStateRepository m_string_repo;


  Cme_Fast_Decoder_GlobalDictionary()
  {
  }

  void reset()
  {
  }
};

class MDIncRefresh_112_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_TradeCondition;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_112
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_112_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_112_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_113_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_113
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_113_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_113_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDSnapshotFullRefresh_114_MDEntries_Element
{
private:
  FAST_String m_MDEntryType;
  FAST_Nullable<double > m_MDEntryPx_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<double > m_NetChgPrevDay_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDEntryPx_state.has_value = false;
    m_MDEntrySize_state.has_value = false;
    m_MDPriceLevel_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<double >(buf, m_MDEntryPx_state.value, m_MDEntryPx_state.has_value);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<double >& MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
};

class MDSnapshotFullRefresh_114
{
private:
  FAST_String m_MessageType_state;
  FAST_String m_ApplVerID_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint64_t m_SendingTime_state;
  uint32_t m_LastMsgSeqNumProcessed_state;
  uint32_t m_TotNumReports_state;
  uint32_t m_RptSeq_state;
  uint32_t m_MDBookType_state;
  uint32_t m_SecurityID_state;
  uint32_t m_SecurityIDSource_state;
  FAST_Nullable<uint32_t > m_MDSecurityTradingStatus_state;
  FAST_Sequence<MDSnapshotFullRefresh_114_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("W", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_LastMsgSeqNumProcessed_state = 0;
    m_TotNumReports_state = 0;
    m_RptSeq_state = 0;
    m_MDBookType_state = 0;
    m_SecurityID_state = 0;
    m_SecurityIDSource_state = 8;
    m_MDSecurityTradingStatus_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    decode_field_mandatory<uint32_t >(buf, m_LastMsgSeqNumProcessed_state);
    decode_field_mandatory<uint32_t >(buf, m_TotNumReports_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<uint32_t >(buf, m_MDBookType_state);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_optional<uint32_t >(buf, m_MDSecurityTradingStatus_state.value, m_MDSecurityTradingStatus_state.has_value);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline uint32_t LastMsgSeqNumProcessed()
  {
    return m_LastMsgSeqNumProcessed_state;
  }
  
  inline uint32_t TotNumReports()
  {
    return m_TotNumReports_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline uint32_t MDBookType()
  {
    return m_MDBookType_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDSecurityTradingStatus()
  {
    return m_MDSecurityTradingStatus_state;
  }
  
  inline FAST_Sequence<MDSnapshotFullRefresh_114_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_115_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  FAST_Nullable<double > m_MDEntryPx_state;
  FAST_Nullable<uint32_t > m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state.has_value = false;
    m_MDEntryTime_state.has_value = false;
    m_MDEntrySize_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_TradeVolume_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_optional<double >(buf, m_MDEntryPx_state.value, m_MDEntryPx_state.has_value);
    decode_field_optional<uint32_t >(buf, m_MDEntryTime_state.value, m_MDEntryTime_state.has_value);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_Nullable<double >& MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
};

class MDIncRefresh_115
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_115_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_115_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_116_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  uint32_t m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  uint32_t m_MDEntryTime_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  int32_t m_MDEntrySize_state;
  FAST_String m_TradingSessionID;
  uint32_t m_NumberOfOrders_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state = 0;
    m_MDEntryTime_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntrySize_state = 0;
    m_NumberOfOrders_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_mandatory<uint32_t >(buf, m_MDPriceLevel_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_mandatory<int32_t >(buf, m_MDEntrySize_state);
    perParseStringRepo.read_string_mandatory(buf, m_TradingSessionID);
    decode_field_mandatory<uint32_t >(buf, m_NumberOfOrders_state);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline uint32_t MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline int32_t MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_String& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline uint32_t NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
};

class MDIncRefresh_116
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_116_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_116_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_117_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  uint32_t m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  uint32_t m_MDEntryTime_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  int32_t m_MDEntrySize_state;
  uint32_t m_NumberOfOrders_state;
  FAST_String m_TradingSessionID;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state = 0;
    m_MDEntryTime_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntrySize_state = 0;
    m_NumberOfOrders_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_mandatory<uint32_t >(buf, m_MDPriceLevel_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_mandatory<int32_t >(buf, m_MDEntrySize_state);
    decode_field_mandatory<uint32_t >(buf, m_NumberOfOrders_state);
    perParseStringRepo.read_string_mandatory(buf, m_TradingSessionID);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline uint32_t MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline int32_t MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline uint32_t NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline FAST_String& TradingSessionID()
  {
    return m_TradingSessionID;
  }
};

class MDIncRefresh_117
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_117_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_117_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDSnapshotFullRefresh_118_MDEntries_Element
{
private:
  FAST_String m_MDEntryType;
  FAST_Nullable<double > m_MDEntryPx_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<double > m_NetChgPrevDay_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDEntryPx_state.has_value = false;
    m_MDEntrySize_state.has_value = false;
    m_MDPriceLevel_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<double >(buf, m_MDEntryPx_state.value, m_MDEntryPx_state.has_value);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<double >& MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
};

class MDSnapshotFullRefresh_118
{
private:
  FAST_String m_MessageType_state;
  FAST_String m_ApplVerID_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint64_t m_SendingTime_state;
  uint32_t m_LastMsgSeqNumProcessed_state;
  uint32_t m_TotNumReports_state;
  uint32_t m_RptSeq_state;
  uint32_t m_MDBookType_state;
  uint32_t m_SecurityID_state;
  uint32_t m_SecurityIDSource_state;
  FAST_Nullable<uint32_t > m_MDSecurityTradingStatus_state;
  FAST_Sequence<MDSnapshotFullRefresh_118_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("W", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_LastMsgSeqNumProcessed_state = 0;
    m_TotNumReports_state = 0;
    m_RptSeq_state = 0;
    m_MDBookType_state = 0;
    m_SecurityID_state = 0;
    m_SecurityIDSource_state = 8;
    m_MDSecurityTradingStatus_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    decode_field_mandatory<uint32_t >(buf, m_LastMsgSeqNumProcessed_state);
    decode_field_mandatory<uint32_t >(buf, m_TotNumReports_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<uint32_t >(buf, m_MDBookType_state);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_optional<uint32_t >(buf, m_MDSecurityTradingStatus_state.value, m_MDSecurityTradingStatus_state.has_value);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline uint32_t LastMsgSeqNumProcessed()
  {
    return m_LastMsgSeqNumProcessed_state;
  }
  
  inline uint32_t TotNumReports()
  {
    return m_TotNumReports_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline uint32_t MDBookType()
  {
    return m_MDBookType_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDSecurityTradingStatus()
  {
    return m_MDSecurityTradingStatus_state;
  }
  
  inline FAST_Sequence<MDSnapshotFullRefresh_118_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_119_MDEntries_Element
{
private:
  FAST_Nullable<uint32_t > m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_SecurityIDSource_state;
  FAST_Nullable<uint32_t > m_SecurityID_state;
  FAST_Nullable<uint32_t > m_RptSeq_state;
  FAST_Nullable<double > m_MDEntryPx_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state.has_value = false;
    m_MDPriceLevel_state.has_value = false;
    m_SecurityIDSource_state.assign_valid_value(8);
    m_SecurityID_state.has_value = false;
    m_RptSeq_state.has_value = false;
    m_MDEntryPx_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_field_optional<uint32_t >(buf, m_MDUpdateAction_state.value, m_MDUpdateAction_state.has_value);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    m_SecurityIDSource_state.has_value = pmap.read_next_pmap_entry();
    decode_field_optional<uint32_t >(buf, m_SecurityID_state.value, m_SecurityID_state.has_value);
    decode_field_optional<uint32_t >(buf, m_RptSeq_state.value, m_RptSeq_state.has_value);
    decode_field_optional<double >(buf, m_MDEntryPx_state.value, m_MDEntryPx_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
  }
  
  inline FAST_Nullable<uint32_t >& MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline FAST_Nullable<uint32_t >& RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_Nullable<double >& MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
};

class MDIncRefresh_119
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  FAST_Sequence<MDIncRefresh_119_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline FAST_Sequence<MDIncRefresh_119_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_120_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  uint32_t m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  double m_MDEntryPx_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  uint32_t m_MDEntryTime_state;
  int32_t m_MDEntrySize_state;
  FAST_String m_TradingSessionID;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_NumberOfOrders_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_mandatory<uint32_t >(buf, m_MDPriceLevel_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_mandatory<int32_t >(buf, m_MDEntrySize_state);
    perParseStringRepo.read_string_mandatory(buf, m_TradingSessionID);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline uint32_t MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline int32_t MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_String& TradingSessionID()
  {
    return m_TradingSessionID;
  }
};

class MDIncRefresh_120
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_120_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_120_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_121_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  uint32_t m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  uint32_t m_MDEntryTime_state;
  int32_t m_MDEntrySize_state;
  FAST_String m_QuoteCondition;
  FAST_String m_TradingSessionID;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_mandatory<uint32_t >(buf, m_MDPriceLevel_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_mandatory<int32_t >(buf, m_MDEntrySize_state);
    perParseStringRepo.read_string_mandatory(buf, m_QuoteCondition);
    perParseStringRepo.read_string_mandatory(buf, m_TradingSessionID);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline uint32_t MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline int32_t MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_String& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_String& TradingSessionID()
  {
    return m_TradingSessionID;
  }
};

class MDIncRefresh_121
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_121_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_121_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_122_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_TradeCondition;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_122
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_122_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_122_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDSecurityDefinition_123_Events_Element
{
private:
  FAST_Nullable<uint32_t > m_EventType_state;
  FAST_Nullable<uint64_t > m_EventDate_state;
  FAST_Nullable<uint64_t > m_EventTime_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_EventType_state.has_value = false;
    m_EventDate_state.has_value = false;
    m_EventTime_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_optional<uint32_t >(buf, m_EventType_state.value, m_EventType_state.has_value);
    decode_field_optional<uint64_t >(buf, m_EventDate_state.value, m_EventDate_state.has_value);
    decode_field_optional<uint64_t >(buf, m_EventTime_state.value, m_EventTime_state.has_value);
  }
  
  inline FAST_Nullable<uint32_t >& EventType()
  {
    return m_EventType_state;
  }
  
  inline FAST_Nullable<uint64_t >& EventDate()
  {
    return m_EventDate_state;
  }
  
  inline FAST_Nullable<uint64_t >& EventTime()
  {
    return m_EventTime_state;
  }
};

class MDSecurityDefinition_123_SecurityAltIDs_Element
{
private:
  FAST_Nullable<FAST_String> m_SecurityAltID;
  FAST_Nullable<uint32_t > m_SecurityAltIDSource_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_SecurityAltIDSource_state.assign_valid_value(8);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    perParseStringRepo.read_string_optional(buf, m_SecurityAltID);
    m_SecurityAltIDSource_state.has_value = pmap.read_next_pmap_entry();
  }
  
  inline FAST_Nullable<FAST_String>& SecurityAltID()
  {
    return m_SecurityAltID;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityAltIDSource()
  {
    return m_SecurityAltIDSource_state;
  }
};

class MDSecurityDefinition_123_MDFeedTypes_Element
{
private:
  FAST_String m_MDFeedType;
  uint32_t m_MarketDepth_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MarketDepth_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_MDFeedType);
    decode_field_mandatory<uint32_t >(buf, m_MarketDepth_state);
  }
  
  inline FAST_String& MDFeedType()
  {
    return m_MDFeedType;
  }
  
  inline uint32_t MarketDepth()
  {
    return m_MarketDepth_state;
  }
};

class MDSecurityDefinition_123_Underlyings_Element
{
private:
  FAST_String m_UnderlyingSymbol_state;
  uint32_t m_UnderlyingSecurityID_state;
  uint32_t m_UnderlyingSecurityIDSource_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("[N/A]", m_UnderlyingSymbol_state);
    m_UnderlyingSecurityID_state = 0;
    m_UnderlyingSecurityIDSource_state = 8;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_UnderlyingSecurityID_state);
  }
  
  inline FAST_String& UnderlyingSymbol()
  {
    return m_UnderlyingSymbol_state;
  }
  
  inline uint32_t UnderlyingSecurityID()
  {
    return m_UnderlyingSecurityID_state;
  }
  
  inline uint32_t UnderlyingSecurityIDSource()
  {
    return m_UnderlyingSecurityIDSource_state;
  }
};

class MDSecurityDefinition_123_InstrAttrib_Element
{
private:
  uint64_t m_InstrAttribType_state;
  FAST_Nullable<FAST_String> m_InstrAttribValue;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_InstrAttribType_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint64_t >(buf, m_InstrAttribType_state);
    perParseStringRepo.read_string_optional(buf, m_InstrAttribValue);
  }
  
  inline uint64_t InstrAttribType()
  {
    return m_InstrAttribType_state;
  }
  
  inline FAST_Nullable<FAST_String>& InstrAttribValue()
  {
    return m_InstrAttribValue;
  }
};

class MDSecurityDefinition_123_Legs_Element
{
private:
  FAST_String m_LegSymbol;
  uint32_t m_LegRatioQty_state;
  uint64_t m_LegSecurityID_state;
  FAST_Nullable<FAST_String> m_LegSecurityDesc;
  uint32_t m_LegSecurityIDSource_state;
  FAST_Nullable<FAST_String> m_LegSide;
  FAST_Nullable<FAST_String> m_LegSecurityGroup;
  FAST_Nullable<FAST_String> m_LegCFICode;
  FAST_Nullable<FAST_String> m_LegSecuritySubType;
  FAST_Nullable<FAST_String> m_LegCurrency;
  FAST_Nullable<uint64_t > m_LegMaturityMonthYear_state;
  FAST_Nullable<double > m_LegStrikePrice_state;
  FAST_Nullable<FAST_String> m_LegSecurityExchange;
  FAST_Nullable<FAST_String> m_LegStrikeCurrency;
  FAST_Nullable<double > m_LegPrice_state;
  FAST_Nullable<double > m_LegOptionDelta_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_LegRatioQty_state = 0;
    m_LegSecurityID_state = 0;
    m_LegSecurityIDSource_state = 8;
    m_LegMaturityMonthYear_state.has_value = false;
    m_LegStrikePrice_state.has_value = false;
    m_LegPrice_state.has_value = false;
    m_LegOptionDelta_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_LegSymbol);
    decode_field_mandatory<uint32_t >(buf, m_LegRatioQty_state);
    decode_field_mandatory<uint64_t >(buf, m_LegSecurityID_state);
    perParseStringRepo.read_string_optional(buf, m_LegSecurityDesc);
    perParseStringRepo.read_string_optional(buf, m_LegSide);
    perParseStringRepo.read_string_optional(buf, m_LegSecurityGroup);
    perParseStringRepo.read_string_optional(buf, m_LegCFICode);
    perParseStringRepo.read_string_optional(buf, m_LegSecuritySubType);
    perParseStringRepo.read_string_optional(buf, m_LegCurrency);
    decode_field_optional<uint64_t >(buf, m_LegMaturityMonthYear_state.value, m_LegMaturityMonthYear_state.has_value);
    decode_field_optional<double >(buf, m_LegStrikePrice_state.value, m_LegStrikePrice_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_LegSecurityExchange);
    perParseStringRepo.read_string_optional(buf, m_LegStrikeCurrency);
    decode_field_optional<double >(buf, m_LegPrice_state.value, m_LegPrice_state.has_value);
    decode_field_optional<double >(buf, m_LegOptionDelta_state.value, m_LegOptionDelta_state.has_value);
  }
  
  inline FAST_String& LegSymbol()
  {
    return m_LegSymbol;
  }
  
  inline uint32_t LegRatioQty()
  {
    return m_LegRatioQty_state;
  }
  
  inline uint64_t LegSecurityID()
  {
    return m_LegSecurityID_state;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecurityDesc()
  {
    return m_LegSecurityDesc;
  }
  
  inline uint32_t LegSecurityIDSource()
  {
    return m_LegSecurityIDSource_state;
  }
  
  inline FAST_Nullable<FAST_String>& LegSide()
  {
    return m_LegSide;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecurityGroup()
  {
    return m_LegSecurityGroup;
  }
  
  inline FAST_Nullable<FAST_String>& LegCFICode()
  {
    return m_LegCFICode;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecuritySubType()
  {
    return m_LegSecuritySubType;
  }
  
  inline FAST_Nullable<FAST_String>& LegCurrency()
  {
    return m_LegCurrency;
  }
  
  inline FAST_Nullable<uint64_t >& LegMaturityMonthYear()
  {
    return m_LegMaturityMonthYear_state;
  }
  
  inline FAST_Nullable<double >& LegStrikePrice()
  {
    return m_LegStrikePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecurityExchange()
  {
    return m_LegSecurityExchange;
  }
  
  inline FAST_Nullable<FAST_String>& LegStrikeCurrency()
  {
    return m_LegStrikeCurrency;
  }
  
  inline FAST_Nullable<double >& LegPrice()
  {
    return m_LegPrice_state;
  }
  
  inline FAST_Nullable<double >& LegOptionDelta()
  {
    return m_LegOptionDelta_state;
  }
};

class MDSecurityDefinition_123_LotTypeRules_Element
{
private:
  FAST_Nullable<FAST_String> m_LotType;
  FAST_Nullable<double > m_MinLotSize_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MinLotSize_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_optional(buf, m_LotType);
    decode_field_optional<double >(buf, m_MinLotSize_state.value, m_MinLotSize_state.has_value);
  }
  
  inline FAST_Nullable<FAST_String>& LotType()
  {
    return m_LotType;
  }
  
  inline FAST_Nullable<double >& MinLotSize()
  {
    return m_MinLotSize_state;
  }
};

class MDSecurityDefinition_123
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  FAST_Nullable<uint32_t > m_TotNumReports_state;
  FAST_Sequence<MDSecurityDefinition_123_Events_Element, 100> m_Events;
  FAST_Nullable<double > m_TradingReferencePrice_state;
  FAST_Nullable<FAST_String> m_SettlePriceType;
  FAST_Nullable<double > m_HighLimitPx_state;
  FAST_Nullable<double > m_LowLimitPx_state;
  FAST_Nullable<FAST_String> m_SecurityGroup;
  FAST_Nullable<FAST_String> m_Symbol;
  FAST_Nullable<FAST_String> m_SecurityDesc;
  FAST_Nullable<uint32_t > m_SecurityID_state;
  FAST_Nullable<uint32_t > m_SecurityIDSource_state;
  FAST_Nullable<FAST_String> m_CFICode;
  FAST_Nullable<FAST_String> m_UnderlyingProduct;
  FAST_Nullable<FAST_String> m_SecurityExchange;
  FAST_Nullable<FAST_String> m_PricingModel;
  FAST_Nullable<double > m_MinCabPrice_state;
  FAST_Sequence<MDSecurityDefinition_123_SecurityAltIDs_Element, 100> m_SecurityAltIDs;
  FAST_Nullable<uint32_t > m_ExpirationCycle_state;
  FAST_Nullable<FAST_String> m_UnitOfMeasureQty;
  FAST_Nullable<double > m_StrikePrice_state;
  FAST_Nullable<FAST_String> m_StrikeCurrency;
  FAST_Nullable<uint64_t > m_MinTradeVol_state;
  FAST_Nullable<uint64_t > m_MaxTradeVol_state;
  FAST_Nullable<FAST_String> m_Currency;
  FAST_Nullable<FAST_String> m_SettlCurrency;
  FAST_Sequence<MDSecurityDefinition_123_MDFeedTypes_Element, 100> m_MDFeedTypes;
  FAST_Nullable<FAST_String> m_MatchAlgo;
  FAST_Nullable<FAST_String> m_SecuritySubType;
  FAST_Sequence<MDSecurityDefinition_123_Underlyings_Element, 100> m_Underlyings;
  FAST_Nullable<FAST_String> m_MaxPriceVariation;
  FAST_Nullable<FAST_String> m_ImpliedMarketIndicator;
  FAST_Sequence<MDSecurityDefinition_123_InstrAttrib_Element, 100> m_InstrAttrib;
  FAST_Nullable<uint64_t > m_MaturityMonthYear_state;
  FAST_Nullable<double > m_MinPriceIncrement_state;
  FAST_Nullable<double > m_MinPriceIncrementAmount_state;
  FAST_Nullable<uint64_t > m_LastUpdateTime_state;
  FAST_Nullable<FAST_String> m_SecurityUpdateAction;
  FAST_Nullable<double > m_DisplayFactor_state;
  FAST_Sequence<MDSecurityDefinition_123_Legs_Element, 100> m_Legs;
  FAST_Nullable<FAST_String> m_ApplID;
  FAST_Nullable<FAST_String> m_UserDefinedInstrument_state;
  FAST_Nullable<double > m_PriceRatio_state;
  FAST_Nullable<uint32_t > m_ContractMultiplierType_state;
  FAST_Nullable<uint32_t > m_FlowScheduleType_state;
  FAST_Nullable<uint32_t > m_ContractMultiplier_state;
  FAST_Nullable<FAST_String> m_UnitofMeasure;
  FAST_Nullable<uint64_t > m_DecayQuantity_state;
  FAST_Nullable<uint64_t > m_DecayStartDate_state;
  FAST_Nullable<uint64_t > m_OriginalContractSize_state;
  FAST_Nullable<uint32_t > m_ClearedVolume_state;
  FAST_Nullable<uint32_t > m_OpenInterestQty_state;
  FAST_Nullable<uint32_t > m_TradingReferenceDate_state;
  FAST_Sequence<MDSecurityDefinition_123_LotTypeRules_Element, 100> m_LotTypeRules;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("d", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TotNumReports_state.has_value = false;
    m_TradingReferencePrice_state.has_value = false;
    m_HighLimitPx_state.has_value = false;
    m_LowLimitPx_state.has_value = false;
    m_SecurityID_state.has_value = false;
    m_SecurityIDSource_state.assign_valid_value(8);
    m_MinCabPrice_state.has_value = false;
    m_ExpirationCycle_state.has_value = false;
    m_StrikePrice_state.has_value = false;
    m_MinTradeVol_state.has_value = false;
    m_MaxTradeVol_state.has_value = false;
    m_MaturityMonthYear_state.has_value = false;
    m_MinPriceIncrement_state.has_value = false;
    m_MinPriceIncrementAmount_state.has_value = false;
    m_LastUpdateTime_state.has_value = false;
    m_DisplayFactor_state.has_value = false;
    stringRepo.assign_next_const_string_state<FAST_String>("Y", m_UserDefinedInstrument_state.value);
    m_PriceRatio_state.has_value = false;
    m_ContractMultiplierType_state.has_value = false;
    m_FlowScheduleType_state.has_value = false;
    m_ContractMultiplier_state.has_value = false;
    m_DecayQuantity_state.has_value = false;
    m_DecayStartDate_state.has_value = false;
    m_OriginalContractSize_state.has_value = false;
    m_ClearedVolume_state.has_value = false;
    m_OpenInterestQty_state.has_value = false;
    m_TradingReferenceDate_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_optional<uint32_t >(buf, m_TotNumReports_state.value, m_TotNumReports_state.has_value);
    decode_optional_sequence(buf, m_Events, perParseStringRepo, globalDictionary);
    decode_field_optional<double >(buf, m_TradingReferencePrice_state.value, m_TradingReferencePrice_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_SettlePriceType);
    decode_field_optional<double >(buf, m_HighLimitPx_state.value, m_HighLimitPx_state.has_value);
    decode_field_optional<double >(buf, m_LowLimitPx_state.value, m_LowLimitPx_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_SecurityGroup);
    perParseStringRepo.read_string_optional(buf, m_Symbol);
    perParseStringRepo.read_string_optional(buf, m_SecurityDesc);
    decode_field_optional<uint32_t >(buf, m_SecurityID_state.value, m_SecurityID_state.has_value);
    m_SecurityIDSource_state.has_value = pmap.read_next_pmap_entry();
    perParseStringRepo.read_string_optional(buf, m_CFICode);
    perParseStringRepo.read_string_optional(buf, m_UnderlyingProduct);
    perParseStringRepo.read_string_optional(buf, m_SecurityExchange);
    perParseStringRepo.read_string_optional(buf, m_PricingModel);
    decode_field_optional<double >(buf, m_MinCabPrice_state.value, m_MinCabPrice_state.has_value);
    decode_optional_sequence(buf, m_SecurityAltIDs, perParseStringRepo, globalDictionary);
    decode_field_optional<uint32_t >(buf, m_ExpirationCycle_state.value, m_ExpirationCycle_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_UnitOfMeasureQty);
    decode_field_optional<double >(buf, m_StrikePrice_state.value, m_StrikePrice_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_StrikeCurrency);
    decode_field_optional<uint64_t >(buf, m_MinTradeVol_state.value, m_MinTradeVol_state.has_value);
    decode_field_optional<uint64_t >(buf, m_MaxTradeVol_state.value, m_MaxTradeVol_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_Currency);
    perParseStringRepo.read_string_optional(buf, m_SettlCurrency);
    decode_optional_sequence(buf, m_MDFeedTypes, perParseStringRepo, globalDictionary);
    perParseStringRepo.read_string_optional(buf, m_MatchAlgo);
    perParseStringRepo.read_string_optional(buf, m_SecuritySubType);
    decode_optional_sequence(buf, m_Underlyings, perParseStringRepo, globalDictionary);
    perParseStringRepo.read_string_optional(buf, m_MaxPriceVariation);
    perParseStringRepo.read_string_optional(buf, m_ImpliedMarketIndicator);
    decode_optional_sequence(buf, m_InstrAttrib, perParseStringRepo, globalDictionary);
    decode_field_optional<uint64_t >(buf, m_MaturityMonthYear_state.value, m_MaturityMonthYear_state.has_value);
    decode_field_optional<double >(buf, m_MinPriceIncrement_state.value, m_MinPriceIncrement_state.has_value);
    decode_field_optional<double >(buf, m_MinPriceIncrementAmount_state.value, m_MinPriceIncrementAmount_state.has_value);
    decode_field_optional<uint64_t >(buf, m_LastUpdateTime_state.value, m_LastUpdateTime_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_SecurityUpdateAction);
    decode_field_optional<double >(buf, m_DisplayFactor_state.value, m_DisplayFactor_state.has_value);
    decode_optional_sequence(buf, m_Legs, perParseStringRepo, globalDictionary);
    perParseStringRepo.read_string_optional(buf, m_ApplID);
    m_UserDefinedInstrument_state.has_value = pmap.read_next_pmap_entry();
    decode_field_optional<double >(buf, m_PriceRatio_state.value, m_PriceRatio_state.has_value);
    decode_field_optional<uint32_t >(buf, m_ContractMultiplierType_state.value, m_ContractMultiplierType_state.has_value);
    decode_field_optional<uint32_t >(buf, m_FlowScheduleType_state.value, m_FlowScheduleType_state.has_value);
    decode_field_optional<uint32_t >(buf, m_ContractMultiplier_state.value, m_ContractMultiplier_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_UnitofMeasure);
    decode_field_optional<uint64_t >(buf, m_DecayQuantity_state.value, m_DecayQuantity_state.has_value);
    decode_field_optional<uint64_t >(buf, m_DecayStartDate_state.value, m_DecayStartDate_state.has_value);
    decode_field_optional<uint64_t >(buf, m_OriginalContractSize_state.value, m_OriginalContractSize_state.has_value);
    decode_field_optional<uint32_t >(buf, m_ClearedVolume_state.value, m_ClearedVolume_state.has_value);
    decode_field_optional<uint32_t >(buf, m_OpenInterestQty_state.value, m_OpenInterestQty_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradingReferenceDate_state.value, m_TradingReferenceDate_state.has_value);
    decode_optional_sequence(buf, m_LotTypeRules, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline FAST_Nullable<uint32_t >& TotNumReports()
  {
    return m_TotNumReports_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_123_Events_Element, 100>& Events()
  {
    return m_Events;
  }
  
  inline FAST_Nullable<double >& TradingReferencePrice()
  {
    return m_TradingReferencePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& SettlePriceType()
  {
    return m_SettlePriceType;
  }
  
  inline FAST_Nullable<double >& HighLimitPx()
  {
    return m_HighLimitPx_state;
  }
  
  inline FAST_Nullable<double >& LowLimitPx()
  {
    return m_LowLimitPx_state;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityGroup()
  {
    return m_SecurityGroup;
  }
  
  inline FAST_Nullable<FAST_String>& Symbol()
  {
    return m_Symbol;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityDesc()
  {
    return m_SecurityDesc;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Nullable<FAST_String>& CFICode()
  {
    return m_CFICode;
  }
  
  inline FAST_Nullable<FAST_String>& UnderlyingProduct()
  {
    return m_UnderlyingProduct;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityExchange()
  {
    return m_SecurityExchange;
  }
  
  inline FAST_Nullable<FAST_String>& PricingModel()
  {
    return m_PricingModel;
  }
  
  inline FAST_Nullable<double >& MinCabPrice()
  {
    return m_MinCabPrice_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_123_SecurityAltIDs_Element, 100>& SecurityAltIDs()
  {
    return m_SecurityAltIDs;
  }
  
  inline FAST_Nullable<uint32_t >& ExpirationCycle()
  {
    return m_ExpirationCycle_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnitOfMeasureQty()
  {
    return m_UnitOfMeasureQty;
  }
  
  inline FAST_Nullable<double >& StrikePrice()
  {
    return m_StrikePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& StrikeCurrency()
  {
    return m_StrikeCurrency;
  }
  
  inline FAST_Nullable<uint64_t >& MinTradeVol()
  {
    return m_MinTradeVol_state;
  }
  
  inline FAST_Nullable<uint64_t >& MaxTradeVol()
  {
    return m_MaxTradeVol_state;
  }
  
  inline FAST_Nullable<FAST_String>& Currency()
  {
    return m_Currency;
  }
  
  inline FAST_Nullable<FAST_String>& SettlCurrency()
  {
    return m_SettlCurrency;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_123_MDFeedTypes_Element, 100>& MDFeedTypes()
  {
    return m_MDFeedTypes;
  }
  
  inline FAST_Nullable<FAST_String>& MatchAlgo()
  {
    return m_MatchAlgo;
  }
  
  inline FAST_Nullable<FAST_String>& SecuritySubType()
  {
    return m_SecuritySubType;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_123_Underlyings_Element, 100>& Underlyings()
  {
    return m_Underlyings;
  }
  
  inline FAST_Nullable<FAST_String>& MaxPriceVariation()
  {
    return m_MaxPriceVariation;
  }
  
  inline FAST_Nullable<FAST_String>& ImpliedMarketIndicator()
  {
    return m_ImpliedMarketIndicator;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_123_InstrAttrib_Element, 100>& InstrAttrib()
  {
    return m_InstrAttrib;
  }
  
  inline FAST_Nullable<uint64_t >& MaturityMonthYear()
  {
    return m_MaturityMonthYear_state;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrement()
  {
    return m_MinPriceIncrement_state;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrementAmount()
  {
    return m_MinPriceIncrementAmount_state;
  }
  
  inline FAST_Nullable<uint64_t >& LastUpdateTime()
  {
    return m_LastUpdateTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityUpdateAction()
  {
    return m_SecurityUpdateAction;
  }
  
  inline FAST_Nullable<double >& DisplayFactor()
  {
    return m_DisplayFactor_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_123_Legs_Element, 100>& Legs()
  {
    return m_Legs;
  }
  
  inline FAST_Nullable<FAST_String>& ApplID()
  {
    return m_ApplID;
  }
  
  inline FAST_Nullable<FAST_String>& UserDefinedInstrument()
  {
    return m_UserDefinedInstrument_state;
  }
  
  inline FAST_Nullable<double >& PriceRatio()
  {
    return m_PriceRatio_state;
  }
  
  inline FAST_Nullable<uint32_t >& ContractMultiplierType()
  {
    return m_ContractMultiplierType_state;
  }
  
  inline FAST_Nullable<uint32_t >& FlowScheduleType()
  {
    return m_FlowScheduleType_state;
  }
  
  inline FAST_Nullable<uint32_t >& ContractMultiplier()
  {
    return m_ContractMultiplier_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnitofMeasure()
  {
    return m_UnitofMeasure;
  }
  
  inline FAST_Nullable<uint64_t >& DecayQuantity()
  {
    return m_DecayQuantity_state;
  }
  
  inline FAST_Nullable<uint64_t >& DecayStartDate()
  {
    return m_DecayStartDate_state;
  }
  
  inline FAST_Nullable<uint64_t >& OriginalContractSize()
  {
    return m_OriginalContractSize_state;
  }
  
  inline FAST_Nullable<uint32_t >& ClearedVolume()
  {
    return m_ClearedVolume_state;
  }
  
  inline FAST_Nullable<uint32_t >& OpenInterestQty()
  {
    return m_OpenInterestQty_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradingReferenceDate()
  {
    return m_TradingReferenceDate_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_123_LotTypeRules_Element, 100>& LotTypeRules()
  {
    return m_LotTypeRules;
  }
};

class MDLogon_124
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_String m_ApplID_state;
  uint32_t m_EncryptMethod_state;
  uint32_t m_HeartbeatInt_state;
  FAST_String m_DefaultApplVerID_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("A", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("REPLAY", m_ApplID_state);
    m_EncryptMethod_state = 0;
    m_HeartbeatInt_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_DefaultApplVerID_state);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    decode_field_mandatory<uint32_t >(buf, m_HeartbeatInt_state);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_String& ApplID()
  {
    return m_ApplID_state;
  }
  
  inline uint32_t EncryptMethod()
  {
    return m_EncryptMethod_state;
  }
  
  inline uint32_t HeartbeatInt()
  {
    return m_HeartbeatInt_state;
  }
  
  inline FAST_String& DefaultApplVerID()
  {
    return m_DefaultApplVerID_state;
  }
};

class MDIncRefresh_125_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<double > m_MDEntryPx_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  FAST_Nullable<FAST_String> m_FixingBracket;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state.has_value = false;
    m_MDEntrySize_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<double >(buf, m_MDEntryPx_state.value, m_MDEntryPx_state.has_value);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_FixingBracket);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<double >& MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline FAST_Nullable<FAST_String>& FixingBracket()
  {
    return m_FixingBracket;
  }
};

class MDIncRefresh_125
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_125_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_125_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDLogout_126
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_String m_ApplID_state;
  FAST_Nullable<FAST_String> m_Text;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("5", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("REPLAY", m_ApplID_state);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_Text);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_String& ApplID()
  {
    return m_ApplID_state;
  }
  
  inline FAST_Nullable<FAST_String>& Text()
  {
    return m_Text;
  }
};

class MDSecurityStatus_127
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  FAST_Nullable<uint32_t > m_SecurityID_state;
  FAST_Nullable<uint32_t > m_SecurityIDSource_state;
  uint32_t m_TradeDate_state;
  FAST_Nullable<double > m_HighPx_state;
  FAST_Nullable<double > m_LowPx_state;
  FAST_Nullable<FAST_String> m_Symbol;
  FAST_Nullable<uint32_t > m_SecurityTradingStatus_state;
  FAST_Nullable<uint32_t > m_HaltReason_state;
  FAST_Nullable<uint32_t > m_SecurityTradingEvent_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("f", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_SecurityID_state.has_value = false;
    m_SecurityIDSource_state.assign_valid_value(8);
    m_TradeDate_state = 0;
    m_HighPx_state.has_value = false;
    m_LowPx_state.has_value = false;
    m_SecurityTradingStatus_state.has_value = false;
    m_HaltReason_state.has_value = false;
    m_SecurityTradingEvent_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_optional<uint32_t >(buf, m_SecurityID_state.value, m_SecurityID_state.has_value);
    m_SecurityIDSource_state.has_value = pmap.read_next_pmap_entry();
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_field_optional<double >(buf, m_HighPx_state.value, m_HighPx_state.has_value);
    decode_field_optional<double >(buf, m_LowPx_state.value, m_LowPx_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_Symbol);
    decode_field_optional<uint32_t >(buf, m_SecurityTradingStatus_state.value, m_SecurityTradingStatus_state.has_value);
    decode_field_optional<uint32_t >(buf, m_HaltReason_state.value, m_HaltReason_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SecurityTradingEvent_state.value, m_SecurityTradingEvent_state.has_value);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Nullable<double >& HighPx()
  {
    return m_HighPx_state;
  }
  
  inline FAST_Nullable<double >& LowPx()
  {
    return m_LowPx_state;
  }
  
  inline FAST_Nullable<FAST_String>& Symbol()
  {
    return m_Symbol;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityTradingStatus()
  {
    return m_SecurityTradingStatus_state;
  }
  
  inline FAST_Nullable<uint32_t >& HaltReason()
  {
    return m_HaltReason_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityTradingEvent()
  {
    return m_SecurityTradingEvent_state;
  }
};

class MDQuoteRequest_128_RelatedSym_Element
{
private:
  FAST_String m_Symbol_state;
  FAST_Nullable<uint64_t > m_OrderQty_state;
  FAST_Nullable<uint32_t > m_Side_state;
  uint64_t m_TransactTime_state;
  uint32_t m_QuoteType_state;
  uint32_t m_SecurityID_state;
  uint32_t m_SecurityIDSource_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("[N/A]", m_Symbol_state);
    m_OrderQty_state.has_value = false;
    m_Side_state.has_value = false;
    m_TransactTime_state = 0;
    m_QuoteType_state = 0;
    m_SecurityID_state = 0;
    m_SecurityIDSource_state = 8;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_optional<uint64_t >(buf, m_OrderQty_state.value, m_OrderQty_state.has_value);
    decode_field_optional<uint32_t >(buf, m_Side_state.value, m_Side_state.has_value);
    decode_field_mandatory<uint64_t >(buf, m_TransactTime_state);
    decode_field_mandatory<uint32_t >(buf, m_QuoteType_state);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
  }
  
  inline FAST_String& Symbol()
  {
    return m_Symbol_state;
  }
  
  inline FAST_Nullable<uint64_t >& OrderQty()
  {
    return m_OrderQty_state;
  }
  
  inline FAST_Nullable<uint32_t >& Side()
  {
    return m_Side_state;
  }
  
  inline uint64_t TransactTime()
  {
    return m_TransactTime_state;
  }
  
  inline uint32_t QuoteType()
  {
    return m_QuoteType_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
};

class MDQuoteRequest_128
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  FAST_Nullable<FAST_String> m_QuoteReqID;
  FAST_Sequence<MDQuoteRequest_128_RelatedSym_Element, 100> m_RelatedSym;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("R", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    perParseStringRepo.read_string_optional(buf, m_QuoteReqID);
    decode_mandatory_sequence(buf, m_RelatedSym, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteReqID()
  {
    return m_QuoteReqID;
  }
  
  inline FAST_Sequence<MDQuoteRequest_128_RelatedSym_Element, 100>& RelatedSym()
  {
    return m_RelatedSym;
  }
};

class MDIncRefresh_129_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  double m_MDEntryPx_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_NumberOfOrders_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
};

class MDIncRefresh_129
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_129_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_129_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDSecurityStatus_130
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  FAST_Nullable<uint32_t > m_SecurityID_state;
  FAST_Nullable<uint32_t > m_SecurityIDSource_state;
  FAST_Nullable<uint32_t > m_TradeDate_state;
  FAST_Nullable<double > m_HighPx_state;
  FAST_Nullable<double > m_LowPx_state;
  FAST_Nullable<FAST_String> m_Symbol;
  FAST_Nullable<uint32_t > m_SecurityTradingStatus_state;
  FAST_Nullable<uint32_t > m_HaltReason_state;
  FAST_Nullable<uint32_t > m_SecurityTradingEvent_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("f", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_SecurityID_state.has_value = false;
    m_SecurityIDSource_state.assign_valid_value(8);
    m_TradeDate_state.has_value = false;
    m_HighPx_state.has_value = false;
    m_LowPx_state.has_value = false;
    m_SecurityTradingStatus_state.has_value = false;
    m_HaltReason_state.has_value = false;
    m_SecurityTradingEvent_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_optional<uint32_t >(buf, m_SecurityID_state.value, m_SecurityID_state.has_value);
    m_SecurityIDSource_state.has_value = pmap.read_next_pmap_entry();
    decode_field_optional<uint32_t >(buf, m_TradeDate_state.value, m_TradeDate_state.has_value);
    decode_field_optional<double >(buf, m_HighPx_state.value, m_HighPx_state.has_value);
    decode_field_optional<double >(buf, m_LowPx_state.value, m_LowPx_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_Symbol);
    decode_field_optional<uint32_t >(buf, m_SecurityTradingStatus_state.value, m_SecurityTradingStatus_state.has_value);
    decode_field_optional<uint32_t >(buf, m_HaltReason_state.value, m_HaltReason_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SecurityTradingEvent_state.value, m_SecurityTradingEvent_state.has_value);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Nullable<double >& HighPx()
  {
    return m_HighPx_state;
  }
  
  inline FAST_Nullable<double >& LowPx()
  {
    return m_LowPx_state;
  }
  
  inline FAST_Nullable<FAST_String>& Symbol()
  {
    return m_Symbol;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityTradingStatus()
  {
    return m_SecurityTradingStatus_state;
  }
  
  inline FAST_Nullable<uint32_t >& HaltReason()
  {
    return m_HaltReason_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityTradingEvent()
  {
    return m_SecurityTradingEvent_state;
  }
};

class MDIncRefresh_131_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_131
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_131_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_131_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDNewsMessage_132_LinesOfText_Element
{
private:
  FAST_String m_text;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_text);
  }
  
  inline FAST_String& text()
  {
    return m_text;
  }
};

class MDNewsMessage_132
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_String m_Headline;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  FAST_Sequence<MDNewsMessage_132_LinesOfText_Element, 100> m_LinesOfText;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("B", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_mandatory(buf, m_Headline);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_mandatory_sequence(buf, m_LinesOfText, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_String& Headline()
  {
    return m_Headline;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline FAST_Sequence<MDNewsMessage_132_LinesOfText_Element, 100>& LinesOfText()
  {
    return m_LinesOfText;
  }
};

class MDHeartbeat_133
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("0", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
};

class MDIncRefresh_134_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_Nullable<double > m_MDEntryPx_state;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_NumberOfOrders_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_MDEntryPx_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    decode_field_optional<double >(buf, m_MDEntryPx_state.value, m_MDEntryPx_state.has_value);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_Nullable<double >& MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_134
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_134_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_134_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_135_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  double m_MDEntryPx_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_NumberOfOrders_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_135
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_135_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_135_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_136_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  uint32_t m_MDEntryTime_state;
  double m_MDEntryPx_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryTime_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_136
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_136_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_136_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_138_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<double > m_MDEntryPx_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_optional<double >(buf, m_MDEntryPx_state.value, m_MDEntryPx_state.has_value);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<double >& MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_138
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_138_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_138_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDSecurityDefinition_139_Events_Element
{
private:
  FAST_Nullable<uint32_t > m_EventType_state;
  FAST_Nullable<uint64_t > m_EventDate_state;
  FAST_Nullable<uint64_t > m_EventTime_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_EventType_state.has_value = false;
    m_EventDate_state.has_value = false;
    m_EventTime_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_optional<uint32_t >(buf, m_EventType_state.value, m_EventType_state.has_value);
    decode_field_optional<uint64_t >(buf, m_EventDate_state.value, m_EventDate_state.has_value);
    decode_field_optional<uint64_t >(buf, m_EventTime_state.value, m_EventTime_state.has_value);
  }
  
  inline FAST_Nullable<uint32_t >& EventType()
  {
    return m_EventType_state;
  }
  
  inline FAST_Nullable<uint64_t >& EventDate()
  {
    return m_EventDate_state;
  }
  
  inline FAST_Nullable<uint64_t >& EventTime()
  {
    return m_EventTime_state;
  }
};

class MDSecurityDefinition_139_SecurityAltIDs_Element
{
private:
  FAST_Nullable<FAST_String> m_SecurityAltID;
  FAST_Nullable<uint32_t > m_SecurityAltIDSource_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_SecurityAltIDSource_state.assign_valid_value(8);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    perParseStringRepo.read_string_optional(buf, m_SecurityAltID);
    m_SecurityAltIDSource_state.has_value = pmap.read_next_pmap_entry();
  }
  
  inline FAST_Nullable<FAST_String>& SecurityAltID()
  {
    return m_SecurityAltID;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityAltIDSource()
  {
    return m_SecurityAltIDSource_state;
  }
};

class MDSecurityDefinition_139_MDFeedTypes_Element
{
private:
  FAST_String m_MDFeedType;
  uint32_t m_MarketDepth_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MarketDepth_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_MDFeedType);
    decode_field_mandatory<uint32_t >(buf, m_MarketDepth_state);
  }
  
  inline FAST_String& MDFeedType()
  {
    return m_MDFeedType;
  }
  
  inline uint32_t MarketDepth()
  {
    return m_MarketDepth_state;
  }
};

class MDSecurityDefinition_139_Underlyings_Element
{
private:
  FAST_String m_UnderlyingSymbol;
  uint32_t m_UnderlyingSecurityID_state;
  uint32_t m_UnderlyingSecurityIDSource_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_UnderlyingSecurityID_state = 0;
    m_UnderlyingSecurityIDSource_state = 8;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_UnderlyingSymbol);
    decode_field_mandatory<uint32_t >(buf, m_UnderlyingSecurityID_state);
  }
  
  inline FAST_String& UnderlyingSymbol()
  {
    return m_UnderlyingSymbol;
  }
  
  inline uint32_t UnderlyingSecurityID()
  {
    return m_UnderlyingSecurityID_state;
  }
  
  inline uint32_t UnderlyingSecurityIDSource()
  {
    return m_UnderlyingSecurityIDSource_state;
  }
};

class MDSecurityDefinition_139_InstrAttrib_Element
{
private:
  uint64_t m_InstrAttribType_state;
  FAST_Nullable<FAST_String> m_InstrAttribValue;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_InstrAttribType_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint64_t >(buf, m_InstrAttribType_state);
    perParseStringRepo.read_string_optional(buf, m_InstrAttribValue);
  }
  
  inline uint64_t InstrAttribType()
  {
    return m_InstrAttribType_state;
  }
  
  inline FAST_Nullable<FAST_String>& InstrAttribValue()
  {
    return m_InstrAttribValue;
  }
};

class MDSecurityDefinition_139_Legs_Element
{
private:
  FAST_String m_LegSymbol;
  FAST_Nullable<uint32_t > m_LegRatioQty_state;
  uint64_t m_LegSecurityID_state;
  FAST_Nullable<FAST_String> m_LegSecurityDesc;
  uint32_t m_LegSecurityIDSource_state;
  FAST_Nullable<FAST_String> m_LegSide;
  FAST_Nullable<FAST_String> m_LegSecurityGroup;
  FAST_Nullable<FAST_String> m_LegCFICode;
  FAST_Nullable<FAST_String> m_LegSecuritySubType;
  FAST_Nullable<FAST_String> m_LegCurrency;
  FAST_Nullable<uint64_t > m_LegMaturityMonthYear_state;
  FAST_Nullable<double > m_LegStrikePrice_state;
  FAST_Nullable<FAST_String> m_LegSecurityExchange;
  FAST_Nullable<FAST_String> m_LegStrikeCurrency;
  FAST_Nullable<double > m_LegPrice_state;
  FAST_Nullable<double > m_LegOptionDelta_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_LegRatioQty_state.has_value = false;
    m_LegSecurityID_state = 0;
    m_LegSecurityIDSource_state = 8;
    m_LegMaturityMonthYear_state.has_value = false;
    m_LegStrikePrice_state.has_value = false;
    m_LegPrice_state.has_value = false;
    m_LegOptionDelta_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_LegSymbol);
    decode_field_optional<uint32_t >(buf, m_LegRatioQty_state.value, m_LegRatioQty_state.has_value);
    decode_field_mandatory<uint64_t >(buf, m_LegSecurityID_state);
    perParseStringRepo.read_string_optional(buf, m_LegSecurityDesc);
    perParseStringRepo.read_string_optional(buf, m_LegSide);
    perParseStringRepo.read_string_optional(buf, m_LegSecurityGroup);
    perParseStringRepo.read_string_optional(buf, m_LegCFICode);
    perParseStringRepo.read_string_optional(buf, m_LegSecuritySubType);
    perParseStringRepo.read_string_optional(buf, m_LegCurrency);
    decode_field_optional<uint64_t >(buf, m_LegMaturityMonthYear_state.value, m_LegMaturityMonthYear_state.has_value);
    decode_field_optional<double >(buf, m_LegStrikePrice_state.value, m_LegStrikePrice_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_LegSecurityExchange);
    perParseStringRepo.read_string_optional(buf, m_LegStrikeCurrency);
    decode_field_optional<double >(buf, m_LegPrice_state.value, m_LegPrice_state.has_value);
    decode_field_optional<double >(buf, m_LegOptionDelta_state.value, m_LegOptionDelta_state.has_value);
  }
  
  inline FAST_String& LegSymbol()
  {
    return m_LegSymbol;
  }
  
  inline FAST_Nullable<uint32_t >& LegRatioQty()
  {
    return m_LegRatioQty_state;
  }
  
  inline uint64_t LegSecurityID()
  {
    return m_LegSecurityID_state;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecurityDesc()
  {
    return m_LegSecurityDesc;
  }
  
  inline uint32_t LegSecurityIDSource()
  {
    return m_LegSecurityIDSource_state;
  }
  
  inline FAST_Nullable<FAST_String>& LegSide()
  {
    return m_LegSide;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecurityGroup()
  {
    return m_LegSecurityGroup;
  }
  
  inline FAST_Nullable<FAST_String>& LegCFICode()
  {
    return m_LegCFICode;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecuritySubType()
  {
    return m_LegSecuritySubType;
  }
  
  inline FAST_Nullable<FAST_String>& LegCurrency()
  {
    return m_LegCurrency;
  }
  
  inline FAST_Nullable<uint64_t >& LegMaturityMonthYear()
  {
    return m_LegMaturityMonthYear_state;
  }
  
  inline FAST_Nullable<double >& LegStrikePrice()
  {
    return m_LegStrikePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecurityExchange()
  {
    return m_LegSecurityExchange;
  }
  
  inline FAST_Nullable<FAST_String>& LegStrikeCurrency()
  {
    return m_LegStrikeCurrency;
  }
  
  inline FAST_Nullable<double >& LegPrice()
  {
    return m_LegPrice_state;
  }
  
  inline FAST_Nullable<double >& LegOptionDelta()
  {
    return m_LegOptionDelta_state;
  }
};

class MDSecurityDefinition_139_LotTypeRules_Element
{
private:
  FAST_Nullable<FAST_String> m_LotType;
  FAST_Nullable<double > m_MinLotSize_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MinLotSize_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_optional(buf, m_LotType);
    decode_field_optional<double >(buf, m_MinLotSize_state.value, m_MinLotSize_state.has_value);
  }
  
  inline FAST_Nullable<FAST_String>& LotType()
  {
    return m_LotType;
  }
  
  inline FAST_Nullable<double >& MinLotSize()
  {
    return m_MinLotSize_state;
  }
};

class MDSecurityDefinition_139
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  FAST_Nullable<uint32_t > m_TotNumReports_state;
  FAST_Sequence<MDSecurityDefinition_139_Events_Element, 100> m_Events;
  FAST_Nullable<double > m_TradingReferencePrice_state;
  FAST_Nullable<FAST_String> m_SettlePriceType;
  FAST_Nullable<double > m_HighLimitPx_state;
  FAST_Nullable<double > m_LowLimitPx_state;
  FAST_Nullable<FAST_String> m_SecurityGroup;
  FAST_Nullable<FAST_String> m_Symbol;
  FAST_Nullable<FAST_String> m_SecurityDesc;
  FAST_Nullable<uint32_t > m_SecurityID_state;
  FAST_Nullable<uint32_t > m_SecurityIDSource_state;
  FAST_Nullable<FAST_String> m_CFICode;
  FAST_Nullable<FAST_String> m_UnderlyingProduct;
  FAST_Nullable<FAST_String> m_SecurityExchange;
  FAST_Nullable<FAST_String> m_PricingModel;
  FAST_Nullable<double > m_MinCabPrice_state;
  FAST_Sequence<MDSecurityDefinition_139_SecurityAltIDs_Element, 100> m_SecurityAltIDs;
  FAST_Nullable<uint32_t > m_ExpirationCycle_state;
  FAST_Nullable<FAST_String> m_UnitOfMeasureQty;
  FAST_Nullable<double > m_StrikePrice_state;
  FAST_Nullable<FAST_String> m_StrikeCurrency;
  FAST_Nullable<uint64_t > m_MinTradeVol_state;
  FAST_Nullable<uint64_t > m_MaxTradeVol_state;
  FAST_Nullable<FAST_String> m_Currency;
  FAST_Nullable<FAST_String> m_SettlCurrency;
  FAST_Sequence<MDSecurityDefinition_139_MDFeedTypes_Element, 100> m_MDFeedTypes;
  FAST_Nullable<FAST_String> m_MatchAlgo;
  FAST_Nullable<FAST_String> m_SecuritySubType;
  FAST_Sequence<MDSecurityDefinition_139_Underlyings_Element, 100> m_Underlyings;
  FAST_Nullable<FAST_String> m_MaxPriceVariation;
  FAST_Nullable<FAST_String> m_ImpliedMarketIndicator;
  FAST_Sequence<MDSecurityDefinition_139_InstrAttrib_Element, 100> m_InstrAttrib;
  FAST_Nullable<uint64_t > m_MaturityMonthYear_state;
  FAST_Nullable<double > m_MinPriceIncrement_state;
  FAST_Nullable<double > m_MinPriceIncrementAmount_state;
  FAST_Nullable<uint64_t > m_LastUpdateTime_state;
  FAST_Nullable<FAST_String> m_SecurityUpdateAction;
  FAST_Nullable<double > m_DisplayFactor_state;
  FAST_Sequence<MDSecurityDefinition_139_Legs_Element, 100> m_Legs;
  FAST_Nullable<FAST_String> m_ApplID;
  FAST_Nullable<FAST_String> m_UserDefinedInstrument_state;
  FAST_Nullable<double > m_PriceRatio_state;
  FAST_Nullable<uint32_t > m_ContractMultiplierType_state;
  FAST_Nullable<uint32_t > m_FlowScheduleType_state;
  FAST_Nullable<double > m_ContractMultiplier_state;
  FAST_Nullable<FAST_String> m_UnitofMeasure;
  FAST_Nullable<uint64_t > m_DecayQuantity_state;
  FAST_Nullable<uint64_t > m_DecayStartDate_state;
  FAST_Nullable<uint64_t > m_OriginalContractSize_state;
  FAST_Nullable<uint32_t > m_ClearedVolume_state;
  FAST_Nullable<uint32_t > m_OpenInterestQty_state;
  FAST_Nullable<uint32_t > m_TradingReferenceDate_state;
  FAST_Sequence<MDSecurityDefinition_139_LotTypeRules_Element, 100> m_LotTypeRules;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("d", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TotNumReports_state.has_value = false;
    m_TradingReferencePrice_state.has_value = false;
    m_HighLimitPx_state.has_value = false;
    m_LowLimitPx_state.has_value = false;
    m_SecurityID_state.has_value = false;
    m_SecurityIDSource_state.assign_valid_value(8);
    m_MinCabPrice_state.has_value = false;
    m_ExpirationCycle_state.has_value = false;
    m_StrikePrice_state.has_value = false;
    m_MinTradeVol_state.has_value = false;
    m_MaxTradeVol_state.has_value = false;
    m_MaturityMonthYear_state.has_value = false;
    m_MinPriceIncrement_state.has_value = false;
    m_MinPriceIncrementAmount_state.has_value = false;
    m_LastUpdateTime_state.has_value = false;
    m_DisplayFactor_state.has_value = false;
    stringRepo.assign_next_const_string_state<FAST_String>("Y", m_UserDefinedInstrument_state.value);
    m_PriceRatio_state.has_value = false;
    m_ContractMultiplierType_state.has_value = false;
    m_FlowScheduleType_state.has_value = false;
    m_ContractMultiplier_state.has_value = false;
    m_DecayQuantity_state.has_value = false;
    m_DecayStartDate_state.has_value = false;
    m_OriginalContractSize_state.has_value = false;
    m_ClearedVolume_state.has_value = false;
    m_OpenInterestQty_state.has_value = false;
    m_TradingReferenceDate_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_optional<uint32_t >(buf, m_TotNumReports_state.value, m_TotNumReports_state.has_value);
    decode_optional_sequence(buf, m_Events, perParseStringRepo, globalDictionary);
    decode_field_optional<double >(buf, m_TradingReferencePrice_state.value, m_TradingReferencePrice_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_SettlePriceType);
    decode_field_optional<double >(buf, m_HighLimitPx_state.value, m_HighLimitPx_state.has_value);
    decode_field_optional<double >(buf, m_LowLimitPx_state.value, m_LowLimitPx_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_SecurityGroup);
    perParseStringRepo.read_string_optional(buf, m_Symbol);
    perParseStringRepo.read_string_optional(buf, m_SecurityDesc);
    decode_field_optional<uint32_t >(buf, m_SecurityID_state.value, m_SecurityID_state.has_value);
    m_SecurityIDSource_state.has_value = pmap.read_next_pmap_entry();
    perParseStringRepo.read_string_optional(buf, m_CFICode);
    perParseStringRepo.read_string_optional(buf, m_UnderlyingProduct);
    perParseStringRepo.read_string_optional(buf, m_SecurityExchange);
    perParseStringRepo.read_string_optional(buf, m_PricingModel);
    decode_field_optional<double >(buf, m_MinCabPrice_state.value, m_MinCabPrice_state.has_value);
    decode_optional_sequence(buf, m_SecurityAltIDs, perParseStringRepo, globalDictionary);
    decode_field_optional<uint32_t >(buf, m_ExpirationCycle_state.value, m_ExpirationCycle_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_UnitOfMeasureQty);
    decode_field_optional<double >(buf, m_StrikePrice_state.value, m_StrikePrice_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_StrikeCurrency);
    decode_field_optional<uint64_t >(buf, m_MinTradeVol_state.value, m_MinTradeVol_state.has_value);
    decode_field_optional<uint64_t >(buf, m_MaxTradeVol_state.value, m_MaxTradeVol_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_Currency);
    perParseStringRepo.read_string_optional(buf, m_SettlCurrency);
    decode_optional_sequence(buf, m_MDFeedTypes, perParseStringRepo, globalDictionary);
    perParseStringRepo.read_string_optional(buf, m_MatchAlgo);
    perParseStringRepo.read_string_optional(buf, m_SecuritySubType);
    decode_optional_sequence(buf, m_Underlyings, perParseStringRepo, globalDictionary);
    perParseStringRepo.read_string_optional(buf, m_MaxPriceVariation);
    perParseStringRepo.read_string_optional(buf, m_ImpliedMarketIndicator);
    decode_optional_sequence(buf, m_InstrAttrib, perParseStringRepo, globalDictionary);
    decode_field_optional<uint64_t >(buf, m_MaturityMonthYear_state.value, m_MaturityMonthYear_state.has_value);
    decode_field_optional<double >(buf, m_MinPriceIncrement_state.value, m_MinPriceIncrement_state.has_value);
    decode_field_optional<double >(buf, m_MinPriceIncrementAmount_state.value, m_MinPriceIncrementAmount_state.has_value);
    decode_field_optional<uint64_t >(buf, m_LastUpdateTime_state.value, m_LastUpdateTime_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_SecurityUpdateAction);
    decode_field_optional<double >(buf, m_DisplayFactor_state.value, m_DisplayFactor_state.has_value);
    decode_optional_sequence(buf, m_Legs, perParseStringRepo, globalDictionary);
    perParseStringRepo.read_string_optional(buf, m_ApplID);
    m_UserDefinedInstrument_state.has_value = pmap.read_next_pmap_entry();
    decode_field_optional<double >(buf, m_PriceRatio_state.value, m_PriceRatio_state.has_value);
    decode_field_optional<uint32_t >(buf, m_ContractMultiplierType_state.value, m_ContractMultiplierType_state.has_value);
    decode_field_optional<uint32_t >(buf, m_FlowScheduleType_state.value, m_FlowScheduleType_state.has_value);
    decode_field_optional<double >(buf, m_ContractMultiplier_state.value, m_ContractMultiplier_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_UnitofMeasure);
    decode_field_optional<uint64_t >(buf, m_DecayQuantity_state.value, m_DecayQuantity_state.has_value);
    decode_field_optional<uint64_t >(buf, m_DecayStartDate_state.value, m_DecayStartDate_state.has_value);
    decode_field_optional<uint64_t >(buf, m_OriginalContractSize_state.value, m_OriginalContractSize_state.has_value);
    decode_field_optional<uint32_t >(buf, m_ClearedVolume_state.value, m_ClearedVolume_state.has_value);
    decode_field_optional<uint32_t >(buf, m_OpenInterestQty_state.value, m_OpenInterestQty_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradingReferenceDate_state.value, m_TradingReferenceDate_state.has_value);
    decode_optional_sequence(buf, m_LotTypeRules, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline FAST_Nullable<uint32_t >& TotNumReports()
  {
    return m_TotNumReports_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_139_Events_Element, 100>& Events()
  {
    return m_Events;
  }
  
  inline FAST_Nullable<double >& TradingReferencePrice()
  {
    return m_TradingReferencePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& SettlePriceType()
  {
    return m_SettlePriceType;
  }
  
  inline FAST_Nullable<double >& HighLimitPx()
  {
    return m_HighLimitPx_state;
  }
  
  inline FAST_Nullable<double >& LowLimitPx()
  {
    return m_LowLimitPx_state;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityGroup()
  {
    return m_SecurityGroup;
  }
  
  inline FAST_Nullable<FAST_String>& Symbol()
  {
    return m_Symbol;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityDesc()
  {
    return m_SecurityDesc;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Nullable<FAST_String>& CFICode()
  {
    return m_CFICode;
  }
  
  inline FAST_Nullable<FAST_String>& UnderlyingProduct()
  {
    return m_UnderlyingProduct;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityExchange()
  {
    return m_SecurityExchange;
  }
  
  inline FAST_Nullable<FAST_String>& PricingModel()
  {
    return m_PricingModel;
  }
  
  inline FAST_Nullable<double >& MinCabPrice()
  {
    return m_MinCabPrice_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_139_SecurityAltIDs_Element, 100>& SecurityAltIDs()
  {
    return m_SecurityAltIDs;
  }
  
  inline FAST_Nullable<uint32_t >& ExpirationCycle()
  {
    return m_ExpirationCycle_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnitOfMeasureQty()
  {
    return m_UnitOfMeasureQty;
  }
  
  inline FAST_Nullable<double >& StrikePrice()
  {
    return m_StrikePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& StrikeCurrency()
  {
    return m_StrikeCurrency;
  }
  
  inline FAST_Nullable<uint64_t >& MinTradeVol()
  {
    return m_MinTradeVol_state;
  }
  
  inline FAST_Nullable<uint64_t >& MaxTradeVol()
  {
    return m_MaxTradeVol_state;
  }
  
  inline FAST_Nullable<FAST_String>& Currency()
  {
    return m_Currency;
  }
  
  inline FAST_Nullable<FAST_String>& SettlCurrency()
  {
    return m_SettlCurrency;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_139_MDFeedTypes_Element, 100>& MDFeedTypes()
  {
    return m_MDFeedTypes;
  }
  
  inline FAST_Nullable<FAST_String>& MatchAlgo()
  {
    return m_MatchAlgo;
  }
  
  inline FAST_Nullable<FAST_String>& SecuritySubType()
  {
    return m_SecuritySubType;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_139_Underlyings_Element, 100>& Underlyings()
  {
    return m_Underlyings;
  }
  
  inline FAST_Nullable<FAST_String>& MaxPriceVariation()
  {
    return m_MaxPriceVariation;
  }
  
  inline FAST_Nullable<FAST_String>& ImpliedMarketIndicator()
  {
    return m_ImpliedMarketIndicator;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_139_InstrAttrib_Element, 100>& InstrAttrib()
  {
    return m_InstrAttrib;
  }
  
  inline FAST_Nullable<uint64_t >& MaturityMonthYear()
  {
    return m_MaturityMonthYear_state;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrement()
  {
    return m_MinPriceIncrement_state;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrementAmount()
  {
    return m_MinPriceIncrementAmount_state;
  }
  
  inline FAST_Nullable<uint64_t >& LastUpdateTime()
  {
    return m_LastUpdateTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityUpdateAction()
  {
    return m_SecurityUpdateAction;
  }
  
  inline FAST_Nullable<double >& DisplayFactor()
  {
    return m_DisplayFactor_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_139_Legs_Element, 100>& Legs()
  {
    return m_Legs;
  }
  
  inline FAST_Nullable<FAST_String>& ApplID()
  {
    return m_ApplID;
  }
  
  inline FAST_Nullable<FAST_String>& UserDefinedInstrument()
  {
    return m_UserDefinedInstrument_state;
  }
  
  inline FAST_Nullable<double >& PriceRatio()
  {
    return m_PriceRatio_state;
  }
  
  inline FAST_Nullable<uint32_t >& ContractMultiplierType()
  {
    return m_ContractMultiplierType_state;
  }
  
  inline FAST_Nullable<uint32_t >& FlowScheduleType()
  {
    return m_FlowScheduleType_state;
  }
  
  inline FAST_Nullable<double >& ContractMultiplier()
  {
    return m_ContractMultiplier_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnitofMeasure()
  {
    return m_UnitofMeasure;
  }
  
  inline FAST_Nullable<uint64_t >& DecayQuantity()
  {
    return m_DecayQuantity_state;
  }
  
  inline FAST_Nullable<uint64_t >& DecayStartDate()
  {
    return m_DecayStartDate_state;
  }
  
  inline FAST_Nullable<uint64_t >& OriginalContractSize()
  {
    return m_OriginalContractSize_state;
  }
  
  inline FAST_Nullable<uint32_t >& ClearedVolume()
  {
    return m_ClearedVolume_state;
  }
  
  inline FAST_Nullable<uint32_t >& OpenInterestQty()
  {
    return m_OpenInterestQty_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradingReferenceDate()
  {
    return m_TradingReferenceDate_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_139_LotTypeRules_Element, 100>& LotTypeRules()
  {
    return m_LotTypeRules;
  }
};

class MDSecurityDefinition_140_Events_Element
{
private:
  FAST_Nullable<uint32_t > m_EventType_state;
  FAST_Nullable<uint64_t > m_EventDate_state;
  FAST_Nullable<uint64_t > m_EventTime_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_EventType_state.has_value = false;
    m_EventDate_state.has_value = false;
    m_EventTime_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_optional<uint32_t >(buf, m_EventType_state.value, m_EventType_state.has_value);
    decode_field_optional<uint64_t >(buf, m_EventDate_state.value, m_EventDate_state.has_value);
    decode_field_optional<uint64_t >(buf, m_EventTime_state.value, m_EventTime_state.has_value);
  }
  
  inline FAST_Nullable<uint32_t >& EventType()
  {
    return m_EventType_state;
  }
  
  inline FAST_Nullable<uint64_t >& EventDate()
  {
    return m_EventDate_state;
  }
  
  inline FAST_Nullable<uint64_t >& EventTime()
  {
    return m_EventTime_state;
  }
};

class MDSecurityDefinition_140_SecurityAltIDs_Element
{
private:
  FAST_Nullable<FAST_String> m_SecurityAltID;
  FAST_Nullable<uint32_t > m_SecurityAltIDSource_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_SecurityAltIDSource_state.assign_valid_value(8);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    perParseStringRepo.read_string_optional(buf, m_SecurityAltID);
    m_SecurityAltIDSource_state.has_value = pmap.read_next_pmap_entry();
  }
  
  inline FAST_Nullable<FAST_String>& SecurityAltID()
  {
    return m_SecurityAltID;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityAltIDSource()
  {
    return m_SecurityAltIDSource_state;
  }
};

class MDSecurityDefinition_140_MDFeedTypes_Element
{
private:
  FAST_String m_MDFeedType;
  uint32_t m_MarketDepth_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MarketDepth_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_MDFeedType);
    decode_field_mandatory<uint32_t >(buf, m_MarketDepth_state);
  }
  
  inline FAST_String& MDFeedType()
  {
    return m_MDFeedType;
  }
  
  inline uint32_t MarketDepth()
  {
    return m_MarketDepth_state;
  }
};

class MDSecurityDefinition_140_Underlyings_Element
{
private:
  FAST_String m_UnderlyingSymbol_state;
  uint32_t m_UnderlyingSecurityID_state;
  uint32_t m_UnderlyingSecurityIDSource_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("[N/A]", m_UnderlyingSymbol_state);
    m_UnderlyingSecurityID_state = 0;
    m_UnderlyingSecurityIDSource_state = 8;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_UnderlyingSecurityID_state);
  }
  
  inline FAST_String& UnderlyingSymbol()
  {
    return m_UnderlyingSymbol_state;
  }
  
  inline uint32_t UnderlyingSecurityID()
  {
    return m_UnderlyingSecurityID_state;
  }
  
  inline uint32_t UnderlyingSecurityIDSource()
  {
    return m_UnderlyingSecurityIDSource_state;
  }
};

class MDSecurityDefinition_140_InstrAttrib_Element
{
private:
  uint64_t m_InstrAttribType_state;
  FAST_Nullable<FAST_String> m_InstrAttribValue;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_InstrAttribType_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint64_t >(buf, m_InstrAttribType_state);
    perParseStringRepo.read_string_optional(buf, m_InstrAttribValue);
  }
  
  inline uint64_t InstrAttribType()
  {
    return m_InstrAttribType_state;
  }
  
  inline FAST_Nullable<FAST_String>& InstrAttribValue()
  {
    return m_InstrAttribValue;
  }
};

class MDSecurityDefinition_140_Legs_Element
{
private:
  FAST_String m_LegSymbol;
  uint32_t m_LegRatioQty_state;
  uint64_t m_LegSecurityID_state;
  FAST_Nullable<FAST_String> m_LegSecurityDesc;
  uint32_t m_LegSecurityIDSource_state;
  FAST_Nullable<FAST_String> m_LegSide;
  FAST_Nullable<FAST_String> m_LegSecurityGroup;
  FAST_Nullable<FAST_String> m_LegCFICode;
  FAST_Nullable<FAST_String> m_LegSecuritySubType;
  FAST_Nullable<FAST_String> m_LegCurrency;
  FAST_Nullable<uint64_t > m_LegMaturityMonthYear_state;
  FAST_Nullable<double > m_LegStrikePrice_state;
  FAST_Nullable<FAST_String> m_LegSecurityExchange;
  FAST_Nullable<FAST_String> m_LegStrikeCurrency;
  FAST_Nullable<double > m_LegPrice_state;
  FAST_Nullable<double > m_LegOptionDelta_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_LegRatioQty_state = 0;
    m_LegSecurityID_state = 0;
    m_LegSecurityIDSource_state = 8;
    m_LegMaturityMonthYear_state.has_value = false;
    m_LegStrikePrice_state.has_value = false;
    m_LegPrice_state.has_value = false;
    m_LegOptionDelta_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_LegSymbol);
    decode_field_mandatory<uint32_t >(buf, m_LegRatioQty_state);
    decode_field_mandatory<uint64_t >(buf, m_LegSecurityID_state);
    perParseStringRepo.read_string_optional(buf, m_LegSecurityDesc);
    perParseStringRepo.read_string_optional(buf, m_LegSide);
    perParseStringRepo.read_string_optional(buf, m_LegSecurityGroup);
    perParseStringRepo.read_string_optional(buf, m_LegCFICode);
    perParseStringRepo.read_string_optional(buf, m_LegSecuritySubType);
    perParseStringRepo.read_string_optional(buf, m_LegCurrency);
    decode_field_optional<uint64_t >(buf, m_LegMaturityMonthYear_state.value, m_LegMaturityMonthYear_state.has_value);
    decode_field_optional<double >(buf, m_LegStrikePrice_state.value, m_LegStrikePrice_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_LegSecurityExchange);
    perParseStringRepo.read_string_optional(buf, m_LegStrikeCurrency);
    decode_field_optional<double >(buf, m_LegPrice_state.value, m_LegPrice_state.has_value);
    decode_field_optional<double >(buf, m_LegOptionDelta_state.value, m_LegOptionDelta_state.has_value);
  }
  
  inline FAST_String& LegSymbol()
  {
    return m_LegSymbol;
  }
  
  inline uint32_t LegRatioQty()
  {
    return m_LegRatioQty_state;
  }
  
  inline uint64_t LegSecurityID()
  {
    return m_LegSecurityID_state;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecurityDesc()
  {
    return m_LegSecurityDesc;
  }
  
  inline uint32_t LegSecurityIDSource()
  {
    return m_LegSecurityIDSource_state;
  }
  
  inline FAST_Nullable<FAST_String>& LegSide()
  {
    return m_LegSide;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecurityGroup()
  {
    return m_LegSecurityGroup;
  }
  
  inline FAST_Nullable<FAST_String>& LegCFICode()
  {
    return m_LegCFICode;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecuritySubType()
  {
    return m_LegSecuritySubType;
  }
  
  inline FAST_Nullable<FAST_String>& LegCurrency()
  {
    return m_LegCurrency;
  }
  
  inline FAST_Nullable<uint64_t >& LegMaturityMonthYear()
  {
    return m_LegMaturityMonthYear_state;
  }
  
  inline FAST_Nullable<double >& LegStrikePrice()
  {
    return m_LegStrikePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& LegSecurityExchange()
  {
    return m_LegSecurityExchange;
  }
  
  inline FAST_Nullable<FAST_String>& LegStrikeCurrency()
  {
    return m_LegStrikeCurrency;
  }
  
  inline FAST_Nullable<double >& LegPrice()
  {
    return m_LegPrice_state;
  }
  
  inline FAST_Nullable<double >& LegOptionDelta()
  {
    return m_LegOptionDelta_state;
  }
};

class MDSecurityDefinition_140_LotTypeRules_Element
{
private:
  FAST_Nullable<FAST_String> m_LotType;
  FAST_Nullable<double > m_MinLotSize_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MinLotSize_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_optional(buf, m_LotType);
    decode_field_optional<double >(buf, m_MinLotSize_state.value, m_MinLotSize_state.has_value);
  }
  
  inline FAST_Nullable<FAST_String>& LotType()
  {
    return m_LotType;
  }
  
  inline FAST_Nullable<double >& MinLotSize()
  {
    return m_MinLotSize_state;
  }
};

class MDSecurityDefinition_140
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  FAST_Nullable<uint32_t > m_TotNumReports_state;
  FAST_Sequence<MDSecurityDefinition_140_Events_Element, 100> m_Events;
  FAST_Nullable<double > m_TradingReferencePrice_state;
  FAST_Nullable<FAST_String> m_SettlePriceType;
  FAST_Nullable<double > m_HighLimitPx_state;
  FAST_Nullable<double > m_LowLimitPx_state;
  FAST_Nullable<FAST_String> m_SecurityGroup;
  FAST_Nullable<FAST_String> m_Symbol;
  FAST_Nullable<FAST_String> m_SecurityDesc;
  FAST_Nullable<uint32_t > m_SecurityID_state;
  FAST_Nullable<uint32_t > m_SecurityIDSource_state;
  FAST_Nullable<FAST_String> m_CFICode;
  FAST_Nullable<FAST_String> m_UnderlyingProduct;
  FAST_Nullable<FAST_String> m_SecurityExchange;
  FAST_Nullable<FAST_String> m_PricingModel;
  FAST_Nullable<double > m_MinCabPrice_state;
  FAST_Sequence<MDSecurityDefinition_140_SecurityAltIDs_Element, 100> m_SecurityAltIDs;
  FAST_Nullable<uint32_t > m_ExpirationCycle_state;
  FAST_Nullable<FAST_String> m_UnitOfMeasureQty;
  FAST_Nullable<double > m_StrikePrice_state;
  FAST_Nullable<FAST_String> m_StrikeCurrency;
  FAST_Nullable<uint64_t > m_MinTradeVol_state;
  FAST_Nullable<uint64_t > m_MaxTradeVol_state;
  FAST_Nullable<FAST_String> m_Currency;
  FAST_Nullable<FAST_String> m_SettlCurrency;
  FAST_Sequence<MDSecurityDefinition_140_MDFeedTypes_Element, 100> m_MDFeedTypes;
  FAST_Nullable<FAST_String> m_MatchAlgo;
  FAST_Nullable<FAST_String> m_SecuritySubType;
  FAST_Sequence<MDSecurityDefinition_140_Underlyings_Element, 100> m_Underlyings;
  FAST_Nullable<FAST_String> m_MaxPriceVariation;
  FAST_Nullable<FAST_String> m_ImpliedMarketIndicator;
  FAST_Sequence<MDSecurityDefinition_140_InstrAttrib_Element, 100> m_InstrAttrib;
  FAST_Nullable<uint32_t > m_MarketSegmentID_state;
  FAST_Nullable<uint64_t > m_MaturityMonthYear_state;
  FAST_Nullable<double > m_MinPriceIncrement_state;
  FAST_Nullable<double > m_MinPriceIncrementAmount_state;
  FAST_Nullable<uint64_t > m_LastUpdateTime_state;
  FAST_Nullable<FAST_String> m_SecurityUpdateAction;
  FAST_Nullable<double > m_DisplayFactor_state;
  FAST_Sequence<MDSecurityDefinition_140_Legs_Element, 100> m_Legs;
  FAST_Nullable<FAST_String> m_ApplID;
  FAST_Nullable<FAST_String> m_UserDefinedInstrument_state;
  FAST_Nullable<double > m_PriceRatio_state;
  FAST_Nullable<uint32_t > m_ContractMultiplierType_state;
  FAST_Nullable<uint32_t > m_FlowScheduleType_state;
  FAST_Nullable<uint32_t > m_ContractMultiplier_state;
  FAST_Nullable<FAST_String> m_UnitofMeasure;
  FAST_Nullable<uint64_t > m_DecayQuantity_state;
  FAST_Nullable<uint64_t > m_DecayStartDate_state;
  FAST_Nullable<uint64_t > m_OriginalContractSize_state;
  FAST_Nullable<uint32_t > m_ClearedVolume_state;
  FAST_Nullable<uint32_t > m_OpenInterestQty_state;
  FAST_Nullable<uint32_t > m_TradingReferenceDate_state;
  FAST_Sequence<MDSecurityDefinition_140_LotTypeRules_Element, 100> m_LotTypeRules;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("d", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TotNumReports_state.has_value = false;
    m_TradingReferencePrice_state.has_value = false;
    m_HighLimitPx_state.has_value = false;
    m_LowLimitPx_state.has_value = false;
    m_SecurityID_state.has_value = false;
    m_SecurityIDSource_state.assign_valid_value(8);
    m_MinCabPrice_state.has_value = false;
    m_ExpirationCycle_state.has_value = false;
    m_StrikePrice_state.has_value = false;
    m_MinTradeVol_state.has_value = false;
    m_MaxTradeVol_state.has_value = false;
    m_MarketSegmentID_state.has_value = false;
    m_MaturityMonthYear_state.has_value = false;
    m_MinPriceIncrement_state.has_value = false;
    m_MinPriceIncrementAmount_state.has_value = false;
    m_LastUpdateTime_state.has_value = false;
    m_DisplayFactor_state.has_value = false;
    stringRepo.assign_next_const_string_state<FAST_String>("Y", m_UserDefinedInstrument_state.value);
    m_PriceRatio_state.has_value = false;
    m_ContractMultiplierType_state.has_value = false;
    m_FlowScheduleType_state.has_value = false;
    m_ContractMultiplier_state.has_value = false;
    m_DecayQuantity_state.has_value = false;
    m_DecayStartDate_state.has_value = false;
    m_OriginalContractSize_state.has_value = false;
    m_ClearedVolume_state.has_value = false;
    m_OpenInterestQty_state.has_value = false;
    m_TradingReferenceDate_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_optional<uint32_t >(buf, m_TotNumReports_state.value, m_TotNumReports_state.has_value);
    decode_optional_sequence(buf, m_Events, perParseStringRepo, globalDictionary);
    decode_field_optional<double >(buf, m_TradingReferencePrice_state.value, m_TradingReferencePrice_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_SettlePriceType);
    decode_field_optional<double >(buf, m_HighLimitPx_state.value, m_HighLimitPx_state.has_value);
    decode_field_optional<double >(buf, m_LowLimitPx_state.value, m_LowLimitPx_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_SecurityGroup);
    perParseStringRepo.read_string_optional(buf, m_Symbol);
    perParseStringRepo.read_string_optional(buf, m_SecurityDesc);
    decode_field_optional<uint32_t >(buf, m_SecurityID_state.value, m_SecurityID_state.has_value);
    m_SecurityIDSource_state.has_value = pmap.read_next_pmap_entry();
    perParseStringRepo.read_string_optional(buf, m_CFICode);
    perParseStringRepo.read_string_optional(buf, m_UnderlyingProduct);
    perParseStringRepo.read_string_optional(buf, m_SecurityExchange);
    perParseStringRepo.read_string_optional(buf, m_PricingModel);
    decode_field_optional<double >(buf, m_MinCabPrice_state.value, m_MinCabPrice_state.has_value);
    decode_optional_sequence(buf, m_SecurityAltIDs, perParseStringRepo, globalDictionary);
    decode_field_optional<uint32_t >(buf, m_ExpirationCycle_state.value, m_ExpirationCycle_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_UnitOfMeasureQty);
    decode_field_optional<double >(buf, m_StrikePrice_state.value, m_StrikePrice_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_StrikeCurrency);
    decode_field_optional<uint64_t >(buf, m_MinTradeVol_state.value, m_MinTradeVol_state.has_value);
    decode_field_optional<uint64_t >(buf, m_MaxTradeVol_state.value, m_MaxTradeVol_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_Currency);
    perParseStringRepo.read_string_optional(buf, m_SettlCurrency);
    decode_optional_sequence(buf, m_MDFeedTypes, perParseStringRepo, globalDictionary);
    perParseStringRepo.read_string_optional(buf, m_MatchAlgo);
    perParseStringRepo.read_string_optional(buf, m_SecuritySubType);
    decode_optional_sequence(buf, m_Underlyings, perParseStringRepo, globalDictionary);
    perParseStringRepo.read_string_optional(buf, m_MaxPriceVariation);
    perParseStringRepo.read_string_optional(buf, m_ImpliedMarketIndicator);
    decode_optional_sequence(buf, m_InstrAttrib, perParseStringRepo, globalDictionary);
    decode_field_optional<uint32_t >(buf, m_MarketSegmentID_state.value, m_MarketSegmentID_state.has_value);
    decode_field_optional<uint64_t >(buf, m_MaturityMonthYear_state.value, m_MaturityMonthYear_state.has_value);
    decode_field_optional<double >(buf, m_MinPriceIncrement_state.value, m_MinPriceIncrement_state.has_value);
    decode_field_optional<double >(buf, m_MinPriceIncrementAmount_state.value, m_MinPriceIncrementAmount_state.has_value);
    decode_field_optional<uint64_t >(buf, m_LastUpdateTime_state.value, m_LastUpdateTime_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_SecurityUpdateAction);
    decode_field_optional<double >(buf, m_DisplayFactor_state.value, m_DisplayFactor_state.has_value);
    decode_optional_sequence(buf, m_Legs, perParseStringRepo, globalDictionary);
    perParseStringRepo.read_string_optional(buf, m_ApplID);
    m_UserDefinedInstrument_state.has_value = pmap.read_next_pmap_entry();
    decode_field_optional<double >(buf, m_PriceRatio_state.value, m_PriceRatio_state.has_value);
    decode_field_optional<uint32_t >(buf, m_ContractMultiplierType_state.value, m_ContractMultiplierType_state.has_value);
    decode_field_optional<uint32_t >(buf, m_FlowScheduleType_state.value, m_FlowScheduleType_state.has_value);
    decode_field_optional<uint32_t >(buf, m_ContractMultiplier_state.value, m_ContractMultiplier_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_UnitofMeasure);
    decode_field_optional<uint64_t >(buf, m_DecayQuantity_state.value, m_DecayQuantity_state.has_value);
    decode_field_optional<uint64_t >(buf, m_DecayStartDate_state.value, m_DecayStartDate_state.has_value);
    decode_field_optional<uint64_t >(buf, m_OriginalContractSize_state.value, m_OriginalContractSize_state.has_value);
    decode_field_optional<uint32_t >(buf, m_ClearedVolume_state.value, m_ClearedVolume_state.has_value);
    decode_field_optional<uint32_t >(buf, m_OpenInterestQty_state.value, m_OpenInterestQty_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradingReferenceDate_state.value, m_TradingReferenceDate_state.has_value);
    decode_optional_sequence(buf, m_LotTypeRules, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline FAST_Nullable<uint32_t >& TotNumReports()
  {
    return m_TotNumReports_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_140_Events_Element, 100>& Events()
  {
    return m_Events;
  }
  
  inline FAST_Nullable<double >& TradingReferencePrice()
  {
    return m_TradingReferencePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& SettlePriceType()
  {
    return m_SettlePriceType;
  }
  
  inline FAST_Nullable<double >& HighLimitPx()
  {
    return m_HighLimitPx_state;
  }
  
  inline FAST_Nullable<double >& LowLimitPx()
  {
    return m_LowLimitPx_state;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityGroup()
  {
    return m_SecurityGroup;
  }
  
  inline FAST_Nullable<FAST_String>& Symbol()
  {
    return m_Symbol;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityDesc()
  {
    return m_SecurityDesc;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Nullable<FAST_String>& CFICode()
  {
    return m_CFICode;
  }
  
  inline FAST_Nullable<FAST_String>& UnderlyingProduct()
  {
    return m_UnderlyingProduct;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityExchange()
  {
    return m_SecurityExchange;
  }
  
  inline FAST_Nullable<FAST_String>& PricingModel()
  {
    return m_PricingModel;
  }
  
  inline FAST_Nullable<double >& MinCabPrice()
  {
    return m_MinCabPrice_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_140_SecurityAltIDs_Element, 100>& SecurityAltIDs()
  {
    return m_SecurityAltIDs;
  }
  
  inline FAST_Nullable<uint32_t >& ExpirationCycle()
  {
    return m_ExpirationCycle_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnitOfMeasureQty()
  {
    return m_UnitOfMeasureQty;
  }
  
  inline FAST_Nullable<double >& StrikePrice()
  {
    return m_StrikePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& StrikeCurrency()
  {
    return m_StrikeCurrency;
  }
  
  inline FAST_Nullable<uint64_t >& MinTradeVol()
  {
    return m_MinTradeVol_state;
  }
  
  inline FAST_Nullable<uint64_t >& MaxTradeVol()
  {
    return m_MaxTradeVol_state;
  }
  
  inline FAST_Nullable<FAST_String>& Currency()
  {
    return m_Currency;
  }
  
  inline FAST_Nullable<FAST_String>& SettlCurrency()
  {
    return m_SettlCurrency;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_140_MDFeedTypes_Element, 100>& MDFeedTypes()
  {
    return m_MDFeedTypes;
  }
  
  inline FAST_Nullable<FAST_String>& MatchAlgo()
  {
    return m_MatchAlgo;
  }
  
  inline FAST_Nullable<FAST_String>& SecuritySubType()
  {
    return m_SecuritySubType;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_140_Underlyings_Element, 100>& Underlyings()
  {
    return m_Underlyings;
  }
  
  inline FAST_Nullable<FAST_String>& MaxPriceVariation()
  {
    return m_MaxPriceVariation;
  }
  
  inline FAST_Nullable<FAST_String>& ImpliedMarketIndicator()
  {
    return m_ImpliedMarketIndicator;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_140_InstrAttrib_Element, 100>& InstrAttrib()
  {
    return m_InstrAttrib;
  }
  
  inline FAST_Nullable<uint32_t >& MarketSegmentID()
  {
    return m_MarketSegmentID_state;
  }
  
  inline FAST_Nullable<uint64_t >& MaturityMonthYear()
  {
    return m_MaturityMonthYear_state;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrement()
  {
    return m_MinPriceIncrement_state;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrementAmount()
  {
    return m_MinPriceIncrementAmount_state;
  }
  
  inline FAST_Nullable<uint64_t >& LastUpdateTime()
  {
    return m_LastUpdateTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityUpdateAction()
  {
    return m_SecurityUpdateAction;
  }
  
  inline FAST_Nullable<double >& DisplayFactor()
  {
    return m_DisplayFactor_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_140_Legs_Element, 100>& Legs()
  {
    return m_Legs;
  }
  
  inline FAST_Nullable<FAST_String>& ApplID()
  {
    return m_ApplID;
  }
  
  inline FAST_Nullable<FAST_String>& UserDefinedInstrument()
  {
    return m_UserDefinedInstrument_state;
  }
  
  inline FAST_Nullable<double >& PriceRatio()
  {
    return m_PriceRatio_state;
  }
  
  inline FAST_Nullable<uint32_t >& ContractMultiplierType()
  {
    return m_ContractMultiplierType_state;
  }
  
  inline FAST_Nullable<uint32_t >& FlowScheduleType()
  {
    return m_FlowScheduleType_state;
  }
  
  inline FAST_Nullable<uint32_t >& ContractMultiplier()
  {
    return m_ContractMultiplier_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnitofMeasure()
  {
    return m_UnitofMeasure;
  }
  
  inline FAST_Nullable<uint64_t >& DecayQuantity()
  {
    return m_DecayQuantity_state;
  }
  
  inline FAST_Nullable<uint64_t >& DecayStartDate()
  {
    return m_DecayStartDate_state;
  }
  
  inline FAST_Nullable<uint64_t >& OriginalContractSize()
  {
    return m_OriginalContractSize_state;
  }
  
  inline FAST_Nullable<uint32_t >& ClearedVolume()
  {
    return m_ClearedVolume_state;
  }
  
  inline FAST_Nullable<uint32_t >& OpenInterestQty()
  {
    return m_OpenInterestQty_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradingReferenceDate()
  {
    return m_TradingReferenceDate_state;
  }
  
  inline FAST_Sequence<MDSecurityDefinition_140_LotTypeRules_Element, 100>& LotTypeRules()
  {
    return m_LotTypeRules;
  }
};

class MDIncRefresh_141_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_TradeCondition;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;
  FAST_Nullable<uint32_t > m_TradeID_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_AggressorSide_state.has_value = false;
    m_TradeID_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
    decode_field_optional<uint32_t >(buf, m_TradeID_state.value, m_TradeID_state.has_value);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
  
  inline FAST_Nullable<uint32_t >& TradeID()
  {
    return m_TradeID_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
};

class MDIncRefresh_141
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_141_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_141_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_142_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;
  FAST_Nullable<uint32_t > m_TradeID_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_AggressorSide_state.has_value = false;
    m_TradeID_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
    decode_field_optional<uint32_t >(buf, m_TradeID_state.value, m_TradeID_state.has_value);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
  
  inline FAST_Nullable<uint32_t >& TradeID()
  {
    return m_TradeID_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
};

class MDIncRefresh_142
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_142_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_142_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_143_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_String m_MDEntryType;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_TradeCondition;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;
  FAST_Nullable<uint32_t > m_TradeID_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_AggressorSide_state.has_value = false;
    m_TradeID_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
    decode_field_optional<uint32_t >(buf, m_TradeID_state.value, m_TradeID_state.has_value);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
  
  inline FAST_Nullable<uint32_t >& TradeID()
  {
    return m_TradeID_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
};

class MDIncRefresh_143
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_143_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_143_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_144_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  double m_MDEntryPx_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<uint32_t > m_TradeID_state;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_TradeID_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_optional<uint32_t >(buf, m_TradeID_state.value, m_TradeID_state.has_value);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<uint32_t >& TradeID()
  {
    return m_TradeID_state;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_144
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_144_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_144_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_145_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_Nullable<double > m_MDEntryPx_state;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<uint32_t > m_TradeID_state;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_NumberOfOrders_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_MDEntryPx_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_TradeID_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    decode_field_optional<double >(buf, m_MDEntryPx_state.value, m_MDEntryPx_state.has_value);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    decode_field_optional<uint32_t >(buf, m_TradeID_state.value, m_TradeID_state.has_value);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_Nullable<double >& MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<uint32_t >& TradeID()
  {
    return m_TradeID_state;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_145
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_145_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_145_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_146_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  double m_MDEntryPx_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  uint32_t m_MDEntryTime_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<uint32_t > m_TradeID_state;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryPx_state = 0;
    m_NumberOfOrders_state.has_value = false;
    m_MDEntryTime_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_TradeID_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    decode_field_optional<uint32_t >(buf, m_TradeID_state.value, m_TradeID_state.has_value);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<uint32_t >& TradeID()
  {
    return m_TradeID_state;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_146
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_146_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_146_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};

class MDIncRefresh_147_MDEntries_Element
{
private:
  uint32_t m_MDUpdateAction_state;
  FAST_Nullable<uint32_t > m_MDPriceLevel_state;
  FAST_String m_MDEntryType;
  FAST_Nullable<uint32_t > m_OpenCloseSettleFlag_state;
  FAST_Nullable<uint32_t > m_SettlDate_state;
  FAST_Nullable<FAST_String> m_TradingSessionID;
  FAST_Nullable<double > m_NetChgPrevDay_state;
  FAST_Nullable<uint32_t > m_TradeVolume_state;
  FAST_Nullable<uint32_t > m_NumberOfOrders_state;
  uint32_t m_SecurityIDSource_state;
  uint32_t m_SecurityID_state;
  uint32_t m_RptSeq_state;
  uint32_t m_MDEntryTime_state;
  double m_MDEntryPx_state;
  FAST_Nullable<int32_t > m_MDEntrySize_state;
  FAST_Nullable<FAST_String> m_TradeCondition;
  FAST_Nullable<FAST_String> m_TickDirection;
  FAST_Nullable<FAST_String> m_QuoteCondition;
  FAST_Nullable<uint32_t > m_TradeID_state;
  FAST_Nullable<uint32_t > m_AggressorSide_state;
  FAST_Nullable<FAST_String> m_MatchEventIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDUpdateAction_state = 0;
    m_MDPriceLevel_state.has_value = false;
    m_OpenCloseSettleFlag_state.has_value = false;
    m_SettlDate_state.has_value = false;
    m_NetChgPrevDay_state.has_value = false;
    m_TradeVolume_state.has_value = false;
    m_NumberOfOrders_state.has_value = false;
    m_SecurityIDSource_state = 8;
    m_SecurityID_state = 0;
    m_RptSeq_state = 0;
    m_MDEntryTime_state = 0;
    m_MDEntryPx_state = 0;
    m_MDEntrySize_state.has_value = false;
    m_TradeID_state.has_value = false;
    m_AggressorSide_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_MDUpdateAction_state);
    decode_field_optional<uint32_t >(buf, m_MDPriceLevel_state.value, m_MDPriceLevel_state.has_value);
    perParseStringRepo.read_string_mandatory(buf, m_MDEntryType);
    decode_field_optional<uint32_t >(buf, m_OpenCloseSettleFlag_state.value, m_OpenCloseSettleFlag_state.has_value);
    decode_field_optional<uint32_t >(buf, m_SettlDate_state.value, m_SettlDate_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradingSessionID);
    decode_field_optional<double >(buf, m_NetChgPrevDay_state.value, m_NetChgPrevDay_state.has_value);
    decode_field_optional<uint32_t >(buf, m_TradeVolume_state.value, m_TradeVolume_state.has_value);
    decode_field_optional<uint32_t >(buf, m_NumberOfOrders_state.value, m_NumberOfOrders_state.has_value);
    decode_field_mandatory<uint32_t >(buf, m_SecurityID_state);
    decode_field_mandatory<uint32_t >(buf, m_RptSeq_state);
    decode_field_mandatory<uint32_t >(buf, m_MDEntryTime_state);
    decode_field_mandatory<double >(buf, m_MDEntryPx_state);
    decode_field_optional<int32_t >(buf, m_MDEntrySize_state.value, m_MDEntrySize_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_TradeCondition);
    perParseStringRepo.read_string_optional(buf, m_TickDirection);
    perParseStringRepo.read_string_optional(buf, m_QuoteCondition);
    decode_field_optional<uint32_t >(buf, m_TradeID_state.value, m_TradeID_state.has_value);
    decode_field_optional<uint32_t >(buf, m_AggressorSide_state.value, m_AggressorSide_state.has_value);
    perParseStringRepo.read_string_optional(buf, m_MatchEventIndicator);
  }
  
  inline uint32_t MDUpdateAction()
  {
    return m_MDUpdateAction_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDPriceLevel()
  {
    return m_MDPriceLevel_state;
  }
  
  inline FAST_String& MDEntryType()
  {
    return m_MDEntryType;
  }
  
  inline FAST_Nullable<uint32_t >& OpenCloseSettleFlag()
  {
    return m_OpenCloseSettleFlag_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlDate()
  {
    return m_SettlDate_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradingSessionID()
  {
    return m_TradingSessionID;
  }
  
  inline FAST_Nullable<double >& NetChgPrevDay()
  {
    return m_NetChgPrevDay_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradeVolume()
  {
    return m_TradeVolume_state;
  }
  
  inline FAST_Nullable<uint32_t >& NumberOfOrders()
  {
    return m_NumberOfOrders_state;
  }
  
  inline uint32_t SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline uint32_t SecurityID()
  {
    return m_SecurityID_state;
  }
  
  inline uint32_t RptSeq()
  {
    return m_RptSeq_state;
  }
  
  inline uint32_t MDEntryTime()
  {
    return m_MDEntryTime_state;
  }
  
  inline double MDEntryPx()
  {
    return m_MDEntryPx_state;
  }
  
  inline FAST_Nullable<int32_t >& MDEntrySize()
  {
    return m_MDEntrySize_state;
  }
  
  inline FAST_Nullable<FAST_String>& TradeCondition()
  {
    return m_TradeCondition;
  }
  
  inline FAST_Nullable<FAST_String>& TickDirection()
  {
    return m_TickDirection;
  }
  
  inline FAST_Nullable<FAST_String>& QuoteCondition()
  {
    return m_QuoteCondition;
  }
  
  inline FAST_Nullable<uint32_t >& TradeID()
  {
    return m_TradeID_state;
  }
  
  inline FAST_Nullable<uint32_t >& AggressorSide()
  {
    return m_AggressorSide_state;
  }
  
  inline FAST_Nullable<FAST_String>& MatchEventIndicator()
  {
    return m_MatchEventIndicator;
  }
};

class MDIncRefresh_147
{
private:
  FAST_String m_ApplVerID_state;
  FAST_String m_MessageType_state;
  FAST_String m_SenderCompID_state;
  uint32_t m_MsgSeqNum_state;
  uint64_t m_SendingTime_state;
  FAST_Nullable<FAST_String> m_PosDupFlag;
  uint32_t m_TradeDate_state;
  FAST_Sequence<MDIncRefresh_147_MDEntries_Element, 100> m_MDEntries;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("9", m_ApplVerID_state);
    stringRepo.assign_next_const_string_state<FAST_String>("X", m_MessageType_state);
    stringRepo.assign_next_const_string_state<FAST_String>("CME", m_SenderCompID_state);
    m_MsgSeqNum_state = 0;
    m_SendingTime_state = 0;
    m_TradeDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Cme_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_MsgSeqNum_state);
    decode_field_mandatory<uint64_t >(buf, m_SendingTime_state);
    perParseStringRepo.read_string_optional(buf, m_PosDupFlag);
    decode_field_mandatory<uint32_t >(buf, m_TradeDate_state);
    decode_mandatory_sequence(buf, m_MDEntries, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& ApplVerID()
  {
    return m_ApplVerID_state;
  }
  
  inline FAST_String& MessageType()
  {
    return m_MessageType_state;
  }
  
  inline FAST_String& SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint64_t SendingTime()
  {
    return m_SendingTime_state;
  }
  
  inline FAST_Nullable<FAST_String>& PosDupFlag()
  {
    return m_PosDupFlag;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_Sequence<MDIncRefresh_147_MDEntries_Element, 100>& MDEntries()
  {
    return m_MDEntries;
  }
};


class Cme_Fast_Decoder
{
private:
  PerParseStringRepository m_per_parse_string_repo;
  StringStateRepository m_string_repo;
  Cme_Fast_Decoder_GlobalDictionary m_global_dictionary;
  MDIncRefresh_112 m_MDIncRefresh_112;
  MDIncRefresh_113 m_MDIncRefresh_113;
  MDSnapshotFullRefresh_114 m_MDSnapshotFullRefresh_114;
  MDIncRefresh_115 m_MDIncRefresh_115;
  MDIncRefresh_116 m_MDIncRefresh_116;
  MDIncRefresh_117 m_MDIncRefresh_117;
  MDSnapshotFullRefresh_118 m_MDSnapshotFullRefresh_118;
  MDIncRefresh_119 m_MDIncRefresh_119;
  MDIncRefresh_120 m_MDIncRefresh_120;
  MDIncRefresh_121 m_MDIncRefresh_121;
  MDIncRefresh_122 m_MDIncRefresh_122;
  MDSecurityDefinition_123 m_MDSecurityDefinition_123;
  MDLogon_124 m_MDLogon_124;
  MDIncRefresh_125 m_MDIncRefresh_125;
  MDLogout_126 m_MDLogout_126;
  MDSecurityStatus_127 m_MDSecurityStatus_127;
  MDQuoteRequest_128 m_MDQuoteRequest_128;
  MDIncRefresh_129 m_MDIncRefresh_129;
  MDSecurityStatus_130 m_MDSecurityStatus_130;
  MDIncRefresh_131 m_MDIncRefresh_131;
  MDNewsMessage_132 m_MDNewsMessage_132;
  MDHeartbeat_133 m_MDHeartbeat_133;
  MDIncRefresh_134 m_MDIncRefresh_134;
  MDIncRefresh_135 m_MDIncRefresh_135;
  MDIncRefresh_136 m_MDIncRefresh_136;
  MDIncRefresh_138 m_MDIncRefresh_138;
  MDSecurityDefinition_139 m_MDSecurityDefinition_139;
  MDSecurityDefinition_140 m_MDSecurityDefinition_140;
  MDIncRefresh_141 m_MDIncRefresh_141;
  MDIncRefresh_142 m_MDIncRefresh_142;
  MDIncRefresh_143 m_MDIncRefresh_143;
  MDIncRefresh_144 m_MDIncRefresh_144;
  MDIncRefresh_145 m_MDIncRefresh_145;
  MDIncRefresh_146 m_MDIncRefresh_146;
  MDIncRefresh_147 m_MDIncRefresh_147;

public:
  Cme_Fast_Decoder()
  {
    Reset();
  }

  void Reset()
  {
    m_MDIncRefresh_112.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_113.Reset_Dictionary(m_string_repo);
    m_MDSnapshotFullRefresh_114.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_115.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_116.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_117.Reset_Dictionary(m_string_repo);
    m_MDSnapshotFullRefresh_118.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_119.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_120.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_121.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_122.Reset_Dictionary(m_string_repo);
    m_MDSecurityDefinition_123.Reset_Dictionary(m_string_repo);
    m_MDLogon_124.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_125.Reset_Dictionary(m_string_repo);
    m_MDLogout_126.Reset_Dictionary(m_string_repo);
    m_MDSecurityStatus_127.Reset_Dictionary(m_string_repo);
    m_MDQuoteRequest_128.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_129.Reset_Dictionary(m_string_repo);
    m_MDSecurityStatus_130.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_131.Reset_Dictionary(m_string_repo);
    m_MDNewsMessage_132.Reset_Dictionary(m_string_repo);
    m_MDHeartbeat_133.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_134.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_135.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_136.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_138.Reset_Dictionary(m_string_repo);
    m_MDSecurityDefinition_139.Reset_Dictionary(m_string_repo);
    m_MDSecurityDefinition_140.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_141.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_142.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_143.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_144.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_145.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_146.Reset_Dictionary(m_string_repo);
    m_MDIncRefresh_147.Reset_Dictionary(m_string_repo);
    m_string_repo.reset();
    m_global_dictionary.reset();
  }

  MDIncRefresh_112& Parse_MDIncRefresh_112(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_112.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_112;
  }

  MDIncRefresh_113& Parse_MDIncRefresh_113(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_113.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_113;
  }

  MDSnapshotFullRefresh_114& Parse_MDSnapshotFullRefresh_114(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDSnapshotFullRefresh_114.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDSnapshotFullRefresh_114;
  }

  MDIncRefresh_115& Parse_MDIncRefresh_115(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_115.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_115;
  }

  MDIncRefresh_116& Parse_MDIncRefresh_116(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_116.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_116;
  }

  MDIncRefresh_117& Parse_MDIncRefresh_117(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_117.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_117;
  }

  MDSnapshotFullRefresh_118& Parse_MDSnapshotFullRefresh_118(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDSnapshotFullRefresh_118.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDSnapshotFullRefresh_118;
  }

  MDIncRefresh_119& Parse_MDIncRefresh_119(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_119.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_119;
  }

  MDIncRefresh_120& Parse_MDIncRefresh_120(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_120.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_120;
  }

  MDIncRefresh_121& Parse_MDIncRefresh_121(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_121.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_121;
  }

  MDIncRefresh_122& Parse_MDIncRefresh_122(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_122.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_122;
  }

  MDSecurityDefinition_123& Parse_MDSecurityDefinition_123(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDSecurityDefinition_123.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDSecurityDefinition_123;
  }

  MDLogon_124& Parse_MDLogon_124(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDLogon_124.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDLogon_124;
  }

  MDIncRefresh_125& Parse_MDIncRefresh_125(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_125.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_125;
  }

  MDLogout_126& Parse_MDLogout_126(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDLogout_126.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDLogout_126;
  }

  MDSecurityStatus_127& Parse_MDSecurityStatus_127(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDSecurityStatus_127.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDSecurityStatus_127;
  }

  MDQuoteRequest_128& Parse_MDQuoteRequest_128(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDQuoteRequest_128.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDQuoteRequest_128;
  }

  MDIncRefresh_129& Parse_MDIncRefresh_129(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_129.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_129;
  }

  MDSecurityStatus_130& Parse_MDSecurityStatus_130(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDSecurityStatus_130.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDSecurityStatus_130;
  }

  MDIncRefresh_131& Parse_MDIncRefresh_131(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_131.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_131;
  }

  MDNewsMessage_132& Parse_MDNewsMessage_132(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDNewsMessage_132.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDNewsMessage_132;
  }

  MDHeartbeat_133& Parse_MDHeartbeat_133(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDHeartbeat_133.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDHeartbeat_133;
  }

  MDIncRefresh_134& Parse_MDIncRefresh_134(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_134.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_134;
  }

  MDIncRefresh_135& Parse_MDIncRefresh_135(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_135.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_135;
  }

  MDIncRefresh_136& Parse_MDIncRefresh_136(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_136.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_136;
  }

  MDIncRefresh_138& Parse_MDIncRefresh_138(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_138.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_138;
  }

  MDSecurityDefinition_139& Parse_MDSecurityDefinition_139(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDSecurityDefinition_139.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDSecurityDefinition_139;
  }

  MDSecurityDefinition_140& Parse_MDSecurityDefinition_140(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDSecurityDefinition_140.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDSecurityDefinition_140;
  }

  MDIncRefresh_141& Parse_MDIncRefresh_141(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_141.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_141;
  }

  MDIncRefresh_142& Parse_MDIncRefresh_142(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_142.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_142;
  }

  MDIncRefresh_143& Parse_MDIncRefresh_143(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_143.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_143;
  }

  MDIncRefresh_144& Parse_MDIncRefresh_144(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_144.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_144;
  }

  MDIncRefresh_145& Parse_MDIncRefresh_145(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_145.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_145;
  }

  MDIncRefresh_146& Parse_MDIncRefresh_146(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_146.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_146;
  }

  MDIncRefresh_147& Parse_MDIncRefresh_147(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_MDIncRefresh_147.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MDIncRefresh_147;
  }

  uint32_t Get_Template_Id(uint8_t* buf)
  {
    skip_field_mandatory(buf); // get past the pmap
    uint32_t ret;
    decode_field_mandatory<uint32_t>(buf, ret);
    return ret;
  }
};
