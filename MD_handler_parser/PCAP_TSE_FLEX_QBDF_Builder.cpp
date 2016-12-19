#include <Data/PCAP_TSE_FLEX_QBDF_Builder.h>

#include <Data/TSE_FLEX_Handler.h>
#include <Data/PCAP_Parser.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  int PCAP_TSE_FLEX_QBDF_Builder::process_raw_file(const string& file_name)
  {
    const char* where = "PCAP_TSE_FLEX_QBDF_Builder::process_raw_file";

    m_logger->log_printf(Log::INFO, "%s: Reading [%s]", where, file_name.c_str());
    m_logger->sync_flush();

    boost::shared_ptr<TSE_FLEX_Handler> message_handler(new TSE_FLEX_Handler(m_app));
    message_handler->init(m_source_name, ExchangeID::XTKS, m_params);
    message_handler->set_qbdf_builder(this);

    Handler::channel_info_map_t& cim = message_handler->m_channel_info_map;
    for (size_t i = 0; i < 256; ++i) {
      cim.push_back(channel_info("", i));
      cim.back().context = i; //
      cim.back().has_reordering = false;
    }

    PCAP_Parser p;

    p.read(file_name, message_handler.get());
    message_handler->write_morning_closes();
    message_handler->write_afternoon_closes();
    message_handler->write_special_quotes();

    return 0;
  }

}
