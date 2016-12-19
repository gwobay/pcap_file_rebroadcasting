#ifndef hf_data_CME_Hist_QBDF_Builder_H
#define hf_data_CME_Hist_QBDF_Builder_H

#include "CME_MDP_FAST_Handler.h"
#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  class CME_Hist_QBDF_Builder : public QBDF_Builder {
  public:
    CME_Hist_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date)
    {
    }

  protected:
    int process_raw_file(const string& file_name);

  };
}

#endif /* hf_data_CME_Hist_QBDF_Builder_H */
