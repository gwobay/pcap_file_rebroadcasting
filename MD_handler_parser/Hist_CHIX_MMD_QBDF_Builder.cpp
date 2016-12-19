#include <math.h>

#include <ASIO/IO_Service.h>
#include <Logger/Logger_Manager.h>
#include <Data/MoldUDP64.h>
#include <Data/Hist_CHIX_MMD_QBDF_Builder.h>
#include <Data/CHIX_MMD_Handler.h>
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
  int Hist_CHIX_MMD_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "Hist_CHIX_MMD_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    Time::set_current_time(Time(m_date, "%Y%m%d"));

    m_message_handler = new CHIX_MMD_Handler(m_app);
    m_message_handler->init(m_source_name, ExchangeID::CHIJ, m_params);
    m_message_handler->set_qbdf_builder(this);
    m_message_handler->enable_hist_mode();

    size_t buffer_size = 1024 * 1024;
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

      while ((parsed_len < bytes_to_parse)) {
        //cout << "parsed_len=" << parsed_len << " bytes_to_parse=" << bytes_to_parse << endl;
        size_t i = parsed_len;
        while (i < bytes_to_parse && buffer[i] != '\n') {
          //cout << "i=" << i << " buffer[i]=" << buffer[i] << endl;
          i++;
        }
        if (buffer[i] == '\n') {
          uint16_t packet_size = i - parsed_len + 1;
          //cout << "start=" << buffer[parsed_len] << " end=" << buffer[parsed_len+packet_size-1] << endl;
          //cout << "packet_size=" << packet_size << endl;
          //size_t handler_bytes_read = m_message_handler->parse(0, buffer+parsed_len, packet_size);
          parsed_len += packet_size;
        } else {
          //need to read more
          break;
        }
      }
    }
    return 0;
  }
}
