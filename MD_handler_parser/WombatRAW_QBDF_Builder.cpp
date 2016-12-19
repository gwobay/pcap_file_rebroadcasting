#include <fstream>
#include <iostream>
#include <limits>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>

#include <Util/CSV_Parser.h>
#include <Util/File_Utils.h>
#include <Data/QBDF_Util.h>
#include <Data/WombatRAW_QBDF_Builder.h>
#include <Data/Wombat_Util.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

#define micros_per_day 86400000000

  string WombatRAW_QBDF_Builder::get_date_from_time_field(string& ts) {
    size_t dash_posn;
    while ((dash_posn = ts.find('-')) != string::npos) {
      ts.erase(dash_posn, 1);
    }

    size_t space_posn = ts.find(' ');
    if (space_posn == string::npos) {
      return "";
    }
    return ts.substr(0, space_posn);
  }


  bool WombatRAW_QBDF_Builder::get_exch_time_delta(uint64_t recv_time, uint64_t exch_time,
                                                   int& exch_time_delta, bool& delta_in_millis) {
    int64_t temp_delta = qbdf_time_to_micros(exch_time) - qbdf_time_to_micros(recv_time);
    if (temp_delta >= micros_per_day) {
      // something is wrong here, exch time delta shouldn't be a more than 24 hours
      return false;
    } else if (labs(temp_delta) >= numeric_limits<int>::max()) {
      if (m_qbdf_version.compare("1.0") == 0) {
        m_logger->log_printf(Log::WARN, "%s: Exch delta too big, set qbdf_version > 1.0 for delta_in_millis!",
                             "WombatRAW_QBDF_Builder::get_exch_time_delta");
      }
      exch_time_delta = (int)(temp_delta / 1000L);
      delta_in_millis = true;
    } else {
      exch_time_delta = (int)temp_delta;
      delta_in_millis = false;
    }

    return true;
  }

  #define GEN_QBDF_IMBALANCE()                                                      \
    string near_price_str = get_map_value(name_value_map, "wUncrossPrice");         \
    bool near_price_exists = (near_price_str != "");                                \
    MDPrice near_price = MDPrice::from_fprice(strtod(near_price_str.c_str(), 0));   \
                                                                                    \
    if (!near_price_exists) {                                                       \
      near_price_str = get_map_value(name_value_map, "wIndicationPrice");    \
      near_price_exists = (near_price_str != "");                              \
      near_price = MDPrice::from_fprice(strtod(near_price_str.c_str(), 0)); \
    }                                                                               \
                                                                                    \
    string paired_size_str = get_map_value(name_value_map, "wUncrossVolume");       \
    bool paired_size_exists = (paired_size_str != "");                              \
    int paired_size = strtol(paired_size_str.c_str(), 0, 10);                       \
                                                                                    \
    if (!paired_size_exists) {                                                      \
      paired_size_str = get_map_value(name_value_map, "wMatchVolume");              \
      paired_size_exists = (paired_size_str != "");                                 \
      paired_size = strtol(paired_size_str.c_str(), 0, 10);                         \
    }                                                                               \
                                                                                    \
    if (near_price_exists && paired_size_exists) {                                  \
      m_prev_indicative_prices[symbol_id] = near_price;                             \
      m_prev_indicative_sizes[symbol_id] = paired_size;                             \
    } else if (!near_price_exists && !paired_size_exists) {                         \
      near_price = m_prev_indicative_prices[symbol_id];                             \
      paired_size = m_prev_indicative_sizes[symbol_id];		                    \
    } else if (!near_price_exists) {                                                \
      if (m_prev_indicative_prices[symbol_id].iprice32() > 0) {                     \
        near_price = m_prev_indicative_prices[symbol_id];                           \
      }                                                                             \
      m_prev_indicative_sizes[symbol_id] = paired_size;                             \
    } else {                                                                        \
      int prev_paired_size = m_prev_indicative_sizes[symbol_id];                    \
      if (prev_paired_size > 0) {                                                   \
        paired_size = prev_paired_size;                                             \
      }                                                                             \
      m_prev_indicative_prices[symbol_id] = near_price;                             \
    }                                                                               \
                                                                                    \
    if (!gen_qbdf_imbalance_micros_qbdf_time(this, symbol_id, recv_time, exch_time_delta, \
                                             exch_char, MDPrice(), near_price, MDPrice(), \
                                             paired_size, 0, ' ', ' ')) {                 \
      m_logger->log_printf(Log::ERROR, "%s: Unable to write imbalance for symbol=[%s] time=[%lu]", \
                           where, symbol.c_str(), recv_time);                     \
      return 1;                                                                   \
    }                                                                             \

  // ----------------------------------------------------------------
  // load in raw file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int WombatRAW_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "WombatRAW_QBDF_Builder::process_raw_file";

    // BMC 20101203: Only imbalance and book messages implemented at this point
    bool exclude_imbalance = m_params.getDefaultBool("exclude_imbalance", false);
    //bool exclude_orders = m_params.getDefaultBool("exclude_orders", false);
    bool exclude_quotes = m_params.getDefaultBool("exclude_quotes", false);
    bool exclude_status = m_params.getDefaultBool("exclude_status", false);
    bool exclude_trades = m_params.getDefaultBool("exclude_trades", false);

    glob_t glob_buf;
    glob_buf.gl_offs = 0;
    int glob_ret = glob(file_name.c_str(), GLOB_DOOFFS, NULL, &glob_buf);
    if (glob_ret || glob_buf.gl_pathc == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to find files with glob on %s", where, file_name.c_str());
      m_logger->sync_flush();
      return 1;
    }

    string line;
    string symbol_exch;
    string ts;
    string ts_date;

    WombatFields w_entry(WombatFields::W_ENTRY);
    WombatFields w_entry_id(WombatFields::W_ENTRY_ID);
    WombatFields w_entry_action(WombatFields::W_ENTRY_ACTION);
    WombatFields w_entry_size(WombatFields::W_ENTRY_SIZE);
    WombatFields w_entry_time(WombatFields::W_ENTRY_TIME);
    WombatFields w_pl_side(WombatFields::W_PL_SIDE);
    WombatFields w_pl_size(WombatFields::W_PL_SIZE);
    WombatFields w_pl_action(WombatFields::W_PL_ACTION);
    WombatFields w_pl_price(WombatFields::W_PL_PRICE);

    Wombat_Util m_wombat_util;

    name_value_map_t name_value_map;
    vector<field_map_t> parsed_price_levels;
    vector<field_map_t> parsed_entries;
    vector<string> tokens;

    for (size_t i = 0; i < glob_buf.gl_pathc; ++i) {
      string glob_file_name = (glob_buf.gl_pathv[i]);
      m_logger->log_printf(Log::INFO, "%s: Reading %s", where, glob_file_name.c_str());

      boost::iostreams::filtering_istream in;
      if(has_gzip_extension(glob_file_name)) {
        in.push(boost::iostreams::gzip_decompressor());
      }
      in.push(boost::iostreams::file_source(glob_file_name));

      uint64_t last_recv_time = 0;
      uint64_t last_book_time = 0;
      uint64_t last_quote_time = 0;
      uint64_t last_src_time = 0;
      uint64_t last_trade_time = 0;

      char recv_time_buffer[12];
      char exch_time_buffer[12];
      int line_num = 0;
      while (getline(in, line)) {
        line_num++;
        tokens.clear();
        tokenize_quoted(line.c_str(), &tokens, ',', '\\', '{', '}');

        name_value_map.clear();
        for (size_t i = 0; i < tokens.size(); i++) {
          string token = tokens[i];
          size_t token_delim_posn = token.find_first_of('\1');
          string field_name = token.substr(0, token_delim_posn);
          string field_value = token.substr(token_delim_posn+1);
          name_value_map[field_name] = field_value;
        }

        int msg_type = atoi(get_map_value(name_value_map, "MdMsgType").c_str());
        int seq_num = atoi(get_map_value(name_value_map, "MdSeqNum").c_str());
        symbol_exch = get_map_value(name_value_map, "SubscriptionSymbol");
        ts = get_map_value(name_value_map, "wApplicationTime");

        size_t period_posn = ts.rfind(".");
        if (period_posn == string::npos)
          continue;

        while (ts.size() - period_posn <= 6) {
          ts.insert(period_posn+1, "0");
        }

        if (symbol_exch.empty() || ts.empty())
          continue;

        size_t delim_posn = symbol_exch.find('.');
        if (delim_posn == string::npos)
          continue;

        string symbol = symbol_exch.substr(0, delim_posn);
        string exch = symbol_exch.substr(delim_posn+1);
        map<string,char>::iterator exchange_iter = m_vendor_exch_to_qbdf_exch.find(exch);
        if (exchange_iter == m_vendor_exch_to_qbdf_exch.end()) {
          m_logger->log_printf(Log::DEBUG, "%s: Unable to find mapping for exch %s", where, exch.c_str());
          continue;
        }

        char exch_char = exchange_iter->second;
        uint16_t symbol_id = process_taq_symbol(symbol_exch);
        if (symbol_id == 0) {
          m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]",
                               where, symbol.c_str());
          m_logger->sync_flush();
          return 1;
        }

        while (symbol_id >= m_latest_seq_num.size())
	  m_latest_seq_num.push_back(0);

        while (symbol_id >= m_prev_indicative_prices.size())
          m_prev_indicative_prices.push_back(MDPrice());

        while (symbol_id >= m_prev_indicative_sizes.size())
          m_prev_indicative_sizes.push_back(0);

        bool is_gap = false;
        if (seq_num > static_cast<int>(m_latest_seq_num[symbol_id])+1 || msg_type == WombatMamaMsgType::OrderBookRecap) {
          //gap
          is_gap = true;
        } else if (seq_num > 0 && seq_num <= static_cast<int>(m_latest_seq_num[symbol_id])) {
          // Wombat doesn't increment seq no unless there is new data
        } else {
          m_latest_seq_num[symbol_id] = seq_num;
        }


        ts = remove_time_formatting(ts);
        if (ts == "000000000" or ts == "000000000000")
          continue;

        strncpy(recv_time_buffer, ts.c_str(), 12);
        uint64_t recv_time = fixed_length_strtol(recv_time_buffer, 12);
        if (recv_time == 0) {
          recv_time = last_recv_time;
        } else {
          if (m_recv_time_adj_micros) {
            uint64_t recv_time_micros = qbdf_time_to_micros(recv_time);
            recv_time_micros += m_recv_time_adj_micros;
            recv_time = micros_to_qbdf_time(recv_time_micros);
          }
          last_recv_time = recv_time;
        }

        if (is_gap) {
          gen_qbdf_gap_marker_micros_qbdf_time(this, recv_time, symbol_id, exch_char, 0);
          m_latest_seq_num[symbol_id] = seq_num;
        }

        if (msg_type == WombatMamaMsgType::InitialValue || msg_type == WombatMamaMsgType::GeneralUpdate) {
          ts = get_map_value(name_value_map, "wSrcTime");
          ts_date = get_date_from_time_field(ts);
          if (ts_date != "" && ts_date != m_date) {
            ts = "000000000000";
          } else {
            ts = remove_time_formatting(ts);
            if (ts.size() == 9)
              ts.append("000");
          }

          strncpy(exch_time_buffer, ts.c_str(), 12);
          uint64_t exch_time = fixed_length_strtol(exch_time_buffer, 12);
          if (exch_time == 0) {
            exch_time = last_src_time;
          } else {
            last_src_time = exch_time;
          }

          //cout << "symbol=" << symbol << " ts=" << ts << " recv_time=" << recv_time << " exch_time=" << exch_time << endl;
          int exch_time_delta;
          bool delta_in_millis;
          if (!get_exch_time_delta(recv_time, exch_time, exch_time_delta, delta_in_millis)) {
            m_logger->log_printf(Log::ERROR, "%s: Exch time delta is more than 24 hours: [recv_time=%lu] [exch_time=%lu]",
                                 where, recv_time, exch_time);
            continue;
          }


          if (!exclude_status && is_in_map(name_value_map, "wSecurityStatus")) {
            string status_str = get_map_value(name_value_map, "wSecurityStatus");
            string orig_status_str = get_map_value(name_value_map, "wSecurityStatusOrig");

            QBDFStatusType qbdf_status;
            if (status_str == "Closed" || status_str == "AtLast") {
              qbdf_status = QBDFStatusType::Closed;
            } else if (status_str == "Auction" || status_str == "Crossing") {
              qbdf_status = QBDFStatusType::Auction;
            } else if (status_str == "Normal") {
              qbdf_status = QBDFStatusType::Open;
            } else if (status_str == "Halted") {
              qbdf_status = QBDFStatusType::Halted;
            } else if (status_str == "Deleted") {
              qbdf_status = QBDFStatusType::Suspended;
            } else {
              m_logger->log_printf(Log::INFO, "%s: Converting %s wSecurityStatus to QBDFStatusType::Invalid, symbol=[%s] time=[%lu]",
                                   where, status_str.c_str(), symbol.c_str(), recv_time);
              qbdf_status = QBDFStatusType::Invalid;
            }

            if (qbdf_status != QBDFStatusType::Invalid) {
              if (!gen_qbdf_status_micros_qbdf_time(this, symbol_id, recv_time, exch_time_delta,
                                                    exch_char, qbdf_status.index(), orig_status_str)) {
                m_logger->log_printf(Log::ERROR, "%s: Unable to write status for symbol=[%s] time=[%lu]",
                                     where, symbol.c_str(), recv_time);
                return 1;
              }
            }
          }

          if (!exclude_imbalance && get_map_value(name_value_map, "wUncrossPriceInd") == "I") {
            GEN_QBDF_IMBALANCE();
          }

        } else if (msg_type == WombatMamaMsgType::QuoteUpdate) {
          if (exclude_quotes && exclude_imbalance)
            continue;

          ts = get_map_value(name_value_map, "wQuoteTime");
          ts_date = get_date_from_time_field(ts);
          if (ts_date != "" && ts_date != m_date) {
            ts = "000000000000";
          } else {
            ts = remove_time_formatting(ts);
            if (ts.size() == 9)
              ts.append("000");
          }

          strncpy(exch_time_buffer, ts.c_str(), 12);
          uint64_t exch_time = fixed_length_strtol(exch_time_buffer, 12);
          if (exch_time == 0 and last_quote_time) {
            exch_time = last_quote_time;
          } else {
            last_quote_time = exch_time;
          }

          int exch_time_delta;
          bool delta_in_millis;
          if (!get_exch_time_delta(recv_time, exch_time, exch_time_delta, delta_in_millis)) {
            m_logger->log_printf(Log::ERROR, "%s: Exch time delta is more than 24 hours: [recv_time=%lu] [exch_time=%lu]",
                                 where, recv_time, exch_time);
            continue;
          }

          if (!exclude_imbalance && get_map_value(name_value_map, "wUncrossPriceInd") == "I") {
            GEN_QBDF_IMBALANCE();
          }

          if (exclude_quotes)
            continue;

          string bid_price_str = get_map_value(name_value_map, "wBidPrice");
          MDPrice bid_price = MDPrice::from_fprice(strtod(bid_price_str.c_str(), 0));

          string ask_price_str = get_map_value(name_value_map, "wAskPrice");
          MDPrice ask_price = MDPrice::from_fprice(strtod(ask_price_str.c_str(), 0));

          string bid_size_str = get_map_value(name_value_map, "wBidSize");
          uint32_t bid_size = (uint32_t)strtoll(bid_size_str.c_str(), 0, 10);

          string ask_size_str = get_map_value(name_value_map, "wAskSize");
          uint32_t ask_size = (uint32_t)strtoll(ask_size_str.c_str(), 0, 10);

          string cond_str = get_map_value(name_value_map, "wQuoteQualifier");
          char cond = ' ';
          if (cond_str.size() > 0)
            cond = cond_str.c_str()[0];

          if (!gen_qbdf_quote_micros_qbdf_time(this, symbol_id, recv_time, exch_time_delta,
                                               exch_char, cond, bid_price, ask_price,
                                               bid_size, ask_size)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write quote for symbol=[%s] time=[%lu]",
                                 where, symbol.c_str(), recv_time);
            return 1;
          }
        } else if (msg_type == WombatMamaMsgType::TradeUpdate) {
          if (exclude_trades)
            continue;

          // Wombat VTX recording had bogus wTradeTime values between
          // 20120423 and 20120727 inclusive, use wSrcTime instead
          if (exch == "XVTX" && m_date >= "20120423" && m_date <= "20120727") {
            ts = get_map_value(name_value_map, "wSrcTime");
          } else {
            ts = get_map_value(name_value_map, "wTradeTime");
          }

          ts_date = get_date_from_time_field(ts);
          if (ts_date != "" && ts_date != m_date) {
            ts = "000000000000";
          } else {
            ts = remove_time_formatting(ts);
            if (ts.size() == 9)
              ts.append("000");
          }

          strncpy(exch_time_buffer, ts.c_str(), 12);
          uint64_t exch_time = fixed_length_strtol(exch_time_buffer, 12);
          if (exch_time == 0 and last_trade_time) {
            exch_time = last_trade_time;
          } else {
            last_trade_time = exch_time;
          }

          int exch_time_delta;
          bool delta_in_millis;
          if (!get_exch_time_delta(recv_time, exch_time, exch_time_delta, delta_in_millis)) {
            m_logger->log_printf(Log::ERROR, "%s: Exch time delta is more than 24 hours: [recv_time=%lu] [exch_time=%lu]",
                                 where, recv_time, exch_time);
            continue;
          }

          string cond_str;
          if (is_in_map(name_value_map, "wSaleCondition")) {
            cond_str = get_map_value(name_value_map, "wSaleCondition");
          } else {
            cond_str = get_map_value(name_value_map, "wOrigCondition");
          }

          if (cond_str.length() > 4 && cond_str[0] == '2' && cond_str[1] == '4' && cond_str[3] == ':') {
            cond_str = cond_str.substr(4);
          }

          string trade_size_str = get_map_value(name_value_map, "wTradeVolume");
          uint32_t trade_size = (uint32_t)strtoll(trade_size_str.c_str(), 0, 10);

          string trade_price_str = get_map_value(name_value_map, "wTradePrice");
          MDPrice md_price = MDPrice::from_fprice(strtod(trade_price_str.c_str(), 0));

          if (!gen_qbdf_trade_micros_qbdf_time(this, symbol_id, recv_time, exch_time_delta,
                                               exch_char, ' ', '0', cond_str,
                                               trade_size, md_price)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write quote for symbol=[%s] time=[%lu]",
                                 where, symbol.c_str(), recv_time);
            return 1;
          }
        } else if (msg_type == WombatMamaMsgType::OrderBookInit ||
		   msg_type == WombatMamaMsgType::OrderBookDelta ||
                   msg_type == WombatMamaMsgType::OrderBookRecap) {

          ts = get_map_value(name_value_map, "wBookTime");
          ts_date = get_date_from_time_field(ts);
          if (ts_date != "" && ts_date != m_date) {
            ts = "000000000000";
          } else {
            ts = remove_time_formatting(ts);
            if (ts.size() == 9)
              ts.append("000");
          }

          strncpy(exch_time_buffer, ts.c_str(), 12);
          uint64_t exch_time = fixed_length_strtol(exch_time_buffer, 12);
          if (exch_time == 0 and last_book_time) {
            exch_time = last_book_time;
          } else {
            last_book_time = exch_time;
          }

          int exch_time_delta;
          bool delta_in_millis;
          if (!get_exch_time_delta(recv_time, exch_time, exch_time_delta, delta_in_millis)) {
            m_logger->log_printf(Log::ERROR, "%s: Exch time delta is more than 24 hours: [recv_time=%lu] [exch_time=%lu]",
                                 where, recv_time, exch_time);
            continue;
          }

          bool valid_data = false;
          char side = 'B';
          string pl_side_str = get_map_value(name_value_map, "wPlSide");
          if (!pl_side_str.empty()) {
            side = pl_side_str[0];
            if (side == 'A' || side == 'B') {
              side = side == 'B' ? 'B' : 'S';
            } else {
              side = (char)strtol(pl_side_str.c_str(), 0, 10) == 'B' ? 'B' : 'S';
            }
            valid_data = true;
          }

          uint32_t pl_size = 0;
          string pl_size_str = get_map_value(name_value_map, "wPlSize");
          if (!pl_size_str.empty()) {
            pl_size = strtol(pl_size_str.c_str(), 0, 10);
            valid_data = true;
          }

          MDPrice price;
          string pl_price_str = get_map_value(name_value_map, "wPlPrice");
          if (!pl_price_str.empty()) {
            price = MDPrice::from_fprice(strtod(pl_price_str.c_str(), NULL));
            valid_data = true;
          }

          uint64_t order_id = 0;
          string entry_id_str = get_map_value(name_value_map, "wEntryId");
          if (!entry_id_str.empty()) {
            order_id = strtoll(entry_id_str.c_str(), 0, 10);
            valid_data = true;
          }

          uint32_t size = 0;
          string entry_size_str = get_map_value(name_value_map, "wEntrySize");
          if (!entry_size_str.empty()) {
            size = strtol(entry_size_str.c_str(), 0, 10);
            valid_data = true;
          }

          char entry_action = 'D';
          string entry_action_str = get_map_value(name_value_map, "wEntryAction");
          if (!entry_action_str.empty()) {
            entry_action = entry_action_str.c_str()[0];
            valid_data = true;
          }

          char pl_action = 'D';
          string pl_action_str = get_map_value(name_value_map, "wPlAction");
          if (!pl_action_str.empty()) {
            pl_action = pl_action_str.c_str()[0];
            valid_data = true;
          }

          uint64_t unique_order_id;

          if (m_level_feed) {
            if (valid_data) {
              if (pl_action == 'D') {
                gen_qbdf_level_iprice64_qbdf_time(this, symbol_id, recv_time, exch_time_delta,
                                                  exch_char, side, ' ', ' ', ' ', 0, 0, price,
                                                  true, false, delta_in_millis);
              } else {
                gen_qbdf_level_iprice64_qbdf_time(this, symbol_id, recv_time, exch_time_delta,
                                                  exch_char, side, ' ', ' ', ' ', 0, size, price,
                                                  true, false, delta_in_millis);
              }
            }
          } else if (!entry_action_str.empty() && order_id != 0) {
            unique_order_id = m_wombat_util.get_unique_order_id(symbol_id, order_id, price.iprice64());
            switch (entry_action) {
            case 'A':
              gen_qbdf_order_add_micros_qbdf_time(this, symbol_id, unique_order_id,
                                                  recv_time, exch_time_delta,
                                                  exch_char, side, size, price,
                                                  delta_in_millis);
              break;
            case 'D':
              gen_qbdf_order_modify_micros_qbdf_time(this, symbol_id, unique_order_id,
                                                     recv_time, exch_time_delta,
                                                     exch_char, side, 0, price,
                                                     delta_in_millis);
              break;
            case 'U':
              gen_qbdf_order_modify_micros_qbdf_time(this, symbol_id, unique_order_id,
                                                     recv_time, exch_time_delta,
                                                     exch_char, side, pl_size, price,
                                                     delta_in_millis);
              break;
            default:
              m_logger->log_printf(Log::ERROR, "%s: Unknown entry_action: %c", where, entry_action);
              m_logger->log_printf(Log::ERROR, "%s: %s", where, entry_action_str.c_str());
              m_logger->log_printf(Log::ERROR, " -> %s", line.c_str());
            }
          }

          string price_levels_str = get_map_value(name_value_map, "wPriceLevels");
          vector<string> price_levels = get_vector_from_string(price_levels_str);
          for (size_t i = 0; i < price_levels.size(); i++) {
            bool valid_data = false;
            string price_level_str = price_levels[i];
            field_map_t price_level_map = get_map_from_string(price_level_str);
            field_map_t::iterator plmi;

            // defaults
            side = 'B';
            pl_size = 0;
            pl_action = 'D';
            entry_action = 'D';

            vector<string> entries;
            plmi = price_level_map.find(w_entry.value());
            if (plmi != price_level_map.end()) {
              entries = get_vector_from_string(plmi->second);
            } else if (!m_level_feed) {
              continue;
            }

            plmi = price_level_map.find(w_pl_side.value());
            if (plmi != price_level_map.end()) {
              string pl_side_str = plmi->second;
              if (!pl_side_str.empty()) {
                side = pl_side_str[0];
                if (side == 'A' || side == 'B') {
                  side = side == 'B' ? 'B' : 'S';
                } else {
                  side = (char)strtol(pl_side_str.c_str(), 0, 10) == 'B' ? 'B' : 'S';
                }
                valid_data = true;
              }
            }

            plmi = price_level_map.find(w_pl_price.value());
            if (plmi != price_level_map.end()) {
              string pl_price_str = plmi->second;
              if (!pl_price_str.empty()) {
                price = MDPrice::from_fprice(strtod(pl_price_str.c_str(), NULL));
                valid_data = true;
              }
            }

            plmi = price_level_map.find(w_pl_action.value());
            if (plmi != price_level_map.end()) {
              string pl_action_str = plmi->second;
              if (!pl_action_str.empty()) {
                pl_action = pl_action_str.c_str()[0];
                valid_data = true;
              }
            }

            plmi = price_level_map.find(w_pl_size.value());
            if (plmi != price_level_map.end()) {
              string pl_size_str = plmi->second;
              if (!pl_size_str.empty()) {
                pl_size = strtol(pl_size_str.c_str(), 0, 10);
                valid_data = true;
              }
            }

            if (m_level_feed) {
              if (valid_data) {
                if (pl_action == 'D') {
                  gen_qbdf_level_iprice64_qbdf_time(this, symbol_id, recv_time, exch_time_delta,
                                                    exch_char, side, ' ', ' ', ' ', 0, 0, price,
                                                    true, false, delta_in_millis);
                } else {
                  gen_qbdf_level_iprice64_qbdf_time(this, symbol_id, recv_time, exch_time_delta,
                                                    exch_char, side, ' ', ' ', ' ', 0, pl_size,
                                                    price, true, false, delta_in_millis);
                }
              }
              continue;
            }

            for (size_t j = 0; j < entries.size(); j++) {
              uint64_t entry_time = exch_time;

              int entry_time_delta;
              bool delta_in_millis;
              if (!get_exch_time_delta(recv_time, exch_time, entry_time_delta, delta_in_millis)) {
                m_logger->log_printf(Log::ERROR, "%s: Entry time delta is more than 24 hours: [recv_time=%lu] [exch_time=%lu]",
                                     where, recv_time, exch_time);
                continue;
              }

              string entry_str = entries[j];
              field_map_t entry_map = get_map_from_string(entry_str);
              field_map_t::iterator emi;

              emi = entry_map.find(w_entry_id.value());
              if (emi == entry_map.end())
                continue;

              char* p_end = 0;
              order_id = strtoll(emi->second.c_str(), &p_end, 10);
              if (p_end == 0 || *p_end != 0) {
                order_id = strtoll(emi->second.c_str(), 0, 36);
              }

              if (order_id == 0)
                continue;

              // This is only to read in book init
              if (exch_time == 0 || msg_type == WombatMamaMsgType::OrderBookInit) {
                emi = entry_map.find(w_entry_time.value());
                if (emi != entry_map.end()) {
                  ts = emi->second;
                  ts_date = get_date_from_time_field(ts);
                  if (ts_date != "" && ts_date != m_date) {
                    ts = "000000000000";
                  } else {
                    ts = remove_time_formatting(ts);
                    if (ts.size() == 9)
                      ts.append("000");
                  }

                  strncpy(exch_time_buffer, ts.c_str(), 12);
                  entry_time = fixed_length_strtol(exch_time_buffer, 12);

                  if (!get_exch_time_delta(recv_time, entry_time, entry_time_delta, delta_in_millis)) {
                    m_logger->log_printf(Log::ERROR, "%s: Entry time delta is more than 24 hours: [recv_time=%lu] [exch_time=%lu]",
                                         where, recv_time, exch_time);
                    continue;
                  }
                }
              }

              emi = entry_map.find(w_entry_action.value());
              if (emi == entry_map.end()) {
                entry_action = 'D';
              } else {
                int entry_action_int = strtol(emi->second.c_str(), 0, 10);
                if (entry_action_int < 65) {
                  entry_action = emi->second.c_str()[0];
                } else {
                  entry_action = (char)entry_action_int;
                }
              }

              emi = entry_map.find(w_entry_size.value());
              if (emi == entry_map.end()) {
                size = 0;
              } else {
                size = strtol(emi->second.c_str(), 0, 10);
              }

              unique_order_id = m_wombat_util.get_unique_order_id(symbol_id, order_id, price.iprice64());

              switch (entry_action) {
              case 'A':
                gen_qbdf_order_add_micros_qbdf_time(this, symbol_id, unique_order_id,
                                                    recv_time, entry_time_delta,
                                                    exch_char, side, size, price,
                                                    delta_in_millis);
                break;
              case 'D':
                gen_qbdf_order_modify_micros_qbdf_time(this, symbol_id, unique_order_id,
                                                       recv_time, entry_time_delta,
                                                       exch_char, side, 0, price,
                                                       delta_in_millis);
                break;
              case 'U':
                gen_qbdf_order_modify_micros_qbdf_time(this, symbol_id, unique_order_id,
                                                       recv_time, entry_time_delta,
                                                       exch_char, side, size, price,
                                                       delta_in_millis);
                break;
              default:
                m_logger->log_printf(Log::ERROR, "%s: Unknown entry_action: %c", where, entry_action);
                m_logger->log_printf(Log::ERROR, " price_level -> %s", price_level_str.c_str());
                m_logger->log_printf(Log::ERROR, " entry -> %s", entry_str.c_str());
              }
            }
	  }
	} else if (msg_type == WombatMamaMsgType::OrderBookClear) {
	  gen_qbdf_gap_marker_micros_qbdf_time(this, recv_time, symbol_id, exch_char, 0);
	}
      }
    }
    return 0;
  }

    vector<string> WombatRAW_QBDF_Builder::get_vector_from_string(const string& str) {
      vector<string> tokens;
      if (str.size() <= 2)
	return tokens;

      tokenize_quoted(str.substr(1, str.size()-2).c_str(), &tokens, ',', '\\', '{', '}');
      return tokens;
    }

    field_map_t WombatRAW_QBDF_Builder::get_map_from_string(const string& str) {
      field_map_t price_level_map;
      if (str.size() <= 2)
	return price_level_map;

      vector<string> tokens;
      tokenize_quoted(str.substr(1, str.size()-2).c_str(), &tokens, ',', '\\', '{', '}');

      for (size_t i = 0; i < tokens.size(); i++) {
	string token = tokens[i];
	size_t token_delim_posn = token.find_first_of('=');
	// skip leading [ and trailing ]
	int64_t field = strtol(token.substr(1, token_delim_posn-1).c_str(), 0, 10);
	string field_value = token.substr(token_delim_posn+1);
	price_level_map[field] = field_value;
      }

      return price_level_map;
    }

    string WombatRAW_QBDF_Builder::get_map_value(const name_value_map_t& data_map, const string& key) {
      name_value_map_citer_t name_value_map_iter = data_map.find(key);
      if (name_value_map_iter == data_map.end()) {
	return "";
      } else {
	return name_value_map_iter->second;
      }
    }

    bool WombatRAW_QBDF_Builder::is_in_map(const name_value_map_t& data_map, const string& key) {
      name_value_map_citer_t name_value_map_iter = data_map.find(key);
      if (name_value_map_iter == data_map.end()) {
	return false;
      }
      return true;
    }
  }
