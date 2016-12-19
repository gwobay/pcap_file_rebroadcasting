#include <Data/QBDF_Util.h>
#include <Data/NYSE_Handler.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>

#include <MD/MDProvider.h>

#include <Logger/Logger_Manager.h>

#include <stdarg.h>

namespace hf_core {

  using namespace NYSE_structs;

  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  size_t expect_len = sizeof(message_header) + (header.num_body_entries * sizeof(type)); \
    if(len != expect_len) {                                             \
      send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(),  len, expect_len); \
      return len;                                                       \
    }                                                                   \


#define SET_SOURCE_TIME(obj)                                            \
  bool stale_msg __attribute__((unused)) = false;                       \
  {                                                                     \
    Time_Duration source_time = msec(ntohl(msg.source_time));           \
                                                                        \
    if(source_time.is_set()) {                                          \
      obj.set_time( m_midnight + source_time);                          \
      Time_Duration diff = send_time - source_time;                     \
      if(diff.abs() > m_max_send_source_difference) {                   \
        Product& prod = m_app->pm()->find_by_id(id);                    \
        send_alert("%s: %s send_time %s is %s lagged from source_time %s for symbol %s", \
                   where, m_name.c_str(), send_time.to_string().c_str(), \
                   diff.to_string().c_str(), source_time.to_string().c_str(), prod.symbol().c_str()); \
        stale_msg = true;                                               \
      }                                                                 \
    } else {                                                            \
      obj.set_time(Time::currentish_time());                            \
    }                                                                   \
  }

  size_t
  NYSE_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "NYSE_Handler::parse";

    if(len < sizeof(message_header)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd", where, m_name.c_str(), len, sizeof(message_header));
      return 0;
    }

    if(m_drop_packets) {
      --m_drop_packets;
      return len;
    }

    const message_header& header = reinterpret_cast<const message_header&>(*buf);

    if (ntohl(header.send_time) > (24 * Time_Constants::millis_per_hr)) {
      send_alert("%s: Send time > 24 hours; likely from prev day, skipping! ", where);
      return len;
    }

    if (m_qbdf_builder) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                             m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }

    channel_info& ci = m_channel_info_map[context];

    uint16_t msg_type = ntohs(header.msg_type);
    uint32_t seq_no = ntohl(header.seq_num);
    uint32_t next_seq_no = seq_no + 1;

    if(msg_type == 1) {
      // sequence number reset
      const sequence_reset_message& msg = reinterpret_cast<const sequence_reset_message&>(*(buf + sizeof(message_header)));
      uint32_t new_seq_no = ntohl(msg.new_seq_no);
      send_alert_forced("%s: %s received sequence reset message orig=%d  new=%d", where, m_name.c_str(), seq_no, new_seq_no);
      ci.seq_no = new_seq_no;
      ci.clear();
      this->parse2(context, buf, len);
      return len;
    }

    if(seq_no < ci.seq_no) {
      return len; // duplicate
    } else if(seq_no > ci.seq_no) {
      if(ci.begin_recovery(this, buf, len, seq_no, next_seq_no)) {
        return len;
      }
    }

    this->parse2(context, buf, len);

    ci.seq_no = next_seq_no;
    ci.last_update = Time::currentish_time();

    if(!ci.queue.empty()) {
      ci.process_recovery(this);
    }
    return len;
  }

  size_t
  NYSE_Handler::parse2(size_t context, const char* buf, size_t len)
  {
    const char* where = "NYSE_Handler::parse2";

    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
    }
    if(m_record_only) {
      return len;
    }

    const message_header& header = reinterpret_cast<const message_header&>(*buf);

    uint16_t msg_type = ntohs(header.msg_type);

    Time_Duration send_time = msec(ntohl(header.send_time));

    switch(msg_type) {
    case 140: // BBO
      {
        SIZE_CHECK(bbo_message);

        for(char i = 0; i < header.num_body_entries; ++i) {
          const bbo_message& msg = reinterpret_cast<const bbo_message&>(*(buf + sizeof(message_header) + (i*sizeof(bbo_message))));

          ProductID id = product_id_of_symbol(msg.symbol);
          if(id != Product::invalid_product_id) {
            m_quote.m_id = id;
            SET_SOURCE_TIME(m_quote);

            double psm = get_price_scale_mult(msg.price_scale_code);
            m_quote.m_bid = ntohl(msg.bid_price_numerator) * psm;
            m_quote.m_ask = ntohl(msg.ask_price_numerator) * psm;
            m_quote.m_bid_size = ntohl(msg.bid_size) * m_lot_size;
            m_quote.m_ask_size = ntohl(msg.ask_size) * m_lot_size;
            m_quote.m_cond = msg.quote_condition;

            if (m_qbdf_builder) {
              m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              MDPrice bid = get_int_price(msg.bid_price_numerator, msg.price_scale_code);
              MDPrice ask = get_int_price(msg.ask_price_numerator, msg.price_scale_code);
              uint32_t bid_size = (uint32_t)(ntohl(msg.bid_size) * m_lot_size);
              uint32_t ask_size = (uint32_t)(ntohl(msg.ask_size) * m_lot_size);
              gen_qbdf_quote_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                                    'N', msg.quote_condition, bid, ask, bid_size, ask_size);
            } else {
              send_quote_update('O');
            }
          }
        }
      }
      break;
    case 220: // Trades
      {
        SIZE_CHECK(trade_message);

        for(char i = 0; i < header.num_body_entries; ++i) {
          const trade_message& msg = reinterpret_cast<const trade_message&>(*(buf + sizeof(message_header) + (i*sizeof(trade_message))));

          ProductID id = product_id_of_symbol(msg.symbol);
          if(id != Product::invalid_product_id) {
            m_trade.m_id = id;
            SET_SOURCE_TIME(m_trade);

            double psm = get_price_scale_mult(msg.price_scale_code);
            m_trade.m_price = ntohl(msg.price_numerator) * psm;
            m_trade.m_size = ntohl(msg.volume); // * m_lot_size;
            m_trade.set_side(0);
            uint32_t link_id = ntohl(msg.link_id);
            if(m_linkid_queue) {
              deque<linkid_to_side>& q = (*m_linkid_queue)[id];
              while(!q.empty() && q.front().link_id < link_id) {
                q.pop_front();
              }
              if(!q.empty() && q.front().link_id == link_id) {
                m_trade.set_side(q.front().side);
              }
            }

            char side = '\0';
            if (m_read_link_records) {
              string symbol_str(msg.symbol);
              side = m_link_record_map["ticker"][symbol_str][link_id];
            }

            memcpy(m_trade.m_trade_qualifier, msg.trade_condition, sizeof(m_trade.m_trade_qualifier));

            if (m_qbdf_builder) {
              m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              MDPrice price = get_int_price(msg.price_numerator, msg.price_scale_code);
              uint32_t size = (uint32_t)(ntohl(msg.volume));
              gen_qbdf_trade_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                                    'N', side, '0', string(msg.trade_condition, 4), size, price);
            } else {
              send_trade_update();
            }

            if(stale_msg) {
              m_logger->log_printf(Log::INFO, "%s: stale Trade message %s %0.2f %4s", where,
                                   msg.symbol, m_trade.m_price, m_trade.m_trade_qualifier);
            }

          }
        }
      }
      break;
    case 230: // OpenBook Ultra Full Update
      {
        size_t byte_offset = 0;
        for(char i = 0; i < header.num_body_entries; ++i) {
          const full_update_message& msg = reinterpret_cast<const full_update_message&>(*(buf + sizeof(message_header) + byte_offset));
          uint16_t msg_size = ntohs(msg.msg_size);
          uint16_t security_index = ntohs(msg.security_index);
          ProductID id = product_id_of_symbol_index(security_index);
          if(id != Product::invalid_product_id) {
            m_quote.m_id = id;
            SET_SOURCE_TIME(m_quote);

            int num_price_points = (msg_size - sizeof(full_update_message)) / sizeof(full_price_point);

            bool inside_changed = false;
            for(int j = 0; j < num_price_points; ++j) {
              const full_price_point& fpp = reinterpret_cast<const full_price_point&>(*(buf + sizeof(message_header) + byte_offset +
                                                                                        sizeof(full_update_message) + j*sizeof(full_price_point)));
              MDPrice price = get_int_price(fpp.price_numerator, msg.price_scale_code);
              if(!price.blank()) {
                uint32_t shares = ntohl(fpp.volume);
                uint16_t num_orders = ntohs(fpp.num_orders);

                if(m_qbdf_builder) {
                  m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                  uint8_t last_in_group = (j == (num_price_points - 1)) ? 1 : 0;
                  gen_qbdf_level_iprice64(m_qbdf_builder, id,  m_micros_recv_time,
                                          m_micros_exch_time_delta, 'N', fpp.side,
                                          msg.trading_status, msg.quote_condition, ' ',
                                          num_orders, shares, price, last_in_group, false);
                }

                if(shares != 0) {
                  inside_changed |= m_order_book->add_level_ip(m_quote.time(), id, fpp.side, price, shares, num_orders);
                } else {
                  inside_changed |= m_order_book->remove_level_ip(m_quote.time(), id, fpp.side, price, false);
                }
              }
            }
            if(inside_changed) {
              m_quote.m_id = id;
              m_quote.m_cond = msg.quote_condition;
              m_order_book->fill_quote_update(id, m_quote);
              send_quote_update(msg.trading_status);
            }
          }
          byte_offset += msg_size;  // We have a varying number of price_points following the fixed fields
        }
      }
      break;
    case 231: // OpenBook Ultra Delta
      {
        size_t byte_offset = 0;
        for(char i = 0; i < header.num_body_entries; ++i) {
          const delta_update_message& msg = reinterpret_cast<const delta_update_message&>(*(buf + sizeof(message_header) + byte_offset));
          int msg_size = ntohs(msg.msg_size);
          uint16_t security_index = ntohs(msg.security_index);
          ProductID id = product_id_of_symbol_index(security_index);
          if(id != Product::invalid_product_id) {
            m_quote.m_id = id;
            SET_SOURCE_TIME(m_quote);

            int num_price_points = (msg_size - sizeof(delta_update_message)) / sizeof(delta_price_point);

            bool inside_changed = false;
            for(int j = 0; j < num_price_points; ++j) {
              const delta_price_point& dpp = reinterpret_cast<const delta_price_point&>(*(buf + sizeof(message_header) + byte_offset +
                                                                                          sizeof(delta_update_message) + j*sizeof(delta_price_point)));
              MDPrice price = get_int_price(dpp.price_numerator, msg.price_scale_code);
              if(!price.blank()) {
                uint32_t shares = ntohl(dpp.volume);
                uint16_t num_orders = ntohs(dpp.num_orders);
                char side_c = dpp.side == 'B' ? 'S' : 'B';
                bool executed = (dpp.linkid_1 != 0);

                if(executed && m_linkid_queue) {
                  deque<linkid_to_side>& q = (*m_linkid_queue)[id];
                  while(q.size() > m_max_linkid_queue_size) {
                    q.pop_front();
                  }
                  q.push_back(linkid_to_side(ntohl(dpp.linkid_1), side_c));
                  if(dpp.linkid_2) { q.push_back(linkid_to_side(ntohl(dpp.linkid_2), side_c)); }
                  if(dpp.linkid_3) { q.push_back(linkid_to_side(ntohl(dpp.linkid_3), side_c)); }
                }

                if (executed && m_write_link_records) {
                  ostringstream oss;
                  oss << ntohs(msg.security_index);

                  string ticker = m_openbook_tickers[security_index];
                  gen_qbdf_link(m_qbdf_builder, id, oss.str(), ticker, ntohl(dpp.linkid_1), side_c);
                  if (dpp.linkid_2) { gen_qbdf_link(m_qbdf_builder, id, oss.str(), ticker, ntohl(dpp.linkid_2), side_c); }
                  if (dpp.linkid_3) { gen_qbdf_link(m_qbdf_builder, id, oss.str(), ticker, ntohl(dpp.linkid_3), side_c); }
                }

                if(m_qbdf_builder) {
                  m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                  uint8_t last_in_group = (j == (num_price_points - 1)) ? 1 : 0;
                  gen_qbdf_level_iprice64(m_qbdf_builder, id,  m_micros_recv_time,
                                          m_micros_exch_time_delta, 'N', dpp.side,
                                          msg.trading_status, msg.quote_condition, dpp.reason_code,
                                          num_orders, shares, price, last_in_group, executed);
                }

                if(shares != 0) {
                  inside_changed |= m_order_book->modify_level_ip(m_quote.time(), id, dpp.side, price, shares, num_orders, executed);
                } else {
                  inside_changed |= m_order_book->remove_level_ip(m_quote.time(), id, dpp.side, price, executed);
                }

                /*
                if(executed) {
                  char timestamp[64];
                  Time_Utils::print_time(timestamp, m_quote.time(), Timestamp::MICRO);
                  m_logger->printf("%s %d %0.4f %d %d %c %c %u %u %u\n", timestamp, id, mdob_iprice_to_double(price), shares, ntohl(dpp.change_qty),
                                   dpp.side, dpp.reason_code, ntohl(dpp.linkid_1), ntohl(dpp.linkid_2), ntohl(dpp.linkid_3));
                }
                */
              }
            }
            if(inside_changed) {
              m_quote.m_id = id;
              m_quote.m_cond = msg.quote_condition;
              m_order_book->fill_quote_update(id, m_quote);
              send_quote_update(msg.trading_status);
            }
          }
          byte_offset += msg_size;  // We have a varying number of price_points following the fixed fields
        }
      }
      break;
    case 240: // opening imbalance
      {
        if (m_use_old_imbalance) {
          SIZE_CHECK(old_open_imbalance_message);

          for(char i = 0; i < header.num_body_entries; ++i) {
            const old_open_imbalance_message& msg = reinterpret_cast<const old_open_imbalance_message&>(*(buf + sizeof(message_header) + (i*sizeof(old_open_imbalance_message))));
            ProductID id = product_id_of_symbol(msg.symbol);
            if(id != Product::invalid_product_id) {
              m_auction.m_id = id;
              SET_SOURCE_TIME(m_auction);

              double psm = get_price_scale_mult(msg.price_scale_code);

              m_auction.m_ref_price =  ntohl(msg.reference_price_numerator) * psm;
              m_auction.m_far_price =  ntohl(msg.clearing_price_numerator) * psm;
              m_auction.m_near_price = ntohl(msg.clearing_price_numerator) * psm;
              m_auction.m_paired_size = ntohl(msg.paired_quantity);
              m_auction.m_imbalance_size = ntohl(msg.imbalance_quantity);
              m_auction.m_imbalance_direction = msg.imbalance_side;

              if(m_auction.m_imbalance_size == 0 || m_auction.m_imbalance_direction == ' ') {
                m_auction.m_imbalance_direction = 'N';
              }

              if (m_qbdf_builder) {
                m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
                m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                MDPrice ref_price = get_int_price(msg.reference_price_numerator, msg.price_scale_code);
                MDPrice far_price = get_int_price(msg.clearing_price_numerator, msg.price_scale_code);
                MDPrice near_price = get_int_price(msg.clearing_price_numerator, msg.price_scale_code);
                uint32_t paired_size = (uint32_t)(ntohl(msg.paired_quantity));
                uint32_t imbalance_size = (uint32_t)(ntohl(msg.imbalance_quantity));
                char imbalance_direction = msg.imbalance_side;

                if (imbalance_size == 0 || imbalance_direction == ' ') {
                  imbalance_direction = 'N';
                }

                gen_qbdf_imbalance_micros(m_qbdf_builder, id, m_micros_recv_time,
                                          m_micros_exch_time_delta, 'N', ref_price,
                                          near_price, far_price, paired_size,
                                          imbalance_size, imbalance_direction, ' ');
              }

              if(m_subscribers) {
                m_subscribers->update_subscribers(m_auction);
              }
            }
          }
        } else {
          SIZE_CHECK(open_imbalance_message);

          for(char i = 0; i < header.num_body_entries; ++i) {
            const open_imbalance_message& msg = reinterpret_cast<const open_imbalance_message&>(*(buf + sizeof(message_header) + (i*sizeof(open_imbalance_message))));
            ProductID id = product_id_of_symbol(msg.symbol);
            if(id != Product::invalid_product_id) {
              m_auction.m_id = id;
              SET_SOURCE_TIME(m_auction);

              double psm = get_price_scale_mult(msg.price_scale_code);

              m_auction.m_ref_price =  ntohl(msg.reference_price_numerator) * psm;
              m_auction.m_far_price =  ntohl(msg.clearing_price_numerator) * psm;
              m_auction.m_near_price = ntohl(msg.clearing_price_numerator) * psm;
              m_auction.m_paired_size = ntohl(msg.paired_quantity);
              m_auction.m_imbalance_size = ntohl(msg.imbalance_quantity);
              m_auction.m_imbalance_direction = msg.imbalance_side;

              if(m_auction.m_imbalance_size == 0 || m_auction.m_imbalance_direction == ' ') {
                m_auction.m_imbalance_direction = 'N';
              }

              if (m_qbdf_builder) {
                m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
                m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                MDPrice ref_price = get_int_price(msg.reference_price_numerator, msg.price_scale_code);
                MDPrice far_price = get_int_price(msg.clearing_price_numerator, msg.price_scale_code);
                MDPrice near_price = get_int_price(msg.clearing_price_numerator, msg.price_scale_code);
                uint32_t paired_size = (uint32_t)(ntohl(msg.paired_quantity));
                uint32_t imbalance_size = (uint32_t)(ntohl(msg.imbalance_quantity));
                char imbalance_direction = msg.imbalance_side;

                if (imbalance_size == 0 || imbalance_direction == ' ') {
                  imbalance_direction = 'N';
                }

                gen_qbdf_imbalance_micros(m_qbdf_builder, id, m_micros_recv_time,
                                          m_micros_exch_time_delta, 'N', ref_price,
                                          near_price, far_price, paired_size,
                                          imbalance_size, imbalance_direction, ' ');
              }

              if(m_subscribers) {
                m_subscribers->update_subscribers(m_auction);
              }
            }
          }
        }
      }
      break;
      case 241: // closing imbalance
      {
        SIZE_CHECK(close_imbalance_message);

        for(char i = 0; i < header.num_body_entries; ++i) {
          const close_imbalance_message& msg = reinterpret_cast<const close_imbalance_message&>(*(buf + sizeof(message_header) + (i*sizeof(close_imbalance_message))));
          ProductID id = product_id_of_symbol(msg.symbol);
          if(id != Product::invalid_product_id) {
            m_auction.m_id = id;
            SET_SOURCE_TIME(m_auction);

            double psm = get_price_scale_mult(msg.price_scale_code);

            m_auction.m_ref_price =  ntohl(msg.reference_price_numerator) * psm;
            m_auction.m_far_price =  ntohl(msg.closing_clearing_price_numerator) * psm;
            m_auction.m_near_price = ntohl(msg.continuous_clearing_price_numerator) * psm;
            m_auction.m_paired_size = ntohl(msg.paired_quantity);
            m_auction.m_imbalance_size = ntohl(msg.imbalance_quantity);
            m_auction.m_imbalance_direction = msg.imbalance_side;

            if(m_auction.m_imbalance_size == 0 || m_auction.m_imbalance_direction == ' ') {
              m_auction.m_imbalance_direction = 'N';
            }

            if (m_qbdf_builder) {
              m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              MDPrice ref_price = get_int_price(msg.reference_price_numerator, msg.price_scale_code);
              MDPrice far_price = get_int_price(msg.closing_clearing_price_numerator, msg.price_scale_code);
              MDPrice near_price = get_int_price(msg.continuous_clearing_price_numerator, msg.price_scale_code);
              uint32_t paired_size = (uint32_t)(ntohl(msg.paired_quantity));
              uint32_t imbalance_size = (uint32_t)(ntohl(msg.imbalance_quantity));
              char imbalance_direction = msg.imbalance_side;

              if (imbalance_size == 0 || imbalance_direction == ' ') {
                imbalance_direction = 'N';
              }

              gen_qbdf_imbalance_micros(m_qbdf_builder, id, m_micros_recv_time,
                                        m_micros_exch_time_delta, 'N', ref_price,
                                        near_price, far_price, paired_size,
                                        imbalance_size, imbalance_direction, ' ');
	    }

            if(m_subscribers) {
              m_subscribers->update_subscribers(m_auction);
            }
          }
        }
      }
      break;
    case 121 : // delay halt
      {
        SIZE_CHECK(delay_halt_message);

        for(char i = 0; i < header.num_body_entries; ++i) {
          const delay_halt_message& msg = reinterpret_cast<const delay_halt_message&>(*(buf + sizeof(message_header) + (i*sizeof(delay_halt_message))));
          ProductID id = product_id_of_symbol(msg.symbol);
          if(id != Product::invalid_product_id) { 
            m_msu.m_id = id;

            bool send_update = true;
            switch(msg.security_status) {
            case 3:
              m_qbdf_status = QBDFStatusType::OpeningDelay;
              send_update = false;
              break;
            case 4:
              m_msu.m_market_status =  MarketStatus::Halted;
              m_qbdf_status = QBDFStatusType::Halted;
              break;
            case 5:
              m_msu.m_market_status =  MarketStatus::Open;
              m_qbdf_status = QBDFStatusType::Resume;
              break;
            case 6:
              m_qbdf_status = QBDFStatusType::NoOpenNoResume;
              send_update = false;
              break;
            default:
              send_update = false;
              break;
            }

            if (m_qbdf_builder) {
              m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              m_qbdf_status_reason = string("NYSE Halt Cond ") + msg.halt_condition;
              gen_qbdf_status_micros(m_qbdf_builder, id,  m_micros_recv_time,
                                     m_micros_exch_time_delta, 'N', m_qbdf_status.index(),
                                     m_qbdf_status_reason.c_str());
            }

            if(send_update) {
              switch(msg.halt_condition) {
              case 'A': m_msu.m_reason = "As of Update"; break;
              case 'D': m_msu.m_reason = "News dissemination"; break;
              case 'E': m_msu.m_reason = "Order influx"; break;
              case 'I': m_msu.m_reason = "Order imbalance"; break;
              case 'P': m_msu.m_reason = "News pending"; break;
              case 'J': m_msu.m_reason = "Due to related security - news dissemination"; break;
              case 'K': m_msu.m_reason = "Due to related security - news pending"; break;
              case 'M': m_msu.m_reason = "Volatility Trading Pause"; break;
              case 'Q': m_msu.m_reason = "Due to related security"; break;
              case 'T': m_msu.m_reason = "Resume"; break;
              case 'V': m_msu.m_reason = "In view of common"; break;
              case 'X': m_msu.m_reason = "Equiptment changeover"; break;
              case 'Y': m_msu.m_reason = "Sub penny Trading"; break;
              case 'Z': m_msu.m_reason = "No open/No resume"; break;
              case '~': m_msu.m_reason = "Not delayed/halted"; break;
              default:
                break;
              }
              m_security_status[m_msu.m_id] = m_msu.m_market_status;
              if(m_subscribers) {
                m_subscribers->update_subscribers(m_msu);
              }
              m_msu.m_reason.clear();
            }
          }
        }
      }
      break;
    case 124 : // circuit breaker message
      {
        for(char i = 0; i < header.num_body_entries; ++i) {
          const circuit_breaker_message& msg = reinterpret_cast<const circuit_breaker_message&>(*(buf + sizeof(message_header) + (i*sizeof(circuit_breaker_message))));

          const char* status;
          switch(msg.status) {
          case '0': status = "Circuit Breakers are not currently in effect"; break;
          case '1': status = "Circuit Breakers are now in effect and Trading has been halted.  Trading will resume in 1/2 hour."; break;
          case '2': status = "Circuit Breakers are now in effect and Trading has been halted.  Trading will resume in 1 hour."; break;
          case '3': status = "Circuit Breakers are now in effect and Trading has been halted.  Trading will resume in 2 hours."; break;
          case '4': status = "Circuit Breakers are now in effect and Trading has been halted.  Trading will not resume today."; break;
          default:
            status = "Unknown!";
            break;
          }

          m_last_alert_time.clear();
          send_alert("NYSE Circuit Breaker Message: %d %s %128s", msg.status, status, msg.url);
        }
      }
      break;
    case 125 : // short sale restriction
      {
        SIZE_CHECK(short_sale_restriction_message);
        for(char i = 0; i < header.num_body_entries; ++i) {
          const short_sale_restriction_message& msg = reinterpret_cast<const short_sale_restriction_message&>(*(buf + sizeof(message_header) + (i*sizeof(short_sale_restriction_message))));

          SSRestriction rest(SSRestriction::Unknown);
          ProductID id = product_id_of_symbol(msg.symbol);
          if(id != Product::invalid_product_id) {

            const char* status;
            switch(msg.short_sale_restriction_indicator) {
            case 'A':
              status = "Short Sale Restriction Activated";
              rest = SSRestriction::Activated;
              m_qbdf_status = QBDFStatusType::ShortSaleRestrictActivated;
              break;
            case 'C':
              status = "Short Sale Restriction Continued";
              rest = SSRestriction::Continued;
              m_qbdf_status = QBDFStatusType::ShortSaleRestrictContinued;
              break;
            case 'D':
              status = "Short Sale Restriction Deactivated";
              rest = SSRestriction::None;
              m_qbdf_status = QBDFStatusType::NoShortSaleRestrictionInEffect;
              break;
            case '~':
              status = "No Short Sale in Effect";
              rest = SSRestriction::None;
              m_qbdf_status = QBDFStatusType::NoShortSaleRestrictionInEffect;
              break;
            case 'E':
              status = "Short Sale Restriction in Effect";
              rest = SSRestriction::Continued;
              m_qbdf_status = QBDFStatusType::ShortSaleRestrictContinued;
              break;
            default:
              status = "Unknown!";
              break;
            }

            if (m_qbdf_builder) {
              m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              m_qbdf_status_reason = string("NYSE SSR Indicator ") + msg.short_sale_restriction_indicator;
              gen_qbdf_status_micros(m_qbdf_builder, id,  m_micros_recv_time,
                                     m_micros_exch_time_delta, 'N', m_qbdf_status.index(),
                                     m_qbdf_status_reason.c_str());
            }

            send_alert(AlertSeverity::LOW, "NYSE ALERTS SSR symbol=%.11s %c %s", msg.symbol, msg.short_sale_restriction_indicator, status);
          }

	  if (rest != SSRestriction::Unknown) {
	    m_msu.m_id = id;
	    m_msu.m_ssr=rest;
	    m_msu.m_market_status = m_security_status[id];
	    if(m_subscribers) {
	      m_subscribers->update_subscribers(m_msu);
	    }
	    m_msu.m_ssr=SSRestriction::Unknown;
	  }
        }
      }
      break;
    case 126:
      {
        SIZE_CHECK(retail_price_improvement);
        for(char i = 0; i < header.num_body_entries; ++i) {
          const retail_price_improvement& msg = reinterpret_cast<const retail_price_improvement&>(*(buf + sizeof(message_header) + (i*sizeof(retail_price_improvement))));
          //const char* status = "UNKNOWN";

          switch(msg.rpi_indicator) {
          case '0': /*status = "neither bid or ask (default)"; */break;
          case '1': /*status = "bid"; */break;
          case '2': /*status = "ask"; */break;
          case '3': /*status = "bid and ask"; */break;
          default: break;
          }

          //send_alert("NYSE ALERTS RPI symbol=%.11s %c %s", msg.symbol, msg.rpi_indicator, status);
        }
      }
      break;
    default:
      break;
    }

    return len;
  }


  size_t
  NYSE_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len)
  {
    const message_header& header = reinterpret_cast<const message_header&>(*buf);

    uint16_t msg_type = ntohs(header.msg_type);
    Time_Duration send_time = msec(ntohl(header.send_time));

    char time_buf[32];
    Time_Utils::print_time(time_buf, send_time, Timestamp::MILI);

    log.printf(" msg_type:%u send_time:%s seq_num:%u entries:%u\n", msg_type, time_buf, ntohl(header.seq_num), header.num_body_entries);

    const char* msg_buf = buf + sizeof(message_header);

    switch(msg_type) {
    case 121: // delay halt
      {
        for(char i = 0; i < header.num_body_entries; ++i) {
          const delay_halt_message& msg = reinterpret_cast<const delay_halt_message&>(*(buf + sizeof(message_header) + (i*sizeof(delay_halt_message))));
          Time_Duration source_time = msec(ntohl(msg.source_time));
          Time_Utils::print_time(time_buf, source_time, Timestamp::MILI);
	  
	  log.printf("  halt source_time:%s symbol:%.16s security_status:%d halt_condition:%c\n", time_buf, msg.symbol, msg.security_status, msg.halt_condition);
	}
      }
      break;
    case 125: // ssr
      {
        for(char i = 0; i < header.num_body_entries; ++i) {
          const short_sale_restriction_message& msg = reinterpret_cast<const short_sale_restriction_message&>(*(buf + sizeof(message_header) + (i*sizeof(short_sale_restriction_message))));
          Time_Duration source_time = msec(ntohl(msg.source_time));
          Time_Utils::print_time(time_buf, source_time, Timestamp::MILI);
	  
	  log.printf("  ssr source_time:%s symbol:%.16s indicator:%c\n", time_buf, msg.symbol, msg.short_sale_restriction_indicator);
	}
      }
      break;
    case 220: // Trade
      {
        for(char i = 0; i < header.num_body_entries; ++i) {
          const trade_message& msg = reinterpret_cast<const trade_message&>(*(msg_buf + (i*sizeof(trade_message))));
          double psm = get_price_scale_mult(msg.price_scale_code);
          double price = ntohl(msg.price_numerator) * psm;
          uint32_t volume = ntohl(msg.volume);
          Time_Duration source_time = msec(ntohl(msg.source_time));
          Time_Utils::print_time(time_buf, source_time, Timestamp::MILI);

          log.printf("  trade source_time:%s symbol:%.16s price:%0.4f volume:%u price_num:%u source_seq_num:%lu source_sess_id:%u price_scale_code:%u exchange_id:%c security_type:%c trade_condition:%.4s quote_link_id:%lu\n",
                     time_buf, msg.symbol, price, volume,
                     ntohl(msg.price_numerator), ntohll(msg.source_seq_num), msg.source_session_id, msg.price_scale_code, msg.exchange_id, msg.security_type, msg.trade_condition, ntohll(msg.link_id));
        }
      }
      break;
    case 230:
      {
        size_t byte_offset = 0;
        for(char i = 0; i < header.num_body_entries; ++i) {
          const full_update_message& msg = reinterpret_cast<const full_update_message&>(*(msg_buf + byte_offset));
          uint16_t msg_size = ntohs(msg.msg_size);
          uint16_t security_index = ntohs(msg.security_index);

          Time_Duration source_time = msec(ntohl(msg.source_time));
          source_time += usec(ntohs(msg.source_time_micro));
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);

          int num_price_points = (msg_size - sizeof(full_update_message)) / sizeof(full_price_point);

          log.printf("  full_update source_time:%s symbol:%.11s security_index:%u quote_condition:%c trading_status:%c num_price_points:%u\n", time_buf, msg.symbol, security_index, msg.quote_condition, msg.trading_status, num_price_points);

          for(int j = 0; j < num_price_points; ++j) {
            const full_price_point& fpp = reinterpret_cast<const full_price_point&>(*(msg_buf + byte_offset +
                                                                                      sizeof(full_update_message) + j*sizeof(full_price_point)));
            MDPrice price = get_int_price(fpp.price_numerator, msg.price_scale_code);
            uint32_t shares = ntohl(fpp.volume);
            uint16_t num_orders = ntohs(fpp.num_orders);
            log.printf("    fpp price:%0.4f side:%c shares:%u orders:%u\n", price.fprice(), fpp.side, shares, num_orders);
          }
          byte_offset += msg_size;  // We have a varying number of price_points following the fixed fields
        }
      }
      break;
    case 231:
      {
        size_t byte_offset = 0;
        for(char i = 0; i < header.num_body_entries; ++i) {
          const delta_update_message& msg = reinterpret_cast<const delta_update_message&>(*(msg_buf + byte_offset));
          int msg_size = ntohs(msg.msg_size);
          uint16_t security_index = ntohs(msg.security_index);

          Time_Duration source_time = msec(ntohl(msg.source_time));
          source_time += usec(ntohs(msg.source_time_micro));
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);

          int num_price_points = (msg_size - sizeof(delta_update_message)) / sizeof(delta_price_point);

          log.printf("  delta_update source_time:%s security_index:%u quote_condition:%c trading_status:%c num_price_points:%u\n", time_buf, security_index, msg.quote_condition, msg.trading_status, num_price_points);

          for(int j = 0; j < num_price_points; ++j) {
              const delta_price_point& dpp = reinterpret_cast<const delta_price_point&>(*(msg_buf + byte_offset +
                                                                                          sizeof(delta_update_message) + j*sizeof(delta_price_point)));
              MDPrice price = get_int_price(dpp.price_numerator, msg.price_scale_code);
              uint32_t shares = ntohl(dpp.volume);
              uint16_t num_orders = ntohs(dpp.num_orders);
              uint32_t linkid_1 = ntohl(dpp.linkid_1);
              uint32_t linkid_2 = ntohl(dpp.linkid_2);
              uint32_t linkid_3 = ntohl(dpp.linkid_3);
              log.printf("    dpp price:%0.4f side:%c shares:%u orders:%u reason:%c linkid_1:%u linkid_2:%u linkid_3:%u\n", price.fprice(), dpp.side, shares, num_orders, dpp.reason_code, linkid_1, linkid_2, linkid_3);
          }
          byte_offset += msg_size;  // We have a varying number of price_points following the fixed fields
        }
      }
      break;
     case 240: // opening imbalance
       {
         for(char i = 0; i < header.num_body_entries; ++i) {
           const open_imbalance_message& msg = reinterpret_cast<const open_imbalance_message&>(*(buf + sizeof(message_header) + (i*sizeof(open_imbalance_message))));
           
           double psm = get_price_scale_mult(msg.price_scale_code);
           
           double ref_price = ntohl(msg.reference_price_numerator) * psm;
           double clearing_price = ntohl(msg.clearing_price_numerator) * psm;
           
           log.printf("   open_imbalance  source_time:%s  symbol:%.11s  side:%c  imbalance_size:%d  paired_size:%d  ref_price:%0.4f  near_price:%0.4f\n",
                      time_buf, msg.symbol, msg.imbalance_side, ntohl(msg.imbalance_quantity), ntohl(msg.paired_quantity), ref_price, clearing_price);
           
         }
       }
       break;

     }

    return packet_len;
  }


  void
  NYSE_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);

    m_name = name;

    m_write_link_records = params.getDefaultBool("write_link_records", false);
    if (params.has("read_link_records"))
      m_read_link_records = true;
    
    m_source_id = params.getDefaultString("source_id", "");

    m_midnight = Time::today();

    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;

    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';

    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);

    m_lot_size = params.getDefaultInt("lot_size", m_lot_size);

    m_max_linkid_queue_size = params.getDefaultInt("max_linkid_queue_size", 5000);

    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);

    m_max_send_source_difference = params.getDefaultTimeDuration("max_send_source_difference", m_max_send_source_difference);
    
    if(params.has("nyse_openbook")) {
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_books.push_back(m_order_book);
      m_order_book->init(params);
    }
    
    if(params.getDefaultBool("use_old_imbalance", false)) {
      m_use_old_imbalance = true;
    } else {
      m_use_old_imbalance = false;
    }
  }

  void
  NYSE_Handler::start()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }

    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  NYSE_Handler::stop()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }

    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  NYSE_Handler::send_retransmit_request(channel_info& cib)
  {
    channel_info& ci = static_cast<channel_info&>(cib);

    if(ci.recovery_socket == -1) {
      ci.has_recovery = false;
      m_logger->log_printf(Log::INFO, "%s sent retransmit request called but socket =- -1", ci.name.c_str());
      return;
    }

    uint32_t begin = ci.seq_no;
    uint32_t end = ci.queue.top().seq_num;

    retransmission_request_message req;
    memset(&req, 0, sizeof(retransmission_request_message));
    req.header.msg_size = htons(42);
    req.header.msg_type = htons(20);
    req.header.seq_num = 0;
    req.header.product_id =  htons(m_nyse_product_id);
    req.header.retrans_flag = 1;
    req.header.num_body_entries = 1;
    req.header.link_flag = 0;
    req.begin_seq_num = htonl(begin);
    req.end_seq_num = htonl(end);
    strncpy(req.source_id, m_source_id.c_str(), 20);

    int ret = 0;
    m_logger->log_printf(Log::INFO, "%s sent retransmit request via TCP %d to %d", ci.name.c_str(), begin, end);
    ret = ::send(ci.recovery_socket, &req, sizeof(req), MSG_DONTWAIT);

    if(ret < 0) {
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert("NYSE_Handler::send_retransmit_request %s sendto returned %d - %s", m_name.c_str(), errno, err_msg);
    }

    ci.sent_resend_request = true;
  }

  void
  NYSE_Handler::send_heartbeat_response(channel_info& ci)
  {
    if(!ci.recovery_socket.connected()) {
      return;
    }
    
    heartbeat_response_message resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.msg_size     = htons(34);
    resp.header.msg_type     = htons(24);
    resp.header.seq_num      = htonl(1);
    resp.header.product_id   = htons(m_nyse_product_id);
    resp.header.retrans_flag = 1;
    resp.header.num_body_entries = 1;
    strncpy(resp.source_id, m_source_id.c_str(), 20);

    if(ci.recovery_socket.connected()) {
      if(send(ci.recovery_socket, &resp, sizeof(resp), 0) < 0) {
        ci.recovery_socket.close();
        send_alert("%s error sending heartbeat response, closing socket", m_name.c_str());
      }
    }
  }

  size_t
  NYSE_Handler::read(size_t context, const char* buf, size_t len)
  {
    if(len < 2) {
      return 0;
    }

    const char* where = "NYSE_Handler::handle_retran_msg";
    const message_header& header = reinterpret_cast<const message_header&>(*buf);

    uint16_t msg_size = 2 + ntohs(header.msg_size);
    if(len < msg_size) {
      return 0;
    }

    uint16_t msg_type = ntohs(header.msg_type);

    channel_info& ci = m_channel_info_map[context];

    //m_logger->log_printf(Log::INFO, "%s: got msg %d", where, msg_type);

    switch(msg_type) {
    case 2 : // heartbeat, we must send heartbeat response
      {
        send_heartbeat_response(ci);
      }
      break;
    case 5: // message unavailable
      {
        const message_unavailable& msg = reinterpret_cast<const message_unavailable&>(*(buf + sizeof(message_header)));
        uint32_t b = ntohl(msg.begin_seq_num);
        uint32_t e = ntohl(msg.end_seq_num);
        m_logger->log_printf(Log::WARN, "%s: %s received message_unavailable for seq_no %d to %d", where, m_name.c_str(), b, e);
      }
      break;
    case 10: // retransmission response
      {
        const retransmission_response_message& msg = reinterpret_cast<const retransmission_response_message&>(*(buf + sizeof(message_header)));
        if(msg.status == 'R') {
          const char* text;
          switch(msg.reject_reason) {
          case 0:
          case '0' : text = "Message was accepted."; break;
          case 1:
          case '1' : text = "Rejected due to permissions"; break;
          case 2:
          case '2' : text = "Rejected due to invalid sequence range"; break;
          case 3:
          case '3' : text = "Rejected due to maximum sequence range (>1000)"; break;
          case 4:    text = "Rejected due to maximum request in a day"; break;
          case 5:    text = "Rejected due to a maximum number of refresh requests in a day"; break;
          default  : text = "Unknown reason code"; break;
          }
          m_logger->log_printf(Log::WARN, "%s: %s received retransmission rejected source_id='%s' response %c - %s", where, m_name.c_str(),
                               msg.source_id, msg.reject_reason, text);
          ci.recovery_socket.close();
        } else {
          m_logger->log_printf(Log::DEBUG, "%s: %s received retransmission accepted", where, m_name.c_str());
        }
      }
      break;
    default:
      m_logger->log_printf(Log::WARN,  "%s: %s read unknown message type %d", where, m_name.c_str(), msg_type);
    }

    return msg_size;
  }

  void
  NYSE_Handler::add_context_product_id(size_t context, ProductID id)
  {
    if(m_context_to_product_ids.size() <= context) {
      m_context_to_product_ids.resize(context+1);
    }
    m_context_to_product_ids[context].push_back(id);
  }

  void
  NYSE_Handler::subscribe_product(ProductID id, ExchangeID exch,
                                  const string& mdSymbol, const string& mdExch)
  {
    mutex_t::scoped_lock lock(m_mutex);

    char* end = 0;
    uint16_t idx = strtol(mdSymbol.c_str(), &end, 10);

    if(idx != 0 && end && *end == '\0') {
      m_security_index_to_product_id[idx] = id;
    } else {
      pair<set<string>::iterator, bool> s_i = m_symbols.insert(mdSymbol.c_str());
      const char* symbol = s_i.first->c_str();
      m_symbol_to_product_id[symbol] = id;
    }
  }

  void
  NYSE_Handler::reset(size_t context, const char* msg)
  {
    if(m_context_to_product_ids.empty() || context >= m_context_to_product_ids.size()) {
      reset(msg);
      return;
    }

    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    const vector<ProductID>& clear_products = m_context_to_product_ids[context];

    if(m_subscribers) {
      for(vector<ProductID>::const_iterator p = clear_products.begin(), p_end = clear_products.end();
          p != p_end; ++p) {
        m_subscribers->post_invalid_quote(*p, m_quote.m_exch);
      }
    }

    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, 'N', context);
      }

      for(vector<ProductID>::const_iterator p = clear_products.begin(), p_end = clear_products.end();
          p != p_end; ++p) {
        m_order_book->clear(*p, false);
      }
    }
    m_logger->log_printf(Log::WARN, "NYSE_Handler::reset %s called with %zd products", m_name.c_str(), clear_products.size());
  }

  void
  NYSE_Handler::reset(const char* msg)
  {
    if(m_order_book) {
      if (m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, 'N', -1);
      }

      m_order_book->clear(false);
    }
    m_logger->log_printf(Log::WARN, "NYSE_Handler::reset called %s -- %s", m_name.c_str(), msg);
  }


  ProductID
  NYSE_Handler::product_id_of_symbol(const char* symbol)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol, 8);
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string);
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol [%s]",
                             "NYSE_Handler::product_id_of_symbol", symbol_string.c_str());
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      return symbol_id;
    }

    symbol_to_product_id_t::const_iterator prod_id = m_symbol_to_product_id.find(symbol);
    if(prod_id != m_symbol_to_product_id.end()) {
      return prod_id->second;
    } else {
      return Product::invalid_product_id;
    }
  }

  ProductID
  NYSE_Handler::product_id_of_symbol_index(uint16_t symbol_index)
  {
    if (m_qbdf_builder) {
      stringstream ss;
      ss << symbol_index;
      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(ss.str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol index [%d]",
                             "NYSE_Handler::product_id_of_symbol", symbol_index);
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      return symbol_id;
    }

    return m_security_index_to_product_id[symbol_index];
  }

  void
  NYSE_Handler::send_quote_update(char trading_status)
  {
    m_msu.m_market_status = m_security_status[m_quote.m_id];
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);

    if(trading_status != 'O') {
      m_quote.m_quote_status = QuoteStatus::Auction;
    }

    if(m_msu.m_market_status != m_security_status[m_quote.m_id]) {
      m_msu.m_id = m_quote.m_id;
      m_msu.m_exch = m_quote.m_exch;
      m_security_status[m_quote.m_id] = m_msu.m_market_status;
      if(m_subscribers) {
        m_subscribers->update_subscribers(m_msu);
      }
    }
    if(m_provider) {
      m_provider->check_quote(m_quote);
    }
    update_subscribers(m_quote);
  }

  void
  NYSE_Handler::send_trade_update()
  {
    m_msu.m_market_status = m_security_status[m_trade.m_id];
    m_feed_rules->apply_trade_conditions(m_trade, m_msu);
    if(m_msu.m_market_status != m_security_status[m_trade.m_id]) {
      m_msu.m_id = m_trade.m_id;
      m_security_status[m_trade.m_id] = m_msu.m_market_status;
      if(m_subscribers) {
        m_subscribers->update_subscribers(m_msu);
      }
    }
    if(m_provider) {
      m_provider->check_trade(m_trade);
    }
    update_subscribers(m_trade);
  }

  void
  NYSE_Handler::set_linkid_source(NYSE_Handler* nh)
  {
    if(!nh->m_linkid_queue) {
      nh->m_linkid_queue.reset(new vector< deque< linkid_to_side > >);
      nh->m_linkid_queue->resize(m_app->pm()->max_product_id());
    }

    m_linkid_queue = nh->m_linkid_queue;
  }

  NYSE_Handler::NYSE_Handler(Application* app) :
    Handler(app, "NYSE_Handler"),
    m_max_send_source_difference(msec(10000)),
    m_lot_size(100),
    m_nyse_product_id(115),
    m_read_link_records(false),
    m_write_link_records(false)
  {
    m_security_index_to_product_id.resize(65536, -1);
  }

  NYSE_Handler::~NYSE_Handler()
  {
  }



}
