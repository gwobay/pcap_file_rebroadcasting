#ifndef hf_data_CME_QBDF_Builder_H
#define hf_data_CME_QBDF_Builder_H

#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  class CME_QBDF_Builder : public QBDF_Builder {
  public:
    CME_QBDF_Builder(Application* app, const string& date)
      : QBDF_Builder(app, date)
    {
        if (m_app->pm()->max_product_id() == 50000) {
        for (int i=50001; i <= 100000; ++i) {
          char symbol[32];
          snprintf(symbol, sizeof(symbol), "%d", i);
          m_app->pm()->add_product(Product(i, string(symbol)));
        }
      }
    }
  };
}

#endif /* hf_data_CME_QBDF_Builder_H */
