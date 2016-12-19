#ifndef hf_data_SMDS_PB_QBDF_Builder_H
#define hf_data_SMDS_PB_QBDF_Builder_H

#include <boost/algorithm/string.hpp>
#include <Data/Handler.h>
#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  typedef hash_map<string, size_t> ip_context_map_t;

  class SMDS_PB_QBDF_Builder : public QBDF_Builder {
  public:
    SMDS_PB_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date) {}

  protected:
    int process_raw_file(const string& file_name);

    Handler* m_message_handler;
    ip_context_map_t m_ip_context_map;

  private:
    void populate_ip_context_map();

  };
}

#endif /* hf_data_SMDS_PB_QBDF_Builder_H */
