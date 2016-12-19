#include <math.h>
#include <Util/File_Utils.h>
#include <Data/PITCH_QBDF_Builder.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  // ----------------------------------------------------------------
  // load in raw trades file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int PITCH_QBDF_Builder::process_raw_file(const string& file_name)
  {
#ifdef CW_DEBUG
    printf("PITCH_QBDF_Builder::process_raw_file: %s\n", file_name.c_str());
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
      printf("PITCH_QBDF_Builder::process_raw_file: ERROR unable to open trade file %s for reading\n",
             file_name.c_str());
#endif
      return 1;
    }

    char exch = ' ';
    if (m_source_name == "BATS_Europe") {
      exch = 'y';
    }

    char line[1024];
    string symbol;
    string order_id_str;
    ushort taqId = 0;
    float price;
    int size;

    int line_num = 1;
    while (fgets(line, 1024, f)) {
      line_num++;

      PITCH_CommonMsg* pCommon = (PITCH_CommonMsg*)line;
      PITCH_CommonOTMsg* pCommonOT = (PITCH_CommonOTMsg*)line;
      PITCH_CommonOTLongMsg* pCommonOTLong = (PITCH_CommonOTLongMsg*)line;

      int millis = fixed_length_atoi(pCommon->timestamp, 8);
      int hh = (millis / 3600000);
      int mm = (millis / 60000) % 60;
      int ss = (millis / 1000) % 60;
      int ms = (millis) % 1000;
      int rec_time = (hh * 10000000) + (mm * 100000) + (ss * 1000) + ms;


      if (pCommon->msg_type == 'E' || pCommon->msg_type == 'e') {
        // Order Executed
        continue;
      } else if (pCommon->msg_type == 'X' || pCommon->msg_type == 'x') {
        // Order Cancel
        continue;
      } else if (pCommon->msg_type == 'B') {
        // Trade Break
        continue;
      }

      if (pCommon->msg_type == 'A' || pCommon->msg_type == 'P') {
        symbol = string(pCommonOT->symbol, 6);
        taqId = clean_and_process_taq_symbol(symbol);
        if (taqId == 0) {
#ifdef CW_DEBUG
          printf("PITCH_QBDF_Builder::process_raw_file: ERROR creating id for [%s]\n", symbol.c_str());
#endif
          fclose(f);
          return 1;
        }

        order_id_str = string(pCommonOT->order_id, 12);
        price = fixed_length_atof(pCommonOT->price, 10, 6);
        size = fixed_length_atoi(pCommonOT->shares, 6);
      } else {
        symbol = string(pCommonOTLong->symbol, 6);
        taqId = clean_and_process_taq_symbol(symbol);
        if (taqId == 0) {
#ifdef CW_DEBUG
          printf("PITCH_QBDF_Builder::process_raw_file: ERROR creating id for [%s]\n", symbol.c_str());
#endif
          fclose(f);
          return 1;
        }

        order_id_str = string(pCommonOTLong->order_id, 12);
        price = fixed_length_atof(pCommonOTLong->price, 19);
        size = fixed_length_atoi(pCommonOTLong->shares, 10);
      }

      if (pCommon->msg_type == 'A' || pCommon->msg_type == 'a') {
/*
        if (size > USHRT_MAX) {
          TaqAddOrderLargeRec order;
          order.hdr.format = QBDFMsgFmt::OrderAddLarge;
#ifdef CW_DEBUG
          cout << order.hdr.format << endl;
#endif
          order.time = (ss * 1000) + ms;
          order.symbol_id = taqId;
          order.exch = exch;
          order.order_id = base36decode(order_id_str);
          order.side = pCommonOT->side;
          order.price = price;
          order.size = size;

          if (write_binary_record(symbol.c_str(), taqId, rec_time, &order, sizeof(TaqAddOrderLargeRec))) {
#ifdef CW_DEBUG
            printf("PITCH_QBDF_Builder::process_raw_file: ERROR unable to write TaqAddOrderLargeRec for symbol=[%s] id=[%d] trade_time=[%d] line=[%d]\n",
                   symbol.c_str(), taqId, rec_time, line_num);
#endif
            fclose(f);
            return 1;
          }
        } else {
          TaqAddOrderSmallRec order;
          order.hdr.format = QBDFMsgFmt::OrderAddSmall;
          order.time = (ss * 1000) + ms;
          order.symbol_id = taqId;
          order.exch = exch;
          order.order_id = base36decode(order_id_str);
          order.side = pCommonOT->side;
          order.price = price;
          order.size = size;

          if (write_binary_record(symbol.c_str(), taqId, rec_time, &order, sizeof(TaqAddOrderLargeRec))) {
#ifdef CW_DEBUG
            printf("PITCH_QBDF_Builder::process_raw_file: ERROR unable to write TaqAddOrderLargeRec for symbol=[%s] id=[%d] trade_time=[%d] line=[%d]\n",
                   symbol.c_str(), taqId, rec_time, line_num);
#endif
            fclose(f);
            return 1;
          }
        }
*/
      } else {
        qbdf_trade trade;
        trade.hdr.format = QBDFMsgFmt::Trade;
        trade.time = (ss * 1000) + ms;
        trade.symbolId = taqId;
        trade.exch = exch;
        trade.price = price;
        trade.size = size;
        trade.stop = ' ';
        trade.correct = ' ';
        trade.source = ' ';
        trade.side = ' ';
        strcpy(trade.cond, "    ");

        if (write_binary_record(symbol.c_str(), taqId, rec_time, &trade, sizeof(qbdf_trade))) {
#ifdef CW_DEBUG
          printf("PITCH_QBDF_Builder::process_raw_file: ERROR unable to write qbdf_trade for symbol=[%s] id=[%d] trade_time=[%d] line=[%d]\n",
                 symbol.c_str(), taqId, rec_time, line_num);
#endif
          fclose(f);
          return 1;
        }
      }
    }

    if (f) fclose(f);
    return 0;
  }


  ullong PITCH_QBDF_Builder::base36decode(const string& base36_str)
  {
    int sum = 0;
    for (int i = 0; i <= (int)base36_str.size(); ++i) {
      int char_value = (int)base36_str[i];
      if (char_value >= 65)
        char_value -= 55;
      else
        char_value -= 48;

      sum += (char_value * pow(36, (base36_str.size() - i - 1)));
    }
    return sum;
  }
}
