#include <Data/FAST_Decoder.h>


struct Xetra_Fast_Decoder_GlobalDictionary
{
  StringStateRepository m_string_repo;
  uint64_t delta_timeStamp;
  uint32_t copy_srcId;
  uint32_t delta_seqNum;
  uint32_t delta_isix;
  FAST_String copy_streamType;
  uint32_t delta_inetAddr;
  uint32_t copy_port;
  FAST_Nullable<uint32_t > copy_mktDepth;
  FAST_String delta_isin;
  uint32_t delta_setId;
  FAST_String copy_currCode;
  FAST_String copy_instTypCod;
  FAST_Nullable<FAST_String> copy_instSubTypCode;
  uint32_t copy_lglMktSeg;
  FAST_Nullable<FAST_String> copy_mktSegCod;
  FAST_Decimal delta_TickIncrement;
  FAST_Decimal delta_EndTickPriceRange;
  FAST_String copy_trdmdl;
  uint32_t copy_optGwLoc;
  FAST_Decimal delta_referencePrice;
  FAST_Nullable<uint32_t > delta_frstTrdDat;
  FAST_Nullable<uint32_t > delta_lstTrdDat;
  FAST_Nullable<uint32_t > delta_bonMrtyDat;
  double copy_minOrdrSiz;
  double copy_minTrdbUnt;
  double copy_rondLotQty;
  FAST_String copy_tradCal;
  uint32_t copy_untOfQotn;
  uint32_t copy_indicators;
  FAST_Decimal delta_minHiddOrdrVal;
  FAST_Decimal delta_minMidPntOrdrVal;
  FAST_Decimal delta_minIceQty;
  FAST_Decimal delta_minPeakQty;
  FAST_Decimal delta_maxordrvalbest;
  FAST_Decimal delta_vdoMinExec;
  FAST_Nullable<FAST_String> copy_clgloc;
  FAST_String copy_postTrdAty;
  FAST_String copy_stlPeriodFlag;
  FAST_String copy_stlCurrCod;
  FAST_String copy_stlCal;
  FAST_String copy_homeMkt;
  FAST_String copy_refMkt;
  FAST_String copy_reportMarket;
  FAST_Nullable<FAST_String> copy_cmexind;
  uint32_t delta_issueDate;
  FAST_Decimal delta_bonCpnRatWss;
  FAST_Decimal delta_bonCurPoolFact;
  FAST_Nullable<uint32_t > delta_bonCrtCpnDat;
  FAST_Nullable<uint32_t > delta_bonNxtCpnDat;
  FAST_String copy_dnomCurrCod;
  FAST_String copy_bonFlatInd;
  FAST_Nullable<uint32_t > copy_bonAcrIntCalcMd;
  FAST_Nullable<FAST_String> copy_xetraIssrMnem;
  FAST_String copy_warSeg;
  FAST_Defaultable<FAST_String > default_warTyp;
  FAST_String copy_exchangeSegment;
  FAST_String copy_issrMembId;
  FAST_String copy_issrSubGrp;
  FAST_Nullable<FAST_String> copy_specMembId;
  FAST_Nullable<FAST_String> copy_specSubGrp;
  FAST_Nullable<FAST_String> copy_knockOutInd;
  FAST_Decimal delta_maxsrpqty;
  uint64_t delta_entryTime;
  uint32_t delta_consolSeqNum;
  FAST_Decimal delta_entryPrc;
  FAST_Decimal delta_entryQty;
  FAST_Decimal delta_totTrdQty;
  FAST_Nullable<uint32_t > delta_numTrades;
  FAST_Nullable<uint32_t > delta_tranMtchIdNo_opt;
  uint32_t delta_numOrders;
  uint32_t delta_actnCod;
  uint32_t delta_tranMtchIdNo;
  uint32_t increment_seqNum;
  FAST_String copy_priceTypeCod;
  uint32_t copy_entryType;
  FAST_Defaultable<uint32_t > default_actnCod;
  uint32_t copy_state;
  FAST_Nullable<FAST_String> copy_exchId;


  Xetra_Fast_Decoder_GlobalDictionary()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(copy_streamType, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(delta_isin, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_currCode, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_instTypCod, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_instSubTypCode.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_mktSegCod.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_trdmdl, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_tradCal, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_clgloc.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_postTrdAty, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_stlPeriodFlag, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_stlCurrCod, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_stlCal, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_homeMkt, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_refMkt, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_reportMarket, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_cmexind.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_dnomCurrCod, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_bonFlatInd, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_xetraIssrMnem.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_warSeg, 100);
    m_string_repo.assign_next_const_string_state<FAST_String>("C", default_warTyp.default_value);
    m_string_repo.allocate_next_string_state<FAST_String>(default_warTyp.non_default_value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_exchangeSegment, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_issrMembId, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_issrSubGrp, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_specMembId.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_specSubGrp.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_knockOutInd.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_priceTypeCod, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(copy_exchId.value, 100);
  }

  void reset()
  {
    delta_timeStamp = 0;
    copy_srcId = 0;
    delta_seqNum = 0;
    delta_isix = 0;
    copy_streamType.len = 0;

    delta_inetAddr = 0;
    copy_port = 0;
    copy_mktDepth.has_value = false;
    delta_isin.len = 0;

    delta_setId = 0;
    copy_currCode.len = 0;

    copy_instTypCod.len = 0;

    copy_instSubTypCode.has_value = false;

    copy_lglMktSeg = 0;
    copy_mktSegCod.has_value = false;

    delta_TickIncrement.exponent = 0;
    delta_TickIncrement.mantissa = 0;
    delta_EndTickPriceRange.exponent = 0;
    delta_EndTickPriceRange.mantissa = 0;
    copy_trdmdl.len = 0;

    copy_optGwLoc = 0;
    delta_referencePrice.exponent = 0;
    delta_referencePrice.mantissa = 0;
    delta_frstTrdDat.has_value = false;
    delta_lstTrdDat.has_value = false;
    delta_bonMrtyDat.has_value = false;
    copy_minOrdrSiz = 0;
    copy_minTrdbUnt = 0;
    copy_rondLotQty = 0;
    copy_tradCal.len = 0;

    copy_untOfQotn = 0;
    copy_indicators = 0;
    delta_minHiddOrdrVal.exponent = 0;
    delta_minHiddOrdrVal.mantissa = 0;
    delta_minMidPntOrdrVal.exponent = 0;
    delta_minMidPntOrdrVal.mantissa = 0;
    delta_minIceQty.exponent = 0;
    delta_minIceQty.mantissa = 0;
    delta_minPeakQty.exponent = 0;
    delta_minPeakQty.mantissa = 0;
    delta_maxordrvalbest.exponent = 0;
    delta_maxordrvalbest.mantissa = 0;
    delta_vdoMinExec.exponent = 0;
    delta_vdoMinExec.mantissa = 0;
    copy_clgloc.has_value = false;

    copy_postTrdAty.len = 0;

    copy_stlPeriodFlag.len = 0;

    copy_stlCurrCod.len = 0;

    copy_stlCal.len = 0;

    copy_homeMkt.len = 0;

    copy_refMkt.len = 0;

    copy_reportMarket.len = 0;

    copy_cmexind.has_value = false;

    delta_issueDate = 0;
    delta_bonCpnRatWss.exponent = 0;
    delta_bonCpnRatWss.mantissa = 0;
    delta_bonCurPoolFact.exponent = 0;
    delta_bonCurPoolFact.mantissa = 0;
    delta_bonCrtCpnDat.has_value = false;
    delta_bonNxtCpnDat.has_value = false;
    copy_dnomCurrCod.len = 0;

    copy_bonFlatInd.len = 0;

    copy_bonAcrIntCalcMd.has_value = false;
    copy_xetraIssrMnem.has_value = false;

    copy_warSeg.len = 0;

    assign_string(default_warTyp.default_value, "C", 1);

    copy_exchangeSegment.len = 0;

    copy_issrMembId.len = 0;

    copy_issrSubGrp.len = 0;

    copy_specMembId.has_value = false;

    copy_specSubGrp.has_value = false;

    copy_knockOutInd.has_value = false;

    delta_maxsrpqty.exponent = 0;
    delta_maxsrpqty.mantissa = 0;
    delta_entryTime = 0;
    delta_consolSeqNum = 0;
    delta_entryPrc.exponent = 0;
    delta_entryPrc.mantissa = 0;
    delta_entryQty.exponent = 0;
    delta_entryQty.mantissa = 0;
    delta_totTrdQty.exponent = 0;
    delta_totTrdQty.mantissa = 0;
    delta_numTrades.has_value = false;
    delta_tranMtchIdNo_opt.has_value = false;
    delta_numOrders = 0;
    delta_actnCod = 0;
    delta_tranMtchIdNo = 0;
    increment_seqNum = 0;
    copy_priceTypeCod.len = 0;

    copy_entryType = 0;
    default_actnCod.is_default = true;
    default_actnCod.default_value = 4;
    copy_state = 0;
    copy_exchId.has_value = false;

  }
};

class PacketHeader
{
private:
  uint32_t m_senderCompID_state;
  FAST_ByteVector m_packetSeqNum;
  FAST_ByteVector m_sendingTime;
  FAST_ByteVector m_performanceIndicator;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_senderCompID_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_senderCompID_state);
    perParseStringRepo.read_byte_vector_mandatory(buf, m_packetSeqNum);
    perParseStringRepo.read_byte_vector_mandatory(buf, m_sendingTime);
    perParseStringRepo.read_byte_vector_mandatory(buf, m_performanceIndicator);
  }
  
  inline uint32_t senderCompID()
  {
    return m_senderCompID_state;
  }
  
  inline FAST_ByteVector& packetSeqNum()
  {
    return m_packetSeqNum;
  }
  
  inline FAST_ByteVector& sendingTime()
  {
    return m_sendingTime;
  }
  
  inline FAST_ByteVector& performanceIndicator()
  {
    return m_performanceIndicator;
  }
};

class StartService
{
private:
  uint64_t m_timeStamp_state;
  FAST_String m_busDate;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_timeStamp_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint64_t >(buf, m_timeStamp_state);
    perParseStringRepo.read_string_mandatory(buf, m_busDate);
  }
  
  inline uint64_t timeStamp()
  {
    return m_timeStamp_state;
  }
  
  inline FAST_String& busDate()
  {
    return m_busDate;
  }
};

class EndService
{
private:
  uint64_t m_timeStamp_state;
  FAST_String m_busDate;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_timeStamp_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint64_t >(buf, m_timeStamp_state);
    perParseStringRepo.read_string_mandatory(buf, m_busDate);
  }
  
  inline uint64_t timeStamp()
  {
    return m_timeStamp_state;
  }
  
  inline FAST_String& busDate()
  {
    return m_busDate;
  }
};

class StartRefData
{
private:
  uint64_t m_timeStamp_state;
  FAST_String m_busDate;
  uint32_t m_noOfMsg_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_timeStamp_state = 0;
    m_noOfMsg_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint64_t >(buf, m_timeStamp_state);
    perParseStringRepo.read_string_mandatory(buf, m_busDate);
    decode_field_mandatory<uint32_t >(buf, m_noOfMsg_state);
  }
  
  inline uint64_t timeStamp()
  {
    return m_timeStamp_state;
  }
  
  inline FAST_String& busDate()
  {
    return m_busDate;
  }
  
  inline uint32_t noOfMsg()
  {
    return m_noOfMsg_state;
  }
};

class EndRefData
{
private:
  uint64_t m_timeStamp_state;
  FAST_String m_busDate;
  uint32_t m_noOfMsg_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_timeStamp_state = 0;
    m_noOfMsg_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint64_t >(buf, m_timeStamp_state);
    perParseStringRepo.read_string_mandatory(buf, m_busDate);
    decode_field_mandatory<uint32_t >(buf, m_noOfMsg_state);
  }
  
  inline uint64_t timeStamp()
  {
    return m_timeStamp_state;
  }
  
  inline FAST_String& busDate()
  {
    return m_busDate;
  }
  
  inline uint32_t noOfMsg()
  {
    return m_noOfMsg_state;
  }
};

class StartMtnData
{
private:
  uint64_t m_timeStamp_state;
  FAST_String m_busDate;
  uint32_t m_noOfMsg_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_timeStamp_state = 0;
    m_noOfMsg_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint64_t >(buf, m_timeStamp_state);
    perParseStringRepo.read_string_mandatory(buf, m_busDate);
    decode_field_mandatory<uint32_t >(buf, m_noOfMsg_state);
  }
  
  inline uint64_t timeStamp()
  {
    return m_timeStamp_state;
  }
  
  inline FAST_String& busDate()
  {
    return m_busDate;
  }
  
  inline uint32_t noOfMsg()
  {
    return m_noOfMsg_state;
  }
};

class EndMtnData
{
private:
  uint64_t m_timeStamp_state;
  FAST_String m_busDate;
  uint32_t m_noOfMsg_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_timeStamp_state = 0;
    m_noOfMsg_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint64_t >(buf, m_timeStamp_state);
    perParseStringRepo.read_string_mandatory(buf, m_busDate);
    decode_field_mandatory<uint32_t >(buf, m_noOfMsg_state);
  }
  
  inline uint64_t timeStamp()
  {
    return m_timeStamp_state;
  }
  
  inline FAST_String& busDate()
  {
    return m_busDate;
  }
  
  inline uint32_t noOfMsg()
  {
    return m_noOfMsg_state;
  }
};

class BeaconMessage
{
private:
  uint64_t m_timeStamp_state;
  uint32_t m_srcId_state;
  uint32_t m_seqNum_state;
  uint32_t m_isix_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_timeStamp_state = 0;
    m_srcId_state = 0;
    m_seqNum_state = 0;
    m_isix_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_mandatory_delta<uint64_t >(buf, m_timeStamp_state, perParseStringRepo, globalDictionary.delta_timeStamp);
    decode_mandatory_copy<uint32_t >(buf, m_srcId_state, pmap, globalDictionary.copy_srcId);
    decode_mandatory_delta<uint32_t >(buf, m_seqNum_state, perParseStringRepo, globalDictionary.delta_seqNum);
    decode_mandatory_delta<uint32_t >(buf, m_isix_state, perParseStringRepo, globalDictionary.delta_isix);
  }
  
  inline uint64_t timeStamp()
  {
    return m_timeStamp_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline uint32_t seqNum()
  {
    return m_seqNum_state;
  }
  
  inline uint32_t isix()
  {
    return m_isix_state;
  }
};

class InstrumentReferenceData_MDFeedTypes_Element
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_streamService;
  FAST_String m_streamType_state;
  uint32_t m_inetAddr_state;
  uint32_t m_port_state;
  FAST_Nullable<uint32_t > m_mktDepth_state;
  FAST_Nullable<uint32_t > m_mdBookType_state;

public:
  InstrumentReferenceData_MDFeedTypes_Element()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_streamType_state, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_streamType_state.len = 0;

    m_inetAddr_state = 0;
    m_port_state = 0;
    m_mktDepth_state.has_value = false;
    m_mdBookType_state.assign_valid_value(2);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    perParseStringRepo.read_string_mandatory(buf, m_streamService);
    decode_mandatory_copy<FAST_String>(buf, m_streamType_state, pmap, globalDictionary.copy_streamType);
    decode_mandatory_delta<uint32_t >(buf, m_inetAddr_state, perParseStringRepo, globalDictionary.delta_inetAddr);
    decode_mandatory_copy<uint32_t >(buf, m_port_state, pmap, globalDictionary.copy_port);
    decode_optional_copy<uint32_t >(buf, m_mktDepth_state, pmap, globalDictionary.copy_mktDepth);
    m_mdBookType_state.has_value = pmap.read_next_pmap_entry();
  }
  
  inline FAST_String& streamService()
  {
    return m_streamService;
  }
  
  inline FAST_String& streamType()
  {
    return m_streamType_state;
  }
  
  inline uint32_t inetAddr()
  {
    return m_inetAddr_state;
  }
  
  inline uint32_t port()
  {
    return m_port_state;
  }
  
  inline FAST_Nullable<uint32_t >& mktDepth()
  {
    return m_mktDepth_state;
  }
  
  inline FAST_Nullable<uint32_t >& mdBookType()
  {
    return m_mdBookType_state;
  }
};

class InstrumentReferenceData_SecListGroup_Element_TickRules_Element
{
private:
  double m_TickIncrement_state;
  FAST_Nullable<double > m_EndTickPriceRange_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_TickIncrement_state = 0;
    m_EndTickPriceRange_state.value = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_mandatory_decimal_delta(buf, m_TickIncrement_state, perParseStringRepo, globalDictionary.delta_TickIncrement);
    decode_optional_decimal_delta(buf, m_EndTickPriceRange_state, perParseStringRepo, globalDictionary.delta_EndTickPriceRange);
  }
  
  inline double TickIncrement()
  {
    return m_TickIncrement_state;
  }
  
  inline FAST_Nullable<double >& EndTickPriceRange()
  {
    return m_EndTickPriceRange_state;
  }
};

class InstrumentReferenceData_SecListGroup_Element_subscription_Group
{
private:
  uint32_t m_issueDate_state;
  FAST_String m_inSubscription;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_issueDate_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_mandatory_delta<uint32_t >(buf, m_issueDate_state, perParseStringRepo, globalDictionary.delta_issueDate);
    perParseStringRepo.read_string_mandatory(buf, m_inSubscription);
  }
  
  inline uint32_t issueDate()
  {
    return m_issueDate_state;
  }
  
  inline FAST_String& inSubscription()
  {
    return m_inSubscription;
  }
};

class InstrumentReferenceData_SecListGroup_Element_bond_Group
{
private:
  StringStateRepository m_string_repo;
  FAST_Nullable<double > m_bonCpnRatWss_state;
  FAST_Nullable<double > m_bonCurPoolFact_state;
  FAST_Nullable<FAST_String> m_bonYldTrdInd_state;
  FAST_Nullable<uint32_t > m_bonCrtCpnDat_state;
  FAST_Nullable<uint32_t > m_bonNxtCpnDat_state;
  FAST_String m_dnomCurrCod_state;
  FAST_String m_bonFlatInd_state;
  FAST_Nullable<uint32_t > m_bonAcrIntCalcMd_state;
  FAST_Nullable<FAST_String> m_xetraIssrMnem_state;

public:
  InstrumentReferenceData_SecListGroup_Element_bond_Group()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_dnomCurrCod_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_bonFlatInd_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_xetraIssrMnem_state.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_bonCpnRatWss_state.value = 0;
    m_bonCurPoolFact_state.value = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("Y", m_bonYldTrdInd_state.value);
    m_bonCrtCpnDat_state.has_value = false;
    m_bonNxtCpnDat_state.has_value = false;
    m_dnomCurrCod_state.len = 0;

    m_bonFlatInd_state.len = 0;

    m_bonAcrIntCalcMd_state.has_value = false;
    m_xetraIssrMnem_state.has_value = false;

  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_optional_decimal_delta(buf, m_bonCpnRatWss_state, perParseStringRepo, globalDictionary.delta_bonCpnRatWss);
    decode_optional_decimal_delta(buf, m_bonCurPoolFact_state, perParseStringRepo, globalDictionary.delta_bonCurPoolFact);
    m_bonYldTrdInd_state.has_value = pmap.read_next_pmap_entry();
    decode_optional_delta<uint32_t >(buf, m_bonCrtCpnDat_state, perParseStringRepo, globalDictionary.delta_bonCrtCpnDat);
    decode_optional_delta<uint32_t >(buf, m_bonNxtCpnDat_state, perParseStringRepo, globalDictionary.delta_bonNxtCpnDat);
    decode_mandatory_copy<FAST_String>(buf, m_dnomCurrCod_state, pmap, globalDictionary.copy_dnomCurrCod);
    decode_mandatory_copy<FAST_String>(buf, m_bonFlatInd_state, pmap, globalDictionary.copy_bonFlatInd);
    decode_optional_copy<uint32_t >(buf, m_bonAcrIntCalcMd_state, pmap, globalDictionary.copy_bonAcrIntCalcMd);
    decode_optional_copy<FAST_String>(buf, m_xetraIssrMnem_state, pmap, globalDictionary.copy_xetraIssrMnem);
  }
  
  inline FAST_Nullable<double >& bonCpnRatWss()
  {
    return m_bonCpnRatWss_state;
  }
  
  inline FAST_Nullable<double >& bonCurPoolFact()
  {
    return m_bonCurPoolFact_state;
  }
  
  inline FAST_Nullable<FAST_String>& bonYldTrdInd()
  {
    return m_bonYldTrdInd_state;
  }
  
  inline FAST_Nullable<uint32_t >& bonCrtCpnDat()
  {
    return m_bonCrtCpnDat_state;
  }
  
  inline FAST_Nullable<uint32_t >& bonNxtCpnDat()
  {
    return m_bonNxtCpnDat_state;
  }
  
  inline FAST_String& dnomCurrCod()
  {
    return m_dnomCurrCod_state;
  }
  
  inline FAST_String& bonFlatInd()
  {
    return m_bonFlatInd_state;
  }
  
  inline FAST_Nullable<uint32_t >& bonAcrIntCalcMd()
  {
    return m_bonAcrIntCalcMd_state;
  }
  
  inline FAST_Nullable<FAST_String>& xetraIssrMnem()
  {
    return m_xetraIssrMnem_state;
  }
};

class InstrumentReferenceData_SecListGroup_Element_warrant_Group
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_warSeg_state;
  FAST_Nullable<FAST_String> m_warStrPrc;
  FAST_Defaultable<FAST_String > m_warTyp_state;
  FAST_Nullable<FAST_String> m_warUnd;

public:
  InstrumentReferenceData_SecListGroup_Element_warrant_Group()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_warSeg_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_warTyp_state.default_value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_warSeg_state.len = 0;

    assign_string(m_warTyp_state.default_value, "C", 1);

  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<FAST_String>(buf, m_warSeg_state, pmap, globalDictionary.copy_warSeg);
    perParseStringRepo.read_string_optional(buf, m_warStrPrc);
    decode_mandatory_default<FAST_String>(buf, m_warTyp_state, pmap, globalDictionary.default_warTyp);
    perParseStringRepo.read_string_optional(buf, m_warUnd);
  }
  
  inline FAST_String& warSeg()
  {
    return m_warSeg_state;
  }
  
  inline FAST_Nullable<FAST_String>& warStrPrc()
  {
    return m_warStrPrc;
  }
  
  inline FAST_String& warTyp()
  {
    return m_warTyp_state.current_value();
  }
  
  inline FAST_Nullable<FAST_String>& warUnd()
  {
    return m_warUnd;
  }
};

class InstrumentReferenceData_SecListGroup_Element_continuousauction_Group
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_exchangeSegment_state;
  FAST_String m_issrMembId_state;
  FAST_String m_issrSubGrp_state;
  FAST_Nullable<FAST_String> m_specMembId_state;
  FAST_Nullable<FAST_String> m_specSubGrp_state;
  FAST_Nullable<FAST_String> m_knockOutInd_state;
  FAST_Nullable<FAST_String> m_singleAuctionIndicator_state;
  FAST_Nullable<FAST_String> m_specialAuctionIndicator_state;

public:
  InstrumentReferenceData_SecListGroup_Element_continuousauction_Group()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_exchangeSegment_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_issrMembId_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_issrSubGrp_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_specMembId_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_specSubGrp_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_knockOutInd_state.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_exchangeSegment_state.len = 0;

    m_issrMembId_state.len = 0;

    m_issrSubGrp_state.len = 0;

    m_specMembId_state.has_value = false;

    m_specSubGrp_state.has_value = false;

    m_knockOutInd_state.has_value = false;

    stringRepo.assign_next_const_string_state<FAST_String>("Y", m_singleAuctionIndicator_state.value);
    stringRepo.assign_next_const_string_state<FAST_String>("Y", m_specialAuctionIndicator_state.value);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_copy<FAST_String>(buf, m_exchangeSegment_state, pmap, globalDictionary.copy_exchangeSegment);
    decode_mandatory_copy<FAST_String>(buf, m_issrMembId_state, pmap, globalDictionary.copy_issrMembId);
    decode_mandatory_copy<FAST_String>(buf, m_issrSubGrp_state, pmap, globalDictionary.copy_issrSubGrp);
    decode_optional_copy<FAST_String>(buf, m_specMembId_state, pmap, globalDictionary.copy_specMembId);
    decode_optional_copy<FAST_String>(buf, m_specSubGrp_state, pmap, globalDictionary.copy_specSubGrp);
    decode_optional_copy<FAST_String>(buf, m_knockOutInd_state, pmap, globalDictionary.copy_knockOutInd);
    m_singleAuctionIndicator_state.has_value = pmap.read_next_pmap_entry();
    m_specialAuctionIndicator_state.has_value = pmap.read_next_pmap_entry();
  }
  
  inline FAST_String& exchangeSegment()
  {
    return m_exchangeSegment_state;
  }
  
  inline FAST_String& issrMembId()
  {
    return m_issrMembId_state;
  }
  
  inline FAST_String& issrSubGrp()
  {
    return m_issrSubGrp_state;
  }
  
  inline FAST_Nullable<FAST_String>& specMembId()
  {
    return m_specMembId_state;
  }
  
  inline FAST_Nullable<FAST_String>& specSubGrp()
  {
    return m_specSubGrp_state;
  }
  
  inline FAST_Nullable<FAST_String>& knockOutInd()
  {
    return m_knockOutInd_state;
  }
  
  inline FAST_Nullable<FAST_String>& singleAuctionIndicator()
  {
    return m_singleAuctionIndicator_state;
  }
  
  inline FAST_Nullable<FAST_String>& specialAuctionIndicator()
  {
    return m_specialAuctionIndicator_state;
  }
};

class InstrumentReferenceData_SecListGroup_Element_marketmaker_Group_license_Element
{
private:
  FAST_String m_licensetype;
  FAST_String m_sprdtypecode;
  FAST_Nullable<double > m_sprdFact_state;
  double m_sprdminqty_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_sprdFact_state.has_value = false;
    m_sprdminqty_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_licensetype);
    perParseStringRepo.read_string_mandatory(buf, m_sprdtypecode);
    decode_field_optional<double >(buf, m_sprdFact_state.value, m_sprdFact_state.has_value);
    decode_field_mandatory<double >(buf, m_sprdminqty_state);
  }
  
  inline FAST_String& licensetype()
  {
    return m_licensetype;
  }
  
  inline FAST_String& sprdtypecode()
  {
    return m_sprdtypecode;
  }
  
  inline FAST_Nullable<double >& sprdFact()
  {
    return m_sprdFact_state;
  }
  
  inline double sprdminqty()
  {
    return m_sprdminqty_state;
  }
};

class InstrumentReferenceData_SecListGroup_Element_marketmaker_Group
{
private:
  FAST_Sequence<InstrumentReferenceData_SecListGroup_Element_marketmaker_Group_license_Element, 50> m_license;
  double m_maxsrpqty_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_maxsrpqty_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_mandatory_sequence(buf, m_license, perParseStringRepo, globalDictionary);
    decode_mandatory_decimal_delta(buf, m_maxsrpqty_state, perParseStringRepo, globalDictionary.delta_maxsrpqty);
  }
  
  inline FAST_Sequence<InstrumentReferenceData_SecListGroup_Element_marketmaker_Group_license_Element, 50>& license()
  {
    return m_license;
  }
  
  inline double maxsrpqty()
  {
    return m_maxsrpqty_state;
  }
};

class InstrumentReferenceData_SecListGroup_Element_basisinstrument_Group
{
private:
  FAST_Nullable<FAST_String> m_basund;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_optional(buf, m_basund);
  }
  
  inline FAST_Nullable<FAST_String>& basund()
  {
    return m_basund;
  }
};

class InstrumentReferenceData_SecListGroup_Element
{
private:
  StringStateRepository m_string_repo;
  uint32_t m_isix_state;
  FAST_String m_isin_state;
  FAST_Nullable<FAST_String> m_instMnem;
  FAST_Nullable<FAST_String> m_instShtNam;
  FAST_Nullable<FAST_String> m_wknNo;
  uint32_t m_setId_state;
  FAST_String m_currCode_state;
  FAST_String m_instTypCod_state;
  FAST_Nullable<FAST_String> m_instSubTypCode_state;
  uint32_t m_lglMktSeg_state;
  FAST_Nullable<FAST_String> m_mktSegCod_state;
  FAST_Sequence<InstrumentReferenceData_SecListGroup_Element_TickRules_Element, 50> m_TickRules;
  FAST_String m_trdmdl_state;
  uint32_t m_optGwLoc_state;
  double m_referencePrice_state;
  FAST_Nullable<uint32_t > m_frstTrdDat_state;
  FAST_Nullable<uint32_t > m_lstTrdDat_state;
  FAST_Nullable<uint32_t > m_bonMrtyDat_state;
  double m_minOrdrSiz_state;
  double m_minTrdbUnt_state;
  double m_rondLotQty_state;
  FAST_String m_tradCal_state;
  uint32_t m_untOfQotn_state;
  uint32_t m_indicators_state;
  FAST_Nullable<double > m_minHiddOrdrVal_state;
  FAST_Nullable<double > m_minMidPntOrdrVal_state;
  FAST_Nullable<double > m_minIceQty_state;
  FAST_Nullable<double > m_minPeakQty_state;
  FAST_Nullable<double > m_maxordrvalbest_state;
  FAST_Nullable<double > m_vdoMinExec_state;
  FAST_Nullable<FAST_String> m_clgloc_state;
  FAST_String m_postTrdAty_state;
  FAST_String m_stlPeriodFlag_state;
  FAST_String m_stlCurrCod_state;
  FAST_String m_stlCal_state;
  FAST_String m_homeMkt_state;
  FAST_String m_refMkt_state;
  FAST_String m_reportMarket_state;
  FAST_Nullable<FAST_String> m_cmexind_state;
  FAST_Group<InstrumentReferenceData_SecListGroup_Element_subscription_Group> m_subscription;
  FAST_Group<InstrumentReferenceData_SecListGroup_Element_bond_Group> m_bond;
  FAST_Group<InstrumentReferenceData_SecListGroup_Element_warrant_Group> m_warrant;
  FAST_Group<InstrumentReferenceData_SecListGroup_Element_continuousauction_Group> m_continuousauction;
  FAST_Group<InstrumentReferenceData_SecListGroup_Element_marketmaker_Group> m_marketmaker;
  FAST_Group<InstrumentReferenceData_SecListGroup_Element_basisinstrument_Group> m_basisinstrument;

public:
  InstrumentReferenceData_SecListGroup_Element()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_isin_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_currCode_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_instTypCod_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_instSubTypCode_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_mktSegCod_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_trdmdl_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_tradCal_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_clgloc_state.value, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_postTrdAty_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_stlPeriodFlag_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_stlCurrCod_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_stlCal_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_homeMkt_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_refMkt_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_reportMarket_state, 100);
    m_string_repo.allocate_next_string_state<FAST_String>(m_cmexind_state.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_isix_state = 0;
    m_isin_state.len = 0;

    m_setId_state = 0;
    m_currCode_state.len = 0;

    m_instTypCod_state.len = 0;

    m_instSubTypCode_state.has_value = false;

    m_lglMktSeg_state = 0;
    m_mktSegCod_state.has_value = false;

    m_trdmdl_state.len = 0;

    m_optGwLoc_state = 0;
    m_referencePrice_state = 0;
    m_frstTrdDat_state.has_value = false;
    m_lstTrdDat_state.has_value = false;
    m_bonMrtyDat_state.has_value = false;
    m_minOrdrSiz_state = 0;
    m_minTrdbUnt_state = 0;
    m_rondLotQty_state = 0;
    m_tradCal_state.len = 0;

    m_untOfQotn_state = 0;
    m_indicators_state = 0;
    m_minHiddOrdrVal_state.value = 0;
    m_minMidPntOrdrVal_state.value = 0;
    m_minIceQty_state.value = 0;
    m_minPeakQty_state.value = 0;
    m_maxordrvalbest_state.value = 0;
    m_vdoMinExec_state.value = 0;
    m_clgloc_state.has_value = false;

    m_postTrdAty_state.len = 0;

    m_stlPeriodFlag_state.len = 0;

    m_stlCurrCod_state.len = 0;

    m_stlCal_state.len = 0;

    m_homeMkt_state.len = 0;

    m_refMkt_state.len = 0;

    m_reportMarket_state.len = 0;

    m_cmexind_state.has_value = false;

  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    decode_mandatory_delta<uint32_t >(buf, m_isix_state, perParseStringRepo, globalDictionary.delta_isix);
    decode_mandatory_delta<FAST_String>(buf, m_isin_state, perParseStringRepo, globalDictionary.delta_isin);
    perParseStringRepo.read_string_optional(buf, m_instMnem);
    perParseStringRepo.read_string_optional(buf, m_instShtNam);
    perParseStringRepo.read_string_optional(buf, m_wknNo);
    decode_mandatory_delta<uint32_t >(buf, m_setId_state, perParseStringRepo, globalDictionary.delta_setId);
    decode_mandatory_copy<FAST_String>(buf, m_currCode_state, pmap, globalDictionary.copy_currCode);
    decode_mandatory_copy<FAST_String>(buf, m_instTypCod_state, pmap, globalDictionary.copy_instTypCod);
    decode_optional_copy<FAST_String>(buf, m_instSubTypCode_state, pmap, globalDictionary.copy_instSubTypCode);
    decode_mandatory_copy<uint32_t >(buf, m_lglMktSeg_state, pmap, globalDictionary.copy_lglMktSeg);
    decode_optional_copy<FAST_String>(buf, m_mktSegCod_state, pmap, globalDictionary.copy_mktSegCod);
    decode_mandatory_sequence(buf, m_TickRules, perParseStringRepo, globalDictionary);
    decode_mandatory_copy<FAST_String>(buf, m_trdmdl_state, pmap, globalDictionary.copy_trdmdl);
    decode_mandatory_copy<uint32_t >(buf, m_optGwLoc_state, pmap, globalDictionary.copy_optGwLoc);
    decode_mandatory_decimal_delta(buf, m_referencePrice_state, perParseStringRepo, globalDictionary.delta_referencePrice);
    decode_optional_delta<uint32_t >(buf, m_frstTrdDat_state, perParseStringRepo, globalDictionary.delta_frstTrdDat);
    decode_optional_delta<uint32_t >(buf, m_lstTrdDat_state, perParseStringRepo, globalDictionary.delta_lstTrdDat);
    decode_optional_delta<uint32_t >(buf, m_bonMrtyDat_state, perParseStringRepo, globalDictionary.delta_bonMrtyDat);
    decode_mandatory_copy<double >(buf, m_minOrdrSiz_state, pmap, globalDictionary.copy_minOrdrSiz);
    decode_mandatory_copy<double >(buf, m_minTrdbUnt_state, pmap, globalDictionary.copy_minTrdbUnt);
    decode_mandatory_copy<double >(buf, m_rondLotQty_state, pmap, globalDictionary.copy_rondLotQty);
    decode_mandatory_copy<FAST_String>(buf, m_tradCal_state, pmap, globalDictionary.copy_tradCal);
    decode_mandatory_copy<uint32_t >(buf, m_untOfQotn_state, pmap, globalDictionary.copy_untOfQotn);
    decode_mandatory_copy<uint32_t >(buf, m_indicators_state, pmap, globalDictionary.copy_indicators);
    decode_optional_decimal_delta(buf, m_minHiddOrdrVal_state, perParseStringRepo, globalDictionary.delta_minHiddOrdrVal);
    decode_optional_decimal_delta(buf, m_minMidPntOrdrVal_state, perParseStringRepo, globalDictionary.delta_minMidPntOrdrVal);
    decode_optional_decimal_delta(buf, m_minIceQty_state, perParseStringRepo, globalDictionary.delta_minIceQty);
    decode_optional_decimal_delta(buf, m_minPeakQty_state, perParseStringRepo, globalDictionary.delta_minPeakQty);
    decode_optional_decimal_delta(buf, m_maxordrvalbest_state, perParseStringRepo, globalDictionary.delta_maxordrvalbest);
    decode_optional_decimal_delta(buf, m_vdoMinExec_state, perParseStringRepo, globalDictionary.delta_vdoMinExec);
    decode_optional_copy<FAST_String>(buf, m_clgloc_state, pmap, globalDictionary.copy_clgloc);
    decode_mandatory_copy<FAST_String>(buf, m_postTrdAty_state, pmap, globalDictionary.copy_postTrdAty);
    decode_mandatory_copy<FAST_String>(buf, m_stlPeriodFlag_state, pmap, globalDictionary.copy_stlPeriodFlag);
    decode_mandatory_copy<FAST_String>(buf, m_stlCurrCod_state, pmap, globalDictionary.copy_stlCurrCod);
    decode_mandatory_copy<FAST_String>(buf, m_stlCal_state, pmap, globalDictionary.copy_stlCal);
    decode_mandatory_copy<FAST_String>(buf, m_homeMkt_state, pmap, globalDictionary.copy_homeMkt);
    decode_mandatory_copy<FAST_String>(buf, m_refMkt_state, pmap, globalDictionary.copy_refMkt);
    decode_mandatory_copy<FAST_String>(buf, m_reportMarket_state, pmap, globalDictionary.copy_reportMarket);
    decode_optional_copy<FAST_String>(buf, m_cmexind_state, pmap, globalDictionary.copy_cmexind);
    decode_optional_group(buf, m_subscription, pmap, perParseStringRepo, globalDictionary);
    decode_optional_group(buf, m_bond, pmap, perParseStringRepo, globalDictionary);
    decode_optional_group(buf, m_warrant, pmap, perParseStringRepo, globalDictionary);
    decode_optional_group(buf, m_continuousauction, pmap, perParseStringRepo, globalDictionary);
    decode_optional_group(buf, m_marketmaker, pmap, perParseStringRepo, globalDictionary);
    decode_optional_group(buf, m_basisinstrument, pmap, perParseStringRepo, globalDictionary);
  }
  
  inline uint32_t isix()
  {
    return m_isix_state;
  }
  
  inline FAST_String& isin()
  {
    return m_isin_state;
  }
  
  inline FAST_Nullable<FAST_String>& instMnem()
  {
    return m_instMnem;
  }
  
  inline FAST_Nullable<FAST_String>& instShtNam()
  {
    return m_instShtNam;
  }
  
  inline FAST_Nullable<FAST_String>& wknNo()
  {
    return m_wknNo;
  }
  
  inline uint32_t setId()
  {
    return m_setId_state;
  }
  
  inline FAST_String& currCode()
  {
    return m_currCode_state;
  }
  
  inline FAST_String& instTypCod()
  {
    return m_instTypCod_state;
  }
  
  inline FAST_Nullable<FAST_String>& instSubTypCode()
  {
    return m_instSubTypCode_state;
  }
  
  inline uint32_t lglMktSeg()
  {
    return m_lglMktSeg_state;
  }
  
  inline FAST_Nullable<FAST_String>& mktSegCod()
  {
    return m_mktSegCod_state;
  }
  
  inline FAST_Sequence<InstrumentReferenceData_SecListGroup_Element_TickRules_Element, 50>& TickRules()
  {
    return m_TickRules;
  }
  
  inline FAST_String& trdmdl()
  {
    return m_trdmdl_state;
  }
  
  inline uint32_t optGwLoc()
  {
    return m_optGwLoc_state;
  }
  
  inline double referencePrice()
  {
    return m_referencePrice_state;
  }
  
  inline FAST_Nullable<uint32_t >& frstTrdDat()
  {
    return m_frstTrdDat_state;
  }
  
  inline FAST_Nullable<uint32_t >& lstTrdDat()
  {
    return m_lstTrdDat_state;
  }
  
  inline FAST_Nullable<uint32_t >& bonMrtyDat()
  {
    return m_bonMrtyDat_state;
  }
  
  inline double minOrdrSiz()
  {
    return m_minOrdrSiz_state;
  }
  
  inline double minTrdbUnt()
  {
    return m_minTrdbUnt_state;
  }
  
  inline double rondLotQty()
  {
    return m_rondLotQty_state;
  }
  
  inline FAST_String& tradCal()
  {
    return m_tradCal_state;
  }
  
  inline uint32_t untOfQotn()
  {
    return m_untOfQotn_state;
  }
  
  inline uint32_t indicators()
  {
    return m_indicators_state;
  }
  
  inline FAST_Nullable<double >& minHiddOrdrVal()
  {
    return m_minHiddOrdrVal_state;
  }
  
  inline FAST_Nullable<double >& minMidPntOrdrVal()
  {
    return m_minMidPntOrdrVal_state;
  }
  
  inline FAST_Nullable<double >& minIceQty()
  {
    return m_minIceQty_state;
  }
  
  inline FAST_Nullable<double >& minPeakQty()
  {
    return m_minPeakQty_state;
  }
  
  inline FAST_Nullable<double >& maxordrvalbest()
  {
    return m_maxordrvalbest_state;
  }
  
  inline FAST_Nullable<double >& vdoMinExec()
  {
    return m_vdoMinExec_state;
  }
  
  inline FAST_Nullable<FAST_String>& clgloc()
  {
    return m_clgloc_state;
  }
  
  inline FAST_String& postTrdAty()
  {
    return m_postTrdAty_state;
  }
  
  inline FAST_String& stlPeriodFlag()
  {
    return m_stlPeriodFlag_state;
  }
  
  inline FAST_String& stlCurrCod()
  {
    return m_stlCurrCod_state;
  }
  
  inline FAST_String& stlCal()
  {
    return m_stlCal_state;
  }
  
  inline FAST_String& homeMkt()
  {
    return m_homeMkt_state;
  }
  
  inline FAST_String& refMkt()
  {
    return m_refMkt_state;
  }
  
  inline FAST_String& reportMarket()
  {
    return m_reportMarket_state;
  }
  
  inline FAST_Nullable<FAST_String>& cmexind()
  {
    return m_cmexind_state;
  }
  
  inline FAST_Group<InstrumentReferenceData_SecListGroup_Element_subscription_Group>& subscription()
  {
    return m_subscription;
  }
  
  inline FAST_Group<InstrumentReferenceData_SecListGroup_Element_bond_Group>& bond()
  {
    return m_bond;
  }
  
  inline FAST_Group<InstrumentReferenceData_SecListGroup_Element_warrant_Group>& warrant()
  {
    return m_warrant;
  }
  
  inline FAST_Group<InstrumentReferenceData_SecListGroup_Element_continuousauction_Group>& continuousauction()
  {
    return m_continuousauction;
  }
  
  inline FAST_Group<InstrumentReferenceData_SecListGroup_Element_marketmaker_Group>& marketmaker()
  {
    return m_marketmaker;
  }
  
  inline FAST_Group<InstrumentReferenceData_SecListGroup_Element_basisinstrument_Group>& basisinstrument()
  {
    return m_basisinstrument;
  }
};

class InstrumentReferenceData
{
private:
  uint32_t m_seqNum_state;
  uint32_t m_srcId_state;
  FAST_String m_exchId;
  FAST_String m_instGrp;
  FAST_Sequence<InstrumentReferenceData_MDFeedTypes_Element, 50> m_MDFeedTypes;
  FAST_Sequence<InstrumentReferenceData_SecListGroup_Element, 50> m_SecListGroup;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_seqNum_state = 0;
    m_srcId_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_seqNum_state);
    decode_field_mandatory<uint32_t >(buf, m_srcId_state);
    perParseStringRepo.read_string_mandatory(buf, m_exchId);
    perParseStringRepo.read_string_mandatory(buf, m_instGrp);
    decode_mandatory_sequence(buf, m_MDFeedTypes, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_SecListGroup, perParseStringRepo, globalDictionary);
  }
  
  inline uint32_t seqNum()
  {
    return m_seqNum_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline FAST_String& exchId()
  {
    return m_exchId;
  }
  
  inline FAST_String& instGrp()
  {
    return m_instGrp;
  }
  
  inline FAST_Sequence<InstrumentReferenceData_MDFeedTypes_Element, 50>& MDFeedTypes()
  {
    return m_MDFeedTypes;
  }
  
  inline FAST_Sequence<InstrumentReferenceData_SecListGroup_Element, 50>& SecListGroup()
  {
    return m_SecListGroup;
  }
};

class MaintenanceReferenceData_Exchanges_Element_exchangeSegments_Element
{
private:
  FAST_String m_exchangeSegment;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_exchangeSegment);
  }
  
  inline FAST_String& exchangeSegment()
  {
    return m_exchangeSegment;
  }
};

class MaintenanceReferenceData_Exchanges_Element
{
private:
  FAST_String m_exchId;
  FAST_Sequence<MaintenanceReferenceData_Exchanges_Element_exchangeSegments_Element, 50> m_exchangeSegments;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_mandatory(buf, m_exchId);
    decode_optional_sequence(buf, m_exchangeSegments, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_String& exchId()
  {
    return m_exchId;
  }
  
  inline FAST_Sequence<MaintenanceReferenceData_Exchanges_Element_exchangeSegments_Element, 50>& exchangeSegments()
  {
    return m_exchangeSegments;
  }
};

class MaintenanceReferenceData_MDFeedTypes_Element
{
private:
  StringStateRepository m_string_repo;
  FAST_String m_streamService;
  FAST_String m_streamType_state;
  uint32_t m_inetAddr_state;
  uint32_t m_port_state;
  FAST_Nullable<uint32_t > m_mktDepth_state;
  FAST_Nullable<uint32_t > m_mdBookType_state;

public:
  MaintenanceReferenceData_MDFeedTypes_Element()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_streamType_state, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_streamType_state.len = 0;

    m_inetAddr_state = 0;
    m_port_state = 0;
    m_mktDepth_state.has_value = false;
    m_mdBookType_state.assign_valid_value(2);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    perParseStringRepo.read_string_mandatory(buf, m_streamService);
    decode_mandatory_copy<FAST_String>(buf, m_streamType_state, pmap, globalDictionary.copy_streamType);
    decode_mandatory_delta<uint32_t >(buf, m_inetAddr_state, perParseStringRepo, globalDictionary.delta_inetAddr);
    decode_mandatory_copy<uint32_t >(buf, m_port_state, pmap, globalDictionary.copy_port);
    decode_optional_copy<uint32_t >(buf, m_mktDepth_state, pmap, globalDictionary.copy_mktDepth);
    m_mdBookType_state.has_value = pmap.read_next_pmap_entry();
  }
  
  inline FAST_String& streamService()
  {
    return m_streamService;
  }
  
  inline FAST_String& streamType()
  {
    return m_streamType_state;
  }
  
  inline uint32_t inetAddr()
  {
    return m_inetAddr_state;
  }
  
  inline uint32_t port()
  {
    return m_port_state;
  }
  
  inline FAST_Nullable<uint32_t >& mktDepth()
  {
    return m_mktDepth_state;
  }
  
  inline FAST_Nullable<uint32_t >& mdBookType()
  {
    return m_mdBookType_state;
  }
};

class MaintenanceReferenceData
{
private:
  uint32_t m_seqNum_state;
  uint32_t m_srcId_state;
  FAST_Sequence<MaintenanceReferenceData_Exchanges_Element, 50> m_Exchanges;
  FAST_Sequence<MaintenanceReferenceData_MDFeedTypes_Element, 50> m_MDFeedTypes;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_seqNum_state = 0;
    m_srcId_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_field_mandatory<uint32_t >(buf, m_seqNum_state);
    decode_field_mandatory<uint32_t >(buf, m_srcId_state);
    decode_mandatory_sequence(buf, m_Exchanges, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_MDFeedTypes, perParseStringRepo, globalDictionary);
  }
  
  inline uint32_t seqNum()
  {
    return m_seqNum_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline FAST_Sequence<MaintenanceReferenceData_Exchanges_Element, 50>& Exchanges()
  {
    return m_Exchanges;
  }
  
  inline FAST_Sequence<MaintenanceReferenceData_MDFeedTypes_Element, 50>& MDFeedTypes()
  {
    return m_MDFeedTypes;
  }
};

class InsideMarketSnapshotInformation_EntriesAtp_Element
{
private:
  uint32_t m_entryType_state;
  double m_entryPrc_state;
  double m_entryQty_state;
  FAST_Nullable<double > m_totTrdQty_state;
  uint64_t m_entryTime_state;
  FAST_Nullable<uint32_t > m_numTrades_state;
  FAST_Nullable<uint32_t > m_tranMtchIdNo_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryType_state = 0;
    m_entryPrc_state = 0;
    m_entryQty_state = 0;
    m_totTrdQty_state.value = 0;
    m_entryTime_state = 0;
    m_numTrades_state.has_value = false;
    m_tranMtchIdNo_state.has_value = false;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_entryType_state);
    decode_mandatory_decimal_delta(buf, m_entryPrc_state, perParseStringRepo, globalDictionary.delta_entryPrc);
    decode_mandatory_decimal_delta(buf, m_entryQty_state, perParseStringRepo, globalDictionary.delta_entryQty);
    decode_optional_decimal_delta(buf, m_totTrdQty_state, perParseStringRepo, globalDictionary.delta_totTrdQty);
    decode_mandatory_delta<uint64_t >(buf, m_entryTime_state, perParseStringRepo, globalDictionary.delta_entryTime);
    decode_optional_delta<uint32_t >(buf, m_numTrades_state, perParseStringRepo, globalDictionary.delta_numTrades);
    decode_optional_delta<uint32_t >(buf, m_tranMtchIdNo_state, perParseStringRepo, globalDictionary.delta_tranMtchIdNo_opt);
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline double entryPrc()
  {
    return m_entryPrc_state;
  }
  
  inline double entryQty()
  {
    return m_entryQty_state;
  }
  
  inline FAST_Nullable<double >& totTrdQty()
  {
    return m_totTrdQty_state;
  }
  
  inline uint64_t entryTime()
  {
    return m_entryTime_state;
  }
  
  inline FAST_Nullable<uint32_t >& numTrades()
  {
    return m_numTrades_state;
  }
  
  inline FAST_Nullable<uint32_t >& tranMtchIdNo()
  {
    return m_tranMtchIdNo_state;
  }
};

class InsideMarketSnapshotInformation_EntriesDepth_Element
{
private:
  uint32_t m_entryType_state;
  double m_entryPrc_state;
  double m_entryQty_state;
  uint32_t m_numOrders_state;
  uint32_t m_entryPrcLvl_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryType_state = 0;
    m_entryPrc_state = 0;
    m_entryQty_state = 0;
    m_numOrders_state = 0;
    m_entryPrcLvl_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_entryType_state);
    decode_mandatory_decimal_delta(buf, m_entryPrc_state, perParseStringRepo, globalDictionary.delta_entryPrc);
    decode_mandatory_decimal_delta(buf, m_entryQty_state, perParseStringRepo, globalDictionary.delta_entryQty);
    decode_mandatory_delta<uint32_t >(buf, m_numOrders_state, perParseStringRepo, globalDictionary.delta_numOrders);
    decode_field_mandatory<uint32_t >(buf, m_entryPrcLvl_state);
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline double entryPrc()
  {
    return m_entryPrc_state;
  }
  
  inline double entryQty()
  {
    return m_entryQty_state;
  }
  
  inline uint32_t numOrders()
  {
    return m_numOrders_state;
  }
  
  inline uint32_t entryPrcLvl()
  {
    return m_entryPrcLvl_state;
  }
};

class InsideMarketSnapshotInformation_auctionGroup_Group_EntriesAuction_Element
{
private:
  uint32_t m_entryType_state;
  FAST_Nullable<double > m_entryPrc_state;
  FAST_Nullable<double > m_entryQty_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryType_state = 0;
    m_entryPrc_state.value = 0;
    m_entryQty_state.value = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_entryType_state);
    decode_optional_decimal_delta(buf, m_entryPrc_state, perParseStringRepo, globalDictionary.delta_entryPrc);
    decode_optional_decimal_delta(buf, m_entryQty_state, perParseStringRepo, globalDictionary.delta_entryQty);
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline FAST_Nullable<double >& entryPrc()
  {
    return m_entryPrc_state;
  }
  
  inline FAST_Nullable<double >& entryQty()
  {
    return m_entryQty_state;
  }
};

class InsideMarketSnapshotInformation_auctionGroup_Group
{
private:
  FAST_Nullable<FAST_String> m_moiInd;
  FAST_Nullable<FAST_String> m_volInd;
  FAST_Sequence<InsideMarketSnapshotInformation_auctionGroup_Group_EntriesAuction_Element, 50> m_EntriesAuction;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_optional(buf, m_moiInd);
    perParseStringRepo.read_string_optional(buf, m_volInd);
    decode_mandatory_sequence(buf, m_EntriesAuction, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_Nullable<FAST_String>& moiInd()
  {
    return m_moiInd;
  }
  
  inline FAST_Nullable<FAST_String>& volInd()
  {
    return m_volInd;
  }
  
  inline FAST_Sequence<InsideMarketSnapshotInformation_auctionGroup_Group_EntriesAuction_Element, 50>& EntriesAuction()
  {
    return m_EntriesAuction;
  }
};

class InsideMarketSnapshotInformation_EntriesPrc_Element
{
private:
  uint32_t m_entryType_state;
  double m_entryPrc_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryType_state = 0;
    m_entryPrc_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_entryType_state);
    decode_mandatory_decimal_delta(buf, m_entryPrc_state, perParseStringRepo, globalDictionary.delta_entryPrc);
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline double entryPrc()
  {
    return m_entryPrc_state;
  }
};

class InsideMarketSnapshotInformation
{
private:
  uint64_t m_entryTime_state;
  uint32_t m_srcId_state;
  uint32_t m_isix_state;
  uint32_t m_consolSeqNum_state;
  uint32_t m_instrStatus_state;
  FAST_Sequence<InsideMarketSnapshotInformation_EntriesAtp_Element, 50> m_EntriesAtp;
  FAST_Sequence<InsideMarketSnapshotInformation_EntriesDepth_Element, 50> m_EntriesDepth;
  FAST_Group<InsideMarketSnapshotInformation_auctionGroup_Group> m_auctionGroup;
  FAST_Sequence<InsideMarketSnapshotInformation_EntriesPrc_Element, 50> m_EntriesPrc;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryTime_state = 0;
    m_srcId_state = 0;
    m_isix_state = 0;
    m_consolSeqNum_state = 0;
    m_instrStatus_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_optional(buf, pmap);
    //pmap.read_next_pmap_entry();
    //skip_field_mandatory(buf); // template id
    decode_mandatory_delta<uint64_t >(buf, m_entryTime_state, perParseStringRepo, globalDictionary.delta_entryTime);
    decode_mandatory_copy<uint32_t >(buf, m_srcId_state, pmap, globalDictionary.copy_srcId);
    decode_mandatory_delta<uint32_t >(buf, m_isix_state, perParseStringRepo, globalDictionary.delta_isix);
    decode_mandatory_delta<uint32_t >(buf, m_consolSeqNum_state, perParseStringRepo, globalDictionary.delta_consolSeqNum);
    decode_field_mandatory<uint32_t >(buf, m_instrStatus_state);
    decode_mandatory_sequence(buf, m_EntriesAtp, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_EntriesDepth, perParseStringRepo, globalDictionary);
    decode_optional_group(buf, m_auctionGroup, pmap, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_EntriesPrc, perParseStringRepo, globalDictionary);
  }
  
  inline uint64_t entryTime()
  {
    return m_entryTime_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline uint32_t isix()
  {
    return m_isix_state;
  }
  
  inline uint32_t consolSeqNum()
  {
    return m_consolSeqNum_state;
  }
  
  inline uint32_t instrStatus()
  {
    return m_instrStatus_state;
  }
  
  inline FAST_Sequence<InsideMarketSnapshotInformation_EntriesAtp_Element, 50>& EntriesAtp()
  {
    return m_EntriesAtp;
  }
  
  inline FAST_Sequence<InsideMarketSnapshotInformation_EntriesDepth_Element, 50>& EntriesDepth()
  {
    return m_EntriesDepth;
  }
  
  inline FAST_Group<InsideMarketSnapshotInformation_auctionGroup_Group>& auctionGroup()
  {
    return m_auctionGroup;
  }
  
  inline FAST_Sequence<InsideMarketSnapshotInformation_EntriesPrc_Element, 50>& EntriesPrc()
  {
    return m_EntriesPrc;
  }
};

class InsideMarketDeltaInformation_EntriesTrade_Element_EntriesTradePrices_Element
{
private:
  uint32_t m_entryType_state;
  FAST_String m_priceTypeCod;
  double m_entryPrc_state;
  FAST_Nullable<double > m_entryQty_state;
  uint64_t m_entryTime_state;
  uint32_t m_actnCod_state;
  uint32_t m_tranMtchIdNo_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryType_state = 0;
    m_entryPrc_state = 0;
    m_entryQty_state.value = 0;
    m_entryTime_state = 0;
    m_actnCod_state = 0;
    m_tranMtchIdNo_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_entryType_state);
    perParseStringRepo.read_string_mandatory(buf, m_priceTypeCod);
    decode_mandatory_decimal_delta(buf, m_entryPrc_state, perParseStringRepo, globalDictionary.delta_entryPrc);
    decode_optional_decimal_delta(buf, m_entryQty_state, perParseStringRepo, globalDictionary.delta_entryQty);
    decode_mandatory_delta<uint64_t >(buf, m_entryTime_state, perParseStringRepo, globalDictionary.delta_entryTime);
    decode_mandatory_delta<uint32_t >(buf, m_actnCod_state, perParseStringRepo, globalDictionary.delta_actnCod);
    decode_mandatory_delta<uint32_t >(buf, m_tranMtchIdNo_state, perParseStringRepo, globalDictionary.delta_tranMtchIdNo);
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline FAST_String& priceTypeCod()
  {
    return m_priceTypeCod;
  }
  
  inline double entryPrc()
  {
    return m_entryPrc_state;
  }
  
  inline FAST_Nullable<double >& entryQty()
  {
    return m_entryQty_state;
  }
  
  inline uint64_t entryTime()
  {
    return m_entryTime_state;
  }
  
  inline uint32_t actnCod()
  {
    return m_actnCod_state;
  }
  
  inline uint32_t tranMtchIdNo()
  {
    return m_tranMtchIdNo_state;
  }
};

class InsideMarketDeltaInformation_EntriesTrade_Element
{
private:
  uint32_t m_entryType_state;
  FAST_Nullable<uint64_t > m_aggressorTime_state;
  double m_totTrdQty_state;
  uint32_t m_numTrades_state;
  FAST_Sequence<InsideMarketDeltaInformation_EntriesTrade_Element_EntriesTradePrices_Element, 50> m_EntriesTradePrices;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryType_state = 0;
    m_aggressorTime_state.has_value = false;
    m_totTrdQty_state = 0;
    m_numTrades_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_entryType_state);
    decode_field_optional<uint64_t >(buf, m_aggressorTime_state.value, m_aggressorTime_state.has_value);
    decode_field_mandatory<double >(buf, m_totTrdQty_state);
    decode_field_mandatory<uint32_t >(buf, m_numTrades_state);
    decode_mandatory_sequence(buf, m_EntriesTradePrices, perParseStringRepo, globalDictionary);
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline FAST_Nullable<uint64_t >& aggressorTime()
  {
    return m_aggressorTime_state;
  }
  
  inline double totTrdQty()
  {
    return m_totTrdQty_state;
  }
  
  inline uint32_t numTrades()
  {
    return m_numTrades_state;
  }
  
  inline FAST_Sequence<InsideMarketDeltaInformation_EntriesTrade_Element_EntriesTradePrices_Element, 50>& EntriesTradePrices()
  {
    return m_EntriesTradePrices;
  }
};

class InsideMarketDeltaInformation_EntriesDepth_Element
{
private:
  uint32_t m_entryType_state;
  double m_entryPrc_state;
  double m_entryQty_state;
  uint32_t m_numOrders_state;
  uint32_t m_entryPrcLvl_state;
  uint32_t m_updateAction_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryType_state = 0;
    m_entryPrc_state = 0;
    m_entryQty_state = 0;
    m_numOrders_state = 0;
    m_entryPrcLvl_state = 0;
    m_updateAction_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_entryType_state);
    decode_mandatory_decimal_delta(buf, m_entryPrc_state, perParseStringRepo, globalDictionary.delta_entryPrc);
    decode_mandatory_decimal_delta(buf, m_entryQty_state, perParseStringRepo, globalDictionary.delta_entryQty);
    decode_mandatory_delta<uint32_t >(buf, m_numOrders_state, perParseStringRepo, globalDictionary.delta_numOrders);
    decode_field_mandatory<uint32_t >(buf, m_entryPrcLvl_state);
    decode_field_mandatory<uint32_t >(buf, m_updateAction_state);
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline double entryPrc()
  {
    return m_entryPrc_state;
  }
  
  inline double entryQty()
  {
    return m_entryQty_state;
  }
  
  inline uint32_t numOrders()
  {
    return m_numOrders_state;
  }
  
  inline uint32_t entryPrcLvl()
  {
    return m_entryPrcLvl_state;
  }
  
  inline uint32_t updateAction()
  {
    return m_updateAction_state;
  }
};

class InsideMarketDeltaInformation_auctionGroup_Group_EntriesAuction_Element
{
private:
  uint32_t m_entryType_state;
  FAST_Nullable<double > m_entryPrc_state;
  FAST_Nullable<double > m_entryQty_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryType_state = 0;
    m_entryPrc_state.value = 0;
    m_entryQty_state.value = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_entryType_state);
    decode_optional_decimal_delta(buf, m_entryPrc_state, perParseStringRepo, globalDictionary.delta_entryPrc);
    decode_optional_decimal_delta(buf, m_entryQty_state, perParseStringRepo, globalDictionary.delta_entryQty);
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline FAST_Nullable<double >& entryPrc()
  {
    return m_entryPrc_state;
  }
  
  inline FAST_Nullable<double >& entryQty()
  {
    return m_entryQty_state;
  }
};

class InsideMarketDeltaInformation_auctionGroup_Group
{
private:
  FAST_Nullable<FAST_String> m_moiInd;
  FAST_Nullable<FAST_String> m_volInd;
  FAST_Sequence<InsideMarketDeltaInformation_auctionGroup_Group_EntriesAuction_Element, 50> m_EntriesAuction;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    perParseStringRepo.read_string_optional(buf, m_moiInd);
    perParseStringRepo.read_string_optional(buf, m_volInd);
    decode_mandatory_sequence(buf, m_EntriesAuction, perParseStringRepo, globalDictionary);
  }
  
  inline FAST_Nullable<FAST_String>& moiInd()
  {
    return m_moiInd;
  }
  
  inline FAST_Nullable<FAST_String>& volInd()
  {
    return m_volInd;
  }
  
  inline FAST_Sequence<InsideMarketDeltaInformation_auctionGroup_Group_EntriesAuction_Element, 50>& EntriesAuction()
  {
    return m_EntriesAuction;
  }
};

class InsideMarketDeltaInformation_EntriesPrc_Element
{
private:
  uint32_t m_entryType_state;
  double m_entryPrc_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryType_state = 0;
    m_entryPrc_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    decode_field_mandatory<uint32_t >(buf, m_entryType_state);
    decode_mandatory_decimal_delta(buf, m_entryPrc_state, perParseStringRepo, globalDictionary.delta_entryPrc);
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline double entryPrc()
  {
    return m_entryPrc_state;
  }
};

class InsideMarketDeltaInformation
{
private:
  uint64_t m_entryTime_state;
  uint32_t m_srcId_state;
  uint32_t m_isix_state;
  uint32_t m_seqNum_state;
  uint32_t m_instrStatus_state;
  FAST_Nullable<FAST_String> m_gapIndicator_state;
  FAST_Nullable<FAST_String> m_trdgapIndicator_state;
  FAST_Sequence<InsideMarketDeltaInformation_EntriesTrade_Element, 100> m_EntriesTrade;
  FAST_Sequence<InsideMarketDeltaInformation_EntriesDepth_Element, 100> m_EntriesDepth;
  FAST_Group<InsideMarketDeltaInformation_auctionGroup_Group> m_auctionGroup;
  FAST_Sequence<InsideMarketDeltaInformation_EntriesPrc_Element, 100> m_EntriesPrc;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryTime_state = 0;
    m_srcId_state = 0;
    m_isix_state = 0;
    m_seqNum_state = 0;
    m_instrStatus_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("Y", m_gapIndicator_state.value);
    stringRepo.assign_next_const_string_state<FAST_String>("Y", m_trdgapIndicator_state.value);
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_optional(buf, pmap);
    //pmap.read_next_pmap_entry();
    //skip_field_mandatory(buf); // template id
    decode_mandatory_delta<uint64_t >(buf, m_entryTime_state, perParseStringRepo, globalDictionary.delta_entryTime);
    decode_mandatory_copy<uint32_t >(buf, m_srcId_state, pmap, globalDictionary.copy_srcId);
    decode_mandatory_delta<uint32_t >(buf, m_isix_state, perParseStringRepo, globalDictionary.delta_isix);
    decode_mandatory_delta<uint32_t >(buf, m_seqNum_state, perParseStringRepo, globalDictionary.delta_seqNum);
    decode_field_mandatory<uint32_t >(buf, m_instrStatus_state);
    m_gapIndicator_state.has_value = pmap.read_next_pmap_entry();
    m_trdgapIndicator_state.has_value = pmap.read_next_pmap_entry();
    decode_mandatory_sequence(buf, m_EntriesTrade, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_EntriesDepth, perParseStringRepo, globalDictionary);
    decode_optional_group(buf, m_auctionGroup, pmap, perParseStringRepo, globalDictionary);
    decode_mandatory_sequence(buf, m_EntriesPrc, perParseStringRepo, globalDictionary);
  }
  
  inline uint64_t entryTime()
  {
    return m_entryTime_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline uint32_t isix()
  {
    return m_isix_state;
  }
  
  inline uint32_t seqNum()
  {
    return m_seqNum_state;
  }
  
  inline uint32_t instrStatus()
  {
    return m_instrStatus_state;
  }
  
  inline FAST_Nullable<FAST_String>& gapIndicator()
  {
    return m_gapIndicator_state;
  }
  
  inline FAST_Nullable<FAST_String>& trdgapIndicator()
  {
    return m_trdgapIndicator_state;
  }
  
  inline FAST_Sequence<InsideMarketDeltaInformation_EntriesTrade_Element, 100>& EntriesTrade()
  {
    return m_EntriesTrade;
  }
  
  inline FAST_Sequence<InsideMarketDeltaInformation_EntriesDepth_Element, 100>& EntriesDepth()
  {
    return m_EntriesDepth;
  }
  
  inline FAST_Group<InsideMarketDeltaInformation_auctionGroup_Group>& auctionGroup()
  {
    return m_auctionGroup;
  }
  
  inline FAST_Sequence<InsideMarketDeltaInformation_EntriesPrc_Element, 100>& EntriesPrc()
  {
    return m_EntriesPrc;
  }
};

class AllTradePrice
{
private:
  StringStateRepository m_string_repo;
  uint64_t m_timeStamp_state;
  uint32_t m_srcId_state;
  uint32_t m_isix_state;
  uint32_t m_seqNum_state;
  FAST_String m_priceTypeCod_state;
  uint32_t m_entryType_state;
  double m_entryPrc_state;
  FAST_Nullable<double > m_entryQty_state;
  uint64_t m_entryTime_state;
  uint32_t m_tranMtchIdNo_state;
  FAST_Defaultable<uint32_t > m_actnCod_state;

public:
  AllTradePrice()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_priceTypeCod_state, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_timeStamp_state = 0;
    m_srcId_state = 0;
    m_isix_state = 0;
    m_seqNum_state = 0;
    m_priceTypeCod_state.len = 0;

    m_entryType_state = 0;
    m_entryPrc_state = 0;
    m_entryQty_state.value = 0;
    m_entryTime_state = 0;
    m_tranMtchIdNo_state = 0;
    m_actnCod_state.is_default = true;
    m_actnCod_state.default_value = 4;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_optional(buf, pmap);
    //pmap.read_next_pmap_entry();
    //skip_field_mandatory(buf); // template id
    decode_mandatory_delta<uint64_t >(buf, m_timeStamp_state, perParseStringRepo, globalDictionary.delta_timeStamp);
    decode_mandatory_copy<uint32_t >(buf, m_srcId_state, pmap, globalDictionary.copy_srcId);
    decode_mandatory_delta<uint32_t >(buf, m_isix_state, perParseStringRepo, globalDictionary.delta_isix);
    decode_mandatory_increment<uint32_t >(buf, m_seqNum_state, pmap, globalDictionary.increment_seqNum);
    decode_mandatory_copy<FAST_String>(buf, m_priceTypeCod_state, pmap, globalDictionary.copy_priceTypeCod);
    decode_mandatory_copy<uint32_t >(buf, m_entryType_state, pmap, globalDictionary.copy_entryType);
    decode_mandatory_decimal_delta(buf, m_entryPrc_state, perParseStringRepo, globalDictionary.delta_entryPrc);
    decode_optional_decimal_delta(buf, m_entryQty_state, perParseStringRepo, globalDictionary.delta_entryQty);
    decode_mandatory_delta<uint64_t >(buf, m_entryTime_state, perParseStringRepo, globalDictionary.delta_entryTime);
    decode_mandatory_delta<uint32_t >(buf, m_tranMtchIdNo_state, perParseStringRepo, globalDictionary.delta_tranMtchIdNo);
    decode_mandatory_default<uint32_t >(buf, m_actnCod_state, pmap, globalDictionary.default_actnCod);
  }
  
  inline uint64_t timeStamp()
  {
    return m_timeStamp_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline uint32_t isix()
  {
    return m_isix_state;
  }
  
  inline uint32_t seqNum()
  {
    return m_seqNum_state;
  }
  
  inline FAST_String& priceTypeCod()
  {
    return m_priceTypeCod_state;
  }
  
  inline uint32_t entryType()
  {
    return m_entryType_state;
  }
  
  inline double entryPrc()
  {
    return m_entryPrc_state;
  }
  
  inline FAST_Nullable<double >& entryQty()
  {
    return m_entryQty_state;
  }
  
  inline uint64_t entryTime()
  {
    return m_entryTime_state;
  }
  
  inline uint32_t tranMtchIdNo()
  {
    return m_tranMtchIdNo_state;
  }
  
  inline uint32_t actnCod()
  {
    return m_actnCod_state.current_value();
  }
};

class StateChangesMessage
{
private:
  StringStateRepository m_string_repo;
  uint64_t m_timeStamp_state;
  uint32_t m_srcId_state;
  FAST_Nullable<FAST_String> m_changeIndicator_state;
  uint32_t m_state_state;
  FAST_Nullable<FAST_String> m_exchId_state;

public:
  StateChangesMessage()
  {
    m_string_repo.allocate_next_string_state<FAST_String>(m_exchId_state.value, 100);
  }

  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_timeStamp_state = 0;
    m_srcId_state = 0;
    stringRepo.assign_next_const_string_state<FAST_String>("Y", m_changeIndicator_state.value);
    m_state_state = 0;
    m_exchId_state.has_value = false;

  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_mandatory_delta<uint64_t >(buf, m_timeStamp_state, perParseStringRepo, globalDictionary.delta_timeStamp);
    decode_mandatory_copy<uint32_t >(buf, m_srcId_state, pmap, globalDictionary.copy_srcId);
    m_changeIndicator_state.has_value = pmap.read_next_pmap_entry();
    decode_mandatory_copy<uint32_t >(buf, m_state_state, pmap, globalDictionary.copy_state);
    decode_optional_copy<FAST_String>(buf, m_exchId_state, pmap, globalDictionary.copy_exchId);
  }
  
  inline uint64_t timeStamp()
  {
    return m_timeStamp_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline FAST_Nullable<FAST_String>& changeIndicator()
  {
    return m_changeIndicator_state;
  }
  
  inline uint32_t state()
  {
    return m_state_state;
  }
  
  inline FAST_Nullable<FAST_String>& exchId()
  {
    return m_exchId_state;
  }
};

class CrossRequestMessage
{
private:
  uint64_t m_entryTime_state;
  uint32_t m_srcId_state;
  uint32_t m_isix_state;
  double m_entryQty_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryTime_state = 0;
    m_srcId_state = 0;
    m_isix_state = 0;
    m_entryQty_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_mandatory_delta<uint64_t >(buf, m_entryTime_state, perParseStringRepo, globalDictionary.delta_entryTime);
    decode_mandatory_copy<uint32_t >(buf, m_srcId_state, pmap, globalDictionary.copy_srcId);
    decode_field_mandatory<uint32_t >(buf, m_isix_state);
    decode_field_mandatory<double >(buf, m_entryQty_state);
  }
  
  inline uint64_t entryTime()
  {
    return m_entryTime_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline uint32_t isix()
  {
    return m_isix_state;
  }
  
  inline double entryQty()
  {
    return m_entryQty_state;
  }
};

class TechnicalMarketResetMessage
{
private:
  uint64_t m_entryTime_state;
  uint32_t m_srcId_state;
  uint32_t m_setId_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryTime_state = 0;
    m_srcId_state = 0;
    m_setId_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    pmap.read_next_pmap_entry();
    skip_field_mandatory(buf); // template id
    decode_mandatory_delta<uint64_t >(buf, m_entryTime_state, perParseStringRepo, globalDictionary.delta_entryTime);
    decode_mandatory_copy<uint32_t >(buf, m_srcId_state, pmap, globalDictionary.copy_srcId);
    decode_field_mandatory<uint32_t >(buf, m_setId_state);
  }
  
  inline uint64_t entryTime()
  {
    return m_entryTime_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline uint32_t setId()
  {
    return m_setId_state;
  }
};

class InstrumentStateMessage
{
private:
  uint64_t m_entryTime_state;
  uint32_t m_srcId_state;
  uint32_t m_isix_state;
  uint32_t m_state_state;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    m_entryTime_state = 0;
    m_srcId_state = 0;
    m_isix_state = 0;
    m_state_state = 0;
  }

  void Parse(uint8_t*& buf, PerParseStringRepository& perParseStringRepo, Xetra_Fast_Decoder_GlobalDictionary& globalDictionary)
  {
    Pmap pmap(buf);
    skip_field_mandatory(buf); // get past the pmap

    skip_field_optional(buf, pmap);
    //pmap.read_next_pmap_entry();
    //skip_field_mandatory(buf); // template id
    decode_mandatory_delta<uint64_t >(buf, m_entryTime_state, perParseStringRepo, globalDictionary.delta_entryTime);
    decode_mandatory_copy<uint32_t >(buf, m_srcId_state, pmap, globalDictionary.copy_srcId);
    decode_field_mandatory<uint32_t >(buf, m_isix_state);
    decode_field_mandatory<uint32_t >(buf, m_state_state);
  }
  
  inline uint64_t entryTime()
  {
    return m_entryTime_state;
  }
  
  inline uint32_t srcId()
  {
    return m_srcId_state;
  }
  
  inline uint32_t isix()
  {
    return m_isix_state;
  }
  
  inline uint32_t state()
  {
    return m_state_state;
  }
};


class Xetra_Fast_Decoder
{
private:
  PerParseStringRepository m_per_parse_string_repo;
  StringStateRepository m_string_repo;
  Xetra_Fast_Decoder_GlobalDictionary m_global_dictionary;
  PacketHeader m_PacketHeader;
  StartService m_StartService;
  EndService m_EndService;
  StartRefData m_StartRefData;
  EndRefData m_EndRefData;
  StartMtnData m_StartMtnData;
  EndMtnData m_EndMtnData;
  BeaconMessage m_BeaconMessage;
  InstrumentReferenceData m_InstrumentReferenceData;
  MaintenanceReferenceData m_MaintenanceReferenceData;
  InsideMarketSnapshotInformation m_InsideMarketSnapshotInformation;
  InsideMarketDeltaInformation m_InsideMarketDeltaInformation;
  AllTradePrice m_AllTradePrice;
  StateChangesMessage m_StateChangesMessage;
  CrossRequestMessage m_CrossRequestMessage;
  TechnicalMarketResetMessage m_TechnicalMarketResetMessage;
  InstrumentStateMessage m_InstrumentStateMessage;

public:
  Xetra_Fast_Decoder()
  {
    Reset();
  }

  void Reset()
  {
    m_PacketHeader.Reset_Dictionary(m_string_repo);
    m_StartService.Reset_Dictionary(m_string_repo);
    m_EndService.Reset_Dictionary(m_string_repo);
    m_StartRefData.Reset_Dictionary(m_string_repo);
    m_EndRefData.Reset_Dictionary(m_string_repo);
    m_StartMtnData.Reset_Dictionary(m_string_repo);
    m_EndMtnData.Reset_Dictionary(m_string_repo);
    m_BeaconMessage.Reset_Dictionary(m_string_repo);
    m_InstrumentReferenceData.Reset_Dictionary(m_string_repo);
    m_MaintenanceReferenceData.Reset_Dictionary(m_string_repo);
    m_InsideMarketSnapshotInformation.Reset_Dictionary(m_string_repo);
    m_InsideMarketDeltaInformation.Reset_Dictionary(m_string_repo);
    m_AllTradePrice.Reset_Dictionary(m_string_repo);
    m_StateChangesMessage.Reset_Dictionary(m_string_repo);
    m_CrossRequestMessage.Reset_Dictionary(m_string_repo);
    m_TechnicalMarketResetMessage.Reset_Dictionary(m_string_repo);
    m_InstrumentStateMessage.Reset_Dictionary(m_string_repo);
    m_string_repo.reset();
    m_global_dictionary.reset();
  }

  PacketHeader& Parse_PacketHeader(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_PacketHeader.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_PacketHeader;
  }

  StartService& Parse_StartService(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_StartService.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_StartService;
  }

  EndService& Parse_EndService(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_EndService.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_EndService;
  }

  StartRefData& Parse_StartRefData(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_StartRefData.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_StartRefData;
  }

  EndRefData& Parse_EndRefData(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_EndRefData.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_EndRefData;
  }

  StartMtnData& Parse_StartMtnData(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_StartMtnData.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_StartMtnData;
  }

  EndMtnData& Parse_EndMtnData(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_EndMtnData.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_EndMtnData;
  }

  BeaconMessage& Parse_BeaconMessage(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_BeaconMessage.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_BeaconMessage;
  }

  InstrumentReferenceData& Parse_InstrumentReferenceData(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_InstrumentReferenceData.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_InstrumentReferenceData;
  }

  MaintenanceReferenceData& Parse_MaintenanceReferenceData(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_MaintenanceReferenceData.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_MaintenanceReferenceData;
  }

  InsideMarketSnapshotInformation& Parse_InsideMarketSnapshotInformation(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_InsideMarketSnapshotInformation.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_InsideMarketSnapshotInformation;
  }

  InsideMarketDeltaInformation& Parse_InsideMarketDeltaInformation(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_InsideMarketDeltaInformation.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_InsideMarketDeltaInformation;
  }

  AllTradePrice& Parse_AllTradePrice(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_AllTradePrice.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_AllTradePrice;
  }

  StateChangesMessage& Parse_StateChangesMessage(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_StateChangesMessage.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_StateChangesMessage;
  }

  CrossRequestMessage& Parse_CrossRequestMessage(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_CrossRequestMessage.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_CrossRequestMessage;
  }

  TechnicalMarketResetMessage& Parse_TechnicalMarketResetMessage(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_TechnicalMarketResetMessage.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_TechnicalMarketResetMessage;
  }

  InstrumentStateMessage& Parse_InstrumentStateMessage(uint8_t*& buf)
  {
    m_per_parse_string_repo.reset();
    m_InstrumentStateMessage.Parse(buf, m_per_parse_string_repo, m_global_dictionary);
    return m_InstrumentStateMessage;
  }

  uint32_t Get_Template_Id(uint8_t* buf)
  {
    skip_field_mandatory(buf); // get past the pmap
    uint32_t ret;
    decode_field_mandatory<uint32_t>(buf, ret);
    return ret;
  }
};
