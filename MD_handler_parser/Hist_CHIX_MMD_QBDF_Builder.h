#ifndef hf_data_Hist_CHIX_MMD_QBDF_Builder_H
#define hf_data_Hist_CHIX_MMD_QBDF_Builder_H

#include <Data/CHIX_MMD_Handler.h>
#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  class Hist_CHIX_MMD_QBDF_Builder : public QBDF_Builder {
  public:
    Hist_CHIX_MMD_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    CHIX_MMD_Handler* m_message_handler;
  };
}

#endif /* hf_data_Hist_CHIX_MMD_QBDF_Builder_H */
