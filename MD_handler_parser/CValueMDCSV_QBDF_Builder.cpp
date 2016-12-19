#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Util/File_Utils.h>
#include <Data/CValueMDCSV_QBDF_Builder.h>
#include <Data/QBDF_Util.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  int CValueMDCSV_QBDF_Builder::process_trade_file(const string& file_name)
  {
    const char* where = "CValueMDCSV_QBDF_Builder::process_trade_file";

    m_logger->log_printf(Log::INFO, "%s: Reading trades from %s\n", where, file_name.c_str());
    boost::iostreams::filtering_istream in;
    if(has_gzip_extension(file_name)) {
      in.push(boost::iostreams::gzip_decompressor());
    }
    in.push(boost::iostreams::file_source(file_name));

    string line;
    vector<string> fields;

    if (!getline(in, line)) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read header in %s\n", where, file_name.c_str());
      return 1;
    }

    int systime_offset = 0;
    if (line.find("system_time") != string::npos)
      systime_offset = 1;

    int last_hhmm = 0;
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

      if ((int)fields.size() < 10+systime_offset)
        continue;

      string ts = fields[0];
      string symbol = fields[2+systime_offset];
      string exch = fields[3+systime_offset];

      map<string,int>::iterator exchange_iter = m_hf_exch_name_to_qbdf_exch.find(exch);
      if (exchange_iter == m_hf_exch_name_to_qbdf_exch.end())
        continue;

      char exch_char = (char)exchange_iter->second;
      ushort symbol_id = process_taq_symbol(symbol);
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]",
                             where, symbol.c_str());
        return 1;
      }

      ts = remove_time_formatting(ts);
      if (ts == "000000000")
        continue;

      char time_buffer[9];
      strncpy(time_buffer, ts.c_str(), 9);
      int rec_time = fixed_length_atoi(time_buffer, 9);
      if ((rec_time / 100000) !=  last_hhmm) {
        last_hhmm = (rec_time / 100000);
        m_logger->log_printf(Log::INFO, "%s: Processing trades at %d", where, last_hhmm);
      }

      TradeUpdate trade_update;
      trade_update.m_id = symbol_id;
      trade_update.m_price = atof(fields[4+systime_offset].c_str());
      trade_update.m_size = strtod(fields[5+systime_offset].c_str(), NULL);
      sprintf(trade_update.m_trade_qualifier, "%-4s", fields[7+systime_offset].c_str());

      char source = ' ';
      char corr = 0;
      char stop = ' ';
      char side = fields[6+systime_offset].c_str()[0];
      trade_update.set_side(side);

      gen_qbdf_trade_rec_time(this, &trade_update, rec_time, exch_char, source, corr, stop, side);
    }
    return 0;
  }


  int CValueMDCSV_QBDF_Builder::process_quote_file(const string& file_name)
  {
    const char* where = "CValueMDCSV_QBDF_Builder::process_quote_file";

    m_logger->log_printf(Log::INFO, "%s: Reading quotes from %s\n", where, file_name.c_str());
    boost::iostreams::filtering_istream in;
    if(has_gzip_extension(file_name)) {
      in.push(boost::iostreams::gzip_decompressor());
    }
    in.push(boost::iostreams::file_source(file_name));

    string line;
    vector<string> fields;

    if (!getline(in, line)) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read header in %s\n", where, file_name.c_str());
      return 1;
    }

    int systime_offset = 0;
    if (line.find("system_time") != string::npos)
      systime_offset = 1;

    int last_hhmm(0);
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
      if ((int)fields.size() < 10+systime_offset)
        continue;

      string ts = fields[0];
      string symbol = fields[2+systime_offset];
      string exch = fields[3+systime_offset];

      map<string,int>::iterator exchange_iter = m_hf_exch_name_to_qbdf_exch.find(exch);
      if (exchange_iter == m_hf_exch_name_to_qbdf_exch.end())
        continue;

      char exch_char = (char)exchange_iter->second;
      ushort symbol_id = process_taq_symbol(symbol);
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]",
                             where, symbol.c_str());
        return 1;
      }

      ts = remove_time_formatting(ts);
      if (ts == "000000000")
        continue;

      char time_buffer[9];
      strncpy(time_buffer, ts.c_str(), 9);
      int rec_time = fixed_length_atoi(time_buffer, 9);
      if ((rec_time / 100000) !=  last_hhmm) {
        last_hhmm = (rec_time / 100000);
        m_logger->log_printf(Log::INFO, "%s: Processing quotes at %d", where, last_hhmm);
      }

      QuoteUpdate quote_update;
      quote_update.m_id = symbol_id;
      quote_update.m_bid = atof(fields[4+systime_offset].c_str());
      quote_update.m_ask = atof(fields[5+systime_offset].c_str());
      quote_update.m_bid_size = strtod(fields[6+systime_offset].c_str(), NULL);
      quote_update.m_ask_size = strtod(fields[7+systime_offset].c_str(), NULL);
      quote_update.m_cond = fields[8+systime_offset].c_str()[0];
      if (quote_update.m_cond == ' ')
        quote_update.m_cond = 'R';

      gen_qbdf_quote_rec_time(this, &quote_update, rec_time, exch_char, ' ', 0);
    }
    return 0;
  }


  CValueMDCSV_QBDF_Builder::CValueMDCSV_QBDF_Builder(Application* app, const string& date)
  : QBDF_Builder(app, date)
  {
  }

}
