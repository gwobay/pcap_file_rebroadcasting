#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>

#include <Util/File_Utils.h>
#include <Data/QBDF_Util.h>
#include <Data/ReutersRDF_QBDF_Builder.h>
#include <Logger/Logger_Manager.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  
  bool ReutersRDF_QBDF_Builder::get_exch_time_delta(uint64_t recv_time, uint64_t exch_time,
                                                   int& exch_time_delta, bool& delta_in_millis) {
    int64_t temp_delta = exch_time - recv_time;
    if (temp_delta >= 86400000000L) { // micros per day
      // something is wrong here, exch time delta shouldn't be a more than 24 hours
      return false;
    } else if (labs(temp_delta) >= numeric_limits<int>::max()) {
      if (m_qbdf_version.compare("1.0") == 0) {
	m_logger->log_printf(Log::WARN, "%s: Exch delta too big, set qbdf_version > 1.0 for delta_in_millis!",
			     "ReutersRDF_QBDF_Builder::get_exch_time_delta");
      }
      exch_time_delta = (int)(temp_delta / 1000L);
      delta_in_millis = true;
    } else {
      exch_time_delta = (int)temp_delta;
      delta_in_millis = false;
    }

    return true;
  }


  // ----------------------------------------------------------------
  // load in raw file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int ReutersRDF_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "ReutersRDF_QBDF_Builder::process_raw_file";

    Time::set_simulation_mode(true);
    Time::set_current_time(Time(m_date, "%Y%m%d"));
    m_midnight = Time::today();

    //time_t current_time;
    //time(&current_time);
    //struct tm* timeinfo = localtime(&current_time);
    //long gmt_offset = timeinfo->tm_gmtoff;
    
    glob_t glob_buf;
    glob_buf.gl_offs = 0;
    int glob_ret = glob(file_name.c_str(), GLOB_DOOFFS, NULL, &glob_buf);
    if (glob_ret || glob_buf.gl_pathc == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Could not find files using glob on %s\n", where, file_name.c_str());
      m_logger->sync_flush();
      return 1;
    }

    string line;
    char exch_time_str[32];
    
    m_reuters_rdf_state_lookup.resize(1000);
    for (size_t i = 0; i <  m_reuters_rdf_state_lookup.size(); ++i) {
      m_reuters_rdf_state_lookup[i] = reuters_rdf_state_rp(new reuters_rdf_state());
    }

    for (size_t i = 0; i < glob_buf.gl_pathc; ++i) {
      string glob_file_name = (glob_buf.gl_pathv[i]);
      m_logger->log_printf(Log::INFO, "%s: Reading %s", where, glob_file_name.c_str());

      boost::iostreams::filtering_istream in;
      if(has_gzip_extension(glob_file_name)) {
        in.push(boost::iostreams::gzip_decompressor());
      }
      in.push(boost::iostreams::file_source(glob_file_name));

      int line_num = 0;
      while (getline(in, line)) {
        line_num++;

        bool is_trade = false;
        bool is_quote = false;
        size_t start_posn, end_posn;

        start_posn = 0;
        end_posn = line.find_first_of(' ', start_posn);
        string time_epoch_sec = line.substr(start_posn, end_posn-start_posn);

        start_posn = end_posn+1;
        end_posn = line.find_first_of(' ', start_posn);
        string time_millis = line.substr(start_posn, end_posn-start_posn);

        start_posn = end_posn+1;
        end_posn = line.find_first_of(' ', start_posn);
        string subject = line.substr(start_posn, end_posn-start_posn);

        string symbol = subject.substr(subject.find_first_of('.')+5);
        uint16_t id = process_taq_symbol(symbol);
        if (id == 0) {
	  m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n", where, symbol.c_str());
	  m_logger->sync_flush();
          return 1;
        }

	while (id >= m_reuters_rdf_state_lookup.size()) {
	  m_reuters_rdf_state_lookup.push_back(reuters_rdf_state_rp(new reuters_rdf_state()));
	}

	reuters_rdf_state_rp sd = m_reuters_rdf_state_lookup.at(id);
        start_posn = end_posn+1;
        populate_data(line, subject, sd, is_trade, is_quote);

        if (!is_trade and !is_quote)
          continue;


        uint64_t recv_time_sec_part = strtoll(time_epoch_sec.c_str(), 0, 10);
        uint64_t recv_time_msec_part = strtoll(time_millis.c_str(), 0, 10);
        uint64_t recv_time_epoch_micros = (recv_time_sec_part * 1000000UL) + (recv_time_msec_part * 1000UL);
	uint64_t recv_time_micros = recv_time_epoch_micros - (m_midnight.ticks() / Time_Constants::ticks_per_micro);

        if (is_trade) {
          if (sd->trade_time[0] == '\0' && sd->exch_time[0] == '\0') {
            continue;
          } else if (sd->exch_time[0] == '\0' ||
              strncmp(sd->trade_time, sd->exch_time, 8) >= 0) {
            sprintf(exch_time_str, "%s %s", sd->date, sd->trade_time);
          } else {
            sprintf(exch_time_str, "%s %s", sd->date, sd->exch_time);
          }

          struct tm exch_time_tm;
          if (strptime(exch_time_str, "%d%b%Y %H:%M:%S", &exch_time_tm)) {
          } else {
            if (strptime(exch_time_str, "%Y%m%d %H:%M:%S", &exch_time_tm)) {
            } else {
              m_logger->log_printf(Log::WARN, "%s: Time %s does not match expected format", where, exch_time_str);
              continue;
            }
          }

          // mktime converts local tm struct to GMT epoch
          // but in this case tm struct is already in GMT
          // so we need to add offset back after conversion
          time_t exch_time_epoch = mktime(&exch_time_tm);
	  uint64_t exch_time_micros = ((uint64_t)exch_time_epoch * Time_Constants::micros_per_second) - (m_midnight.ticks() / Time_Constants::ticks_per_micro);

	  int exch_time_delta;
	  bool delta_in_millis;
	  if (!get_exch_time_delta(recv_time_micros, exch_time_micros, exch_time_delta, delta_in_millis)) {
            m_logger->log_printf(Log::ERROR, "%s: Exch time delta is more than 24 hours: [recv_time=%lu] [exch_time=%lu]",
                                 where, recv_time_micros, exch_time_micros);
            continue;
          }

          char cond_buf[4];
          snprintf(cond_buf, 4, "%-2s%-2s", sd->trade_cond_code1, sd->trade_cond_code2);
          string cond(cond_buf);

          MDPrice md_price = MDPrice::from_fprice(strtod(sd->trade_price, 0));
          uint32_t size = strtol(sd->trade_size, 0, 10);

          if (!gen_qbdf_trade_micros(this, id, recv_time_micros, exch_time_delta, //record_time_delta,
				     sd->rdn_exch, ' ', '0', cond, size, md_price, delta_in_millis)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write trade for id=[%d] time=[%lu]",
                                 where, id, recv_time_micros);
            return 1;
          }
        }

        if (is_quote) {
          if (sd->quote_time[0] == '\0' && sd->exch_time[0] == '\0') {
            continue;
          } else if (sd->exch_time[0] == '\0' ||
              strncmp(sd->quote_time, sd->exch_time, 8) >= 0) {
            sprintf(exch_time_str, "%s %s", sd->date, sd->quote_time);
          } else {
            sprintf(exch_time_str, "%s %s", sd->date, sd->exch_time);
          }

          struct tm exch_time_tm;
          if (strptime(exch_time_str, "%d%b%Y %H:%M:%S", &exch_time_tm)) {
          } else {
            if (strptime(exch_time_str, "%Y%m%d %H:%M:%S", &exch_time_tm)) {
            } else {
              m_logger->log_printf(Log::WARN, "%s: Time %s does not match expected format", where, exch_time_str);
              continue;
            }
          }

          // mktime converts local tm struct to GMT epoch
          // but in this case tm struct is already in GMT
          // so we need to add offset back after conversion
          time_t exch_time_epoch = mktime(&exch_time_tm);
	  uint64_t exch_time_micros = ((uint64_t)exch_time_epoch * Time_Constants::micros_per_second) - (m_midnight.ticks() / Time_Constants::ticks_per_micro);

	  int exch_time_delta;
	  bool delta_in_millis;
	  if (!get_exch_time_delta(recv_time_micros, exch_time_micros, exch_time_delta, delta_in_millis)) {
            m_logger->log_printf(Log::ERROR, "%s: Exch time delta is more than 24 hours: [recv_time=%lu] [exch_time=%lu]",
                                 where, recv_time_micros, exch_time_micros);
            continue;
          }

          MDPrice md_bid_price = MDPrice::from_fprice(strtod(sd->bid_price, 0));
          uint32_t bid_size = strtol(sd->bid_size, 0, 10);

          MDPrice md_ask_price = MDPrice::from_fprice(strtod(sd->ask_price, 0));
          uint32_t ask_size = strtol(sd->ask_size, 0, 10);

          if (!gen_qbdf_quote_micros(this, id, recv_time_micros, exch_time_delta, //exch_time, record_time_delta,
				     sd->rdn_exch, sd->quote_cond_code,
				     md_bid_price, md_ask_price, bid_size, ask_size, delta_in_millis)) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write quote for id=[%d] time=[%lu]",
                                 where, id, exch_time_micros);
            return 1;
          }
        }
      }
    }
    return 0;
  }


  bool ReutersRDF_QBDF_Builder::populate_data(const string &msg, const string &subject,
                                              reuters_rdf_state_rp sd, bool &is_trade, bool &is_quote) {

    const char* where = "ReutersRDF_QBDF_Builder::populate_data";

    if (sd->date[0] == '\0') {
      strncpy(sd->date, m_date.c_str(), sizeof(sd->date));
    }

    memset(sd->trade_cond_code1, ' ', 2);
    memset(sd->trade_cond_code2, ' ', 2);
    sd->quote_cond_code = ' ';

    size_t start_posn;
    string value;

    if ((start_posn = msg.find("TRDPRC_1=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->trade_price, value.c_str(), sizeof(sd->trade_price));
      is_trade = true;
    }
    if ((start_posn = msg.find("TRDVOL_1=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->trade_size, value.c_str(), sizeof(sd->trade_size));
      is_trade = true;
    }
    if ((start_posn = msg.find("SALTIM=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->trade_time, value.c_str(), sizeof(sd->trade_time));
      is_trade = true;
    }
    if ((start_posn = msg.find("EXCHTIM=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->exch_time, value.c_str(), sizeof(sd->exch_time));
      is_trade = true;
    }
    if ((start_posn = msg.find("QUOTIM=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->quote_time, value.c_str(), sizeof(sd->quote_time));
      is_quote = true;
    }
    if ((start_posn = msg.find("TRADE_DATE=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->date, value.c_str(), sizeof(sd->date));
      is_trade = true;
    }
    if ((start_posn = msg.find("BID=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->bid_price, value.c_str(), sizeof(sd->bid_price));
      is_quote = true;
    }
    if ((start_posn = msg.find("BIDSIZE=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->bid_size, value.c_str(), sizeof(sd->bid_size));
      is_quote = true;
    }
    if ((start_posn = msg.find("ASK=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->ask_price, value.c_str(), sizeof(sd->ask_price));
      is_quote = true;
    }
    if ((start_posn = msg.find("ASKSIZE=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->ask_size, value.c_str(), sizeof(sd->ask_size));
      is_quote = true;
    }
    if ((start_posn = msg.find("CURRENCY=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->currency, value.c_str(), sizeof(sd->currency));
    }
    if ((start_posn = msg.find("RDN_EXCHID=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      if (m_vendor_exch_to_qbdf_exch.find(value) != m_vendor_exch_to_qbdf_exch.end()) {
        sd->rdn_exch = m_vendor_exch_to_qbdf_exch[value];
      } else {
        sd->rdn_exch = ' ';
#ifdef CW_DEBUG
        printf("%s: Unrecognized RDN EXCHID (%s)\n", where, value.c_str());
#endif
      }
    }
    if ((start_posn = msg.find("CONDCODE_1=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->trade_cond_code1, value.c_str(), sizeof(sd->trade_cond_code1));
      is_trade = true;
    }
    if ((start_posn = msg.find("CONDCODE_2=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->trade_cond_code2, value.c_str(), sizeof(sd->trade_cond_code2));
      is_trade = true;
    }
    if ((start_posn = msg.find("PRC_QL_CD=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      sd->quote_cond_code = (char)atoi(value.c_str());
      is_quote = true;
    }
    if ((start_posn = msg.find("ALIAS=")) != string::npos) {
      value = extract_val_string(msg, start_posn);
      remove_char_from_string(value, '"');
      remove_char_from_string(value, ' ');
      strncpy(sd->alias, value.c_str(), sizeof(sd->alias));
      is_trade = true;
    }
    return true;
  }


  string ReutersRDF_QBDF_Builder::extract_val_string(const string &msg, size_t start_posn) {
    string valueStr;

    size_t nextDelimPos;
    size_t delimPos = msg.find("=", start_posn);
    size_t nextDelim1Pos = msg.find('=', delimPos+1);
    size_t nextDelim2Pos = msg.find('<', delimPos+1);
    if (nextDelim1Pos < nextDelim2Pos)
      nextDelimPos = nextDelim1Pos;
    else
      nextDelimPos = nextDelim2Pos;

    if (nextDelimPos != string::npos) {
      size_t valueEndPos = msg.find_last_of(' ', nextDelimPos-1);
      valueStr = msg.substr(delimPos+1, valueEndPos-delimPos);
    } else {
      valueStr = msg.substr(delimPos+1);
    }
    return valueStr;
  }

  void ReutersRDF_QBDF_Builder::remove_char_from_string(string &str, char removeChar) {
    size_t removePos;
    while ((removePos = str.find(removeChar)) != string::npos) {
      str.erase(removePos, 1);
    }
  }

}
