#ifndef hf_core_Data_QRec_h
#define hf_core_Data_QRec_h

#include <Util/Time.h>

#include <string>

namespace hf_core  {
  using namespace hf_tools;
  
  const char record_header_title[] = "qRec";
  
  struct record_header {
    char     title[4];
    uint64_t ticks;
    uint64_t context;
    uint16_t len;
    
    record_header(const hf_tools::Time t, size_t context, size_t len) : ticks(t.ticks()), context(context), len(len) {
      memcpy(title, record_header_title, 4); // don't wish to include the NUL
    }
    record_header() { }
    
  } __attribute__ ((packed));


  const char long_record_header_title[] = "qRel";
  
  struct long_record_header {
    char     title[4];
    uint64_t ticks;
    uint64_t context;
    uint32_t len;
    
    long_record_header(const hf_tools::Time t, size_t context, size_t len) : ticks(t.ticks()), context(context), len(len) {
      memcpy(title, long_record_header_title, 4); // don't wish to include the NUL
    }
    long_record_header() { }
    
  } __attribute__ ((packed));
  
  
  struct Dump_Filters {
    uint64_t    order_id; // If 0, no filter
    std::string symbol;   // If empty, no filter
    bool        skip_heartbeats;
    Time        start;
    
    Dump_Filters() :
      order_id(0),
      skip_heartbeats(false)
    {}
  };
  
}

#endif
