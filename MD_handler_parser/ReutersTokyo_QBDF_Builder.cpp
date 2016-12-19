#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Util/File_Utils.h>
#include <Data/ReutersTokyo_QBDF_Builder.h>
#include <Data/QBDF_Util.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  // ----------------------------------------------------------------
  // load in quotes file and generate qbdf mesages
  // ----------------------------------------------------------------
  int ReutersTokyo_QBDF_Builder::process_trade_file(const string& file_name)
  {
    const char* where = "ReutersTokyo_QBDF_Builder::process_trade_file";

    glob_t glob_buf;
    glob_buf.gl_offs = 0;
    int glob_ret = glob(file_name.c_str(), GLOB_DOOFFS, NULL, &glob_buf);
    if (glob_ret || glob_buf.gl_pathc == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Could not find files using glob on %s", where, file_name.c_str());
      return 1;
    }

    int counter = 0;
    //int sequence_counter = 0;
    //int last_rec_time = 0;
    for (size_t i = 0; i < glob_buf.gl_pathc; ++i) {
      string glob_file_name = (glob_buf.gl_pathv[i]);
      m_logger->log_printf(Log::INFO, "%s: Reading %s", where, glob_file_name.c_str());
      boost::iostreams::filtering_istream in;
      if(has_gzip_extension(glob_file_name)) {
        in.push(boost::iostreams::gzip_decompressor());
      }
      in.push(boost::iostreams::file_source(glob_file_name));

      vector<string> fields;
      string line;

      while (getline(in, line)) {
        counter++;
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

        if (fields.size() < 10)
          continue;

        string ts = fields[1];
        string symbol_exch = fields[2];

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

        ts = remove_time_formatting(ts);
        if (ts == "000000000")
          continue;

        char time_buffer[9];
        strncpy(time_buffer, ts.c_str(), 9);
        int rec_time = fixed_length_atoi(time_buffer, 9);

        /*
        if (last_rec_time != rec_time) {
          sequence_counter = 0;
          last_rec_time = rec_time;
        }*/

        TradeUpdate tu;
        tu.m_id = symbol_id;
        tu.m_price = atof(fields[3].c_str());
        tu.m_size = atoi(fields[4].c_str());
        memset(tu.m_trade_qualifier, ' ', sizeof(tu.m_trade_qualifier));
        tu.m_trade_qualifier[0] = fields[9].c_str()[0];

        gen_qbdf_trade_rec_time(this, &tu, rec_time, exch_char, ' ', '0', ' ', '0');
      }
    }

    return 0;
  }

  // ----------------------------------------------------------------
  // load in quotes file and generate qbdf mesages
  // ----------------------------------------------------------------
  int ReutersTokyo_QBDF_Builder::process_quote_file(const string& file_name)
  {
    const char* where = "ReutersTokyo_QBDF_Builder::process_raw_file";

    m_bid_levels.clear();
    m_ask_levels.clear();

    m_bid_levels.push_back(vector<level_pair>());
    m_ask_levels.push_back(vector<level_pair>());

    glob_t glob_buf;
    glob_buf.gl_offs = 0;
    int glob_ret = glob(file_name.c_str(), GLOB_DOOFFS, NULL, &glob_buf);
    if (glob_ret || glob_buf.gl_pathc == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Could not find files using glob on %s", where, file_name.c_str());
      return 1;
    }

    for (size_t i = 0; i < glob_buf.gl_pathc; ++i) {
      string glob_file_name = (glob_buf.gl_pathv[i]);
      m_logger->log_printf(Log::INFO, "%s: Reading %s", where, glob_file_name.c_str());

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

        if (fields.size() < 27)
          continue;

        string ts = fields[1];
        string symbol_exch = fields[2];

        size_t delimPos = symbol_exch.find('.');
        if (delimPos == string::npos)
          continue;

        string symbol = symbol_exch.substr(0, delimPos);
        string exch = symbol_exch.substr(delimPos+1);
        map<string,char>::iterator exchange_iter = m_vendor_exch_to_qbdf_exch.find(exch);
        if (exchange_iter == m_vendor_exch_to_qbdf_exch.end())
          continue;

        ushort symbol_id = process_taq_symbol(symbol_exch);
        if (symbol_id == 0) {
          m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]",
                               where, symbol.c_str());
          return 1;
        }

        // Add empty level map to vector of symbol ids
        while (symbol_id >= m_bid_levels.size()) {
          vector<level_pair> sym_bid_levels(5, level_pair());
          m_bid_levels.push_back(sym_bid_levels);
        }

        // Add empty level map to vector of symbol ids
        while (symbol_id >= m_ask_levels.size()) {
          vector<level_pair> sym_ask_levels(5, level_pair());
          m_ask_levels.push_back(sym_ask_levels);
        }

        ts = remove_time_formatting(ts);
        if (ts == "000000000")
          continue;

        char time_buffer[9];
        strncpy(time_buffer, ts.c_str(), 9);
        int rec_time = fixed_length_atoi(time_buffer, 9);

        char atone = ' ';
        if (fields[23] != "") { atone = fields[23].c_str()[0]; }
        if (atone == 'P')
          continue;

        char btone = ' ';
        if (fields[24] != "") { btone = fields[24].c_str()[0]; }
        if (btone == 'P')
          continue;

        float bid1_px = atof(fields[3].c_str());
        float bid2_px = atof(fields[4].c_str());
        float bid3_px = atof(fields[5].c_str());
        float bid4_px = atof(fields[6].c_str());
        float bid5_px = atof(fields[7].c_str());

        int bid1_sz = atoi(fields[13].c_str());
        int bid2_sz = atoi(fields[14].c_str());
        int bid3_sz = atoi(fields[15].c_str());
        int bid4_sz = atoi(fields[16].c_str());
        int bid5_sz = atoi(fields[17].c_str());

        float ask1_px = atof(fields[8].c_str());
        float ask2_px = atof(fields[9].c_str());
        float ask3_px = atof(fields[10].c_str());
        float ask4_px = atof(fields[11].c_str());
        float ask5_px = atof(fields[12].c_str());

        int ask1_sz = atoi(fields[18].c_str());
        int ask2_sz = atoi(fields[19].c_str());
        int ask3_sz = atoi(fields[20].c_str());
        int ask4_sz = atoi(fields[21].c_str());
        int ask5_sz = atoi(fields[22].c_str());


        uint8_t last_in_group = 0;
        vector<level_pair> bid_levels_to_add;
        vector<level_pair> ask_levels_to_add;
        vector<level_pair> sym_bid_levels = m_bid_levels[symbol_id];
        vector<level_pair> sym_ask_levels = m_ask_levels[symbol_id];


        float prev_bid1_px = sym_bid_levels[0].price;
        int prev_bid1_sz = sym_bid_levels[0].size;
        if (bid1_px > 0) {
          if (bid1_px != prev_bid1_px) {
            if (prev_bid1_px > 0) { bid_levels_to_add.push_back(level_pair(prev_bid1_px, 0)); }
            bid_levels_to_add.push_back(level_pair(bid1_px, bid1_sz));
          } else if (bid1_sz != prev_bid1_sz) {
            bid_levels_to_add.push_back(level_pair(bid1_px, bid1_sz));
          }
          sym_bid_levels[0] = level_pair(bid1_px, bid1_sz);
        }

        float prev_bid2_px = sym_bid_levels[1].price;
        int prev_bid2_sz = sym_bid_levels[1].size;
        if (bid2_px > 0) {
          if (bid2_px != prev_bid2_px) {
            if (prev_bid2_px > 0) { bid_levels_to_add.push_back(level_pair(prev_bid2_px, 0)); }
            bid_levels_to_add.push_back(level_pair(bid2_px, bid2_sz));
          } else if (bid2_sz != prev_bid2_sz) {
            bid_levels_to_add.push_back(level_pair(bid2_px, bid2_sz));
          }
          sym_bid_levels[1] = level_pair(bid2_px, bid2_sz);
        }

        float prev_bid3_px = sym_bid_levels[2].price;
        int prev_bid3_sz = sym_bid_levels[2].size;
        if (bid3_px > 0) {
          if (bid3_px != prev_bid3_px) {
            if (prev_bid3_px > 0) { bid_levels_to_add.push_back(level_pair(prev_bid3_px, 0)); }
            bid_levels_to_add.push_back(level_pair(bid3_px, bid3_sz));
          } else if (bid3_sz != prev_bid3_sz) {
            bid_levels_to_add.push_back(level_pair(bid3_px, bid3_sz));
          }
          sym_bid_levels[2] = level_pair(bid3_px, bid3_sz);
        }

        float prev_bid4_px = sym_bid_levels[3].price;
        int prev_bid4_sz = sym_bid_levels[3].size;
        if (bid4_px > 0) {
          if (bid4_px != prev_bid4_px) {
            if (prev_bid4_px > 0) { bid_levels_to_add.push_back(level_pair(prev_bid4_px, 0)); }
            bid_levels_to_add.push_back(level_pair(bid4_px, bid4_sz));
          } else if (bid4_sz != prev_bid4_sz) {
            bid_levels_to_add.push_back(level_pair(bid4_px, bid4_sz));
          }
          sym_bid_levels[3] = level_pair(bid4_px, bid4_sz);
        }

        float prev_bid5_px = sym_bid_levels[4].price;
        int prev_bid5_sz = sym_bid_levels[4].size;
        if (bid5_px > 0) {
          if (bid5_px != prev_bid5_px) {
            if (prev_bid5_px > 0) { bid_levels_to_add.push_back(level_pair(prev_bid5_px, 0)); }
            bid_levels_to_add.push_back(level_pair(bid5_px, bid5_sz));
          } else if (bid5_sz != prev_bid5_sz) {
            bid_levels_to_add.push_back(level_pair(bid5_px, bid5_sz));
          }
          sym_bid_levels[4] = level_pair(bid5_px, bid5_sz);
        }

        float prev_ask1_px = sym_ask_levels[0].price;
        int prev_ask1_sz = sym_ask_levels[0].size;
        if (ask1_px) {
          if (ask1_px != prev_ask1_px) {
            if (prev_ask1_px > 0) { ask_levels_to_add.push_back(level_pair(prev_ask1_px, 0)); }
            ask_levels_to_add.push_back(level_pair(ask1_px, ask1_sz));
          } else if (ask1_sz != prev_ask1_sz) {
            ask_levels_to_add.push_back(level_pair(ask1_px, ask1_sz));
          }
          sym_ask_levels[0] = level_pair(ask1_px, ask1_sz);
        }

        float prev_ask2_px = sym_ask_levels[1].price;
        int prev_ask2_sz = sym_ask_levels[1].size;
        if (ask2_px > 0) {
          if (ask2_px != prev_ask2_px) {
            if (prev_ask2_px > 0) { ask_levels_to_add.push_back(level_pair(prev_ask2_px, 0)); }
            ask_levels_to_add.push_back(level_pair(ask2_px, ask2_sz));
          } else if (ask2_sz != prev_ask2_sz) {
            ask_levels_to_add.push_back(level_pair(ask2_px, ask2_sz));
          }
          sym_ask_levels[1] = level_pair(ask2_px, ask2_sz);
        }

        float prev_ask3_px = sym_ask_levels[2].price;
        int prev_ask3_sz = sym_ask_levels[2].size;
        if (ask3_px > 0) {
          if (ask3_px != prev_ask3_px) {
            if (prev_ask3_px > 0) { ask_levels_to_add.push_back(level_pair(prev_ask3_px, 0)); }
            ask_levels_to_add.push_back(level_pair(ask3_px, ask3_sz));
          } else if (ask3_sz != prev_ask3_sz) {
            ask_levels_to_add.push_back(level_pair(ask3_px, ask3_sz));
          }
          sym_ask_levels[2] = level_pair(ask3_px, ask3_sz);
        }

        float prev_ask4_px = sym_ask_levels[3].price;
        int prev_ask4_sz = sym_ask_levels[3].size;
        if (ask4_px > 0) {
          if (ask4_px != prev_ask4_px) {
            if (prev_ask4_px > 0) { ask_levels_to_add.push_back(level_pair(prev_ask4_px, 0)); }
            ask_levels_to_add.push_back(level_pair(ask4_px, ask4_sz));
          } else if (ask4_sz != prev_ask4_sz) {
            ask_levels_to_add.push_back(level_pair(ask4_px, ask4_sz));
          }
          sym_ask_levels[3] = level_pair(ask4_px, ask4_sz);
        }

        float prev_ask5_px = sym_ask_levels[4].price;
        int prev_ask5_sz = sym_ask_levels[4].size;
        if (ask5_px > 0) {
          if (ask5_px != prev_ask5_px) {
            if (prev_ask5_px > 0) { ask_levels_to_add.push_back(level_pair(prev_ask5_px, 0)); }
            ask_levels_to_add.push_back(level_pair(ask5_px, ask5_sz));
          } else if (ask5_sz != prev_ask5_sz) {
            ask_levels_to_add.push_back(level_pair(ask5_px, ask5_sz));
          }
          sym_ask_levels[4] = level_pair(ask5_px, ask5_sz);
        }

        for (size_t i = 0; i < bid_levels_to_add.size(); i++) {
          if (ask_levels_to_add.size() == 0 && i+1 == bid_levels_to_add.size())
            last_in_group = 1;

          level_pair lp = bid_levels_to_add[i];
          gen_qbdf_level_rec_time(this, symbol_id, rec_time, 'T', 'B', lp.size, lp.price, 0,
                                  atone, btone, ' ', 0, 0, 0, last_in_group);
        }

        for (size_t i = 0; i < ask_levels_to_add.size(); i++) {
          if (i+1 == ask_levels_to_add.size())
            last_in_group = 1;

          level_pair lp = ask_levels_to_add[i];
          gen_qbdf_level_rec_time(this, symbol_id, rec_time, 'T', 'S', lp.size, lp.price, 0,
                                  atone, btone, ' ', 0, 0, 0, last_in_group);
        }

        m_bid_levels[symbol_id] = sym_bid_levels;
        m_ask_levels[symbol_id] = sym_ask_levels;
      }
    }
    m_bid_levels.clear();
    m_ask_levels.clear();
    return 0;
  }

  ReutersTokyo_QBDF_Builder::ReutersTokyo_QBDF_Builder(Application* app, const string& date)
  : QBDF_Builder(app, date)
  {
  }

}
