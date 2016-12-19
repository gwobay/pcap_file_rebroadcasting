#ifndef hf_data_NYSE_Imbalance_QBDF_Builder_H
#define hf_data_NYSE_Imbalance_QBDF_Builder_H

#include <Data/QBDF_Builder.h>

typedef unsigned long ulong;

namespace hf_core {
  using namespace std;

  typedef struct {
    uint32_t msg_seqno;
    uint16_t msg_type;
    uint32_t send_time;
    char symbol[11];
    char filler;
    char imbalance_side; // B-Buy, S-Sell, 0-No Imbalance
    char price_scale_code; // # of decimal places
    uint32_t ref_price;
    uint32_t imbalance_size;
    uint32_t paired_size;
    uint32_t source_time;
  } __attribute__((packed)) NYSE_Imbalance_Rec;

  class NYSE_Imbalance_QBDF_Builder : public QBDF_Builder {
  public:
    NYSE_Imbalance_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date) {}

  protected:
    int process_raw_file(const string& file_name);
    int process_trade_file(const string& file_name) { return 0; }
    int process_quote_file(const string& file_name) { return 0; }
  };
}

#endif /* hf_data_NYSE_Imbalance_QBDF_Builder_H */
