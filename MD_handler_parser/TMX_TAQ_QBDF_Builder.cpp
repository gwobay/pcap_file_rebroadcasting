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
#include <Data/QBDF_Util.h>
#include <Data/TMX_TAQ_QBDF_Builder.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  // ----------------------------------------------------------------
  // load in raw trades file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int TMX_TAQ_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "TMX_TAQ_QBDF_Builder::process_raw_file";
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
    
    string last_symbol = "";
    uint32_t line_num = 0;
    char line[1024];
    while (fgets(line, 1024, f_in)) {
      ++line_num;

      char record_type;
      string symbol;
      uint64_t qbdf_time;
      uint32_t seq_no;

      if (m_date >= "20151116") {
	tmx_taq_common* p_common = (tmx_taq_common*)line;
	record_type = p_common->record_type;
	symbol = string(p_common->symbol, 8);
	qbdf_time = fixed_length_strtol(p_common->time, 12);
	seq_no = fixed_length_strtol(p_common->seq_no, 9);
      } else {
	tmx_taq_common_old* p_common = (tmx_taq_common_old*)line;
	record_type = p_common->record_type;
	symbol = string(p_common->symbol, 8);
	qbdf_time = fixed_length_strtol(p_common->time, 8) * 10000UL;
	seq_no = fixed_length_strtol(p_common->seq_no, 9);
      }

      if (record_type == 'D')
	continue;
      
      size_t whitespace_posn = symbol.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol = symbol.substr(0, whitespace_posn+1);

      if (symbol != last_symbol) {
	last_symbol = symbol;
	m_logger->log_printf(Log::INFO, "%s: Processing symbol %s", where, symbol.c_str());
	m_logger->sync_flush();
      }

      uint16_t symbol_id = process_taq_symbol(symbol);
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create id for sym [%s]", where, symbol.c_str());
        fclose(f_in);
        return 1;
      }


      if (record_type == 'Q') {
	MDPrice bid_price;
	MDPrice ask_price;
	uint32_t bid_size;
	uint32_t ask_size;
	char halted;

	if (m_date >= "20151116") {
	  tmx_taq_quote* p_quote = (tmx_taq_quote*)line;
	  bid_price = MDPrice::from_iprice32(fixed_length_strtol(p_quote->bid_price, 7) * 10);
	  ask_price = MDPrice::from_iprice32(fixed_length_strtol(p_quote->ask_price, 7) * 10);
	  bid_size = fixed_length_strtol(p_quote->bid_size, 3);
	  ask_size = fixed_length_strtol(p_quote->ask_size, 3);
	  halted = p_quote->halted;
	} else {
	  tmx_taq_quote_old* p_quote = (tmx_taq_quote_old*)line;
	  bid_price = MDPrice::from_iprice32(fixed_length_strtol(p_quote->bid_price, 7) * 10);
	  ask_price = MDPrice::from_iprice32(fixed_length_strtol(p_quote->ask_price, 7) * 10);
	  bid_size = fixed_length_strtol(p_quote->bid_size, 3);
	  ask_size = fixed_length_strtol(p_quote->ask_size, 3);
	  halted = p_quote->halted;
	}

        // Handle board lots
        if (bid_price.fprice() >= 1.0) {
          bid_size *= 100;
          ask_size *= 100;
        } else if (bid_price.fprice() >= 0.1) {
          bid_size *= 500;
          ask_size *= 500;
        } else {
          bid_size *= 1000;
          ask_size *= 1000;
        }

	gen_qbdf_quote_micros_seq_qbdf_time(this, symbol_id, qbdf_time, 0, 't', halted,
					    bid_price, ask_price, bid_size, ask_size, seq_no);


        if (bid_price == ask_price) {

	  MDPrice near_price = bid_price;
	  MDPrice ref_price = bid_price;
	  MDPrice far_price = bid_price;
	  
	  char imbalance_side;
	  uint32_t paired_size, imbalance_size;

	  if (bid_size > ask_size) {
	    paired_size = ask_size;
	    imbalance_size = bid_size - ask_size;
	    imbalance_side = 'B';
	  } else if (ask_size > bid_size) {
	    paired_size = bid_size;
	    imbalance_size = ask_size - bid_size;
	    imbalance_side = 'S';
	  } else {
	    paired_size = bid_size;
	    imbalance_size = 0;
	    imbalance_side = 'N';
	  }
	      
	  gen_qbdf_imbalance_micros_seq_qbdf_time(this, symbol_id, qbdf_time, 0, 't', ref_price,
						  near_price, far_price, paired_size, imbalance_size,
						  imbalance_side, 'O' , seq_no);
        }

      } else if (record_type == 'T') {
	MDPrice price;
	uint32_t size;
	char cond_1;
	char cancellation;
	char cancelled;
	char cash;
	char non_net;
	char odd_lot;
	char special_terms;
	char correction;

	if (m_date >= "20151116") {
	  tmx_taq_trade* p_trade = (tmx_taq_trade*)line;
	  price = MDPrice::from_iprice32(fixed_length_strtol(p_trade->price, 7) * 10);
	  size = fixed_length_strtol(p_trade->shares, 9);
	  cond_1 = p_trade->trade_session;
	  cancellation = p_trade->cancellation;
	  cancelled = p_trade->cancelled;
	  cash = p_trade->cash;
	  non_net = p_trade->non_net;
	  odd_lot = p_trade->odd_lot;
	  special_terms = p_trade->special_terms;
	  correction = p_trade->correction;
	} else {
	  tmx_taq_trade_old* p_trade = (tmx_taq_trade_old*)line;
	  price = MDPrice::from_iprice32(fixed_length_strtol(p_trade->price, 7) * 10);
	  size = fixed_length_strtol(p_trade->shares, 9);
	  cond_1 = p_trade->trade_session;
	  cancellation = p_trade->cancellation;
	  cancelled = p_trade->cancelled;
	  cash = p_trade->cash;
	  non_net = p_trade->non_net;
	  odd_lot = p_trade->odd_lot;
	  special_terms = p_trade->special_terms;
	  correction = p_trade->correction;
	}

	char cond_2 = '0';
	if (cancellation == '1') {
	  if (cancelled == '1') {
	    cond_2 = '3';
	  } else {
	    cond_2 = '1';
	  }
	} else if (cancelled == '1') {
	  cond_2 = '2';
	}

	char cond_3 = '0';
	if (cash == '1') {
	  if (non_net == '1') {
	    cond_2 = '3';
	  } else {
	    cond_2 = '1';
	  }
	} else if (non_net == '1') {
	  cond_2 = '2';
	}
	
	char cond_4 = '0';
	if (odd_lot == '1') {
	  if (special_terms == '1') {
	    cond_2 = '3';
	  } else {
	    cond_2 = '1';
	  }
	} else if (special_terms == '1') {
	  cond_2 = '2';
	}

	stringstream ss;
	string cond;
	ss << cond_1 << cond_2 << cond_3 << cond_4;
	ss >> cond;

	gen_qbdf_trade_micros_seq_qbdf_time(this, symbol_id, qbdf_time, 0, 't', ' ', correction,
					    cond, size, price, seq_no);

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
