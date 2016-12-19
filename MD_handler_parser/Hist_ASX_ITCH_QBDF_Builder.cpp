#include <math.h>

#include <ASIO/IO_Service.h>
#include <Logger/Logger_Manager.h>
#include <Data/Hist_ASX_ITCH_QBDF_Builder.h>
#include <Data/ASX_ITCH_1_1_Handler.h>
#include <MD/MDManager.h>
#include <Util/File_Utils.h>
#include <Util/Network_Utils.h>
#include <Util/Parameter.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  
  struct soup_bin_tcp_header {
    uint16_t packet_len;
    char     packet_type;
  } __attribute__ ((packed));

  struct soup_bin_login_accepted_packet {
    char     session[10];
    char     seq_num[20];
  } __attribute__ ((packed));


  // ----------------------------------------------------------------
  // load in raw trades file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int Hist_ASX_ITCH_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "Hist_ASX_ITCH_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());

    FILE *f_in = NULL;
    if (file_name.substr(file_name.size() - 4, 4) == ".7za") {
      string file_cmd("/tools/p7zip/redhat/x_64/9.20.1/bin/7z x -so -bd -y " + file_name);
      f_in = popen(file_cmd.c_str(), "r");
    } else {
      f_in = fopen(file_name.c_str(), "r");
    }

    if (f_in == 0) {
#ifdef CW_DEBUG
      printf("%s: ERROR unable to open raw file %s for reading\n", where, file_name.c_str());
#endif
      return 1;
    }

    Time::set_simulation_mode(true);
    Time::set_current_time(Time(m_date, "%Y%m%d"));
    m_message_handler = new ASX_ITCH_1_1_Handler(m_app);
    m_message_handler->init(m_source_name, m_hf_exch_id, m_params);
    m_message_handler->enable_hist_mode();
    m_message_handler->set_qbdf_builder(this);

    size_t buffer_size = 1024 * 1024;

    size_t total_parsed_len = 0;
    size_t parsed_len = 0;
    size_t bytes_to_parse = 0;
    char buffer[buffer_size];
    size_t max_msg_size = 1500;

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

      //cout << "bytes_to_parse=" << bytes_to_parse << " max_mold_msg_size=" << max_mold_msg_size << endl;

      while (parsed_len + max_msg_size < bytes_to_parse) {
        uint64_t context = 0;

	const soup_bin_tcp_header& hdr = reinterpret_cast<const soup_bin_tcp_header&>(*(buffer+parsed_len));
	uint16_t packet_len = ntohs(hdr.packet_len)-1; // packet_len includes packet_type byte
	char packet_type = hdr.packet_type;
	parsed_len += sizeof(soup_bin_tcp_header);

	if (packet_type == 'S') {
	  size_t handler_bytes_read = m_message_handler->parse(context, buffer+parsed_len, packet_len);
	  parsed_len += handler_bytes_read;
	} else {
	  parsed_len += packet_len;
	}
      }
      total_parsed_len += parsed_len;
    }

    fclose(f_in);
    return 0;
  }
}
