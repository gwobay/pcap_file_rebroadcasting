#ifndef hf_data_NYSETAQ_QBDF_Builder_H
#define hf_data_NYSETAQ_QBDF_Builder_H

#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;
  
  typedef struct {
    char time[9];
    char exch[1];
    char symbol[16];
  } nyse_taq_raw_common_rec;

  typedef struct {
    char time[12];
    char exch[1];
    char symbol[16];
  } nyse_taq_raw_common_micros_rec;

  // HHMMSSXXXXXXXXX effective on/after Oct. 3, 2016 
  typedef struct { 
    char time[15];
    char exch[1];
    char symbol[16];
  } nyse_taq_raw_common_nanos_rec;
  
  typedef struct {
    char sale_condition[4];
    char trade_volume[9];
    char trade_price[11];
    char trade_stop_indicator[1];
    char trade_correction_indicator[2];
    char trade_sequence_number[16];
    char source_of_trade[1];
    char trade_reporting_facility[1];
  } nyse_taq_raw_trade_rec;
  

  typedef struct {
    char sale_condition[4];
    char trade_volume[10];
    char trade_price[21];
    char trade_stop_indicator[1];
    char trade_correction_indicator[2];
    char trade_sequence_number[16];
    char trade_id[20];
    char source_of_trade[1];
    char trade_reporting_facility[1];
    //    char participant_timestamp[15]; //nanos for UTP, micros for CTA with appending 3 zeros
    //    char trf_timestamp[15]; //Trade Reporting Facility, nanos for UTP, micros for CTA with appending 3 zeros
    //    char tte_indicator[1];  //Trade Through Exempt
  } nyse_taq_2016_raw_trade_rec;


  typedef struct {
    char bid_price[11];
    char bid_size[7];
    char ask_price[11];
    char ask_size[7];
    char quote_condition[1];
    char market_maker[4];
    char bid_exch[1];
    char ask_exch[1];
    char sequence_number[16];
    char national_bbo_indicator[1];
    char nasdaq_bbo_indicator[1];
    char quote_cancel_correction[1];
    char source_of_quote[1];
  } nyse_taq_raw_quote_rec;

  // can use this after 20130205 to access extra fields
  typedef struct {
    char bid_price[11];
    char bid_size[7];
    char ask_price[11];
    char ask_size[7];
    char quote_condition[1];
    char market_maker[4];
    char bid_exch[1];
    char ask_exch[1];
    char sequence_number[16];
    char national_bbo_indicator[1];
    char nasdaq_bbo_indicator[1];
    char quote_cancel_correction[1];
    char source_of_quote[1];
    char retail_interest_indicator[1];
    char short_sale_restrict_indicator[1];
    char luld_bbo_indicator_cqs[1];
    char luld_bbo_indicator_utp[1];
    char finra_adf_mpid_indicator[1];
  } nyse_taq_new_raw_quote_rec;


  typedef struct {
    char bid_price[21];
    char bid_size[10];
    char ask_price[21];
    char ask_size[10];
    char quote_condition[1];
    char sequence_number[20];
    char national_bbo_indicator[1];
    char finra_bbo_indicator[1];
    char finra_adf_mpid_indicator[1]; // FINRA ADF MPID Appendage Indicator
    char quote_cancel_correction[1];
    char source_of_quote[1];
    char retail_interest_indicator[1];
    char short_sale_restriction_indicator[1];
    char luld_bbo_indicator[1];
    char sip_generated_message_identifier[1];
    char national_bbo_luld_indicator[1];
    char participant_timestamp[15]; //nanos for UTP, micros for CTA with appending 3 zeros
    char finra_adf_timestamp[15];   //nanos for UTP, micros for CTA with appending 3 zeros
    char finra_adf_market_participant_quote_indicator[1]; //UTP only
  } nyse_taq_2016_raw_quote_rec;


  class NYSETAQ_QBDF_Builder : public QBDF_Builder {
  public:
    NYSETAQ_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date) {}

  protected:
    int process_trade_file(const string& file_name);
    int process_quote_file(const string& file_name);
    vector<float> m_prev_indicative_prices;
    vector<int> m_prev_indicative_sizes;

  private:
    void parseNYSETAQTradeRec(istringstream& ss, nyse_taq_2016_raw_trade_rec *p_2016_raw_trade);
    void parseNYSETAQQuoteRec(istringstream& ss, nyse_taq_2016_raw_quote_rec* p_2016_raw_quote);
  };
}

#endif /* hf_data_NYSETAQ_QBDF_Builder_H */
