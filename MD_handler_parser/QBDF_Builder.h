#ifndef hf_data_QBDF_Builder_H
#define hf_data_QBDF_Builder_H

#include <map>
#include <string>
#include <vector>
#include <app/app.h>
#include <Data/Compression_Util.h>
#include <Data/LZO_Util.h>
#include <Data/Snappy_Util.h>
#include <Data/Binary_Data.h>
#include <Exchange/Exchange_Manager.h>
#include <Exchange/Feed_Rules.h>
#include <Interface/QuoteUpdate.h>
#include <MD/Quote.h>
#include <MD/MDOrderBook.h>
#include <Util/ObjectFactory.h>
#include <Util/Parameter.h>
#include <Util/Time.h>


namespace hf_core {
  using namespace std;

  typedef map<string,map<string,map<uint16_t,char> > > link_record_map_t;

  struct manual_reset {
    uint64_t qbdf_time;
    size_t context;
    char exch_code;
    ProductID id;

    manual_reset(uint64_t q, size_t c, char e, ProductID i) {
      qbdf_time = q;
      context = c;
      exch_code = e;
      id = i;
    }
  };

  class QBDF_Builder {
  public:
    QBDF_Builder(Application* app, const string& date);
    virtual ~QBDF_Builder();

    virtual void init(const Parameter& params);

    virtual int create_bin_files(const string& quote_file, const string& trade_file,
                                 const string& start_hhmm, const string& end_hhmm);

    virtual int create_summ_files(const string& start_hhmm, const string & end_hhmm,
                                  const string& summ_start_hhmm, const string & summ_end_hhmm,
                                  bool late_summary);

    virtual int sort_bin_files(const string& start_hhmm, const string & end_hhmm,
                               const string& summ_start_hhmm, const string & summ_end_hhmm,
                               bool late_summary);

    virtual int create_intraday_price_files(const string& region, const string& version,
                                            const string& start_hhmm, const string& end_hhmm,
                                            int interval_secs, const string& sids);

    virtual int compress_files(const string& start_hhmm, const string& end_hhmm, int level);
    virtual int millis_to_qbdf_time(uint32_t millis_since_midnight);
    virtual uint64_t micros_to_qbdf_time(uint64_t micros_since_midnight);
    virtual uint64_t qbdf_time_to_micros(uint64_t qbdf_time);

    virtual uint64_t nanos_to_qbdf_time(uint64_t nanos_since_midnight);
    virtual uint64_t qbdf_time_to_nanos(uint64_t qbdf_time);
    virtual uint16_t process_taq_symbol(const string & symbol_);
    virtual int write_binary_record(const string& symbol_, int taqId_, int time_,
                                    void* pRec_, int size_);
    virtual int write_binary_record_micros(uint64_t qbdf_time, void* p_rec, size_t size);
    virtual int write_binary_record_nanos(uint64_t qbdf_time, void* p_rec, size_t size);

    virtual char get_qbdf_exch_by_hf_exch_name(const string& hf_exch_name);
    virtual char get_qbdf_exch_by_hf_exch_id(ExchangeID hf_exch_id);
    virtual ExchangeID get_hf_exch_id_by_qbdf_exch(char qbdf_exch);
    virtual int write_link_record(uint32_t symbol_id, string vendor_id, string ticker,
                                  uint32_t link_id, char side);
    virtual link_record_map_t get_link_records(const string& file_name);
    virtual int write_cancel_correct_record(uint32_t symbol_id, int rec_time, int orig_rec_time,
                                            string vendor_id, string type, uint64_t orig_seq_no,
                                            double orig_price, int orig_size, string orig_cond,
                                            double new_price, int new_size, string new_cond);
    virtual int write_gap_summary_record(int rec_time, size_t context, uint64_t expected_seq_no,
                                         uint64_t received_seq_no);
    virtual string get_output_path(const string& root_path, const string& date);
    virtual bool write_qbdf_version_file(const string& qbdf_version_file_name, const string& version);
    inline virtual int get_bin_tz_adjust() { return m_bin_tz_adjust; }

  protected:
    virtual int dump_summ_file(int hh, int mm, int summ_start_time, int summ_end_time);
    virtual int dump_oc_summ_file();
    //virtual bool update_summary_trade(int hh, int mm, qbdf_trade* trade);
    //virtual bool update_summary_trade(int hh, int mm, TradeUpdate& trade_update);
    virtual bool update_summary_trade(int hh, int mm, uint16_t id, char exch,
                                      uint32_t micros_recv_time, int32_t exch_time_delta,
                                      string cond, char correct, MDPrice md_price, uint32_t size);
    virtual bool update_summary_quote(int hh, int mm, uint16_t id, char exch, uint32_t micros,
                                      char cond, MDPrice bid_price, uint32_t bid_size,
                                      MDPrice ask_price, uint32_t ask_size);
    virtual bool update_summary_imbalance(int hh, int mm, uint16_t id, char exch, uint32_t micros, int32_t exch_time_delta);
    virtual bool update_book_level(int hh, int mm, qbdf_level* p_level);
    virtual bool update_book_order_add(int hh, int mm, qbdf_order_add_price* p_order_add_price, uint32_t size);
    virtual bool update_book_order_exec(int hh, int mm, qbdf_order_exec* p_order_exec);
    virtual bool update_book_order_replace(int hh, int mm, qbdf_order_replace* p_order_replace);
    virtual bool update_book_order_modify(int hh, int mm, qbdf_order_modify* p_order_modify);
    virtual bool update_book_order_add_iprice32(int hh, int mm, qbdf_order_add_iprice32* p_order);
    virtual bool update_book_order_exec_iprice32(int hh, int mm, qbdf_order_exec_iprice32* p_order);
    virtual bool update_book_order_replace_iprice32(int hh, int mm, qbdf_order_replace_iprice32* p_order);
    virtual bool update_book_order_modify_iprice32(int hh, int mm, qbdf_order_modify_iprice32* p_order);
    virtual bool update_book_level_iprice32(int hh, int mm, qbdf_level_iprice32* p_level);
    virtual bool update_book_level_iprice64(int hh, int mm, qbdf_level_iprice64* p_level);
    virtual int process_raw_file(const string& file_name);
    virtual int process_trade_file(const string& file_name) { return 0; }
    virtual int process_quote_file(const string& file_name) { return 0; }
    virtual uint16_t clean_and_process_taq_symbol(string & symbol_);
    virtual int fixed_length_atoi(char* str_, int len_);
    virtual uint64_t fixed_length_strtol(char* str_, int len_);
    virtual double fixed_length_atof(char* str_, int len_);
    virtual double fixed_length_atof(char* str_, int len_, int decimal_pos_);
    virtual int close_open_files();
    virtual map<string,float> getSid2PriceMap(const string& region, const string& version,
                                              const string& asset_class);
    virtual int write_price_file(FILE* f, int time, const map<uint16_t,string>& id2sid,
                                 const map<string,int>& sid_filter,
                                 const map<string,float>& sid2prev_close);
    virtual string remove_time_formatting(const string& time_str, bool micros=false);
    virtual int get_exch_time_delta(uint64_t recv_time, uint64_t exch_time);
    virtual void populate_exchange_mapping(const string& mapping_file);
    virtual void populate_manual_resets(const string& manual_reset_file);
    virtual bool verify_time(const string& start_hhmm, const string& end_hhmm);
    virtual void load_ids_file();
    virtual map<uint16_t,string> get_sid_map();
    virtual int calc_timezone_offset(int src_hh, int src_dd, int tgt_hh, int tgt_dd);
    virtual bool write_qbdf_source_file(const string& qbdf_source_file_name, const string& quote_file, const string& trade_file);

    Application* m_app;
    Parameter m_params;
    LoggerRP m_logger;
    Compression_Util_RP m_compression_util;
    string m_compression_type;
    string m_date, m_id_file_name, m_root_output_path, m_output_path, m_source_name;
    Feed_Rules* m_feed_rules;

    FILE* m_id_file;
    FILE* m_raw_files[2400];
    char* m_raw_file_mode[2400];
    FILE* m_summary_files[2400];
    FILE* m_link_file;
    FILE* m_cancel_correct_file;
    FILE* m_gap_summary_file;

    uint16_t m_taq_id;
    int m_out_count;
    int m_rows_between_fcloses;
    int m_total_trades;
    int m_total_small_quotes;
    int m_total_large_quotes;
    Time m_midnight;
    bool m_tmp_bin_files;
    int m_adjust_secs;
    
    ExchangeID m_hf_exch_id;
    vector<ExchangeID> m_hf_exch_ids;
    
    map<int,int> m_tradable_exch_map;
    map<string,uint16_t> m_symbol_map;
    map<string,char> m_vendor_exch_to_qbdf_exch;
    map<int,const Exchange*> m_qbdf_exch_to_hf_exch;
    map<string,int> m_hf_exch_name_to_qbdf_exch;
    map<ExchangeID,int> m_hf_exch_id_to_qbdf_exch;
    map<uint16_t,float> m_last_trade_map;
    map<string,uint64_t> m_cum_total_volume_map;
    map<string,uint64_t> m_cum_clean_volume_map;
    map<string,uint64_t> m_cum_clean_value_map;
    vector<qbdf_summary_cache_rp> m_summary_vec;
    vector<qbdf_oc_summary_cache_rp> m_oc_summary_vec;
    vector<qbdf_oc_summary_multi_cache_rp> m_oc_summary_multi_vec;
    vector<uint32_t> m_qbdf_message_sizes;
    vector<manual_reset> m_manual_resets;

    typedef boost::shared_ptr<MDOrderBookHandler> order_book_rp;
    vector<order_book_rp> m_order_books;

    //MDOrderBookHandler *m_order_book;
    bool m_pending_inside_changed;
    bool m_find_order_id_mode;
    bool m_level_feed;
    bool m_first_write_record;
    int  m_first_hh_value;

    string m_raw_timezone;
    string m_bin_file_timezone;
    string m_ipx_file_timezone;
    string m_sum_file_timezone;
    int m_bin_tz_adjust;
    int m_ipx_tz_adjust;
    int m_sum_tz_adjust;

    int64_t m_recv_time_adj_micros;
    string m_qbdf_version;

    bool m_only_one_open_file_at_a_time;
    int m_prev_hhmm;
  };

}
#endif /* hf_data_QBDF_Builder_H */
