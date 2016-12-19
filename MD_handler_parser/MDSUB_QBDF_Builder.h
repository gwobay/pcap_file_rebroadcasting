#ifndef hf_data_MDSUB_QBDF_Builder_H
#define hf_data_MDSUB_QBDF_Builder_H

#include <map>
#include <string>
#include <vector>
#include <Data/Binary_Data.h>
#include <MD/Quote.h>
#include <Data/QBDF_Builder.h>

typedef unsigned long ulong;

namespace hf_core {
  using namespace std;

  BOOST_ENUM_VALUES(MDSource, const char*,
                    (Unknown)("unknown")
                    (CTA)("CTA")
                    (UTP)("UTP")
                    (ITCH4)("ITCH4")
                    (ArcaBook)("ArcaBook")
                    (ArcaTrade)("ArcaTrade")
                    (NYSEBest)("NYSEBest")
                    (NYSEImbalance)("NYSEImbalance")
                    (NYSEOpenBook)("NYSEOpenBook")
                    (NYSETrade)("NYSETrade")
                    (OMXBX4)("OMXBX4")
                    (PITCHMC)("PITCHMC")
                    );

  class MDSUB_QBDF_Builder : public QBDF_Builder {
  public:
    MDSUB_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date) {}

  protected:
    int process_raw_file(const string& file_name);
    int process_trade_file(const string& file_name) { return 0; }
    int process_quote_file(const string& file_name) { return 0; }
    MDSource m_md_source;
  };
}

#endif /* hf_data_MDSUB_QBDF_Builder_H */
