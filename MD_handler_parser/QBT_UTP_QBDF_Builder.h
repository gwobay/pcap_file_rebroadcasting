#ifndef hf_data_QBT_UTP_QBDF_Builder_H
#define hf_data_QBT_UTP_QBDF_Builder_H

#include <Data/UTP_Handler.h>
#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  class QBT_UTP_QBDF_Builder : public QBDF_Builder {
  public:
    QBT_UTP_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    UTP_Handler* m_message_handler;
  };
}

#endif /* hf_data_QBT_UTP_QBDF_Builder_H */
