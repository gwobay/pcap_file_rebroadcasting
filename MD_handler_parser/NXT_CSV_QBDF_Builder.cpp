#include <Util/CSV_Parser.h>
#include <Data/NXT_CSV_QBDF_Builder.h>

using namespace std;
namespace hf_core {

    void NXT_CSV_QBDF_Builder::initialize_buffer() {
      m_len = 0;
      m_pl_count = 0;
      m_multiple_price_levels = false;
      m_order_count = 0;
      m_multiple_attached_orders = false;
      memset(m_buf, 0, MAX_PACKET_SIZE);
      nxt_book::multicast_packet_header_v06 *head = reinterpret_cast<nxt_book::multicast_packet_header_v06*>(m_buf);
      head->protocol_version = 5;
      head->sender_id = -1;
      head->packet_size = 0;
      head->sequence_number = m_seq_no++;
      head->packet_type = 'D';
      m_len += sizeof(nxt_book::multicast_packet_header_v06);

      struct nxt_book::data_packet *dp = reinterpret_cast<nxt_book::data_packet*>(m_buf + m_len);
      dp->fragment_indicator = 0;
      increment_message_count();
      m_len += sizeof(nxt_book::data_packet);
    }


  int NXT_CSV_QBDF_Builder::process_raw_file(const string& file_name)
  {
    boost::shared_ptr<NXT_Book_Handler> message_handler(new NXT_Book_Handler(m_app));

    Time::set_current_time(Time(m_date, "%Y%m%d"));

    message_handler->set_qbdf_builder(this);
    message_handler->init(m_source_name, m_hf_exch_ids, m_params);
    message_handler->create_qbdf_channel_info();

    CSV_Parser parser(file_name);
    parser.set_ignore_errors(true);
    parser.read_row(); // first line is garbage

    // uint64_t attached_order_count = 0;

    // actually -- we just pretend each message comes in its own packet. it makes
    // no difference to the Handler and is easier to assemble on our end.
    //
    // price book updates come with repetitions, so we have to rebuild the
    // buffer before passing it to the parser. the strategy is to populate the
    // buffer for these messages, then continue (in the loop sense) to get the
    // next line of the CSV. once we have determined that we have received the
    // last message of the sequece, we call parse. for other message types, we
    // call parse as each one is received. zero out the buffer and reset the
    // size after the parse count.

    while(CSV_Parser::OK == parser.read_row()) {
      string message_type = parser.get_column(0);
      uint8_t c = message_type.c_str()[0];
      switch(c) {
        case 'R':
          {
            nxt_book::trading_status *msg = reinterpret_cast<nxt_book::trading_status *>(m_buf + m_len);
            msg->message_type = c;
            msg->message_size = sizeof(nxt_book::trading_status);
            msg->symbol_id = strtoul(parser.get_column(1), NULL, 10);
            msg->symbol_seq_number = strtoul(parser.get_column(2), NULL, 10);
            msg->symbol_status = strtoul(parser.get_column(3), NULL, 10);
            msg->trading_phase = parser.get_column(4)[0];
            msg->short_sale_restriction = strtoul(parser.get_column(5), NULL, 10) == 1;
            msg->padding = 0;
            msg->timestamp = strtoul(parser.get_column(6), NULL, 10);
            m_len += msg->message_size;

            Time t(m_midnight + convert_timestamp(msg->timestamp));
            if (t > Time::current_time()) {
              Time::set_current_time(t);
            }
          }
          break;
        case 'J':
          {
            nxt_book::trade *msg = reinterpret_cast<nxt_book::trade *>(m_buf + m_len);
            msg->message_type = c;
            msg->message_size = sizeof(nxt_book::trade);
            msg->symbol_id = strtoul(parser.get_column(1), NULL, 10);
            msg->symbol_seq_number = strtoul(parser.get_column(2), NULL, 10);
            msg->symbol_status = strtoul(parser.get_column(3), NULL, 10);
            msg->price = strtod(parser.get_column(4), NULL);
            msg->high_price = strtod(parser.get_column(5), NULL);
            msg->low_price = strtod(parser.get_column(6), NULL);
            msg->trade_id = strtoul(parser.get_column(7), NULL, 10);
            msg->order_id = strtoul(parser.get_column(8), NULL, 10);

            // 2 lines seems to avoid strict aliasing warning (instead of just dereferencing in the same line as the cast)
            uint64_t trade_flag = strtoul(parser.get_column(9), NULL, 10);
            struct nxt_book::trade::TradeFlags *trade_flags = reinterpret_cast<nxt_book::trade::TradeFlags*>(&trade_flag);

            msg->trade_flags = *trade_flags;
            msg->cumulative_volume = strtoul(parser.get_column(10), NULL, 10);
            msg->trade_size = strtoul(parser.get_column(11), NULL, 10);
            msg->trade_timestamp = strtoul(parser.get_column(12), NULL, 10);
            msg->side = 1;
            m_len += msg->message_size;


            Time t(m_midnight + convert_timestamp(msg->trade_timestamp));
            if (t > Time::current_time()) {
              Time::set_current_time(t);
            }
          }
          break;
        case 'F':
          {

            // a bit of a state-machine. possible states are:
            // 1. starting new message
            //    state variables (incoming): m_multiple_price_levels == false
            //                                m_multiple_attached_orders == false
            //    m_buf only contains multicast_packet_header_v06 and data_packet only.
            //    in this case, we want to process the entire line to populate a price_book struct and
            //    the first price level (price_book_bid_ask struct). we also note if there are multiple price levels
            //    associated with this message. if there are mulitple price levels and we are not at the last one, we
            //    want to continue after processing this entire line so that parse does not get called until the buffer
            //    is fully populated.
            // 2. starting new price level within a message.
            //    state variables (incoming): m_multiple_price_levels == true
            //                                m_multiple_attached_orders == false
            //    here the buffer contains what it did when we were
            //    in state 1, plus it will contain the first few fields of the message (those that are not repeated)
            //    and all of the data associated with the previous price level(s) for the message. we want to skip
            //    the first few fields (everything in the price_book struct). we populate a price_book_bid_ask struct.
            //    and also note if there are any orders attached to the message. if there is one or more orders attached,
            //    we create the first attached_order struct. if there are multiple orders attached
            // 3. starting new order within a price level
            //    state variables (incoming): m_multiple_price_levels true or false
            //                                m_multiple_attached_orders == true
            //    we only want to create the orders on this pass, skipping the code that populates the prrice_book and
            //    price_book_bid_ask structs

            uint64_t pl_count = strtoul(parser.get_column(4), NULL, 10);
            if (!m_multiple_price_levels && !m_multiple_attached_orders) {
              m_multiple_attached_orders = false; // for each price level, reset
              m_order_count = 0;

              nxt_book::price_book *msg = reinterpret_cast<nxt_book::price_book *>(m_buf + m_len);
              msg->message_type = c;
              msg->message_size = sizeof(nxt_book::price_book);
              msg->symbol_id = strtoul(parser.get_column(1), NULL, 10);
              msg->symbol_seq_number = strtoul(parser.get_column(2), NULL, 10);
              msg->symbol_status = strtoul(parser.get_column(3), NULL, 10);
              msg->pl_count = pl_count;
              m_len += msg->message_size;

              if (pl_count > 1) {
                m_multiple_price_levels = true;
              }
            }

            uint64_t attached_order_count = strtoul(parser.get_column(10), NULL, 10);

            if (!m_multiple_attached_orders) {
              m_order_count = 0; // reset at each price level
              m_pl_count++;
              nxt_book::price_book_bid_ask *pbba = reinterpret_cast<nxt_book::price_book_bid_ask *>(m_buf + m_len);
              pbba->side = strtoul(parser.get_column(5), NULL, 10);
              pbba->price = strtod(parser.get_column(6), NULL);
              pbba->size = strtoul(parser.get_column(7), NULL, 10);
              pbba->timestamp = strtoul(parser.get_column(8), NULL, 10);
              pbba->pl_order_count = strtoul(parser.get_column(9), NULL, 10);
              pbba->attached_order_count  = attached_order_count;
              m_len += sizeof(nxt_book::price_book_bid_ask);

              Time t(m_midnight + convert_timestamp(pbba->timestamp));
              if (t > Time::current_time()) {
                Time::set_current_time(t);
              }

              if (attached_order_count > 1) {
                m_multiple_attached_orders = true;
              }
            }

            if (m_order_count < attached_order_count) {
              nxt_book::attached_order *order = reinterpret_cast<nxt_book::attached_order *>(m_buf + m_len);
              order->order_id = strtoul(parser.get_column(11), NULL, 10);
              order->order_size = strtoul(parser.get_column(12), NULL, 10);
              order->order_timestamp = strtoul(parser.get_column(13), NULL, 10);
              m_len += sizeof(nxt_book::attached_order);
              m_order_count++;
            }

            if (m_pl_count != pl_count || m_order_count != attached_order_count) {
              continue;
            }
          }
          break;
        case 'N':
          {
            nxt_book::imbalance *msg = reinterpret_cast<nxt_book::imbalance *>(m_buf + m_len);
            msg->message_type = c;
            msg->message_size = sizeof(nxt_book::imbalance);
            msg->symbol_id = strtoul(parser.get_column(1), NULL, 10);
            msg->symbol_seq_number = strtoul(parser.get_column(2), NULL, 10);
            msg->symbol_status = strtoul(parser.get_column(3), NULL, 10);
            msg->imbalance_volume = strtoul(parser.get_column(4), NULL, 10);
            msg->match_volume = strtoul(parser.get_column(5), NULL, 10);
            msg->ref_price = strtod(parser.get_column(6), NULL);
            msg->near_price = strtod(parser.get_column(7), NULL);
            msg->far_price = strtod(parser.get_column(8), NULL);
            msg->timestamp = strtoul(parser.get_column(9), NULL, 10);
            msg->imbalance_status = strtoul(parser.get_column(10), NULL, 10);
            msg->variance = parser.get_column(11)[0];

            m_len += sizeof(nxt_book::imbalance);

            Time t(m_midnight + convert_timestamp(msg->timestamp));
            if (t > Time::current_time()) {
              Time::set_current_time(t);
            }
          }
          break;
        default:
          {
            continue;
          }
          break;
      }

// do_parse:
      nxt_book::multicast_packet_header_v06* head = reinterpret_cast<nxt_book::multicast_packet_header_v06*>(m_buf);
      head->packet_size = m_len;
      message_handler->parse(m_context, m_buf, m_len);
      initialize_buffer();
    }
    return 0;
  }
}
