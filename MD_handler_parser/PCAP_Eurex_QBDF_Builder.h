#pragma once

#include <Data/QBDF_Builder.h>

namespace hf_core {

  class PCAP_Eurex_QBDF_Builder : public QBDF_Builder {
  public:
    PCAP_Eurex_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date)
    { }

  protected:
    int process_raw_file(const string& file_name);

  };

}
