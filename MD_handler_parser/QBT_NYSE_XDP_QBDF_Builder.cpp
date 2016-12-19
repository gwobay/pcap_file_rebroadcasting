#include <math.h>

#include <ASIO/IO_Service.h>
#include <Logger/Logger_Manager.h>
#include <Data/MoldUDP64.h>
#include <Data/QBT_NYSE_XDP_QBDF_Builder.h>
#include <MD/MDManager.h>
#include <Util/CSV_Parser.h>
#include <Util/File_Utils.h>
#include <Util/Network_Utils.h>
#include <Util/Parameter.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  // ----------------------------------------------------------------
  // load in raw trades file and sort it for later dumping to binary format
  // ----------------------------------------------------------------
  int QBT_NYSE_XDP_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_NYSE_XDP_QBDF_Builder::process_raw_file";
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
    m_message_handler = new NYSE_XDP_Handler(m_app);
    m_message_handler->init(m_source_name, ExchangeID::XNYS, m_params);
    m_message_handler->set_qbdf_builder(this);

    if (m_params.has("write_link_records")) {
      map<uint32_t,string> openbook_ids;
      map<uint32_t,string> nysetrade_ids;

      string submap_file = m_params["submap_file"].getString();
      CSV_Parser csv_reader(submap_file);

      int sid_posn = csv_reader.column_index("sid");
      int service_posn = csv_reader.column_index("service");
      int source_posn = csv_reader.column_index("source");
      int symbol_root_posn = csv_reader.column_index("symbol_root");

      CSV_Parser::state_t s = csv_reader.read_row();
      while (s == CSV_Parser::OK) {
        hf_core::Product::sid_t sid = strtol(csv_reader.get_column(sid_posn), 0, 10);
        string service = csv_reader.get_column(service_posn);
        string source = csv_reader.get_column(source_posn);
        string symbol = csv_reader.get_column(symbol_root_posn);

        if (service == "DIRECT" and source == "OPENBOOK") {
          openbook_ids[sid] = symbol;
        } else if (service == "DIRECT" and source == "NYSETRADES") {
          nysetrade_ids[sid] = symbol;
        }
        s = csv_reader.read_row();
      }

      map<uint16_t,string> openbook_tickers;
      map<uint32_t,string>::iterator ob_iter, nt_iter;
      for (ob_iter = openbook_ids.begin(); ob_iter != openbook_ids.end(); ob_iter++) {
        nt_iter = nysetrade_ids.find(ob_iter->first);
        if (nt_iter == nysetrade_ids.end())
          continue;

        uint16_t openbook_symbol_id = strtol(ob_iter->second.c_str(), 0, 10);
        string nysetrade_ticker = nt_iter->second;
        openbook_tickers[openbook_symbol_id] = nysetrade_ticker;
      }
      m_message_handler->set_openbook_tickers(openbook_tickers);

    } else if (m_params.has("read_link_records")) {
      string link_file = m_params["read_link_records"].getString();
      link_record_map_t link_record_map = get_link_records(link_file);
      m_message_handler->set_link_record_map(link_record_map);
    }


    NYSE_Handler::channel_info_map_t& cim = m_message_handler->m_channel_info_map;
    for (size_t i = 0; i < 32; ++i) {
      cim.push_back(channel_info("", i));
      cim.back().has_reordering = false;
    }

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

  void QBT_NYSE_XDP_QBDF_Builder::populate_subscription_map (NYSE_Handler* handler) {
    string submap_file = m_params["submap_file"].getString();
    string submap_file_source = m_params["submap_file_source"].getString();
    CSV_Parser csv_reader(submap_file);

    int service_posn = csv_reader.column_index("service");
    int source_posn = csv_reader.column_index("source");
    int symbol_root_posn = csv_reader.column_index("symbol_root");

    CSV_Parser::state_t s = csv_reader.read_row();
    while (s == CSV_Parser::OK) {
      string service = csv_reader.get_column(service_posn);
      string source = csv_reader.get_column(source_posn);
      if (service != "DIRECT" or source != submap_file_source) {
        s = csv_reader.read_row();
        continue;
      }

      string symbol = csv_reader.get_column(symbol_root_posn);
      ProductID pid = process_taq_symbol(symbol);
      handler->subscribe_product(pid, ExchangeID::XNYS, symbol, "");
      s = csv_reader.read_row();
    }
  }
}
