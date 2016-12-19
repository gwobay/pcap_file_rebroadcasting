#pragma once
#include <Data/QBDF_Builder.h>
#include <Data/NXT_Book_Handler.h>

namespace hf_core {
  class NXT_CSV_QBDF_Builder : public QBDF_Builder {
  public:

    NXT_CSV_QBDF_Builder(Application* app, const std::string& date)
    : QBDF_Builder(app, date),
      m_context(0),
      m_seq_no(0)
    {
      initialize_buffer();
    }

  protected:
    int process_raw_file(const std::string& file_name);
    int process_trade_file(const std::string& file_name) { return 0; }
    int process_quote_file(const std::string& file_name) { return 0; }

  private:
    static const size_t MAX_PACKET_SIZE = 1024 * 1024; // one megabyte, enough for 28000+ price level updates (37 bytes each)

    void initialize_buffer(); // stuff specific to the NXT_CSV_QBDF_Builder. QBDF_Builder itself has its own init function;

    void increment_message_count() {
      struct nxt_book::data_packet *dp = reinterpret_cast<nxt_book::data_packet*>(m_buf + sizeof(nxt_book::multicast_packet_header_v06));
      dp->message_count++;
    }

    char m_buf[MAX_PACKET_SIZE];
    uint16_t m_len; // running length of packet (i.e., how much of m_buf is filled)
    uint64_t m_pl_count;
    uint64_t m_order_count;
    size_t m_context;
    uint32_t m_seq_no;
    bool m_multiple_price_levels;
    bool m_multiple_attached_orders;
  };
}
