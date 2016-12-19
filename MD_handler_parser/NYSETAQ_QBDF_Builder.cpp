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
#include <Data/NYSETAQ_QBDF_Builder.h>
#include <Data/QBDF_Util.h>


namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  
  const char pipe = '|';  // 
  const string tradeFileEnd = "END";

  // ----------------------------------------------------------------
  // load in raw trades file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int NYSETAQ_QBDF_Builder::process_trade_file(const string& file_name)
  {
#ifdef CW_DEBUG
    printf("NYSETAQ_QBDF_Builder::process_trade_file: %s\n", file_name.c_str());
#endif
    
    FILE *f = NULL;
    if(has_zip_extension(file_name) || has_gzip_extension(file_name)) {
      string file_cmd("/bin/zcat " + file_name);
      f = popen(file_cmd.c_str(),"r");
    } else {
      f = fopen(file_name.c_str(),"r");
    }
    
    if (f == 0) {
#ifdef CW_DEBUG
      printf("NYSETAQ_QBDF_Builder::process_trade_file: ERROR unable to open trade file %s for reading\n",
             file_name.c_str());
#endif
      return 1;
    }
    
    char line[1024];
    if (!fgets(line,1024,f)) {
#ifdef CW_DEBUG
      printf("NYSETAQ_QBDF_Builder::process_trade_file: ERROR unable to read header line in raw file %s\n",
             file_name.c_str());
#endif
      return 1;
    }
    
#ifdef CW_DEBUG
    bool is_nanos = (m_date >= "20160801") ? true : false;  
#else
    bool is_nanos = (m_date >= "20161003") ? true : false; // actual production time  
#endif
    
    bool is_micros = (m_date >= "20150728") ? true : false;
    nyse_taq_raw_common_rec *p_raw_common = nullptr;  
    nyse_taq_raw_common_micros_rec *p_raw_common_micros = nullptr; 
    nyse_taq_raw_common_nanos_rec *p_raw_common_nanos = nullptr; 
    if (is_nanos) { 
      p_raw_common_nanos = new nyse_taq_raw_common_nanos_rec; 
    } else if (is_micros) { 
      p_raw_common_micros = (nyse_taq_raw_common_micros_rec*)line;
    } else { 
      p_raw_common = (nyse_taq_raw_common_rec*)line; 
    } 
    
    nyse_taq_raw_trade_rec *p_raw_trade = nullptr;
    nyse_taq_2016_raw_trade_rec *p_2016_raw_trade = nullptr;
    
    string last_symbol_str = "";
    string symbol_str = "";
    char exch;
    uint64_t exch_time;
    ushort symbol_id = 0;
    uint32_t rec_count = 0;
    while (fgets(line,1024,f)) {
      if (is_nanos) { //pipe delimited format after Oct. 3, 2016
	istringstream ss(line);
	string data;
	getline(ss, data, pipe); //
	if (data.compare(tradeFileEnd) == 0 ) { // end of trade records
	  string date;
	  getline(ss, date, pipe);
	  uint32_t records;
	  getline(ss, data, pipe);
	  records = atoi(data.c_str());
	  // TO-DO: move the following info into log.
#ifdef CW_DEBUG
	  cout << "Trade date " << date;
#endif
#ifdef CW_DEBUG
	  cout << " Record Count in file : " << records << " , processed record count: " << rec_count << endl;
#endif
	  continue;
	}	
       
	exch_time = fixed_length_strtol(const_cast<char*>(data.c_str()), 15);
	strcpy(p_raw_common_nanos->time, data.c_str());
	
	getline(ss, data, pipe);      //	
	strcpy(p_raw_common_nanos->exch, data.c_str());
	exch = p_raw_common_nanos->exch[0];
	if (exch == 'T')
	  exch = 'Q';
	
	getline(ss, symbol_str, pipe); //symbol_str = string(p_raw_common_nanos->symbol, 16);
	strcpy(p_raw_common_nanos->symbol, symbol_str.c_str());
	
	p_2016_raw_trade = new nyse_taq_2016_raw_trade_rec; // 
	parseNYSETAQTradeRec(ss, p_2016_raw_trade);
      }else if (is_micros) {
	symbol_str = string(p_raw_common_micros->symbol, 16);
	exch_time  = fixed_length_strtol(p_raw_common_micros->time, 12);
	exch = p_raw_common_micros->exch[0];
	if (exch == 'T')
	  exch = 'Q';
	
	p_raw_trade = (nyse_taq_raw_trade_rec*)(line+sizeof(nyse_taq_raw_common_micros_rec));
      } else {
	symbol_str = string(p_raw_common->symbol, 16);
	exch_time  = fixed_length_strtol(p_raw_common->time, 9) * Time_Constants::micros_per_mili;
	exch = p_raw_common->exch[0];
	if (exch == 'T')
	  exch = 'Q';
	
	p_raw_trade = (nyse_taq_raw_trade_rec*)(line+sizeof(nyse_taq_raw_common_rec));
      }
      
      // Process rare case of multiple headers
      if (symbol_str[0] == ' ') {
	continue;
      }
      
      ++rec_count;
      if (symbol_str != last_symbol_str) {
	// Moved onto data for a new symbol, skip off trailing spaces and find an ID for it
	size_t found = symbol_str.find_last_not_of(" \t\f\v\n\r");
        if (found != string::npos)
          symbol_str.erase(found+1);
        else
          symbol_str.clear();
	
        symbol_id = process_taq_symbol(symbol_str);
        if (symbol_id == 0) {
#ifdef CW_DEBUG
          printf("NYSETAQ_QBDF_Builder::process_trade_file: ERROR unable to create or find id for symbol [%s]\n",symbol_str.c_str());
#endif
          fclose(f);
          return 1;
        }
	last_symbol_str = symbol_str;
      }
      
      MDPrice price;
      uint32_t size;
      char correct; 
      string cond = "";
      uint32_t seq_no;
      if (is_nanos) {
	price = MDPrice::from_fprice(atof(p_2016_raw_trade->trade_price));
	size = atoi(p_2016_raw_trade->trade_volume);
	correct = (char)fixed_length_atoi(p_2016_raw_trade->trade_correction_indicator,2);
	cond = string(p_2016_raw_trade->sale_condition, 4);
	seq_no = atoi(p_2016_raw_trade->trade_sequence_number);
	gen_qbdf_trade_nanos_seq_qbdf_time(this, symbol_id, exch_time, 0, exch, ' ', correct, cond, size, price, seq_no);
      } else { 
	price = MDPrice::from_fprice(fixed_length_atof(p_raw_trade->trade_price, 11, 7));
	size = fixed_length_atoi(p_raw_trade->trade_volume, 9);
	correct = (char)fixed_length_atoi(p_raw_trade->trade_correction_indicator,2);
	cond = string(p_raw_trade->sale_condition, 4);
	seq_no = fixed_length_atoi(p_raw_trade->trade_sequence_number, 16);
	gen_qbdf_trade_micros_seq_qbdf_time(this, symbol_id, exch_time, 0, exch, ' ', correct, cond, size, price, seq_no);
      }
      
    }
    if (f) fclose(f);
    return 0;
  }
  
  
  // ----------------------------------------------------------------
  // load in raw quotes file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int NYSETAQ_QBDF_Builder::process_quote_file(const string& file_name)
  {
#ifdef CW_DEBUG
    printf("NYSETAQ_QBDF_Builder::process_quote_file: %s\n", file_name.c_str());
#endif

    FILE *f = NULL;
    if(has_zip_extension(file_name) || has_gzip_extension(file_name)) {
      string file_cmd("/bin/zcat " + file_name);
      f = popen(file_cmd.c_str(),"r");
    } else {
      f = fopen(file_name.c_str(),"r");
    }
    
    if (f == 0) {
#ifdef CW_DEBUG
      printf("NYSETAQ_QBDF_Builder::process_quote_file: ERROR unable to open quote file %s for reading\n",file_name.c_str());
#endif
      return 1;
    }
    
    char line[1024];
    if (!fgets(line,1024,f)) {
#ifdef CW_DEBUG
      printf("NYSETAQ_QBDF_Builder::process_quote_file: ERROR unable to read header line in raw file %s\n",file_name.c_str());
#endif
      fclose(f);
      return 1;
    }
    
#ifdef CW_DEBUG
    bool is_nanos = (m_date >= "20160801") ? true : false;  
#else
    bool is_nanos = (m_date >= "20161003") ? true : false; // actual production time  
#endif
    
    bool is_micros = (m_date >= "20150727") ? true : false;
    nyse_taq_raw_common_rec *p_raw_common = nullptr; // (nyse_taq_raw_common_rec*)line;
    nyse_taq_raw_common_micros_rec *p_raw_common_micros = nullptr; // (nyse_taq_raw_common_micros_rec*)line;
    nyse_taq_raw_common_nanos_rec *p_raw_common_nanos = nullptr; // (nyse_taq_raw_common_micros_rec*)line;
    
    if (is_nanos) { 
      p_raw_common_nanos = new nyse_taq_raw_common_nanos_rec;
    } else if (is_micros) { 
      p_raw_common_micros = (nyse_taq_raw_common_micros_rec*)line;
    } else {
      p_raw_common = (nyse_taq_raw_common_rec*)line;
    }
    
    nyse_taq_raw_quote_rec *p_raw_quote = NULL;
    nyse_taq_2016_raw_quote_rec *p_2016_raw_quote = nullptr;
    
    string last_symbol_str = "";
    string symbol_str = "";
    char exch;
    uint64_t exch_time;
    ushort symbol_id = 0;
    uint32_t rec_count = 0;

    while (fgets(line,1024,f)) {
      if (is_nanos) { 
	istringstream ss(line);
	string data;
	getline(ss, data, pipe); //
	if (data.compare(tradeFileEnd) == 0 ) { // end of trade records
	  string date;
	  getline(ss, date, pipe);
	  uint32_t records;
	  getline(ss, data, pipe);
	  records = atoi(data.c_str());
	  // TO-DO: move the following info into log.
#ifdef CW_DEBUG
	  cout << "Quote date " << date;
#endif
#ifdef CW_DEBUG
	  cout << " Record Count : " << records << " in file " << file_name << " , processed record count: " << rec_count << endl;
#endif
	  continue;
	}	

	exch_time = fixed_length_strtol(const_cast<char*>(data.c_str()), 15);
	strcpy(p_raw_common_nanos->time, data.c_str());
	
	getline(ss, data, pipe);      //	
	strcpy(p_raw_common_nanos->exch, data.c_str());
	exch = p_raw_common_nanos->exch[0];
	if (exch == 'T')
	  exch = 'Q';
	
	getline(ss, symbol_str, pipe);
	strcpy(p_raw_common_nanos->symbol, symbol_str.c_str());

	++rec_count;
	p_2016_raw_quote = new nyse_taq_2016_raw_quote_rec;
	parseNYSETAQQuoteRec(ss, p_2016_raw_quote);

      } else if (is_micros) {
	symbol_str = string(p_raw_common_micros->symbol, 16);
	exch_time = fixed_length_strtol(p_raw_common_micros->time, 12);
	exch = p_raw_common_micros->exch[0];
	if (exch == 'T')
	  exch = 'Q';

	p_raw_quote = (nyse_taq_raw_quote_rec*)(line+sizeof(nyse_taq_raw_common_micros_rec));
      } else {
	symbol_str = string(p_raw_common->symbol, 16);
	exch_time  = fixed_length_strtol(p_raw_common->time, 9) * Time_Constants::micros_per_mili;
	exch = p_raw_common->exch[0];
	if (exch == 'T')
	  exch = 'Q';

	p_raw_quote = (nyse_taq_raw_quote_rec*)(line+sizeof(nyse_taq_raw_common_rec));
      }

      // Process rare case of multiple headers
      if (symbol_str[0] == ' ') {
	continue;
      }

      if (symbol_str != last_symbol_str) {
        // Moved onto data for a new symbol, skip off trailing spaces and find an ID for it
        size_t found = symbol_str.find_last_not_of(" \t\f\v\n\r");
        if (found != string::npos)
          symbol_str.erase(found+1);
        else
          symbol_str.clear();
	
        symbol_id = process_taq_symbol(symbol_str);
        if (symbol_id == 0) {
#ifdef CW_DEBUG
          printf("NYSETAQ_QBDF_Builder::process_quote_file: ERROR unable to create or find id for symbol [%s]\n",symbol_str.c_str());
#endif
          fclose(f);
          return 1;
        }
	last_symbol_str = symbol_str;
      }
      
      MDPrice bid_price;
      uint32_t bid_size;
      MDPrice ask_price;
      uint32_t ask_size;
      char cond;
      uint32_t seq_no;
      
      if (is_nanos) { 
	
	bid_price = MDPrice::from_fprice(atof(p_2016_raw_quote->bid_price));
	bid_size = atoi(p_2016_raw_quote->bid_size) * 100;
	
	ask_price = MDPrice::from_fprice(atof(p_2016_raw_quote->ask_price));
	ask_size = atoi(p_2016_raw_quote->ask_size) * 100;

	cond = p_2016_raw_quote->quote_condition[0];
	seq_no = atoi(p_2016_raw_quote->sequence_number); //may need to use 
	
	gen_qbdf_quote_nanos_seq_qbdf_time(this, symbol_id, exch_time, 0, exch, cond, bid_price, ask_price, bid_size, ask_size, seq_no);
	
      } else {
	bid_price = MDPrice::from_fprice(fixed_length_atof(p_raw_quote->bid_price, 11, 7));
	bid_size = fixed_length_atoi(p_raw_quote->bid_size, 7) * 100;
	
	ask_price = MDPrice::from_fprice(fixed_length_atof(p_raw_quote->ask_price, 11, 7));
	ask_size = fixed_length_atoi(p_raw_quote->ask_size, 7) * 100;

	cond = p_raw_quote->quote_condition[0];
	seq_no = fixed_length_atoi(p_raw_quote->sequence_number, 16);
	
	gen_qbdf_quote_micros_seq_qbdf_time(this, symbol_id, exch_time, 0, exch, cond, bid_price, ask_price, bid_size, ask_size, seq_no);
      }
    }

    if (f) fclose(f);
    return 0;
  }


  void NYSETAQ_QBDF_Builder::parseNYSETAQTradeRec(istringstream& ss, nyse_taq_2016_raw_trade_rec *p_2016_raw_trade) {
    string data;
    getline(ss, data, pipe); 
    strcpy(p_2016_raw_trade->sale_condition, data.c_str());
    getline(ss, data, pipe); 
    strcpy(p_2016_raw_trade->trade_volume, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_trade->trade_price, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_trade->trade_stop_indicator, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_trade->trade_correction_indicator, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_trade->trade_sequence_number, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_trade->trade_id, data.c_str());	
    getline(ss, data, pipe);
    strcpy(p_2016_raw_trade->source_of_trade, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_trade->trade_reporting_facility, data.c_str());
  }


  void NYSETAQ_QBDF_Builder::parseNYSETAQQuoteRec(istringstream& ss, nyse_taq_2016_raw_quote_rec* p_2016_raw_quote){
    string data;
    getline(ss, data, pipe); 
    strcpy(p_2016_raw_quote->bid_price, data.c_str());
    getline(ss, data, pipe); 
    strcpy(p_2016_raw_quote->bid_size, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->ask_price, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->ask_size, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->quote_condition, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->sequence_number, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->national_bbo_indicator, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->finra_bbo_indicator, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->finra_adf_mpid_indicator, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->quote_cancel_correction, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->source_of_quote, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->retail_interest_indicator, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->short_sale_restriction_indicator, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->luld_bbo_indicator, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->sip_generated_message_identifier, data.c_str());
    getline(ss, data, pipe);
    strcpy(p_2016_raw_quote->national_bbo_luld_indicator, data.c_str());
  }
}
