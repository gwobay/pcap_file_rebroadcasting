#ifndef hf_data_LSE_Indicative_QBDF_Builder_H
#define hf_data_LSE_Indicative_QBDF_Builder_H

#include <Data/QBDF_Builder.h>

typedef unsigned long ulong;

namespace hf_core {
  using namespace std;

  class LSE_Indicative_QBDF_Builder : public QBDF_Builder {
  public:
    LSE_Indicative_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date) {}

  protected:
    int process_raw_file(const string& file_name);
    int process_trade_file(const string& file_name) { return 0; }
    int process_quote_file(const string& file_name) { return 0; }

    vector<float> m_prev_indicative_prices;
    vector<int> m_prev_indicative_sizes;
  };
}

#endif /* hf_data_LSE_Indicative_QBDF_Builder_H */
