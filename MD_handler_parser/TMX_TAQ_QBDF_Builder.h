#ifndef hf_data_TMX_TAQ_QBDF_Builder_H
#define hf_data_TMX_TAQ_QBDF_Builder_H

#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  struct tmx_taq_date {
    char record_type;
    char date[8];
  } __attribute__ ((packed));

  struct tmx_taq_common_old {
    char record_type;
    char symbol[8];
    char time[8];
    char seq_no[9];
  } __attribute__ ((packed));

  struct tmx_taq_trade_old {
    tmx_taq_common_old common;
    char price[7];
    char shares[9];
    char buyer[3];
    char seller[3];
    char odd_lot;
    char trade_session;
    char cancellation;
    char cancelled;
    char correction;
    char delayed_delivery;
    char cash;
    char non_net;
    char special_terms;
    char specialty_cross;
    char listed_market;
  } __attribute__ ((packed));

  struct tmx_taq_quote_old {
    tmx_taq_common_old common;
    char bid_price[7];
    char ask_price[7];
    char bid_size[3];
    char ask_size[3];
    char halted;
    char listed_market;
  } __attribute__ ((packed));

  struct tmx_taq_common {
    char record_type;
    char symbol[8];
    char time[15];
    char seq_no[9];
  } __attribute__ ((packed));

  struct tmx_taq_trade {
    tmx_taq_common common;
    char price[7];
    char shares[9];
    char buyer[3];
    char seller[3];
    char odd_lot;
    char trade_session;
    char cancellation;
    char cancelled;
    char correction;
    char delayed_delivery;
    char cash;
    char non_net;
    char special_terms;
    char specialty_cross;
    char listed_market;
  } __attribute__ ((packed));

  struct tmx_taq_quote {
    tmx_taq_common common;
    char bid_price[7];
    char ask_price[7];
    char bid_size[3];
    char ask_size[3];
    char halted;
    char listed_market;
  } __attribute__ ((packed));


  class TMX_TAQ_QBDF_Builder : public QBDF_Builder {
  public:
    TMX_TAQ_QBDF_Builder(Application* app, const string& date)
      : QBDF_Builder(app, date), m_last_min_reported(0) {}

  protected:
    int process_raw_file(const string& file_name);

    uint64_t m_last_min_reported;
  };
}

#endif /* hf_data_TMX_TAQ_QBDF_Builder_H */
