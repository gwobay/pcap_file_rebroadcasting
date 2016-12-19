#ifndef hf_data_QBT_ARCA_XDP_QBDF_Builder_H
#define hf_data_QBT_ARCA_XDP_QBDF_Builder_H

#include <Data/NYSE_XDP_Handler.h>
#include <Data/MoldUDP64.h>
#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  class QBT_ARCA_XDP_QBDF_Builder : public QBDF_Builder {
  public:
    QBT_ARCA_XDP_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    NYSE_XDP_Handler* m_message_handler;
  };
}

#endif /* hf_data_QBT_ARCA_XDP_QBDF_Builder_H */
