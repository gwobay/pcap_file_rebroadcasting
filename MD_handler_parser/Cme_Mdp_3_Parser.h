namespace hf_core
{

struct CME_MDP_3_v5
{
  template<typename T, uint64_t NULL_VALUE>
  struct CME_MDP_Nullable
  {
    T value;
    bool has_value()
    {
      return value != NULL_VALUE;
    }
  } __attribute__ ((packed));

  typedef char Asset[6];
  typedef char CFICode[6];
  typedef char CHAR;
  typedef char Currency[3];
  typedef int16_t Int16;
  typedef int32_t Int32;
  typedef CME_MDP_Nullable<int32_t, 2147483647> Int32NULL;
  typedef int8_t Int8;
  typedef CME_MDP_Nullable<int8_t, 127> Int8NULL;
  typedef CME_MDP_Nullable<uint16_t, 65535> LocalMktDate;
  typedef char MDFeedType[3];
  typedef char QuoteReqId[23];
  typedef char SecurityExchange[4];
  typedef char SecurityGroup[6];
  typedef char SecuritySubType[5];
  typedef char SecurityType[6];
  typedef char Symbol[20];
  typedef char Text[180];
  typedef char UnderlyingSymbol[20];
  typedef char UnitOfMeasure[30];
  typedef char UserDefinedInstrument;
  typedef uint32_t uInt32;
  typedef CME_MDP_Nullable<uint32_t, 4294967295> uInt32NULL;
  typedef uint64_t uInt64;
  typedef uint8_t uInt8;
  typedef CME_MDP_Nullable<uint8_t, 255> uInt8NULL;

  struct DecimalQty
  {
    CME_MDP_Nullable<int32_t, 2147483647> mantissa;
    //static const int8_t exponent = -4;
  } __attribute__ ((packed));

  struct FLOAT
  {
    int64_t mantissa;
    //static const int8_t exponent = -7;
  } __attribute__ ((packed));

  struct MaturityMonthYear
  {
    CME_MDP_Nullable<uint16_t, 65535> year;
    CME_MDP_Nullable<uint8_t, 255> month;
    CME_MDP_Nullable<uint8_t, 255> day;
    CME_MDP_Nullable<uint8_t, 255> week;
  } __attribute__ ((packed));

  struct PRICE
  {
    int64_t mantissa;
    //static const int8_t exponent = -7;
  } __attribute__ ((packed));

  struct PRICENULL
  {
    CME_MDP_Nullable<int64_t, 9223372036854775807> mantissa;
    //static const int8_t exponent = -7;
  } __attribute__ ((packed));

  struct groupSize
  {
    uint16_t blockLength;
    uint8_t numInGroup;
  } __attribute__ ((packed));

  struct groupSize8Byte
  {
    uint16_t blockLength;
    uint8_t numInGroup;
  } __attribute__ ((packed));

  struct groupSizeEncoding
  {
    uint16_t blockLength;
    uint16_t numInGroup;
  } __attribute__ ((packed));

  struct messageHeader
  {
    uint16_t blockLength;
    uint16_t templateId;
    uint16_t schemaId;
    uint16_t version;
  } __attribute__ ((packed));

  struct AggressorSide
  {
    uInt8NULL value;
    enum Enum
    {
      NoAggressor = 0,
      Buy = 1,
      Sell = 2,
    };
  } __attribute__ ((packed));

  struct EventType
  {
    uInt8 value;
    enum Enum
    {
      Activation = 5,
      LastEligibleTradeDate = 7,
    };
  } __attribute__ ((packed));

  struct HaltReason
  {
    uInt8 value;
    enum Enum
    {
      GroupSchedule = 0,
      SurveillanceIntervention = 1,
      MarketEvent = 2,
      InstrumentActivation = 3,
      InstrumentExpiration = 4,
      Unknown = 5,
      RecoveryInProcess = 6,
    };
  } __attribute__ ((packed));

  struct LegSide
  {
    uInt8 value;
    enum Enum
    {
      BuySide = 1,
      SellSide = 2,
    };
  } __attribute__ ((packed));

  struct MDEntryType
  {
    CHAR value;
    enum Enum
    {
      Bid = '0',
      Offer = '1',
      Trade = '2',
      OpeningPrice = '4',
      SettlementPrice = '6',
      TradingSessionHighPrice = '7',
      TradingSessionLowPrice = '8',
      TradeVolume = 'B',
      OpenInterest = 'C',
      ImpliedBid = 'E',
      ImpliedOffer = 'F',
      EmptyBook = 'J',
      SessionHighBid = 'N',
      SessionLowOffer = 'O',
      FixingPrice = 'W',
      ElectronicVolume = 'e',
      ThresholdLimitsandPriceBandVariation = 'g',
    };
  } __attribute__ ((packed));

  struct MDEntryTypeBook
  {
    CHAR value;
    enum Enum
    {
      Bid = '0',
      Offer = '1',
      ImpliedBid = 'E',
      ImpliedOffer = 'F',
      BookReset = 'J',
    };
  } __attribute__ ((packed));

  struct MDEntryTypeDailyStatistics
  {
    CHAR value;
    enum Enum
    {
      SettlementPrice = '6',
      ClearedVolume = 'B',
      OpenInterest = 'C',
      FixingPrice = 'W',
    };
  } __attribute__ ((packed));

  struct MDEntryTypeStatistics
  {
    CHAR value;
    enum Enum
    {
      OpenPrice = '4',
      HighTrade = '7',
      LowTrade = '8',
      HighestBid = 'N',
      LowestOffer = 'O',
    };
  } __attribute__ ((packed));

  struct MDUpdateAction
  {
    uInt8 value;
    enum Enum
    {
      New = 0,
      Change = 1,
      Delete = 2,
      DeleteThru = 3,
      DeleteFrom = 4,
      Overlay = 5,
    };
  } __attribute__ ((packed));

  struct OpenCloseSettlFlag
  {
    uInt8NULL value;
    enum Enum
    {
      DailyOpenPrice = 0,
      IndicativeOpeningPrice = 5,
    };
  } __attribute__ ((packed));

  struct PutOrCall
  {
    uInt8 value;
    enum Enum
    {
      Put = 0,
      Call = 1,
    };
  } __attribute__ ((packed));

  struct SecurityTradingEvent
  {
    uInt8 value;
    enum Enum
    {
      NoEvent = 0,
      NoCancel = 1,
      ResetStatistics = 4,
      ImpliedMatchingON = 5,
      ImpliedMatchingOFF = 6,
    };
  } __attribute__ ((packed));

  struct SecurityTradingStatus
  {
    uInt8NULL value;
    enum Enum
    {
      TradingHalt = 2,
      Close = 4,
      NewPriceIndication = 15,
      ReadyToTrade = 17,
      NotAvailableForTrading = 18,
      UnknownorInvalid = 20,
      PreOpen = 21,
      PreCross = 24,
      Cross = 25,
      PostClose = 26,
      NoChange = 103,
    };
  } __attribute__ ((packed));

  struct SecurityUpdateAction
  {
    CHAR value;
    enum Enum
    {
      Add = 'A',
      Delete = 'D',
      Modify = 'M',
    };
  } __attribute__ ((packed));

  struct InstAttribValue
  {
  private:
    uint32_t value;
  public:
    bool ElectronicMatchEligible() { return (value & (0x01 << 0)); }
    bool OrderCrossEligible() { return (value & (0x01 << 1)); }
    bool BlockTradeEligible() { return (value & (0x01 << 2)); }
    bool EFPEligible() { return (value & (0x01 << 3)); }
    bool EBFEligible() { return (value & (0x01 << 4)); }
    bool EFSEligible() { return (value & (0x01 << 5)); }
    bool EFREligible() { return (value & (0x01 << 6)); }
    bool OTCEligible() { return (value & (0x01 << 7)); }
    bool iLinkIndicativeMassQuotingEligible() { return (value & (0x01 << 8)); }
    bool NegativeStrikeEligible() { return (value & (0x01 << 9)); }
    bool NegativePriceOutrightEligible() { return (value & (0x01 << 10)); }
    bool IsFractional() { return (value & (0x01 << 11)); }
    bool VolatilityQuotedOption() { return (value & (0x01 << 12)); }
    bool RFQCrossEligible() { return (value & (0x01 << 13)); }
    bool ZeroPriceOutrightEligible() { return (value & (0x01 << 14)); }
    bool DecayingProductEligibility() { return (value & (0x01 << 15)); }
    bool VariableProductEligibility() { return (value & (0x01 << 16)); }
    bool DailyProductEligibility() { return (value & (0x01 << 17)); }
    bool GTOrdersEligibility() { return (value & (0x01 << 18)); }
    bool ImpliedMatchingEligibility() { return (value & (0x01 << 19)); }
  } __attribute__ ((packed));

  struct MatchEventIndicator
  {
  private:
  public:
    uint8_t value;
    bool LastTradeMsg() { return (value & (0x01 << 0)); }
    bool LastVolumeMsg() { return (value & (0x01 << 1)); }
    bool LastQuoteMsg() { return (value & (0x01 << 2)); }
    bool LastStatsMsg() { return (value & (0x01 << 3)); }
    bool LastImpliedMsg() { return (value & (0x01 << 4)); }
    bool RecoveryMsg() { return (value & (0x01 << 5)); }
    bool Reserved() { return (value & (0x01 << 6)); }
    bool EndOfEvent() { return (value & (0x01 << 7)); }

    void print_str()
    {
      std::cout << std::bitset<8>(value);
      if(LastTradeMsg()) { std::cout << " LastTradeMsg"; }
      if(LastVolumeMsg()) { std::cout << " LastVolumeMsg"; }
      if(LastQuoteMsg()) { std::cout << " LastQuoteMsg"; }
      if(LastStatsMsg()) { std::cout << " LastStatsMsg"; }
      if(LastImpliedMsg()) { std::cout << " LastImpliedMsg"; }
      if(RecoveryMsg()) { std::cout << " RecoveryMsg"; }
      if(Reserved()) { std::cout << " Reserved"; }
      if(EndOfEvent()) { std::cout << " EndOfEvent"; }
    }
  } __attribute__ ((packed));

  struct SettlPriceType
  {
  private:
uint8_t value;
public:
bool Final() { return (value & (0x01 << 0)); }
bool Actual() { return (value & (0x01 << 1)); }
bool Rounded() { return (value & (0x01 << 2)); }
bool Intraday() { return (value & (0x01 << 3)); }
bool ReservedBits() { return (value & (0x01 << 4)); }
bool NullValue() { return (value & (0x01 << 7)); }
  } __attribute__ ((packed));

  struct ChannelReset4_MDEntries_Entry
  {
    //static const int8_t md_update_action = 0: // Market Data update action
    //static const char * md_entry_type = "J": // Market Data entry type  
    Int16 appl_id; // Indicates the channel ID as defined in the XML configuration file
  } __attribute__ ((packed));

  struct ChannelReset4
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event

    inline groupSize& no_md_entries()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(ChannelReset4));
    }

    typedef ChannelReset4_MDEntries_Entry md_entries_t;
    md_entries_t& md_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<ChannelReset4_MDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(ChannelReset4) + sizeof(groupSize) + (2*entry_offset));
    }
  } __attribute__ ((packed));

  struct AdminHeartbeat12
  {
  } __attribute__ ((packed));

  struct AdminLogin15
  {
    Int8 heart_bt_int; // Heartbeat interval (seconds)
  } __attribute__ ((packed));

  struct AdminLogout16
  {
    Text text; // Free format text string. May include logout confirmation or reason for logout
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionFuture27_Events_Entry
  {
    EventType event_type; // Code to represent the type of event
    uInt64 event_time; // Date and Time of instument Activation or Expiration event sent as number of nanoseconds since Unix epoch
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionFuture27_MDFeedTypes_Entry
  {
    MDFeedType md_feed_type; // Describes a class of service for a given data feed. GBX- Real Book, GBI-Implied Book
    Int8 market_depth; // Book depth
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionFuture27_InstAttrib_Entry
  {
    //static const int8_t inst_attrib_type = 24: // Instrument eligibility attributes
    InstAttribValue inst_attrib_value; // Bitmap field of 32 Boolean type instrument eligibility flags
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionFuture27_LotTypeRules_Entry
  {
    Int8 lot_type; // This tag is required to interpret the value in tag 1231-MinLotSize
    DecimalQty min_lot_size; // Minimum quantity accepted for order entry. If tag 1093-LotType=4, this value is the minimum quantity for order entry expressed in the applicable units, specified in tag 996-UnitOfMeasure, e.g. megawatts
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionFuture27
  {
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event
    uInt32NULL tot_num_reports; // Total number of instruments in the Replay loop. Used on Replay Feed only 
    SecurityUpdateAction security_update_action; // Last Security update action on Incremental feed, 'D' or 'M' is used when a mid-week deletion or modification (i.e. extension) occurs
    uInt64 last_update_time; // Timestamp of when the instrument was last added, modified or deleted
    SecurityTradingStatus md_security_trading_status; // Identifies the current state of the instrument. In Security Definition message this tag is available in the Instrument Replay feed only 
    Int16 appl_id; // The channel ID as defined in the XML Configuration file
    uInt8 market_segment_id; // Last Security update action on Incremental feed, 'D' or 'M' is used when a mid-week deletion or modification (i.e. extension) occurs
    uInt8 underlying_product; // Product complex
    SecurityExchange security_exchange; // Exchange used to identify a security
    SecurityGroup security_group; // Security Group Code.
    Asset asset; // The underlying asset code also known as Product Code
    Symbol symbol; // Instrument Name or Symbol 
    Int32 security_id; // Unique instrument ID
    //static const char security_id_source = '8': // Identifies class or source of tag 48-SecurityID value
    SecurityType security_type; // Security Type
    CFICode cfi_code; // ISO standard instrument categorization code
    MaturityMonthYear maturity_month_year; // This field provides the actual calendar date for contract maturity
    Currency currency; // Identifies currency used for price
    Currency settl_currency; // Identifies currency used for settlement, if different from trading currency
    CHAR match_algorithm; // Matching algorithm 
    uInt32 min_trade_vol; // The minimum trading volume for a security
    uInt32 max_trade_vol; // The maximum trading volume for a security
    PRICE min_price_increment; // Minimum constant tick for the instrument, sent only if instrument is non-VTT (Variable Tick table) eligible
    FLOAT display_factor; // Contains the multiplier to convert the CME Globex display price to the conventional price
    uInt8NULL main_fraction; // Price Denominator of Main Fraction
    uInt8NULL sub_fraction; // Price Denominator of Sub Fraction
    uInt8NULL price_display_format; // Number of decimals in fractional display price
    UnitOfMeasure unit_of_measure; // Unit of measure for the products' original contract size. This will be populated for all products listed on CME Globex
    PRICENULL unit_of_measure_qty; // This field contains the contract size for each instrument. Used in combination with tag 996-UnitofMeasure
    PRICENULL trading_reference_price; // Reference price for prelisted instruments or the last calculated Settlement whether it be Theoretical, Preliminary or a Final Settle of the session.
    SettlPriceType settl_price_type; // Bitmap field of eight Boolean type indicators representing settlement price type
    Int32NULL open_interest_qty; // The total open interest for the market at the close of the prior trading session.
    Int32NULL cleared_volume; // The total cleared volume of instrument traded during the prior trading session.
    PRICENULL high_limit_price; // Allowable high limit price for the trading day
    PRICENULL low_limit_price; // Allowable low limit price for the trading day
    PRICENULL max_price_variation; // Differential value for price banding.
    Int32NULL decay_quantity; // Indicates the quantity that a contract will decay daily by once the decay start date is reached
    LocalMktDate decay_start_date; // Indicates the date at which a decaying contract will begin to decay
    Int32NULL original_contract_size; // Fixed contract value assigned to each product
    Int32NULL contract_multiplier; // Number of deliverable units per instrument, e.g., peak days in maturity month or number of calendar days in maturity month
    Int8NULL contract_multiplier_unit; // Indicates the type of multiplier being applied to the product. Optionally used in combination with tag 231-ContractMultiplier
    Int8NULL flow_schedule_type; // The schedule according to which the electricity is delivered in a physical contract, or priced in a financial contract. Specifies whether the contract is defined according to the Easter Peak, Eastern Off-Peak, Western Peak or Western Off-Peak.
    PRICENULL min_price_increment_amount; // Monetary value equivalent to the minimum price fluctuation
    UserDefinedInstrument user_defined_instrument; // User-defined instruments flag

    inline groupSize& no_events()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionFuture27));
    }

    typedef MDInstrumentDefinitionFuture27_Events_Entry events_t;
    events_t& events(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionFuture27_Events_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionFuture27) + sizeof(groupSize) + (9*entry_offset));
    }

    inline groupSize& no_md_feed_types()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionFuture27) + sizeof(groupSize) + (no_events().numInGroup * 9));
    }

    typedef MDInstrumentDefinitionFuture27_MDFeedTypes_Entry md_feed_types_t;
    md_feed_types_t& md_feed_types(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionFuture27_MDFeedTypes_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionFuture27) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (4*entry_offset));
    }

    inline groupSize& no_inst_attrib()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionFuture27) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4));
    }

    typedef MDInstrumentDefinitionFuture27_InstAttrib_Entry inst_attrib_t;
    inst_attrib_t& inst_attrib(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionFuture27_InstAttrib_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionFuture27) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (4*entry_offset));
    }

    inline groupSize& no_lot_type_rules()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionFuture27) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4));
    }

    typedef MDInstrumentDefinitionFuture27_LotTypeRules_Entry lot_type_rules_t;
    lot_type_rules_t& lot_type_rules(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionFuture27_LotTypeRules_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionFuture27) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4) + sizeof(groupSize) + (5*entry_offset));
    }
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionSpread29_Events_Entry
  {
    EventType event_type; // Code to represent the type of event
    uInt64 event_time; // Date and time of instument Activation or Expiration event sent as number of nanoseconds since Unix epoch
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionSpread29_MDFeedTypes_Entry
  {
    MDFeedType md_feed_type; // Describes a class of service for a given data feed. GBX- Real Book, GBI-Implied Book
    Int8 market_depth; // Identifies the depth of book
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionSpread29_InstAttrib_Entry
  {
    //static const int8_t inst_attrib_type = 24: // Instrument Eligibility Attributes
    InstAttribValue inst_attrib_value; // Bitmap field of 32 Boolean type Instrument eligibility flags
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionSpread29_LotTypeRules_Entry
  {
    Int8 lot_type; // This tag is required to interpret the value in tag 1231-MinLotSize
    DecimalQty min_lot_size; // Minimum quantity accepted for order entry. If tag 1093-LotType=4, this value is the minimum quantity for order entry expressed in the applicable units, specified in tag 996-UnitOfMeasure, e.g. megawatts
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionSpread29_Legs_Entry
  {
    Int32 leg_security_id; // Leg Security ID
    //static const char leg_security_id_source = '8': // Identifies source of tag 602-LegSecurityID value
    LegSide leg_side; // Leg side
    Int8 leg_ratio_qty; // Leg ratio of quantity for this individual leg relative to the entire multi-leg instrument
    PRICENULL leg_price; // Price for the future leg of a UDS Covered instrument 
    DecimalQty leg_option_delta; // Delta used to calculate the quantity of futures used to cover the option or option strategy
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionSpread29
  {
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event
    uInt32NULL tot_num_reports; // Total number of instruments in the Replay loop. Used on Replay Feed only
    SecurityUpdateAction security_update_action; // Last Security update action on Incremental feed, 'D' or 'M' is used when a mid-week deletion or modification (i.e. extension) occurs
    uInt64 last_update_time; // Timestamp of when the instrument was last added, modified or deleted
    SecurityTradingStatus md_security_trading_status; // Identifies the current state of the instrument. The data is available in the Instrument Replay feed only
    Int16 appl_id; // The channel ID as defined in the XML Configuration file
    uInt8 market_segment_id; // Identifies the market segment, populated for all CME Globex instruments
    uInt8NULL underlying_product; // Product complex
    SecurityExchange security_exchange; // Exchange used to identify a security
    SecurityGroup security_group; // Security Group Code
    Asset asset; // The underlying asset code also known as Product Code
    Symbol symbol; // Instrument Name or Symbol. Previously used as  Group Code 
    Int32 security_id; // Unique instrument ID
    //static const char security_id_source = '8': // Identifies class or source of the security ID (Tag 48) value
    SecurityType security_type; // Security Type
    CFICode cfi_code; // ISO standard instrument categorization code
    MaturityMonthYear maturity_month_year; // This field provides the actual calendar date for contract maturity
    Currency currency; // Identifies currency used for price
    SecuritySubType security_sub_type; // Strategy type
    UserDefinedInstrument user_defined_instrument; // User-defined instruments flag
    CHAR match_algorithm; // Matching algorithm
    uInt32 min_trade_vol; // The minimum trading volume for a security
    uInt32 max_trade_vol; // The maximum trading volume for a security
    PRICE min_price_increment; // Minimum constant tick for the instrument, sent only if instrument is non-VTT (Variable Tick table) eligible
    FLOAT display_factor; // Contains the multiplier to convert the CME Globex display price to the conventional price
    uInt8NULL price_display_format; // Number of decimals in fractional display price
    PRICENULL price_ratio; // Used for price calculation in spread and leg pricing
    Int8NULL tick_rule; // Tick Rule 
    UnitOfMeasure unit_of_measure; // Unit of measure for the products' original contract size
    PRICENULL trading_reference_price; // Reference price - the most recently available Settlement whether it be Theoretical, Preliminary or a Final Settle of the session
    SettlPriceType settl_price_type; // Bitmap field of eight Boolean type indicators representing settlement price type
    Int32NULL open_interest_qty; // The total open interest for the market at the close of the prior trading session
    Int32NULL cleared_volume; // The total cleared volume of instrument traded during the prior trading session
    PRICENULL high_limit_price; // Allowable high limit price for the trading day
    PRICENULL low_limit_price; // Allowable low limit price for the trading day
    PRICENULL max_price_variation; // Differential value for price banding
    uInt8NULL main_fraction; // Price Denominator of Main Fraction
    uInt8NULL sub_fraction; // Price Denominator of Sub Fraction

    inline groupSize& no_events()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29));
    }

    typedef MDInstrumentDefinitionSpread29_Events_Entry events_t;
    events_t& events(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionSpread29_Events_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29) + sizeof(groupSize) + (9*entry_offset));
    }

    inline groupSize& no_md_feed_types()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29) + sizeof(groupSize) + (no_events().numInGroup * 9));
    }

    typedef MDInstrumentDefinitionSpread29_MDFeedTypes_Entry md_feed_types_t;
    md_feed_types_t& md_feed_types(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionSpread29_MDFeedTypes_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (4*entry_offset));
    }

    inline groupSize& no_inst_attrib()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4));
    }

    typedef MDInstrumentDefinitionSpread29_InstAttrib_Entry inst_attrib_t;
    inst_attrib_t& inst_attrib(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionSpread29_InstAttrib_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (4*entry_offset));
    }

    inline groupSize& no_lot_type_rules()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4));
    }

    typedef MDInstrumentDefinitionSpread29_LotTypeRules_Entry lot_type_rules_t;
    lot_type_rules_t& lot_type_rules(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionSpread29_LotTypeRules_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4) + sizeof(groupSize) + (5*entry_offset));
    }

    inline groupSize& no_legs()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4) + sizeof(groupSize) + (no_lot_type_rules().numInGroup * 5));
    }

    typedef MDInstrumentDefinitionSpread29_Legs_Entry legs_t;
    legs_t& legs(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionSpread29_Legs_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionSpread29) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4) + sizeof(groupSize) + (no_lot_type_rules().numInGroup * 5) + sizeof(groupSize) + (18*entry_offset));
    }
  } __attribute__ ((packed));

  struct SecurityStatus30
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch.
    SecurityGroup security_group; // Security Group
    Asset asset; // Product Code within Security Group specified
    Int32NULL security_id; // If this tag is present, 35=f message is sent for the instrument
    LocalMktDate trade_date; // Trade Session Date
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event
    SecurityTradingStatus security_trading_status; // Identifies the trading status applicable to the instrument or Security Group
    HaltReason halt_reason; // Identifies the reason for the status change
    SecurityTradingEvent security_trading_event; // Identifies an additional event or a rule related to the status
  } __attribute__ ((packed));

  struct MDIncrementalRefreshBook32_MDEntries_Entry
  {
    PRICENULL md_entry_px; // Market Data entry price
    Int32NULL md_entry_size; // Market Data entry size
    Int32 security_id; // Security ID
    uInt32 rpt_seq; // Market Data entry sequence number per instrument update
    Int32NULL number_of_orders; // In Book entry - aggregate number of orders at given price level
    uInt8 md_price_level; // Aggregate book level
    MDUpdateAction md_update_action; //  Market Data update action
    MDEntryTypeBook md_entry_type; // Market Data entry type
    char padding[5];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshBook32
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event

    inline groupSize& no_md_entries()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshBook32));
    }

    typedef MDIncrementalRefreshBook32_MDEntries_Entry md_entries_t;
    md_entries_t& md_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDIncrementalRefreshBook32_MDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshBook32) + sizeof(groupSize) + (32*entry_offset));
    }
    char padding[2];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshDailyStatistics33_MDEntries_Entry
  {
    PRICENULL md_entry_px; // Market Data entry price
    Int32NULL md_entry_size; // Market Data entry size
    Int32 security_id; // Security ID 
    uInt32 rpt_seq; // Market Data entry sequence number per instrument update
    LocalMktDate trading_reference_date; // Indicates trade session date corresponding to a statistic entry
    SettlPriceType settl_price_type; // Bitmap field of eight Boolean type indicators representing settlement price type
    MDUpdateAction md_update_action; // Market Data update action
    MDEntryTypeDailyStatistics md_entry_type; // Market Data entry type
    char padding[7];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshDailyStatistics33
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event

    inline groupSize& no_md_entries()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshDailyStatistics33));
    }

    typedef MDIncrementalRefreshDailyStatistics33_MDEntries_Entry md_entries_t;
    md_entries_t& md_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDIncrementalRefreshDailyStatistics33_MDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshDailyStatistics33) + sizeof(groupSize) + (32*entry_offset));
    }
    char padding[2];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshLimitsBanding34_MDEntries_Entry
  {
    PRICENULL high_limit_price; // Upper price threshold for the instrument
    PRICENULL low_limit_price; // Lower price threshold for the instrument
    PRICENULL max_price_variation; // Differential static value for price banding
    Int32 security_id; // Security ID 
    uInt32 rpt_seq; // MD Entry sequence number per instrument update
    //static const int8_t md_update_action = 0: // Market Data entry update action. In order to delete banding value, high or low limit, the deleted price field is populated with a NULL 
    //static const char * md_entry_type = "g": // Market Data entry type   
  } __attribute__ ((packed));

  struct MDIncrementalRefreshLimitsBanding34
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event

    inline groupSize& no_md_entries()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshLimitsBanding34));
    }

    typedef MDIncrementalRefreshLimitsBanding34_MDEntries_Entry md_entries_t;
    md_entries_t& md_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDIncrementalRefreshLimitsBanding34_MDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshLimitsBanding34) + sizeof(groupSize) + (32*entry_offset));
    }
    char padding[2];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshSessionStatistics35_MDEntries_Entry
  {
    PRICE md_entry_px; // Market Data entry price
    Int32 security_id; // Security ID 
    uInt32 rpt_seq; // MD Entry sequence number per instrument update
    OpenCloseSettlFlag open_close_settl_flag; // Flag describing IOP and Open Price entries
    MDUpdateAction md_update_action; // Market Data update action 
    MDEntryTypeStatistics md_entry_type; // Market Data entry type   
    char padding[5];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshSessionStatistics35
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event

    inline groupSize& no_md_entries()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshSessionStatistics35));
    }

    typedef MDIncrementalRefreshSessionStatistics35_MDEntries_Entry md_entries_t;
    md_entries_t& md_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDIncrementalRefreshSessionStatistics35_MDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshSessionStatistics35) + sizeof(groupSize) + (24*entry_offset));
    }
    char padding[2];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshTrade36_MDEntries_Entry
  {
    PRICE md_entry_px; // Trade price
    Int32 md_entry_size; // Trade quantity
    Int32 security_id; // Security ID
    uInt32 rpt_seq; // Market Data entry sequence number per instrument update
    Int32NULL number_of_orders; // Total number of real orders per instrument that participated in the trade
    Int32 trade_id; // Unique Trade ID per instrument and Trading Date
    AggressorSide aggressor_side; // Indicates aggressor side in the trade, if value is 0 then there is no aggressor
    MDUpdateAction md_update_action; // Market Data update action
    //static const char * md_entry_type = "2": // Market Data entry type
    char padding[2];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshTrade36
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event

    inline groupSize& no_md_entries()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshTrade36));
    }

    typedef MDIncrementalRefreshTrade36_MDEntries_Entry md_entries_t;
    md_entries_t& md_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDIncrementalRefreshTrade36_MDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshTrade36) + sizeof(groupSize) + (32*entry_offset));
    }
    char padding[2];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshVolume37_MDEntries_Entry
  {
    Int32 md_entry_size; // Cumulative traded volume
    Int32 security_id; // Security ID
    uInt32 rpt_seq; // Market Data entry sequence number per instrument update
    MDUpdateAction md_update_action; // Market Data update action
    //static const char * md_entry_type = "e": // Electronic Volume entry provides cumulative session trade volume updated with the event
    char padding[3];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshVolume37
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event

    inline groupSize& no_md_entries()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshVolume37));
    }

    typedef MDIncrementalRefreshVolume37_MDEntries_Entry md_entries_t;
    md_entries_t& md_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDIncrementalRefreshVolume37_MDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshVolume37) + sizeof(groupSize) + (16*entry_offset));
    }
    char padding[2];
  } __attribute__ ((packed));

  struct SnapshotFullRefresh38_MDEntries_Entry
  {
    PRICENULL md_entry_px; // Market Data entry price
    Int32NULL md_entry_size; // Market Data entry quantity
    Int32NULL number_of_orders; // Aggregate number of orders at the given price level
    Int8NULL md_price_level; // Aggregate book position
    LocalMktDate trading_reference_date; // Indicates the date of trade session corresponding to a statistic entry
    OpenCloseSettlFlag open_close_settl_flag; // Flag describing  Open Price entry
    SettlPriceType settl_price_type; // Bitmap field of eight Boolean type indicators representing settlement price type
    MDEntryType md_entry_type; // Market Data entry type
  } __attribute__ ((packed));

  struct SnapshotFullRefresh38
  {
    uInt32 last_msg_seq_num_processed; // Sequence number of the last Incremental feed packet processed. This value is used to synchronize the snapshot loop with the real-time feed
    uInt32 tot_num_reports; // Total number of messages replayed in the loop
    Int32 security_id; // Security ID
    uInt32 rpt_seq; // Sequence number of the last Market Data entry processed for the instrument
    uInt64 transact_time; // Timestamp of the last event security participated in, sent as number of nanoseconds since Unix epoch
    uInt64 last_update_time; // UTC Date and time of last Security Definition add, update or delete on a given Market Data channel
    LocalMktDate trade_date; // Trade session date sent as number of days since Unix epoch
    SecurityTradingStatus md_security_trading_status; // Identifies the current trading state of the instrument
    PRICENULL high_limit_price; // Upper price threshold for the instrument
    PRICENULL low_limit_price; // Lower price threshold for the instrument
    PRICENULL max_price_variation; // Differential value for price banding

    inline groupSize& no_md_entries()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(SnapshotFullRefresh38));
    }

    typedef SnapshotFullRefresh38_MDEntries_Entry md_entries_t;
    md_entries_t& md_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<SnapshotFullRefresh38_MDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(SnapshotFullRefresh38) + sizeof(groupSize) + (22*entry_offset));
    }
  } __attribute__ ((packed));

  struct QuoteRequest39_RelatedSym_Entry
  {
    Symbol symbol; // Instrument Name or Symbol
    Int32 security_id; // Security ID
    Int32NULL order_qty; // Quantity requested
    Int8 quote_type; // Type of quote requested
    Int8NULL side; // Side requested
    char padding[2];
  } __attribute__ ((packed));

  struct QuoteRequest39
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch
    QuoteReqId quote_req_id; // Quote Request ID defined by the exchange
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event

    inline groupSize& no_related_sym()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(QuoteRequest39));
    }

    typedef QuoteRequest39_RelatedSym_Entry related_sym_t;
    related_sym_t& related_sym(unsigned int entry_offset)
    {
      return *reinterpret_cast<QuoteRequest39_RelatedSym_Entry*>(reinterpret_cast<char*>(this) + sizeof(QuoteRequest39) + sizeof(groupSize) + (32*entry_offset));
    }
    char padding[3];
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionOption41_Events_Entry
  {
    EventType event_type; // Code to represent the type of event
    uInt64 event_time; // Date and Time of instument Activation or Expiration event sent as number of nanoseconds since Unix epoch
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionOption41_MDFeedTypes_Entry
  {
    MDFeedType md_feed_type; // Describes a class of service for a given data feed. GBX- Real Book, GBI-Implied Book
    Int8 market_depth; // Book depth
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionOption41_InstAttrib_Entry
  {
    //static const int8_t inst_attrib_type = 24: // Instrument Eligibility Attributes
    InstAttribValue inst_attrib_value; // Bitmap field of 32 Boolean type Instrument eligibility flags
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionOption41_LotTypeRules_Entry
  {
    Int8 lot_type; // This tag is required to interpret the value in tag 1231-MinLotSize
    DecimalQty min_lot_size; // Minimum quantity accepted for order entry. If tag 1093-LotType=4, this value is the minimum quantity for order entry expressed in the applicable units, specified in tag 996-UnitOfMeasure, e.g. megawatts
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionOption41_Underlyings_Entry
  {
    Int32 underlying_security_id; // Unique Instrument ID as qualified by the exchange per tag 305-UnderlyingSecurityIDSource
    //static const char underlying_security_id_source = '8': // This value is always '8' for CME
    UnderlyingSymbol underlying_symbol; // Underlying Instrument Symbol (Contract Name)
  } __attribute__ ((packed));

  struct MDInstrumentDefinitionOption41
  {
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event
    uInt32NULL tot_num_reports; // Total number of instruments in the Replay loop. Used on Replay Feed only 
    SecurityUpdateAction security_update_action; // Last Security update action on Incremental feed, 'D' or 'M' is used when a mid-week deletion or modification (i.e. extension) occurs
    uInt64 last_update_time; // Timestamp of when the instrument was last added, modified or deleted
    SecurityTradingStatus md_security_trading_status; // Identifies the current state of the instrument. The data is available in the Instrument Replay feed only 
    Int16 appl_id; // The channel ID as defined in the XML Configuration file
    uInt8 market_segment_id; // Identifies the market segment, populated for all CME Globex instruments
    uInt8 underlying_product; // Indicates the product complex
    SecurityExchange security_exchange; // Exchange used to identify a security
    SecurityGroup security_group; // Security Group Code 
    Asset asset; // The underlying asset code also known as Product Code
    Symbol symbol; // Instrument Name or Symbol. Previously used as Instrument Group Code 
    Int32 security_id; // Unique Instrument ID
    //static const char security_id_source = '8': // Identifies class or source of tag 48-SecurityID value
    SecurityType security_type; // Security Type
    CFICode cfi_code; // ISO standard instrument categorization code
    PutOrCall put_or_call; // Indicates whether an option instrument is a put or call
    MaturityMonthYear maturity_month_year; // This field provides the actual calendar date for contract maturity
    Currency currency; // Identifies currency used for price
    PRICENULL strike_price; // Strike Price for an option instrument
    Currency strike_currency; // Currency in which the StrikePrice is denominated
    Currency settl_currency; // Identifies currency used for settlement, if different from trade price currency
    PRICENULL min_cab_price; // Defines cabinet price for outright options products
    CHAR match_algorithm; // Matching algorithm
    uInt32 min_trade_vol; // The minimum trading volume for a security.
    uInt32 max_trade_vol; // The maximum trading volume for a security.
    PRICENULL min_price_increment; // Minimum constant tick for the instrument
    PRICENULL min_price_increment_amount; // Monetary value equivalent to the minimum price fluctuation
    FLOAT display_factor; // Contains the multiplier to convert the CME Globex display price to the conventional price
    Int8NULL tick_rule; // VTT code referencing variable tick table 
    uInt8NULL main_fraction; // Price Denominator of Main Fraction
    uInt8NULL sub_fraction; // Price Denominator of Sub Fraction
    uInt8NULL price_display_format; // Number of decimals in fractional display price
    UnitOfMeasure unit_of_measure; // Unit of measure for the products' original contract size. This will be populated for all products listed on CME Globex
    PRICENULL unit_of_measure_qty; // This field contains the contract size for each instrument. Used in combination with tag 996-UnitofMeasure
    PRICENULL trading_reference_price; // Reference price - the most recently available Settlement whether it be Theoretical, Preliminary or a Final Settle of the session
    SettlPriceType settl_price_type; // Bitmap field of eight Boolean type indicators representing settlement price type
    Int32NULL cleared_volume; // The total cleared volume of instrument traded during the prior trading session
    Int32NULL open_interest_qty; // The total open interest for the market at the close of the prior trading session.
    PRICENULL low_limit_price; // Allowable low limit price for the trading day 
    PRICENULL high_limit_price; // Allowable high limit price for the trading day
    UserDefinedInstrument user_defined_instrument; // User-defined instruments flag

    inline groupSize& no_events()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41));
    }

    typedef MDInstrumentDefinitionOption41_Events_Entry events_t;
    events_t& events(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionOption41_Events_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41) + sizeof(groupSize) + (9*entry_offset));
    }

    inline groupSize& no_md_feed_types()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41) + sizeof(groupSize) + (no_events().numInGroup * 9));
    }

    typedef MDInstrumentDefinitionOption41_MDFeedTypes_Entry md_feed_types_t;
    md_feed_types_t& md_feed_types(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionOption41_MDFeedTypes_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (4*entry_offset));
    }

    inline groupSize& no_inst_attrib()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4));
    }

    typedef MDInstrumentDefinitionOption41_InstAttrib_Entry inst_attrib_t;
    inst_attrib_t& inst_attrib(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionOption41_InstAttrib_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (4*entry_offset));
    }

    inline groupSize& no_lot_type_rules()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4));
    }

    typedef MDInstrumentDefinitionOption41_LotTypeRules_Entry lot_type_rules_t;
    lot_type_rules_t& lot_type_rules(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionOption41_LotTypeRules_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4) + sizeof(groupSize) + (5*entry_offset));
    }

    inline groupSize& no_underlyings()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4) + sizeof(groupSize) + (no_lot_type_rules().numInGroup * 5));
    }

    typedef MDInstrumentDefinitionOption41_Underlyings_Entry underlyings_t;
    underlyings_t& underlyings(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDInstrumentDefinitionOption41_Underlyings_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDInstrumentDefinitionOption41) + sizeof(groupSize) + (no_events().numInGroup * 9) + sizeof(groupSize) + (no_md_feed_types().numInGroup * 4) + sizeof(groupSize) + (no_inst_attrib().numInGroup * 4) + sizeof(groupSize) + (no_lot_type_rules().numInGroup * 5) + sizeof(groupSize) + (24*entry_offset));
    }
  } __attribute__ ((packed));

  struct MDIncrementalRefreshTradeSummary42_MDEntries_Entry
  {
    PRICE md_entry_px; // Trade price
    Int32 md_entry_size; // Consolidated trade quantity
    Int32 security_id; // Security ID as defined by CME
    uInt32 rpt_seq; // Sequence number per instrument update
    Int32NULL number_of_orders; // The total number of real orders per instrument that participated in a match step within a match event
    AggressorSide aggressor_side; // Indicates which side is the aggressor or if there is no aggressor
    MDUpdateAction md_update_action; // Market Data update action
    //static const char * md_entry_type = "2": // Market Data entry type
    char padding[6];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshTradeSummary42_OrderIDEntries_Entry
  {
    uInt64 order_id; // Unique order identifier as assigned by the exchange
    Int32 last_qty; // Quantity bought or sold on this last fill
    char padding[4];
  } __attribute__ ((packed));

  struct MDIncrementalRefreshTradeSummary42
  {
    uInt64 transact_time; // Start of event processing time in number of nanoseconds since Unix epoch
    MatchEventIndicator match_event_indicator; // Bitmap field of eight Boolean type indicators reflecting the end of updates for a given Globex event

    inline groupSize& no_md_entries()
    {
       return *reinterpret_cast<groupSize*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshTradeSummary42));
    }

    typedef MDIncrementalRefreshTradeSummary42_MDEntries_Entry md_entries_t;
    md_entries_t& md_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDIncrementalRefreshTradeSummary42_MDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshTradeSummary42) + sizeof(groupSize) + (32*entry_offset));
    }

    inline groupSize8Byte& no_order_id_entries()
    {
       return *reinterpret_cast<groupSize8Byte*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshTradeSummary42) + sizeof(groupSize) + (no_md_entries().numInGroup * 32));
    }

    typedef MDIncrementalRefreshTradeSummary42_OrderIDEntries_Entry order_id_entries_t;
    order_id_entries_t& order_id_entries(unsigned int entry_offset)
    {
      return *reinterpret_cast<MDIncrementalRefreshTradeSummary42_OrderIDEntries_Entry*>(reinterpret_cast<char*>(this) + sizeof(MDIncrementalRefreshTradeSummary42) + sizeof(groupSize) + (no_md_entries().numInGroup * 32) + sizeof(groupSize8Byte) + (16*entry_offset));
    }
    char padding[2];
  } __attribute__ ((packed));
};

}
