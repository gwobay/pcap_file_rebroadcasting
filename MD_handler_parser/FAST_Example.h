#include "FAST_Decoder.h"


class ExampleFASTClass
{
public:
 
  class ExampleFASTClass_TestSequenceEntries_Element
  {
    private:
      uint32_t m_TestUint32Mandatory_seq;
      uint64_t m_TestUint64Mandatory_seq;
    public:
      void Reset_Dictionary(StringStateRepository& stringRepo)
      {
        // Follow the same pattern that we use in other Reset_Dictionary functions
      }

      void Parse(uint8_t* buf, PerParseStringRepository& perParseStringRepo)
      {
        // Follow the same pattern that we use in other Parse functions, except we don't have to parse the pmap since it's already given
        
        decode_field_mandatory<uint32_t>(buf, m_TestUint32Mandatory_seq);
        decode_field_mandatory<uint64_t>(buf, m_TestUint64Mandatory_seq);
        //etc
      } 

      // define all the same getter functions as usual 
  }; 

private:
  uint32_t m_TestUint32Mandatory;
  uint64_t m_TestUint64Mandatory;
  int32_t m_TestInt32Mandatory;
  FAST_String m_TestStringMandatory;
  FAST_ByteVector m_TestByteVectorMandatory;
  double m_TestDoubleMandatory;

  FAST_Nullable<uint32_t> m_TestUint32Optional;
  FAST_Nullable<uint64_t> m_TestUint64Optional;
  FAST_Nullable<int32_t> m_TestInt32Optional;
  FAST_Nullable<FAST_String> m_TestStringOptional;
  FAST_Nullable<FAST_ByteVector> m_TestByteVectorOptional;
  FAST_Nullable<double> m_TestDoubleOptional;

  // ToDo: should we move the _states to a separate class based on the template's dictionary?  Just in case two templates share a dictionary

  uint32_t m_TestUint32MandatoryConstant_state;
  uint64_t m_TestUint64MandatoryConstant_state;
  int32_t m_TestInt32MandatoryConstant_state;
  FAST_String m_TestStringMandatoryConstant_state;
  // ToDo: byteVector
  double m_TestDoubleMandatoryConstant_state;

  FAST_Nullable<uint32_t> m_TestUint32OptionalConstant_state;
  FAST_Nullable<uint64_t> m_TestUint64OptionalConstant_state;
  FAST_Nullable<int32_t> m_TestInt32OptionalConstant_state;
  FAST_Nullable<FAST_String> m_TestStringOptionalConstant_state;
  // ToDo: byteVector
  FAST_Nullable<double> m_TestDoubleOptionalConstant_state;

  uint32_t m_TestUint32MandatoryCopy_state;
  uint64_t m_TestUint64MandatoryCopy_state;
  int32_t m_TestInt32MandatoryCopy_state;
  FAST_String m_TestStringMandatoryCopy_state;
  // ToDo: byteVector
  double m_TestDoubleMandatoryCopy_state;

  FAST_Nullable<uint32_t> m_TestUint32OptionalCopy_state;
  FAST_Nullable<uint64_t> m_TestUint64OptionalCopy_state;
  FAST_Nullable<int32_t> m_TestInt32OptionalCopy_state;
  FAST_Nullable<FAST_String> m_TestStringOptionalCopy_state;
  // ToDo: byteVector
  FAST_Nullable<double> m_TestDoubleOptionalCopy_state;

  FAST_Defaultable<uint32_t> m_TestUint32MandatoryDefault_state;
  FAST_Defaultable<uint64_t> m_TestUint64MandatoryDefault_state;
  FAST_Defaultable<int32_t> m_TestInt32MandatoryDefault_state;
  FAST_Defaultable<FAST_String> m_TestStringMandatoryDefault_state;
  // ToDo: byteVector
  FAST_Defaultable<double> m_TestDoubleMandatoryDefault_state;

  FAST_Defaultable<FAST_Nullable<uint32_t> > m_TestUint32OptionalDefault_state;
  FAST_Defaultable<FAST_Nullable<uint64_t> > m_TestUint64OptionalDefault_state;
  FAST_Defaultable<FAST_Nullable<int32_t> > m_TestInt32OptionalDefault_state;
  FAST_Defaultable<FAST_Nullable<FAST_String> > m_TestStringOptionalDefault_state;
  // ToDo: byteVector
  FAST_Defaultable<FAST_Nullable<double> > m_TestDoubleOptionalDefault_state;

  uint32_t m_TestUint32MandatoryIncrement_state;
  uint64_t m_TestUint64MandatoryIncrement_state;
  int32_t m_TestInt32MandatoryIncrement_state;

  FAST_Nullable<uint32_t> m_TestUint32OptionalIncrement_state;
  FAST_Nullable<uint64_t> m_TestUint64OptionalIncrement_state;
  FAST_Nullable<int32_t> m_TestInt32OptionalIncrement_state;

  uint32_t m_TestUint32MandatoryDelta_state;
  uint64_t m_TestUint64MandatoryDelta_state;
  int32_t m_TestInt32MandatoryDelta_state;
  FAST_String m_TestStringMandatoryDelta_state;
  // ToDo: byteVector
  double m_TestDoubleMandatoryDelta_state;

  FAST_Nullable<uint32_t> m_TestUint32OptionalDelta_state;
  FAST_Nullable<uint64_t> m_TestUint64OptionalDelta_state;
  FAST_Nullable<int32_t> m_TestInt32OptionalDelta_state;
  FAST_Nullable<FAST_String> m_TestStringOptionalDelta_state;
  // ToDo: byteVector
  FAST_Nullable<double> m_TestDoubleOptionalDelta_state;

  FAST_Sequence<ExampleFASTClass_TestSequenceEntries_Element, 50> m_TestSequenceMandatory;

public:
  void Reset_Dictionary(StringStateRepository& stringRepo)
  {
    // Assign starting values
    // Will need to have space local to this instance for the states

    m_TestUint32MandatoryConstant_state = 12;
    m_TestUint64MandatoryConstant_state = 34;
    m_TestInt32MandatoryConstant_state = -56;
    stringRepo.assign_next_const_string_state<FAST_String>("seven_eight", m_TestStringMandatoryConstant_state);
    // ToDo: byteVector
    m_TestDoubleMandatoryConstant_state = 910.0;

    m_TestUint32OptionalConstant_state.assign_valid_value(12);
    m_TestUint64OptionalConstant_state.assign_valid_value(34);
    m_TestInt32OptionalConstant_state.assign_valid_value(-56);
    stringRepo.assign_next_const_string_state<FAST_String>("seven_eight", m_TestStringOptionalConstant_state.value);
    // ToDo: byteVector
    m_TestDoubleOptionalConstant_state.assign_valid_value(910.0);

    stringRepo.allocate_next_string_state<FAST_String>(m_TestStringMandatoryCopy_state, 100); // 100 is arbitrarily selected string lenth
    // ToDo: byteVector

    stringRepo.allocate_next_string_state<FAST_String>(m_TestStringOptionalCopy_state.value, 100);
    // ToDo: byteVector

    // Set all default optional values back to false
    
    m_TestUint32MandatoryDefault_state.default_value = 12;
    m_TestUint64MandatoryDefault_state.default_value = 34;
    m_TestInt32MandatoryDefault_state.default_value = -56;
    stringRepo.assign_next_const_string_state<FAST_String>("seven_eight", m_TestStringMandatoryDefault_state.default_value);
    stringRepo.allocate_next_string_state<FAST_String>(m_TestStringMandatoryDefault_state.non_default_value, 100);
    // ToDo: byteVector
    m_TestDoubleMandatoryDefault_state.default_value = 910.0;

    m_TestUint32OptionalDefault_state.default_value.has_value = false;
    m_TestUint64OptionalDefault_state.default_value.has_value = false;
    m_TestInt32OptionalDefault_state.default_value.has_value = false;
    m_TestStringOptionalDefault_state.default_value.has_value = false;
    stringRepo.allocate_next_string_state<FAST_String>(m_TestStringOptionalDefault_state.non_default_value.value, 100);
    // ToDo: byteVector
    m_TestDoubleOptionalDefault_state.default_value.has_value = false;

    // For any delta operators with values specified, reset them here
    m_TestUint32MandatoryDelta_state = 1234;

    // If the value has a default:
    m_TestUint32OptionalDelta_state.assign_valid_value(12);
    // If the value doesn't have a default
    m_TestUint64OptionalDelta_state.has_value = false;
    m_TestInt32OptionalDelta_state.assign_valid_value(34);
    // Need to do this regardless of whether there's a default:
    stringRepo.allocate_next_string_state<FAST_String>(m_TestStringOptionalDelta_state.value, 100);
    // Only do this if there's not a default:
    m_TestStringOptionalDelta_state.has_value = false;
    // ToDo: byteVector
    //FAST_Nullable<double> m_TestDoubleOptionalDelta_state;
  

    stringRepo.assign_next_const_string_state<FAST_String>("default_value", m_TestStringMandatoryDelta_state);
  }

  void Parse(uint8_t* buf, PerParseStringRepository& perParseStringRepo)
  {
    Pmap pmap(buf);
    // ToDo: If there are no pmap entries, don't skip this
    skip_field_mandatory(buf); // get past the pmap

    decode_field_mandatory<uint32_t>(buf, m_TestUint32Mandatory);
    decode_field_mandatory<uint64_t>(buf, m_TestUint64Mandatory);
    decode_field_mandatory<int32_t>(buf, m_TestInt32Mandatory);
    perParseStringRepo.read_string_mandatory(buf, m_TestStringMandatory);
    perParseStringRepo.read_byte_vector_mandatory(buf, m_TestByteVectorMandatory);
    decode_field_mandatory<double>(buf, m_TestDoubleMandatory);

    decode_field_optional<uint32_t>(buf, m_TestUint32Optional.value, m_TestUint32Optional.has_value);
    decode_field_optional<uint64_t>(buf, m_TestUint64Optional.value, m_TestUint64Optional.has_value);
    decode_field_optional<int32_t>(buf, m_TestInt32Optional.value, m_TestInt32Optional.has_value);
    perParseStringRepo.read_string_optional(buf, m_TestStringOptional);
    perParseStringRepo.read_byte_vector_optional(buf, m_TestByteVectorOptional);
    decode_field_optional<double>(buf, m_TestDoubleOptional.value, m_TestDoubleOptional.has_value);

    m_TestUint32OptionalConstant_state.has_value = pmap.read_next_pmap_entry();
    m_TestUint64OptionalConstant_state.has_value = pmap.read_next_pmap_entry();
    m_TestInt32OptionalConstant_state.has_value = pmap.read_next_pmap_entry();
    m_TestStringOptionalConstant_state.has_value = pmap.read_next_pmap_entry();
    // ToDo: byteVector
    m_TestDoubleOptionalConstant_state.has_value = pmap.read_next_pmap_entry();

    decode_mandatory_copy<uint32_t>(buf, m_TestUint32MandatoryCopy_state, pmap);
    decode_mandatory_copy<uint64_t>(buf, m_TestUint64MandatoryCopy_state, pmap);
    decode_mandatory_copy<int32_t>(buf, m_TestInt32MandatoryCopy_state, pmap);
    decode_mandatory_copy<FAST_String>(buf, m_TestStringMandatoryCopy_state, pmap);
    // ToDo: byteVector
    decode_mandatory_copy<double>(buf, m_TestDoubleMandatoryCopy_state, pmap);
 
    decode_optional_copy<uint32_t>(buf, m_TestUint32OptionalCopy_state, pmap);
    decode_optional_copy<uint64_t>(buf, m_TestUint64OptionalCopy_state, pmap);
    decode_optional_copy<int32_t>(buf, m_TestInt32OptionalCopy_state, pmap);
    decode_optional_copy<FAST_String>(buf, m_TestStringOptionalCopy_state, pmap);
    // ToDo: byteVector
    decode_optional_copy<double>(buf, m_TestDoubleOptionalCopy_state, pmap);

    decode_mandatory_default<uint32_t>(buf, m_TestUint32MandatoryDefault_state, pmap);
    decode_mandatory_default<uint64_t>(buf, m_TestUint64MandatoryDefault_state, pmap);
    decode_mandatory_default<int32_t>(buf, m_TestInt32MandatoryDefault_state, pmap);
    decode_mandatory_default<FAST_String>(buf, m_TestStringMandatoryDefault_state, pmap);
    // ToDo: byteVector
    decode_mandatory_default<double>(buf, m_TestDoubleMandatoryDefault_state, pmap);
 
    decode_optional_default<uint32_t>(buf, m_TestUint32OptionalDefault_state, pmap);
    decode_optional_default<uint64_t>(buf, m_TestUint64OptionalDefault_state, pmap);
    decode_optional_default<int32_t>(buf, m_TestInt32OptionalDefault_state, pmap);
    decode_optional_default<FAST_String>(buf, m_TestStringOptionalDefault_state, pmap);
    // ToDo: byteVector
    decode_optional_default<double>(buf, m_TestDoubleOptionalDefault_state, pmap);

    decode_mandatory_increment<uint32_t>(buf, m_TestUint32MandatoryIncrement_state, pmap);
    decode_mandatory_increment<uint64_t>(buf, m_TestUint64MandatoryIncrement_state, pmap);
    decode_mandatory_increment<int32_t>(buf, m_TestInt32MandatoryIncrement_state, pmap);

    decode_optional_increment<uint32_t>(buf, m_TestUint32OptionalIncrement_state, pmap);
    decode_optional_increment<uint64_t>(buf, m_TestUint64OptionalIncrement_state, pmap);
    decode_optional_increment<int32_t>(buf, m_TestInt32OptionalIncrement_state, pmap);

    decode_mandatory_delta<uint32_t>(buf, m_TestUint32MandatoryDelta_state, perParseStringRepo);
    decode_mandatory_delta<uint64_t>(buf, m_TestUint64MandatoryDelta_state, perParseStringRepo);
    decode_mandatory_delta<int32_t>(buf, m_TestInt32MandatoryDelta_state, perParseStringRepo);
    decode_mandatory_delta<FAST_String>(buf, m_TestStringMandatoryDelta_state, perParseStringRepo);
    // ToDo: byteVector
    decode_mandatory_delta<double>(buf, m_TestDoubleMandatoryDelta_state, perParseStringRepo);
  
    decode_optional_delta<uint32_t>(buf, m_TestUint32OptionalDelta_state, perParseStringRepo);
    decode_optional_delta<uint64_t>(buf, m_TestUint64OptionalDelta_state, perParseStringRepo);
    decode_optional_delta<int32_t>(buf, m_TestInt32OptionalDelta_state, perParseStringRepo);
    decode_optional_delta<FAST_String>(buf, m_TestStringOptionalDelta_state, perParseStringRepo);
    // ToDo: byteVector
    decode_optional_delta<double>(buf, m_TestDoubleOptionalDelta_state, perParseStringRepo);
 
    decode_mandatory_sequence(buf, m_TestSequenceMandatory,  perParseStringRepo); 

    // Add some fields to skip

    /*
    m_NoTestSequenceEntries = decode_field_mandatory<uint32_t>(buf);
    for(unsigned int i = 0; i < m_NoTestSequenceEntries; ++i)
    {
      ExampleFASTClass_TestSequenceEntries_Element& current_element = m_TestSequenceEntries[i];
      current_element.Parse(buf, perParseStringRepo);
    }
    */
    // etc
  }

  // Mandatory
  const inline uint32_t TestUint32Mandatory()
  {
    return m_TestUint32Mandatory;
  }

  const inline uint64_t TestUint64Mandatory()
  {
    return m_TestUint64Mandatory;
  }

  const inline int32_t TestInt32Mandatory()
  {
    return m_TestInt32Mandatory;
  }

  const inline FAST_String& TestStringMandatory()
  {
    return m_TestStringMandatory;
  }

  const inline FAST_ByteVector& TestByteVectorMandatory()
  {
    return m_TestByteVectorMandatory;
  }

  const inline double TestDoubleMandatory()
  {
    return m_TestDoubleMandatory;
  }
 
  // Optional
  const inline FAST_Nullable<uint32_t>& TestUint32Optional()
  {
    return m_TestUint32Optional;
  }

  const inline FAST_Nullable<uint64_t>& TestUint64Optional()
  {
    return m_TestUint64Optional;
  }

  const inline FAST_Nullable<int32_t>& TestInt32Optional()
  {
    return m_TestInt32Optional;
  }

  const inline FAST_Nullable<FAST_String>& TestStringOptional()
  {
    return m_TestStringOptional;
  }

  const inline FAST_Nullable<FAST_ByteVector>& TestByteVectorOptional()
  {
    return m_TestByteVectorOptional;
  }

  const inline FAST_Nullable<double>& TestDoubleOptional()
  {
    return m_TestDoubleOptional;
  }

  // Mandatory Constant
  const inline uint32_t TestUint32MandatoryConstant()
  {
    return m_TestUint32MandatoryConstant_state;
  }

  const inline uint64_t TestUint64MandatoryConstant()
  {
    return m_TestUint64MandatoryConstant_state;
  }

  const inline int32_t TestInt32MandatoryConstant()
  {
    return m_TestInt32MandatoryConstant_state;
  }

  const inline FAST_String& TestStringMandatoryConstant()
  {
    return m_TestStringMandatoryConstant_state;
  }

  const inline double TestDoubleMandatoryConstant()
  {
    return m_TestDoubleMandatoryConstant_state;
  }

  // Optional Constant
  const inline FAST_Nullable<uint32_t>& TestUint32OptionalConstant()
  {
    return m_TestUint32OptionalConstant_state;
  }

  const inline FAST_Nullable<uint64_t>& TestUint64OptionalConstant()
  {
    return m_TestUint64OptionalConstant_state;
  }

  const inline FAST_Nullable<int32_t>& TestInt32OptionalConstant()
  {
    return m_TestInt32OptionalConstant_state;
  }

  const inline FAST_Nullable<FAST_String>& TestStringOptionalConstant()
  {
    return m_TestStringOptionalConstant_state;
  }

  const inline FAST_Nullable<double>& TestDoubleOptionalConstant()
  {
    return m_TestDoubleOptionalConstant_state;
  }

  // Mandatory Copy
  const inline uint32_t TestUint32MandatoryCopy()
  {
    return m_TestUint32MandatoryCopy_state;
  }

  const inline uint64_t TestUint64MandatoryCopy()
  {
    return m_TestUint64MandatoryCopy_state;
  }

  const inline int32_t TestInt32MandatoryCopy()
  {
    return m_TestInt32MandatoryCopy_state;
  }

  const inline FAST_String& TestStringMandatoryCopy()
  {
    return m_TestStringMandatoryCopy_state;
  }

  const inline double TestDoubleMandatoryCopy()
  {
    return m_TestDoubleMandatoryCopy_state;
  }

  // Optional Copy
  const inline FAST_Nullable<uint32_t>& TestUint32OptionalCopy()
  {
    return m_TestUint32OptionalCopy_state;
  }

  const inline FAST_Nullable<uint64_t>& TestUint64OptionalCopy()
  {
    return m_TestUint64OptionalCopy_state;
  }

  const inline FAST_Nullable<int32_t>& TestInt32OptionalCopy()
  {
    return m_TestInt32OptionalCopy_state;
  }

  const inline FAST_Nullable<FAST_String>& TestStringOptionalCopy()
  {
    return m_TestStringOptionalCopy_state;
  }

  const inline FAST_Nullable<double>& TestDoubleOptionalCopy()
  {
    return m_TestDoubleOptionalCopy_state;
  }

  // Mandatory Default
  const inline uint32_t TestUint32MandatoryDefault()
  {
    return m_TestUint32MandatoryDefault_state.current_value();
  }

  const inline uint64_t TestUint64MandatoryDefault()
  {
    return m_TestUint64MandatoryDefault_state.current_value();
  }

  const inline int32_t TestInt32MandatoryDefault()
  {
    return m_TestInt32MandatoryDefault_state.current_value();
  }

  const inline FAST_String& TestStringMandatoryDefault()
  {
    return m_TestStringMandatoryDefault_state.current_value();
  }

  const inline double TestDoubleMandatoryDefault()
  {
    return m_TestDoubleMandatoryDefault_state.current_value();
  }

  // Optional Default
  const inline FAST_Nullable<uint32_t>& TestUint32OptionalDefault()
  {
    return m_TestUint32OptionalDefault_state.current_value();
  }

  const inline FAST_Nullable<uint64_t>& TestUint64OptionalDefault()
  {
    return m_TestUint64OptionalDefault_state.current_value();
  }

  const inline FAST_Nullable<int32_t>& TestInt32OptionalDefault()
  {
    return m_TestInt32OptionalDefault_state.current_value();
  }

  const inline FAST_Nullable<FAST_String>& TestStringOptionalDefault()
  {
    return m_TestStringOptionalDefault_state.current_value();
  }

  const inline FAST_Nullable<double>& TestDoubleOptionalDefault()
  {
    return m_TestDoubleOptionalDefault_state.current_value();
  }

  // Mandatory Increment
  const inline uint32_t TestUint32MandatoryIncrement()
  {
    return m_TestUint32MandatoryIncrement_state;
  }

  const inline uint64_t TestUint64MandatoryIncrement()
  {
    return m_TestUint64MandatoryIncrement_state;
  }

  const inline int32_t TestInt32MandatoryIncrement()
  {
    return m_TestInt32MandatoryIncrement_state;
  }

  // Optional Increment
  const inline FAST_Nullable<uint32_t> TestUint32OptionalIncrement()
  {
    return m_TestUint32OptionalIncrement_state;
  }

  const inline FAST_Nullable<uint64_t> TestUint64OptionalIncrement()
  {
    return m_TestUint64OptionalIncrement_state;
  }

  const inline FAST_Nullable<int32_t> TestInt32OptionalIncrement()
  {
    return m_TestInt32OptionalIncrement_state;
  }

  // Mandatory Delta
  const inline uint32_t TestUint32MandatoryDelta()
  {
    return m_TestUint32MandatoryDelta_state;
  }

  const inline uint64_t TestUint64MandatoryDelta()
  {
    return m_TestUint64MandatoryDelta_state;
  }

  const inline int32_t TestInt32MandatoryDelta()
  {
    return m_TestInt32MandatoryDelta_state;
  }

  const inline FAST_String& TestStringMandatoryDelta()
  {
    return m_TestStringMandatoryDelta_state;
  }

  const inline double TestDoubleMandatoryDelta()
  {
    return m_TestDoubleMandatoryDelta_state;
  }


  // Optional Delta
  const inline FAST_Nullable<uint32_t>& TestUint32OptionalDelta()
  {
    return m_TestUint32OptionalDelta_state;
  }

  const inline FAST_Nullable<uint64_t>& TestUint64OptionalDelta()
  {
    return m_TestUint64OptionalDelta_state;
  }

  const inline FAST_Nullable<int32_t>& TestInt32OptionalDelta()
  {
    return m_TestInt32OptionalDelta_state;
  }

  const inline FAST_Nullable<FAST_String>& TestStringOptionalDelta()
  {
    return m_TestStringOptionalDelta_state;
  }

  const inline FAST_Nullable<double>& TestDoubleOptionalDelta()
  {
    return m_TestDoubleOptionalDelta_state;
  }

  const inline FAST_Sequence<ExampleFASTClass_TestSequenceEntries_Element, 50>& TestSequenceMandatory()
  {
    return m_TestSequenceMandatory;
  }

};


class Example_Fast_Decoder
{
private:
  ExampleFASTClass m_ExampleFASTClass;
  // ...

  PerParseStringRepository m_per_parse_string_repo;
  StringStateRepository m_string_repo;

public:
  void Reset_Dictionaries()
  {
    m_ExampleFASTClass.Reset_Dictionary(m_string_repo);
    //...

    m_string_repo.reset();
  }

  ExampleFASTClass& Parse_ExampleFASTClass(uint8_t* buf)
  {
    m_per_parse_string_repo.reset();
    m_ExampleFASTClass.Parse(buf, m_per_parse_string_repo);
    return m_ExampleFASTClass;
  }

};
