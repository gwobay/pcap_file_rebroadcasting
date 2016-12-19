#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>

#include <Util/File_Utils.h>
#include <Data/MDSUB_QBDF_Builder.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  // ----------------------------------------------------------------
  // load in raw file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int MDSUB_QBDF_Builder::process_raw_file(const string& file_name)
  {
#ifdef CW_DEBUG
    cout << "MDSUB_QBDF_Builder::process_raw_file: " << file_name << endl;
#endif

    string mdsub_source = m_params["mdsub_source"].getString();
    if (mdsub_source == "MDSUB_CTA") {
      m_md_source = MDSource::CTA;
    } else if (mdsub_source == "MDSUB_UTP") {
      m_md_source = MDSource::UTP;
    } else if (mdsub_source == "MDSUB_ITCH4") {
      m_md_source = MDSource::ITCH4;
    } else if (mdsub_source == "MDSUB_ArcaBook") {
      m_md_source = MDSource::ArcaBook;
    } else if (mdsub_source == "MDSUB_ArcaTrade") {
      m_md_source = MDSource::ArcaTrade;
    } else if (mdsub_source == "MDSUB_NYSEBest") {
      m_md_source = MDSource::NYSEBest;
    } else if (mdsub_source == "MDSUB_NYSEImbalance") {
      m_md_source = MDSource::NYSEImbalance;
    } else if (mdsub_source == "MDSUB_NYSETrade") {
      m_md_source = MDSource::NYSETrade;
    } else if (mdsub_source == "MDSUB_NYSEOpenBook") {
      m_md_source = MDSource::NYSEOpenBook;
    } else if (mdsub_source == "MDSUB_PITCHMC") {
      m_md_source = MDSource::PITCHMC;
    } else if (mdsub_source == "MDSUB_OMXBX4") {
      m_md_source = MDSource::OMXBX4;
    } else {
      m_md_source = MDSource::Unknown;
    }


    // Add support for gzipped files
    ifstream fileIn(file_name.c_str(), ifstream::in);
    if (!fileIn.is_open()) {
#ifdef CW_DEBUG
      cout << "MDSUB_QBDF_Builder::process_raw_file: ERROR unable to open raw file "
      << file_name << " for reading" << endl;
#endif
      return 1;
    }
    struct tm tm;
    Time_Utils::clear_tm(tm);
    char timebuffer[32];
    char statusBuffer[4];
    vector<string> fields;
    string line;

    int lineNo = 0;
    while (!fileIn.eof()) {
      lineNo++;
      getline(fileIn, line);

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
      if (fields.size() < 12)
        continue;

      char exch;
      size_t delim_pos;
      string symbol, symbol_exch;

      switch (m_md_source.index()) {
      case MDSource::CTA:
      case MDSource::UTP:
      case MDSource::NYSEBest:
        symbol_exch = fields[0];
        delim_pos = symbol_exch.find('.');
        if (delim_pos == string::npos)
          continue;

        symbol = symbol_exch.substr(0, delim_pos);
        exch = symbol_exch.substr(delim_pos+1).c_str()[0];
        if (exch == 'T') exch = 'Q';
        break;
      case MDSource::ITCH4:
        symbol = fields[0];
        exch = 'Q';
        break;
      case MDSource::ArcaBook:
      case MDSource::ArcaTrade:
        symbol = fields[0];
        exch = 'P';
        break;
      case MDSource::NYSEOpenBook:
      case MDSource::NYSEImbalance:
      case MDSource::NYSETrade:
        symbol = fields[0];
        exch = 'N';
        break;
      case MDSource::PITCHMC:
        symbol = fields[0];
        exch = 'Z';
        break;
      case MDSource::OMXBX4:
        symbol = fields[0];
        exch = 'B';
        break;
      default:
#ifdef CW_DEBUG
        printf("MDSUB_QBDF_Builder::processTradeFile: Undefined MDSource\n");
#endif
        fileIn.close();
        return 1;
      }

      ushort taqId = process_taq_symbol(symbol);
      if (taqId == 0) {
#ifdef CW_DEBUG
        printf("MDSUB_QBDF_Builder::processTradeFile: ERROR unable to create or find id for symbol [%s]\n",symbol.c_str());
#endif
        fileIn.close();
        return 1;
      }

      string ts = fields[3];
      const char * time_str = ts.c_str();
      if (time_str[8] != '.') {
#ifdef CW_DEBUG
        printf("MDSUB_QBDF_Builder::processTradeFile: Could not parse timestamp: %s", ts.c_str());
#endif
        continue;
      }

      string millis = ts.substr(9, 3);
      strptime(time_str, "%T", &tm);
      char timebuffer0[32];
      strftime(timebuffer0, 32, "%H%M%S", &tm);
      snprintf(timebuffer, 32, "%s%03d", timebuffer0, atoi(millis.c_str()));
      int recTime = fixed_length_atoi(timebuffer,9);

      string msgType = fields[2];
      if (msgType == "T") {
        //qbdf_trade tradeRecord;
        //qbdf_trade *pRec = &tradeRecord;
        qbdf_trade_seq seqTradeRecord;
        qbdf_trade_seq *pSeq = &seqTradeRecord;
        qbdf_trade *pRec = &pSeq->tr;

        string last = fields[6];
        string size = fields[7];
        string status = fields[8];
        strcpy(statusBuffer, status.c_str());
        string cond_codes = fields[11];

        pSeq->seq = recTime;
        pRec->hdr.format = QBDFMsgFmt::Trade;
        pRec->time = fixed_length_atoi(timebuffer+4,5);
        pRec->exch = exch;
        pRec->symbolId = taqId;
        pRec->price = atof(last.c_str());
        pRec->size = atoi(size.c_str());
        if (status == "")
          pRec->correct = ' ';
        else
          pRec->correct = status.c_str()[0];
        pRec->stop = ' ';
        pRec->source = ' ';
        pRec->side = ' ';
        memset(pRec->cond, ' ', 4);
        strncpy(pRec->cond, cond_codes.c_str(), (int)cond_codes.size());

        if (write_binary_record(symbol, taqId, recTime, pSeq, sizeof(qbdf_trade_seq))) {
          // this should never happen
#ifdef CW_DEBUG
          printf("MDSUB_QBDF_Builder::process_raw_file: ERROR unable to write out a trade record for symbol=[%s] id=[%d] trade_time=[%d] line=[%d]\n",
                 symbol.c_str(), taqId, recTime, lineNo);
#endif
          fileIn.close();
          return 1;
        }
      } else if (msgType == "I") {
        //qbdf_imbalance indicativeRecord;
        //qbdf_imbalance *pRec = &indicativeRecord;
        qbdf_imbalance_seq seqIndicativeRecord;
        qbdf_imbalance_seq *pSeq = &seqIndicativeRecord;
        qbdf_imbalance *pRec = &pSeq->ir;

        float price = atof(fields[7].c_str());
        float near_price = atof(fields[9].c_str());
        float far_price = atof(fields[8].c_str());
        if (price <= 0.0 && near_price <= 0 && far_price <= 0)
          continue;

        char imbalance_side;
        int imb_side_code = atoi(fields[16].c_str());

        switch (imb_side_code) {
        case 1:
          imbalance_side = 'B';
          break;
        case 2:
          imbalance_side = 'S';
          break;
        default:
#ifdef CW_DEBUG
          printf("MDSUB_QBDF_Builder::processTradeFile: Undefined imbalance side [%d]\n", imb_side_code);
#endif
          fileIn.close();
          return 1;
        }

        int imbalance_size = atoi(fields[12].c_str());
        if (imbalance_size == 0) {
          imbalance_side = 'N';
        } else {
          char when_code = fields[4].c_str()[0];
          switch (when_code) {
          case 'L':
          case 'N':
          case 'P':
            imbalance_side = 'N';
            break;
          default:
            break;
          }
        }

        pSeq->seq = recTime;
        pRec->hdr.format = QBDFMsgFmt::Imbalance;
        pRec->time = fixed_length_atoi(timebuffer+4,5);
        pRec->exch = exch;
        pRec->symbolId = taqId;

        pRec->ref_price = price;
        pRec->near_price = near_price;
        pRec->far_price = far_price;
        pRec->paired_size = atoi(fields[13].c_str());
        pRec->imbalance_size = imbalance_size;
        pRec->imbalance_side = imbalance_side;
        string cross_type = fields[6];
        if (cross_type == "")
          pRec->cross_type = ' ';
        else
          pRec->cross_type = cross_type.c_str()[0];

        if (write_binary_record(symbol, taqId, recTime, pSeq, sizeof(qbdf_imbalance_seq))) {
#ifdef CW_DEBUG
          printf("MDSUB_QBDF_Builder::process_raw_file: ERROR unable to write out an indicative auction record for symbol=[%s] id=[%d] trade_time=[%d] line=[%d]\n",
                 symbol.c_str(), taqId, recTime, lineNo);
#endif
          return 1;
        }
      } else if (msgType == "Q") {
        qbdf_quote_seq_small seqSmallQuoteRecord;
        qbdf_quote_seq_small *pSmallSeq = &seqSmallQuoteRecord;
        qbdf_quote_small *pSmall = &pSmallSeq->sqr;

        qbdf_quote_seq_large seqLargeQuoteRecord;
        qbdf_quote_seq_large *pLargeSeq = &seqLargeQuoteRecord;
        qbdf_quote_large *pLarge = &pLargeSeq->lqr;

        void *pData = NULL;
        int recSize = 0;

        qbdf_quote_price *pPrices = NULL;

        float bidPrice = atof(fields[6].c_str());
        float bidSize = atoi(fields[7].c_str()) * 100;
        float askPrice = atof(fields[8].c_str());
        float askSize = atoi(fields[9].c_str()) * 100;

        if (bidSize > USHRT_MAX || askSize > USHRT_MAX) {
          // This needs to be a Large record using ints rather than ushorts
          pLargeSeq->seq = recTime;
          pLarge->hdr.format = QBDFMsgFmt::QuoteLarge;
          pPrices = &pLarge->qr;
          //pLargeSeq->seq = fixed_length_atoi(pRaw->sequence_number,16);
          pLarge->sizes.bidSize = bidSize;
          pLarge->sizes.askSize = askSize;
          pData = pLarge;
          recSize = sizeof(*pLargeSeq);
        } else {
          // Small record
          pSmallSeq->seq = recTime;
          pSmall->hdr.format = QBDFMsgFmt::QuoteSmall;
          pPrices = &pSmall->qr;
          //pSmallSeq->seq = fixed_length_atoi(pRaw->sequence_number,16);
          pSmall->sizes.bidSize = bidSize;
          pSmall->sizes.askSize = askSize;
          pData = pSmall;
          recSize = sizeof(*pSmallSeq);
        }

        pPrices->time = fixed_length_atoi(timebuffer+4,5);
        pPrices->exch = exch;
        pPrices->symbolId = taqId;
        pPrices->bidPrice = bidPrice;
        pPrices->askPrice = askPrice;
        pPrices->cond = fields[13].c_str()[0];
        pPrices->source = ' ';
        pPrices->cancelCorrect = ' ';

        // Write out this record to the correct 1 minute binary file, it will only be in symbol/time order
        // rather than straight time order but that will be fixed once we sort the files.
        if (write_binary_record(symbol, taqId, recTime, pData, recSize)) {
          // this should never happen
#ifdef CW_DEBUG
          printf("MDSUB_QBDF_Builder::processQuoteFile: ERROR unable to write out a quote record for symbol=[%s] id=[%d] trade_time=[%d] line=[%d]\n",
                 symbol.c_str(), taqId, recTime, lineNo);
#endif
          fileIn.close();
          return 1;
        }
      }

    }
    return 0;
  }
}
