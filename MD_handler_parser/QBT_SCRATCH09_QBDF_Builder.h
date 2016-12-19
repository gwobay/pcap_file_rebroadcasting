#ifndef hf_data_QBT_SCRATCH09_QBDF_Builder_H
#define hf_data_QBT_SCRATCH09_QBDF_Builder_H

#include <Data/DirectEdge_0_9_Handler.h>
#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  class QBT_SCRATCH09_QBDF_Builder : public QBDF_Builder {
  public:
    QBT_SCRATCH09_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    DirectEdge_0_9_Handler* m_message_handler;
  };
}

#endif /* hf_data_QBT_SCRATCH09_QBDF_Builder_H */
