#ifndef hf_data_CValueMDCSV_QBDF_Builder_H
#define hf_data_CValueMDCSV_QBDF_Builder_H

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <map>
#include <glob.h>

#include <Data/QBDF_Builder.h>
#include <Data/Wombat_Util.h>
#include <MD/Quote.h>

namespace hf_core {
  using namespace std;

  class CValueMDCSV_QBDF_Builder : public QBDF_Builder {
  public:
    CValueMDCSV_QBDF_Builder(Application* app, const string& date);

  protected:
    int process_raw_file(const string& file_name) { return 0; }
    int process_trade_file(const string& file_name);
    int process_quote_file(const string& file_name);
  };
}

#endif /* hf_data_CValueMDCSV_QBDF_Builder_H */
