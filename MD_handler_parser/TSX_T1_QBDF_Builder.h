#ifndef hf_data_TSX_T1_QBDF_Builder_H
#define hf_data_TSX_T1_QBDF_Builder_H

#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  struct tsx_t1_common {
    char msg_format;
    char date[6];
    char symbol[8];
    char time[6];
  };

  struct tsx_t1_trade {
    tsx_t1_common common;
    char price[7];
    char size[9];
    char input_time[6]; // hhmmss
    char seller[3];
    char buyer[3];
    char spare_1[4];
    char odd_lot;
    char spare_2;
    char delayed_delivery;
    char cash;
    char spare_3;
    char non_net;
    char cancellation;
    char cancelled;
    char special_terms;
    char spare_4[2];
    char posit_trade;
    char spare_5;
    char open_trade;
    char correction;
    char spare_6[20];
  };

  struct tsx_t1_quote {
    tsx_t1_common common;
    char bid_price[7];
    char ask_price[7];
    char bid_size[3];
    char ask_size[3];
    char spare[46];
  };


  class TSX_T1_QBDF_Builder : public QBDF_Builder {
  public:
    TSX_T1_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date) {}

  protected:
    int process_raw_file(const string& file_name);
  };
}

#endif /* hf_data_TSX_T1_QBDF_Builder_H */
