#ifndef hf_data_PITCH_QBDF_Builder_H
#define hf_data_PITCH_QBDF_Builder_H

#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
  } PITCH_CommonMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char side;
    char shares[6];
    char symbol[6];
    char price[10];
  } PITCH_CommonOTMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char side;
    char shares[10];
    char symbol[6];
    char price[19];
  } PITCH_CommonOTLongMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char side;
    char shares[6];
    char symbol[6];
    char price[10];
    char display;
  } PITCH_OrderAddMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char side;
    char shares[10];
    char symbol[6];
    char price[19];
    char display;
  } PITCH_OrderAddLongMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char exec_shares[6];
    char exec_id[12];
  } PITCH_OrderExecMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char exec_shares[10];
    char exec_id[12];
  } PITCH_OrderExecLongMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char cancel_shares[6];
  } PITCH_OrderCancelMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char cancel_shares[10];
  } PITCH_OrderCancelLongMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char side;
    char shares[6];
    char symbol[6];
    char price[10];
    char exec_id[12];
  } PITCH_TradeMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char order_id[12];
    char side;
    char shares[10];
    char symbol[6];
    char price[19];
    char exec_id[12];
  } PITCH_TradeLongMsg;

  typedef struct {
    char bogus;
    char timestamp[8];
    char msg_type;
    char exec_id[12];
  } PITCH_TradeBreakMsg;

  class PITCH_QBDF_Builder : public QBDF_Builder {
  public:
    PITCH_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date) {}

  protected:
    int process_raw_file(const string& file_name);
    ullong base36decode(const string& base36_str);

    vector<float> m_prev_indicative_prices;
    vector<int> m_prev_indicative_sizes;
  };
}

#endif /* hf_data_PITCH_QBDF_Builder_H */
