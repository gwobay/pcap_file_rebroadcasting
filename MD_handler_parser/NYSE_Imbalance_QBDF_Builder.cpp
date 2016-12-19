#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <arpa/inet.h>

#include <Util/File_Utils.h>
#include <Data/NYSE_Imbalance_QBDF_Builder.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  // ----------------------------------------------------------------
  // load in raw file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int NYSE_Imbalance_QBDF_Builder::process_raw_file(const string& file_name)
  {
#ifdef CW_DEBUG
    printf("NYSETAQ_QBDF_Builder::process_raw_file: %s\n", file_name.c_str());
#endif

    FILE *f = NULL;
    if(has_zip_extension(file_name)) {
      string file_cmd("/bin/zcat " + file_name);
#ifdef CW_DEBUG
      printf("Using file command %s\n", file_cmd.c_str());
#endif
      f = popen(file_cmd.c_str(),"r");
    } else {
      f = fopen(file_name.c_str(),"rb");
    }

    if (f == 0) {
#ifdef CW_DEBUG
      printf("NYSETAQ_QBDF_Builder::process_raw_file: ERROR unable to open trade file %s for reading\n",
             file_name.c_str());
#endif
      return 1;
    }

    NYSE_Imbalance_Rec rawRec;
    NYSE_Imbalance_Rec* pRawRec = &rawRec;
    while (fread(pRawRec, sizeof(NYSE_Imbalance_Rec), 1, f)) {
      uint16_t converted_msg_type = ntohs(pRawRec->msg_type);
      uint32_t converted_ref_price = ntohl(pRawRec->ref_price);
      uint32_t converted_imbalance_size = ntohl(pRawRec->imbalance_size);
      uint32_t converted_paired_size = ntohl(pRawRec->paired_size);
      uint32_t converted_source_time = ntohl(pRawRec->source_time);

      string ticker(pRawRec->symbol);
      ushort taq_id = process_taq_symbol(ticker);
      if (taq_id == 0) {
#ifdef CW_DEBUG
        printf("NYSE_Imbalance_QBDF_Builder::process_raw_file: ERROR could not create/find id for ticker [%s]\n",
               ticker.c_str());
#endif
        return 1;
      }

      int converted_time = millis_to_qbdf_time(converted_source_time);
      int rec_time = converted_time;

      int price_scale_code = 4;
      if (pRawRec->price_scale_code != ' ')
        price_scale_code = (int)pRawRec->price_scale_code;

      qbdf_imbalance binRec;
      qbdf_imbalance *pBinRec = &binRec;
      pBinRec->hdr.format = QBDFMsgFmt::Imbalance;
      pBinRec->time = converted_time % 60000;
      pBinRec->exch = 'N';
      pBinRec->symbolId = taq_id;

      pBinRec->ref_price = 1.0 * converted_ref_price / pow(10, price_scale_code);
      pBinRec->near_price = -1.0;
      pBinRec->far_price = -1.0;
      pBinRec->paired_size = converted_paired_size;
      pBinRec->imbalance_size = converted_imbalance_size;
      pBinRec->imbalance_side = pRawRec->imbalance_side;

      switch (pRawRec->imbalance_side) {
      case 'B':
      case 'S':
        pBinRec->imbalance_side = pRawRec->imbalance_side;
        break;
      case '0':
      case ' ':
      case '\0':
        pBinRec->imbalance_side = 'N';
        break;
      default:
#ifdef CW_DEBUG
        printf("NYSE_Imbalance_QBDF_Builder::process_raw_file: Unknown imbalance Side [%c]\n",
               pRawRec->imbalance_side);
#endif
        break;
      }

      switch (converted_msg_type) {
      case 240:
        pBinRec->cross_type = 'O';
        break;
      case 241:
        pBinRec->cross_type = 'C';
        break;
      default:
#ifdef CW_DEBUG
        printf("NYSE_Imbalance_QBDF_Builder::process_raw_file: Unknown imbalance MsgType [%d]\n",
               converted_msg_type);
#endif
        break;
      }

      if (write_binary_record(ticker, taq_id, rec_time, pBinRec, sizeof(qbdf_imbalance))) {
#ifdef CW_DEBUG
        printf("NYSE_Imbalance_QBDF_Builder::process_raw_file: ERROR unable to write out imbalance rec for ticker=[%s] id=[%d] trade_time=[%d]\n",
               ticker.c_str(), taq_id, rec_time);
#endif
        return 1;
      }
    }
    return 0;
  }
}
