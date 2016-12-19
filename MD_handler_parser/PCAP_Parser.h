#ifndef hf_core_Data_PCAP_Parser_h
#define hf_core_Data_PCAP_Parser_h

#include <Data/Handler.h>
#include <Data/LZO_Util.h>

#include <Logger/Logger.h>
#include <Util/Parameter.h>

namespace hf_core {
  using namespace std;
  
  class PCAP_Parser {
  public:
    
    void read(const string& filename, Handler* handler);
    void set_inside_file_name(const string& filename);
    
    PCAP_Parser();
    ~PCAP_Parser();
    
  private:
    
    size_t m_buf_size;
    
    FILE* m_file;
    bool  m_used_popen;
    
    bool m_nanoseconds_mode;
    
    hash_map<uint64_t, size_t> m_ip_to_context;

    string m_inside_file_name;
    
  };
  typedef boost::shared_ptr<PCAP_Parser> PCAP_ParserRP;
  
}

#endif /* ifndef hf_core_Data_PCAP_Parser_h */
