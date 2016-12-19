#ifndef hf_core_Data_PCAP_TSE_FLEX_QBDF_Builder_h
#define hf_core_Data_PCAP_TSE_FLEX_QBDF_Builder_h

#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;
  
  class PCAP_TSE_FLEX_QBDF_Builder : public QBDF_Builder {
  public:
    
    PCAP_TSE_FLEX_QBDF_Builder(Application* app, const string& date) :
      QBDF_Builder(app, date) {}
      
  protected:
    int process_raw_file(const string& file_name);
    
  };
}

#endif /* ifndef hf_core_Data_PCAP_TSE_FLEX_QBDF_Builder_h */
