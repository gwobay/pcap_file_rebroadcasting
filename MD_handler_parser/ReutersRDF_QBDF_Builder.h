#ifndef hf_data_ReutersRDF_QBDF_Builder_H
#define hf_data_ReutersRDF_QBDF_Builder_H

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <map>
#include <glob.h>

#include <Data/QBDF_Builder.h>
#include <MD/Quote.h>

namespace hf_core {
  using namespace std;

  struct reuters_rdf_state {
    char date[16];
    char exch_time[16];
    char trade_price[32];
    char trade_size[32];
    char trade_time[16];
    char bid_price[32];
    char bid_size[32];
    char ask_price[32];
    char ask_size[32];
    char quote_time[16];
    char currency[4];
    char rdn_exch;
    char trade_cond_code1[2];
    char trade_cond_code2[2];
    char quote_cond_code;
    char alias[16];

    reuters_rdf_state()
    {
      date[0] = '\0';
      exch_time[0] = '\0';
      trade_time[0] = '\0';
      quote_time[0] = '\0';
      rdn_exch = ' ';
      quote_cond_code = ' ';
      memset(trade_cond_code1, ' ', sizeof(trade_cond_code1));
      memset(trade_cond_code2, ' ', sizeof(trade_cond_code2));
      memset(currency, ' ', sizeof(currency));
      memset(alias, ' ', sizeof(alias));
    }
  };

  typedef boost::shared_ptr<reuters_rdf_state> reuters_rdf_state_rp;
  typedef vector<reuters_rdf_state_rp> reuters_rdf_state_lookup;

  //typedef map<string, reuters_rdf_state*> reuters_rdf_state_map;
  //typedef pair<string, reuters_rdf_state*> reuters_rdf_state_pair;

  class ReutersRDF_QBDF_Builder : public QBDF_Builder {
  public:
    ReutersRDF_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date) {}

  protected:
    bool get_exch_time_delta(uint64_t recv_time, uint64_t exch_time, int& exch_time_delta, bool& delta_in_millis);
    int process_raw_file(const string& file_name);

    bool populate_data(const string &msg, const string &subject,
                       reuters_rdf_state_rp sd, bool &is_trade, bool &is_quote);
    string extract_val_string(const string &msg, size_t start_posn);
    void remove_char_from_string(string &str, char remove_char);

    reuters_rdf_state_lookup m_reuters_rdf_state_lookup;
  };
}

#endif /* hf_data_ReutersRDF_QBDF_Builder_H */
