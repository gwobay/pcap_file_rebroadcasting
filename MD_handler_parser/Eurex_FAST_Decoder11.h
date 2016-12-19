#pragma once
#include <Data/FAST_Decoder.h>

namespace hf_core {
namespace Eurex {


struct Eurex_FAST_Decoder11_GlobalDictionary
{
  StringStateRepository m_string_repo;
  uint32_t increment_MsgSeqNum;
  FAST_Defaultable<FAST_String > default_MarketID;
  uint32_t delta_MarketSegmentID;
  uint32_t copy_TradeDate;
  FAST_String delta_MarketSegment;
  FAST_String delta_MarketSegmentDesc;
  FAST_Nullable<FAST_String> delta_MarketSegmentSymbol;
  FAST_String copy_ParentMktSegmID;
  FAST_Defaultable<FAST_String > default_Currency;
  uint32_t copy_MarketSegmentStatus;
  FAST_Defaultable<uint32_t > default_USFirmFlag;
  uint32_t copy_PartitionID;
  FAST_Nullable<FAST_String> copy_UnderlyingSecurityExchange;
  FAST_Nullable<FAST_String> delta_UnderlyingSymbol;
  FAST_Nullable<FAST_String> delta_UnderlyingSecurityID;
  FAST_Defaultable<FAST_Nullable<FAST_String> > default_UnderlyingSecurityIDSource;
  FAST_Decimal delta_UnderlyingPrevClosePx;
  FAST_Nullable<uint32_t > copy_RefMarketSegmentID;
  FAST_Defaultable<uint32_t > default_QuoteSideIndicator;
  FAST_Nullable<double > copy_FastMarketPercentage;
  FAST_Defaultable<FAST_Nullable<uint32_t > > default_MarketSegmentPoolType;
  FAST_Defaultable<uint32_t > default_InstrumentScopeOperator;
  FAST_Defaultable<uint32_t > default_InstrumentScopeSecurityType;
  uint32_t delta_InstrumentScopeSecuritySubType;
  FAST_String delta_RelatedMarketSegmentID;
  FAST_Defaultable<uint32_t > default_MarketSegmentsRelationship;
  FAST_Defaultable<uint32_t > default_TickRuleProductComplex;
  double copy_StartTickPriceRange;
  double copy_EndTickPriceRange;
  double copy_TickIncrement;
  FAST_Defaultable<uint32_t > default_MatchRuleProductComplex;
  FAST_Defaultable<uint32_t > default_MatchAlgorithm;
  FAST_Nullable<uint32_t > copy_MatchType;
  uint32_t copy_MinBidSize;
  uint32_t copy_MinOfferSize;
  uint32_t copy_FastMarketIndicator;
  uint32_t copy_FlexProductEligibilityComplex;
  FAST_Defaultable<uint32_t > default_FlexProductEligibilityIndicator;
  FAST_Defaultable<uint32_t > default_MDFeedType;
  FAST_Defaultable<uint32_t > default_MDBookType;
  FAST_Nullable<uint32_t > copy_MarketDepth;
  FAST_Nullable<uint32_t > copy_MarketDepthTimeInterval;
  FAST_Nullable<uint32_t > copy_MDRecoveryTimeInterval;
  FAST_String delta_MDPrimaryFeedLineID;
  uint32_t delta_MDPrimaryFeedLineSubID;
  FAST_Nullable<FAST_String> delta_MDSecondaryFeedLineID;
  FAST_Nullable<uint32_t > delta_MDSecondaryFeedLineSubID;
  uint32_t copy_PriceRangeRuleID;
  uint32_t copy_PriceRangeProductComplex;
  FAST_Defaultable<double > default_StartPriceRange;
  double copy_EndPriceRange;
  FAST_Nullable<double > copy_PriceRangeValue;
  FAST_Nullable<double > copy_PriceRangePercentage;
  uint64_t delta_SecurityAltID;
  uint32_t copy_SecurityType;
  FAST_Nullable<uint32_t > copy_SecuritySubType;
  uint32_t copy_ProductComplex;
  FAST_Nullable<FAST_String> copy_SecurityExchange;
  uint32_t copy_MaturityDate;
  uint32_t copy_MaturityMonthYear;
  FAST_Nullable<double > copy_StrikePrice;
  FAST_Nullable<uint32_t > copy_StrikePricePrecision;
  FAST_Nullable<uint32_t > copy_PricePrecision;
  FAST_Nullable<double > copy_ContractMultiplier;
  FAST_Defaultable<FAST_Nullable<uint32_t > > default_PutOrCall;
  FAST_Nullable<FAST_String> copy_OptAttribute;
  FAST_Nullable<uint32_t > copy_ExerciseStyle;
  FAST_Nullable<double > copy_OrigStrikePrice;
  FAST_Nullable<FAST_String> copy_ContractGenerationNumber;
  FAST_Defaultable<FAST_Nullable<uint32_t > > default_LowExercisePriceOptionIndicator;
  uint32_t copy_BlockTradeEligibilityIndicator;
  FAST_Nullable<uint32_t > copy_ValuationMethod;
  FAST_Nullable<uint32_t > copy_SettlMethod;
  FAST_Nullable<uint32_t > copy_SettlSubMethod;
  FAST_Defaultable<uint32_t > default_EventType;
  FAST_Nullable<uint32_t > copy_EventDate;
  FAST_String delta_SecurityDesc;
  uint32_t copy_LegSymbol;
  FAST_Defaultable<uint32_t > default_LegSide;
  FAST_Defaultable<uint32_t > default_LegRatioQty;
  FAST_Nullable<double > copy_LegPrice;
  FAST_Nullable<double > copy_MinPriceIncrement;
  FAST_Nullable<double > copy_MinPriceIncrementAmount;
  uint32_t copy_SecurityStatus;
  FAST_Nullable<uint64_t > copy_PrevAdjustedOpenInterest;
  FAST_Nullable<uint64_t > copy_PrevUnadjustedOpenInterest;
  FAST_Nullable<double > copy_PriorSettlPrice;
  uint32_t copy_MarketSegmentID;
  FAST_Nullable<uint32_t > copy_ImpliedMarketIndicator;
  FAST_Nullable<uint32_t > copy_MultilegModel;
  FAST_Defaultable<uint32_t > default_BusinessDayType;
  FAST_Nullable<double > copy_ClearingPriceOffset;
  FAST_Nullable<uint64_t > copy_VegaMultiplier;
  FAST_Nullable<uint32_t > copy_AnnualTradingBusinessDays;
  FAST_Nullable<uint32_t > copy_TotalTradingBusinessDays;
  FAST_Nullable<uint32_t > copy_TradingBusinessDays;
  FAST_Nullable<double > copy_StandardVariance;
  FAST_Nullable<double > copy_RelatedClosePrice;
  FAST_Nullable<double > copy_RealisedVariance;
  FAST_Nullable<double > copy_InterestRate;
  FAST_Nullable<double > copy_DiscountFactor;
  FAST_Nullable<double > copy_OvernightInterestRate;
  FAST_Nullable<double > copy_ARMVM;
  FAST_Nullable<double > copy_Volatility;
  FAST_Nullable<double > copy_SettlPrice;
  FAST_Defaultable<FAST_Nullable<uint32_t > > default_CalculationMethod;
  FAST_Nullable<uint32_t > delta_LastMsgSeqNumProcessed;
  uint32_t copy_MDReportEvent;
  FAST_Nullable<uint32_t > copy_TotNoMarketSegments;
  FAST_Nullable<uint32_t > copy_TotNoInstruments;
  uint32_t copy_SenderCompID;
  uint32_t copy_SenderSubID;


  Eurex_FAST_Decoder11_GlobalDictionary()
  {
    m_string_repo.assign_next_const_string_state<FAST_String>("XEUR", default_MarketID.default_value);
    m_string_repo.allocate_next_string_state<FAST_String>(default_MarketID.non_default_value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_MarketSegment, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_MarketSegmentDesc, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_MarketSegmentSymbol.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_ParentMktSegmID, 100);
    m_string_repo.assign_next_const_string_state<FAST_String>("EUR", default_Currency.default_value);
    m_string_repo.allocate_next_string_state<FAST_String>(default_Currency.non_default_value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_UnderlyingSecurityExchange.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_UnderlyingSymbol.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_UnderlyingSecurityID.value, 100);
    m_string_repo.assign_next_const_string_state<FAST_String>("4", default_UnderlyingSecurityIDSource.default_value.value);
    m_string_repo.allocate_next_string_state<FAST_String>(default_UnderlyingSecurityIDSource.non_default_value.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_RelatedMarketSegmentID, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_MDPrimaryFeedLineID, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_MDSecondaryFeedLineID.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_SecurityExchange.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_OptAttribute.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_ContractGenerationNumber.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_SecurityDesc, 100);
  }

  void reset()
  {
    increment_MsgSeqNum = 0;
    assign_string(default_MarketID.default_value, "XEUR", 4);

    delta_MarketSegmentID = 0;
    copy_TradeDate = 0;
    delta_MarketSegment.len = 0;

    delta_MarketSegmentDesc.len = 0;

    delta_MarketSegmentSymbol.has_value = false;

    copy_ParentMktSegmID.len = 0;

    assign_string(default_Currency.default_value, "EUR", 3);

    copy_MarketSegmentStatus = 0;
    default_USFirmFlag.is_default = true;
    default_USFirmFlag.default_value = 0;
    copy_PartitionID = 0;
    copy_UnderlyingSecurityExchange.has_value = false;

    delta_UnderlyingSymbol.has_value = false;

    delta_UnderlyingSecurityID.has_value = false;

    default_UnderlyingSecurityIDSource.default_value.has_value = true;
    assign_string(default_UnderlyingSecurityIDSource.default_value.value, "4", 1);

    delta_UnderlyingPrevClosePx.exponent = 0;
    delta_UnderlyingPrevClosePx.mantissa = 0;
    copy_RefMarketSegmentID.has_value = false;
    default_QuoteSideIndicator.is_default = true;
    default_QuoteSideIndicator.default_value = 1;
    copy_FastMarketPercentage.has_value = false;
    default_MarketSegmentPoolType.is_default = true;
    default_MarketSegmentPoolType.default_value.has_value = true;
    default_MarketSegmentPoolType.default_value.value = 1;
    default_InstrumentScopeOperator.is_default = true;
    default_InstrumentScopeOperator.default_value = 0;
    default_InstrumentScopeSecurityType.is_default = true;
    default_InstrumentScopeSecurityType.default_value = 2;
    delta_InstrumentScopeSecuritySubType = 0;
    delta_RelatedMarketSegmentID.len = 0;

    default_MarketSegmentsRelationship.is_default = true;
    default_MarketSegmentsRelationship.default_value = 100;
    default_TickRuleProductComplex.is_default = true;
    default_TickRuleProductComplex.default_value = 0;
    copy_StartTickPriceRange = 0;
    copy_EndTickPriceRange = 0;
    copy_TickIncrement = 0;
    default_MatchRuleProductComplex.is_default = true;
    default_MatchRuleProductComplex.default_value = 0;
    default_MatchAlgorithm.is_default = true;
    default_MatchAlgorithm.default_value = 0;
    copy_MatchType.has_value = false;
    copy_MinBidSize = 0;
    copy_MinOfferSize = 0;
    copy_FastMarketIndicator = 0;
    copy_FlexProductEligibilityComplex = 0;
    default_FlexProductEligibilityIndicator.is_default = true;
    default_FlexProductEligibilityIndicator.default_value = 1;
    default_MDFeedType.is_default = true;
    default_MDFeedType.default_value = 0;
    default_MDBookType.is_default = true;
    default_MDBookType.default_value = 1;
    copy_MarketDepth.has_value = false;
    copy_MarketDepthTimeInterval.has_value = false;
    copy_MDRecoveryTimeInterval.has_value = false;
    delta_MDPrimaryFeedLineID.len = 0;

    delta_MDPrimaryFeedLineSubID = 0;
    delta_MDSecondaryFeedLineID.has_value = false;

    delta_MDSecondaryFeedLineSubID.has_value = false;
    copy_PriceRangeRuleID = 0;
    copy_PriceRangeProductComplex = 0;
    default_StartPriceRange.is_default = true;
    default_StartPriceRange.default_value = 0;
    copy_EndPriceRange = 0;
    copy_PriceRangeValue.has_value = false;
    copy_PriceRangePercentage.has_value = false;
    delta_SecurityAltID = 0;
    copy_SecurityType = 0;
    copy_SecuritySubType.has_value = false;
    copy_ProductComplex = 0;
    copy_SecurityExchange.has_value = false;

    copy_MaturityDate = 0;
    copy_MaturityMonthYear = 0;
    copy_StrikePrice.has_value = false;
    copy_StrikePricePrecision.has_value = false;
    copy_PricePrecision.has_value = false;
    copy_ContractMultiplier.has_value = false;
    default_PutOrCall.is_default = true;
    default_PutOrCall.default_value.has_value = true;
    default_PutOrCall.default_value.value = 0;
    copy_OptAttribute.has_value = false;

    copy_ExerciseStyle.has_value = false;
    copy_OrigStrikePrice.has_value = false;
    copy_ContractGenerationNumber.has_value = false;

    default_LowExercisePriceOptionIndicator.is_default = true;
    default_LowExercisePriceOptionIndicator.default_value.has_value = true;
    default_LowExercisePriceOptionIndicator.default_value.value = 0;
    copy_BlockTradeEligibilityIndicator = 0;
    copy_ValuationMethod.has_value = false;
    copy_SettlMethod.has_value = false;
    copy_SettlSubMethod.has_value = false;
    default_EventType.is_default = true;
    default_EventType.default_value = 0;
    copy_EventDate.has_value = false;
    delta_SecurityDesc.len = 0;

    copy_LegSymbol = 0;
    default_LegSide.is_default = true;
    default_LegSide.default_value = 0;
    default_LegRatioQty.is_default = true;
    default_LegRatioQty.default_value = 1;
    copy_LegPrice.has_value = false;
    copy_MinPriceIncrement.has_value = false;
    copy_MinPriceIncrementAmount.has_value = false;
    copy_SecurityStatus = 0;
    copy_PrevAdjustedOpenInterest.has_value = false;
    copy_PrevUnadjustedOpenInterest.has_value = false;
    copy_PriorSettlPrice.has_value = false;
    copy_MarketSegmentID = 0;
    copy_ImpliedMarketIndicator.has_value = false;
    copy_MultilegModel.has_value = false;
    default_BusinessDayType.is_default = true;
    default_BusinessDayType.default_value = 0;
    copy_ClearingPriceOffset.has_value = false;
    copy_VegaMultiplier.has_value = false;
    copy_AnnualTradingBusinessDays.has_value = false;
    copy_TotalTradingBusinessDays.has_value = false;
    copy_TradingBusinessDays.has_value = false;
    copy_StandardVariance.has_value = false;
    copy_RelatedClosePrice.has_value = false;
    copy_RealisedVariance.has_value = false;
    copy_InterestRate.has_value = false;
    copy_DiscountFactor.has_value = false;
    copy_OvernightInterestRate.has_value = false;
    copy_ARMVM.has_value = false;
    copy_Volatility.has_value = false;
    copy_SettlPrice.has_value = false;
    default_CalculationMethod.is_default = true;
    default_CalculationMethod.default_value.has_value = true;
    default_CalculationMethod.default_value.value = 0;
    delta_LastMsgSeqNumProcessed.has_value = false;
    copy_MDReportEvent = 0;
    copy_TotNoMarketSegments.has_value = false;
    copy_TotNoInstruments.has_value = false;
    copy_SenderCompID = 0;
    copy_SenderSubID = 0;
  }
};

class RDPacketHeader
{
private:
  uint32_t m_SenderCompID_state;
  FAST_ByteVector m_PacketSeqNum;
  FAST_ByteVector m_SendingTime;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_SenderCompID_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_SenderCompID_state);
    perParseStringRepo.read_byte_vector_mandatory(buf, m_PacketSeqNum);
    perParseStringRepo.read_byte_vector_mandatory(buf, m_SendingTime);
  }
  
  inline uint32_t SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline FAST_ByteVector& PacketSeqNum()
  {
    return m_PacketSeqNum;
  }
  
  inline FAST_ByteVector& SendingTime()
  {
    return m_SendingTime;
  }
};

class ProductSnapshot_InstrumentScopes_Element
{
private:
  FAST_Defaultable<uint32_t > m_InstrumentScopeOperator_state;
  FAST_Defaultable<uint32_t > m_InstrumentScopeSecurityType_state;
  uint32_t m_InstrumentScopeSecuritySubType_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_InstrumentScopeOperator_state.is_default = true;
    m_InstrumentScopeOperator_state.default_value = 0;
    m_InstrumentScopeSecurityType_state.is_default = true;
    m_InstrumentScopeSecurityType_state.default_value = 2;
    m_InstrumentScopeSecuritySubType_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_default<uint32_t >(buf, m_InstrumentScopeOperator_state, pmap, globalDictionary.default_InstrumentScopeOperator);
    decode_mandatory_default<uint32_t >(buf, m_InstrumentScopeSecurityType_state, pmap, globalDictionary.default_InstrumentScopeSecurityType);
    decode_mandatory_delta<uint32_t >(buf, m_InstrumentScopeSecuritySubType_state, perParseStringRepo, globalDictionary.delta_InstrumentScopeSecuritySubType);
  }
  
  inline uint32_t InstrumentScopeOperator()
  {
    return m_InstrumentScopeOperator_state.current_value();
  }
  
  inline uint32_t InstrumentScopeSecurityType()
  {
    return m_InstrumentScopeSecurityType_state.current_value();
  }
  
  inline uint32_t InstrumentScopeSecuritySubType()
  {
    return m_InstrumentScopeSecuritySubType_state;
  }
};

class ProductSnapshot_RelatedMarketSegments_Element
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_RelatedMarketSegmentID_state;
  FAST_Defaultable<uint32_t > m_MarketSegmentsRelationship_state;

public:
  ProductSnapshot_RelatedMarketSegments_Element()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_RelatedMarketSegmentID_state, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_RelatedMarketSegmentID_state.len = 0;

    m_MarketSegmentsRelationship_state.is_default = true;
    m_MarketSegmentsRelationship_state.default_value = 100;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_delta<FAST_String>(buf, m_RelatedMarketSegmentID_state, perParseStringRepo, globalDictionary.delta_RelatedMarketSegmentID);
    decode_mandatory_default<uint32_t >(buf, m_MarketSegmentsRelationship_state, pmap, globalDictionary.default_MarketSegmentsRelationship);
  }
  
  inline FAST_String& RelatedMarketSegmentID()
  {
    return m_RelatedMarketSegmentID_state;
  }
  
  inline uint32_t MarketSegmentsRelationship()
  {
    return m_MarketSegmentsRelationship_state.current_value();
  }
};

class ProductSnapshot_TickRules_Element
{
private:
  FAST_Defaultable<uint32_t > m_TickRuleProductComplex_state;
  double m_StartTickPriceRange_state;
  double m_EndTickPriceRange_state;
  double m_TickIncrement_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_TickRuleProductComplex_state.is_default = true;
    m_TickRuleProductComplex_state.default_value = 0;
    m_StartTickPriceRange_state = 0;
    m_EndTickPriceRange_state = 0;
    m_TickIncrement_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_default<uint32_t >(buf, m_TickRuleProductComplex_state, pmap, globalDictionary.default_TickRuleProductComplex);
    decode_mandatory_copy<double >(buf, m_StartTickPriceRange_state, pmap, globalDictionary.copy_StartTickPriceRange);
    decode_mandatory_copy<double >(buf, m_EndTickPriceRange_state, pmap, globalDictionary.copy_EndTickPriceRange);
    decode_mandatory_copy<double >(buf, m_TickIncrement_state, pmap, globalDictionary.copy_TickIncrement);
  }
  
  inline uint32_t TickRuleProductComplex()
  {
    return m_TickRuleProductComplex_state.current_value();
  }
  
  inline double StartTickPriceRange()
  {
    return m_StartTickPriceRange_state;
  }
  
  inline double EndTickPriceRange()
  {
    return m_EndTickPriceRange_state;
  }
  
  inline double TickIncrement()
  {
    return m_TickIncrement_state;
  }
};

class ProductSnapshot_MatchRules_Element
{
private:
  FAST_Defaultable<uint32_t > m_MatchRuleProductComplex_state;
  FAST_Defaultable<uint32_t > m_MatchAlgorithm_state;
  FAST_Nullable<uint32_t > m_MatchType_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MatchRuleProductComplex_state.is_default = true;
    m_MatchRuleProductComplex_state.default_value = 0;
    m_MatchAlgorithm_state.is_default = true;
    m_MatchAlgorithm_state.default_value = 0;
    m_MatchType_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_default<uint32_t >(buf, m_MatchRuleProductComplex_state, pmap, globalDictionary.default_MatchRuleProductComplex);
    decode_mandatory_default<uint32_t >(buf, m_MatchAlgorithm_state, pmap, globalDictionary.default_MatchAlgorithm);
    decode_optional_copy<uint32_t >(buf, m_MatchType_state, pmap, globalDictionary.copy_MatchType);
  }
  
  inline uint32_t MatchRuleProductComplex()
  {
    return m_MatchRuleProductComplex_state.current_value();
  }
  
  inline uint32_t MatchAlgorithm()
  {
    return m_MatchAlgorithm_state.current_value();
  }
  
  inline FAST_Nullable<uint32_t >& MatchType()
  {
    return m_MatchType_state;
  }
};

class ProductSnapshot_QuoteSizeRules_Element
{
private:
  uint32_t m_MinBidSize_state;
  uint32_t m_MinOfferSize_state;
  uint32_t m_FastMarketIndicator_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MinBidSize_state = 0;
    m_MinOfferSize_state = 0;
    m_FastMarketIndicator_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_MinBidSize_state, pmap, globalDictionary.copy_MinBidSize);
    decode_mandatory_copy<uint32_t >(buf, m_MinOfferSize_state, pmap, globalDictionary.copy_MinOfferSize);
    decode_mandatory_copy<uint32_t >(buf, m_FastMarketIndicator_state, pmap, globalDictionary.copy_FastMarketIndicator);
  }
  
  inline uint32_t MinBidSize()
  {
    return m_MinBidSize_state;
  }
  
  inline uint32_t MinOfferSize()
  {
    return m_MinOfferSize_state;
  }
  
  inline uint32_t FastMarketIndicator()
  {
    return m_FastMarketIndicator_state;
  }
};

class ProductSnapshot_FlexRules_Element
{
private:
  uint32_t m_FlexProductEligibilityComplex_state;
  FAST_Defaultable<uint32_t > m_FlexProductEligibilityIndicator_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_FlexProductEligibilityComplex_state = 0;
    m_FlexProductEligibilityIndicator_state.is_default = true;
    m_FlexProductEligibilityIndicator_state.default_value = 1;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_FlexProductEligibilityComplex_state, pmap, globalDictionary.copy_FlexProductEligibilityComplex);
    decode_mandatory_default<uint32_t >(buf, m_FlexProductEligibilityIndicator_state, pmap, globalDictionary.default_FlexProductEligibilityIndicator);
  }
  
  inline uint32_t FlexProductEligibilityComplex()
  {
    return m_FlexProductEligibilityComplex_state;
  }
  
  inline uint32_t FlexProductEligibilityIndicator()
  {
    return m_FlexProductEligibilityIndicator_state.current_value();
  }
};

class ProductSnapshot_Feeds_Element
{
private:
  StringStateRepository m_string_repo;
  FAST_Defaultable<uint32_t > m_MDFeedType_state;
  FAST_Defaultable<uint32_t > m_MDBookType_state;
  FAST_Nullable<uint32_t > m_MarketDepth_state;
  FAST_Nullable<uint32_t > m_MarketDepthTimeInterval_state;
  FAST_Nullable<uint32_t > m_MDRecoveryTimeInterval_state;
  FAST_String m_MDPrimaryFeedLineID_state;
  uint32_t m_MDPrimaryFeedLineSubID_state;
  FAST_Nullable<FAST_String> m_MDSecondaryFeedLineID_state;
  FAST_Nullable<uint32_t > m_MDSecondaryFeedLineSubID_state;

public:
  ProductSnapshot_Feeds_Element()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_MDPrimaryFeedLineID_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_MDSecondaryFeedLineID_state.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MDFeedType_state.is_default = true;
    m_MDFeedType_state.default_value = 0;
    m_MDBookType_state.is_default = true;
    m_MDBookType_state.default_value = 1;
    m_MarketDepth_state.has_value = false;
    m_MarketDepthTimeInterval_state.has_value = false;
    m_MDRecoveryTimeInterval_state.has_value = false;
    m_MDPrimaryFeedLineID_state.len = 0;

    m_MDPrimaryFeedLineSubID_state = 0;
    m_MDSecondaryFeedLineID_state.has_value = false;

    m_MDSecondaryFeedLineSubID_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_default<uint32_t >(buf, m_MDFeedType_state, pmap, globalDictionary.default_MDFeedType);
    decode_mandatory_default<uint32_t >(buf, m_MDBookType_state, pmap, globalDictionary.default_MDBookType);
    decode_optional_copy<uint32_t >(buf, m_MarketDepth_state, pmap, globalDictionary.copy_MarketDepth);
    decode_optional_copy<uint32_t >(buf, m_MarketDepthTimeInterval_state, pmap, globalDictionary.copy_MarketDepthTimeInterval);
    decode_optional_copy<uint32_t >(buf, m_MDRecoveryTimeInterval_state, pmap, globalDictionary.copy_MDRecoveryTimeInterval);
    decode_mandatory_delta<FAST_String>(buf, m_MDPrimaryFeedLineID_state, perParseStringRepo, globalDictionary.delta_MDPrimaryFeedLineID);
    decode_mandatory_delta<uint32_t >(buf, m_MDPrimaryFeedLineSubID_state, perParseStringRepo, globalDictionary.delta_MDPrimaryFeedLineSubID);
    decode_optional_delta<FAST_String>(buf, m_MDSecondaryFeedLineID_state, perParseStringRepo, globalDictionary.delta_MDSecondaryFeedLineID);
    decode_optional_delta<uint32_t >(buf, m_MDSecondaryFeedLineSubID_state, perParseStringRepo, globalDictionary.delta_MDSecondaryFeedLineSubID);
  }
  
  inline uint32_t MDFeedType()
  {
    return m_MDFeedType_state.current_value();
  }
  
  inline uint32_t MDBookType()
  {
    return m_MDBookType_state.current_value();
  }
  
  inline FAST_Nullable<uint32_t >& MarketDepth()
  {
    return m_MarketDepth_state;
  }
  
  inline FAST_Nullable<uint32_t >& MarketDepthTimeInterval()
  {
    return m_MarketDepthTimeInterval_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDRecoveryTimeInterval()
  {
    return m_MDRecoveryTimeInterval_state;
  }
  
  inline FAST_String& MDPrimaryFeedLineID()
  {
    return m_MDPrimaryFeedLineID_state;
  }
  
  inline uint32_t MDPrimaryFeedLineSubID()
  {
    return m_MDPrimaryFeedLineSubID_state;
  }
  
  inline FAST_Nullable<FAST_String>& MDSecondaryFeedLineID()
  {
    return m_MDSecondaryFeedLineID_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDSecondaryFeedLineSubID()
  {
    return m_MDSecondaryFeedLineSubID_state;
  }
};

class ProductSnapshot_PriceRangeRules_Element
{
private:
  uint32_t m_PriceRangeRuleID_state;
  uint32_t m_PriceRangeProductComplex_state;
  FAST_Defaultable<double > m_StartPriceRange_state;
  double m_EndPriceRange_state;
  FAST_Nullable<double > m_PriceRangeValue_state;
  FAST_Nullable<double > m_PriceRangePercentage_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_PriceRangeRuleID_state = 0;
    m_PriceRangeProductComplex_state = 0;
    m_StartPriceRange_state.is_default = true;
    m_StartPriceRange_state.default_value = 0;
    m_EndPriceRange_state = 0;
    m_PriceRangeValue_state.has_value = false;
    m_PriceRangePercentage_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_PriceRangeRuleID_state, pmap, globalDictionary.copy_PriceRangeRuleID);
    decode_mandatory_copy<uint32_t >(buf, m_PriceRangeProductComplex_state, pmap, globalDictionary.copy_PriceRangeProductComplex);
    decode_mandatory_default<double >(buf, m_StartPriceRange_state, pmap, globalDictionary.default_StartPriceRange);
    decode_mandatory_copy<double >(buf, m_EndPriceRange_state, pmap, globalDictionary.copy_EndPriceRange);
    decode_optional_copy<double >(buf, m_PriceRangeValue_state, pmap, globalDictionary.copy_PriceRangeValue);
    decode_optional_copy<double >(buf, m_PriceRangePercentage_state, pmap, globalDictionary.copy_PriceRangePercentage);
  }
  
  inline uint32_t PriceRangeRuleID()
  {
    return m_PriceRangeRuleID_state;
  }
  
  inline uint32_t PriceRangeProductComplex()
  {
    return m_PriceRangeProductComplex_state;
  }
  
  inline double StartPriceRange()
  {
    return m_StartPriceRange_state.current_value();
  }
  
  inline double EndPriceRange()
  {
    return m_EndPriceRange_state;
  }
  
  inline FAST_Nullable<double >& PriceRangeValue()
  {
    return m_PriceRangeValue_state;
  }
  
  inline FAST_Nullable<double >& PriceRangePercentage()
  {
    return m_PriceRangePercentage_state;
  }
};

class ProductSnapshot
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_MsgType_state;
  uint32_t m_MsgSeqNum_state;
  FAST_Defaultable<FAST_String > m_MarketID_state;
  uint32_t m_MarketSegmentID_state;
  uint32_t m_TradeDate_state;
  FAST_String m_MarketSegment_state;
  FAST_String m_MarketSegmentDesc_state;
  FAST_Nullable<FAST_String> m_MarketSegmentSymbol_state;
  FAST_String m_ParentMktSegmID_state;
  FAST_Defaultable<FAST_String > m_Currency_state;
  uint32_t m_MarketSegmentStatus_state;
  FAST_Defaultable<uint32_t > m_USFirmFlag_state;
  uint32_t m_PartitionID_state;
  FAST_Nullable<FAST_String> m_UnderlyingSecurityExchange_state;
  FAST_Nullable<FAST_String> m_UnderlyingSymbol_state;
  FAST_Nullable<FAST_String> m_UnderlyingSecurityID_state;
  FAST_Defaultable<FAST_Nullable<FAST_String> > m_UnderlyingSecurityIDSource_state;
  FAST_Nullable<double > m_UnderlyingPrevClosePx_state;
  FAST_Nullable<uint32_t > m_RefMarketSegmentID_state;
  FAST_Defaultable<uint32_t > m_QuoteSideIndicator_state;
  FAST_Nullable<double > m_FastMarketPercentage_state;
  FAST_Defaultable<FAST_Nullable<uint32_t > > m_MarketSegmentPoolType_state;
  FAST_Sequence<ProductSnapshot_InstrumentScopes_Element, 50> m_InstrumentScopes;
  FAST_Sequence<ProductSnapshot_RelatedMarketSegments_Element, 50> m_RelatedMarketSegments;
  FAST_Sequence<ProductSnapshot_TickRules_Element, 50> m_TickRules;
  FAST_Sequence<ProductSnapshot_MatchRules_Element, 50> m_MatchRules;
  FAST_Sequence<ProductSnapshot_QuoteSizeRules_Element, 50> m_QuoteSizeRules;
  FAST_Sequence<ProductSnapshot_FlexRules_Element, 50> m_FlexRules;
  FAST_Sequence<ProductSnapshot_Feeds_Element, 50> m_Feeds;
  FAST_Sequence<ProductSnapshot_PriceRangeRules_Element, 50> m_PriceRangeRules;

public:
  ProductSnapshot()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_MarketID_state.default_value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_MarketID_state.non_default_value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_MarketSegment_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_MarketSegmentDesc_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_MarketSegmentSymbol_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_ParentMktSegmID_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_Currency_state.default_value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_Currency_state.non_default_value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_UnderlyingSecurityExchange_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_UnderlyingSymbol_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_UnderlyingSecurityID_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_UnderlyingSecurityIDSource_state.default_value.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_UnderlyingSecurityIDSource_state.non_default_value.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("BU", m_MsgType_state);
    m_MsgSeqNum_state = 0;
    assign_string(m_MarketID_state.default_value, "XEUR", 4);

    m_MarketSegmentID_state = 0;
    m_TradeDate_state = 0;
    m_MarketSegment_state.len = 0;

    m_MarketSegmentDesc_state.len = 0;

    m_MarketSegmentSymbol_state.has_value = false;

    m_ParentMktSegmID_state.len = 0;

    assign_string(m_Currency_state.default_value, "EUR", 3);

    m_MarketSegmentStatus_state = 0;
    m_USFirmFlag_state.is_default = true;
    m_USFirmFlag_state.default_value = 0;
    m_PartitionID_state = 0;
    m_UnderlyingSecurityExchange_state.has_value = false;

    m_UnderlyingSymbol_state.has_value = false;

    m_UnderlyingSecurityID_state.has_value = false;

    m_UnderlyingSecurityIDSource_state.default_value.has_value = true;
    assign_string(m_UnderlyingSecurityIDSource_state.default_value.value, "4", 1);

    m_UnderlyingPrevClosePx_state.value = 0;
    m_RefMarketSegmentID_state.has_value = false;
    m_QuoteSideIndicator_state.is_default = true;
    m_QuoteSideIndicator_state.default_value = 1;
    m_FastMarketPercentage_state.has_value = false;
    m_MarketSegmentPoolType_state.is_default = true;
    m_MarketSegmentPoolType_state.default_value.has_value = true;
    m_MarketSegmentPoolType_state.default_value.value = 1;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_mandatory_increment<uint32_t >(buf, m_MsgSeqNum_state, pmap, globalDictionary.increment_MsgSeqNum);
    decode_mandatory_default<FAST_String>(buf, m_MarketID_state, pmap, globalDictionary.default_MarketID);
    decode_mandatory_delta<uint32_t >(buf, m_MarketSegmentID_state, perParseStringRepo, globalDictionary.delta_MarketSegmentID);
    decode_mandatory_copy<uint32_t >(buf, m_TradeDate_state, pmap, globalDictionary.copy_TradeDate);
    decode_mandatory_delta<FAST_String>(buf, m_MarketSegment_state, perParseStringRepo, globalDictionary.delta_MarketSegment);
    decode_mandatory_delta<FAST_String>(buf, m_MarketSegmentDesc_state, perParseStringRepo, globalDictionary.delta_MarketSegmentDesc);
    decode_optional_delta<FAST_String>(buf, m_MarketSegmentSymbol_state, perParseStringRepo, globalDictionary.delta_MarketSegmentSymbol);
    decode_mandatory_copy<FAST_String>(buf, m_ParentMktSegmID_state, pmap, globalDictionary.copy_ParentMktSegmID);
    decode_mandatory_default<FAST_String>(buf, m_Currency_state, pmap, globalDictionary.default_Currency);
    decode_mandatory_copy<uint32_t >(buf, m_MarketSegmentStatus_state, pmap, globalDictionary.copy_MarketSegmentStatus);
    decode_mandatory_default<uint32_t >(buf, m_USFirmFlag_state, pmap, globalDictionary.default_USFirmFlag);
    decode_mandatory_copy<uint32_t >(buf, m_PartitionID_state, pmap, globalDictionary.copy_PartitionID);
    decode_optional_copy<FAST_String>(buf, m_UnderlyingSecurityExchange_state, pmap, globalDictionary.copy_UnderlyingSecurityExchange);
    decode_optional_delta<FAST_String>(buf, m_UnderlyingSymbol_state, perParseStringRepo, globalDictionary.delta_UnderlyingSymbol);
    decode_optional_delta<FAST_String>(buf, m_UnderlyingSecurityID_state, perParseStringRepo, globalDictionary.delta_UnderlyingSecurityID);
    decode_optional_default<FAST_String>(buf, m_UnderlyingSecurityIDSource_state, pmap, globalDictionary.default_UnderlyingSecurityIDSource);
    decode_optional_decimal_delta(buf, m_UnderlyingPrevClosePx_state, perParseStringRepo, globalDictionary.delta_UnderlyingPrevClosePx);
    decode_optional_copy<uint32_t >(buf, m_RefMarketSegmentID_state, pmap, globalDictionary.copy_RefMarketSegmentID);
    decode_mandatory_default<uint32_t >(buf, m_QuoteSideIndicator_state, pmap, globalDictionary.default_QuoteSideIndicator);
    decode_optional_copy<double >(buf, m_FastMarketPercentage_state, pmap, globalDictionary.copy_FastMarketPercentage);
    decode_optional_default<uint32_t >(buf, m_MarketSegmentPoolType_state, pmap, globalDictionary.default_MarketSegmentPoolType);
    decode_optional_sequence(buf, m_InstrumentScopes, perParseStringRepo, globalDictionary);
    decode_optional_sequence(buf, m_RelatedMarketSegments, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_TickRules, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_MatchRules, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_QuoteSizeRules, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_FlexRules, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_Feeds, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_PriceRangeRules, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& MsgType()
  {
    return m_MsgType_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline FAST_String& MarketID()
  {
    return m_MarketID_state.current_value();
  }
  
  inline uint32_t MarketSegmentID()
  {
    return m_MarketSegmentID_state;
  }
  
  inline uint32_t TradeDate()
  {
    return m_TradeDate_state;
  }
  
  inline FAST_String& MarketSegment()
  {
    return m_MarketSegment_state;
  }
  
  inline FAST_String& MarketSegmentDesc()
  {
    return m_MarketSegmentDesc_state;
  }
  
  inline FAST_Nullable<FAST_String>& MarketSegmentSymbol()
  {
    return m_MarketSegmentSymbol_state;
  }
  
  inline FAST_String& ParentMktSegmID()
  {
    return m_ParentMktSegmID_state;
  }
  
  inline FAST_String& Currency()
  {
    return m_Currency_state.current_value();
  }
  
  inline uint32_t MarketSegmentStatus()
  {
    return m_MarketSegmentStatus_state;
  }
  
  inline uint32_t USFirmFlag()
  {
    return m_USFirmFlag_state.current_value();
  }
  
  inline uint32_t PartitionID()
  {
    return m_PartitionID_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnderlyingSecurityExchange()
  {
    return m_UnderlyingSecurityExchange_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnderlyingSymbol()
  {
    return m_UnderlyingSymbol_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnderlyingSecurityID()
  {
    return m_UnderlyingSecurityID_state;
  }
  
  inline FAST_Nullable<FAST_String>& UnderlyingSecurityIDSource()
  {
    return m_UnderlyingSecurityIDSource_state.current_value();
  }
  
  inline FAST_Nullable<double >& UnderlyingPrevClosePx()
  {
    return m_UnderlyingPrevClosePx_state;
  }
  
  inline FAST_Nullable<uint32_t >& RefMarketSegmentID()
  {
    return m_RefMarketSegmentID_state;
  }
  
  inline uint32_t QuoteSideIndicator()
  {
    return m_QuoteSideIndicator_state.current_value();
  }
  
  inline FAST_Nullable<double >& FastMarketPercentage()
  {
    return m_FastMarketPercentage_state;
  }
  
  inline FAST_Nullable<uint32_t >& MarketSegmentPoolType()
  {
    return m_MarketSegmentPoolType_state.current_value();
  }
  
  inline FAST_Sequence<ProductSnapshot_InstrumentScopes_Element, 50>& InstrumentScopes()
  {
    return m_InstrumentScopes;
  }
  
  inline FAST_Sequence<ProductSnapshot_RelatedMarketSegments_Element, 50>& RelatedMarketSegments()
  {
    return m_RelatedMarketSegments;
  }
  
  inline FAST_Sequence<ProductSnapshot_TickRules_Element, 50>& TickRules()
  {
    return m_TickRules;
  }
  
  inline FAST_Sequence<ProductSnapshot_MatchRules_Element, 50>& MatchRules()
  {
    return m_MatchRules;
  }
  
  inline FAST_Sequence<ProductSnapshot_QuoteSizeRules_Element, 50>& QuoteSizeRules()
  {
    return m_QuoteSizeRules;
  }
  
  inline FAST_Sequence<ProductSnapshot_FlexRules_Element, 50>& FlexRules()
  {
    return m_FlexRules;
  }
  
  inline FAST_Sequence<ProductSnapshot_Feeds_Element, 50>& Feeds()
  {
    return m_Feeds;
  }
  
  inline FAST_Sequence<ProductSnapshot_PriceRangeRules_Element, 50>& PriceRangeRules()
  {
    return m_PriceRangeRules;
  }
};

class InstrumentSnapshot_SecurityAlt_Element
{
private:
  uint64_t m_SecurityAltID_state;
  FAST_String m_SecurityAltIDSource_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_SecurityAltID_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("M", m_SecurityAltIDSource_state);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    decode_mandatory_delta<uint64_t >(buf, m_SecurityAltID_state, perParseStringRepo, globalDictionary.delta_SecurityAltID);
  }
  
  inline uint64_t SecurityAltID()
  {
    return m_SecurityAltID_state;
  }
  
  inline FAST_String& SecurityAltIDSource()
  {
    return m_SecurityAltIDSource_state;
  }
};

class InstrumentSnapshot_SimpleInstrumentDescriptorGroup_Group_Events_Element
{
private:
  FAST_Defaultable<uint32_t > m_EventType_state;
  FAST_Nullable<uint32_t > m_EventDate_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_EventType_state.is_default = true;
    m_EventType_state.default_value = 0;
    m_EventDate_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_default<uint32_t >(buf, m_EventType_state, pmap, globalDictionary.default_EventType);
    decode_optional_copy<uint32_t >(buf, m_EventDate_state, pmap, globalDictionary.copy_EventDate);
  }
  
  inline uint32_t EventType()
  {
    return m_EventType_state.current_value();
  }
  
  inline FAST_Nullable<uint32_t >& EventDate()
  {
    return m_EventDate_state;
  }
};

class InstrumentSnapshot_SimpleInstrumentDescriptorGroup_Group
{
private:
  StringStateRepository m_string_repo;
  uint32_t m_MaturityDate_state;
  uint32_t m_MaturityMonthYear_state;
  FAST_Nullable<double > m_StrikePrice_state;
  FAST_Nullable<uint32_t > m_StrikePricePrecision_state;
  FAST_Nullable<uint32_t > m_PricePrecision_state;
  FAST_Nullable<double > m_ContractMultiplier_state;
  FAST_Defaultable<FAST_Nullable<uint32_t > > m_PutOrCall_state;
  FAST_Nullable<FAST_String> m_OptAttribute_state;
  FAST_Nullable<uint32_t > m_ExerciseStyle_state;
  FAST_Nullable<double > m_OrigStrikePrice_state;
  FAST_Nullable<FAST_String> m_ContractGenerationNumber_state;
  FAST_Defaultable<FAST_Nullable<uint32_t > > m_LowExercisePriceOptionIndicator_state;
  uint32_t m_BlockTradeEligibilityIndicator_state;
  FAST_Nullable<uint32_t > m_ValuationMethod_state;
  FAST_Nullable<uint32_t > m_SettlMethod_state;
  FAST_Nullable<uint32_t > m_SettlSubMethod_state;
  FAST_Sequence<InstrumentSnapshot_SimpleInstrumentDescriptorGroup_Group_Events_Element, 50> m_Events;

public:
  InstrumentSnapshot_SimpleInstrumentDescriptorGroup_Group()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_OptAttribute_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_ContractGenerationNumber_state.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MaturityDate_state = 0;
    m_MaturityMonthYear_state = 0;
    m_StrikePrice_state.has_value = false;
    m_StrikePricePrecision_state.has_value = false;
    m_PricePrecision_state.has_value = false;
    m_ContractMultiplier_state.has_value = false;
    m_PutOrCall_state.is_default = true;
    m_PutOrCall_state.default_value.has_value = true;
    m_PutOrCall_state.default_value.value = 0;
    m_OptAttribute_state.has_value = false;

    m_ExerciseStyle_state.has_value = false;
    m_OrigStrikePrice_state.has_value = false;
    m_ContractGenerationNumber_state.has_value = false;

    m_LowExercisePriceOptionIndicator_state.is_default = true;
    m_LowExercisePriceOptionIndicator_state.default_value.has_value = true;
    m_LowExercisePriceOptionIndicator_state.default_value.value = 0;
    m_BlockTradeEligibilityIndicator_state = 0;
    m_ValuationMethod_state.has_value = false;
    m_SettlMethod_state.has_value = false;
    m_SettlSubMethod_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_MaturityDate_state, pmap, globalDictionary.copy_MaturityDate);
    decode_mandatory_copy<uint32_t >(buf, m_MaturityMonthYear_state, pmap, globalDictionary.copy_MaturityMonthYear);
    decode_optional_copy<double >(buf, m_StrikePrice_state, pmap, globalDictionary.copy_StrikePrice);
    decode_optional_copy<uint32_t >(buf, m_StrikePricePrecision_state, pmap, globalDictionary.copy_StrikePricePrecision);
    decode_optional_copy<uint32_t >(buf, m_PricePrecision_state, pmap, globalDictionary.copy_PricePrecision);
    decode_optional_copy<double >(buf, m_ContractMultiplier_state, pmap, globalDictionary.copy_ContractMultiplier);
    decode_optional_default<uint32_t >(buf, m_PutOrCall_state, pmap, globalDictionary.default_PutOrCall);
    decode_optional_copy<FAST_String>(buf, m_OptAttribute_state, pmap, globalDictionary.copy_OptAttribute);
    decode_optional_copy<uint32_t >(buf, m_ExerciseStyle_state, pmap, globalDictionary.copy_ExerciseStyle);
    decode_optional_copy<double >(buf, m_OrigStrikePrice_state, pmap, globalDictionary.copy_OrigStrikePrice);
    decode_optional_copy<FAST_String>(buf, m_ContractGenerationNumber_state, pmap, globalDictionary.copy_ContractGenerationNumber);
    decode_optional_default<uint32_t >(buf, m_LowExercisePriceOptionIndicator_state, pmap, globalDictionary.default_LowExercisePriceOptionIndicator);
    decode_mandatory_copy<uint32_t >(buf, m_BlockTradeEligibilityIndicator_state, pmap, globalDictionary.copy_BlockTradeEligibilityIndicator);
    decode_optional_copy<uint32_t >(buf, m_ValuationMethod_state, pmap, globalDictionary.copy_ValuationMethod);
    decode_optional_copy<uint32_t >(buf, m_SettlMethod_state, pmap, globalDictionary.copy_SettlMethod);
    decode_optional_copy<uint32_t >(buf, m_SettlSubMethod_state, pmap, globalDictionary.copy_SettlSubMethod);
    decode_optional_sequence(buf, m_Events, perParseStringRepo, globalDictionary);
  }
  
  inline uint32_t MaturityDate()
  {
    return m_MaturityDate_state;
  }
  
  inline uint32_t MaturityMonthYear()
  {
    return m_MaturityMonthYear_state;
  }
  
  inline FAST_Nullable<double >& StrikePrice()
  {
    return m_StrikePrice_state;
  }
  
  inline FAST_Nullable<uint32_t >& StrikePricePrecision()
  {
    return m_StrikePricePrecision_state;
  }
  
  inline FAST_Nullable<uint32_t >& PricePrecision()
  {
    return m_PricePrecision_state;
  }
  
  inline FAST_Nullable<double >& ContractMultiplier()
  {
    return m_ContractMultiplier_state;
  }
  
  inline FAST_Nullable<uint32_t >& PutOrCall()
  {
    return m_PutOrCall_state.current_value();
  }
  
  inline FAST_Nullable<FAST_String>& OptAttribute()
  {
    return m_OptAttribute_state;
  }
  
  inline FAST_Nullable<uint32_t >& ExerciseStyle()
  {
    return m_ExerciseStyle_state;
  }
  
  inline FAST_Nullable<double >& OrigStrikePrice()
  {
    return m_OrigStrikePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& ContractGenerationNumber()
  {
    return m_ContractGenerationNumber_state;
  }
  
  inline FAST_Nullable<uint32_t >& LowExercisePriceOptionIndicator()
  {
    return m_LowExercisePriceOptionIndicator_state.current_value();
  }
  
  inline uint32_t BlockTradeEligibilityIndicator()
  {
    return m_BlockTradeEligibilityIndicator_state;
  }
  
  inline FAST_Nullable<uint32_t >& ValuationMethod()
  {
    return m_ValuationMethod_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlMethod()
  {
    return m_SettlMethod_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlSubMethod()
  {
    return m_SettlSubMethod_state;
  }
  
  inline FAST_Sequence<InstrumentSnapshot_SimpleInstrumentDescriptorGroup_Group_Events_Element, 50>& Events()
  {
    return m_Events;
  }
};

class InstrumentSnapshot_ComplexInstrumentDescriptorGroup_Group_InstrmtLegGrp_Element
{
private:
  uint32_t m_LegSymbol_state;
  FAST_String m_LegSecurityIDSource_state;
  FAST_Defaultable<uint32_t > m_LegSide_state;
  FAST_Defaultable<uint32_t > m_LegRatioQty_state;
  FAST_Nullable<double > m_LegPrice_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_LegSymbol_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("M", m_LegSecurityIDSource_state);
    m_LegSide_state.is_default = true;
    m_LegSide_state.default_value = 0;
    m_LegRatioQty_state.is_default = true;
    m_LegRatioQty_state.default_value = 1;
    m_LegPrice_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_LegSymbol_state, pmap, globalDictionary.copy_LegSymbol);
    decode_mandatory_default<uint32_t >(buf, m_LegSide_state, pmap, globalDictionary.default_LegSide);
    decode_mandatory_default<uint32_t >(buf, m_LegRatioQty_state, pmap, globalDictionary.default_LegRatioQty);
    decode_optional_copy<double >(buf, m_LegPrice_state, pmap, globalDictionary.copy_LegPrice);
  }
  
  inline uint32_t LegSymbol()
  {
    return m_LegSymbol_state;
  }
  
  inline FAST_String& LegSecurityIDSource()
  {
    return m_LegSecurityIDSource_state;
  }
  
  inline uint32_t LegSide()
  {
    return m_LegSide_state.current_value();
  }
  
  inline uint32_t LegRatioQty()
  {
    return m_LegRatioQty_state.current_value();
  }
  
  inline FAST_Nullable<double >& LegPrice()
  {
    return m_LegPrice_state;
  }
};

class InstrumentSnapshot_ComplexInstrumentDescriptorGroup_Group
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_SecurityDesc_state;
  FAST_Sequence<InstrumentSnapshot_ComplexInstrumentDescriptorGroup_Group_InstrmtLegGrp_Element, 50> m_InstrmtLegGrp;

public:
  InstrumentSnapshot_ComplexInstrumentDescriptorGroup_Group()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_SecurityDesc_state, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_SecurityDesc_state.len = 0;

  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    decode_mandatory_delta<FAST_String>(buf, m_SecurityDesc_state, perParseStringRepo, globalDictionary.delta_SecurityDesc);
    decode_mandatory_sequence(buf, m_InstrmtLegGrp, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& SecurityDesc()
  {
    return m_SecurityDesc_state;
  }
  
  inline FAST_Sequence<InstrumentSnapshot_ComplexInstrumentDescriptorGroup_Group_InstrmtLegGrp_Element, 50>& InstrmtLegGrp()
  {
    return m_InstrmtLegGrp;
  }
};

class InstrumentSnapshot_MarketSegmentGrp_Element_PriceRangeRules_Element
{
private:
  uint32_t m_PriceRangeRuleID_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_PriceRangeRuleID_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_PriceRangeRuleID_state, pmap, globalDictionary.copy_PriceRangeRuleID);
  }
  
  inline uint32_t PriceRangeRuleID()
  {
    return m_PriceRangeRuleID_state;
  }
};

class InstrumentSnapshot_MarketSegmentGrp_Element
{
private:
  uint32_t m_MarketSegmentID_state;
  FAST_Nullable<uint32_t > m_ImpliedMarketIndicator_state;
  FAST_Nullable<uint32_t > m_MultilegModel_state;
  FAST_Sequence<InstrumentSnapshot_MarketSegmentGrp_Element_PriceRangeRules_Element, 50> m_PriceRangeRules;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MarketSegmentID_state = 0;
    m_ImpliedMarketIndicator_state.has_value = false;
    m_MultilegModel_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_MarketSegmentID_state, pmap, globalDictionary.copy_MarketSegmentID);
    decode_optional_copy<uint32_t >(buf, m_ImpliedMarketIndicator_state, pmap, globalDictionary.copy_ImpliedMarketIndicator);
    decode_optional_copy<uint32_t >(buf, m_MultilegModel_state, pmap, globalDictionary.copy_MultilegModel);
    decode_optional_sequence(buf, m_PriceRangeRules, perParseStringRepo, globalDictionary);
  }
  
  inline uint32_t MarketSegmentID()
  {
    return m_MarketSegmentID_state;
  }
  
  inline FAST_Nullable<uint32_t >& ImpliedMarketIndicator()
  {
    return m_ImpliedMarketIndicator_state;
  }
  
  inline FAST_Nullable<uint32_t >& MultilegModel()
  {
    return m_MultilegModel_state;
  }
  
  inline FAST_Sequence<InstrumentSnapshot_MarketSegmentGrp_Element_PriceRangeRules_Element, 50>& PriceRangeRules()
  {
    return m_PriceRangeRules;
  }
};

class InstrumentSnapshot
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_MsgType_state;
  uint32_t m_MsgSeqNum_state;
  FAST_String m_SecurityIDSource_state;
  FAST_Sequence<InstrumentSnapshot_SecurityAlt_Element, 50> m_SecurityAlt;
  uint32_t m_SecurityType_state;
  FAST_Nullable<uint32_t > m_SecuritySubType_state;
  uint32_t m_ProductComplex_state;
  FAST_Nullable<FAST_String> m_SecurityExchange_state;
  FAST_Group<InstrumentSnapshot_SimpleInstrumentDescriptorGroup_Group> m_SimpleInstrumentDescriptorGroup;
  FAST_Group<InstrumentSnapshot_ComplexInstrumentDescriptorGroup_Group> m_ComplexInstrumentDescriptorGroup;
  FAST_Nullable<double > m_MinPriceIncrement_state;
  FAST_Nullable<double > m_MinPriceIncrementAmount_state;
  uint32_t m_SecurityStatus_state;
  FAST_Nullable<uint64_t > m_PrevAdjustedOpenInterest_state;
  FAST_Nullable<uint64_t > m_PrevUnadjustedOpenInterest_state;
  FAST_Nullable<double > m_PriorSettlPrice_state;
  FAST_Sequence<InstrumentSnapshot_MarketSegmentGrp_Element, 50> m_MarketSegmentGrp;

public:
  InstrumentSnapshot()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_SecurityExchange_state.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("d", m_MsgType_state);
    m_MsgSeqNum_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("M", m_SecurityIDSource_state);
    m_SecurityType_state = 0;
    m_SecuritySubType_state.has_value = false;
    m_ProductComplex_state = 0;
    m_SecurityExchange_state.has_value = false;

    m_MinPriceIncrement_state.has_value = false;
    m_MinPriceIncrementAmount_state.has_value = false;
    m_SecurityStatus_state = 0;
    m_PrevAdjustedOpenInterest_state.has_value = false;
    m_PrevUnadjustedOpenInterest_state.has_value = false;
    m_PriorSettlPrice_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_mandatory_increment<uint32_t >(buf, m_MsgSeqNum_state, pmap, globalDictionary.increment_MsgSeqNum);
    decode_optional_sequence(buf, m_SecurityAlt, perParseStringRepo, globalDictionary);
    decode_mandatory_copy<uint32_t >(buf, m_SecurityType_state, pmap, globalDictionary.copy_SecurityType);
    decode_optional_copy<uint32_t >(buf, m_SecuritySubType_state, pmap, globalDictionary.copy_SecuritySubType);
    decode_mandatory_copy<uint32_t >(buf, m_ProductComplex_state, pmap, globalDictionary.copy_ProductComplex);
    decode_optional_copy<FAST_String>(buf, m_SecurityExchange_state, pmap, globalDictionary.copy_SecurityExchange);
    decode_optional_group(buf, m_SimpleInstrumentDescriptorGroup, pmap, perParseStringRepo, globalDictionary);
    decode_optional_group(buf, m_ComplexInstrumentDescriptorGroup, pmap, perParseStringRepo, globalDictionary);
    decode_optional_copy<double >(buf, m_MinPriceIncrement_state, pmap, globalDictionary.copy_MinPriceIncrement);
    decode_optional_copy<double >(buf, m_MinPriceIncrementAmount_state, pmap, globalDictionary.copy_MinPriceIncrementAmount);
    decode_mandatory_copy<uint32_t >(buf, m_SecurityStatus_state, pmap, globalDictionary.copy_SecurityStatus);
    decode_optional_copy<uint64_t >(buf, m_PrevAdjustedOpenInterest_state, pmap, globalDictionary.copy_PrevAdjustedOpenInterest);
    decode_optional_copy<uint64_t >(buf, m_PrevUnadjustedOpenInterest_state, pmap, globalDictionary.copy_PrevUnadjustedOpenInterest);
    decode_optional_copy<double >(buf, m_PriorSettlPrice_state, pmap, globalDictionary.copy_PriorSettlPrice);
    decode_mandatory_sequence(buf, m_MarketSegmentGrp, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& MsgType()
  {
    return m_MsgType_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline FAST_String& SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Sequence<InstrumentSnapshot_SecurityAlt_Element, 50>& SecurityAlt()
  {
    return m_SecurityAlt;
  }
  
  inline uint32_t SecurityType()
  {
    return m_SecurityType_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecuritySubType()
  {
    return m_SecuritySubType_state;
  }
  
  inline uint32_t ProductComplex()
  {
    return m_ProductComplex_state;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityExchange()
  {
    return m_SecurityExchange_state;
  }
  
  inline FAST_Group<InstrumentSnapshot_SimpleInstrumentDescriptorGroup_Group>& SimpleInstrumentDescriptorGroup()
  {
    return m_SimpleInstrumentDescriptorGroup;
  }
  
  inline FAST_Group<InstrumentSnapshot_ComplexInstrumentDescriptorGroup_Group>& ComplexInstrumentDescriptorGroup()
  {
    return m_ComplexInstrumentDescriptorGroup;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrement()
  {
    return m_MinPriceIncrement_state;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrementAmount()
  {
    return m_MinPriceIncrementAmount_state;
  }
  
  inline uint32_t SecurityStatus()
  {
    return m_SecurityStatus_state;
  }
  
  inline FAST_Nullable<uint64_t >& PrevAdjustedOpenInterest()
  {
    return m_PrevAdjustedOpenInterest_state;
  }
  
  inline FAST_Nullable<uint64_t >& PrevUnadjustedOpenInterest()
  {
    return m_PrevUnadjustedOpenInterest_state;
  }
  
  inline FAST_Nullable<double >& PriorSettlPrice()
  {
    return m_PriorSettlPrice_state;
  }
  
  inline FAST_Sequence<InstrumentSnapshot_MarketSegmentGrp_Element, 50>& MarketSegmentGrp()
  {
    return m_MarketSegmentGrp;
  }
};

class InstrumentIncremental_SecurityAlt_Element
{
private:
  uint64_t m_SecurityAltID_state;
  FAST_String m_SecurityAltIDSource_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_SecurityAltID_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("M", m_SecurityAltIDSource_state);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    decode_mandatory_delta<uint64_t >(buf, m_SecurityAltID_state, perParseStringRepo, globalDictionary.delta_SecurityAltID);
  }
  
  inline uint64_t SecurityAltID()
  {
    return m_SecurityAltID_state;
  }
  
  inline FAST_String& SecurityAltIDSource()
  {
    return m_SecurityAltIDSource_state;
  }
};

class InstrumentIncremental_SimpleInstrumentDescriptorGroup_Group_Events_Element
{
private:
  FAST_Defaultable<uint32_t > m_EventType_state;
  FAST_Nullable<uint32_t > m_EventDate_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_EventType_state.is_default = true;
    m_EventType_state.default_value = 0;
    m_EventDate_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_default<uint32_t >(buf, m_EventType_state, pmap, globalDictionary.default_EventType);
    decode_optional_copy<uint32_t >(buf, m_EventDate_state, pmap, globalDictionary.copy_EventDate);
  }
  
  inline uint32_t EventType()
  {
    return m_EventType_state.current_value();
  }
  
  inline FAST_Nullable<uint32_t >& EventDate()
  {
    return m_EventDate_state;
  }
};

class InstrumentIncremental_SimpleInstrumentDescriptorGroup_Group
{
private:
  StringStateRepository m_string_repo;
  uint32_t m_MaturityDate_state;
  uint32_t m_MaturityMonthYear_state;
  FAST_Nullable<double > m_StrikePrice_state;
  FAST_Nullable<uint32_t > m_StrikePricePrecision_state;
  FAST_Nullable<uint32_t > m_PricePrecision_state;
  FAST_Nullable<double > m_ContractMultiplier_state;
  FAST_Defaultable<FAST_Nullable<uint32_t > > m_PutOrCall_state;
  FAST_Nullable<FAST_String> m_OptAttribute_state;
  FAST_Nullable<uint32_t > m_ExerciseStyle_state;
  FAST_Nullable<double > m_OrigStrikePrice_state;
  FAST_Nullable<FAST_String> m_ContractGenerationNumber_state;
  FAST_Defaultable<FAST_Nullable<uint32_t > > m_LowExercisePriceOptionIndicator_state;
  uint32_t m_BlockTradeEligibilityIndicator_state;
  FAST_Nullable<uint32_t > m_ValuationMethod_state;
  FAST_Nullable<uint32_t > m_SettlMethod_state;
  FAST_Nullable<uint32_t > m_SettlSubMethod_state;
  FAST_Sequence<InstrumentIncremental_SimpleInstrumentDescriptorGroup_Group_Events_Element, 50> m_Events;

public:
  InstrumentIncremental_SimpleInstrumentDescriptorGroup_Group()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_OptAttribute_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_ContractGenerationNumber_state.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MaturityDate_state = 0;
    m_MaturityMonthYear_state = 0;
    m_StrikePrice_state.has_value = false;
    m_StrikePricePrecision_state.has_value = false;
    m_PricePrecision_state.has_value = false;
    m_ContractMultiplier_state.has_value = false;
    m_PutOrCall_state.is_default = true;
    m_PutOrCall_state.default_value.has_value = true;
    m_PutOrCall_state.default_value.value = 0;
    m_OptAttribute_state.has_value = false;

    m_ExerciseStyle_state.has_value = false;
    m_OrigStrikePrice_state.has_value = false;
    m_ContractGenerationNumber_state.has_value = false;

    m_LowExercisePriceOptionIndicator_state.is_default = true;
    m_LowExercisePriceOptionIndicator_state.default_value.has_value = true;
    m_LowExercisePriceOptionIndicator_state.default_value.value = 0;
    m_BlockTradeEligibilityIndicator_state = 0;
    m_ValuationMethod_state.has_value = false;
    m_SettlMethod_state.has_value = false;
    m_SettlSubMethod_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_MaturityDate_state, pmap, globalDictionary.copy_MaturityDate);
    decode_mandatory_copy<uint32_t >(buf, m_MaturityMonthYear_state, pmap, globalDictionary.copy_MaturityMonthYear);
    decode_optional_copy<double >(buf, m_StrikePrice_state, pmap, globalDictionary.copy_StrikePrice);
    decode_optional_copy<uint32_t >(buf, m_StrikePricePrecision_state, pmap, globalDictionary.copy_StrikePricePrecision);
    decode_optional_copy<uint32_t >(buf, m_PricePrecision_state, pmap, globalDictionary.copy_PricePrecision);
    decode_optional_copy<double >(buf, m_ContractMultiplier_state, pmap, globalDictionary.copy_ContractMultiplier);
    decode_optional_default<uint32_t >(buf, m_PutOrCall_state, pmap, globalDictionary.default_PutOrCall);
    decode_optional_copy<FAST_String>(buf, m_OptAttribute_state, pmap, globalDictionary.copy_OptAttribute);
    decode_optional_copy<uint32_t >(buf, m_ExerciseStyle_state, pmap, globalDictionary.copy_ExerciseStyle);
    decode_optional_copy<double >(buf, m_OrigStrikePrice_state, pmap, globalDictionary.copy_OrigStrikePrice);
    decode_optional_copy<FAST_String>(buf, m_ContractGenerationNumber_state, pmap, globalDictionary.copy_ContractGenerationNumber);
    decode_optional_default<uint32_t >(buf, m_LowExercisePriceOptionIndicator_state, pmap, globalDictionary.default_LowExercisePriceOptionIndicator);
    decode_mandatory_copy<uint32_t >(buf, m_BlockTradeEligibilityIndicator_state, pmap, globalDictionary.copy_BlockTradeEligibilityIndicator);
    decode_optional_copy<uint32_t >(buf, m_ValuationMethod_state, pmap, globalDictionary.copy_ValuationMethod);
    decode_optional_copy<uint32_t >(buf, m_SettlMethod_state, pmap, globalDictionary.copy_SettlMethod);
    decode_optional_copy<uint32_t >(buf, m_SettlSubMethod_state, pmap, globalDictionary.copy_SettlSubMethod);
    decode_optional_sequence(buf, m_Events, perParseStringRepo, globalDictionary);
  }
  
  inline uint32_t MaturityDate()
  {
    return m_MaturityDate_state;
  }
  
  inline uint32_t MaturityMonthYear()
  {
    return m_MaturityMonthYear_state;
  }
  
  inline FAST_Nullable<double >& StrikePrice()
  {
    return m_StrikePrice_state;
  }
  
  inline FAST_Nullable<uint32_t >& StrikePricePrecision()
  {
    return m_StrikePricePrecision_state;
  }
  
  inline FAST_Nullable<uint32_t >& PricePrecision()
  {
    return m_PricePrecision_state;
  }
  
  inline FAST_Nullable<double >& ContractMultiplier()
  {
    return m_ContractMultiplier_state;
  }
  
  inline FAST_Nullable<uint32_t >& PutOrCall()
  {
    return m_PutOrCall_state.current_value();
  }
  
  inline FAST_Nullable<FAST_String>& OptAttribute()
  {
    return m_OptAttribute_state;
  }
  
  inline FAST_Nullable<uint32_t >& ExerciseStyle()
  {
    return m_ExerciseStyle_state;
  }
  
  inline FAST_Nullable<double >& OrigStrikePrice()
  {
    return m_OrigStrikePrice_state;
  }
  
  inline FAST_Nullable<FAST_String>& ContractGenerationNumber()
  {
    return m_ContractGenerationNumber_state;
  }
  
  inline FAST_Nullable<uint32_t >& LowExercisePriceOptionIndicator()
  {
    return m_LowExercisePriceOptionIndicator_state.current_value();
  }
  
  inline uint32_t BlockTradeEligibilityIndicator()
  {
    return m_BlockTradeEligibilityIndicator_state;
  }
  
  inline FAST_Nullable<uint32_t >& ValuationMethod()
  {
    return m_ValuationMethod_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlMethod()
  {
    return m_SettlMethod_state;
  }
  
  inline FAST_Nullable<uint32_t >& SettlSubMethod()
  {
    return m_SettlSubMethod_state;
  }
  
  inline FAST_Sequence<InstrumentIncremental_SimpleInstrumentDescriptorGroup_Group_Events_Element, 50>& Events()
  {
    return m_Events;
  }
};

class InstrumentIncremental_ComplexInstrumentDescriptorGroup_Group_InstrmtLegGrp_Element
{
private:
  uint32_t m_LegSymbol_state;
  FAST_String m_LegSecurityIDSource_state;
  FAST_Defaultable<uint32_t > m_LegSide_state;
  FAST_Defaultable<uint32_t > m_LegRatioQty_state;
  FAST_Nullable<double > m_LegPrice_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_LegSymbol_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("M", m_LegSecurityIDSource_state);
    m_LegSide_state.is_default = true;
    m_LegSide_state.default_value = 0;
    m_LegRatioQty_state.is_default = true;
    m_LegRatioQty_state.default_value = 1;
    m_LegPrice_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_LegSymbol_state, pmap, globalDictionary.copy_LegSymbol);
    decode_mandatory_default<uint32_t >(buf, m_LegSide_state, pmap, globalDictionary.default_LegSide);
    decode_mandatory_default<uint32_t >(buf, m_LegRatioQty_state, pmap, globalDictionary.default_LegRatioQty);
    decode_optional_copy<double >(buf, m_LegPrice_state, pmap, globalDictionary.copy_LegPrice);
  }
  
  inline uint32_t LegSymbol()
  {
    return m_LegSymbol_state;
  }
  
  inline FAST_String& LegSecurityIDSource()
  {
    return m_LegSecurityIDSource_state;
  }
  
  inline uint32_t LegSide()
  {
    return m_LegSide_state.current_value();
  }
  
  inline uint32_t LegRatioQty()
  {
    return m_LegRatioQty_state.current_value();
  }
  
  inline FAST_Nullable<double >& LegPrice()
  {
    return m_LegPrice_state;
  }
};

class InstrumentIncremental_ComplexInstrumentDescriptorGroup_Group
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_SecurityDesc_state;
  FAST_Sequence<InstrumentIncremental_ComplexInstrumentDescriptorGroup_Group_InstrmtLegGrp_Element, 50> m_InstrmtLegGrp;

public:
  InstrumentIncremental_ComplexInstrumentDescriptorGroup_Group()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_SecurityDesc_state, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_SecurityDesc_state.len = 0;

  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    decode_mandatory_delta<FAST_String>(buf, m_SecurityDesc_state, perParseStringRepo, globalDictionary.delta_SecurityDesc);
    decode_mandatory_sequence(buf, m_InstrmtLegGrp, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& SecurityDesc()
  {
    return m_SecurityDesc_state;
  }
  
  inline FAST_Sequence<InstrumentIncremental_ComplexInstrumentDescriptorGroup_Group_InstrmtLegGrp_Element, 50>& InstrmtLegGrp()
  {
    return m_InstrmtLegGrp;
  }
};

class InstrumentIncremental_MarketSegmentGrp_Element_PriceRangeRules_Element
{
private:
  uint32_t m_PriceRangeRuleID_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_PriceRangeRuleID_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_PriceRangeRuleID_state, pmap, globalDictionary.copy_PriceRangeRuleID);
  }
  
  inline uint32_t PriceRangeRuleID()
  {
    return m_PriceRangeRuleID_state;
  }
};

class InstrumentIncremental_MarketSegmentGrp_Element
{
private:
  uint32_t m_MarketSegmentID_state;
  FAST_Nullable<uint32_t > m_ImpliedMarketIndicator_state;
  FAST_Nullable<uint32_t > m_MultilegModel_state;
  FAST_Sequence<InstrumentIncremental_MarketSegmentGrp_Element_PriceRangeRules_Element, 50> m_PriceRangeRules;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_MarketSegmentID_state = 0;
    m_ImpliedMarketIndicator_state.has_value = false;
    m_MultilegModel_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<uint32_t >(buf, m_MarketSegmentID_state, pmap, globalDictionary.copy_MarketSegmentID);
    decode_optional_copy<uint32_t >(buf, m_ImpliedMarketIndicator_state, pmap, globalDictionary.copy_ImpliedMarketIndicator);
    decode_optional_copy<uint32_t >(buf, m_MultilegModel_state, pmap, globalDictionary.copy_MultilegModel);
    decode_optional_sequence(buf, m_PriceRangeRules, perParseStringRepo, globalDictionary);
  }
  
  inline uint32_t MarketSegmentID()
  {
    return m_MarketSegmentID_state;
  }
  
  inline FAST_Nullable<uint32_t >& ImpliedMarketIndicator()
  {
    return m_ImpliedMarketIndicator_state;
  }
  
  inline FAST_Nullable<uint32_t >& MultilegModel()
  {
    return m_MultilegModel_state;
  }
  
  inline FAST_Sequence<InstrumentIncremental_MarketSegmentGrp_Element_PriceRangeRules_Element, 50>& PriceRangeRules()
  {
    return m_PriceRangeRules;
  }
};

class InstrumentIncremental
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_MsgType_state;
  uint32_t m_MsgSeqNum_state;
  FAST_String m_SecurityUpdateAction_state;
  FAST_String m_SecurityIDSource_state;
  FAST_Sequence<InstrumentIncremental_SecurityAlt_Element, 50> m_SecurityAlt;
  uint32_t m_SecurityType_state;
  FAST_Nullable<uint32_t > m_SecuritySubType_state;
  uint32_t m_ProductComplex_state;
  FAST_Nullable<FAST_String> m_SecurityExchange_state;
  FAST_Group<InstrumentIncremental_SimpleInstrumentDescriptorGroup_Group> m_SimpleInstrumentDescriptorGroup;
  FAST_Group<InstrumentIncremental_ComplexInstrumentDescriptorGroup_Group> m_ComplexInstrumentDescriptorGroup;
  FAST_Nullable<double > m_MinPriceIncrement_state;
  FAST_Nullable<double > m_MinPriceIncrementAmount_state;
  uint32_t m_SecurityStatus_state;
  FAST_Nullable<uint64_t > m_PrevAdjustedOpenInterest_state;
  FAST_Nullable<uint64_t > m_PrevUnadjustedOpenInterest_state;
  FAST_Nullable<double > m_PriorSettlPrice_state;
  FAST_Sequence<InstrumentIncremental_MarketSegmentGrp_Element, 50> m_MarketSegmentGrp;

public:
  InstrumentIncremental()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_SecurityExchange_state.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("BP", m_MsgType_state);
    m_MsgSeqNum_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("A", m_SecurityUpdateAction_state);
    stringRepo.assign_next_const_string_state<FAST_String>("M", m_SecurityIDSource_state);
    m_SecurityType_state = 0;
    m_SecuritySubType_state.has_value = false;
    m_ProductComplex_state = 0;
    m_SecurityExchange_state.has_value = false;

    m_MinPriceIncrement_state.has_value = false;
    m_MinPriceIncrementAmount_state.has_value = false;
    m_SecurityStatus_state = 0;
    m_PrevAdjustedOpenInterest_state.has_value = false;
    m_PrevUnadjustedOpenInterest_state.has_value = false;
    m_PriorSettlPrice_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_mandatory_increment<uint32_t >(buf, m_MsgSeqNum_state, pmap, globalDictionary.increment_MsgSeqNum);
    decode_optional_sequence(buf, m_SecurityAlt, perParseStringRepo, globalDictionary);
    decode_mandatory_copy<uint32_t >(buf, m_SecurityType_state, pmap, globalDictionary.copy_SecurityType);
    decode_optional_copy<uint32_t >(buf, m_SecuritySubType_state, pmap, globalDictionary.copy_SecuritySubType);
    decode_mandatory_copy<uint32_t >(buf, m_ProductComplex_state, pmap, globalDictionary.copy_ProductComplex);
    decode_optional_copy<FAST_String>(buf, m_SecurityExchange_state, pmap, globalDictionary.copy_SecurityExchange);
    decode_optional_group(buf, m_SimpleInstrumentDescriptorGroup, pmap, perParseStringRepo, globalDictionary);
    decode_optional_group(buf, m_ComplexInstrumentDescriptorGroup, pmap, perParseStringRepo, globalDictionary);
    decode_optional_copy<double >(buf, m_MinPriceIncrement_state, pmap, globalDictionary.copy_MinPriceIncrement);
    decode_optional_copy<double >(buf, m_MinPriceIncrementAmount_state, pmap, globalDictionary.copy_MinPriceIncrementAmount);
    decode_mandatory_copy<uint32_t >(buf, m_SecurityStatus_state, pmap, globalDictionary.copy_SecurityStatus);
    decode_optional_copy<uint64_t >(buf, m_PrevAdjustedOpenInterest_state, pmap, globalDictionary.copy_PrevAdjustedOpenInterest);
    decode_optional_copy<uint64_t >(buf, m_PrevUnadjustedOpenInterest_state, pmap, globalDictionary.copy_PrevUnadjustedOpenInterest);
    decode_optional_copy<double >(buf, m_PriorSettlPrice_state, pmap, globalDictionary.copy_PriorSettlPrice);
    decode_mandatory_sequence(buf, m_MarketSegmentGrp, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& MsgType()
  {
    return m_MsgType_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline FAST_String& SecurityUpdateAction()
  {
    return m_SecurityUpdateAction_state;
  }
  
  inline FAST_String& SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Sequence<InstrumentIncremental_SecurityAlt_Element, 50>& SecurityAlt()
  {
    return m_SecurityAlt;
  }
  
  inline uint32_t SecurityType()
  {
    return m_SecurityType_state;
  }
  
  inline FAST_Nullable<uint32_t >& SecuritySubType()
  {
    return m_SecuritySubType_state;
  }
  
  inline uint32_t ProductComplex()
  {
    return m_ProductComplex_state;
  }
  
  inline FAST_Nullable<FAST_String>& SecurityExchange()
  {
    return m_SecurityExchange_state;
  }
  
  inline FAST_Group<InstrumentIncremental_SimpleInstrumentDescriptorGroup_Group>& SimpleInstrumentDescriptorGroup()
  {
    return m_SimpleInstrumentDescriptorGroup;
  }
  
  inline FAST_Group<InstrumentIncremental_ComplexInstrumentDescriptorGroup_Group>& ComplexInstrumentDescriptorGroup()
  {
    return m_ComplexInstrumentDescriptorGroup;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrement()
  {
    return m_MinPriceIncrement_state;
  }
  
  inline FAST_Nullable<double >& MinPriceIncrementAmount()
  {
    return m_MinPriceIncrementAmount_state;
  }
  
  inline uint32_t SecurityStatus()
  {
    return m_SecurityStatus_state;
  }
  
  inline FAST_Nullable<uint64_t >& PrevAdjustedOpenInterest()
  {
    return m_PrevAdjustedOpenInterest_state;
  }
  
  inline FAST_Nullable<uint64_t >& PrevUnadjustedOpenInterest()
  {
    return m_PrevUnadjustedOpenInterest_state;
  }
  
  inline FAST_Nullable<double >& PriorSettlPrice()
  {
    return m_PriorSettlPrice_state;
  }
  
  inline FAST_Sequence<InstrumentIncremental_MarketSegmentGrp_Element, 50>& MarketSegmentGrp()
  {
    return m_MarketSegmentGrp;
  }
};

class VarianceFuturesStatus_ClearingPriceParameters_Element
{
private:
  FAST_Defaultable<uint32_t > m_BusinessDayType_state;
  FAST_Nullable<double > m_ClearingPriceOffset_state;
  FAST_Nullable<uint64_t > m_VegaMultiplier_state;
  FAST_Nullable<uint32_t > m_AnnualTradingBusinessDays_state;
  FAST_Nullable<uint32_t > m_TotalTradingBusinessDays_state;
  FAST_Nullable<uint32_t > m_TradingBusinessDays_state;
  FAST_Nullable<double > m_StandardVariance_state;
  FAST_Nullable<double > m_RelatedClosePrice_state;
  FAST_Nullable<double > m_RealisedVariance_state;
  FAST_Nullable<double > m_InterestRate_state;
  FAST_Nullable<double > m_DiscountFactor_state;
  FAST_Nullable<double > m_OvernightInterestRate_state;
  FAST_Nullable<double > m_ARMVM_state;
  FAST_Nullable<double > m_Volatility_state;
  FAST_Nullable<double > m_SettlPrice_state;
  FAST_Defaultable<FAST_Nullable<uint32_t > > m_CalculationMethod_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_BusinessDayType_state.is_default = true;
    m_BusinessDayType_state.default_value = 0;
    m_ClearingPriceOffset_state.has_value = false;
    m_VegaMultiplier_state.has_value = false;
    m_AnnualTradingBusinessDays_state.has_value = false;
    m_TotalTradingBusinessDays_state.has_value = false;
    m_TradingBusinessDays_state.has_value = false;
    m_StandardVariance_state.has_value = false;
    m_RelatedClosePrice_state.has_value = false;
    m_RealisedVariance_state.has_value = false;
    m_InterestRate_state.has_value = false;
    m_DiscountFactor_state.has_value = false;
    m_OvernightInterestRate_state.has_value = false;
    m_ARMVM_state.has_value = false;
    m_Volatility_state.has_value = false;
    m_SettlPrice_state.has_value = false;
    m_CalculationMethod_state.is_default = true;
    m_CalculationMethod_state.default_value.has_value = true;
    m_CalculationMethod_state.default_value.value = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_default<uint32_t >(buf, m_BusinessDayType_state, pmap, globalDictionary.default_BusinessDayType);
    decode_optional_copy<double >(buf, m_ClearingPriceOffset_state, pmap, globalDictionary.copy_ClearingPriceOffset);
    decode_optional_copy<uint64_t >(buf, m_VegaMultiplier_state, pmap, globalDictionary.copy_VegaMultiplier);
    decode_optional_copy<uint32_t >(buf, m_AnnualTradingBusinessDays_state, pmap, globalDictionary.copy_AnnualTradingBusinessDays);
    decode_optional_copy<uint32_t >(buf, m_TotalTradingBusinessDays_state, pmap, globalDictionary.copy_TotalTradingBusinessDays);
    decode_optional_copy<uint32_t >(buf, m_TradingBusinessDays_state, pmap, globalDictionary.copy_TradingBusinessDays);
    decode_optional_copy<double >(buf, m_StandardVariance_state, pmap, globalDictionary.copy_StandardVariance);
    decode_optional_copy<double >(buf, m_RelatedClosePrice_state, pmap, globalDictionary.copy_RelatedClosePrice);
    decode_optional_copy<double >(buf, m_RealisedVariance_state, pmap, globalDictionary.copy_RealisedVariance);
    decode_optional_copy<double >(buf, m_InterestRate_state, pmap, globalDictionary.copy_InterestRate);
    decode_optional_copy<double >(buf, m_DiscountFactor_state, pmap, globalDictionary.copy_DiscountFactor);
    decode_optional_copy<double >(buf, m_OvernightInterestRate_state, pmap, globalDictionary.copy_OvernightInterestRate);
    decode_optional_copy<double >(buf, m_ARMVM_state, pmap, globalDictionary.copy_ARMVM);
    decode_optional_copy<double >(buf, m_Volatility_state, pmap, globalDictionary.copy_Volatility);
    decode_optional_copy<double >(buf, m_SettlPrice_state, pmap, globalDictionary.copy_SettlPrice);
    decode_optional_default<uint32_t >(buf, m_CalculationMethod_state, pmap, globalDictionary.default_CalculationMethod);
  }
  
  inline uint32_t BusinessDayType()
  {
    return m_BusinessDayType_state.current_value();
  }
  
  inline FAST_Nullable<double >& ClearingPriceOffset()
  {
    return m_ClearingPriceOffset_state;
  }
  
  inline FAST_Nullable<uint64_t >& VegaMultiplier()
  {
    return m_VegaMultiplier_state;
  }
  
  inline FAST_Nullable<uint32_t >& AnnualTradingBusinessDays()
  {
    return m_AnnualTradingBusinessDays_state;
  }
  
  inline FAST_Nullable<uint32_t >& TotalTradingBusinessDays()
  {
    return m_TotalTradingBusinessDays_state;
  }
  
  inline FAST_Nullable<uint32_t >& TradingBusinessDays()
  {
    return m_TradingBusinessDays_state;
  }
  
  inline FAST_Nullable<double >& StandardVariance()
  {
    return m_StandardVariance_state;
  }
  
  inline FAST_Nullable<double >& RelatedClosePrice()
  {
    return m_RelatedClosePrice_state;
  }
  
  inline FAST_Nullable<double >& RealisedVariance()
  {
    return m_RealisedVariance_state;
  }
  
  inline FAST_Nullable<double >& InterestRate()
  {
    return m_InterestRate_state;
  }
  
  inline FAST_Nullable<double >& DiscountFactor()
  {
    return m_DiscountFactor_state;
  }
  
  inline FAST_Nullable<double >& OvernightInterestRate()
  {
    return m_OvernightInterestRate_state;
  }
  
  inline FAST_Nullable<double >& ARMVM()
  {
    return m_ARMVM_state;
  }
  
  inline FAST_Nullable<double >& Volatility()
  {
    return m_Volatility_state;
  }
  
  inline FAST_Nullable<double >& SettlPrice()
  {
    return m_SettlPrice_state;
  }
  
  inline FAST_Nullable<uint32_t >& CalculationMethod()
  {
    return m_CalculationMethod_state.current_value();
  }
};

class VarianceFuturesStatus
{
private:
  FAST_String m_MsgType_state;
  uint32_t m_MsgSeqNum_state;
  uint32_t m_MarketSegmentID_state;
  FAST_String m_SecurityIDSource_state;
  FAST_Sequence<VarianceFuturesStatus_ClearingPriceParameters_Element, 50> m_ClearingPriceParameters;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("f", m_MsgType_state);
    m_MsgSeqNum_state = 0;
    m_MarketSegmentID_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("M", m_SecurityIDSource_state);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_mandatory_increment<uint32_t >(buf, m_MsgSeqNum_state, pmap, globalDictionary.increment_MsgSeqNum);
    decode_mandatory_delta<uint32_t >(buf, m_MarketSegmentID_state, perParseStringRepo, globalDictionary.delta_MarketSegmentID);
    decode_optional_sequence(buf, m_ClearingPriceParameters, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& MsgType()
  {
    return m_MsgType_state;
  }
  
  inline uint32_t MsgSeqNum()
  {
    return m_MsgSeqNum_state;
  }
  
  inline uint32_t MarketSegmentID()
  {
    return m_MarketSegmentID_state;
  }
  
  inline FAST_String& SecurityIDSource()
  {
    return m_SecurityIDSource_state;
  }
  
  inline FAST_Sequence<VarianceFuturesStatus_ClearingPriceParameters_Element, 50>& ClearingPriceParameters()
  {
    return m_ClearingPriceParameters;
  }
};

class MarketDataReport
{
private:
  FAST_String m_MsgType_state;
  FAST_Nullable<uint32_t > m_MDCount_state;
  FAST_Nullable<uint32_t > m_LastMsgSeqNumProcessed_state;
  uint32_t m_MDReportEvent_state;
  FAST_Nullable<uint32_t > m_TotNoMarketSegments_state;
  FAST_Nullable<uint32_t > m_TotNoInstruments_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("U20", m_MsgType_state);
    m_MDCount_state.has_value = false;
    m_LastMsgSeqNumProcessed_state.has_value = false;
    m_MDReportEvent_state = 0;
    m_TotNoMarketSegments_state.has_value = false;
    m_TotNoInstruments_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_optional<uint32_t >(buf, m_MDCount_state.value, m_MDCount_state.has_value);
    decode_optional_delta<uint32_t >(buf, m_LastMsgSeqNumProcessed_state, perParseStringRepo, globalDictionary.delta_LastMsgSeqNumProcessed);
    decode_mandatory_copy<uint32_t >(buf, m_MDReportEvent_state, pmap, globalDictionary.copy_MDReportEvent);
    decode_optional_copy<uint32_t >(buf, m_TotNoMarketSegments_state, pmap, globalDictionary.copy_TotNoMarketSegments);
    decode_optional_copy<uint32_t >(buf, m_TotNoInstruments_state, pmap, globalDictionary.copy_TotNoInstruments);
  }
  
  inline FAST_String& MsgType()
  {
    return m_MsgType_state;
  }
  
  inline FAST_Nullable<uint32_t >& MDCount()
  {
    return m_MDCount_state;
  }
  
  inline FAST_Nullable<uint32_t >& LastMsgSeqNumProcessed()
  {
    return m_LastMsgSeqNumProcessed_state;
  }
  
  inline uint32_t MDReportEvent()
  {
    return m_MDReportEvent_state;
  }
  
  inline FAST_Nullable<uint32_t >& TotNoMarketSegments()
  {
    return m_TotNoMarketSegments_state;
  }
  
  inline FAST_Nullable<uint32_t >& TotNoInstruments()
  {
    return m_TotNoInstruments_state;
  }
};

class Beacon
{
private:
  FAST_String m_MsgType_state;
  uint32_t m_SenderCompID_state;
  uint32_t m_SenderSubID_state;
  uint32_t m_LastMsgSeqNumProcessed_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    stringRepo.assign_next_const_string_state<FAST_String>("0", m_MsgType_state);
    m_SenderCompID_state = 0;
    m_SenderSubID_state = 0;
    m_LastMsgSeqNumProcessed_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Eurex_FAST_Decoder11_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_mandatory_copy<uint32_t >(buf, m_SenderCompID_state, pmap, globalDictionary.copy_SenderCompID);
    decode_mandatory_copy<uint32_t >(buf, m_SenderSubID_state, pmap, globalDictionary.copy_SenderSubID);
    decode_field_mandatory<uint32_t >(buf, m_LastMsgSeqNumProcessed_state);
  }
  
  inline FAST_String& MsgType()
  {
    return m_MsgType_state;
  }
  
  inline uint32_t SenderCompID()
  {
    return m_SenderCompID_state;
  }
  
  inline uint32_t SenderSubID()
  {
    return m_SenderSubID_state;
  }
  
  inline uint32_t LastMsgSeqNumProcessed()
  {
    return m_LastMsgSeqNumProcessed_state;
  }
};


class Eurex_FAST_Decoder11
{
private:
  PerParseStringRepository m_per_parse_string_repo;
  StringStateRepository m_string_repo;
  Eurex_FAST_Decoder11_GlobalDictionary m_global_dictionary;
  RDPacketHeader m_RDPacketHeader;
  ProductSnapshot m_ProductSnapshot;
  InstrumentSnapshot m_InstrumentSnapshot;
  InstrumentIncremental m_InstrumentIncremental;
  VarianceFuturesStatus m_VarianceFuturesStatus;
  MarketDataReport m_MarketDataReport;
  Beacon m_Beacon;

public:
  Eurex_FAST_Decoder11()
  {
    Reset();
  }

  void Reset()
  {
    m_RDPacketHeader.Reset_Dictionary(m_string_repo);
    m_ProductSnapshot.Reset_Dictionary(m_string_repo);
    m_InstrumentSnapshot.Reset_Dictionary(m_string_repo);
    m_InstrumentIncremental.Reset_Dictionary(m_string_repo);
    m_VarianceFuturesStatus.Reset_Dictionary(m_string_repo);
    m_MarketDataReport.Reset_Dictionary(m_string_repo);
    m_Beacon.Reset_Dictionary(m_string_repo);
    m_string_repo.reset();
    m_global_dictionary.reset();
  }

  RDPacketHeader& Parse_RDPacketHeader(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_RDPacketHeader.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_RDPacketHeader;
  }

  ProductSnapshot& Parse_ProductSnapshot(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_ProductSnapshot.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_ProductSnapshot;
  }

  InstrumentSnapshot& Parse_InstrumentSnapshot(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_InstrumentSnapshot.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_InstrumentSnapshot;
  }

  InstrumentIncremental& Parse_InstrumentIncremental(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_InstrumentIncremental.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_InstrumentIncremental;
  }

  VarianceFuturesStatus& Parse_VarianceFuturesStatus(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_VarianceFuturesStatus.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_VarianceFuturesStatus;
  }

  MarketDataReport& Parse_MarketDataReport(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_MarketDataReport.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MarketDataReport;
  }

  Beacon& Parse_Beacon(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_Beacon.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_Beacon;
  }

  uint32_t Get_Template_Id(uint8_t* buf)
  {
    skip_field_mandatory(buf); // get past the pmap
    uint32_t ret;
    decode_field_mandatory<uint32_t>(buf, ret);
    return ret;
  }
};

}
}

