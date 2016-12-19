#include <fstream>
#include <iostream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <glob.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>

#include <Util/File_Utils.h>
#include <Data/TSX_T1_QBDF_Builder.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  // ----------------------------------------------------------------
  // load in raw trades file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int TSX_T1_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "TSX_T1_QBDF_Builder::process_raw_file";
    //int open_time = 93000;

    m_logger->log_printf(Log::INFO, "%s: Reading file: [%s]", where, file_name.c_str());

    FILE *f_in = NULL;
    if(has_gzip_extension(file_name)) {
      string file_cmd("/bin/zcat " + file_name);
      f_in = popen(file_cmd.c_str(), "r");
    } else {
      f_in = fopen(file_name.c_str(), "r");
    }

    if (f_in == 0) {
      if (!has_gzip_extension(file_name)) {
        string file_cmd("/bin/zcat " + file_name + ".gz");
        f_in = popen(file_cmd.c_str(), "r");
      }
      if (f_in == NULL) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
        return 1;
      }
    }

    tsx_t1_common* p_common = NULL;
    tsx_t1_trade* p_trade = NULL;
    tsx_t1_quote* p_quote = NULL;

    uint32_t line_num = 0;
    char line[1024];
    while (fgets(line, 1024, f_in)) {
      ++line_num;

      p_common = (tsx_t1_common*)line;
      p_trade = (tsx_t1_trade*)line;
      p_quote = (tsx_t1_quote*)line;

      string symbol = string(p_common->symbol, 8);
      size_t whitespace_posn = symbol.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol = symbol.substr(0, whitespace_posn+1);

      uint16_t symbol_id = process_taq_symbol(symbol);
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create id for sym [%s]", where, symbol.c_str());
        fclose(f_in);
        return 1;
      }

      int rec_time = fixed_length_atoi(p_common->time, 6) * 1000;

      if (p_common->msg_format == 'Q') {
        int ms = fixed_length_atoi(p_common->time+4, 2) * 1000;
        double bid_price = fixed_length_atof(p_quote->bid_price, 7, 4);
        double ask_price = fixed_length_atof(p_quote->ask_price, 7, 4);
        int bid_size = fixed_length_atoi(p_quote->bid_size, 3);
        int ask_size = fixed_length_atoi(p_quote->ask_size, 3);
        // Handle board lots
        if (bid_price > 1.0) {
          bid_size *= 100;
          ask_size *= 100;
        } else if (bid_price > 0.1) {
          bid_size *= 500;
          ask_size *= 500;
        } else {
          bid_size *= 1000;
          ask_size *= 1000;
        }

        if (bid_price == ask_price) {
          qbdf_imbalance imbalance_rec;
          imbalance_rec.hdr.format = QBDFMsgFmt::Imbalance;
          imbalance_rec.time = ms;
          imbalance_rec.symbolId = symbol_id;
          imbalance_rec.ref_price = bid_price;
          imbalance_rec.near_price = bid_price;
          imbalance_rec.far_price = -1.0;
          imbalance_rec.far_price = -1.0;
          if (bid_size > ask_size) {
            imbalance_rec.paired_size = ask_size;
            imbalance_rec.imbalance_size = bid_size - ask_size;
            imbalance_rec.imbalance_side = 'B';
          } else if (ask_size > bid_size) {
            imbalance_rec.paired_size = bid_size;
            imbalance_rec.imbalance_size = ask_size - bid_size;
            imbalance_rec.imbalance_side = 'S';
          } else {
            imbalance_rec.paired_size = bid_size;
            imbalance_rec.imbalance_size = 0;
            imbalance_rec.imbalance_side = 'N';
          }

          imbalance_rec.cross_type = 'O';
          imbalance_rec.exch = 't';

          if (write_binary_record(symbol, symbol_id, rec_time, &imbalance_rec, sizeof(qbdf_imbalance))) {
            m_logger->log_printf(Log::ERROR, "%s: Unable to write imbalance rec for symbol=[%s] line=[%d]",
                                 where, symbol.c_str(), line_num);
            fclose(f_in);
            return 1;
          }
        }

        qbdf_quote_large quote_rec;
        quote_rec.hdr.format = QBDFMsgFmt::QuoteLarge;
        quote_rec.qr.time = ms;
        quote_rec.qr.symbolId = symbol_id;
        quote_rec.qr.exch = 't';
        quote_rec.qr.cond = ' ';
        quote_rec.qr.source = ' ';
        quote_rec.qr.cancelCorrect = ' ';
        quote_rec.qr.bidPrice = bid_price;
        quote_rec.qr.askPrice = ask_price;
        quote_rec.sizes.bidSize = bid_size;
        quote_rec.sizes.askSize = ask_size;

        if (write_binary_record(symbol, symbol_id, rec_time, &quote_rec, sizeof(qbdf_quote_large))) {
          m_logger->log_printf(Log::ERROR, "%s: Unable to write quote rec for symbol=[%s] line=[%d]",
                               where, symbol.c_str(), line_num);
          fclose(f_in);
          return 1;
        }
      } else if (p_common->msg_format == 'T') {

        qbdf_trade trade_rec;
        trade_rec.hdr.format = QBDFMsgFmt::Trade;
        trade_rec.time = fixed_length_atoi(p_common->time+4,2); // only pull ss
        trade_rec.symbolId = symbol_id;
        trade_rec.exch = 't';
        trade_rec.price = fixed_length_atof(p_trade->price, 7, 4);
        trade_rec.size = fixed_length_atoi(p_trade->size, 9);
        trade_rec.stop = ' ';
        trade_rec.side = '0';
        trade_rec.source = ' ';

        if (p_trade->correction == '1') {
          trade_rec.correct = '1';
        } else if (p_trade->cancelled == '1') {
          trade_rec.correct = '2';
        } else {
          trade_rec.correct = '0';
        }

        memset(trade_rec.cond, ' ', 4);
        if (p_trade->open_trade == '1')
          trade_rec.cond[0] = 'O';
        if (p_trade->special_terms == '1')
          trade_rec.cond[1] = 'S';
        if (p_trade->posit_trade == '1')
          trade_rec.cond[2] = 'P';

        if (write_binary_record(symbol, symbol_id, rec_time, &trade_rec, sizeof(qbdf_trade))) {
          m_logger->log_printf(Log::ERROR, "%s: Unable to write trade rec for symbol=[%s] line=[%d]",
                               where, symbol.c_str(), line_num);
          fclose(f_in);
          return 1;
        }
      } else if (p_common->msg_format == 'N') {
        // Not yet implemented (name records)
      } else {
        m_logger->log_printf(Log::ERROR, "%s: Unsupported message found in line: %s", where, line);
        fclose(f_in);
        return 1;
      }
    }
    fclose(f_in);
    return 0;
  }

}
