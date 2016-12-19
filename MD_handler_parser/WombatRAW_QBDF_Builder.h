#ifndef hf_data_WombatRAW_QBDF_Builder_H
#define hf_data_WombatRAW_QBDF_Builder_H

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <map>
#include <glob.h>

#include <Data/QBDF_Builder.h>
#include <MD/Quote.h>

namespace hf_core {
  using namespace std;

  BOOST_ENUM(WombatMamaMsgType,
             (GeneralUpdate)
             (InitialValue)
             (TradeCancel)
             (TradeError)
             (TradeCorrection)
             (ClosingSummary)
             (RefreshRecap)
             (DeletedSymbol)
             (ExpiredSecurity)
             (FullRecordSnapshot)
             (NoSubscribers)
             (BadSymbol)
             (PreOpenSummary)
             (QuoteUpdate)
             (TradeUpdate)
             (OrderUpdate)
             (OrderBookInit)
             (OrderBookDelta)
             (OrderBookClear)
             (OrderBookRecap)
             (OrderBookSnapshot)
             (NotPermissioned)
             (NotPresentMaybeLater)
             (SecurityStatusUpdate)
             (CorporateAction)
             (Misc)
             (StaleData)
  );

  BOOST_ENUM_VALUES(WombatFields, int,
                    (W_ACTIVITY_TIME)(102)
                    (W_ORDER_SEQNO)(414)
                    (W_SYMBOL)(470)
                    (W_NUM_LEVELS)(651)
                    (W_PL_ACTION)(652)
                    (W_PL_PRICE)(653)
                    (W_PL_SIDE)(654)
                    (W_PL_SIZE)(655)
                    (W_PL_SIZE_CHANGE)(656)
                    (W_PL_NUM_ENTRIES)(657)
                    (W_PL_TIME)(658)
                    (W_PL_NUM_ATTACH)(659)
                    (W_ENTRY_ID)(681)
                    (W_ENTRY_SIZE)(682)
                    (W_ENTRY_ACTION)(683)
                    (W_ENTRY_REASON)(684)
                    (W_ENTRY_TIME)(685)
                    (W_ENTRY_STATUS)(686)
                    (W_PRICE_LEVELS)(699)
                    (W_ENTRY)(700)
  );

  typedef map<int64_t,string> field_map_t;
  typedef map<string,string> name_value_map_t;
  typedef map<string,string>::iterator name_value_map_iter_t;
  typedef map<string,string>::const_iterator name_value_map_citer_t;

  class WombatRAW_QBDF_Builder : public QBDF_Builder {
  public:
    WombatRAW_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date)
    {
      m_prev_indicative_prices.clear();
      m_prev_indicative_prices.push_back(MDPrice());

      m_prev_indicative_sizes.clear();
      m_prev_indicative_sizes.push_back(-1);
    }

  protected:
    int process_raw_file(const string& file_name);
    int process_trade_file(const string& file_name) { return 0; }
    int process_quote_file(const string& file_name) { return 0; }
    string get_map_value(const name_value_map_t& data_map, const string& key);
    bool is_in_map(const name_value_map_t& data_map, const string& key);
    vector<string> get_vector_from_string(const string& str);
    field_map_t get_map_from_string(const string& str);
    bool get_exch_time_delta(uint64_t recv_time, uint64_t exch_time,
                             int& exch_time_delta, bool& delta_in_millis);
    string get_date_from_time_field(string& ts);

    vector<MDPrice> m_prev_indicative_prices;
    vector<uint32_t> m_prev_indicative_sizes;
    vector<uint32_t> m_latest_seq_num;
  };
}

#endif /* hf_data_WombatRAW_QBDF_Builder_H */
