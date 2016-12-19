#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <boost/filesystem.hpp>

#include <Data/Handler.h>
#include <Data/QBDF_Builder.h>
#include <Data/QBDF_Util.h>
#include <Logger/Logger_Manager.h>
#include <MD/MDCache.h>
#include <Product/Product_Manager.h>
#include <Product/Product.h>
#include <Util/CSV_Parser.h>
#include <Util/File_Utils.h>


namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  using namespace Time_Constants;

  static const char* APPEND_MODE = "a";
  static const char* WRITE_MODE = "w";


  // ----------------------------------------------------------------
  // Configure reader based on given parameters
  // ----------------------------------------------------------------
  void QBDF_Builder::init(const Parameter& params)
  {
    const char* where = "QBDF_Builder::init";
    m_logger = m_app->lm()->get_logger("");

    m_only_one_open_file_at_a_time = false;
    m_prev_hhmm = 0;

    m_qbdf_version = params.getDefaultString("qbdf_version", "1.0");
    m_qbdf_message_sizes = get_qbdf_message_sizes(m_qbdf_version);
    m_logger->log_printf(Log::INFO, "%s: Retrieved the following QBDF msg sizes using version %s:",
                         where, m_qbdf_version.c_str());
    for(size_t i = 0; i < m_qbdf_message_sizes.size(); i++) {
      size_t msg_size = m_qbdf_message_sizes[i];
      if (msg_size == 0)
        continue;

      m_logger->log_printf(Log::INFO, "%s:   msg_format=%lu msg_size=%lu", where, i, msg_size);
    }

    m_params = params;
    m_source_name = params.getDefaultString("name", "Default");
    m_root_output_path = params["root_output_path"].getString();
    m_output_path = get_output_path(m_root_output_path, m_date);
    boost::filesystem::path boost_output_path(m_output_path.c_str());
    boost::filesystem::create_directories(boost_output_path);

    if (params.has("find_order_id_list")) {
      m_find_order_id_mode = true;
    } else {
      m_find_order_id_mode = false;
    }

    m_compression_type = params.getDefaultString("compression_type", "lzo");
    if (m_compression_type == "snappy") {
      m_logger->log_printf(Log::INFO, "%s: Will use snappy for compression", where);
      m_compression_util.reset(new Snappy_Util(m_app, m_logger));
    } else if (m_compression_type == "lzo") {
      m_logger->log_printf(Log::INFO, "%s: Will use lzo for compression", where);
      m_compression_util.reset(new LZO_Util(m_app, m_logger));
    } else {
      m_logger->log_printf(Log::ERROR, "%s: Unsupported compression type [%s] (use lzo/snappy)",
                           where, m_compression_type.c_str());
      m_logger->sync_flush();
      return;
    }

    m_rows_between_fcloses = params.getDefaultInt("rows_between_fcloses", 2000000000);
    m_id_file = NULL;
    m_taq_id = 0;
    m_out_count = 0;
    m_total_trades = 0;
    m_total_small_quotes = 0;
    m_total_large_quotes = 0;

    string feed_rules_name = params.getDefaultString("feed_rules_name", "Default");
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, m_date);
    m_logger->log_printf(Log::INFO, "%s: Using %s feed rules for %s",
                         where, m_feed_rules->get_rule_name().c_str(), feed_rules_name.c_str());

    m_tmp_bin_files = params.getDefaultBool("tmp_bin_files", false);
    m_level_feed = params.getDefaultBool("level_feed", false);

    string exch_map_file = params.getDefaultString("exch_map", m_root_output_path + "/exch_map.csv");
    populate_exchange_mapping(exch_map_file);

    populate_manual_resets(m_root_output_path + "/manual_resets.csv");

    Time::set_simulation_mode(true);
    m_raw_timezone = params.getDefaultString("raw_timezone", "");
    m_bin_file_timezone = params.getDefaultString("bin_file_timezone", "");
    m_ipx_file_timezone = params.getDefaultString("ipx_hhmm_timezone", "");
    m_sum_file_timezone = params.getDefaultString("summ_hhmm_timezone", "");

    m_logger->log_printf(Log::INFO, "%s: Raw data is expected to be %s",
                         where, m_raw_timezone.c_str());
    m_logger->log_printf(Log::INFO, "%s: Will convert binary files (bin) to be %s",
                         where, m_bin_file_timezone.c_str());
    m_logger->log_printf(Log::INFO, "%s: Will convert interim prices (ipx) files to be %s",
                         where, m_ipx_file_timezone.c_str());
    m_logger->log_printf(Log::INFO, "%s: Will convert times used for feed rules to be %s",
                         where, m_sum_file_timezone.c_str());

    int file_hours_offset = params.getDefaultInt("file_hours_offset", 0);

    time_t current_time = (time_t)Time(m_date, "%Y%m%d").total_sec();
    //time_t current_time;
    //time(&current_time);
    struct tm* current_tm;

    setenv("TZ", m_raw_timezone.c_str(), 1);
    tzset();
    current_tm = localtime(&current_time);
    int raw_hh = current_tm->tm_hour;
    int raw_dd = current_tm->tm_yday;

    setenv("TZ", m_bin_file_timezone.c_str(), 1);
    tzset();
    current_tm = localtime(&current_time);
    int bin_file_hh = current_tm->tm_hour;
    int bin_file_dd = current_tm->tm_yday;

    setenv("TZ", m_ipx_file_timezone.c_str(), 1);
    tzset();
    current_tm = localtime(&current_time);
    int ipx_file_hh = current_tm->tm_hour;
    int ipx_file_dd = current_tm->tm_yday;

    setenv("TZ", m_sum_file_timezone.c_str(), 1);
    tzset();
    current_tm = localtime(&current_time);
    int sum_file_hh = current_tm->tm_hour;
    int sum_file_dd = current_tm->tm_yday;

    m_bin_tz_adjust = calc_timezone_offset(raw_hh, raw_dd, bin_file_hh, bin_file_dd) + (3600 * file_hours_offset);
    m_ipx_tz_adjust = calc_timezone_offset(bin_file_hh, bin_file_dd, ipx_file_hh, ipx_file_dd) + (3600 * file_hours_offset);
    m_sum_tz_adjust = calc_timezone_offset(bin_file_hh, bin_file_dd, sum_file_hh, sum_file_dd) + (3600 * file_hours_offset);

    m_logger->log_printf(Log::INFO, "%s: Using %d offset to convert raw data times to bin times",
                         where, m_bin_tz_adjust);
    m_logger->log_printf(Log::INFO, "%s: Using %d offset to convert bin times to ipx times",
                         where, m_ipx_tz_adjust);
    m_logger->log_printf(Log::INFO, "%s: Using %d offset to convert bin times to feed rule eval times",
                         where, m_sum_tz_adjust);

    m_midnight = Time(m_date, "%Y%m%d") + seconds(m_sum_tz_adjust);

    if (params.getDefaultBool("build_book", false)) {
      if (!params.has("book_exchange")) {
        m_logger->log_printf(Log::ERROR, "%s: Must specify book_exchange param to build book!", where);
        m_logger->sync_flush();
        return;
      }

      //int max_exchanges = ExchangeID::size;
      m_order_books.resize(ExchangeID::size);
      
      if (params["book_exchange"].isString()) {
	string book_exchange = params["book_exchange"].getString();
	ExchangeID hf_exch_id = m_app->em()->find_by_name(book_exchange)->primary_id();
	m_hf_exch_id = hf_exch_id;
	m_hf_exch_ids.push_back(hf_exch_id);
	m_order_books[hf_exch_id.index()] = order_book_rp(new MDOrderBookHandler(m_app, m_hf_exch_id, m_app->lm()->get_logger("")));
	m_order_books.at(hf_exch_id.index()).get()->init(params);
      } else {
	vector<string> exchanges_vec = m_params["book_exchange"].getStringVector();
	for (vector<string>::iterator it = exchanges_vec.begin(); it != exchanges_vec.end(); ++it) {
	  ExchangeID hf_exch_id = m_app->em()->find_by_name(*it)->primary_id();
	  m_hf_exch_ids.push_back(hf_exch_id);
	  m_order_books[hf_exch_id.index()] = order_book_rp(new MDOrderBookHandler(m_app, m_hf_exch_id, m_app->lm()->get_logger("")));
	  m_order_books.at(hf_exch_id.index()).get()->init(params);
	}
      }
    } else {
      m_logger->log_printf(Log::INFO, "%s: Running without order book (no level 2 processing).", where);
    }

    m_recv_time_adj_micros =
      m_params.getDefaultInt("recv_time_adj_hr", 0) * Time_Constants::micros_per_hr
      + m_params.getDefaultInt("recv_time_adj_min", 0) * Time_Constants::micros_per_min
      + m_params.getDefaultInt("recv_time_adj_sec", 0) * Time_Constants::micros_per_second
      + m_params.getDefaultInt("recv_time_adj_usec", 0);
    
    //write_qbdf_version_file(m_output_path + "/qbdf_version", m_qbdf_version);

    m_first_write_record = true;
    m_logger->sync_flush();
  }

  int QBDF_Builder::calc_timezone_offset(int src_hh, int src_dd, int tgt_hh, int tgt_dd)
  {
    if (src_dd == tgt_dd) {
      return (tgt_hh - src_hh) * 3600;
    } if (src_dd == 0 && tgt_dd >= 364) {
      return (tgt_hh - src_hh - 24) * 3600;
    } else if (tgt_dd == 0 && src_dd >= 364) {
      return (tgt_hh - src_hh + 24) * 3600;
    } else if (src_dd > tgt_dd) {
      return (tgt_hh - src_hh - 24) * 3600;
    } else {
      return (tgt_hh - src_hh + 24) * 3600;
    }
  }

  // ----------------------------------------------------------------
  // QBDF_Builder reads in raw TAQ files and produces optimized, sorted binary
  // files (QBDF: Quantbot Binary Data Format) of all trades and quotes,
  // split into 1 minute chunks and 1 minute summary files.
  // ----------------------------------------------------------------
  QBDF_Builder::QBDF_Builder(Application* app, const string& date)
  : m_app(app), m_date(date), m_link_file(0), m_cancel_correct_file(0),
  m_gap_summary_file(0), m_find_order_id_mode(false),
  m_level_feed(false), m_bin_tz_adjust(0), m_ipx_tz_adjust(0), m_sum_tz_adjust(0)
  {
    for (int i=0; i<2400; i++) {
      m_raw_files[i] = NULL;
      m_raw_file_mode[i] = (char*)WRITE_MODE; // start in overwrite mode
      m_summary_files[i] = NULL;
    }

    qbdf_summary_cache_rp cache = qbdf_summary_cache_rp(new qbdf_summary_cache(0));
    m_summary_vec.push_back(cache);

    qbdf_oc_summary_cache_rp oc_cache = qbdf_oc_summary_cache_rp(new qbdf_oc_summary_cache(0));
    m_oc_summary_vec.push_back(oc_cache);

    qbdf_oc_summary_multi_cache_rp oc_multi_cache = qbdf_oc_summary_multi_cache_rp(new qbdf_oc_summary_multi_cache(0));
    m_oc_summary_multi_vec.push_back(oc_multi_cache);

    if (m_app->pm()->max_product_id() == 0) {
      for (int i=1; i <= 50000; ++i) {
	char symbol[32];
	snprintf(symbol, 32, "%d", i);
	m_app->pm()->add_product(Product(i, string(symbol)));
      }
    }
  }


  // ----------------------------------------------------------------
  // Close any files and clean up
  // ----------------------------------------------------------------
  QBDF_Builder::~QBDF_Builder()
  {
    // Close the id's file
    if (m_id_file != 0) {
      fclose(m_id_file);
      m_id_file = 0;
    }

    if (m_link_file != 0) {
      fclose(m_link_file);
      m_link_file = 0;
    }

    if (m_cancel_correct_file != 0) {
      fclose(m_cancel_correct_file);
      m_cancel_correct_file = 0;
    }

    if (m_gap_summary_file != 0) {
      fclose(m_gap_summary_file);
      m_gap_summary_file = 0;
    }

    close_open_files();

  }


  void QBDF_Builder::populate_manual_resets(const string& manual_reset_file) {
    const char* where = "QBDF_Builder::populate_manual_resets";

    ifstream ifile(manual_reset_file.c_str());
    if (!ifile) {
      m_logger->log_printf(Log::INFO, "%s: No manual reset file found, skipping...", where);
      return;
    }

    m_logger->log_printf(Log::INFO, "%s: Reading manual resets from %s",
                         where, manual_reset_file.c_str());
    CSV_Parser csv_reader(manual_reset_file);

    int date_idx = csv_reader.column_index("date");
    int qbdf_time_idx = csv_reader.column_index("qbdf_time");
    int context_idx = csv_reader.column_index("context");
    int exch_code_idx = csv_reader.column_index("exch_code");
    int vendor_id_idx = csv_reader.column_index("vendor_id");

    CSV_Parser::state_t s = csv_reader.read_row();
    while (s == CSV_Parser::OK) {
      string date = csv_reader.get_column(date_idx);
      if (date != m_date) {
        s = csv_reader.read_row();
        continue;
      }

      uint64_t qbdf_time = strtoll(csv_reader.get_column(qbdf_time_idx), 0, 10);
      size_t context = strtol(csv_reader.get_column(context_idx), 0, 10);
      char exch_code = csv_reader.get_column(exch_code_idx)[0];
      string vendor_id = csv_reader.get_column(vendor_id_idx);

      m_manual_resets.push_back(manual_reset(qbdf_time, context, exch_code,
                                             process_taq_symbol(vendor_id)));
      s = csv_reader.read_row();
    }

  }


  void QBDF_Builder::populate_exchange_mapping(const string& mapping_file) {
    const char* where = "QBDF_Builder::populate_exchange_mappings";

    m_logger->log_printf(Log::INFO, "%s: Reading exchange mappings from %s",
                         where, mapping_file.c_str());
    CSV_Parser csv_reader(mapping_file);

    int taq_exch_char_idx = csv_reader.column_index("taq_exch_char");
    int vendor_exch_code_idx = csv_reader.column_index("vendor_exch_code");
    int prim_exch_id_idx = csv_reader.column_index("prim_exch_id");

    CSV_Parser::state_t s = csv_reader.read_row();
    while (s == CSV_Parser::OK) {
      string vendor_exch_code = csv_reader.get_column(vendor_exch_code_idx);
      char taq_exch_char = csv_reader.get_column(taq_exch_char_idx)[0];
      int prim_exch_id = strtol(csv_reader.get_column(prim_exch_id_idx), 0, 10);
      const Exchange* p_exch = m_app->em()->find_by_primary_id(prim_exch_id);
      if (!p_exch) {
        m_logger->log_printf(Log::WARN, "%s: mapping exch char [%c] to id [%d] BUT can't find ExchangeID",
                             where, taq_exch_char, prim_exch_id);
        s = csv_reader.read_row();
        continue;
      }
      
      m_qbdf_exch_to_hf_exch[(int)taq_exch_char] = p_exch;
      m_hf_exch_name_to_qbdf_exch[p_exch->name()] = (int)taq_exch_char;
      m_hf_exch_id_to_qbdf_exch[p_exch->primary_id()] = (int)taq_exch_char;
      m_vendor_exch_to_qbdf_exch[vendor_exch_code] = taq_exch_char;
      s = csv_reader.read_row();
    }
  }

  char QBDF_Builder::get_qbdf_exch_by_hf_exch_name(const string& hf_exch_name) {
    map<string,int>::iterator iter = m_hf_exch_name_to_qbdf_exch.find(hf_exch_name);
    if (iter == m_hf_exch_name_to_qbdf_exch.end()) {
      m_logger->log_printf(Log::ERROR, "%s: Could not find QBDF exch letter for HF exch %s",
                           "QBDF_Builder::get_qbdf_exch_by_hf_exch_name", hf_exch_name.c_str());
      m_logger->sync_flush();
      return ' ';
    } else {
      return iter->second;
    }
  }

  char QBDF_Builder::get_qbdf_exch_by_hf_exch_id(ExchangeID hf_exch_id) {
    map<ExchangeID,int>::iterator iter = m_hf_exch_id_to_qbdf_exch.find(hf_exch_id);
    if (iter == m_hf_exch_id_to_qbdf_exch.end()) {
      m_logger->log_printf(Log::ERROR, "%s: Could not find QBDF exch letter for HF exch %u",
                           "QBDF_Builder::get_qbdf_exch_by_hf_exch_id", hf_exch_id.index());
      m_logger->sync_flush();
      return ' ';
    } else {
      return iter->second;
    }
  }

  ExchangeID QBDF_Builder::get_hf_exch_id_by_qbdf_exch(char qbdf_exch) {
    map<int,const Exchange*>::iterator iter = m_qbdf_exch_to_hf_exch.find((int)qbdf_exch);
    if (iter == m_qbdf_exch_to_hf_exch.end()) {
      // Suppress log message for 'E' which is the CQS/CTS composite (NBBO) designation
      if (qbdf_exch != 'E') {
        m_logger->log_printf(Log::ERROR, "%s: Could not find HF exch for QBDF exch letter %c",
                             "QBDF_Builder::get_hf_exch_id_by_qbdf_exch", qbdf_exch);
        m_logger->sync_flush();
      }
      return ExchangeID::MULTI;
    } else {
      return iter->second->primary_id();
    }
  }

  string QBDF_Builder::get_output_path(const string& root_path, const string& date) {
    string output_path = m_root_output_path;
    output_path.append("/").append(date.substr(0, 4));
    output_path.append("/").append(date.substr(4, 2));
    output_path.append("/").append(date.substr(6, 2));

    return output_path;
  }


  bool QBDF_Builder::verify_time(const string& start_hhmm, const string& end_hhmm) {
    const char* where = "QBDF_Builder::verify_time";

    int start_time = atoi(start_hhmm.c_str());
    if (start_hhmm.length() != 4 || start_time > 2359 || start_time < 0) {
      m_logger->log_printf(Log::ERROR, "%s: start_hhmm [%s] not in range 0000-2359",
                                 where, start_hhmm.c_str());
      m_logger->sync_flush();
      return false;
    }

    int end_time = atoi(end_hhmm.c_str());
    if (end_hhmm.length() != 4 || end_time > 2359 || end_time < 0) {
      m_logger->log_printf(Log::ERROR, "%s: end_hhmm [%s] not in range 0000-2359",
                                 where, end_hhmm.c_str());
      m_logger->sync_flush();
      return false;
    }

    if ((end_time < start_time) && (m_bin_tz_adjust == 0) & (m_sum_tz_adjust == 0) && (m_ipx_tz_adjust == 0)) {
      m_logger->log_printf(Log::ERROR, "%s: end_hhmm [%s] before start_hhmm [%s]",
                           where, end_hhmm.c_str(), start_hhmm.c_str());
      m_logger->sync_flush();
      return false;
    }

    return true;
  }

  void QBDF_Builder::load_ids_file() {
    const char* where = "QBDF_Builder::load_ids_file";

    // Make sure id file is closed from previous writing...
    if (m_id_file != 0) {
      fclose(m_id_file);
      m_id_file = 0;
    }

    m_summary_vec.clear();
    qbdf_summary_cache_rp cache = qbdf_summary_cache_rp(new qbdf_summary_cache(0));
    m_summary_vec.push_back(cache);

    m_oc_summary_vec.clear();
    qbdf_oc_summary_cache_rp oc_cache = qbdf_oc_summary_cache_rp(new qbdf_oc_summary_cache(0));
    m_oc_summary_vec.push_back(oc_cache);

    m_oc_summary_multi_vec.clear();
    qbdf_oc_summary_multi_cache_rp oc_multi_cache = qbdf_oc_summary_multi_cache_rp(new qbdf_oc_summary_multi_cache(0));
    m_oc_summary_multi_vec.push_back(oc_multi_cache);

    m_id_file_name = m_output_path + "/ids.csv";
    m_logger->log_printf(Log::INFO, "%s: Reading from %s", where, m_id_file_name.c_str());
    CSV_Parser csv_reader(m_id_file_name);
    int id_posn = csv_reader.column_index("BinaryId");

    CSV_Parser::state_t s = csv_reader.read_row();
    while (s == CSV_Parser::OK) {
      uint16_t id = strtol(csv_reader.get_column(id_posn), 0, 10);

      m_summary_vec.push_back(qbdf_summary_cache_rp(new qbdf_summary_cache(id)));
      m_oc_summary_vec.push_back(qbdf_oc_summary_cache_rp(new qbdf_oc_summary_cache(id)));
      m_oc_summary_multi_vec.push_back(qbdf_oc_summary_multi_cache_rp(new qbdf_oc_summary_multi_cache(id)));
      s = csv_reader.read_row();
    }
    m_logger->sync_flush();
  }


  map<uint16_t,string> QBDF_Builder::get_sid_map() {
    const char* where = "QBDF_Builder::get_sid_map";

    map<uint16_t,string> sid_map;
    string sid_map_file = m_output_path + "/ids_to_sids.csv";
    m_logger->log_printf(Log::INFO, "%s: Reading from %s", where, sid_map_file.c_str());
    CSV_Parser csv_reader(sid_map_file);
    int id_posn = csv_reader.column_index("BinaryId");
    int sid_posn = csv_reader.column_index("Sid");

    CSV_Parser::state_t s = csv_reader.read_row();
    while (s == CSV_Parser::OK) {
      uint16_t id = strtol(csv_reader.get_column(id_posn), 0, 10);
      string sid = csv_reader.get_column(sid_posn);
      sid_map[id] = sid;
      s = csv_reader.read_row();
    }
    m_logger->sync_flush();
    return sid_map;
  }

  // ----------------------------------------------------------------
  // create set of binary files based on raw TAQ data
  // ----------------------------------------------------------------
  int QBDF_Builder::create_bin_files(const string& quote_file, const string& trade_file,
                                     const string& start_hhmm, const string& end_hhmm)
  {
    const char* where = "QBDF_Builder::create_bin_files";

    if (quote_file.empty() || trade_file.empty() || m_date.empty()) {
      m_logger->log_printf(Log::ERROR, "%s: missing or empty parameters for initialization", where);
      m_logger->sync_flush();
      return 1;
    }

    if (!verify_time(start_hhmm, end_hhmm))
      return 1;

    // Now process raw file(s)
    if (quote_file != trade_file) {
      if (process_trade_file(trade_file)) {
        m_logger->log_printf(Log::ERROR, "%s: Detected problem processing trades file", where);
        m_logger->sync_flush();
        return 1;
      }

      if (process_quote_file(quote_file)) {
        m_logger->log_printf(Log::ERROR, "%s: Detected problem processing quotes file", where);
        m_logger->sync_flush();
        return 1;
      }
    } else {
      if (process_raw_file(trade_file)) {
        m_logger->log_printf(Log::ERROR, "%s: Detected problem processing raw file", where);
        m_logger->sync_flush();
        return 1;
      }
    }

    if (close_open_files()) {
      m_logger->log_printf(Log::ERROR, "%s: unable to close open files", where);
      m_logger->sync_flush();
      return 1;
    }

    write_qbdf_source_file(m_output_path + "/qbdf_source", quote_file, trade_file);

    return 0;
  }


  // ----------------------------------------------------------------
  // sort previously created set of binary files into true time order
  // ----------------------------------------------------------------
  int QBDF_Builder::sort_bin_files(const string& start_hhmm, const string & end_hhmm,
                                   const string& summ_start_hhmm, const string & summ_end_hhmm,
                                   bool late_summary)
  {
    const char* where = "QBDF_Builder::sort_bin_files";

    m_logger->log_printf(Log::INFO, "%s: start_hhmm=%s end_hhmm=%s summ_start_hhmm=%s summ_end_hhmm=%s",
                         where, start_hhmm.c_str(), end_hhmm.c_str(),
                         summ_start_hhmm.c_str(), summ_end_hhmm.c_str());

    // Ensure all open files are close
    if (close_open_files()) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to close open files", where);
      m_logger->sync_flush();
      return 1;
    }

    load_ids_file();

    if (!verify_time(start_hhmm, end_hhmm)) {
      m_logger->log_printf(Log::ERROR, "%s: problem with bin start/end times", where);
      m_logger->sync_flush();
      return 1;
    }

    if (!verify_time(summ_start_hhmm, summ_end_hhmm)) {
      m_logger->log_printf(Log::ERROR, "%s: problem with summary start/end times", where);
      m_logger->sync_flush();
      return 1;
    }

    int start_time = atoi(start_hhmm.c_str());
    int end_time = atoi(end_hhmm.c_str());
    int summ_start_time = atoi(summ_start_hhmm.c_str());
    int summ_end_time = atoi(summ_end_hhmm.c_str());

    map<int,SequencedRecordMap> taq_map;
    for (int i = start_time; i <= end_time; ++i) {
      // walk through all potential files and sort them
      int hh = i / 100;
      int mm = i % 100;
      if (mm == 60) {
        mm = 0;
        hh++;
        i = 100 * hh;
      }
      if (hh > 23 || hh < 0 || mm < 0 || mm > 59) {
        m_logger->log_printf(Log::ERROR, "%s: invalid time hh=[%d] mm=[%d] time=[%d]", where, hh, mm, i);
        m_logger->sync_flush();
        return 1;
      }

      char bin_file[256];
      sprintf(bin_file, "%s/%02d%02d00.tmp_bin", m_output_path.c_str(), hh, mm);
      FILE *f = fopen(bin_file,"rb");
      if (f == 0) {
        if (dump_summ_file(hh, mm, summ_start_time, summ_end_time)) {
          m_logger->log_printf(Log::ERROR, "%s: dumping to summary file hh=[%d] mm=[%d]", where, hh, mm);
          m_logger->sync_flush();
        }
        continue;
      }

      fseek(f,0,SEEK_END);
      long file_size = ftell(f);
      rewind(f);
      char *buffer = new char[file_size + 1024]; // little safety net
      size_t result = fread(buffer,1,file_size,f);
      if (result != (size_t) file_size) {
        m_logger->log_printf(Log::ERROR, "%s: Could not read file [%s] into memory", where, bin_file);
        m_logger->sync_flush();
        delete [] buffer;
        continue;
      }

      char* pPos = buffer;
      while (pPos < buffer+file_size) {
        qbdf_format_header *pHdr = (qbdf_format_header*) pPos;
        int rec_time = 0;
        int seq = 0;
        int symbolId = 0;

        qbdf_quote_seq_small *p_quote_small = (qbdf_quote_seq_small*)pPos;
        qbdf_quote_seq_large *p_quote_large = (qbdf_quote_seq_large*)pPos;
        qbdf_trade_seq *pTrade = (qbdf_trade_seq*)pPos;
        qbdf_imbalance_seq *pIndicative = (qbdf_imbalance_seq*)pPos;
        qbdf_status_seq *pStatus = (qbdf_status_seq*)pPos;
	qbdf_trade_iprice32_seq *p_trade_iprice32 = (qbdf_trade_iprice32_seq*)pPos;
	qbdf_quote_small_iprice32_seq *p_quote_small_iprice32 = (qbdf_quote_small_iprice32_seq*)pPos;
	qbdf_quote_large_iprice32_seq *p_quote_large_iprice32 = (qbdf_quote_large_iprice32_seq*)pPos;
	qbdf_imbalance_iprice32_seq *p_imbalance_iprice32 = (qbdf_imbalance_iprice32_seq*)pPos;

        switch (pHdr->format) {
        case QBDFMsgFmt::QuoteSmall:
          seq = p_quote_small->seq;
          rec_time = i*100000 + p_quote_small->sqr.qr.time;
          symbolId = p_quote_small->sqr.qr.symbolId;
          pPos += sizeof(qbdf_quote_seq_small);
          break;
        case QBDFMsgFmt::QuoteLarge:
          seq = p_quote_large->seq;
          rec_time = i*100000 + p_quote_large->lqr.qr.time;
          symbolId = p_quote_large->lqr.qr.symbolId;
          pPos += sizeof(qbdf_quote_seq_large);
          break;
        case QBDFMsgFmt::Trade:
          seq = pTrade->seq;
          rec_time = i*100000 + pTrade->tr.time;
          symbolId = pTrade->tr.symbolId;
          pPos += sizeof(qbdf_trade_seq);
          break;
        case QBDFMsgFmt::Imbalance:
          seq = pIndicative->seq;
          rec_time = i*100000 + pIndicative->ir.time;
          symbolId = pIndicative->ir.symbolId;
          pPos += sizeof(qbdf_imbalance_seq);
          break;
        case QBDFMsgFmt::Status:
          seq = pStatus->seq;
          rec_time = i*100000 + pStatus->sr.time;
          symbolId = pStatus->sr.symbolId;
          pPos += sizeof(qbdf_status_seq);
          break;
	case QBDFMsgFmt::trade_iprice32:
	  seq = p_trade_iprice32->seq;
	  rec_time = i*100000 + p_trade_iprice32->t.common.time / 1000UL;
	  symbolId = p_trade_iprice32->t.common.id;
	  pPos += sizeof(qbdf_trade_iprice32_seq);
	  break;
	case QBDFMsgFmt::quote_small_iprice32:
	  seq = p_quote_small_iprice32->seq;
	  rec_time = i*100000 + p_quote_small_iprice32->q.common.time / 1000UL;
	  symbolId = p_quote_small_iprice32->q.common.id;
	  pPos += sizeof(qbdf_quote_small_iprice32_seq);
	  break;
	case QBDFMsgFmt::quote_large_iprice32:
	  seq = p_quote_large_iprice32->seq;
	  rec_time = i*100000 + p_quote_large_iprice32->q.common.time / 1000UL;
	  symbolId = p_quote_large_iprice32->q.common.id;
	  pPos += sizeof(qbdf_quote_large_iprice32_seq);
	  break;
	case QBDFMsgFmt::imbalance_iprice32:
	  seq = p_imbalance_iprice32->seq;
	  rec_time = i*100000 + p_imbalance_iprice32->i.common.time / 1000UL;
	  symbolId = p_imbalance_iprice32->i.common.id;
	  pPos += sizeof(qbdf_imbalance_iprice32_seq);
	  break;
        default:
          if ((size_t)pHdr->format < m_qbdf_message_sizes.size()) {
            pPos += m_qbdf_message_sizes[(size_t)pHdr->format];
          } else {
            m_logger->log_printf(Log::ERROR, "%s: read invalid format [%d] in [%s]",
                                 where, pHdr->format, bin_file);
            m_logger->sync_flush();
            delete [] buffer;
            return 1;
          }
          break;
        }

        pair<map<int, SequencedRecordMap>::iterator, bool> result = taq_map.insert(make_pair(rec_time, SequencedRecordMap() ));
        map<int, SequencedRecordMap>::iterator iter = result.first;

        // Now add record to the correct Sequenced Record Map
        multimap<int, qbdf_format_header*>::iterator miter = (iter->second).insert(make_pair(seq, pHdr));
        if (miter == (iter->second).end()) {
          // this should never happen unless somehow we failed to insert
          m_logger->log_printf(Log::ERROR, "%s: Unable to add record for id=[%d] time=[%d] seq=[%d]",
                               where, symbolId, rec_time, seq);
          m_logger->sync_flush();
        }
      }
      fclose(f);

      // -----------------
      // Finished reading in all the data, now dump it back out in sorted order
      // and when we do this we remove the sequence number as we don't need to store it
      // -----------------
      char sort_bin_file[256];
      sprintf(sort_bin_file,"%s/%02d%02d00.bin", m_output_path.c_str(), hh, mm);
      f = fopen(sort_bin_file, "wb");
      if (f == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Could not open file for writing [%s]", where, sort_bin_file);
        m_logger->sync_flush();
        delete [] buffer;
        continue;
      }

      int smallQuotesThisFile = 0;
      int largeQuotesThisFile = 0;
      int tradesThisFile = 0;

      map<int, SequencedRecordMap>::iterator iter = taq_map.begin();

      while (iter != taq_map.end()) {
        multimap<int, qbdf_format_header*>::iterator miter = (iter->second).begin();
        while (miter != (iter->second).end()) {
          qbdf_format_header *pHdr =  miter->second;
          if (pHdr->format == QBDFMsgFmt::QuoteSmall) {
            fwrite(pHdr, sizeof(qbdf_quote_small), 1, f);
            smallQuotesThisFile++;

            qbdf_quote_small* p_quote = (qbdf_quote_small*)pHdr;
            uint16_t id = p_quote->qr.symbolId;
            MDPrice bid_price = MDPrice::from_fprice(p_quote->qr.bidPrice);
            MDPrice ask_price = MDPrice::from_fprice(p_quote->qr.askPrice);

            // Update summary information for 1 minute summary file
            if (!update_summary_quote(hh, mm, id, p_quote->qr.exch, p_quote->qr.time * 1000,
                                      p_quote->qr.cond, bid_price, p_quote->sizes.bidSize,
                                      ask_price, p_quote->sizes.askSize)) {
              m_logger->log_printf(Log::ERROR, "%s: Unable to update summary quote for id [%d] @ [%d]",
                                   where, id, iter->first);
              m_logger->sync_flush();
              delete [] buffer;
              return 1;
            }
          } else if (pHdr->format == QBDFMsgFmt::QuoteLarge) {
            fwrite(pHdr,sizeof(qbdf_quote_large),1,f);
            largeQuotesThisFile++;

            qbdf_quote_large* p_quote = (qbdf_quote_large*)pHdr;
            uint16_t id = p_quote->qr.symbolId;
            MDPrice bid_price = MDPrice::from_fprice(p_quote->qr.bidPrice);
            MDPrice ask_price = MDPrice::from_fprice(p_quote->qr.askPrice);

            // Update summary information for 1 minute summary file
            if (!update_summary_quote(hh, mm, id, p_quote->qr.exch, p_quote->qr.time * 1000,
                                      p_quote->qr.cond, bid_price, p_quote->sizes.bidSize,
                                      ask_price, p_quote->sizes.askSize)) {
              m_logger->sync_flush();
              delete [] buffer;
              return 1;
            }
          } else if (pHdr->format == QBDFMsgFmt::Trade) {
            fwrite(pHdr,sizeof(qbdf_trade),1,f);
            tradesThisFile++;

            qbdf_trade* p_trade = (qbdf_trade*)pHdr;
            uint16_t id = p_trade->symbolId;
            MDPrice md_price = MDPrice::from_fprice(p_trade->price);

            // Update summary information for 1 minute summary file
            if (!update_summary_trade(hh, mm, id, p_trade->exch, p_trade->time * 1000, 0,
                                      string(p_trade->cond, 4), p_trade->correct,
                                      md_price, p_trade->size)) {
              m_logger->log_printf(Log::ERROR, "%s: Unable to update summary trade for id [%d] @ [%d]",
                                   where, id, iter->first);
              m_logger->sync_flush();
              delete [] buffer;
              return 1;
            }
          } else if (pHdr->format == QBDFMsgFmt::Imbalance) {
            fwrite(pHdr,sizeof(qbdf_imbalance),1,f);

	    qbdf_imbalance* p_imbalance = (qbdf_imbalance*)pHdr;
            // Update summary information for 1 minute summary file
            if (!update_summary_imbalance(hh, mm, p_imbalance->symbolId, p_imbalance->exch, p_imbalance->time * 1000, 0)) {
              m_logger->log_printf(Log::ERROR, "%s: Unable to update summary imbalance for id [%d] @ [%d]",
                                   where, p_imbalance->symbolId, iter->first);
              m_logger->sync_flush();
              delete [] buffer;
              return 1;
            }
          } else if (pHdr->format == QBDFMsgFmt::Status) {
            fwrite(pHdr,sizeof(qbdf_status),1,f);
          } else if (pHdr->format == QBDFMsgFmt::trade_iprice32) {
	    fwrite(pHdr,sizeof(qbdf_trade_iprice32), 1, f);
	    tradesThisFile++;

            qbdf_trade_iprice32* p_trade = (qbdf_trade_iprice32*)pHdr;
            uint16_t id = p_trade->common.id;
            MDPrice md_price = MDPrice::from_iprice32(p_trade->price);

            // Update summary information for 1 minute summary file
            if (!update_summary_trade(hh, mm, id, p_trade->common.exch, p_trade->common.time, 0,
                                      string(p_trade->cond, 4), p_trade->correct,
                                      md_price, p_trade->size)) {
              m_logger->log_printf(Log::ERROR, "%s: Unable to update summary trade for id [%d] @ [%d]",
                                   where, id, iter->first);
              m_logger->sync_flush();
              delete [] buffer;
              return 1;
            }
	  } else if (pHdr->format == QBDFMsgFmt::quote_small_iprice32) {
            fwrite(pHdr, sizeof(qbdf_quote_small_iprice32), 1, f);
            smallQuotesThisFile++;

            qbdf_quote_small_iprice32* p_quote = (qbdf_quote_small_iprice32*)pHdr;
            uint16_t id = p_quote->common.id;
            MDPrice bid_price = MDPrice::from_iprice32(p_quote->bid_price);
            MDPrice ask_price = MDPrice::from_iprice32(p_quote->ask_price);

            // Update summary information for 1 minute summary file
            if (!update_summary_quote(hh, mm, id, p_quote->common.exch, p_quote->common.time,
                                      p_quote->cond, bid_price, p_quote->bid_size, ask_price, p_quote->ask_size)) {
              m_logger->log_printf(Log::ERROR, "%s: Unable to update summary quote for id [%d] @ [%d]",
                                   where, id, iter->first);
              m_logger->sync_flush();
              delete [] buffer;
              return 1;
            }
	  } else if (pHdr->format == QBDFMsgFmt::quote_large_iprice32) {
            fwrite(pHdr, sizeof(qbdf_quote_large_iprice32), 1, f);
            largeQuotesThisFile++;

            qbdf_quote_large_iprice32* p_quote = (qbdf_quote_large_iprice32*)pHdr;
            uint16_t id = p_quote->common.id;
            MDPrice bid_price = MDPrice::from_iprice32(p_quote->bid_price);
            MDPrice ask_price = MDPrice::from_iprice32(p_quote->ask_price);

            // Update summary information for 1 minute summary file
            if (!update_summary_quote(hh, mm, id, p_quote->common.exch, p_quote->common.time,
                                      p_quote->cond, bid_price, p_quote->bid_size, ask_price, p_quote->ask_size)) {
              m_logger->log_printf(Log::ERROR, "%s: Unable to update summary quote for id [%d] @ [%d]",
                                   where, id, iter->first);
              m_logger->sync_flush();
              delete [] buffer;
              return 1;
            }
          } else if (pHdr->format == QBDFMsgFmt::imbalance_iprice32) {
            fwrite(pHdr,sizeof(qbdf_imbalance_iprice32),1,f);

	    qbdf_imbalance_iprice32* p_imbalance = (qbdf_imbalance_iprice32*)pHdr;
            uint16_t id = p_imbalance->common.id;

            // Update summary information for 1 minute summary file
            if (!update_summary_imbalance(hh, mm, id, p_imbalance->common.exch, p_imbalance->common.time, 0)) {
              m_logger->log_printf(Log::ERROR, "%s: Unable to update summary imbalance for id [%d] @ [%d]",
                                   where, id, iter->first);
              m_logger->sync_flush();
              delete [] buffer;
              return 1;
            }
	  } else {
            m_logger->log_printf(Log::ERROR, "%s: Invalid format [%d] of QBDF record at time [%d]",
                                 where, pHdr->format, iter->first);
            m_logger->sync_flush();
            delete [] buffer;
            return 1;
          }
          miter++; // Next sequence number available
        }
        // This set of data are no longer needed
        (iter->second).clear();
        iter++; //Next time available
      }
      fclose(f);

      // -----------------
      //  And now dump out the summary file, which is the end state for the this time period,
      // hence is created as the next minute file name
      // -----------------
      if (dump_summ_file(hh, mm, summ_start_time, summ_end_time)) {
        m_logger->log_printf(Log::ERROR, "%s: dumping to summary file hh=[%d] mm=[%d]", where, hh, mm);
        m_logger->sync_flush();
        return 1;
      }

      // And remove data are no longer needed
      taq_map.clear();
      delete [] buffer;

      // Finally clean up the temporary unsorted
      if (remove(bin_file) != 0) {
        m_logger->log_printf(Log::ERROR, "%s: Removing temporary binary file [%s]", where, bin_file);
        m_logger->sync_flush();
        return 1;
      }
    }
    if (late_summary) {
      if (dump_summ_file(23, 58, 2359, 2359)) {
        m_logger->log_printf(Log::ERROR, "%s: Dumping to summary file hh=[23] mm=[59]", where);
        m_logger->sync_flush();
        return 1;
      }
    }
    if (dump_oc_summ_file()) {
      m_logger->log_printf(Log::ERROR, "%s: dumping to open/close summary file", where);
      m_logger->sync_flush();
      return 1;
    }

    return 0;
  }


  int QBDF_Builder::create_summ_files(const string& start_hhmm, const string & end_hhmm,
                                      const string& summ_start_hhmm, const string & summ_end_hhmm,
                                      bool late_summary = false)
  {
    const char* where = "QBDF_Builder::create_summ_files";

    m_logger->log_printf(Log::INFO, "%s: start_hhmm=%s end_hhmm=%s summ_start_hhmm=%s summ_end_hhmm=%s",
                         where, start_hhmm.c_str(), end_hhmm.c_str(),
                         summ_start_hhmm.c_str(), summ_end_hhmm.c_str());

    m_pending_inside_changed = false;

    // Ensure all open files are close
    if (close_open_files()) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to close open files", where);
      m_logger->sync_flush();
      return 1;
    }

    load_ids_file();

    if (!verify_time(start_hhmm, end_hhmm))
      return 1;

    if (!verify_time(summ_start_hhmm, summ_end_hhmm))
      return 1;

    int start_time = atoi(start_hhmm.c_str());
    int end_time = atoi(end_hhmm.c_str());
    int summ_start_time = atoi(summ_start_hhmm.c_str());
    int summ_end_time=atoi(summ_end_hhmm.c_str());

    for (int i = start_time; i <= end_time; ++i) {
      // walk through all potential files and read them
      int hh = i / 100;
      int mm = i % 100;
      if (mm == 60) {
        mm = 0;
        hh++;
        i = 100 * hh;
      }
      if (hh > 23 || hh < 0 || mm < 0 || mm > 59) {
        m_logger->log_printf(Log::ERROR, "%s: invalid time hh=[%d] mm=[%d] time=[%d]", where, hh, mm, i);
        m_logger->sync_flush();
        return 1;
      }

      char* buffer_start = 0;
      char* buffer_end = 0;
      FILE* f = 0;

      char bin_file[256];
      struct stat st_file_info;
      int stat_ret;

      sprintf(bin_file,"%s/%02d%02d00.bin.lzo", m_output_path.c_str(), hh, mm);
      stat_ret = stat(bin_file, &st_file_info);

      if (!stat_ret) {
        m_logger->log_printf(Log::DEBUG, "%s: Found LZO file: %s", where, bin_file);
        m_compression_type = "lzo";
        m_compression_util.reset(new LZO_Util(m_app, m_logger));
      } else {
        sprintf(bin_file,"%s/%02d%02d00.bin.snz", m_output_path.c_str(), hh, mm);
        stat_ret = stat(bin_file, &st_file_info);
        if (!stat_ret) {
          m_logger->log_printf(Log::DEBUG, "%s: Found Snappy file: %s", where, bin_file);
          m_compression_type = "snappy";
          m_compression_util.reset(new Snappy_Util(m_app, m_logger));
        }
      }

      if (stat_ret) {
        sprintf(bin_file, "%s/%02d%02d00.bin", m_output_path.c_str(), hh, mm);
        f = fopen(bin_file, "rb");
        if (f == 0) {
          if (i >= summ_start_time && i <= summ_end_time) {
            m_logger->log_printf(Log::WARN, "%s: Could not open bin file: %s", where, bin_file);
          } else {
            m_logger->log_printf(Log::DEBUG, "%s: Could not open bin file: %s", where, bin_file);
          }

          if (dump_summ_file(hh, mm, summ_start_time, summ_end_time)) {
            m_logger->log_printf(Log::ERROR, "%s: dumping to summary file hh=[%d] mm=[%d]", where, hh, mm);
            m_logger->sync_flush();
          }
          continue;
        }

        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);

        rewind(f);
        buffer_start = new char[file_size + 1024]; // little safety net
        size_t result = fread(buffer_start, 1, file_size, f);
        if (result != (size_t)file_size) {
          m_logger->log_printf(Log::ERROR, "%s: reading file [%s] into memory", where, bin_file);
          m_logger->sync_flush();
          delete [] buffer_start;
          continue;
        }
        buffer_end = buffer_start+file_size;
      } else {
        if (!m_compression_util->decompress_file_start(string(bin_file))) {
          m_logger->log_printf(Log::ERROR, "%s: Couldn't start decompression on file: [%s]", where, bin_file);
          return false;
        }
        if(!m_compression_util->decompress_file_get_next_block_size()) {
          m_logger->log_printf(Log::ERROR, "%s: Couldn't get decompression size on file: [%s]", where, bin_file);
          return false;
        }
        if (!m_compression_util->decompress_file_next_block(&buffer_start, &buffer_end)) {
          m_logger->log_printf(Log::ERROR, "%s: Couldn't decompress data file: [%s]", where, bin_file);
          return false;
        }
        m_compression_util->decompress_file_close();
        m_logger->log_printf(Log::DEBUG, "%s: Read %s with compression", where, bin_file);
        m_logger->sync_flush();
      }

      char* p_posn = buffer_start;
      while ( p_posn < buffer_end) {
        qbdf_format_header *pHdr = (qbdf_format_header*)p_posn;
        qbdf_level *p_level = (qbdf_level*)p_posn;
        qbdf_order_add_small *p_order_add_small = (qbdf_order_add_small*)p_posn;
        qbdf_order_add_large *p_order_add_large = (qbdf_order_add_large*)p_posn;
        qbdf_order_exec *p_order_exec = (qbdf_order_exec*)p_posn;
        qbdf_order_replace *p_order_replace = (qbdf_order_replace*)p_posn;
        qbdf_order_modify *p_order_modify = (qbdf_order_modify*)p_posn;

        switch (pHdr->format) {
        // ------
        // Quotes, either Small (most of them) or Large (less than 0.00001 %)
        // ------
        case QBDFMsgFmt::QuoteSmall: {
          qbdf_quote_small* p_quote = (qbdf_quote_small*)p_posn;
          p_posn += sizeof(qbdf_quote_small);
          uint16_t id = p_quote->qr.symbolId;
          MDPrice bid_price = MDPrice::from_fprice(p_quote->qr.bidPrice);
          MDPrice ask_price = MDPrice::from_fprice(p_quote->qr.askPrice);

          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, p_quote->qr.exch, p_quote->qr.time * 1000,
                                    p_quote->qr.cond, bid_price, p_quote->sizes.bidSize,
                                    ask_price, p_quote->sizes.askSize)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary quote for id [%d] @ [%d]",
                                 where, id, p_quote->qr.time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::QuoteLarge: {
          qbdf_quote_large* p_quote = (qbdf_quote_large*)pHdr;
          p_posn += sizeof(qbdf_quote_large);
          uint16_t id = p_quote->qr.symbolId;
          MDPrice bid_price = MDPrice::from_fprice(p_quote->qr.bidPrice);
          MDPrice ask_price = MDPrice::from_fprice(p_quote->qr.askPrice);

          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, p_quote->qr.exch, p_quote->qr.time * 1000,
                                    p_quote->qr.cond, bid_price, p_quote->sizes.bidSize,
                                    ask_price, p_quote->sizes.askSize)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary quote for id [%d] @ [%d]",
                                 where, id, p_quote->qr.time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::quote_small_iprice32: {
          qbdf_quote_small_iprice32* p_quote = (qbdf_quote_small_iprice32*)p_posn;
          p_posn += sizeof(qbdf_quote_small_iprice32);

          uint16_t id = p_quote->common.id;
          MDPrice bid_price = MDPrice::from_iprice32(p_quote->bid_price);
          MDPrice ask_price = MDPrice::from_iprice32(p_quote->ask_price);

          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, p_quote->common.exch, p_quote->common.time,
                                    p_quote->cond, bid_price, p_quote->bid_size,
                                    ask_price, p_quote->ask_size)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary quote for id [%d] @ [%lu]",
                                 where, id, (p_quote->common.time / Time_Constants::ticks_per_micro));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::quote_large_iprice32: {
          qbdf_quote_large_iprice32* p_quote = (qbdf_quote_large_iprice32*)p_posn;
          p_posn += sizeof(qbdf_quote_large_iprice32);

          uint16_t id = p_quote->common.id;
          MDPrice bid_price = MDPrice::from_iprice32(p_quote->bid_price);
          MDPrice ask_price = MDPrice::from_iprice32(p_quote->ask_price);

          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, p_quote->common.exch, p_quote->common.time,
                                    p_quote->cond, bid_price, p_quote->bid_size,
                                    ask_price, p_quote->ask_size)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary quote for id [%d] @ [%lu]",
                                 where, id, (p_quote->common.time / Time_Constants::ticks_per_micro));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::quote_iprice64: {
          qbdf_quote_iprice64* p_quote = (qbdf_quote_iprice64*)p_posn;
          p_posn += sizeof(qbdf_quote_iprice64);

          uint16_t id = p_quote->common.id;
          MDPrice bid_price = MDPrice::from_iprice64(p_quote->bid_price);
          MDPrice ask_price = MDPrice::from_iprice64(p_quote->ask_price);

          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, p_quote->common.exch, p_quote->common.time,
                                    p_quote->cond, bid_price, p_quote->bid_size,
                                    ask_price, p_quote->ask_size)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary quote for id [%d] @ [%lu]",
                                 where, id, (p_quote->common.time / Time_Constants::ticks_per_micro));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::Trade: {
          qbdf_trade* p_trade = (qbdf_trade*)p_posn;
          p_posn += sizeof(qbdf_trade);

          uint16_t id = p_trade->symbolId;
          MDPrice md_price = MDPrice::from_fprice(p_trade->price);

          // Update summary information for 1 minute summary file
          if (!update_summary_trade(hh, mm, id, p_trade->exch, p_trade->time * 1000, 0,
                                    string(p_trade->cond, 4), p_trade->correct,
                                    md_price, p_trade->size)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary trade for id [%d] @ [%d]",
                                 where, id, p_trade->time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::trade_iprice32: {
          qbdf_trade_iprice32* p_trade = (qbdf_trade_iprice32*)p_posn;
          p_posn += sizeof(qbdf_trade_iprice32);

          uint16_t id = p_trade->common.id;
          MDPrice md_price = MDPrice::from_iprice32(p_trade->price);

          // Update summary information for 1 minute summary file
          if (!update_summary_trade(hh, mm, id, p_trade->common.exch, p_trade->common.time,
                                    p_trade->common.exch_time_delta, string(p_trade->cond, 4),
                                    p_trade->correct, md_price, p_trade->size)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary trade for id [%d] @ [%lu]",
                                 where, id, (p_trade->common.time / Time_Constants::ticks_per_micro));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::trade_iprice64: {
          qbdf_trade_iprice64* p_trade = (qbdf_trade_iprice64*)p_posn;
          p_posn += sizeof(qbdf_trade_iprice64);

          uint16_t id = p_trade->common.id;
          MDPrice md_price = MDPrice::from_iprice64(p_trade->price);

          // Update summary information for 1 minute summary file
          if (!update_summary_trade(hh, mm, id, p_trade->common.exch, p_trade->common.time,
                                    p_trade->common.exch_time_delta, string(p_trade->cond, 4),
                                    p_trade->correct, md_price, p_trade->size)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary trade for id [%d] @ [%lu]",
                                 where, id, (p_trade->common.time / Time_Constants::ticks_per_micro));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::Imbalance: {
          qbdf_imbalance* p_imbalance = (qbdf_imbalance*)p_posn;
          p_posn += sizeof(qbdf_imbalance);

          uint16_t id = p_imbalance->symbolId;
          if (!update_summary_imbalance(hh, mm, id, p_imbalance->exch, p_imbalance->time * 1000, 0)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary imbalance for id [%d] @ [%d]",
                                 where, id, p_imbalance->time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::imbalance_iprice32: {
          qbdf_imbalance_iprice32* p_imbalance = (qbdf_imbalance_iprice32*)p_posn;
          p_posn += sizeof(qbdf_imbalance_iprice32);

          uint16_t id = p_imbalance->common.id;
          if (!update_summary_imbalance(hh, mm, id, p_imbalance->common.exch, p_imbalance->common.time,
					p_imbalance->common.exch_time_delta)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary imbalance for id [%d] @ [%lu]",
                                 where, id, (p_imbalance->common.time / Time_Constants::ticks_per_micro));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::imbalance_iprice64: {
          qbdf_imbalance_iprice64* p_imbalance = (qbdf_imbalance_iprice64*)p_posn;
          p_posn += sizeof(qbdf_imbalance_iprice64);

          uint16_t id = p_imbalance->common.id;
          if (!update_summary_imbalance(hh, mm, id, p_imbalance->common.exch, p_imbalance->common.time,
					p_imbalance->common.exch_time_delta)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update summary imbalance for id [%d] @ [%lu]",
                                 where, id, (p_imbalance->common.time / Time_Constants::ticks_per_micro));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::OrderAddSmall: {
          p_posn += sizeof(qbdf_order_add_small);
          qbdf_order_add_price* p_order_add_price = &p_order_add_small->pr;
          uint32_t size = p_order_add_small->sz.size;

          if (!update_book_order_add(hh, mm, p_order_add_price, size)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (order add) for id [%d] @ [%d]",
                                 where, p_order_add_price->symbol_id, p_order_add_price->time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::OrderAddLarge:
        {
          p_posn += sizeof(qbdf_order_add_large);
          qbdf_order_add_price* p_order_add_price = &p_order_add_large->pr;
          uint32_t size = p_order_add_large->sz.size;

          if (!update_book_order_add(hh, mm, p_order_add_price, size)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (order add) for id [%d] @ [%d]",
                                 where, p_order_add_price->symbol_id, p_order_add_price->time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::OrderExec: {
          p_posn += sizeof(qbdf_order_exec);
          if (!update_book_order_exec(hh, mm, p_order_exec)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (order exec) for id [%d] @ [%d]",
                                 where, p_order_exec->symbol_id, p_order_exec->time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::OrderReplace: {
          p_posn += sizeof(qbdf_order_replace);
          if (!update_book_order_replace(hh, mm, p_order_replace)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (order replace) for id [%d] @ [%d]",
                                 where, p_order_replace->symbol_id, p_order_replace->time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::OrderModify: {
          p_posn += sizeof(qbdf_order_modify);
          if (!update_book_order_modify(hh, mm, p_order_modify)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (order modify) for id [%d] @ [%d]",
                                 where, p_order_modify->symbol_id, p_order_modify->time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::GapMarker: {
          p_posn += sizeof(qbdf_gap_marker);

          if (m_order_books.size() > 0) {
	    for (vector<order_book_rp>::iterator it  = m_order_books.begin(); it != m_order_books.end(); ++it) {
	      if (*it) {
		(*it)->clear(false);
		m_logger->log_printf(Log::INFO, "GAP: in exch=%s", (*it)->exch().str());
	      }
	    }
            //m_order_book->clear(false);
            //m_logger->log_printf(Log::INFO, "GAP: in exch=%s", m_order_book->exch().str());
            break;
          }
        }
        case QBDFMsgFmt::gap_marker_micros: {
          qbdf_gap_marker_micros* gap = reinterpret_cast<qbdf_gap_marker_micros*>(p_posn);
          p_posn += sizeof(qbdf_gap_marker_micros);

          if (gap->common.id) {
            MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)gap->common.exch]->primary_id().index()).get();
            ob->clear(gap->common.id, true);
          } else if (m_order_books.size() > 0) {
	    for (vector<order_book_rp>::iterator it  = m_order_books.begin(); it != m_order_books.end(); ++it) {
	      if (*it) {
		(*it)->clear(false);
		m_logger->log_printf(Log::INFO, "GAP: in exch=%s", (*it)->exch().str());
	      }
	    }
            break;
          }
        }
        break;
        case QBDFMsgFmt::Level: {
          p_posn += sizeof(qbdf_level);
          if (!update_book_level(hh, mm, p_level)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (level) for id [%d] @ [%d]",
                                 where, p_level->symbol_id, p_level->time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::Status: {
          m_logger->log_printf(Log::DEBUG, "%s: Got qbdf_status record!", where);
          p_posn += sizeof(qbdf_status);
          break;
        }
        case QBDFMsgFmt::order_add_iprice32: {
          qbdf_order_add_iprice32* p_order = (qbdf_order_add_iprice32*)p_posn;
          p_posn += sizeof(qbdf_order_add_iprice32);
          if (!update_book_order_add_iprice32(hh, mm, p_order)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (order add usec), id [%d] @ [%d]",
                                 where, p_order->common.id, p_order->common.time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::order_exec_iprice32: {
          qbdf_order_exec_iprice32* p_order = (qbdf_order_exec_iprice32*)p_posn;
          p_posn += sizeof(qbdf_order_exec_iprice32);
          if (!update_book_order_exec_iprice32(hh, mm, p_order)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (order exec usec), id [%d] @ [%d]",
                                 where, p_order->common.id, p_order->common.time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::order_replace_iprice32: {
          qbdf_order_replace_iprice32* p_order = (qbdf_order_replace_iprice32*)p_posn;
          p_posn += sizeof(qbdf_order_replace_iprice32);
          if (!update_book_order_replace_iprice32(hh, mm, p_order)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (order replace usec), id [%d] @ [%d]",
                                 where, p_order->common.id, p_order->common.time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::order_modify_iprice32: {
          qbdf_order_modify_iprice32* p_order = (qbdf_order_modify_iprice32*)p_posn;
          p_posn += sizeof(qbdf_order_modify_iprice32);
          if (!update_book_order_modify_iprice32(hh, mm, p_order)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (order add usec), id [%d] @ [%d]",
                                 where, p_order->common.id, p_order->common.time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::level_iprice32: {
          qbdf_level_iprice32* p_level = (qbdf_level_iprice32*)p_posn;
          p_posn += sizeof(qbdf_level_iprice32);
          if (!update_book_level_iprice32(hh, mm, p_level)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (level_iprice64), id [%d] @ [%d]",
                                 where, p_level->common.id, p_level->common.time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;

        }
        case QBDFMsgFmt::level_iprice64: {
          qbdf_level_iprice64* p_level = (qbdf_level_iprice64*)p_posn;
          p_posn += sizeof(qbdf_level_iprice64);
          if (!update_book_level_iprice64(hh, mm, p_level)) {
            m_logger->log_printf(Log::ERROR, "%s: unable to update book (level_iprice64), id [%d] @ [%d]",
                                 where, p_level->common.id, p_level->common.time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;

        }
        default: {
          if ((size_t)pHdr->format < m_qbdf_message_sizes.size()) {
            p_posn += m_qbdf_message_sizes[(size_t)pHdr->format];
          } else {
            m_logger->log_printf(Log::ERROR, "%s: invalid format [%d] of QBDF record", where, pHdr->format);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        }
      }
      if (stat_ret) {
        fclose(f);
        delete [] buffer_start;
      }

      // -----------------
      // And now dump out the summary file, which is the end state for the this time period,
      // hence is created as the next minute file name
      // -----------------
      if (dump_summ_file(hh, mm, summ_start_time, summ_end_time)) {
        m_logger->log_printf(Log::ERROR, "%s: dumping to summary file hh=[%d] mm=[%d]", where, hh, mm);
        m_logger->sync_flush();
        return 1;
      }
    }

    if (late_summary) {
      if (dump_summ_file(23, 58, 2359, 2359)) {
        m_logger->log_printf(Log::ERROR, "%s: dumping to summary file hh=[23] mm=[59]", where);
        m_logger->sync_flush();
        return 1;
      }
    }

    if (dump_oc_summ_file()) {
      m_logger->log_printf(Log::ERROR, "%s: dumping to open/close summary file", where);
      m_logger->sync_flush();
      return 1;
    }

    m_logger->sync_flush();
    return 0;
  }


  bool QBDF_Builder::update_book_level(int hh, int mm, qbdf_level* p_level)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)p_level->exch]->primary_id().index()).get();
    if (!ob)
      return false;

    int rec_time = (hh * 3600000) + (mm * 60000) + p_level->time;
    Time m_time = Time(m_midnight + msec(rec_time));

    char side = p_level->side;
    MDPrice iprice = MDPrice::from_fprice(p_level->price);

    bool executed = p_level->link_id_1 != 0;
    uint8_t last_in_group = p_level->last_in_group;

    if(p_level->size != 0) {
      m_pending_inside_changed |= ob->modify_level_ip(m_time, p_level->symbol_id, side, iprice,
                                                                p_level->size, p_level->num_orders, executed);
    } else {
      m_pending_inside_changed |= ob->remove_level_ip(m_time, p_level->symbol_id, side, iprice, executed);
    }

    if (m_pending_inside_changed && last_in_group) {
      m_pending_inside_changed = false;
      QuoteUpdate quote_update;
      quote_update.m_id = p_level->symbol_id;
      quote_update.m_exch = ob->exch();
      uint32_t ms_since_midnight = ((hh*3600 + mm*60) * 1000) + p_level->time;
      quote_update.set_time(Time(m_midnight + msec(ms_since_midnight)));
      ob->fill_quote_update(p_level->symbol_id, quote_update);
      update_summary_quote(hh, mm, p_level->symbol_id, p_level->exch, p_level->time * 1000, ' ',
                           MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                           MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_level_iprice32(int hh, int mm, qbdf_level_iprice32* p_level)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)p_level->common.exch]->primary_id().index()).get();
    if (!ob)
      return false;

    uint64_t micros_since_midnight = \
    ((hh * micros_per_hr) + (mm * micros_per_min) + p_level->common.time);
    Time m_time = Time(m_midnight + usec(micros_since_midnight));

    char side = get_side(*p_level);
    uint8_t executed = get_executed(*p_level);
    uint8_t last_in_group = get_last_in_group(*p_level);
    MDPrice md_price = MDPrice::from_iprice32(p_level->price);

    if(p_level->size != 0) {
      m_pending_inside_changed |= ob->modify_level_ip(m_time, p_level->common.id, side, md_price,
                                                                p_level->size, p_level->num_orders, executed);
    } else {
      m_pending_inside_changed |= ob->remove_level_ip(m_time, p_level->common.id, side, md_price, executed);
    }

    if (m_pending_inside_changed && last_in_group) {
      m_pending_inside_changed = false;
      QuoteUpdate quote_update;
      quote_update.m_id = p_level->common.id;
      quote_update.m_exch = ob->exch();
      quote_update.set_time(m_time);
      ob->fill_quote_update(p_level->common.id, quote_update);
      update_summary_quote(hh, mm, p_level->common.id, p_level->common.exch, p_level->common.time, ' ',
                           MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                           MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_level_iprice64(int hh, int mm, qbdf_level_iprice64* p_level)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)p_level->common.exch]->primary_id().index()).get();
    if (!ob)
      return false;

    if (p_level->common.exch == 'T' and p_level->quote_cond == 'C') {
      // Skip close level updates
      return true;
    }

    uint64_t micros_since_midnight = \
    ((hh * micros_per_hr) + (mm * micros_per_min) + p_level->common.time);
    Time m_time = Time(m_midnight + usec(micros_since_midnight));

    char side = get_side(*p_level);
    uint8_t executed = get_executed(*p_level);
    uint8_t last_in_group = get_last_in_group(*p_level);
    MDPrice md_price = MDPrice::from_iprice64(p_level->price);

    if(p_level->size != 0) {
      m_pending_inside_changed |= ob->modify_level_ip(m_time, p_level->common.id, side, md_price,
                                                                p_level->size, p_level->num_orders, executed);
    } else {
      m_pending_inside_changed |= ob->remove_level_ip(m_time, p_level->common.id, side, md_price, executed);
    }

    if (m_pending_inside_changed && last_in_group) {
      m_pending_inside_changed = false;
      QuoteUpdate quote_update;
      quote_update.m_id = p_level->common.id;
      quote_update.m_exch = ob->exch();
      quote_update.set_time(m_time);
      ob->fill_quote_update(p_level->common.id, quote_update);
      update_summary_quote(hh, mm, p_level->common.id, p_level->common.exch, p_level->common.time, ' ',
                           MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                           MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_order_add(int hh, int mm, qbdf_order_add_price* p_order_add_price, uint32_t size)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)p_order_add_price->exch]->primary_id().index()).get();
    if (!ob)
      return false;

    int rec_time = (hh * 3600000) + (mm * 60000) + p_order_add_price->time;
    Time m_time = Time(m_midnight + msec(rec_time));

    MDPrice iprice = MDPrice::from_fprice(p_order_add_price->price);

    bool inside_changed = ob->unlocked_add_order_ip(m_time, p_order_add_price->order_id,
                                                              p_order_add_price->symbol_id,
                                                              p_order_add_price->side, iprice, size);

    if (inside_changed) {
      QuoteUpdate quote_update;
      quote_update.m_id = p_order_add_price->symbol_id;
      quote_update.m_exch = ob->exch();
      uint32_t ms_since_midnight = ((hh*3600 + mm*60) * 1000) + p_order_add_price->time;
      quote_update.set_time(Time(m_midnight + msec(ms_since_midnight)));
      ob->fill_quote_update(p_order_add_price->symbol_id, quote_update);
      update_summary_quote(hh, mm, p_order_add_price->symbol_id, p_order_add_price->exch,
                           p_order_add_price->time * 1000, ' ',
                           MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                           MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_order_add_iprice32(int hh, int mm, qbdf_order_add_iprice32* p_order)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)p_order->common.exch]->primary_id().index()).get();
    if (!ob)
      return false;

    uint64_t micros_since_midnight = \
    ((hh * micros_per_hr) + (mm * micros_per_min) + p_order->common.time);
    Time m_time = Time(m_midnight + usec(micros_since_midnight));

    char side = get_side(*p_order);
    bool inside_changed = ob->unlocked_add_order_ip(m_time, p_order->order_id,
                                                              p_order->common.id, side,
                                                              MDPrice::from_iprice32(p_order->price),
                                                              p_order->size);

    if (inside_changed) {
      QuoteUpdate quote_update;
      quote_update.m_id = p_order->common.id;
      quote_update.m_exch = ob->exch();
      quote_update.set_time(m_time);
      ob->fill_quote_update(p_order->common.id, quote_update);
      update_summary_quote(hh, mm, p_order->common.id, p_order->common.exch, p_order->common.time, ' ',
                           MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                           MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_order_exec(int hh, int mm, qbdf_order_exec* p_order_exec)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)p_order_exec->exch]->primary_id().index()).get();
    if (!ob)
      return false;

    int rec_time = (hh * 3600000) + (mm * 60000) + p_order_exec->time;
    Time m_time = Time(m_midnight + msec(rec_time));

    double price = p_order_exec->price;

    MDOrder* order = ob->unlocked_find_order(p_order_exec->order_id);
    if(order) {
      uint32_t new_size = order->size - p_order_exec->size;
      bool inside_changed = order->is_top_level();
      if(price == 0.0) {
        price = order->price_as_double();
      }

      if(new_size > 0) {
        ob->unlocked_modify_order(m_time, order, new_size, p_order_exec->size);
      } else {
        ob->unlocked_remove_order(m_time, order, p_order_exec->size);
      }

      char blank_cond[4];
      memset(blank_cond, ' ', 4);
      if (inside_changed) {
        QuoteUpdate quote_update;
        quote_update.m_id = p_order_exec->symbol_id;
        quote_update.m_exch = ob->exch();
        uint32_t ms_since_midnight = ((hh*3600 + mm*60) * 1000) + p_order_exec->time;
        quote_update.set_time(Time(m_midnight + msec(ms_since_midnight)));
        ob->fill_quote_update(p_order_exec->symbol_id, quote_update);
        update_summary_quote(hh, mm, p_order_exec->symbol_id, p_order_exec->exch,
                             p_order_exec->time * 1000, ' ',
                             MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                             MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
      }

      TradeUpdate trade_update;
      trade_update.m_id = p_order_exec->symbol_id;
      trade_update.m_exch = ob->exch();
      uint32_t ms_since_midnight = ((hh*3600 + mm*60) * 1000) + p_order_exec->time;
      trade_update.set_time(Time(m_midnight + msec(ms_since_midnight)));
      trade_update.m_price = price;
      trade_update.m_size = p_order_exec->size;
      trade_update.m_side = order->flip_side();
      trade_update.m_correct = 0;
      memcpy(trade_update.m_trade_qualifier, "    ", sizeof(trade_update.m_trade_qualifier));
      memset(trade_update.m_misc, 0, sizeof(trade_update.m_misc));
      update_summary_trade(hh, mm, trade_update.m_id, p_order_exec->exch, p_order_exec->time * 1000, 0,
                           string(trade_update.m_trade_qualifier, sizeof(trade_update.m_trade_qualifier)),
                           trade_update.m_correct, MDPrice::from_fprice(price), trade_update.m_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_order_exec_iprice32(int hh, int mm, qbdf_order_exec_iprice32* p_order)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)p_order->common.exch]->primary_id().index()).get();
    if (!ob)
      return false;

    uint64_t micros_since_midnight = \
    ((hh * micros_per_hr) + (mm * micros_per_min) + p_order->common.time);
    Time m_time = Time(m_midnight + usec(micros_since_midnight));

    bool order_found = false;
    bool inside_changed = false;

    MDPrice price = MDPrice::from_iprice32(p_order->price);
    MDOrder* order = ob->unlocked_find_order(p_order->order_id);
    if (order) {
      order_found = true;
      uint32_t new_size = order->size - p_order->size;
      inside_changed = order->is_top_level();
      if(price.blank()) {
        price = order->price;
      }

      if(new_size > 0) {
        ob->unlocked_modify_order(m_time, order, new_size, p_order->size);
      } else {
        ob->unlocked_remove_order(m_time, order, p_order->size);
      }
    }

    if (order_found) {
      if (inside_changed) {
        QuoteUpdate quote_update;
        quote_update.m_id = p_order->common.id;
        quote_update.m_exch = ob->exch();
        quote_update.set_time(m_time);
        ob->fill_quote_update(p_order->common.id, quote_update);
        update_summary_quote(hh, mm, p_order->common.id, p_order->common.exch, p_order->common.time, ' ',
                             MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                             MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
      }

      char cond[4];
      memset(cond, ' ', 4);
      cond[0] = (char)p_order->cond;

      TradeUpdate trade_update;
      trade_update.m_id = p_order->common.id;
      trade_update.m_exch = ob->exch();
      trade_update.set_time(m_time);
      trade_update.m_price = price.fprice();
      trade_update.m_size = p_order->size;
      trade_update.m_side = (get_side(*p_order) == 'B' ? 'S' : 'B');
      trade_update.m_correct = 0;
      memcpy(trade_update.m_trade_qualifier, cond, sizeof(trade_update.m_trade_qualifier));
      memset(trade_update.m_misc, 0, sizeof(trade_update.m_misc));
      update_summary_trade(hh, mm, trade_update.m_id, p_order->common.exch,
                           p_order->common.time, p_order->common.exch_time_delta,
                           string(trade_update.m_trade_qualifier, sizeof(trade_update.m_trade_qualifier)),
                           trade_update.m_correct, price, trade_update.m_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_order_replace(int hh, int mm, qbdf_order_replace* p_order_replace)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_hf_exch_id.index()).get();
    if (!ob)
      return false;

    int rec_time = (hh * 3600000) + (mm * 60000) + p_order_replace->time;
    Time m_time = Time(m_midnight + msec(rec_time));

    bool inside_changed = false;

    MDOrder* orig_order = ob->unlocked_find_order(p_order_replace->orig_order_id);
    if(orig_order) {
      inside_changed = orig_order->is_top_level();
      char side = orig_order->side();

      ob->unlocked_remove_order(m_time, orig_order);
      MDPrice iprice = MDPrice::from_fprice(p_order_replace->price);
      inside_changed |= ob->unlocked_add_order_ip(m_time, p_order_replace->new_order_id,
						  p_order_replace->symbol_id, side, iprice,
						  p_order_replace->size);
    }

    if (inside_changed) {
      QuoteUpdate quote_update;
      quote_update.m_id = p_order_replace->symbol_id;
      quote_update.m_exch = ob->exch();
      uint32_t ms_since_midnight = ((hh*3600 + mm*60) * 1000) + p_order_replace->time;
      quote_update.set_time(Time(m_midnight + msec(ms_since_midnight)));
      ob->fill_quote_update(p_order_replace->symbol_id, quote_update);
      update_summary_quote(hh, mm, p_order_replace->symbol_id, m_hf_exch_id_to_qbdf_exch[quote_update.m_exch],
                           p_order_replace->time * 1000, ' ',
                           MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                           MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_order_replace_iprice32(int hh, int mm, qbdf_order_replace_iprice32* p_order)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)p_order->common.exch]->primary_id().index()).get();
    if (!ob)
      return false;

    uint64_t micros_since_midnight = \
    ((hh * micros_per_hr) + (mm * micros_per_min) + p_order->common.time);
    Time m_time = Time(m_midnight + usec(micros_since_midnight));

    bool inside_changed = false;
    MDOrder* orig_order = ob->unlocked_find_order(p_order->orig_order_id);
    if (orig_order) {
      inside_changed = orig_order->is_top_level();
      char side = orig_order->side();

      ob->unlocked_remove_order(m_time, orig_order);
      inside_changed |= ob->unlocked_add_order_ip(m_time, p_order->new_order_id,
                                                            p_order->common.id, side,
                                                            MDPrice::from_iprice32(p_order->price),
                                                            p_order->size);
    }

    if (inside_changed) {
      QuoteUpdate quote_update;
      quote_update.m_id = p_order->common.id;
      quote_update.m_exch = ob->exch();
      quote_update.set_time(m_time);
      ob->fill_quote_update(p_order->common.id, quote_update);
      update_summary_quote(hh, mm, p_order->common.id, p_order->common.exch, p_order->common.time, ' ',
                           MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                           MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_order_modify(int hh, int mm, qbdf_order_modify* p_order_modify)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_hf_exch_id.index()).get();
    if (!ob)
      return false;

    int rec_time = (hh * 3600000) + (mm * 60000) + p_order_modify->time;
    Time m_time = Time(m_midnight + msec(rec_time));

    bool inside_changed = false;

    MDOrder* order = ob->unlocked_find_order(p_order_modify->orig_order_id);
    if(order) {
      inside_changed = order->is_top_level();
      uint32_t new_size = p_order_modify->new_size;
      if(new_size > 0) {
        char side = p_order_modify->new_side;
        MDPrice iprice = MDPrice::from_fprice(p_order_modify->new_price);

        if (iprice == order->price) {
          ob->unlocked_modify_order(m_time, order, new_size);
        } else {
          ob->unlocked_modify_order_ip(m_time, order, side, iprice, new_size);
        }
      } else {
        ob->unlocked_remove_order(m_time, order);
      }
    }

    if (inside_changed) {
      QuoteUpdate quote_update;
      quote_update.m_id = p_order_modify->symbol_id;
      quote_update.m_exch = ob->exch();
      uint32_t ms_since_midnight = ((hh*3600 + mm*60) * 1000) + p_order_modify->time;
      quote_update.set_time(Time(m_midnight + msec(ms_since_midnight)));
      ob->fill_quote_update(p_order_modify->symbol_id, quote_update);
      update_summary_quote(hh, mm, p_order_modify->symbol_id, m_hf_exch_id_to_qbdf_exch[quote_update.m_exch],
                           p_order_modify->time * 1000, ' ',
                           MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                           MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
    }
    return true;
  }


  bool QBDF_Builder::update_book_order_modify_iprice32(int hh, int mm, qbdf_order_modify_iprice32* p_order)
  {
    MDOrderBookHandler* ob = m_order_books.at(m_qbdf_exch_to_hf_exch[(int)p_order->common.exch]->primary_id().index()).get();
    if (!ob)
      return false;
   
    uint64_t micros_since_midnight = \
    ((hh * micros_per_hr) + (mm * micros_per_min) + p_order->common.time);
    Time m_time = Time(m_midnight + usec(micros_since_midnight));

    bool inside_changed = false;
    MDOrder* order = ob->unlocked_find_order(p_order->order_id);
    if(order) {
      inside_changed = order->is_top_level();
      uint32_t new_size = p_order->size;
      if(new_size > 0) {
        char side = get_side(*p_order);
        MDPrice price = MDPrice::from_iprice32(p_order->price);
        if (price == order->price) {
          ob->unlocked_modify_order(m_time, order, new_size);
        } else {
          ob->unlocked_modify_order_ip(m_time, order, side, price, new_size);
        }
      } else {
        ob->unlocked_remove_order(m_time, order);
      }
    }

    if (inside_changed) {
      QuoteUpdate quote_update;
      quote_update.m_id = p_order->common.id;
      quote_update.m_exch = ob->exch();
      quote_update.set_time(m_time);
      ob->fill_quote_update(p_order->common.id, quote_update);
      update_summary_quote(hh, mm, p_order->common.id, p_order->common.exch, p_order->common.time, ' ',
                           MDPrice::from_fprice(quote_update.m_bid), quote_update.m_bid_size,
                           MDPrice::from_fprice(quote_update.m_ask), quote_update.m_ask_size);
    }
    return true;
  }

  // ----------------------------------------------------------------
  // Update the summary information based on the incoming quote
  // ----------------------------------------------------------------
  bool QBDF_Builder::update_summary_quote(int hh, int mm, uint16_t id, char exch, uint32_t micros,
                                          char cond, MDPrice bid_price, uint32_t bid_size,
                                          MDPrice ask_price, uint32_t ask_size)
  {
    // Update summary information for 1 minute summary file
    if (m_summary_vec.size() <= id) {
      m_logger->log_printf(Log::ERROR, "%s: Summary cache only has %lu entries, but looking for id %d",
                           "QBDF_Builder::update_summary_quote", m_summary_vec.size(), id);
      m_logger->sync_flush();
      return false;
    }

    qbdf_summary_cache_rp cache = m_summary_vec.at(id);
    if (!cache) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to find cache record for id [%d]",
                           "QBDF_Builder::update_summary_quote", id);
      m_logger->sync_flush();
      return false;
    }

    qbdf_summary_rp summ = cache->cached_by_exch[(int)exch];
    if (!summ) {
      summ = qbdf_summary_rp(new qbdf_summary);
      summ->hdr.format = QBDFMsgFmt::Summary;
      summ->symbolId = id;
      summ->exch = exch;
      cache->cached_by_exch[(int)exch] = summ;
    }
    uint64_t micros_since_midnight = (hh*3600000000UL) + (mm*60000000UL) + micros;

    QuoteUpdate quote_update;
    quote_update.m_id = id;
    if (m_qbdf_exch_to_hf_exch[(int)exch]) {
      quote_update.m_exch = m_qbdf_exch_to_hf_exch[(int)exch]->primary_id();
    }

    quote_update.m_bid = bid_price.fprice();
    quote_update.m_ask = ask_price.fprice();
    quote_update.m_bid_size = bid_size;
    quote_update.m_ask_size = ask_size;
    quote_update.m_quote_status = *(boost::optional<QuoteStatus>) QuoteStatus::get_by_index(summ->quote_status);
    quote_update.m_cond = cond;

    MarketStatusUpdate status_update;
    status_update.m_market_status = *(boost::optional<MarketStatus>) MarketStatus::get_by_index(summ->market_status);
    m_feed_rules->apply_quote_conditions(quote_update, status_update);

    // Now record summary quote info
    summ->bidPrice = quote_update.m_bid;
    summ->askPrice = quote_update.m_ask;
    summ->bidSize = quote_update.m_bid_size;
    summ->askSize = quote_update.m_ask_size;
    summ->quote_cond = quote_update.m_cond;
    summ->quote_status  = quote_update.m_quote_status.index();
    summ->market_status = status_update.m_market_status.index();
    summ->quote_time = micros_since_midnight / 1000; // store as millis since midnight

    return true;
  }


  bool QBDF_Builder::update_summary_trade(int hh, int mm, uint16_t id, char exch,
                                          uint32_t micros_recv_time, int32_t exch_time_delta,
                                          string cond, char correct, MDPrice md_price,
                                          uint32_t size)
  {
    const char* where = "QBDF_Builder::update_summary_trade";

    // Update summary information for 1 minute summary file
    if (m_summary_vec.size() <= id) {
      m_logger->log_printf(Log::ERROR, "%s: Summary cache only has %lu entries, but looking for id %d",
                           where, m_summary_vec.size(), id);
      m_logger->sync_flush();
      return false;
    }

    qbdf_summary_cache_rp cache = m_summary_vec.at(id);
    if (!cache) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to find cache record for id [%d]", where, id);
      m_logger->sync_flush();
      return false;
    }

    qbdf_summary_rp summ = cache->cached_by_exch[(int)exch];
    if (!summ) {
      summ = qbdf_summary_rp(new qbdf_summary);
      summ->hdr.format = QBDFMsgFmt::Summary;
      summ->symbolId = id;
      summ->exch = exch;
      cache->cached_by_exch[(int)exch] = summ;
    }

    uint64_t micros_recv_time_since_midnight = (hh * Time_Constants::micros_per_hr) + \
    (mm * Time_Constants::micros_per_min) + micros_recv_time;
    uint64_t micros_exch_time_since_midnight = micros_recv_time_since_midnight + exch_time_delta;

    Time recv_time(m_midnight + usec(micros_recv_time_since_midnight));
    Time exch_time(m_midnight + usec(micros_exch_time_since_midnight));

    TradeUpdate trade_update;
    trade_update.set_time(exch_time);
    if (m_qbdf_exch_to_hf_exch[(int)exch]) {
      trade_update.m_exch = m_qbdf_exch_to_hf_exch[(int)exch]->primary_id();
    }
    trade_update.m_id = id;
    trade_update.m_price = md_price.fprice();
    trade_update.m_size = size;
    trade_update.m_trade_status = *(boost::optional<TradeStatus>) TradeStatus::get_by_index(summ->trade_status);
    trade_update.m_market_status = *(boost::optional<MarketStatus>) MarketStatus::get_by_index(summ->market_status);
    memcpy(trade_update.m_trade_qualifier, cond.c_str(), sizeof(trade_update.m_trade_qualifier));
    trade_update.m_correct = correct;

    MarketStatusUpdate status_update;
    status_update.m_market_status = trade_update.m_market_status;
    m_feed_rules->apply_trade_conditions(trade_update, status_update);
    MarketSession session = m_feed_rules->get_session(trade_update);

    if (trade_update.m_trade_status == TradeStatus::New ||
        trade_update.m_trade_status == TradeStatus::Cross ||
        trade_update.m_trade_status == TradeStatus::OpenTrade ||
        trade_update.m_trade_status == TradeStatus::CloseTrade) {
      summ->price = trade_update.m_price;
      summ->size = trade_update.m_size;
      if (trade_update.m_price > summ->high)
        summ->high = trade_update.m_price;
      if (trade_update.m_price < summ->low)
        summ->low = trade_update.m_price;
    }

    if (trade_update.m_trade_status == TradeStatus::OpenTrade ||
        trade_update.m_trade_status == TradeStatus::CloseTrade) {
      qbdf_oc_summary_cache_rp oc_cache = m_oc_summary_vec.at(id);
      if (!oc_cache) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to find open/close rec for id [%d] at [%s]\n",
                             where, id, recv_time.to_string().c_str());
        m_logger->sync_flush();
        return false;
      }

      qbdf_oc_summary_rp oc_summ = oc_cache->cached_by_exch[(int)exch];
      if (!oc_summ) {
        oc_summ = qbdf_oc_summary_rp(new qbdf_oc_summary);
        oc_summ->hdr.format = QBDFMsgFmt::OpenCloseSummary;
        oc_summ->symbolId = id;
        oc_summ->exch = exch;
        oc_cache->cached_by_exch[(int)exch] = oc_summ;
      }

      qbdf_oc_summary_multi_cache_rp oc_multi_cache = m_oc_summary_multi_vec.at(id);
      if (!oc_multi_cache) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to find multi-session open/close rec for id [%d] at [%s]\n",
                             where, id, recv_time.to_string().c_str());
        m_logger->sync_flush();
        return false;
      }

      qbdf_oc_summary_multi_rp oc_summ_multi = \
      oc_multi_cache->cached_by_exch_and_session[(int)exch][session.index()];
      if (!oc_summ_multi) {
        oc_summ_multi = qbdf_oc_summary_multi_rp(new qbdf_oc_summary_multi);
        oc_summ_multi->hdr.format = QBDFMsgFmt::MultiSessionOpenCloseSummary;
        oc_summ_multi->symbolId = id;
        oc_summ_multi->exch = exch;
        oc_summ_multi->session = session.index();
        oc_multi_cache->cached_by_exch_and_session[(int)exch][session.index()] = oc_summ_multi;
      }


      if (trade_update.m_trade_status == TradeStatus::OpenTrade) {
        uint32_t open_time = micros_exch_time_since_midnight / Time_Constants::micros_per_mili;
        if (oc_summ->open_price <= 0.0) {
          oc_summ->open_price = trade_update.m_price;
          oc_summ->open_size = trade_update.m_size;
          oc_summ->open_time = open_time;
        } else if (trade_update.m_misc[0] == 'A' &&
            open_time >= oc_summ->open_time &&
            open_time < oc_summ->open_time + 1000) {
          oc_summ->open_size += trade_update.m_size;
        }

        if (oc_summ_multi->open_price <= 0.0) {
          oc_summ_multi->open_price = trade_update.m_price;
          oc_summ_multi->open_size = trade_update.m_size;
          oc_summ_multi->open_time = open_time;
        } else if (trade_update.m_misc[0] == 'A' &&
            open_time >= oc_summ_multi->open_time &&
            open_time < oc_summ_multi->open_time + 1000) {
          oc_summ_multi->open_size += trade_update.m_size;
        }
      } else if (trade_update.m_trade_status == TradeStatus::CloseTrade) {
        uint32_t close_time = micros_exch_time_since_midnight / Time_Constants::micros_per_mili;
        if (trade_update.m_misc[0] == 'A') {
          if (oc_summ->close_price <= 0.0) {
            oc_summ->close_price = trade_update.m_price;
            oc_summ->close_size = trade_update.m_size;
            oc_summ->close_time = close_time;
          } else if (close_time >= oc_summ->close_time &&
              close_time < oc_summ->close_time + 1000) {
            oc_summ->close_size += trade_update.m_size;
          }
        } else {
          oc_summ->close_price = trade_update.m_price;
          oc_summ->close_size = trade_update.m_size;
          oc_summ->close_time = close_time;
        }

        if (trade_update.m_misc[0] == 'A') {
          if (oc_summ_multi->close_price <= 0.0) {
            oc_summ_multi->close_price = trade_update.m_price;
            oc_summ_multi->close_size = trade_update.m_size;
            oc_summ_multi->close_time = close_time;
          } else if (close_time >= oc_summ_multi->close_time &&
              close_time < oc_summ_multi->close_time + 1000) {
            oc_summ_multi->close_size += trade_update.m_size;
          }
        } else {
          oc_summ_multi->close_price = trade_update.m_price;
          oc_summ_multi->close_size = trade_update.m_size;
          oc_summ_multi->close_time = close_time;
        }
      }
    }


    if (trade_update.m_size_adjustment_to_cum) {
      if (trade_update.m_size_adjustment_to_cum > 0) {
        summ->cum_total_volume += trade_update.m_size_adjustment_to_cum;
        summ->total_trade_count += 1;
        if (trade_update.m_is_clean) {
          summ->cum_clean_volume += trade_update.m_size_adjustment_to_cum;
          summ->cum_clean_value += (trade_update.m_size_adjustment_to_cum * trade_update.m_price);
          summ->clean_trade_count += 1;
        }
      } else {
        if (abs(trade_update.m_size_adjustment_to_cum) > summ->cum_total_volume) {
          summ->cum_total_volume = 0;
        } else {
          summ->cum_total_volume += trade_update.m_size_adjustment_to_cum;
        }

        if (abs(trade_update.m_size_adjustment_to_cum) > summ->cum_clean_volume) {
          summ->cum_clean_volume = 0;
        } else {
          summ->cum_clean_volume += trade_update.m_size_adjustment_to_cum;
	}
	
	if ((trade_update.m_size_adjustment_to_cum * trade_update.m_price) >  summ->cum_clean_value) {
	  summ->cum_clean_value = 0;
	} else {
	  summ->cum_clean_value += (trade_update.m_size_adjustment_to_cum * trade_update.m_price);
	}

        if (summ->total_trade_count > 0) {
          summ->total_trade_count -= 1;
        }

        if (summ->clean_trade_count > 0) {
          summ->clean_trade_count -= 1;
        }

      }
    }
    summ->trade_status  = trade_update.m_trade_status.index();
    summ->market_status = status_update.m_market_status.index();
    summ->trade_time = micros_exch_time_since_midnight / Time_Constants::micros_per_mili;
    memcpy(summ->trade_cond, trade_update.m_trade_qualifier, sizeof(summ->trade_cond));

    return true;

  }
  
  
  int QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();
    
    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }
    
    Time::set_simulation_mode(true);
    Time::set_current_time(Time(m_date, "%Y%m%d"));
    
    boost::scoped_ptr<Handler> message_handler(ObjectFactory1<std::string, Handler, Application*>::allocate(m_params["handler"].getString(), m_app));
    
    message_handler->set_qbdf_builder(this);
    message_handler->init(m_source_name, m_hf_exch_ids, m_params);
    
    if(!message_handler->sets_up_own_channel_contexts())
    {
      Handler::channel_info_map_t& cim = message_handler->m_channel_info_map;
      for (size_t i = 0; i < 256; ++i) {
        cim.push_back(channel_info("", i));
        cim.back().context = i; //
        cim.back().has_reordering = false;
      }
    }

    size_t max_context = 50;
    if(message_handler->m_channel_info_map.size() > max_context)
    {
      max_context = message_handler->m_channel_info_map.size();
#ifdef CW_DEBUG
      cout << "Increasing max_context to " << max_context << endl;
#endif
    }

    parse_qrec_file(message_handler.get(), m_logger.get(), f_in, max_context);

    return 0;
  }

  // ----------------------------------------------------------------
  // Update the summary information based on the incoming indicative
  // ----------------------------------------------------------------
  bool QBDF_Builder::update_summary_imbalance(int hh, int mm, uint16_t id, char exch,
					      uint32_t micros_recv_time, int32_t exch_time_delta)
  {
    // Update summary information for 1 minute summary file
    if (m_summary_vec.size() <= id) {
      m_logger->log_printf(Log::ERROR, "%s: Summary cache only has %lu entries, but looking for id %d",
                           "QBDF_Builder::update_summary_imbalance", m_summary_vec.size(), id);
      m_logger->sync_flush();
      return false;
    }

    qbdf_summary_cache_rp cache = m_summary_vec.at(id);
    if (!cache) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to find cache record for id [%d]",
                           "QBDF_Builder::update_summary_imbalance", id);
      m_logger->sync_flush();
      return false;
    }

    uint64_t micros_recv_time_since_midnight = (hh * Time_Constants::micros_per_hr) + \
    (mm * Time_Constants::micros_per_min) + micros_recv_time;
    uint64_t micros_exch_time_since_midnight = micros_recv_time_since_midnight + exch_time_delta;

    Time recv_time(m_midnight + usec(micros_recv_time_since_midnight));
    Time exch_time(m_midnight + usec(micros_exch_time_since_midnight));

    qbdf_summary_rp summ = cache->cached_by_exch[(int)exch];
    if (!summ) {
      summ = qbdf_summary_rp(new qbdf_summary);
      summ->hdr.format = QBDFMsgFmt::Summary;
      summ->symbolId = id;
      summ->exch = exch;
      cache->cached_by_exch[(int)exch] = summ;
    }
    // Now record summary imbalance info
    summ->market_status = MarketStatus::Auction;
    summ->quote_time = micros_exch_time_since_midnight / Time_Constants::micros_per_mili;
    return true;
  }



  int QBDF_Builder::dump_summ_file(int hh, int mm, int summ_start_time, int summ_end_time) {
    int summ_hh = hh;
    int summ_mm = mm;
    summ_mm ++;
    if (summ_mm == 60) {
      summ_mm = 0;
      summ_hh ++;
    }
    int summ_time = summ_hh*100 + summ_mm;

    if (summ_time >= summ_start_time && summ_time <= summ_end_time) {
      char summ_file[256];
      sprintf(summ_file,"%s/%02d%02d00.sum", m_output_path.c_str(), summ_hh, summ_mm);
      FILE * f = fopen(summ_file,"w");
      if (f == 0) {
#ifdef CW_DEBUG
        printf("QBDF_Builder::dump_summ_file: Unable to create summary file [%s]\n",summ_file);
#endif
        return 1;
      }
      for (vector<qbdf_summary_cache_rp>::iterator iter = m_summary_vec.begin(); iter != m_summary_vec.end(); iter++) {
        qbdf_summary_cache_rp cache = *iter;
        if (!cache)
          continue;
        for (int ex = 0; ex < 256; ex++) {
          qbdf_summary_rp summ = cache->cached_by_exch[ex];
          if (summ) {
            fwrite(summ.get(), sizeof(qbdf_summary), 1, f);
          }
        }
      }
      fclose(f);
    }
    return 0;
  }


  int QBDF_Builder::dump_oc_summ_file() {
    const char* where = "QBDF_Builder::dump_oc_summ_file";

    char oc_summ_file[256];
    sprintf(oc_summ_file, "%s/open_close.sum", m_output_path.c_str());
    m_logger->log_printf(Log::INFO, "%s: Writing to: [%s]", where, oc_summ_file);
    FILE * f = fopen(oc_summ_file, "w");
    if (f == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to create summary file [%s]\n", where, oc_summ_file);
      m_logger->sync_flush();
      return 1;
    }

    for (vector<qbdf_oc_summary_cache_rp>::iterator iter = m_oc_summary_vec.begin(); iter != m_oc_summary_vec.end(); iter++) {
      qbdf_oc_summary_cache_rp cache = *iter;
      if (!cache)
        continue;
      for (int ex = 0; ex < 256; ex++) {
        qbdf_oc_summary_rp summ = cache->cached_by_exch[ex];
        if (summ) {
          fwrite(summ.get(), sizeof(qbdf_oc_summary), 1, f);
        }
      }
    }

    for (vector<qbdf_oc_summary_multi_cache_rp>::iterator iter = m_oc_summary_multi_vec.begin();
    iter != m_oc_summary_multi_vec.end(); iter++) {
      qbdf_oc_summary_multi_cache_rp cache = *iter;
      if (!cache)
        continue;
      for (int ex = 0; ex < 256; ex++) {
        for (int s = 0; s < 16; s++) {
          qbdf_oc_summary_multi_rp summ = cache->cached_by_exch_and_session[ex][s];
          if (summ) {
            fwrite(summ.get(), sizeof(qbdf_oc_summary_multi), 1, f);
          }
        }
      }
    }

    fclose(f);
    return 0;
  }


  int QBDF_Builder::create_intraday_price_files(const string& region, const string& version,
                                                const string& start_hhmm, const string& end_hhmm,
                                                int interval_secs, const string& sids) {
    const char* where = "QBDF_Builder::create_intraday_price_files";

    m_logger->log_printf(Log::INFO, "%s: region=%s version=%s start_hhmm=%s end_hhmm=%s interval=%d sids=%s",
                         where, region.c_str(), version.c_str(), start_hhmm.c_str(), end_hhmm.c_str(),
                         interval_secs, sids.c_str());

    m_last_trade_map.clear();
    m_cum_total_volume_map.clear();
    m_cum_clean_volume_map.clear();
    m_cum_clean_value_map.clear();

    string asset_class = m_params.getDefaultString("asset_class", "equ");

    if (m_params.has("ipx_tradable_exchanges")) {
      vector<string> vec = m_params["ipx_tradable_exchanges"].getStringVector();

      for(vector<string>::const_iterator itr = vec.begin(); itr!=vec.end(); ++itr) {
        string tradable_exchange = *itr;
        boost::optional<ExchangeID> p = ExchangeID::get_by_name(tradable_exchange.c_str());
        if (p) {
          char qbdf_exch = get_qbdf_exch_by_hf_exch_id(*p);
          if (qbdf_exch != ' ') {
            m_tradable_exch_map[(int)qbdf_exch] = 1;
          } else {
            m_logger->log_printf(Log::WARN, "%s: Could not find QBDF exch for tradable exchange %s",
                                 where, tradable_exchange.c_str());
            m_logger->sync_flush();
          }
        }
      }
    } else if (region == "na") {
      m_tradable_exch_map[65] = 1; // A-Amex
      m_tradable_exch_map[66] = 1; // B-Boston
      m_tradable_exch_map[74] = 1; // J-EDGA
      m_tradable_exch_map[75] = 1; // K-EDGX
      m_tradable_exch_map[78] = 1; // N-Nyse
      m_tradable_exch_map[80] = 1; // P-Arca
      m_tradable_exch_map[81] = 1; // Q-Nasdaq
      m_tradable_exch_map[84] = 1; // T-Nasdaq
      m_tradable_exch_map[90] = 1; // Z-BATS
    } else {
      m_tradable_exch_map[65] = 1; // A-Amsterdam
      m_tradable_exch_map[66] = 1; // B-Brussels
      m_tradable_exch_map[67] = 1; // C-Copenhagen
      m_tradable_exch_map[70] = 1; // F-France (Paris)
      m_tradable_exch_map[72] = 1; // H-Helsinki
      m_tradable_exch_map[73] = 1; // I-Italy (Milan)
      m_tradable_exch_map[76] = 1; // L-London
      m_tradable_exch_map[77] = 1; // M-Madrid
      m_tradable_exch_map[79] = 1; // O-Oslo
      m_tradable_exch_map[80] = 1; // P-Portugal (Lisbon)
      m_tradable_exch_map[83] = 1; // S-Stockholm
      m_tradable_exch_map[86] = 1; // V-Virtx (Swiss)
      m_tradable_exch_map[87] = 1; // W-Vienna
      m_tradable_exch_map[88] = 1; // X-Xetra
      m_tradable_exch_map[90] = 1; // Z-ChiX
    }
    m_logger->log_printf(Log::INFO, "%s: Using %lu tradable exchanges", where, m_tradable_exch_map.size());

    map<string,int> sid_filter;
    if (sids.length() > 0) {
      size_t lastPos = sids.find_first_not_of(",", 0);
      size_t pos = sids.find_first_of(",", lastPos);
      while (string::npos != pos || string::npos != lastPos) {
        sid_filter[sids.substr(lastPos, pos - lastPos)] = 1;
        lastPos = sids.find_first_not_of(",", pos);
        pos = sids.find_first_of(",", lastPos);
      }
    }

    // Ensure all open files are close
    if (close_open_files()) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to close open files", where);
      m_logger->sync_flush();
      return 1;
    }

    load_ids_file();

    if (!verify_time(start_hhmm, end_hhmm))
      return 1;

    int start_time=atoi(start_hhmm.c_str());
    int end_time=atoi(end_hhmm.c_str());

    map<uint16_t,string> id2sid = get_sid_map();
    map<string,float> sid2price = getSid2PriceMap(region, version, asset_class);

    char output_file[1024];
    sprintf(output_file, "%s/%s.%s_intraday_prices_%dsec.csv",
            m_output_path.c_str(), m_date.c_str(), asset_class.c_str(), interval_secs);
    m_logger->log_printf(Log::INFO, "%s: Writing to [%s]", where, output_file);
    FILE* fo = fopen(output_file, "w");
    if (fo == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to create prices file [%s]", where, output_file);
      m_logger->sync_flush();
      return 1;
    }
    fprintf(fo, "time,sid,high,low,last,bid,ask,ret_prev_interval,ret_prev_close,tradable_vol,cum_tradable_vol,clean_tradable_vol,cum_clean_tradable_vol,clean_tradable_value,cum_clean_tradable_value\n");

    int start_hh = start_time / 100;
    int start_mm = start_time % 100;
    int next_write_time = (start_hh * 3600000) + (start_mm * 60000) + (interval_secs * 1000);

    for (int i=start_time; i<=end_time; i++) {
      // walk through all potential files and read them
      int hh = i / 100;
      int mm = i % 100;
      if (mm == 60) {
        mm = 0;
        hh++;
        i = 100 * hh;
      }
      if (hh > 23 || hh < 0 || mm < 0 || mm > 59) {
        m_logger->log_printf(Log::ERROR, "%s: Invalid time hh=[%d] mm=[%d] time=[%d]", where, hh, mm, i);
        m_logger->sync_flush();
        return 1;
      }

      int bin_start_time = (hh * 3600000) + (mm * 60000);
      while (bin_start_time >= next_write_time) {
        write_price_file(fo, next_write_time, id2sid, sid_filter, sid2price);
        next_write_time += (interval_secs * 1000);
      }

      char* buffer_start = 0;
      char* buffer_end = 0;
      FILE* f = 0;

      char bin_file[256];
      struct stat st_file_info;
      int stat_ret;

      sprintf(bin_file,"%s/%02d%02d00.bin.lzo", m_output_path.c_str(), hh, mm);
      stat_ret = stat(bin_file, &st_file_info);

      if (!stat_ret) {
        m_logger->log_printf(Log::DEBUG, "%s: Found LZO file: %s", where, bin_file);
        m_compression_type = "lzo";
        m_compression_util.reset(new LZO_Util(m_app, m_logger));
      } else {
        sprintf(bin_file,"%s/%02d%02d00.bin.snz", m_output_path.c_str(), hh, mm);
        stat_ret = stat(bin_file, &st_file_info);
        if (!stat_ret) {
          m_logger->log_printf(Log::DEBUG, "%s: Found Snappy file: %s", where, bin_file);
          m_compression_type = "snappy";
          m_compression_util.reset(new Snappy_Util(m_app, m_logger));
        }
      }

      if (stat_ret) {
        sprintf(bin_file, "%s/%02d%02d00.bin", m_output_path.c_str(), hh, mm);
        f = fopen(bin_file, "rb");
        if (f == 0) {
          m_logger->log_printf(Log::WARN, "%s: Could not open bin file: %s", where, bin_file);
          continue;
        }

        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        rewind(f);
        buffer_start = new char[file_size + 1024]; // little safety net
        size_t result = fread(buffer_start, 1, file_size, f);
        if (result != (size_t)file_size) {
          m_logger->log_printf(Log::ERROR, "%s: reading file [%s] into memory", where, bin_file);
          m_logger->sync_flush();
          delete [] buffer_start;
          continue;
        }
        buffer_end = buffer_start + file_size;
      } else {
        if (!m_compression_util->decompress_file_start(string(bin_file))) {
          m_logger->log_printf(Log::ERROR, "%s: Couldn't start decompression on file: [%s]", where, bin_file);
          return false;
        }
        if(!m_compression_util->decompress_file_get_next_block_size()) {
          m_logger->log_printf(Log::ERROR, "%s: Couldn't get decompression size on file: [%s]", where, bin_file);
          return false;
        }
        if (!m_compression_util->decompress_file_next_block(&buffer_start, &buffer_end)) {
          m_logger->log_printf(Log::ERROR, "%s: Couldn't decompress data file: [%s]", where, bin_file);
          return false;
        }
        m_compression_util->decompress_file_close();
        m_logger->log_printf(Log::DEBUG, "%s: Read %s using compression", where, bin_file);
        m_logger->sync_flush();
      }

      qbdf_format_header *p_hdr;
      char* buf_posn = buffer_start;
      while ( buf_posn < buffer_end) {
        p_hdr = (qbdf_format_header*)buf_posn;

        switch (p_hdr->format) {
        case QBDFMsgFmt::QuoteSmall: {
          qbdf_quote_small* p_quote = (qbdf_quote_small*)buf_posn;
          buf_posn += sizeof(qbdf_quote_small);

          uint16_t id = p_quote->qr.symbolId;
          uint16_t quote_time = p_quote->qr.time;
          char quote_exch = p_quote->qr.exch;
          MDPrice bid_price = MDPrice::from_fprice(p_quote->qr.bidPrice);
          MDPrice ask_price = MDPrice::from_fprice(p_quote->qr.askPrice);

          while ((bin_start_time + quote_time) >= next_write_time) {
            write_price_file(fo, next_write_time, id2sid, sid_filter, sid2price);
            next_write_time += (interval_secs * 1000);
          }

          if (m_tradable_exch_map.size()) {
            if (m_tradable_exch_map.find((int)quote_exch) == m_tradable_exch_map.end()) { break; }
          }


          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, quote_exch, quote_time, p_quote->qr.cond,
                                    bid_price, p_quote->sizes.bidSize,
                                    ask_price, p_quote->sizes.askSize)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to update summary quote for id [%d] at time [%d]",
                                 where, id, quote_time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::QuoteLarge: {
          qbdf_quote_large* p_quote = (qbdf_quote_large*)buf_posn;
          buf_posn += sizeof(qbdf_quote_large);

          uint16_t id = p_quote->qr.symbolId;
          uint16_t quote_time = p_quote->qr.time;
          char quote_exch = p_quote->qr.exch;
          MDPrice bid_price = MDPrice::from_fprice(p_quote->qr.bidPrice);
          MDPrice ask_price = MDPrice::from_fprice(p_quote->qr.askPrice);

          while ((bin_start_time + quote_time) >= next_write_time) {
            write_price_file(fo, next_write_time, id2sid, sid_filter, sid2price);
            next_write_time += (interval_secs * 1000);
          }

          if (m_tradable_exch_map.size()) {
            if (m_tradable_exch_map.find((int)quote_exch) == m_tradable_exch_map.end()) { break; }
          }


          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, quote_exch, quote_time * 1000, p_quote->qr.cond,
                                    bid_price, p_quote->sizes.bidSize,
                                    ask_price, p_quote->sizes.askSize)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to update summary quote for id [%d] at time [%d]",
                                 where, id, quote_time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::quote_small_iprice32:
        {
          qbdf_quote_small_iprice32* p_quote = (qbdf_quote_small_iprice32*)buf_posn;
          buf_posn += sizeof(qbdf_quote_small_iprice32);

          uint16_t id = p_quote->common.id;
          uint64_t quote_time = p_quote->common.time;
          char quote_exch = p_quote->common.exch;
          MDPrice bid_price = MDPrice::from_iprice32(p_quote->bid_price);
          MDPrice ask_price = MDPrice::from_iprice32(p_quote->ask_price);

          while ((bin_start_time + (quote_time / 1000)) >= next_write_time) {
            write_price_file(fo, next_write_time, id2sid, sid_filter, sid2price);
            next_write_time += (interval_secs * 1000);
          }

          if (m_tradable_exch_map.size()) {
            if (m_tradable_exch_map.find((int)quote_exch) == m_tradable_exch_map.end()) { break; }
          }


          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, quote_exch, quote_time, p_quote->cond,
                                    bid_price, p_quote->bid_size, ask_price, p_quote->ask_size)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to update summary quote for id [%d] at time [%lu]",
                                 where, id, (quote_time / 1000));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::quote_large_iprice32:
        {
          qbdf_quote_large_iprice32* p_quote = (qbdf_quote_large_iprice32*)buf_posn;
          buf_posn += sizeof(qbdf_quote_large_iprice32);

          uint16_t id = p_quote->common.id;
          uint64_t quote_time = p_quote->common.time;
          char quote_exch = p_quote->common.exch;
          MDPrice bid_price = MDPrice::from_iprice32(p_quote->bid_price);
          MDPrice ask_price = MDPrice::from_iprice32(p_quote->ask_price);

          while ((bin_start_time + (quote_time / 1000)) >= next_write_time) {
            write_price_file(fo, next_write_time, id2sid, sid_filter, sid2price);
            next_write_time += (interval_secs * 1000);
          }

          if (m_tradable_exch_map.size()) {
            if (m_tradable_exch_map.find((int)quote_exch) == m_tradable_exch_map.end()) { break; }
          }


          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, quote_exch, quote_time, p_quote->cond,
                                    bid_price, p_quote->bid_size, ask_price, p_quote->ask_size)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to update summary quote for id [%d] at time [%lu]",
                                 where, id, (quote_time / 1000));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::quote_iprice64:
        {
          qbdf_quote_iprice64* p_quote = (qbdf_quote_iprice64*)buf_posn;
          buf_posn += sizeof(qbdf_quote_iprice64);

          uint16_t id = p_quote->common.id;
          uint64_t quote_time = p_quote->common.time;
          char quote_exch = p_quote->common.exch;
          MDPrice bid_price = MDPrice::from_iprice64(p_quote->bid_price);
          MDPrice ask_price = MDPrice::from_iprice64(p_quote->ask_price);

          while ((bin_start_time + (quote_time / 1000)) >= next_write_time) {
            write_price_file(fo, next_write_time, id2sid, sid_filter, sid2price);
            next_write_time += (interval_secs * 1000);
          }

          if (m_tradable_exch_map.size()) {
            if (m_tradable_exch_map.find((int)quote_exch) == m_tradable_exch_map.end()) { break; }
          }


          // Update summary information for 1 minute summary file
          if (!update_summary_quote(hh, mm, id, quote_exch, quote_time, p_quote->cond,
                                    bid_price, p_quote->bid_size, ask_price, p_quote->ask_size)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to update summary quote for id [%d] at time [%lu]",
                                 where, id, (quote_time / 1000));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::Trade: {
          qbdf_trade* p_trade = (qbdf_trade*)buf_posn;
          buf_posn += sizeof(qbdf_trade);

          uint16_t id = p_trade->symbolId;
          MDPrice md_price = MDPrice::from_fprice(p_trade->price);

          while ((bin_start_time + p_trade->time) >= next_write_time) {
            write_price_file(fo, next_write_time, id2sid, sid_filter, sid2price);
            next_write_time += (interval_secs * 1000);
          }

          if (m_tradable_exch_map.size()) {
            if (m_tradable_exch_map.find((int)p_trade->exch) == m_tradable_exch_map.end()) { break; }
          }

          if (!update_summary_trade(hh, mm, id, p_trade->exch, p_trade->time * 1000, 0,
                                    string(p_trade->cond, 4), p_trade->correct,
                                    md_price, p_trade->size)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to update summary trade for id [%d] at time [%d]",
                                 where, p_trade->symbolId, p_trade->time);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::trade_iprice32: {
          qbdf_trade_iprice32* p_trade = (qbdf_trade_iprice32*)buf_posn;
          buf_posn += sizeof(qbdf_trade_iprice32);

          uint16_t id = p_trade->common.id;
          MDPrice md_price = MDPrice::from_iprice32(p_trade->price);

          while ((bin_start_time + (p_trade->common.time / 1000)) >= next_write_time) {
            write_price_file(fo, next_write_time, id2sid, sid_filter, sid2price);
            next_write_time += (interval_secs * 1000);
          }

          if (m_tradable_exch_map.size()) {
            if (m_tradable_exch_map.find((int)p_trade->common.exch) == m_tradable_exch_map.end()) { break; }
          }

          if (!update_summary_trade(hh, mm, id, p_trade->common.exch, p_trade->common.time,
                                    p_trade->common.exch_time_delta, string(p_trade->cond, 4),
                                    p_trade->correct, md_price, p_trade->size)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to update summary trade for id [%d] at time [%d]",
                                 where, id, (p_trade->common.time / 1000));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::trade_iprice64: {
          qbdf_trade_iprice64* p_trade = (qbdf_trade_iprice64*)buf_posn;
          buf_posn += sizeof(qbdf_trade_iprice64);

          uint16_t id = p_trade->common.id;
          MDPrice md_price = MDPrice::from_iprice64(p_trade->price);

          while ((bin_start_time + (p_trade->common.time / 1000)) >= next_write_time) {
            write_price_file(fo, next_write_time, id2sid, sid_filter, sid2price);
            next_write_time += (interval_secs * 1000);
          }

          if (m_tradable_exch_map.size()) {
            if (m_tradable_exch_map.find((int)p_trade->common.exch) == m_tradable_exch_map.end()) { break; }
          }

          if (!update_summary_trade(hh, mm, id, p_trade->common.exch, p_trade->common.time,
                                    p_trade->common.exch_time_delta, string(p_trade->cond, 4),
                                    p_trade->correct, md_price, p_trade->size)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to update summary trade for id [%d] at time [%d]",
                                 where, id, (p_trade->common.time / 1000));
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        case QBDFMsgFmt::Imbalance: {
          buf_posn += sizeof(qbdf_imbalance);
          break;
        }
        case QBDFMsgFmt::Status: {
          buf_posn += sizeof(qbdf_status);
          break;
        }
        default: {
          if ((size_t)p_hdr->format < m_qbdf_message_sizes.size()) {
            buf_posn += m_qbdf_message_sizes[(size_t)p_hdr->format];
          } else {
            m_logger->log_printf(Log::ERROR, "%s: Invalid fmt [%d] in file [%s]",
                                 where, p_hdr->format, bin_file);
            m_logger->sync_flush();
            if (stat_ret) { delete [] buffer_start; }
            return 1;
          }
          break;
        }
        }
      }
      if (stat_ret) {
        fclose(f);
        delete [] buffer_start;
      }
    }
    fclose(fo);
    m_logger->sync_flush();
    return 0;
  }


  int QBDF_Builder::write_price_file(FILE* f, int time, const map<uint16_t,string>& id2sid,
                                     const map<string,int>& sid_filter,
                                     const map<string,float>& sid2prev_close) {
    vector<qbdf_summary_cache_rp>::iterator iter;
    for (iter = m_summary_vec.begin(); iter != m_summary_vec.end(); iter++) {
      qbdf_summary_cache_rp cache = *iter;
      if (!cache) { continue; }

      uint16_t symbolId = 0;
      float last_price = 0.0;
      float low_price = 999999.0;
      float high_price = 0.0;
      float bid_price = 0.0;
      float ask_price = 999999.0;
      float last_price_ret = 0.0;
      float prev_close_ret = 0.0;
      uint64_t total_tradable_volume = 0;
      uint64_t clean_tradable_volume = 0;
      uint64_t clean_tradable_value = 0;
      uint64_t cum_total_tradable_volume = 0;
      uint64_t cum_clean_tradable_volume = 0;
      uint64_t cum_clean_tradable_value = 0;
      uint32_t latest_trade_time = 0;

      for (int ex = 0; ex < 256; ex++) {
        qbdf_summary_rp summ = cache->cached_by_exch[ex];
        if (!summ) { continue; }

        symbolId = summ->symbolId;
        if (symbolId == 0) { continue; }

        if (summ->trade_time > latest_trade_time) {
          latest_trade_time = summ->trade_time;
          last_price = summ->price;
        }

        if (summ->high > high_price)
          high_price = summ->high;

        if (summ->low < low_price)
          low_price = summ->low;

        if (summ->bidPrice > bid_price)
          bid_price = summ->bidPrice;

        if (summ->askPrice < ask_price)
          ask_price = summ->askPrice;

        cum_total_tradable_volume += summ->cum_total_volume;
        cum_clean_tradable_volume += summ->cum_clean_volume;
        cum_clean_tradable_value += summ->cum_clean_value;

        // reset summary cache so that high/low is only for interval
        summ->high = 0.0;
        summ->low = 999999.0;
      }

      if (symbolId == 0) { continue; }

      map<uint16_t,string>::const_iterator id_iter = id2sid.find(symbolId);
      if (id_iter == id2sid.end()) { continue; }

      string sid = id_iter->second;
      if (sid == "0") { continue; }

      if (sid_filter.size() > 0) {
        map<string,int>::const_iterator sid_iter = sid_filter.find(sid);
        if (sid_iter == sid_filter.end()) { continue; }
      }

      if (bid_price == 0.0 && ask_price == 0.0 && last_price == 0.0) { continue; }

      if (last_price != 0.0) {
        map<uint16_t,float>::const_iterator m_last_trade_iter = m_last_trade_map.find(symbolId);
        if (m_last_trade_iter != m_last_trade_map.end()) {
          last_price_ret = log(last_price/m_last_trade_iter->second);
        }
        m_last_trade_map[symbolId] = last_price;

        map<string,float>::const_iterator prev_close_iter = sid2prev_close.find(sid);
        if (prev_close_iter != sid2prev_close.end()) {
          prev_close_ret = log(last_price/prev_close_iter->second);
        }
      }

      map<string,uint64_t>::const_iterator m_cum_total_volume_iter = m_cum_total_volume_map.find(sid);
      if (m_cum_total_volume_iter != m_cum_total_volume_map.end()) {
        total_tradable_volume = cum_total_tradable_volume - m_cum_total_volume_iter->second;
      } else {
        total_tradable_volume = cum_total_tradable_volume;
      }
      m_cum_total_volume_map[sid] = cum_total_tradable_volume;

      map<string,uint64_t>::const_iterator m_cum_clean_volume_iter = m_cum_clean_volume_map.find(sid);
      if (m_cum_clean_volume_iter != m_cum_clean_volume_map.end()) {
        clean_tradable_volume = cum_clean_tradable_volume - m_cum_clean_volume_iter->second;
      } else {
        clean_tradable_volume = cum_clean_tradable_volume;
      }
      m_cum_clean_volume_map[sid] = cum_clean_tradable_volume;

      map<string,uint64_t>::const_iterator m_cum_clean_value_iter = m_cum_clean_value_map.find(sid);
      if (m_cum_clean_value_iter != m_cum_clean_value_map.end()) {
        clean_tradable_value = cum_clean_tradable_value - m_cum_clean_value_iter->second;
      } else {
        clean_tradable_value = cum_clean_tradable_value;
      }
      m_cum_clean_value_map[sid] = cum_clean_tradable_value;

      char bid_price_str[16] = "";
      if (bid_price > 0 and bid_price < 999999) { sprintf(bid_price_str, "%.4f", bid_price); }

      char ask_price_str[16] = "";
      if (ask_price > 0 and ask_price < 999999) { sprintf(ask_price_str, "%.4f", ask_price); }

      char high_price_str[16] = "";
      if (high_price > 0 and high_price < 999999) { sprintf(high_price_str, "%.4f", high_price); }

      char low_price_str[16] = "";
      if (low_price > 0 and low_price < 999999) { sprintf(low_price_str, "%.4f", low_price); }

      char last_price_str[16] = "";
      if (last_price > 0 and last_price < 999999) { sprintf(last_price_str, "%.4f", last_price); }

      int output_time = time + (m_ipx_tz_adjust * 1000);
      int hh = output_time / 3600000;
      int mm = (output_time % 3600000) / 60000;
      int ss = (output_time - (hh * 3600000) - (mm * 60000)) / 1000;
      fprintf(f, "%02d:%02d:%02d,%s,%s,%s,%s,%s,%s,%.4f,%.4f,%lu,%lu,%lu,%lu,%lu,%lu\n", hh, mm, ss,
              sid.c_str(), high_price_str, low_price_str, last_price_str, bid_price_str, ask_price_str,
              last_price_ret, prev_close_ret, total_tradable_volume, cum_total_tradable_volume,
              clean_tradable_volume, cum_clean_tradable_volume, clean_tradable_value, cum_clean_tradable_value);
    }
    return 0;
  }


  map<string,float> QBDF_Builder::getSid2PriceMap(const string& region, const string& version,
                                                  const string& asset_class) {
    const char* where = "QBDF_Builder::getSid2PriceMap";

    char file_name[1024];
    if (asset_class == "idx") {
      sprintf(file_name, "/q/coredata/data/prod/misc/index/%s/prices/%s/%s.idx_prices.rev.csv",
              version.c_str(), m_date.substr(0, 4).c_str(), m_date.c_str());
    } else {
      sprintf(file_name, "/q/coredata/data/prod/%s/%s/prices/%s/%s.equ_prices.rev.csv",
              region.c_str(), version.c_str(), m_date.substr(0, 4).c_str(), m_date.c_str());
    }
    m_logger->log_printf(Log::INFO, "%s: Reading prices from %s", where, file_name);

    map<string,float> price_map;
    CSV_Parser csv_reader(file_name);

    int sid_idx = csv_reader.column_index("sid");
    int prev_close_idx = csv_reader.column_index("prev_close");

    CSV_Parser::state_t s = csv_reader.read_row();
    while (s == CSV_Parser::OK) {
      string sid = csv_reader.get_column(sid_idx);
      float price = strtod(csv_reader.get_column(prev_close_idx), NULL);

      price_map[sid] = price;
      s = csv_reader.read_row();
    }

    m_logger->log_printf(Log::INFO, "%s: Got %lu price mappings", where, price_map.size());
    m_logger->sync_flush();
    return price_map;
  }


  // ----------------------------------------------------------------
  // Close all open files
  // ----------------------------------------------------------------
  int QBDF_Builder::close_open_files()
  {
    for (int i=0; i<2400; i++) {
      if (m_raw_files[i]) {
        fclose(m_raw_files[i]);
        m_raw_files[i] = 0;
      }
      // Close 1 min summary files
      if (m_summary_files[i]) {
        fclose(m_summary_files[i]);
        m_summary_files[i] = 0;
      }
    }
    return 0;
  }


  // ----------------------------------------------------------------
  // Compress binary files
  // ----------------------------------------------------------------
  int QBDF_Builder::compress_files(const string& start_hhmm, const string& end_hhmm, int level=9)
  {
    const char* where = "QBDF_Builder::compress_files";
    m_logger->log_printf(Log::INFO, "%s: start_hhmm=%s end_hhmm=%s",
                         where, start_hhmm.c_str(), end_hhmm.c_str());

    if (!verify_time(start_hhmm, end_hhmm))
      return 1;

    //int start_time=atoi(start_hhmm.c_str());
    //int end_time=atoi(end_hhmm.c_str());

    // run for all times, but we will only complain if file is not found for given times
    int start_time = 0;
    int end_time = 2359;

    for (int i = start_time; i <= end_time; i++) {
      // walk through all potential files and read them
      int hh = i / 100;
      int mm = i % 100;
      if (mm == 60) {
        mm = 0;
        hh++;
        i = 100 * hh;
      }

      char input_file[256];
      sprintf(input_file, "%s/%02d%02d00.bin", m_output_path.c_str(), hh, mm);

      // check if file exists and skip if it does not and it is outside given times
      ifstream ifile(input_file);
      if (!ifile) {
        char this_time[16];
        sprintf(this_time, "%02d%02d00", hh, mm);
        string this_time_str(this_time);
        if (this_time_str < start_hhmm || this_time_str > end_hhmm) {
          // file is outside defined bounds so no need to report that it is missing
          continue;
        }
      } else {
        ifile.close();
      }

      char output_file[256];
      if (m_compression_type == "lzo") {
        sprintf(output_file, "%s/%02d%02d00.bin.lzo", m_output_path.c_str(), hh, mm);
      } else if (m_compression_type == "snappy"){
        sprintf(output_file, "%s/%02d%02d00.bin.snz", m_output_path.c_str(), hh, mm);
      } else {
        m_logger->log_printf(Log::ERROR, "%s: Unsupported compression type [%s] (use lzo/snappy)",
                             where, m_compression_type.c_str());
        m_logger->sync_flush();
        return 1;
      }

      bool file_exists = (access(input_file, R_OK) == 0);
      if(file_exists)
      {
        if(!m_compression_util->compress_file(string(input_file), string(output_file),
                                          m_compression_util->get_max_block_size(),
                                          level, false))
        {
          return 1;
        }
      }
      else
      {
        m_logger->log_printf(Log::ERROR, "%s: Could not open file: [%s]", where, input_file);
      }
    }

    m_logger->sync_flush();
    return 0;
  }


  // ----------------------------------------------------------------
  // Process a (potentially) new TAQ symbol
  // ----------------------------------------------------------------
  uint16_t QBDF_Builder::process_taq_symbol(const string& symbol_)
  {
    // Lets see if this has already been processed and an id allocated
    pair<map<string, uint16_t>::iterator,bool> ret = m_symbol_map.insert(pair<string, uint16_t> (symbol_,m_taq_id+1));
    if (!ret.second) {
      return ret.first->second;
    }
    else {
      // If we get here then this is a real new symbol and we just added it
      m_taq_id++;

      m_summary_vec.push_back(qbdf_summary_cache_rp(new qbdf_summary_cache(m_taq_id)));
      m_oc_summary_vec.push_back(qbdf_oc_summary_cache_rp(new qbdf_oc_summary_cache(m_taq_id)));
      m_oc_summary_multi_vec.push_back(qbdf_oc_summary_multi_cache_rp(new qbdf_oc_summary_multi_cache(m_taq_id)));

      if (m_find_order_id_mode) {
        return m_taq_id;
      }

      // Now write it out to the id's file
      if (m_id_file == 0) {
        m_id_file_name = m_output_path + "/ids.csv";
        m_id_file = fopen(m_id_file_name.c_str(),"w");
        if (m_id_file == 0) {
          m_logger->log_printf(Log::ERROR, "%s: Unable to open id's file %s for writing\n",
                               "QBDF_Builder::process_taq_symbol", m_id_file_name.c_str());
          m_logger->sync_flush();
          return 0;
        }
        fprintf(m_id_file,"BinaryId,Symbol\n");
      }
      fprintf(m_id_file,"%d,%s\n",m_taq_id,symbol_.c_str());
      return m_taq_id;
    }
    return 0;
  }


  bool QBDF_Builder::write_qbdf_version_file(const string& qbdf_version_file_name,
                                             const string& version)
  {
    const char* where = "QBDF_Builder::write_qbdf_version_file";

    m_logger->log_printf(Log::INFO, "%s: Writing qbdf version %s to file %s\n",
                         where, version.c_str(), qbdf_version_file_name.c_str());
    FILE* qbdf_version_file = fopen(qbdf_version_file_name.c_str(), "w");
    if (qbdf_version_file == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to open qbdf version file %s for writing\n",
                           where, qbdf_version_file_name.c_str());
      m_logger->sync_flush();
      return false;
    }
    fprintf(qbdf_version_file, "%s\n", version.c_str());
    fclose(qbdf_version_file);
    return true;
  }


  bool QBDF_Builder::write_qbdf_source_file(const string& qbdf_source_file_name,
                                            const string& quote_file, const string& trade_file)
  {
    const char* where = "QBDF_Builder::write_qbdf_source_file";

    m_logger->log_printf(Log::INFO, "%s: Writing raw data source to file %s\n",
                         where, qbdf_source_file_name.c_str());
    FILE* qbdf_source_file = fopen(qbdf_source_file_name.c_str(), "w");
    if (qbdf_source_file == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to open qbdf source file %s for writing\n",
                           where, qbdf_source_file_name.c_str());
      m_logger->sync_flush();
      return false;
    }
    if (quote_file == trade_file) {
      fprintf(qbdf_source_file, "ALL=%s\n", quote_file.c_str());
    } else {
      fprintf(qbdf_source_file, "QUOTES=%s\n", quote_file.c_str());
      fprintf(qbdf_source_file, "TRADES=%s\n", trade_file.c_str());
    }

    fclose(qbdf_source_file);
    return true;
  }


  // ----------------------------------------------------------------
  // Process a (potentially) new TAQ symbol
  // ----------------------------------------------------------------
  uint16_t QBDF_Builder::clean_and_process_taq_symbol(string& symbol_)
  {
    size_t found = symbol_.find_last_not_of(" \t\f\v\n\r");
    if (found != string::npos)
      symbol_.erase(found+1);
    else
      symbol_.clear();

    return process_taq_symbol(symbol_);
  }


  // ----------------------------------------------------------------
  // do some temporarily invasive fixed length conversions
  // ----------------------------------------------------------------
  int QBDF_Builder::fixed_length_atoi(char* str_, int len_)
  {
    char tmp = *(str_ + len_);
    *(str_ + len_) = 0;
    int value = atoi(str_);
    *(str_ + len_) = tmp;
    return value;
  }

  // ----------------------------------------------------------------
  // do some temporarily invasive fixed length conversions
  // ----------------------------------------------------------------
  uint64_t QBDF_Builder::fixed_length_strtol(char* str_, int len_)
  {
    char tmp = *(str_ + len_);
    *(str_ + len_) = 0;
    uint64_t value = strtol(str_, 0, 10);
    *(str_ + len_) = tmp;
    return value;
  }


  double QBDF_Builder::fixed_length_atof(char* str_, int len_)
  {
    char tmp = *(str_ + len_);
    *(str_ + len_) = 0;
    double value = atof(str_);
    *(str_ + len_) = tmp;
    return value;
  }


  double QBDF_Builder::fixed_length_atof(char* str_, int len_, int implicit_decimal_pos_)
  {
    char tmp = *(str_ + implicit_decimal_pos_);
    *(str_ + implicit_decimal_pos_) = 0;
    double value = atof(str_);
    *(str_ + implicit_decimal_pos_) = tmp;

    char tmp2 = *(str_ + implicit_decimal_pos_ -1);
    *(str_ + implicit_decimal_pos_ -1) = '.';
    tmp = *(str_ + len_);
    *(str_ + len_) = 0;
    value += atof(str_ + implicit_decimal_pos_ -1);
    *(str_ + implicit_decimal_pos_ -1) = tmp2;
    *(str_ + len_) = tmp;
    return value;
  }


  int QBDF_Builder::millis_to_qbdf_time(uint32_t millis_since_midnight)
  {
    int hh = (millis_since_midnight / Time_Constants::millis_per_hr);
    int mm = (millis_since_midnight / Time_Constants::millis_per_min) % 60;
    int ss = (millis_since_midnight / Time_Constants::millis_per_second) % 60;
    int ms = millis_since_midnight % 1000;

    return (hh*10000000 + mm*100000 + ss*1000 + ms);
  }


  uint64_t QBDF_Builder::micros_to_qbdf_time(uint64_t micros_since_midnight)
  {
    uint64_t hh = (micros_since_midnight / Time_Constants::micros_per_hr);
    uint64_t mm = (micros_since_midnight / Time_Constants::micros_per_min) % 60;
    uint64_t ss = (micros_since_midnight / Time_Constants::micros_per_second) % 60;
    uint64_t micros = micros_since_midnight % 1000000UL;

    return (hh*10000000000UL + mm*100000000UL + ss*1000000UL + micros);
  }

  uint64_t QBDF_Builder::qbdf_time_to_micros(uint64_t qbdf_time)
  {
    uint64_t hh = (qbdf_time / 10000000000UL);
    uint64_t mm = (qbdf_time - (hh * 10000000000UL)) / 100000000UL;
    uint64_t ss_micros = qbdf_time - (hh * 10000000000UL) - mm * 100000000UL;
    return (hh*Time_Constants::micros_per_hr + mm*Time_Constants::micros_per_min + ss_micros);
  }

  uint64_t QBDF_Builder::nanos_to_qbdf_time(uint64_t nanos_since_midnight)
  {
    uint64_t hh = (nanos_since_midnight / Time_Constants::ticks_per_hr);
    uint64_t mm = (nanos_since_midnight / Time_Constants::ticks_per_min) % 60;
    uint64_t ss = (nanos_since_midnight / Time_Constants::ticks_per_second) % 60;
    uint64_t nanos = nanos_since_midnight % 1000000000UL;

    return (hh*10000000000000UL + mm*100000000000UL + ss*1000000000UL + nanos);
  }

  uint64_t QBDF_Builder::qbdf_time_to_nanos(uint64_t qbdf_time)
  {
    uint64_t hh = (qbdf_time / 10000000000000UL);
    uint64_t mm = (qbdf_time - (hh * 10000000000000UL)) / 100000000000UL;
    uint64_t ss_nanos = qbdf_time - (hh * 10000000000000UL) - mm * 100000000000UL;
    return (hh*Time_Constants::ticks_per_hr + mm*Time_Constants::ticks_per_min + ss_nanos);
  }


  int QBDF_Builder::write_binary_record_micros(uint64_t qbdf_time, void* p_rec, size_t rec_size)
  {
    const char* where = "QBDF_Builder::write_binary_record_micros";

    if (m_find_order_id_mode) {
      m_logger->log_printf(Log::WARN, "%s: Tried to write record in 'find order id mode'", where);
      return 0;
    }

    if (m_manual_resets.size() > 0) {
      manual_reset mr = m_manual_resets[0];
      if (qbdf_time > mr.qbdf_time) {
        m_manual_resets.erase(m_manual_resets.begin());

        m_logger->log_printf(Log::INFO, "%s: Inserting manual reset gap marker at %lu",
                             where, qbdf_time);

        qbdf_gap_marker_micros gap_marker_rec;
        gap_marker_rec.common.format = QBDFMsgFmt::gap_marker_micros;
        gap_marker_rec.common.time = mr.qbdf_time % 100000000;
        gap_marker_rec.common.exch_time_delta = 0;
        gap_marker_rec.common.exch = mr.exch_code;
        gap_marker_rec.common.id = mr.id;
        gap_marker_rec.context = mr.context;
        if (write_binary_record_micros(qbdf_time, &gap_marker_rec, sizeof(qbdf_gap_marker_micros))) {
          m_logger->log_printf(Log::ERROR, "%s: Could not insert manual reset gap marker at %lu",
                               where, qbdf_time);
          m_logger->sync_flush();
          return 1;
        }
      }
    }

    // We write the records to the 1 minute files which START at the time HHMM,
    // so if our record is for 09:30:03 say then HHMM = 0930 and the file it
    // needs to go to is the file 0930
    int hhmm = qbdf_time / 100000000;
    int hh = hhmm / 100;
    int mm = hhmm % 100;

    int hh_adjust = m_bin_tz_adjust / 3600;
    int mm_adjust = (m_bin_tz_adjust - (hh_adjust * 3600)) / 60;

    mm += mm_adjust;
    if (mm >=60) {
      mm -= 60;
      hh += 1;
    } else if (mm < 0) {
      mm += 60;
      hh -= 1;
    }

    hh += hh_adjust;
    if (hh < 0 || hh >= 24) {
      if (m_first_write_record) {
	m_first_hh_value = hh;
	m_first_write_record = false;
      } else if ((hh<0 && m_first_hh_value>=0) || (hh>=24 && m_first_hh_value<24)) {
	m_logger->log_printf(Log::ERROR, "%s: Timezone adj crosses date threshold (qbdf_time=%lu adj=%dsecs)",
			     where, qbdf_time, m_bin_tz_adjust);
	m_logger->sync_flush();
	return 1;
      }

      if (hh < 0) {
	hh += 24;
      } else if (hh >= 24) {
	hh -= 24;
      }
    }

    hhmm = hh * 100 + mm;
    if(hhmm == 0) {
      m_logger->log_printf(Log::INFO, "%s: Got time of 000000 for %lu", where, qbdf_time);
    } else if (hh > 23 || hh < 0 || mm < 0 || mm > 59) {
      m_logger->log_printf(Log::ERROR, "%s: Invalid time hh=[%d] mm=[%d] time=[%lu]",
                           where, hh, mm, qbdf_time);
      m_logger->sync_flush();
      return 1;
    }

    // Is this file open yet ?
    if (m_raw_files[hhmm] == 0) {
      char out_file[256];
      if (m_tmp_bin_files) {
        sprintf(out_file,"%s/%02d%02d00.tmp_bin", m_output_path.c_str(),hh,mm);
      } else {
        sprintf(out_file,"%s/%02d%02d00.bin", m_output_path.c_str(),hh,mm);
      }
      m_raw_files[hhmm] = fopen(out_file, m_raw_file_mode[hhmm]);

      if(m_only_one_open_file_at_a_time)
      {
        if(m_prev_hhmm != 0)
        {
          fclose(m_raw_files[m_prev_hhmm]);
          m_raw_files[m_prev_hhmm] = 0;
        }
        m_prev_hhmm = hhmm;
      }

      m_raw_file_mode[hhmm]=(char*) APPEND_MODE; // move to append from now on
      if (m_raw_files[hhmm] == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create new binary file [%s] fopen retcode %d=[%s]",
                             where, out_file, errno, strerror(errno));
        m_logger->sync_flush();
        return 1;
      }
    }

    fwrite(p_rec, rec_size, 1, m_raw_files[hhmm]);

    // Closing files every so often stops system from using too much memory for buffered FILE I/O
    if (++m_out_count % m_rows_between_fcloses == 0) {
      m_logger->log_printf(Log::INFO, "%s: Closing open files after %d rows written", where, m_out_count);
      close_open_files();
    }

    return 0;
  }

  // write out binary record for data after Oct. 03, 2016
  int QBDF_Builder::write_binary_record_nanos(uint64_t qbdf_time, void* p_rec, size_t rec_size)
  {
    const char* where = "QBDF_Builder::write_binary_record_nanos";
    
    if (m_find_order_id_mode) {
      m_logger->log_printf(Log::WARN, "%s: Tried to write record in 'find order id mode'", where);
      return 0;
    }

    if (m_manual_resets.size() > 0) {
      manual_reset mr = m_manual_resets[0];
      if (qbdf_time > mr.qbdf_time) {
        m_manual_resets.erase(m_manual_resets.begin());

        m_logger->log_printf(Log::INFO, "%s: Inserting manual reset gap marker at %lu",
                             where, qbdf_time);

        qbdf_gap_marker_micros gap_marker_rec;    // will replace with gap_marker_nanos ? 
        gap_marker_rec.common.format = QBDFMsgFmt::gap_marker_micros;
        gap_marker_rec.common.time = mr.qbdf_time % 100000000;
        gap_marker_rec.common.exch_time_delta = 0;
        gap_marker_rec.common.exch = mr.exch_code;
        gap_marker_rec.common.id = mr.id;
        gap_marker_rec.context = mr.context;
        if (write_binary_record_micros(qbdf_time, &gap_marker_rec, sizeof(qbdf_gap_marker_micros))) {
          m_logger->log_printf(Log::ERROR, "%s: Could not insert manual reset gap marker at %lu",
                               where, qbdf_time);
          m_logger->sync_flush();
          return 1;
        }
      }
    }

    // We write the records to the 1 minute files which START at the time HHMM,
    // so if our record is for 09:30:03 say then HHMM = 0930 and the file it
    // needs to go to is the file 0930
    int hhmm = qbdf_time / 100000000000UL;
    int hh = hhmm / 100;
    int mm = hhmm % 100;

    int hh_adjust = m_bin_tz_adjust / 3600;
    int mm_adjust = (m_bin_tz_adjust - (hh_adjust * 3600)) / 60;

    mm += mm_adjust;
    if (mm >=60) {
      mm -= 60;
      hh += 1;
    } else if (mm < 0) {
      mm += 60;
      hh -= 1;
    }

    hh += hh_adjust;
    if (hh < 0 || hh >= 24) {
      if (m_first_write_record) {
	m_first_hh_value = hh;
	m_first_write_record = false;
      } else if ((hh<0 && m_first_hh_value>=0) || (hh>=24 && m_first_hh_value<24)) {
	m_logger->log_printf(Log::ERROR, "%s: Timezone adj crosses date threshold (qbdf_time=%lu adj=%dsecs)",
			     where, qbdf_time, m_bin_tz_adjust);
	m_logger->sync_flush();
	return 1;
      }

      if (hh < 0) {
	hh += 24;
      } else if (hh >= 24) {
	hh -= 24;
      }
    }

    hhmm = hh * 100 + mm;
    if(hhmm == 0) {
      m_logger->log_printf(Log::INFO, "%s: Got time of 000000 for %lu", where, qbdf_time);
    } else if (hh > 23 || hh < 0 || mm < 0 || mm > 59) {
      m_logger->log_printf(Log::ERROR, "%s: Invalid time hh=[%d] mm=[%d] time=[%lu]",
                           where, hh, mm, qbdf_time);
      m_logger->sync_flush();
      return 1;
    }

    // Is this file open yet ?
    if (m_raw_files[hhmm] == 0) {
      char out_file[256];
      if (m_tmp_bin_files) {
        sprintf(out_file,"%s/%02d%02d00.tmp_bin", m_output_path.c_str(),hh,mm);
      } else {
        sprintf(out_file,"%s/%02d%02d00.bin", m_output_path.c_str(),hh,mm);
      }
      m_raw_files[hhmm] = fopen(out_file, m_raw_file_mode[hhmm]);

      if(m_only_one_open_file_at_a_time)
      {
        if(m_prev_hhmm != 0)
        {
          fclose(m_raw_files[m_prev_hhmm]);
          m_raw_files[m_prev_hhmm] = 0;
        }
        m_prev_hhmm = hhmm;
      }

      m_raw_file_mode[hhmm]=(char*) APPEND_MODE; // move to append from now on
      if (m_raw_files[hhmm] == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create new binary file [%s] fopen retcode %d=[%s]",
                             where, out_file, errno, strerror(errno));
        m_logger->sync_flush();
        return 1;
      }
    }

    fwrite(p_rec, rec_size, 1, m_raw_files[hhmm]);

    // Closing files every so often stops system from using too much memory for buffered FILE I/O
    if (++m_out_count % m_rows_between_fcloses == 0) {
      m_logger->log_printf(Log::INFO, "%s: Closing open files after %d rows written", where, m_out_count);
      close_open_files();
    }

    return 0;
  }


  // ----------------------------------------------------------------
  // Dump out a binary record into the correct 1 minute file
  // ----------------------------------------------------------------
  int QBDF_Builder::write_binary_record(const string& symbol_str, int symbol_id, int rec_time,
                                        void* p_rec, int rec_size)
  {
    const char* where = "QBDF_Builder::write_binary_record";

    if (m_find_order_id_mode) {
      m_logger->log_printf(Log::WARN, "%s: Tried to write record in 'find order id mode'", where);
      return 0;
    }

    if (m_manual_resets.size() > 0) {
      manual_reset mr = m_manual_resets[0];
      if (rec_time*1000UL > mr.qbdf_time) {
        m_manual_resets.erase(m_manual_resets.begin());

        m_logger->log_printf(Log::INFO, "%s: Inserting manual reset gap marker at %d",
                             where, rec_time);

        qbdf_gap_marker gap_marker_rec;
        gap_marker_rec.hdr.format = QBDFMsgFmt::GapMarker;
        gap_marker_rec.time = rec_time % 100000;
        gap_marker_rec.symbol_id = mr.id;
        gap_marker_rec.context = mr.context;
        if (write_binary_record("", -1, rec_time, &gap_marker_rec, sizeof(qbdf_gap_marker))) {
          m_logger->log_printf(Log::ERROR, "%s: Could not insert manual reset gap marker at %d",
                               where, rec_time);
          m_logger->sync_flush();
          return 1;
        }
      }
    }


    int hhmm = rec_time / 100000;
    if(hhmm == 0) {
      m_logger->log_printf(Log::INFO, "%s: Got time of 000000 for %d", where, rec_time);
    }

    // We write the records to the 1 minute files which START at the time HHMM,
    // so if our record is for 09:30:03 say then HHMM = 0930 and the file it
    // needs to go to is the file 0930
    int hh = hhmm / 100;
    int mm = hhmm % 100;

    int hh_adjust = m_bin_tz_adjust / 3600;
    int mm_adjust = (m_bin_tz_adjust - (hh_adjust * 3600)) / 60;

    mm += mm_adjust;
    if (mm >=60) {
      mm -= 60;
      hh += 1;
    } else if (mm < 0) {
      mm += 60;
      hh -= 1;
    }

    hh += hh_adjust;
    if (hh < 0) {
      hh += 24;
    } else if (hh > 24) {
      hh -= 24;
    }
//    if (hh < 0 || hh >= 24) {
//      m_logger->log_printf(Log::ERROR, "%s: Timezone adj crosses date threshold (rec_time=%d adj=%dsecs)",
//                           where, rec_time, m_bin_tz_adjust);
//      m_logger->sync_flush();
//      return 1;
//    }

    hhmm = hh * 100 + mm;

    if (hh > 23 || hh < 0 || mm < 0 || mm > 59) {
      m_logger->log_printf(Log::ERROR, "%s: Invalid time hh=[%d] mm=[%d] time=[%d] symbol=[%s] id=[%d]",
                           where, hh, mm, rec_time, symbol_str.c_str(), symbol_id);
      m_logger->sync_flush();
      return 1;
    }

    // Is this file open yet ?
    if (m_raw_files[hhmm] == 0) {
      char out_file[256];
      if (m_tmp_bin_files) {
        sprintf(out_file,"%s/%02d%02d00.tmp_bin", m_output_path.c_str(),hh,mm);
      } else {
        sprintf(out_file,"%s/%02d%02d00.bin", m_output_path.c_str(),hh,mm);
      }
      m_raw_files[hhmm] = fopen(out_file, m_raw_file_mode[hhmm]);

      m_raw_file_mode[hhmm]=(char*) APPEND_MODE; // move to append from now on
      if (m_raw_files[hhmm] == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create new binary file [%s] fopen retcode %d=[%s]",
                             where, out_file, errno, strerror(errno));
        m_logger->sync_flush();
        return 1;
      }
    }

    fwrite(p_rec, rec_size, 1, m_raw_files[hhmm]);

    // Closing files every so often stops system from using too much memory for buffered FILE I/O
    if (++m_out_count % m_rows_between_fcloses == 0) {
      m_logger->log_printf(Log::INFO, "%s: Closing open files after %d rows written", where, m_out_count);
      close_open_files();
    }

    return 0;
  }


  int QBDF_Builder::write_link_record(uint32_t symbol_id, string vendor_id,
                                      string ticker, uint32_t link_id, char side)
  {
    const char* where = "QBDF_Builder::write_link_record";

    if (m_link_file == 0) {
      char out_file[1024];
      sprintf(out_file, "%s/link_ids.csv", m_output_path.c_str());

      m_link_file = fopen(out_file, "w");
      if (m_link_file == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create link file [%s] fopen retcode %d=[%s]",
                             where, out_file, errno, strerror(errno));
        m_logger->sync_flush();
        return 1;
      }
      fprintf(m_link_file, "qbdf_id,vendor_id,ticker,link_id,side\n");
    }

    fprintf(m_link_file, "%u,%s,%s,%u,%c\n", symbol_id, vendor_id.c_str(),
            ticker.c_str(), link_id, side);
    return 0;
  }

  link_record_map_t QBDF_Builder::get_link_records(const string& link_file) {
    const char* where = "QBDF_Builder::read_link_records";

    link_record_map_t link_record_map;
    m_logger->log_printf(Log::INFO, "%s: Reading link records from %s", where, link_file.c_str());
    CSV_Parser csv_reader(link_file);

    int vendor_id_posn = csv_reader.column_index("vendor_id");
    int ticker_posn = csv_reader.column_index("ticker", false); // do not throw exception
    int link_id_posn = csv_reader.column_index("link_id");
    int side_posn = csv_reader.column_index("side");

    CSV_Parser::state_t s = csv_reader.read_row();
    while (s == CSV_Parser::OK) {
      string vendor_id = csv_reader.get_column(vendor_id_posn);
      string ticker = "";
      if (ticker_posn >= 0) {
        ticker = csv_reader.get_column(ticker_posn);
      }
      uint32_t link_id = strtol(csv_reader.get_column(link_id_posn), 0, 10);
      char side = csv_reader.get_column(side_posn)[0];

      if (!vendor_id.empty()) {
        link_record_map["vendor_id"][vendor_id][link_id] = side;
      }

      if (!ticker.empty()) {
        link_record_map["ticker"][ticker][link_id] = side;
      }

      s = csv_reader.read_row();
    }
    return link_record_map;
  }


  int QBDF_Builder::write_cancel_correct_record(uint32_t symbol_id, int rec_time, int orig_rec_time,
                                                string vendor_id, string type, uint64_t orig_seq_no,
                                                double orig_price, int orig_size, string orig_cond,
                                                double new_price, int new_size, string new_cond)
  {
    const char* where = "QBDF_Builder::write_cancel_correct_record";

    if (m_cancel_correct_file == 0) {
      char out_file[1024];
      sprintf(out_file, "%s/cancel_correct.csv", m_output_path.c_str());

      m_cancel_correct_file = fopen(out_file, "w");
      if (m_cancel_correct_file == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create cancel/correct file [%s] fopen retcode %d=[%s]",
                             where, out_file, errno, strerror(errno));
        m_logger->sync_flush();
        return 1;
      }
      fprintf(m_cancel_correct_file, "time,qbdf_id,vendor_id,type,orig_seq_no,orig_time,orig_price,orig_size,orig_cond,new_price,new_size,new_cond\n");
    }

    fprintf(m_cancel_correct_file, "%d,%u,%s,%s,%lu,%d,%f,%d,%s,%f,%d,%s\n",
            rec_time, symbol_id, vendor_id.c_str(), type.c_str(), orig_seq_no, orig_rec_time,
            orig_price, orig_size, orig_cond.c_str(), new_price, new_size, new_cond.c_str());
    return 0;
  }


  int QBDF_Builder::write_gap_summary_record(int rec_time, size_t context, uint64_t expected_seq_no,
                                             uint64_t received_seq_no)
  {
    const char* where = "QBDF_Builder::write_gap_summary_record";

    if (m_gap_summary_file == 0) {
      char out_file[1024];
      sprintf(out_file, "%s/gap_summary.csv", m_output_path.c_str());

      m_gap_summary_file = fopen(out_file, "w");
      if (m_gap_summary_file == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create gap summary file [%s] fopen retcode %d=[%s]",
                             where, out_file, errno, strerror(errno));
        m_logger->sync_flush();
        return 1;
      }
      fprintf(m_gap_summary_file, "time,context,expected_seq_no,received_seq_no\n");
    }

    fprintf(m_gap_summary_file, "%d,%lu,%lu,%lu\n", rec_time, context, expected_seq_no, received_seq_no);
    return 0;
  }

  string QBDF_Builder::remove_time_formatting(const string &time_str, bool micros) {
    size_t str_size;
    if (micros)
      str_size = 12;
    else
      str_size = 9;

    string temp(str_size, '0');
    size_t parse_posn = 0;
    size_t insert_posn = 0;
    size_t first_colon_posn = time_str.find(":");

    if (first_colon_posn == 1)
      ++insert_posn;
    else if (first_colon_posn > 2) {
      parse_posn = first_colon_posn - 2;
      if (time_str[parse_posn] == ' ')
        ++insert_posn;
    }

    for (size_t i = parse_posn; i < time_str.length(); ++i) {
      if (time_str[i] == ':' || time_str[i] == '.' || time_str[i] == ' ')
        continue;

      temp.replace(insert_posn++, 1, 1, time_str[i]);
    }
    return temp;
  }

  int QBDF_Builder::get_exch_time_delta(uint64_t recv_time, uint64_t exch_time) {
    int recv_hh = recv_time / 10000000000;
    int recv_mm = (recv_time - (recv_hh * 10000000000)) / 100000000;
    int recv_micros = recv_time % 100000000;
    uint64_t recv_micros_since_midnight = (recv_hh * 3600000000) + (recv_mm * 60000000) + recv_micros;
    uint64_t recv_mins_since_midnight = (recv_hh * 60) + recv_mm;

    int exch_hh = exch_time / 10000000000;
    int exch_mm = (exch_time - (exch_hh * 10000000000)) / 100000000;
    int exch_micros = exch_time % 100000000;
    uint64_t exch_micros_since_midnight = (exch_hh * 3600000000) + (exch_mm * 60000000) + exch_micros;
    uint64_t exch_mins_since_midnight = (exch_hh * 60) + exch_mm;

    int exch_time_delta_micros;
    int exch_time_delta_mins = exch_mins_since_midnight - recv_mins_since_midnight;
    if (exch_time_delta_mins >= 60) {
      exch_time_delta_micros = (exch_micros_since_midnight - recv_micros_since_midnight - 3600000000);
    } else {
      exch_time_delta_micros = (exch_micros_since_midnight - recv_micros_since_midnight);
    }
    return exch_time_delta_micros;
  }
}
