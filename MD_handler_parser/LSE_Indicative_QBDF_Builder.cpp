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
#include <Data/LSE_Indicative_QBDF_Builder.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  // ----------------------------------------------------------------
  // load in raw file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int LSE_Indicative_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* lse_time_format = "%Y%m%d %H:%M:%S";
    const char* tz_str = "TZ=Europe/London";
    putenv(const_cast<char*>(tz_str));
    tzset();

    glob_t glob_buf;
    glob_buf.gl_offs = 0;
    int glob_ret = glob(file_name.c_str(), GLOB_DOOFFS, NULL, &glob_buf);
    if (glob_ret || glob_buf.gl_pathc == 0) {
#ifdef CW_DEBUG
      cout << "ERROR: LSE_Indicative_QBDF_Builder::process_raw_file: Could not resolve any files using glob from " << file_name << endl;
#endif
      return 1;
    }

    for (size_t i = 0; i < glob_buf.gl_pathc; ++i) {
      string glob_file_name = (glob_buf.gl_pathv[i]);
#ifdef CW_DEBUG
      cout << "LSE_Indicative_QBDF_Builder::process_raw_file: " << glob_file_name << endl;
#endif

      boost::iostreams::filtering_istream in;
      if(has_gzip_extension(glob_file_name)) {
        in.push(boost::iostreams::gzip_decompressor());
      }
      in.push(boost::iostreams::file_source(glob_file_name));

      vector<string> fields;
      string line;

      int lineNo = 0;
      while (getline(in, line)) {
        lineNo++;

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
        if (fields.size() < 14)
          continue;

        string isin = fields[0];
        ushort taqId = process_taq_symbol(isin);
        if (taqId == 0) {
#ifdef CW_DEBUG
          printf("LSE_Indicative_QBDF_Builder::process_raw_file: ERROR unable to create or find id for isin [%s]\n", isin.c_str());
#endif
          return 1;
        }

        if (taqId >= m_prev_indicative_prices.size())
          m_prev_indicative_prices.push_back(-1.0);

        if (taqId >= m_prev_indicative_sizes.size())
          m_prev_indicative_sizes.push_back(-1);

        string date = fields[11];
        string ts = fields[12];
        size_t decimal_pos = ts.find('.');
        if (decimal_pos != string::npos)
          ts = ts.substr(0, decimal_pos);

        string dt = date + " " + ts;
        struct tm local_tm;
        Time_Utils::clear_tm(local_tm);
        strptime(dt.c_str(), lse_time_format, &local_tm);
        time_t local_time = mktime(&local_tm);
        
        struct tm *gmt_tm;
        gmt_tm = gmtime(&local_time);

        char time_buffer[11];
        strftime(time_buffer, 11, "%H%M%S000", gmt_tm);
        int recTime = fixed_length_atoi(time_buffer,9);

        qbdf_imbalance indicativeRecord;
        qbdf_imbalance *pRec = &indicativeRecord;

        if (fields[10] != "I")
          continue;

        float price = atof(fields[8].c_str());
        int size = atoi(fields[9].c_str());

        if (price <= 0.0 && size <= 0) {
          continue;
        } else if (size <= 0.0) {
          int prev_size = m_prev_indicative_sizes[taqId];
          if (prev_size > 0) {
            size = prev_size;
          }
          m_prev_indicative_prices[taqId] = price;
        } else if (price <= 0.0) {
          float prev_price = m_prev_indicative_prices[taqId];
          if (prev_price > 0) {
            price = prev_price;
          }
          m_prev_indicative_sizes[taqId] = size;
        } else {
          m_prev_indicative_prices[taqId] = price;
          m_prev_indicative_sizes[taqId] = size;
        }

        pRec->hdr.format = QBDFMsgFmt::Imbalance;
        pRec->time = fixed_length_atoi(time_buffer+4,5);
        pRec->exch = 'L';
        pRec->symbolId = taqId;
        pRec->near_price = price;
        pRec->paired_size = size;

        pRec->ref_price = -1.0;
        pRec->far_price = -1.0;
        pRec->imbalance_size = -1;
        pRec->imbalance_side = ' ';
        pRec->cross_type = ' ';

        if (write_binary_record(isin, taqId, recTime, pRec, sizeof(qbdf_imbalance))) {
#ifdef CW_DEBUG
          printf("LSE_Indicative_QBDF_Builder::process_raw_file: ERROR unable to write out an indicative record for symbol=[%s] id=[%d] trade_time=[%d] line=[%d]\n",
                 isin.c_str(), taqId, recTime, lineNo);
#endif
          return 1;
        }
      }
    }
    return 0;
  }
}
