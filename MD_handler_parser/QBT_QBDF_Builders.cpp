#include <Data/QBT_QBDF_Builders.h>

#include <Util/File_Utils.h>

#include <ASIO/IO_Service.h>
#include <Logger/Logger_Manager.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  int QBT_LSE_MILLENNIUM_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_LSE_MILLENNIUM_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    m_message_handler = new LSE_Millennium_Handler(m_app);
    m_message_handler->init(m_source_name, ExchangeID::XLON, m_params);
    m_message_handler->set_qbdf_builder(this);

    LSE_Millennium_Handler::channel_info_map_t& cim = m_message_handler->m_channel_info_map;
    for (size_t i = 0; i < 128; ++i) {
      cim.push_back(channel_info("", i));
      cim.back().context = i;
      cim.back().has_reordering = false;
    }

    // Contexts we record start at 67
    parse_qrec_file(m_message_handler, m_logger.get(), f_in, 80);



    return 0;
  }

  int QBT_TSE_FLEX_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_TSE_FLEX_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    m_message_handler = new TSE_FLEX_Handler(m_app);
    m_message_handler->init(m_source_name, ExchangeID::XTKS, m_params);
    m_message_handler->set_qbdf_builder(this);

    TSE_FLEX_Handler::channel_info_map_t& cim = m_message_handler->m_channel_info_map;
    for (size_t i = 0; i < 128; ++i) {
      cim.push_back(channel_info("", i));
      cim.back().context = i;
      cim.back().has_reordering = false;
    }

    // Contexts we record start at 67
    parse_qrec_file(m_message_handler, m_logger.get(), f_in, 80);
    m_message_handler->write_morning_closes();
    m_message_handler->write_afternoon_closes();
    m_message_handler->write_special_quotes();

    if (m_params.getDefaultBool("write_bp_data", false)) {
      m_message_handler->write_bp_data();
    }

    if (m_params.getDefaultBool("write_ii_data", false)) {
      m_message_handler->write_ii_data();
    }


    return 0;
  }


  int QBT_CHIX_MMD_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_CHIX_MMD_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    m_message_handler = new CHIX_MMD_Handler(m_app);
    m_message_handler->init(m_source_name, ExchangeID::CHIJ, m_params);
    m_message_handler->set_qbdf_builder(this);

    CHIX_MMD_Handler::channel_info_map_t& cim = m_message_handler->m_channel_info_map;
    for (size_t i = 0; i < 128; ++i) {
      cim.push_back(channel_info("", i));
      cim.back().context = i;
      cim.back().has_reordering = false;
    }

    parse_qrec_file(m_message_handler, m_logger.get(), f_in, 80);

    return 0;
  }

  int QBT_TMX_QL2_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_TMX_QL2_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    m_message_handler = new TMX_QL2_Handler(m_app);
    m_message_handler->init(m_source_name, ExchangeID::XTSE, m_params);
    m_message_handler->set_qbdf_builder(this);

    TMX_QL2_Handler::channel_info_map_t& cim = m_message_handler->m_channel_info_map;
    for (size_t i = 0; i < 128; ++i) {
      cim.push_back(channel_info("", i));
      cim.back().context = i;
      cim.back().has_reordering = false;
    }

    parse_qrec_file(m_message_handler, m_logger.get(), f_in, 80);
   
    return 0;
  }

  int QBT_TMX_TL1_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_TMX_TL1_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    m_message_handler = new TMX_TL1_Handler(m_app);
    m_message_handler->init(m_source_name, ExchangeID::XTSE, m_params);
    m_message_handler->set_qbdf_builder(this);

    TMX_TL1_Handler::channel_info_map_t& cim = m_message_handler->m_channel_info_map;
    for (size_t i = 0; i < 128; ++i) {
      cim.push_back(channel_info("", i));
      cim.back().context = i;
      cim.back().has_reordering = false;
    }

    parse_qrec_file(m_message_handler, m_logger.get(), f_in, 80);
   
    return 0;
  }

  int Hist_ITCH5_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "Hist_ITCH5_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    struct hist_itch_header {
      uint16_t packet_len;
    } __attribute__ ((packed));

    Time::set_simulation_mode(true);
    Time::set_current_time(Time(m_date, "%Y%m%d"));
    m_message_handler = new TVITCH_5_Handler(m_app);
    m_message_handler->init(m_source_name, m_hf_exch_id, m_params);
    m_message_handler->enable_hist_mode();
    m_message_handler->set_qbdf_builder(this);
    m_message_handler->disable_reordering();

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

      while (parsed_len + max_msg_size < bytes_to_parse) {
        uint64_t context = 0;

        const hist_itch_header& hdr = reinterpret_cast<const hist_itch_header&>(*(buffer+parsed_len));
        uint16_t packet_len = ntohs(hdr.packet_len);
        parsed_len += sizeof(hist_itch_header);

        size_t handler_bytes_read = m_message_handler->parse_message(context, buffer+parsed_len, packet_len);

        parsed_len += handler_bytes_read;
      }
      total_parsed_len += parsed_len;
    }

    return 0;
  }


  int QBT_ITCH5_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_ITCH5_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    Time::set_simulation_mode(true);
    Time::set_current_time(Time(m_date, "%Y%m%d"));
    m_message_handler = new TVITCH_5_Handler(m_app);
    m_message_handler->init(m_source_name, m_hf_exch_id, m_params);
    m_message_handler->set_qbdf_builder(this);
    m_message_handler->disable_reordering();

    parse_qrec_file(m_message_handler, m_logger.get(), f_in, 2);

    return 0;
  }

}
