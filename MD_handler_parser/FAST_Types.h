#pragma once

#include <string>
#include <boost/lexical_cast.hpp>
#include <Data/Handler.h>


struct FAST_String
{
  char* value;
  size_t len;

  FAST_String() :
    value(NULL), len(0)
  {
  }

  FAST_String(char* value_, size_t len_) :
    value(value_), len(len_)
  {
  }

  inline void assign(char* value_, size_t len_)
  {
    value = value_;
    len = len_;
  }

  std::string to_string()
  {
    std::string ret(value, len);
    return ret;
  }
};

static inline double exponent_to_double(int32_t exponent)
{
  switch(exponent)
  {
    case -16: return 1e-16;
    case -15: return 1e-15;
    case -14: return 1e-14;
    case -13: return 1e-13;
    case -12: return 1e-12;
    case -11: return 1e-11;
    case -10: return 0.0000000001;
    case -9:  return 0.000000001;
    case -8:  return 0.00000001;
    case -7:  return 0.0000001;
    case -6:  return 0.000001;
    case -5:  return 0.00001;
    case -4:  return 0.0001;
    case -3:  return 0.001;
    case -2:  return 0.01;
    case -1:  return 0.1;
    case 0:   return 1.0;
    case 1:   return 10.0;
    case 2:   return 100.0;
    case 3:   return 1000.0;
    case 4:   return 10000.0;
    case 5:   return 100000.0;
    case 6:   return 1000000.0;
    case 7:   return 10000000.0;
    case 8:   return 100000000.0;
    case 9:   return 1000000000.0;
    case 10:  return 10000000000.0;
    case 11:  return 1e11;
    case 12:  return 1e12;
    case 13:  return 1e13;
    case 14:  return 1e14;
    case 15:  return 1e15;
    case 16:  return 1e16;
    default: throw std::range_error("exponent should be an integer between -16 and 16, not " + boost::lexical_cast<std::string>(exponent));
  }
}

struct FAST_Decimal
{
  int32_t exponent;
  int64_t mantissa;
  
  double to_double()
  {
    return static_cast<double>(mantissa) * exponent_to_double(exponent);
  }
};

struct FAST_ByteVector
{
  uint8_t* value;
  size_t len;

  FAST_ByteVector() :
    value(NULL), len(0)
  {
  }

  FAST_ByteVector(uint8_t* value_, size_t len_) :
    value(value_), len(len_)
  {
  }

  inline void assign(uint8_t* value_, size_t len_)
  {
    value = value_;
    len = len_;
  }

  inline uint32_t to_uint32()
  {
    uint32_t ret = 0;
    for(size_t i = 0; i < len; ++i)
    {
      ret = ret << 8;
      ret |= value[i];
    }
    return ret;
  }
};

template<typename T>
struct FAST_Nullable
{
   T value;
   bool has_value;

   void assign_valid_value(T v)
   {
     value = v;
     has_value = true;
   }
};

template<>
struct FAST_Nullable<FAST_String>
{
  FAST_String value;
  bool has_value;

  void assign_valid_value(FAST_String v)
  {
    value.value = v.value;
    value.len = v.len;
  }
};

template<>
struct FAST_Nullable<FAST_ByteVector>
{
  FAST_ByteVector value;
  bool has_value;

  void assign_valid_value(FAST_ByteVector v)
  {
    value.value = v.value;
    value.len = v.len;
  }
};

template <typename T>
struct FAST_Defaultable
{
  T default_value;
  T non_default_value;
  bool is_default; 

  inline T& current_value()
  {
    if (is_default)
    {
      return default_value;
    }
    else
    {
      return non_default_value;
    }
  }
};

template <typename T, size_t MAX_NUM_ENTRIES>
struct FAST_Sequence
{
  T entries[MAX_NUM_ENTRIES];
  uint32_t len;
  static const size_t max_num_entries = MAX_NUM_ENTRIES;
};

template <typename T>
struct FAST_Group
{
  bool has_value;
  T value;
};
