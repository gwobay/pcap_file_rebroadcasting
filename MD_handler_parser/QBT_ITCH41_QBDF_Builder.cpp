#include <math.h>

#include <ASIO/IO_Service.h>
#include <Logger/Logger_Manager.h>
#include <Data/MoldUDP64.h>
#include <Data/QBT_ITCH41_QBDF_Builder.h>
#include <Data/TVITCH_4_1_Handler.h>
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
  int QBT_ITCH41_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "QBT_ITCH41_QBDF_Builder::process_raw_file";
    
    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());

    open_FILE f_in(file_name);
    if(!f_in) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to read raw file: [%s]", where, file_name.c_str());
      return 1;
    }

    Time::set_simulation_mode(true);
    Time::set_current_time(Time(m_date, "%Y%m%d"));
    m_message_handler = new TVITCH_4_1_Handler(m_app);
    m_message_handler->init(m_source_name, m_hf_exch_id, m_params);
    m_message_handler->set_qbdf_builder(this);

    m_mold_reader = new MoldUDP64(m_app, m_message_handler);
    m_mold_reader->init(m_source_name, m_params);
    m_mold_reader->disable_reordering();

    parse_qrec_file(m_mold_reader, m_logger.get(), f_in, 2);

    return 0;
  }
}
