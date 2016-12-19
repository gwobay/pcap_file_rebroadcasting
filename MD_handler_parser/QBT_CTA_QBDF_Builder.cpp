#include <math.h>

#include <ASIO/IO_Service.h>
#include <Logger/Logger_Manager.h>
#include <Data/MoldUDP64.h>
#include <Data/QBT_CTA_QBDF_Builder.h>
#include <Data/CQS_CTS_Handler.h>
#include <MD/MDManager.h>
#include <Util/File_Utils.h>
#include <Util/Network_Utils.h>
#include <Util/Parameter.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  // ----------------------------------------------------------------
  // load in raw trades file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int QBT_CTA_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_CTA_QBDF_Builder::process_raw_file";
    size_t buffer_size = 1024 * 1024;

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    FILE *f_in = NULL;
    if(has_gzip_extension(file_name)) {
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

    Time::set_simulation_mode(true);
    Time::set_current_time(Time(m_date, "%Y%m%d"));
    m_message_handler = new CQS_CTS_Handler(m_app);
    m_message_handler->init(m_source_name, m_params);
    m_message_handler->set_qbdf_builder(this);

    populate_ip_context_map();

    size_t parsed_len = 0;
    size_t bytes_to_parse = 0;
    char buffer[buffer_size];

    bool read_more = true;
    while (read_more) {
      size_t unparsed_len = bytes_to_parse - parsed_len;
      if (parsed_len > 0) {
        // Move leftover bytes to beginning of buffer, fill remaining buffer
        memmove(buffer, buffer+parsed_len, unparsed_len);
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

      int failed_count = 0;
      while ((parsed_len + sizeof(record_header)) < bytes_to_parse) {
        record_header* p_head = (record_header*)(buffer+parsed_len);
        if (strncmp("qRec", p_head->title, 4)) {
          m_logger->log_printf(Log::ERROR, "%s: Mangled record title at byte %d out of %d",
                               where, (int)parsed_len, (int)bytes_to_parse);
          size_t i = parsed_len;
          for ( ; i < bytes_to_parse-sizeof(record_header); ++i) {
            p_head = (record_header*)(buffer+i);
            if (strncmp("qRec", p_head->title, 4) == 0) {
              m_logger->log_printf(Log::INFO, "%s: Found next good record title at byte %d", where, (int)i);
              break;
            }
          }
          parsed_len = i;
          if (strncmp("qRec", p_head->title, 4)) {
            m_logger->log_printf(Log::INFO, "%s: Could not find 'qRec', need to read more bytes", where);
            break;
          }
          failed_count++;
        }

        uint16_t packet_size = p_head->len;
        if ((parsed_len + sizeof(record_header) + packet_size) > bytes_to_parse)
          break;

        Time::set_current_time(Time(p_head->ticks));

        uint64_t context = p_head->context;
        parsed_len += sizeof(record_header);
        size_t handler_bytes_read = m_message_handler->parse(context, buffer+parsed_len, packet_size);

        if (handler_bytes_read != packet_size) {
          m_logger->log_printf(Log::ERROR, "%s: handler_bytes_read=%zd packet_size=%u parsed_len=%zd bytes_to_parse=%zd",
                               where, handler_bytes_read, packet_size, parsed_len, bytes_to_parse);
        } else {
          parsed_len += packet_size;
        }

        if (failed_count >= 5)
          return 0;
      }
    }
    return 0;
  }


  void QBT_CTA_QBDF_Builder::populate_ip_context_map()
  {
    CQS_CTS_Handler::channel_info_map_t& cim = m_message_handler->m_channel_info_map;

    const vector<Parameter>& cqs_lines = m_params["cqs_lines"].getVector();
    for(vector<Parameter>::const_iterator l = cqs_lines.begin(); l != cqs_lines.end(); ++l) {
      const string& name = (*l)["name"].getString();
      size_t context = cim.size();
      cim.push_back(CQS_CTS_Handler::channel_info_t(name, context, true));
      CQS_CTS_Handler::channel_info_t& ci = cim.back();
      ci.init(m_params);
      ci.has_reordering = false;
    }

    const vector<Parameter>& cta_lines = m_params["cts_lines"].getVector();
    for(vector<Parameter>::const_iterator l = cta_lines.begin(); l != cta_lines.end(); ++l) {
      const string& name = (*l)["name"].getString();
      size_t context = cim.size();
      cim.push_back(CQS_CTS_Handler::channel_info_t(name, context, false));
      CQS_CTS_Handler::channel_info_t& ci = cim.back();
      ci.init(m_params);
      ci.has_reordering = false;
    }
  }

}
