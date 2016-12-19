#ifndef hf_data_ReutersTokyo_QBDF_Builder_H
#define hf_data_ReutersTokyo_QBDF_Builder_H

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <map>
#include <glob.h>

#include <Data/QBDF_Builder.h>
#include <MD/Quote.h>

namespace hf_core {
  using namespace std;

  class ReutersTokyo_QBDF_Builder : public QBDF_Builder {
  public:
    ReutersTokyo_QBDF_Builder(Application* app, const string& date);

  protected:
    int process_raw_file(const string& file_name) { return 0; }
    int process_trade_file(const string& file_name);
    int process_quote_file(const string& file_name);

    struct level_pair {
      float price;
      int size;
      level_pair() {
        price = 0.0;
        size = 0;
      }
      level_pair(float _price, int _size) {
        price = _price;
        size = _size;
      }
    };

    //vector<map<float,int> > m_bid_levels;
    //vector<map<float,int> > m_ask_levels;

    vector<vector<level_pair> > m_bid_levels;
    vector<vector<level_pair> > m_ask_levels;
  };
}

#endif /* hf_data_ReutersTokyo_QBDF_Builder_H */
