#ifndef hf_data_Hist_ASX_ITCH_QBDF_Builder_H
#define hf_data_Hist_ASX_ITCH_QBDF_Builder_H

#include <Data/ASX_ITCH_1_1_Handler.h>
#include <Data/MoldUDP64.h>
#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  class Hist_ASX_ITCH_QBDF_Builder : public QBDF_Builder {
  public:
    Hist_ASX_ITCH_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date) {}

  protected:
    int process_raw_file(const string& file_name);

    ASX_ITCH_1_1_Handler* m_message_handler;
    MoldUDP64* m_mold_reader;
  };
}

#endif /* hf_data_Hist_ASX_ITCH_QBDF_Builder_H */
