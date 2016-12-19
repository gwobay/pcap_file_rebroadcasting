#include <Data/Hist_TSE_FLEX_QBDF_Builder.h>
#include <Data/TSE_FLEX_Handler.h>
#include <Util/File_Utils.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  int Hist_TSE_FLEX_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "Hist_TSE_FLEX_QBDF_Builder::process_raw_file";
    size_t buffer_size = 1024 * 1024;

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();


    FILE *f_in = NULL;
    if(has_gzip_extension(file_name) || has_zip_extension(file_name)) {
      string file_cmd("/bin/zcat " + file_name);
      f_in = popen(file_cmd.c_str(), "r");
    } else {
      f_in = fopen(file_name.c_str(), "r");
    }

    if (f_in == NULL) {
      if (!has_gzip_extension(file_name)) {
        string file_cmd("/bin/zcat " + file_name + ".gz");
        f_in = popen(file_cmd.c_str(), "r");
      }
      if (f_in == NULL) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
        return 1;
      }
    }

    boost::shared_ptr<TSE_FLEX_Handler> message_handler(new TSE_FLEX_Handler(m_app));
    message_handler->init(m_source_name, ExchangeID::XTKS, m_params);
    message_handler->set_qbdf_builder(this);
    message_handler->set_ms_latest(m_midnight.total_msec());

    Handler::channel_info_map_t& cim = message_handler->m_channel_info_map;
    for (size_t i = 0; i < 256; ++i) {
      cim.push_back(channel_info("", i));
      cim.back().context = i; //
      cim.back().has_reordering = false;
    }

    size_t parsed_len = 0;
    size_t bytes_to_parse = 0;
    char buffer[buffer_size];

    char token;
    int control_code_count = 0;
    size_t data_start = 0;
    size_t data_len = 0;

    Time::set_current_time(Time(m_date, "%Y%m%d"));
    bool read_more = true;
    while (read_more) {
      size_t unparsed_len = data_len; //bytes_to_parse - parsed_len;
      if (parsed_len > 0) {
        // Move leftover bytes to beginning of buffer, fill remaining buffer
        memmove(buffer, buffer+data_start, unparsed_len);
        bytes_to_parse = fread(buffer+unparsed_len, 1, buffer_size-unparsed_len, f_in);
        if (bytes_to_parse < (buffer_size-unparsed_len))
          read_more = false;

        parsed_len = 0;
        bytes_to_parse += unparsed_len;
      } else {
        bytes_to_parse = fread(buffer, 1, buffer_size, f_in);
        if (bytes_to_parse < buffer_size)
          read_more = false;
      }

      control_code_count = 0;
      data_start = 0;
      data_len = 0;

      //if (!buffer) {
      //  m_logger->log_printf(Log::ERROR, "EMPTY BUFFER");
      //  m_logger->sync_flush();
      //  return 1;
      //}

      while (parsed_len < bytes_to_parse) {
        token = (buffer+parsed_len)[0];
        data_len++;
        if (token == 0x11) {
          control_code_count++;
          if (control_code_count == 1) {
            data_len = 1;
            data_start = parsed_len;
          } else {
            if (!message_handler->parse(0, buffer+data_start, data_len)) {
              m_logger->log_printf(Log::ERROR, "Buffer stats: bytes_to_parse=%zd parsed_len=%zd data_len=%zd, data_start=%zd",
                                   bytes_to_parse, parsed_len, data_len, data_start);
              m_logger->sync_flush();
              return 1;
            }

            control_code_count = 0;
          }
        }

        parsed_len++;
      }
    }
    message_handler->write_morning_closes();
    message_handler->write_afternoon_closes();
    message_handler->write_special_quotes();
    return 0;
  }


}
