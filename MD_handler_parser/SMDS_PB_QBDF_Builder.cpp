#include <math.h>

#include <ASIO/IO_Service.h>
#include <Logger/Logger_Manager.h>
#include <Data/MoldUDP64.h>
#include <Data/SMDS_PB_QBDF_Builder.h>
#include <Data/ARCA_Handler.h>
#include <Data/CQS_CTS_Handler.h>
#include <Data/DirectEdge_0_9_Handler.h>
#include <Data/NYSE_Handler.h>
#include <Data/PITCH_2_9_Handler.h>
#include <Data/TVITCH_4_1_Handler.h>
#include <Data/UTP_Handler.h>
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
  int SMDS_PB_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "SMDS_PB_QBDF_Builder::process_raw_file";
    m_logger->log_printf(Log::INFO, "%s: called with file %s", where, file_name.c_str());

    size_t buffer_size = 1024 * 1024;
    size_t metastart_size = 22;
    size_t metaend_size = 16;
    //size_t metadata_min_size = metastart_size + metaend_size + 5;
    size_t metadata_max_size = 100;

    string header;
    header.reserve(100);

    string source_ip, source_port,source_key;
    source_ip.reserve(15);
    source_port.reserve(6);
    source_key.reserve(22);

    std::vector<string> header_fields;
    header_fields.reserve(9);

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

    string handler_class_name = m_params["handler_class_name"].getString();
    // Need factory here
    if (handler_class_name == "hf_core.Data.ARCA_Handler") {
      m_message_handler = new ARCA_Handler(m_app);
    } else if (handler_class_name == "hf_core.Data.CQS_CTS_Handler") {
      m_message_handler = new CQS_CTS_Handler(m_app);
    } else if (handler_class_name == "hf_core.Data.DirectEdge_0_9_Handler") {
      m_message_handler = new DirectEdge_0_9_Handler(m_app);
    } else if (handler_class_name == "hf_core.Data.NYSE_Handler") {
      m_message_handler = new NYSE_Handler(m_app);
    } else if (handler_class_name == "hf_core.Data.PITCH_2_9_Handler") {
      m_message_handler = new PITCH_2_9_Handler(m_app);
    } else if (handler_class_name == "hf_core.Data.TVITCH_4_1_Handler") {
      m_message_handler = new TVITCH_4_1_Handler(m_app);
    } else if (handler_class_name == "hf_core.Data.UTP_Handler") {
      m_message_handler = new UTP_Handler(m_app);
    } else {
      m_logger->log_printf(Log::ERROR, "%s: Unsupported handler: %s", where, handler_class_name.c_str());
      return 1;
    }

    int handler_exchange_id = m_params["handler_exchange_id"].getInt();
    ExchangeID eid = m_app->em()->find_by_primary_id(handler_exchange_id)->primary_id();
    m_message_handler->init(m_source_name, eid, m_params);
    m_message_handler->set_qbdf_builder(this);

    populate_ip_context_map();
    m_logger->sync_flush();

    size_t buffer_posn = 0;
    size_t bytes_to_parse = 0;
    char buffer[buffer_size];

    bool read_more = true;
    while (read_more) {
      size_t unparsed_len = bytes_to_parse - buffer_posn;
      if (unparsed_len > 0) {
        // Move leftover bytes to beginning of buffer, fill remaining buffer
        memmove(buffer, buffer+buffer_posn, unparsed_len);
        bytes_to_parse = fread(buffer+unparsed_len, 1, buffer_size-unparsed_len, f_in);
        if (bytes_to_parse < (buffer_size-unparsed_len))
          read_more = false;

        buffer_posn = 0;
        bytes_to_parse += unparsed_len;
      } else {
        buffer_posn = 0;
        bytes_to_parse = fread(buffer, 1, buffer_size, f_in);
        if (bytes_to_parse < buffer_size)
          read_more = false;
      }

      while ((buffer_posn + metadata_max_size) < bytes_to_parse) {
        char* metastart = (char*)(buffer+buffer_posn);
        if (strncmp("*M*E*T*A*S*T*A*R*T*/4/", metastart, metastart_size)) {
          bool found_metastart = false;
          for ( ; buffer_posn < (bytes_to_parse - metadata_max_size); buffer_posn++) {
            metastart = (char*)(buffer+buffer_posn);
            if (strncmp("*M*E*T*A*S*T*A*R*T*/4/", metastart, metastart_size) == 0) {
              m_logger->log_printf(Log::INFO, "%s: Found next metastart at byte %ld", where, buffer_posn);
              found_metastart = true;
              break;
            }
          }
          if (!found_metastart) {
            // Need to read more data
            m_logger->log_printf(Log::INFO, "%s: Couldn't find next metastart, need more bytes", where);
            break;
          }
        }

        size_t latest_record_start_posn = buffer_posn;
        const char* begin = 0;
        const char* end = 0;

        uint32_t packet_size;
        if (get_next_token_no_escape(metastart+metastart_size, metadata_max_size, &begin, &end, '/')) {
          packet_size = atoi(string(begin, end - begin).c_str());
        } else {
          m_logger->log_printf(Log::ERROR, "%s: Bad header, could not extract packet size", where);
          m_logger->sync_flush();
          continue;
        }

        if (get_next_token_no_escape(metastart+metastart_size, metadata_max_size, &begin, &end, '/')) {
          source_ip.assign(begin, end - begin);
        } else {
          m_logger->log_printf(Log::ERROR, "%s: Bad header, could not extract source IP", where);
          m_logger->sync_flush();
          continue;
        }

        if (get_next_token_no_escape(metastart+metastart_size, metadata_max_size, &begin, &end, '/')) {
          source_port.assign(begin, end - begin);
        } else {
          m_logger->log_printf(Log::ERROR, "%s: Bad header, could not extract source port", where);
          m_logger->sync_flush();
          continue;
        }

        for (int x = 0; x < 3; x++) {
          if (!get_next_token_no_escape(metastart+metastart_size, metadata_max_size, &begin, &end, '/')) {
            m_logger->log_printf(Log::ERROR, "%s: Bad header, could not find enough delimiters", where);
            m_logger->sync_flush();
            continue;
          }
        }

        if (strncmp("/*M*E*T*A*E*N*D*", end, metaend_size)) {
          m_logger->log_printf(Log::ERROR, "%s: Bad header, could not find METAEND: %s",
                               where, string(end, end+metaend_size).c_str());
          m_logger->sync_flush();
          continue;
        }

        buffer_posn = (end - buffer) + metaend_size;
        if ((buffer_posn + packet_size + 1) > buffer_size) {
          buffer_posn = latest_record_start_posn;
          break;
        }

        size_t context;
        source_key = source_ip + ":" + source_port;
        ip_context_map_t::iterator ipc_iter = m_ip_context_map.find(source_key);
        if (ipc_iter != m_ip_context_map.end()) {
          context = ipc_iter->second;
        } else {
          //m_logger->log_printf(Log::DEBUG, "%s: unexpected source key: %s", where, source_key.c_str());
          buffer_posn += (packet_size + 1);
          continue;
        }

        size_t handler_bytes_read = m_message_handler->parse(context, buffer+buffer_posn, packet_size);
        if (handler_bytes_read != packet_size) {
          m_logger->log_printf(Log::ERROR, "%s: handler_bytes_read=%ld, expected packet_size=%d buffer_posn=%ld",
                               where, handler_bytes_read, packet_size, buffer_posn);
          m_logger->sync_flush();
        } else {
          buffer_posn += packet_size;
        }

        buffer_posn++; // always seems to be one extra byte (maybe newline char?)
      }
    }

    if (f_in) {
      if(has_zip_extension(file_name) || has_gzip_extension(file_name)) {
        pclose(f_in);
      } else {
        fclose(f_in);
      }
    }
    return 0;
  }

  void SMDS_PB_QBDF_Builder::populate_ip_context_map() {
    const char* where = "SMDS_PB_QBDF_Builder::populate_ip_context_map";
    Handler::channel_info_map_t& cim = m_message_handler->m_channel_info_map;
    const vector<Parameter>& lines = m_params["network_settings"].getVector();
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      string name;
      if (l->has("name")) {
        name = (*l)["name"].getString();
      } else if (l->has("unit")) {
        int unit = (*l)["unit"].getInt();
        stringstream ss;
        ss << unit;
        name = ss.str();
      }

      const string& A_group = (*l)["A_group"].getString();
      int A_port = (*l)["A_port"].getInt();

      const string& B_group = (*l)["B_group"].getString();
      int B_port = (*l)["B_port"].getInt();

      size_t context = cim.size();
      m_logger->log_printf(Log::INFO, "%s: Adding channel with name=%s context=%lu",
                           where, name.c_str(), context);
      cim.push_back(channel_info(name, context));
      channel_info& ci = cim.back();
      ci.init(m_params);
      ci.has_reordering = false;

      stringstream A_group_stream;
      A_group_stream << A_group << ":" << A_port;
      m_ip_context_map.insert(make_pair(A_group_stream.str(), context));

      stringstream B_group_stream;
      B_group_stream << B_group << ":" << B_port;
      m_ip_context_map.insert(make_pair(B_group_stream.str(), context));

      if(l->has("A_retrans_group")) {
        ci.has_recovery = true;

        const string& A_retrans_group = (*l)["A_retrans_group"].getString();
        int A_retrans_port = (*l)["A_retrans_port"].getInt();

        const string& B_retrans_group = (*l)["B_retrans_group"].getString();
        int B_retrans_port = (*l)["B_retrans_port"].getInt();

        const string& retrans_ip = (*l)["retrans_ip"].getString();
        int retrans_port = (*l)["retrans_port"].getInt();

        memset(&ci.recovery_addr, 0, sizeof(ci.recovery_addr));
        fill_sockaddr_in(ci.recovery_addr, retrans_ip.c_str(), retrans_port);

        stringstream A_retrans_group_stream;
        A_retrans_group_stream << A_retrans_group << ":" << A_retrans_port;
        m_ip_context_map.insert(make_pair(A_retrans_group_stream.str(), context));

        stringstream B_retrans_group_stream;
        B_retrans_group_stream << B_retrans_group << ":" << B_retrans_port;
        m_ip_context_map.insert(make_pair(B_retrans_group_stream.str(), context));

        stringstream retrans_ip_stream;
        retrans_ip_stream << retrans_ip << ":" << retrans_port;
        m_ip_context_map.insert(make_pair(retrans_ip_stream.str(), context));
      }

      if(l->has("A_interval_group")) {
        size_t context = cim.size();
        const string& interval_name = name + "_int";
        m_logger->log_printf(Log::INFO, "%s: Adding channel with name=%s context=%lu",
                             where, interval_name.c_str(), context);
        cim.push_back(channel_info(interval_name, context));
        cim.back().has_reordering = false;

        stringstream A_interval_group_stream;
        A_interval_group_stream << (*l)["A_interval_group"].getString() << ":" << (*l)["A_interval_port"].getInt();
        m_ip_context_map.insert(make_pair(A_interval_group_stream.str(), context));

        stringstream B_interval_group_stream;
        B_interval_group_stream << (*l)["B_interval_group"].getString() << ":" << (*l)["B_interval_port"].getInt();
        m_ip_context_map.insert(make_pair(B_interval_group_stream.str(), context));
      }
    }

  }
}
