#pragma once

#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <Data/Handler.h>
#include "FAST_Types.h"

template<size_t BIT_OFFSET>
inline static bool bit_is_set(uint8_t c)
{
   return (c & (0x01 << BIT_OFFSET)) != 0;
}

inline static bool bit_is_set_var(int offset, uint32_t v)
{
   return (v & (0x01 << offset)) != 0;
}

inline static bool single_bit_is_set_var(int offset, uint8_t v)
{
   return (v & (0x01 << offset)) != 0;
}

template<size_t BIT_OFFSET>
inline static uint8_t remove_bit(uint8_t c)
{
   return c & (~(0x01 << BIT_OFFSET));
}

template<size_t BIT_OFFSET>
inline static uint8_t set_bit(uint8_t c)
{
   return c | (0x01 << BIT_OFFSET);
}


inline static void print_bits(uint32_t v)
{
   for(int i = sizeof(v)*8 - 1; i >= 0; --i)
   {
      bool bitIsSet = bit_is_set_var(i, v);
      std::cout << (bitIsSet ? "1" : "0");
   }
}

inline static void print_hex(uint8_t*buf, int count)
{
  uint16_t* cur = reinterpret_cast<uint16_t*>(buf);
  for(int i = 0; i < count; ++i)
  {
    std::cout << std::hex << *cur;
    ++cur;
  }
  std::cout << std::endl;
}

inline static void print_chars(uint8_t*buf, int count)
{
  char* c = reinterpret_cast<char*>(buf);
  for(int i = 0; i < count; ++i)
  {
    std::cout << *c;
    ++c;
  }
  std::cout << std::endl;
}

inline static void print_field(uint8_t* buf)
{
   while(true)
   {
     for(int i = 7; i >= 0; --i)
     {
       bool bitIsSet = single_bit_is_set_var(static_cast<size_t>(i), *buf);
       std::cout << (bitIsSet ? "1" : "0");
     }
     std::cout << " ";
     if(bit_is_set<7>(*buf))
     {
       break;
     }
     ++buf;
   }
}

// These are just the empty templates
template<typename T>
inline void decode_field(uint8_t*& buf, T& value)
{
}

template<typename T>
inline void decode_field_mandatory(uint8_t*& buf, T& value)
{
}

template<typename T>
inline void decode_field_optional(uint8_t*& buf, T& value, bool& hasValue)
{
  hasValue = false;
}

inline static int32_t decode_int32_negative(uint8_t*& buf)
{
   uint8_t currentBit = *buf;
   uint32_t ret(0xffffffff);
   while(true)
   {
      ret = (ret << 7) | static_cast<uint32_t>(remove_bit<7>(currentBit));
      //cout << "ret=";
      //print_bits(ret);
      //cout << endl;
      buf += 1;
      if(bit_is_set<7>(currentBit))
      {
         break;
      }
      currentBit = *buf;
   }
   return static_cast<int32_t>(ret);
}

inline static int32_t decode_int32_positive(uint8_t*& buf)
{
   uint8_t currentBit = *buf;
   currentBit = remove_bit<6>(currentBit); // Remove the sign bit
   uint32_t ret(0);
   while(true)
   {
      ret = (ret << 7) | static_cast<uint32_t>(remove_bit<7>(currentBit));
      //cout << "ret=";
      //print_bits(ret);
      //cout << endl;
      buf += 1;
      if(bit_is_set<7>(currentBit))
      {
         break;
      }
      currentBit = *buf;
   }
   return static_cast<int32_t>(ret);

}

inline static int32_t decode_int32(uint8_t*& buf)
{
   uint8_t currentBit = *buf;
   if(bit_is_set<6>(currentBit))
   {
     return decode_int32_negative(buf);
   }
   return decode_int32_positive(buf);
}

template<>
inline void decode_field_mandatory<int32_t>(uint8_t*& buf, int32_t& value)
{
   value = decode_int32(buf);
}

template<>
inline void decode_field_optional<int32_t>(uint8_t*& buf, int32_t& value, bool& hasValue)
{
   value = decode_int32(buf);
   if(value == 0)
   {
     hasValue = false;
   }
   else
   {
     hasValue = true;
     if(value > 0)
     {
       --value;
     }
   }
}


inline static int64_t decode_int64_negative(uint8_t*& buf)
{
   uint8_t currentBit = *buf;
   uint64_t ret(0xffffffffffffffff);
   while(true)
   {
      ret = (ret << 7) | static_cast<uint64_t>(remove_bit<7>(currentBit));
      //cout << "ret=";
      //print_bits(ret);
      //cout << endl;
      buf += 1;
      if(bit_is_set<7>(currentBit))
      {
         break;
      }
      currentBit = *buf;
   }
   return static_cast<int64_t>(ret);
}

inline static int64_t decode_int64_positive(uint8_t*& buf)
{
   uint8_t currentBit = *buf;
   currentBit = remove_bit<6>(currentBit); // Remove the sign bit
   uint64_t ret(0);
   while(true)
   {
      ret = (ret << 7) | static_cast<uint64_t>(remove_bit<7>(currentBit));
      //cout << "ret=";
      //print_bits(ret);
      //cout << endl;
      buf += 1;
      if(bit_is_set<7>(currentBit))
      {
         break;
      }
      currentBit = *buf;
   }
   return static_cast<int64_t>(ret);

}

inline static int64_t decode_int64(uint8_t*& buf)
{
   uint8_t currentBit = *buf;
   if(bit_is_set<6>(currentBit))
   {
     return decode_int64_negative(buf);
   }
   return decode_int64_positive(buf);
}

template<>
inline void decode_field_mandatory<int64_t>(uint8_t*& buf, int64_t& value)
{
   value = decode_int64(buf);
}

template<>
inline void decode_field_optional<int64_t>(uint8_t*& buf, int64_t& value, bool& hasValue)
{
   value = decode_int64(buf);
   if(value == 0)
   {
     hasValue = false;
   }
   else
   {
     hasValue = true;
     if(value > 0)
     {
       --value;
     }
   }
}

inline uint32_t decode_uint32(uint8_t*& buf)
{
   uint8_t currentBit = *buf;
   uint32_t ret(0);
   while(true)
   {
      ret = (ret << 7) | static_cast<uint32_t>(remove_bit<7>(currentBit));
      //cout << "ret=";
      //print_bits(ret);
      //cout << endl;
      buf += 1;
      if(bit_is_set<7>(currentBit))
      {
         break;
      }
      currentBit = *buf;
   }
   return ret;
}

template<>
inline void decode_field_mandatory<uint32_t>(uint8_t*& buf, uint32_t& value)
{
   value = decode_uint32(buf);
}

template<>
inline void decode_field_optional<uint32_t>(uint8_t*& buf, uint32_t& value, bool& hasValue)
{
   value = decode_uint32(buf);
   if(value > 0)
   {
      hasValue = true;
      --value;
   }
   else
   {
     hasValue = false;
   }
}

inline static uint64_t decode_uint64(uint8_t*& buf)
{
   uint8_t currentBit = *buf;
   uint64_t ret(0);
   while(true)
   {
      ret = (ret << 7) | static_cast<uint64_t>(remove_bit<7>(currentBit));
      //cout << "ret=";
      //print_bits(ret);
      //cout << endl;
      buf += 1;
      if(bit_is_set<7>(currentBit))
      {
         break;
      }
      currentBit = *buf;
   }
   return ret;
}

template<>
inline void decode_field_mandatory<uint64_t>(uint8_t*& buf, uint64_t& value)
{
   value = decode_uint64(buf);
}

template<>
inline void decode_field_optional<uint64_t>(uint8_t*& buf, uint64_t& value, bool& hasValue)
{
   value = decode_uint64(buf);
   if(value > 0)
   {
      hasValue = true;
      --value;
   }
   else
   {
     hasValue = false;
   }
}

inline static void decode_string(uint8_t*& buf, size_t& len, char* outputBuffer)
{
   uint8_t currentBit = *buf;
   while(true)
   {
      buf += 1;
      outputBuffer[len] = remove_bit<7>(currentBit);
      ++len;
      if(bit_is_set<7>(currentBit))
      {
         break;
      }
      currentBit = *buf;
   }
   outputBuffer[len] = '\0';
   //cout << "decode_string: " << string(outputBuffer, len) << " len=" << len << endl;
}

template<>
inline void decode_field_optional<FAST_String>(uint8_t*& buf, FAST_String& value, bool& hasValue)
{
  value.len = 0;
  decode_string(buf, value.len, value.value);
  hasValue = (value.len > 0 || buf[0] != '\0');
}

template<>
inline void decode_field_mandatory<FAST_String>(uint8_t*& buf, FAST_String& value)
{
  value.len = 0;
  decode_string(buf, value.len, value.value);
}

inline static void decode_byte_vector(uint8_t*& buf, size_t len, uint8_t* outputBuffer)
{
   for(size_t i = 0; i < len; ++i)
   {
      outputBuffer[i] = *buf;
      buf += 1;
   }
}

template<>
inline void decode_field_optional<FAST_ByteVector>(uint8_t*& buf, FAST_ByteVector& value, bool& hasValue)
{
  value.len = static_cast<size_t>(remove_bit<7>(*buf));
  buf += 1;
  if(value.len == 0)
  {
    hasValue = false;
    return;
  }
  --(value.len); // decrement since field is optional
  hasValue = true;
  decode_byte_vector(buf, value.len, value.value);
}

template<>
inline void decode_field_mandatory<FAST_ByteVector>(uint8_t*& buf, FAST_ByteVector& value)
{
  value.len = static_cast<size_t>(remove_bit<7>(*buf));
  buf += 1;
  decode_byte_vector(buf, value.len, value.value);
}

template<>
inline void decode_field_mandatory<double>(uint8_t*& buf, double& value)
{
  //cout << "decode_field_mandatory" << endl;
  int32_t exponent(0);
  decode_field_mandatory<int32_t>(buf, exponent);
  //cout << "exponent=" << exponent << endl;
  int32_t mantissa(0); 
  decode_field_mandatory<int32_t>(buf, mantissa);
  //cout << "mantissa=" << mantissa << endl;
  value = static_cast<double>(mantissa) * exponent_to_double(exponent);
}


template<>
inline void decode_field_optional<double>(uint8_t*& buf, double& value, bool& hasValue)
{
  int32_t exponent(0);
  //cout << "decode_double_optional" << endl;
  decode_field_optional<int32_t>(buf, exponent, hasValue);
  //cout << "exponent hasValue=" << hasValue;
  //cout << "exponent=" << exponent << endl;
  if(!hasValue)
  {
    value = NAN;
    return;
  }
  int32_t mantissa;
  //bool mantissaHasValue(false);
  //decode_field_optional<int32_t>(buf, mantissa, mantissaHasValue);
  decode_field_mandatory<int32_t>(buf, mantissa);
  //cout << "mantissa=" << mantissa << endl;
  value = static_cast<double>(mantissa) * exponent_to_double(exponent);
  //cout << "value=" << value;
}

/*
template<>
inline void decode_field_optional<double>(uint8_t*& buf, double& value, bool& hasValue)
{
  int32_t exponent(0);
  bool exponentHasValue, mantissaHasValue;
  decode_field_optional<int32_t>(buf, exponent, exponentHasValue);
  int32_t mantissa;
  decode_field_optional<int32_t>(buf, mantissa, mantissaHasValue);
  if(exponentHasValue && mantissaHasValue)
  {
    hasValue = true;
    value = static_cast<double>(mantissa) * exponent_to_double(exponent);
  }
  else
  {
    hasValue = false;
    value = NAN;
  }
}*/

#define RETURN_CONST_STRING(S, LEN)\
  static char* val = S;\
  static size_t len = LEN;\
  static  FAST_String ret(val, len);\
  return ret;

struct Pmap
{
  uint8_t* current_pmap_byte;
  size_t current_pmap_bit;
  bool passed_end_of_pmap;

  Pmap(uint8_t* buf)
    : passed_end_of_pmap(false)
  {
    current_pmap_byte = buf;
    //cout << "Pmap(): buf=" << static_cast<uint32_t>(*buf) << endl;
    current_pmap_bit = 6;
    //print_pmap();
  }

  inline bool read_next_pmap_entry()
  {
    if(passed_end_of_pmap)
    {
      return false;
    }
    bool ret = single_bit_is_set_var(current_pmap_bit, *current_pmap_byte);
    if(current_pmap_bit == 0)
    {
      current_pmap_bit = 6;
      if(bit_is_set<7>(*current_pmap_byte))
      {
        passed_end_of_pmap = true;
      } 
      else
      {
        ++current_pmap_byte;
      }
    }
    else
    {
      --current_pmap_bit;
    }
    //++current_pmap_bit;
    //if(current_pmap_bit == 8)
    //{
    //  current_pmap_bit = 0;
    //  ++current_pmap_byte;
    //}
    return ret;
  }

  void print_pmap()
  {
    std::cout << "pmap=";
    uint8_t* c = current_pmap_byte;
    while(true)
    {
      for(int i = 7; i >= 0; --i)
      {
        std::cout << (single_bit_is_set_var(static_cast<size_t>(i), *c) ? "1" : "0");
      }
      if(bit_is_set<7>(*c))
      {
        break;
      }
      ++c;
      std::cout << " ";
    }
    std::cout << std::endl;
  }
};

template<typename T>
inline void FAST_copy(T& dest, T& src)
{
  dest = src;
}

template<>
inline void FAST_copy<FAST_String>(FAST_String& dest, FAST_String& src)
{
  dest.len = src.len;
  memcpy(dest.value, src.value, src.len);
}

// Will need to namespace this whole thing
struct PerParseStringRepository
{
  PerParseStringRepository()
  {
  }

  char string_buffer[2024];
  size_t current_string_buffer_offset;

  inline void reset()
  {
    current_string_buffer_offset = 0;
  }

  inline void read_string_mandatory(uint8_t*& buf, FAST_String& dest)
  {
    dest.value = &(string_buffer[current_string_buffer_offset]);
    decode_field_mandatory<FAST_String>(buf, dest);
    current_string_buffer_offset += dest.len;
  }

  inline void read_string_optional(uint8_t*& buf, FAST_Nullable<FAST_String>& dest)
  {
    dest.value.value = &(string_buffer[current_string_buffer_offset]);
    decode_field_optional<FAST_String>(buf, dest.value, dest.has_value);
    current_string_buffer_offset += dest.value.len;
  }

  inline void decode_mandatory_delta_string(uint8_t*& buf, FAST_String& state, FAST_String& dictionaryState)
  {
    decode_mandatory_delta_string(buf, dictionaryState);
    FAST_copy<FAST_String>(state, dictionaryState);
  }

  inline void decode_mandatory_delta_string(uint8_t*& buf, FAST_String& state)
  {
    //cout << "decode_mandatory_delta_string" << endl;
    int32_t delta_string_len(0);
    decode_field_mandatory<int32_t>(buf, delta_string_len);
    //cout << "delta_string_len=" << delta_string_len << endl;
    //cout << "state.len=" << state.len << endl;
    if(delta_string_len >= 0) // Append to right side
    {
      if(static_cast<uint32_t>(delta_string_len) > state.len)
      {
        // This should not happen
        //cout << "ERROR: got delta string subtract len " << delta_string_len << " > state.len " << state.len << endl;
      }
      else
      {
        size_t appendLen(0);
        char* outputBuffer = state.value + (state.len - delta_string_len);
        decode_string(buf, appendLen, outputBuffer);
        //cout << "appendLen=" << appendLen << endl;
        state.len += static_cast<int32_t>(appendLen) - delta_string_len;
      }
    }
    else // Append to left side
    {
      ++delta_string_len; // Need to increment by one to allow for negativo zero
      if(delta_string_len <= -static_cast<int32_t>(state.len))
      {
        // This should not happen
        //cout << "ERROR: got delta string subtract len " << delta_string_len << " <= state.len " << static_cast<int32_t>(state.len) << endl;
      }
      else
      {     
        FAST_String delta(NULL, 0);
        read_string_mandatory(buf, delta); // actually it's not even necessary to increment the buffer offset here since we're throwing this string away immediately
        if(delta.len == static_cast<size_t>(-delta_string_len))
        {  // Replace the whole string
           memcpy(state.value, delta.value, delta.len);
           // state.len is unchanged
        }
        else if(delta.len > static_cast<size_t>(-delta_string_len))
        {  // Replace 
           size_t moveSize = delta.len - static_cast<size_t>(-delta_string_len);
           memcpy(state.value + moveSize, state.value, (state.len - moveSize)); 
        }
        else
        {
           size_t moveSize = static_cast<size_t>(-delta_string_len) - delta.len;
           memcpy(state.value + delta.len, state.value + static_cast<size_t>(-delta_string_len), moveSize);
        }
        state.len = (state.len + delta.len) - static_cast<uint32_t>(-delta_string_len);
      }
    }
  }

  inline void read_byte_vector_mandatory(uint8_t*& buf, FAST_ByteVector& dest)
  {
    dest.value = reinterpret_cast<uint8_t*>(&(string_buffer[current_string_buffer_offset]));
    decode_field_mandatory<FAST_ByteVector>(buf, dest);
    current_string_buffer_offset += dest.len;
  }

  inline void read_byte_vector_optional(uint8_t*& buf, FAST_Nullable<FAST_ByteVector>& dest)
  {
      dest.value.value = reinterpret_cast<uint8_t*>(&(string_buffer[current_string_buffer_offset]));
      decode_field_optional<FAST_ByteVector>(buf, dest.value, dest.has_value);
      current_string_buffer_offset += dest.value.len;
  }

  inline void decode_optional_delta_string(uint8_t*& buf, FAST_Nullable<FAST_String>& state)
  {
    if (state.has_value) {
      std::cout << "what's going on?" << std::endl;
    FAST_Nullable<int32_t> delta_string_len0;
    decode_field_optional<int32_t>(buf, delta_string_len0.value, delta_string_len0.has_value);

    if (delta_string_len0.has_value) {
      int32_t delta_string_len = delta_string_len0.value;
      if(delta_string_len >= 0) // Append to right side
      {
        if(static_cast<uint32_t>(delta_string_len) > state.value.len)
        {
          // This should not happen
          std::cout << "ERROR: got optional delta string subtract len " << delta_string_len << " > state.value.len " << state.value.len << std::endl;
        }
        else
        {
          size_t appendLen(0);
          char* outputBuffer = state.value.value + (state.value.len - delta_string_len);
          decode_string(buf, appendLen, outputBuffer);
          //cout << "appendLen=" << appendLen << endl;
          state.value.len += static_cast<int32_t>(appendLen) - delta_string_len;
        }
      }
      else // Append to left side
      {
        ++delta_string_len; // Need to increment by one to allow for negativo zero
        if(delta_string_len <= -static_cast<int32_t>(state.value.len))
        {
          // This should not happen
          std::cout << "ERROR: got optional delta string subtract len " << delta_string_len << " <= state.value.len " << static_cast<int32_t>(state.value.len) << std::endl;
        }
        else
        {     
          FAST_String delta(NULL, 0);
          read_string_mandatory(buf, delta); // actually it's not even necessary to increment the buffer offset here since we're throwing this string away immediately
          if(delta.len == static_cast<size_t>(-delta_string_len))
          {  // Replace the whole string
             memcpy(state.value.value, delta.value, delta.len);
             // state.value.len is unchanged
          }
          else if(delta.len > static_cast<size_t>(-delta_string_len))
          {  // Replace 
             size_t moveSize = delta.len - static_cast<size_t>(-delta_string_len);
             memcpy(state.value.value + moveSize, state.value.value, (state.value.len - moveSize)); 
          }
          else
          {
             size_t moveSize = static_cast<size_t>(-delta_string_len) - delta.len;
             memcpy(state.value.value + delta.len, state.value.value + static_cast<size_t>(-delta_string_len), moveSize);
          }
          state.value.len = (state.value.len + delta.len) - static_cast<uint32_t>(-delta_string_len);
        }
      }
    }
    }
  }

/*
  inline void read_string_optional_default(uint8_t*& buf, FAST_Nullable<FAST_String>& dest, FAST_Nullable<FAST_String>& defaultValue)
  {
    read_string_optional(buf, dest.value);
    if(!dest.has_value)
    {
      dest.assign_valid_value(defaultValue);
    }
  }
*/
};
// ToDo: rename "mandatory" and "optional" skip functions to "skip_field" and "skip_field_pmap"
inline static void skip_field_mandatory(uint8_t*& buf)
{
   uint8_t currentBit = *buf;
   while(true)
   {
      buf += 1;
      if(bit_is_set<7>(currentBit))
      {
         break;
      }
      currentBit = *buf;
   }
}

inline static void skip_field_optional(uint8_t*& buf, Pmap& pmap)
{
  if(pmap.read_next_pmap_entry())
  {
    skip_field_mandatory(buf);
  }
}


template<typename T>
inline void decode_mandatory_copy(uint8_t*& buf, T& state, Pmap& pmap)
{
  //cout << "decode_mandatory_copy_start" << endl;
  if(pmap.read_next_pmap_entry())
  {
    //cout << "has entry" << endl;
    decode_field_mandatory<T>(buf, state);
  }
  else
  {
    //cout << "no entry" << endl;
  }
  //cout << "decode_mandatory_copy_end" << endl;
  // ToDo: else check to make sure that the state has been initialized
  // }
  //
}

template<typename T>
inline void decode_mandatory_copy(uint8_t*& buf, T& state, Pmap& pmap, T& dictionaryState)
{
  decode_mandatory_copy<T>(buf, dictionaryState, pmap);
  FAST_copy(state, dictionaryState);
}

template<typename T>
inline void decode_optional_copy(uint8_t*& buf, FAST_Nullable<T>& state, Pmap& pmap)
{
  if(pmap.read_next_pmap_entry())
  {
    //cout << "has entry" << endl;
    decode_field_optional<T>(buf, state.value, state.has_value);
  }
  else
  {
    //cout << "does not have entry" << endl;
  }
  // ToDo: else check to make sure that the state has been initialized
}

template<typename T>
inline void decode_optional_copy(uint8_t*& buf, FAST_Nullable<T>& state, Pmap& pmap, FAST_Nullable<T>& dictionaryState)
{
  decode_optional_copy<T>(buf, dictionaryState, pmap);
  state.has_value = dictionaryState.has_value;
  FAST_copy<T>(state.value, dictionaryState.value);
}

template<typename T>
inline static void decode_mandatory_default(uint8_t*& buf, FAST_Defaultable<T>& state, Pmap& pmap)
{
    if(pmap.read_next_pmap_entry())
    {
       state.is_default = false;
       decode_field_mandatory<T>(buf, state.non_default_value);
    }
    else
    {
       state.is_default = true;
    }
}

// Not sure we need dictioanry states for defaultables
template<typename T>
inline static void decode_mandatory_default(uint8_t*& buf, FAST_Defaultable<T>& state, Pmap& pmap, FAST_Defaultable<T>& dictionaryState)
{
  decode_mandatory_default<T>(buf, dictionaryState, pmap);
  state.is_default = dictionaryState.is_default;
  if(!state.is_default)
  {
    FAST_copy<T>(state.non_default_value, dictionaryState.non_default_value);
  }
}

template<typename T>
inline static void decode_optional_default(uint8_t*& buf, FAST_Defaultable<FAST_Nullable<T> >& state, Pmap& pmap)
{
    if(pmap.read_next_pmap_entry())
    {
       state.is_default = false;
       decode_field_optional<T>(buf, state.non_default_value.value, state.non_default_value.has_value);
    }
    else
    {
       state.is_default = true;
    }
}

template<typename T>
inline static void decode_optional_default(uint8_t*& buf, FAST_Defaultable<FAST_Nullable<T> >& state, Pmap& pmap, FAST_Defaultable<FAST_Nullable<T> >& dictionaryState)
{
  decode_optional_default<T>(buf, dictionaryState, pmap);
  state.is_default = dictionaryState.is_default;
  if(!state.is_default)
  {
    state.non_default_value.has_value = dictionaryState.non_default_value.has_value;
    if(state.non_default_value.has_value)
    {
      FAST_copy<T>(state.non_default_value.value, dictionaryState.non_default_value.value);
    }
  }
}

template<typename T>
inline void decode_mandatory_increment(uint8_t*& buf, T& state, Pmap& pmap)
{
  if(pmap.read_next_pmap_entry())
  {
    decode_field_mandatory<T>(buf, state);
  }
  else
  {
    // ToDo: check to make sure that the state has been initialized
    ++state;
  }
}

template<typename T>
inline void decode_mandatory_increment(uint8_t*& buf, T& state, Pmap& pmap, T& dictionaryState)
{
  decode_mandatory_increment<T>(buf, dictionaryState, pmap);
  FAST_copy<T>(state, dictionaryState);
}

template<typename T>
inline void decode_optional_increment(uint8_t*& buf, FAST_Nullable<T>& state, Pmap& pmap)
{
  if(pmap.read_next_pmap_entry())
  {
    decode_field_optional<T>(buf, state.value, state.has_value);
  }
  else
  {
    ++(state.value);;
  }
}

template<typename T>
inline void decode_optional_increment(uint8_t*& buf, FAST_Nullable<T>& state, Pmap& pmap, FAST_Nullable<T>& dictionaryState)
{
  decode_optional_increment(buf, dictionaryState, pmap);
  state.has_value = dictionaryState.has_value;
  if(state.has_value)
  {
    FAST_copy<T>(state.value, dictionaryState.value);
  }
}

template<typename T>
inline void decode_mandatory_delta(uint8_t*& buf, T& state, PerParseStringRepository& perParseStringRepo)
{
   int64_t delta;
   decode_field_mandatory<int64_t>(buf, delta);
   //cout << "decode_mandatory_delta: delta=" << delta << " state=" << state << endl;
   state += static_cast<T>(delta);
}

template<>
inline void decode_mandatory_delta<double>(uint8_t*& buf, double& state, PerParseStringRepository& perParseStringRepo)
{
   double delta;
   decode_field_mandatory<double>(buf, delta);
   state += delta;
}

template<>
inline void decode_mandatory_delta<FAST_String>(uint8_t*& buf, FAST_String& state, PerParseStringRepository& perParseStringRepo)
{
  perParseStringRepo.decode_mandatory_delta_string(buf, state);
}

template<typename T>
inline void decode_mandatory_delta(uint8_t*& buf, T& state, PerParseStringRepository& perParseStringRepo, T& dictionaryState)
{
  decode_mandatory_delta<T>(buf, dictionaryState, perParseStringRepo);
  FAST_copy<T>(state, dictionaryState);
}

inline void decode_mandatory_decimal_delta(uint8_t*& buf, double& state, PerParseStringRepository& perParseStringRepo, FAST_Decimal& dictionaryState)
{
   decode_mandatory_delta<int32_t>(buf, dictionaryState.exponent, perParseStringRepo);
   decode_mandatory_delta<int64_t>(buf, dictionaryState.mantissa, perParseStringRepo);
   state = dictionaryState.to_double();
}

template<typename T>
inline void decode_optional_delta(uint8_t*& buf, FAST_Nullable<T>& state, PerParseStringRepository& perParseStringRepo)
{
   FAST_Nullable<int64_t> delta;
   decode_field_optional<int64_t>(buf, delta.value, delta.has_value);
   //cout << "decode_optional_delta<T>: delta=" << delta.value << " has_value=" << delta.has_value << " state.has_value=" << state.has_value << "state.value=" << state.value << endl;
   if(delta.has_value)
   {
     //cout << "decode_optional_delta: delta.has_value=true. delta.value=" << delta.value << endl;
     if(!state.has_value)
     {
       //cout << "state.has_value was false" << endl;
       state.has_value = true;
       state.value = static_cast<T>(delta.value);
     }
     else
     {
       //cout << "state.value=" << state.value << endl;
       state.value = static_cast<T>(static_cast<int64_t>(state.value) + delta.value);
     }
     // ToDo: what if state has no value?
   }
   else
   {
     state.value = 0;
     state.has_value = false;
   }
}


template<>
inline void decode_optional_delta<double>(uint8_t*& buf, FAST_Nullable<double>& state, PerParseStringRepository& perParseStringRepo)
{
   FAST_Nullable<double> delta;
   decode_field_optional<double>(buf, delta.value, delta.has_value);
   if(delta.has_value)
   {
     if(!state.has_value)
     {
       state.has_value = true;
       state.value = delta.value;
     }
     else
     {
       state.value += delta.value;
     }
     // ToDo: what if state has no value?
   }
   else
   {
     state.value = 0;
     state.has_value = false;
   }
}

inline void decode_optional_decimal_delta(uint8_t*& buf, FAST_Nullable<double>& state, PerParseStringRepository& perParseStringRepo, FAST_Decimal& dictionaryState)
{
   int32_t exponentDelta;
   decode_field_optional<int32_t>(buf, exponentDelta, state.has_value);
   if(state.has_value)
   {
     dictionaryState.exponent += exponentDelta;
   }
   else
   {
     dictionaryState.exponent = 0;
     dictionaryState.mantissa = 0;
     return;
   }
   int64_t mantissaDelta;
   decode_field_mandatory<int64_t>(buf, mantissaDelta);
   dictionaryState.mantissa += mantissaDelta;
   state.value = dictionaryState.to_double();
}

template<>
inline void decode_optional_delta<FAST_String>(uint8_t*& buf, FAST_Nullable<FAST_String>& state, PerParseStringRepository& perParseStringRepo)
{
  perParseStringRepo.decode_optional_delta_string(buf, state);
}

template<typename T>
inline void decode_optional_delta(uint8_t*& buf, FAST_Nullable<T>& state, PerParseStringRepository& perParseStringRepo, FAST_Nullable<T>& dictionaryState)
{
  decode_optional_delta<T>(buf, dictionaryState, perParseStringRepo);
  state.has_value = dictionaryState.has_value;
  FAST_copy<T>(state.value, dictionaryState.value);
}

static inline void throw_seqlen_error(std::string function_name, uint32_t len, size_t max_entries) {
  std::stringstream ss;
  ss << function_name;
  ss << " sequence.len > sequence.max_num_entries ";
  ss << len;
  ss << " ";
  ss << max_entries;
  throw std::runtime_error(ss.str());
}

template<typename T>
inline void decode_mandatory_sequence(uint8_t*& buf, T& sequence, PerParseStringRepository& perParseStringRepo)
{
  decode_field_mandatory<uint32_t>(buf, sequence.len);
  if(sequence.len > sequence.max_num_entries) {
    throw_seqlen_error(__PRETTY_FUNCTION__, sequence.len, sequence.max_num_entries);
  }
  
  //cout << "mandatory seq len=" << sequence.len << endl;
  for(uint32_t i = 0; i < sequence.len; ++i)
  {
    //cout << "i=" << i << endl;
    sequence.entries[i].Parse(buf, perParseStringRepo);
  }
}

template<typename T, typename D>
inline void decode_mandatory_sequence(uint8_t*& buf, T& sequence, PerParseStringRepository& perParseStringRepo, D& globalDictionary)
{
  decode_field_mandatory<uint32_t>(buf, sequence.len);
  if(sequence.len > sequence.max_num_entries) {
    throw_seqlen_error(__PRETTY_FUNCTION__, sequence.len, sequence.max_num_entries);
  }
  
  //cout << "mandatory seq len (w dict)=" << sequence.len << endl;
//  if(sequence.len == 0)
//  {
//    sequence.len = 1;
//  }
//  
/*
  if(sequence.len == 45)
  {
    cout << "len=45, which can't be right" << endl;
  }
*/
  for(uint32_t i = 0; i < sequence.len; ++i)
  {
    //cout << "i=" << i << endl;
    sequence.entries[i].Parse(buf, perParseStringRepo, globalDictionary);
  }
}

template<typename T, typename D>
inline void decode_optional_sequence(uint8_t*& buf, T& sequence, PerParseStringRepository& perParseStringRepo, D& globalDictionary)
{
  bool hasValue(false);
  decode_field_optional<uint32_t>(buf, sequence.len, hasValue);
  if(hasValue)
  {
    if(sequence.len > sequence.max_num_entries) {
      throw_seqlen_error(__PRETTY_FUNCTION__, sequence.len, sequence.max_num_entries);
    }
    
    for(uint32_t i = 0; i < sequence.len; ++i)
    {
      sequence.entries[i].Parse(buf, perParseStringRepo, globalDictionary);
    }
  }
  else
  {
    sequence.len = 0;
  }
}

template<typename T>
inline void decode_optional_sequence(uint8_t*& buf, T& sequence, PerParseStringRepository& perParseStringRepo)
{
  bool hasValue(false);
  decode_field_optional<uint32_t>(buf, sequence.len, hasValue);
  if(hasValue)
  {
    if(sequence.len > sequence.max_num_entries) {
      throw_seqlen_error(__PRETTY_FUNCTION__, sequence.len, sequence.max_num_entries);
    }
    
    for(uint32_t i = 0; i < sequence.len; ++i)
    {
      sequence.entries[i].Parse(buf, perParseStringRepo);
    }
  }
  else
  {
    sequence.len = 0;
  }
}

template<typename T, typename D>
inline void decode_optional_group(uint8_t*& buf, T& group, Pmap& pmap, PerParseStringRepository& perParseStringRepo, D& globalDictionary)
{
  if(pmap.read_next_pmap_entry())
  {
    group.has_value = true;
    group.value.Parse(buf, perParseStringRepo, globalDictionary);
  }
  else
  {
    group.has_value = false;
  }
}

inline void assign_string(FAST_String& dest, const char* val, size_t len)
{
  dest.len = len;
  memcpy(dest.value, val, len);
}

class StringStateRepository
{
public:
  StringStateRepository()
    : string_state_buffer_current_ptr(0)
  {

  }

  template<typename T>
  void assign_next_const_string_state(const char* value, T& dest)
  {
    strcpy(&(string_state_buffer[string_state_buffer_current_ptr]), value);
    size_t len = strlen(value);
    dest.assign(&(string_state_buffer[string_state_buffer_current_ptr]), len);
    string_state_buffer_current_ptr += len;
  }

  template<typename FAST_STRING_TYPE>
  void allocate_next_string_state(FAST_STRING_TYPE& dest, size_t max_len)
  {
    dest.assign(&(string_state_buffer[string_state_buffer_current_ptr]), 0);
    string_state_buffer_current_ptr += max_len;
  }

  template<typename FAST_STRING_TYPE>
  void allocate_next_string_state_with_default(FAST_STRING_TYPE& dest, size_t max_len, const char* value)
  {
    strcpy(&(string_state_buffer[string_state_buffer_current_ptr]), value);
    size_t len = strlen(value);
    dest.assign(&(string_state_buffer[string_state_buffer_current_ptr]), len);
    string_state_buffer_current_ptr += max_len;
  }

  void reset()
  {
    string_state_buffer_current_ptr = 0;
  }

private:
  char string_state_buffer[4096]; // Some max size
  size_t string_state_buffer_current_ptr;
};

// ToDo: make subclass state repository which has all the same functions and a reference to the base repository,
// then just forwards calls to the base repository


inline static void print_field_possibilities(uint8_t* buf)
{
  uint32_t uint_field(0);
  int64_t int_field(0);
  static char s_field[128];
  uint32_t len;
  FAST_String s(s_field, len);
  decode_field_mandatory<uint32_t>(buf, uint_field);
  decode_field_mandatory<int64_t>(buf, int_field);
  bool string_too_long = true;
  if(uint_field < 128)
  {
    decode_field_mandatory<FAST_String>(buf, s);
    string_too_long = false;
  }
  std::cout << "Field:" << std::endl;
  std::cout << "  uint:   " << uint_field << std::endl;
  std::cout << "  int:    " << int_field << std::endl;
  if(!string_too_long)
  {
    std::cout << "  string: " << s.to_string() << std::endl;
  }
}


class FAST_Decoder_Tester
{
public:
  void run_tests();
};
