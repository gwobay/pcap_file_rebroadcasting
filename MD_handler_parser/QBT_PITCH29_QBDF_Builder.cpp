#include <math.h>

#include <ASIO/IO_Service.h>
#include <Logger/Logger_Manager.h>
#include <Data/MoldUDP64.h>
#include <Data/QBT_PITCH29_QBDF_Builder.h>
#include <Data/PITCH_2_9_Handler.h>
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
  int QBT_PITCH29_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_PITCH29_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    Time::set_simulation_mode(true);
    Time::set_current_time(Time(m_date, "%Y%m%d"));
    m_message_handler = new PITCH_2_9_Handler(m_app);
    m_message_handler->set_qbdf_builder(this);
    m_message_handler->init(m_source_name, m_hf_exch_id, m_params);

    Handler::channel_info_map_t& cim = m_message_handler->m_channel_info_map;
    for (size_t i = 0; i < 64; ++i) {
      cim.push_back(channel_info("", i));
      cim.back().context = i; //
      cim.back().has_reordering = false;
    }

    parse_qrec_file(m_message_handler, m_logger.get(), f_in, 50);

    return 0;
  }
}
