#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Util/File_Utils.h>
#include <Data/WombatCSV_QBDF_Builder.h>
#include <Data/QBDF_Util.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  string WombatCSV_QBDF_Builder::get_date_from_time_field(string& ts) {
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

  // ----------------------------------------------------------------
  // load in raw file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int WombatCSV_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "WombatCSV_QBDF_Builder::process_raw_file";

    m_prev_prices.clear();
    m_prev_indicative_prices.clear();
    m_prev_indicative_sizes.clear();

    m_prev_prices.push_back(-1.0);
    m_prev_indicative_prices.push_back(-1.0);
    m_prev_indicative_sizes.push_back(-1);

    Wombat_Util m_wombat_util;

    glob_t glob_buf;
    glob_buf.gl_offs = 0;
    int glob_ret = glob(file_name.c_str(), GLOB_DOOFFS, NULL, &glob_buf);
    if (glob_ret || glob_buf.gl_pathc == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Could not find files using glob on %s", where, file_name.c_str());
      return 1;
    }

    bool is_nomura = false;
    if (file_name.find("nomura") != string::npos) {
      is_nomura = true;
      m_logger->log_printf(Log::INFO, "%s: Running in nomura mode", where);
    } else {
      m_logger->log_printf(Log::INFO, "%s: Running in qbtwombat mode", where);
    }

    for (size_t i = 0; i < glob_buf.gl_pathc; ++i) {
      string glob_file_name = (glob_buf.gl_pathv[i]);
      m_logger->log_printf(Log::INFO, "%s: Reading %s", where, glob_file_name.c_str());

      bool use_activity_time = false;
      //bool use_exchange_time = false;
      bool use_line_time = false;
      //bool use_app_time = false;
      if (glob_file_name.find("line_time_sort") != string::npos) {
        use_line_time = true;
      } else if (glob_file_name.find("exch_time_sort") != string::npos) {
        //use_exchange_time = true;
      } else if (glob_file_name.find("act_time_sort") != string::npos) {
        use_activity_time = true;
      } else if (glob_file_name.find("app_time_sort") != string::npos) {
        //use_app_time = true;
      }

      boost::iostreams::filtering_istream in;
      if(has_gzip_extension(glob_file_name)) {
        in.push(boost::iostreams::gzip_decompressor());
      }
      in.push(boost::iostreams::file_source(glob_file_name));

      vector<string> fields;
      string line;

      while (getline(in, line)) {
        fields.clear();
        size_t lastPos = 0;
        size_t nextPos = string::npos;
        while (true) {
          nextPos = line.find(',', lastPos);
          if (nextPos == string::npos) {
            fields.push_back(line.substr(lastPos));
            break;
          }
          fields.push_back(line.substr(lastPos, nextPos - lastPos));
          lastPos = nextPos + 1;
        }
        if (fields.size() <= 4)
          continue;

        string ts;
        string ts_date;
        string symbol_exch;
        string msg_type = fields[0];

        if (msg_type == "quote_update") {
          if (fields.size() < 21)
            continue;

          if (use_line_time) {
            ts = fields[21];
          } else if (use_activity_time) {
            ts = fields[3];
          } else {
            ts = fields[4];
          }
          symbol_exch = fields[5];
        } else if (msg_type == "trade_report") {
          if (fields.size() < 32)
            continue;

          if (use_line_time) {
            ts = fields[34];
          } else if (use_activity_time) {
            ts = fields[3];
          } else {
            ts = fields[4];
          }
          symbol_exch = fields[5];
        } else if (msg_type == "on_msg") {
          if (fields.size() < 8)
            continue;

          if (is_nomura) {
            symbol_exch = fields[3];
            if (use_line_time)
              ts = fields[8];
            else
              ts = fields[1];
          } else {
            symbol_exch = fields[4];
            if (use_line_time)
              ts = fields[9];
            else
              ts = fields[2];
          }
        } else if (msg_type == "security_status") {
          if (fields.size() < 6)
            continue;

          if (use_line_time) {
            ts = fields[6];
          } else {
            ts = fields[3];
          }
          symbol_exch = fields[2];
        } else if (msg_type == "level_delta") {
          if (fields.size() < 19)
            continue;

          symbol_exch = fields[3];
          ts = fields[8]; // price level time
        } else if (msg_type == "level_entry_delta") {
          if (fields.size() < 23)
            continue;

          symbol_exch = fields[4];
          ts = fields[12]; // price level time
        } else {
          continue;
        }

        size_t delimPos = symbol_exch.find('.');
        if (delimPos == string::npos)
          continue;

        string symbol = symbol_exch.substr(0, delimPos);
        string exch = symbol_exch.substr(delimPos+1);
        map<string,char>::iterator exchange_iter = m_vendor_exch_to_qbdf_exch.find(exch);
        if (exchange_iter == m_vendor_exch_to_qbdf_exch.end())
          continue;

        char exch_char = exchange_iter->second;
        ushort symbol_id = process_taq_symbol(symbol_exch);
        if (symbol_id == 0) {
          m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]",
                               where, symbol.c_str());
          return 1;
        }

        if (symbol_id >= m_prev_prices.size())
          m_prev_prices.push_back(-1.0);

        if (symbol_id >= m_prev_indicative_prices.size())
          m_prev_indicative_prices.push_back(-1.0);

        if (symbol_id >= m_prev_indicative_sizes.size())
          m_prev_indicative_sizes.push_back(-1);

        size_t t_prepend_posn = ts.find("T:");
        if (t_prepend_posn != string::npos)
          ts = ts.substr(t_prepend_posn+2);

        ts_date = get_date_from_time_field(ts);
        if (ts_date != "" && ts_date != m_date) {
          continue;
        } else {
          size_t space_posn = ts.find(" ");
          if (space_posn != string::npos)
            ts = ts.substr(space_posn+1);

          ts = remove_time_formatting(ts);
          if (ts == "000000000")
            continue;
        }

        char time_buffer[9];
        strncpy(time_buffer, ts.c_str(), 9);
        int rec_time = fixed_length_atoi(time_buffer, 9);

        if (msg_type == "trade_report") {
          qbdf_trade trade_rec;

          float last = atof(fields[7].c_str());
          trade_rec.hdr.format = QBDFMsgFmt::Trade;
          trade_rec.time = fixed_length_atoi(time_buffer+4,5);
          trade_rec.exch = exch_char;
          trade_rec.symbolId = symbol_id;
          trade_rec.price = last;
          trade_rec.size = strtod(fields[8].c_str(), NULL);

          string cond_str = "";
	  // for trade_report msgs, index 6 is supposed to be a seq no
	  if (fields[6] == "" && m_date >= "20100208" && (exch_char == 'C' || exch_char == 'H' || exch_char == 'S')) {
	    // blanks on or after 20100208 seem to indicate duplicates
	    // but before that date, all are blank
	    cond_str = "ZZZZ";
	  }
          else if (fields[6] == "" && (exch_char == 'A' || exch_char == 'B' || exch_char == 'F' || exch_char == 'P')) {
            cond_str = "ZZZZ";
          } else if (fields[10] == "") {
            if (m_date >= "20100715") {
              if (fields[35].length() > 4 && fields[35][0] == '2' && fields[35][1] == '4' && fields[35][3] == ':') {
                cond_str = fields[35].substr(4);
              } else {
                cond_str = fields[35];
              }
            } else if (fields[9] == "O" &&
                (exch_char == 'A' || exch_char == 'B' || exch_char == 'F' || exch_char == 'P')) {
              cond_str = fields[9];
            } else{
              cond_str = fields[10];
            }
          } else {
            cond_str = fields[10];
          }

          memset(trade_rec.cond, ' ', 4);
          int count = 0;
          for( ; count < 4 && cond_str[count] != '\0'; ++count) {
            trade_rec.cond[count] = cond_str[count];
          }

          if (trade_rec.cond[3] == 'X') {
            m_logger->log_printf(Log::INFO, "%s: Marking trade for %s as BUST", where, symbol.c_str());
            trade_rec.correct = 1;
          } else {
            trade_rec.correct = 0;
          }

          trade_rec.stop = ' ';
          trade_rec.side = ' ';
          trade_rec.source = ' ';

          float prev_price = m_prev_prices[symbol_id];
          if (prev_price > 0) {
            float diff_factor = last / prev_price;
            if (diff_factor > 2.0 or diff_factor < 0.5) {
              m_logger->log_printf(Log::INFO, "%s: Bad price for symbol=[%s] price=[%.4f] last_price=[%.4f]",
                     where, symbol.c_str(), last, prev_price);
              continue;
            }
          }
          if (exch_char == 'L' || exch_char == 'I') {
	    if (fields[10] == "AT") {
	      m_prev_prices[symbol_id] = last;
	    }
          } else {
	    m_prev_prices[symbol_id] = last;
	  }

          if (write_binary_record(symbol, symbol_id, rec_time, &trade_rec, sizeof(qbdf_trade))) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write trade rec for symbol=[%s] time=[%d]",
                   where, symbol.c_str(), rec_time);
            return 1;
          }
        } else if (msg_type == "on_msg") {
          qbdf_imbalance imbalance_rec;

          int prev_size = m_prev_indicative_sizes[symbol_id];
          float prev_price = m_prev_indicative_prices[symbol_id];

          int new_size = 0;
          float new_price = 0;

          if (is_nomura) {
            if (fields[6] != "I")
              continue;

            string price_str = fields[4];
            string size_str = fields[5];

            new_price = atof(price_str.c_str());
            if (new_price <= 0.0) {
              new_price = prev_price;
            }

            new_size = atoi(size_str.c_str());
            if (new_size <= 0.0) {
              new_size = prev_size;
            }

          } else {
            if (fields[7] != "I")
              continue;

            string price_str = fields[5];
            string size_str = fields[6];

            if (m_date >= "20120402") {
              if (price_str == "") {
                new_price = prev_price;
              } else {
                new_price = atof(price_str.c_str());
              }

              if (size_str == "") {
                new_size = prev_size;
              } else {
                new_size = atoi(size_str.c_str());
              }
            } else {
              new_price = atof(price_str.c_str());
              if (new_price <= 0.0) {
                new_price = prev_price;
              }

              new_size = atoi(size_str.c_str());
              if (new_size <= 0.0) {
                new_size = prev_size;
              }
            }
          }

          m_prev_indicative_prices[symbol_id] = new_price;
          m_prev_indicative_sizes[symbol_id] = new_size;

          imbalance_rec.hdr.format = QBDFMsgFmt::Imbalance;
          imbalance_rec.time = fixed_length_atoi(time_buffer+4,5);
          imbalance_rec.exch = exch_char;
          imbalance_rec.symbolId = symbol_id;
          imbalance_rec.near_price = new_price;
          imbalance_rec.paired_size = new_size;

          imbalance_rec.ref_price = -1.0;
          imbalance_rec.far_price = -1.0;
          imbalance_rec.imbalance_size = -1;
          imbalance_rec.imbalance_side = ' ';
          imbalance_rec.cross_type = ' ';

          if (write_binary_record(symbol, symbol_id, rec_time, &imbalance_rec, sizeof(qbdf_imbalance))) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write auction rec for symbol=[%s] time=[%d]",
                   where, symbol.c_str(), rec_time);
            return 1;
          }
        } else if (is_nomura && msg_type == "quote_update" && fields.size() >= 23 && fields[22] == "I") {
          qbdf_imbalance imbalance_rec;
          
	  int prev_size = m_prev_indicative_sizes[symbol_id];
          float prev_price = m_prev_indicative_prices[symbol_id];

          int new_size = 0;
          float new_price = 0;

	  string ask_price_str = fields[6];
          string ask_size_str = fields[7];

	  string bid_price_str = fields[8];
          string bid_size_str = fields[9];

          new_price = atof(ask_price_str.c_str());
          if (new_price <= 0.0) {
            new_price = prev_price;
          }

          if (bid_price_str != ask_price_str) {
	    m_logger->log_printf(Log::WARN, "%s: Bid price [%s] does not equal ask price [%s] in imbalance message for symbol [%s]",
				   where, bid_price_str.c_str(), ask_price_str.c_str(), symbol.c_str());
	  }

	  new_size = min(atoi(ask_size_str.c_str()), atoi(bid_size_str.c_str()));
          if (new_size <= 0.0) {
            new_size = prev_size;
          }

          m_prev_indicative_prices[symbol_id] = new_price;
          m_prev_indicative_sizes[symbol_id] = new_size;

	  int imb_size = atoi(bid_size_str.c_str()) - atoi(ask_size_str.c_str());

	  char imb_side = ' ';
          if (imb_size < 0) {
	      imb_side = 'S';
	  } else if (imb_size > 0) {
	      imb_side = 'B';
	  } 

          imbalance_rec.hdr.format = QBDFMsgFmt::Imbalance;
          imbalance_rec.time = fixed_length_atoi(time_buffer+4,5);
          imbalance_rec.exch = exch_char;
          imbalance_rec.symbolId = symbol_id;
          imbalance_rec.near_price = new_price;
          imbalance_rec.paired_size = new_size;

          imbalance_rec.ref_price = new_price;
          imbalance_rec.far_price = new_price;
          imbalance_rec.imbalance_size = abs(imb_size);
          //imbalance_rec.imbalance_size = -1;
          imbalance_rec.imbalance_side = imb_side;
          //imbalance_rec.imbalance_side = ' ';
          imbalance_rec.cross_type = ' ';

          if (write_binary_record(symbol, symbol_id, rec_time, &imbalance_rec, sizeof(qbdf_imbalance))) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write auction rec for symbol=[%s] time=[%d]",
                   where, symbol.c_str(), rec_time);
            return 1;
          }

        } else if (msg_type == "quote_update") {
          qbdf_quote_small small_quote_rec;
          qbdf_quote_large large_quote_rec;

          void *p_quote_rec = NULL;
          int rec_size = 0;

          qbdf_quote_price *p_quote_price_rec = NULL;

          float bid_px = atof(fields[8].c_str());
          float bid_sz = atoi(fields[9].c_str());
          float ask_px = atof(fields[6].c_str());
          float ask_sz = atoi(fields[7].c_str());
          if (bid_px == 0 || ask_px == 0)
            continue;

          if (bid_sz > USHRT_MAX || ask_sz > USHRT_MAX) {
            // This needs to be a Large record using ints rather than ushorts
            large_quote_rec.hdr.format = QBDFMsgFmt::QuoteLarge;
            large_quote_rec.sizes.bidSize = bid_sz;
            large_quote_rec.sizes.askSize = ask_sz;
            p_quote_price_rec = &large_quote_rec.qr;
            p_quote_rec = &large_quote_rec;
            rec_size = sizeof(qbdf_quote_large);
          } else {
            // Small record
            small_quote_rec.hdr.format = QBDFMsgFmt::QuoteSmall;
            small_quote_rec.sizes.bidSize = bid_sz;
            small_quote_rec.sizes.askSize = ask_sz;
            p_quote_price_rec = &small_quote_rec.qr;
            p_quote_rec = &small_quote_rec;
            rec_size = sizeof(qbdf_quote_small);
          }

          p_quote_price_rec->time = fixed_length_atoi(time_buffer+4,5);
          p_quote_price_rec->exch = exch_char;
          p_quote_price_rec->symbolId = symbol_id;
          p_quote_price_rec->bidPrice = bid_px;
          p_quote_price_rec->askPrice = ask_px;
          p_quote_price_rec->cond = fields[10].c_str()[0];
          p_quote_price_rec->source = ' ';
          p_quote_price_rec->cancelCorrect = ' ';

          // Write out this record to the correct 1 minute binary file, it will only be in symbol/time order
          // rather than straight time order but that will be fixed once we sort the files.
          if (write_binary_record(symbol, symbol_id, rec_time, p_quote_rec, rec_size)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write quote rec for symbol=[%s] time=[%d]",
                   where, symbol.c_str(), rec_time);
            return 1;
          }
        } else if (msg_type == "security_status") {
          //continue; // BMC 20100702 disable this for now...
          qbdf_status status_rec;

          status_rec.hdr.format = QBDFMsgFmt::Status;
          status_rec.time = fixed_length_atoi(time_buffer+4,5);
          status_rec.symbolId = symbol_id;
          status_rec.exch = exch_char;
          if (fields[1] == "Closed") {
            status_rec.status_type = MarketStatus::Closed;
          } else if (fields[1] == "Auction" || fields[1] == "Crossing") {
            status_rec.status_type = MarketStatus::Auction;
          } else if (fields[1] == "Normal") {
            status_rec.status_type = MarketStatus::Open;
          } else if (fields[1] == "Halted") {
            status_rec.status_type = MarketStatus::Halted;
          } else if (fields[1] == "Deleted") {
            status_rec.status_type = MarketStatus::Suspended;
          } else {
            status_rec.status_type = MarketStatus::Unknown;
          }
          sprintf(status_rec.status_detail, "%-8s", fields[5].c_str());

          if (write_binary_record(symbol, symbol_id, rec_time, &status_rec, sizeof(qbdf_status))) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write status rec for symbol=[%s] time=[%d]",
                   where, symbol.c_str(), rec_time);
            return 1;
          }
        } else if (msg_type == "level_delta") {
          continue;

          char pl_side = fields[5].c_str()[0] == 'B' ? 'B' : 'S';
          char reason_code = fields[12].c_str()[0];
          //char status = fields[13].c_str()[0];

          uint32_t pl_shares = atoi(fields[6].c_str());
          float pl_price = atof(fields[7].c_str());
          uint16_t num_orders = atoi(fields[16].c_str());
          gen_qbdf_level_rec_time(this, symbol_id, rec_time, exch_char, pl_side, pl_shares, pl_price,
                                  num_orders, ' ', ' ', reason_code, 0, 0, 0, 1);
        } else if (msg_type == "level_entry_delta") {

          uint64_t participant_id = strtoll(fields[5].c_str(), 0, 10);

          string price_str = fields[10];
          // convert price to 6-character string with no decimal
          size_t decimal_posn = price_str.find_first_of('.');
          if (decimal_posn != string::npos) {
            price_str = price_str.erase(decimal_posn, 1);
          }
          uint64_t iprice = strtoll(price_str.c_str(), 0, 10);
          uint64_t order_id = m_wombat_util.get_unique_order_id(symbol_id, participant_id, iprice);

          char order_side = fields[7].c_str()[0] == 'B' ? 'B' : 'S';
          int size = atoi(fields[14].c_str());
          float price = atof(fields[10].c_str());
          //char reason_code = fields[15].c_str()[0];
          char order_action = fields[11].c_str()[0];

          switch (order_action) {
          case 'A':
            gen_qbdf_order_add_rec_time(this, symbol_id, order_id, rec_time, exch_char, order_side, size, price);
            break;
          case 'D':
            gen_qbdf_order_modify_rec_time(this, symbol_id, order_id, rec_time, exch_char, order_side, 0, price);
            break;
          case 'U':
            gen_qbdf_order_modify_rec_time(this, symbol_id, order_id, rec_time, exch_char, order_side, atoi(fields[8].c_str()), price);
            break;
          default:
            m_logger->log_printf(Log::ERROR, "%s: Unknown order_action: %c", where, order_action);
          }
        }
      }
    }
    return 0;
  }

  WombatCSV_QBDF_Builder::WombatCSV_QBDF_Builder(Application* app, const string& date)
  : QBDF_Builder(app, date)
  {
  }

}
