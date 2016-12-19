#include <Data/Eurex_Handler.h>
#include <Data/PCAP_Eurex_QBDF_Builder.h>

#include <Data/PCAP_Parser.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  int PCAP_Eurex_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "PCAP_Eurex_QBDF_Builder::process_raw_file";
    Time::set_simulation_mode(true);
    Time::set_current_time(Time(m_date, "%Y%m%d"));

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    boost::shared_ptr<Eurex_Handler> message_handler(new Eurex_Handler(m_app));



    bool ref_data_mode = m_params.getDefaultBool("get_ref_data", false);
    if (ref_data_mode)
      message_handler->set_ref_data_mode();

    // m_logger->log_printf(Log::INFO, "%s: FYI sizeof(Eurex::RDPacketHeader) == %zd", where, sizeof(Eurex::RDPacketHeader));
    m_logger->log_printf(Log::INFO, "%s: REF DATA MODE %d", where, message_handler->get_ref_data_mode());
    m_logger->sync_flush();


    message_handler->init(m_source_name, ExchangeID::XEUR, m_params);
    message_handler->set_qbdf_builder(this);

    PCAP_Parser p;
    p.set_inside_file_name(m_params.getDefaultString("raw_file_inside_archive", ""));
    p.read(file_name, message_handler.get());
    //// TODO: write closes ...?
    // message_handler->write_morning_closes();
    // message_handler->write_afternoon_closes();
    // message_handler->write_special_quotes();

    return 0;
  }

}
