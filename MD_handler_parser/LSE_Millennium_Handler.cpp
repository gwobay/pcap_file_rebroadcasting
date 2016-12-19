#include <Data/QBDF_Util.h>
#include <Data/LSE_Millennium_Handler.h>
#include <Util/Network_Utils.h>

#include <Exchange/Exchange_Manager.h>
#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <MD/MDProvider.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>

namespace hf_core {

  namespace lse_millennium {

    struct login_request {
      uint8_t  length;
      char     message_type;
      char     username[6];
      char     password[10];
    } __attribute__ ((packed));

    struct replay_request {
      uint8_t  length;
      char     message_type;
      char     market_data_group;
      uint32_t first_message;
      uint16_t count;
    } __attribute__ ((packed));

    struct snapshot_request {
      uint8_t  length;
      char     message_type;
      uint32_t sequence_number;
      char     segment[6];
      uint32_t instrument_id;
    } __attribute__ ((packed));

    struct logout_request {
      uint8_t  length;
      char     message_type;
    } __attribute__ ((packed));

    struct login_resonse {
      uint8_t  length;
      char     message_type;
      char     status;
    } __attribute__ ((packed));

    struct replay_response {
      uint8_t  length;
      char     message_type;
      char     market_data_group;
      uint32_t first_message;
      uint16_t count;
      char     status;
    } __attribute__ ((packed));

    struct snapshot_response {
      uint8_t  length;
      char     message_type;
      uint32_t sequence_number;
      uint32_t order_count;
      char     status;
    } __attribute__ ((packed));

    struct snapshot_complete {
      uint8_t  length;
      char     message_type;
      uint32_t sequence_number;
      char     segment[6];
      uint32_t instrument_id;
      char     flags;
    } __attribute__ ((packed));

    struct message_header {
      uint16_t hdr_length;
      uint8_t  hdr_count;
      uint8_t  hdr_unit;
      uint32_t hdr_sequence;
    } __attribute__ ((packed));

    struct timestamp_message {
      uint8_t  length;
      char     message_type;
      uint32_t seconds;
    } __attribute__ ((packed));

    struct system_event {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      char     event_code;
    } __attribute__ ((packed));

    struct symbol_directory {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint32_t instrument_id;
      char     reserved[2];
      char     status;
      char     isin[12];
      char     sedol[12];
      char     segment[6];
      char     underlying[6];
      char     currency[3];
      char     reserved2;
      char     reserved3[4];
      uint64_t previous_close_price;
    } __attribute__ ((packed));

    struct symbol_status {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint32_t instrument_id;
      char     reserved[2];
      char     trading_status;
      uint8_t  flags;
      char     halt_reason[4];
      uint8_t  session_change_reason;
      char     new_end_time[8];
      uint8_t  book_type;
    } __attribute__ ((packed));

    struct add_order {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      uint32_t instrument_id;
      char     reserved[2];
      uint64_t price;
      char     flags;
    } __attribute__ ((packed));

    struct add_attributed_order {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      uint32_t instrument_id;
      char     reserved[2];
      uint64_t price;
      char     attribution[11];
      char     flags;
    } __attribute__ ((packed));

    struct order_deleted {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     flags;
      uint32_t instrument_id;
    } __attribute__ ((packed));

    struct order_modified {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t new_quantity;
      uint64_t new_price;
      char     flags;
    } __attribute__ ((packed));

    struct order_book_clear {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint32_t instrument_id;
      char     reserved[2];
      char     flags;
    } __attribute__ ((packed));


    struct order_executed {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t executed_shares;
      uint64_t match_number;
    } __attribute__ ((packed));

    struct order_executed_with_price {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t executed_shares;
      uint32_t remaining_shares;
      uint64_t match_number;
      char     printable;
      uint64_t execution_price;
    } __attribute__ ((packed));

    struct trade {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint32_t executed;
      uint32_t instrument_id;
      char     reserved[2];
      uint64_t price;
      uint64_t match_number;
      char     cross_type;
    } __attribute__ ((packed));

    struct auction_trade {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint32_t executed;
      uint32_t instrument_id;
      char     reserved[2];
      uint64_t price;
      uint64_t match_number;
      char     auction_type;
    } __attribute__ ((packed));

    struct off_book_trade {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint32_t executed;
      uint32_t instrument_id;
      char     reserved[2];
      uint64_t price;
      uint64_t match_number;
      char     trade_type[4];
      char     trade_time[8];
      char     trade_date[8];
      char     traded_currency[4];
      uint64_t original_price;
      char     execution_venue[5];
      char     flags;
    } __attribute__ ((packed));

    struct broken_trade_message {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t match_number;
      char     trade_type;
      uint32_t instrument_id;
    } __attribute__ ((packed));

    struct auction_info {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint32_t paired_quantity;
      uint32_t reserved;
      char     reserved2;
      uint32_t instrument_id;
      char     reserved3[2];
      uint64_t indicative_price;
      char     auction_type;
    } __attribute__ ((packed));

    struct statistics {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint32_t instrument_id;
      char     reserved[2];
      char     statistic_type;
      uint64_t price;
      char     open_close_price_indicator;
      char     flags;
    } __attribute__ ((packed));

    struct top_of_book {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint32_t instrument_id;
      uint64_t buy_limit_price;
      uint32_t buy_limit_size;
      uint64_t sell_limit_price;
      uint32_t sell_limit_size;
    } __attribute__ ((packed));

  }
  
  
  ProductID
  LSE_Millennium_Handler::product_id_of_symbol(uint32_t instrument_id)
  {
    if (m_qbdf_builder) {
      stringstream ss;
      ss << instrument_id;

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(ss.str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
                             "LSE_Millennium_Handler::product_id_of_symbol", ss.str().c_str());
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      return symbol_id;
    }

    hash_map<uint32_t, ProductID>::iterator i = m_prod_map.find(instrument_id);
    if(i == m_prod_map.end()) {
      return Product::invalid_product_id;
    } else {
      return i->second;
    }
  }


  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  if(len != sizeof(type)) {                                             \
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), len, sizeof(type)); \
    continue;                                                           \
  }                                                                     \

  size_t
  LSE_Millennium_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "LSE_Millennium_Handler::parse";

    if(len < sizeof(lse_millennium::message_header)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd", where, m_name.c_str(), len, sizeof(lse_millennium::message_header));
      return 0;
    }

    mutex_t::scoped_lock lock(m_mutex);
    
    if (m_qbdf_builder) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                             m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }
    
    const lse_millennium::message_header& header = reinterpret_cast<const lse_millennium::message_header&>(*buf);
    
    if(header.hdr_unit >= m_channel_info_map.size()) {
      m_logger->log_printf(Log::INFO, "%s: %s increasing channel_info_map due to new unit %d", where, m_name.c_str(), header.hdr_unit);    
      while(header.hdr_unit >= m_channel_info_map.size()) {
        m_channel_info_map.push_back(channel_info("", m_channel_info_map.size()));
      }
    }
    
    if(m_drop_packets) {
      --m_drop_packets;
      return 0;
    }
    
    channel_info& ci = m_channel_info_map[header.hdr_unit];

    uint32_t seq_no = header.hdr_sequence;
    uint32_t next_seq_no = seq_no + header.hdr_count;

    if(seq_no == 0) {
      parse2(header.hdr_unit, buf, len);
      return len;
    }

    if(seq_no != ci.seq_no) {
      if(seq_no < ci.seq_no) {
        // duplicate
        return len;
      } else {
        if(ci.begin_recovery(this, buf, len, seq_no, next_seq_no)) {
          return len;
        }
      }
    }

    parse2(header.hdr_unit, buf, len);
    ci.seq_no = next_seq_no;
    ci.last_update = Time::currentish_time();

    if(!ci.queue.empty()) {
      ci.process_recovery(this);
    }

    return len;
  }

#define SEEN()                                                  \
  if(!m_seen_product.test(id)) {                                \
    m_seen_product.set(id);                                     \
    m_channel_info_map[context].clear_products.push_back(id);   \
  }

  const uint64_t MARKET_BID =       99999999900000000ULL;

  void
  LSE_Millennium_Handler::add_order(size_t context, Time t, uint64_t order_id, ProductID id, char side,
                                    MDPrice price, uint32_t shares)
  {
    SEEN();
    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                  m_micros_exch_time_delta, 'L', side, shares, price);
      }

      bool inside_changed = m_order_book->add_order_ip(t, order_id, id, side, price, shares);

      if(inside_changed) {
        m_quote.set_time(t);
        m_quote.m_id = id;
        send_quote_update();
      }
    }
  }

  void
  LSE_Millennium_Handler::modify_order(size_t context, Time t, uint64_t order_id,
                                       uint32_t shares, MDPrice price)
  {
    if(m_order_book) {
      ProductID id = Product::invalid_product_id;
      bool inside_changed = false;
      {
        MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
        MDOrder* order = m_order_book->unlocked_find_order(order_id);
        if(order) {
          id = order->product_id;

          if(price.blank() && order->side() == 'B') {
            price.set_iprice64(MARKET_BID);
          }
          MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
          inside_changed = m_order_book->unlocked_modify_order_ip(t, order, order->side(),
                                                                  price, shares);
          SEEN();

          if(m_qbdf_builder) {
            gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                         m_micros_recv_time, m_micros_exch_time_delta, 'L',
                                         order->side(), shares, price);
          }
        }
      }

      if(inside_changed) {
        m_quote.set_time(t);
        m_quote.m_id = id;
        send_quote_update();
      }
    }
  }


  size_t
  LSE_Millennium_Handler::parse2(size_t context, const char* packet_buf, size_t packet_len)
  {
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, packet_len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(packet_buf, packet_len);
    }

    if(m_record_only) {
      return packet_len;
    }

    const char* where = "LSE_Millennium_Handler::parse2";
    
    const lse_millennium::message_header& header = reinterpret_cast<const lse_millennium::message_header&>(*packet_buf);

    if(header.hdr_count == 0) {
      return packet_len;
    }

    channel_info& ci = m_channel_info_map[context];
    const char* buf = packet_buf + sizeof(lse_millennium::message_header);

    uint8_t len = buf[0];
    for(uint8_t i = 0; i < header.hdr_count; ++i, buf += len) {
      len =               buf[0];
      char message_type = buf[1];

      switch(message_type) {
      case 0x54: // timestamp
        {
          SIZE_CHECK(lse_millennium::timestamp_message);
          const lse_millennium::timestamp_message& msg = reinterpret_cast<const lse_millennium::timestamp_message&>(*buf);
          uint32_t sec = msg.seconds;
          ci.timestamp = m_midnight + seconds(sec);
          m_micros_exch_time = sec * Time_Constants::micros_per_second;
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
          m_micros_exch_time_marker = m_micros_exch_time;
        }
        break;
      case 0x52: // symbol_directory
        {
          SIZE_CHECK(lse_millennium::symbol_directory);
          const lse_millennium::symbol_directory& msg = reinterpret_cast<const lse_millennium::symbol_directory&>(*buf);

          m_logger->log_printf(Log::LOG, "symbol_directory %d  %12s  %12s  %6s", msg.instrument_id,
                               msg.isin, msg.sedol, msg.underlying);

        }
        break;
      case 0x48: // symbol_status
        {
          SIZE_CHECK(lse_millennium::symbol_status);
          const lse_millennium::symbol_status& msg = reinterpret_cast<const lse_millennium::symbol_status&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          if(m_qbdf_builder) {
            // nothing yet
          } else {
            ProductID id = product_id_of_symbol(msg.instrument_id);
            if(id != Product::invalid_product_id) {
              m_msu.m_id = id;
              bool send_msu = true;
              switch(msg.trading_status) {
              case 'H': m_msu.m_market_status = MarketStatus::Halted; break;
              case 'T': m_msu.m_market_status = MarketStatus::Open; break;
              default:
                send_msu = false;
                break;
              }
              if(send_msu) {
                m_security_status[id] = m_msu.m_market_status;
                if(m_subscribers) {
                  m_subscribers->update_subscribers(m_msu);
                }
              }
            }
          }
        }
        break;
      case 0x41:  // add_order
        {
          SIZE_CHECK(lse_millennium::add_order);
          const lse_millennium::add_order& msg = reinterpret_cast<const lse_millennium::add_order&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          ProductID id = product_id_of_symbol(msg.instrument_id);
          if(id != Product::invalid_product_id) {
            uint64_t order_id = msg.order_id;
            uint32_t shares = msg.shares;
            MDPrice price = MDPrice::from_iprice64(msg.price);

            if(price.blank() && msg.side == 'B') {
              price.set_iprice64(MARKET_BID);
            }

            Time t = ci.timestamp + Time_Duration(ns);
            add_order(context, t, order_id, id, msg.side, price, shares);
          }
        }
        break;
      case 0x46:  // add_attributed_order
        {
          SIZE_CHECK(lse_millennium::add_attributed_order);
          const lse_millennium::add_attributed_order& msg = reinterpret_cast<const lse_millennium::add_attributed_order&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          ProductID id = product_id_of_symbol(msg.instrument_id);
          if(id != Product::invalid_product_id) {
            uint64_t order_id = msg.order_id;
            uint32_t shares = msg.shares;
            MDPrice price = MDPrice::from_iprice64(msg.price);

            if(price.blank() && msg.side == 'B') {
              price.set_iprice64(MARKET_BID);
            }

            Time t = ci.timestamp + Time_Duration(ns);
            add_order(context, t, order_id, id, msg.side, price, shares);
          }
        }
        break;
      case 0x44:  // order_deleted
        {
          // As of 20120430 LSE uses a 19 byte packet which includes the instrument_id, previously it was 15 bytes
          // Right now we ignore instrument_id
          if(len != 19 && len != 15) {
            send_alert("%s %s called with lse_millennium::order_deleted len=%d, expecting 19 or 15", where, m_name.c_str(), len);
            continue;
          }

          const lse_millennium::order_deleted& msg = reinterpret_cast<const lse_millennium::order_deleted&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          uint64_t order_id = msg.order_id;
          Time t = ci.timestamp + Time_Duration(ns);
          if(m_order_book) {
            bool inside_changed = false;
            {
              MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

              MDOrder* order = m_order_book->unlocked_find_order(order_id);
              if(order) {
                if(m_qbdf_builder) {
                  gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                               m_micros_recv_time, m_micros_exch_time_delta, 'L',
                                               order->side(), 0, MDPrice());
                }

                inside_changed = order->is_top_level();
                if(inside_changed) {
                  m_quote.m_id = order->product_id;
                }
                MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
                m_order_book->unlocked_remove_order(t, order);
              }
            }
            if(inside_changed) {
              m_quote.set_time(t);
              send_quote_update();
            }
          }
        }
        break;
      case 0x55: // order_modified
        {
          SIZE_CHECK(lse_millennium::order_modified);
          const lse_millennium::order_modified& msg = reinterpret_cast<const lse_millennium::order_modified&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          uint64_t order_id = msg.order_id;
          uint32_t shares =   msg.new_quantity;
          MDPrice price = MDPrice::from_iprice64(msg.new_price);

          Time t = ci.timestamp + Time_Duration(ns);
          modify_order(context, t, order_id, shares, price);


        }
        break;
      case 0x79: // order_book_clear
        {
          SIZE_CHECK(lse_millennium::order_book_clear);
          const lse_millennium::order_book_clear& msg = reinterpret_cast<const lse_millennium::order_book_clear&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          ProductID id = product_id_of_symbol(msg.instrument_id);
          if(id != Product::invalid_product_id) {
            if(m_order_book) {
              m_order_book->clear(id, true);

              if(m_qbdf_builder) {
                gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, id, 'L', context);
              }
            }

            if(m_subscribers) {
              m_subscribers->post_invalid_quote(id, m_quote.m_exch);
            }
          }
        }
        break;
      case 0x45:  // order executed
        {
          SIZE_CHECK(lse_millennium::order_executed);
          const lse_millennium::order_executed& msg = reinterpret_cast<const lse_millennium::order_executed&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          uint64_t order_id = msg.order_id;
          uint32_t shares =   msg.executed_shares;

          Time t = ci.timestamp + Time_Duration(ns);
          if(m_order_book) {
            bool order_found = false;
            bool inside_changed = false;
            {
              MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
              MDOrder* order = m_order_book->unlocked_find_order(order_id);
              if(order) {
                if(m_qbdf_builder) {
                  gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                             m_micros_recv_time, m_micros_exch_time_delta,
                                             'L', order->flip_side(), ' ', shares, order->price);
                }

                order_found = true;
                m_trade.m_id = order->product_id;
                m_trade.m_price = order->price_as_double();
                m_trade.set_side(order->flip_side());
                memcpy(m_trade.m_trade_qualifier, "E   ", 4);

                uint32_t new_size = order->size - shares;
                inside_changed = order->is_top_level();
                MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
                if(new_size > 0) {
                  m_order_book->unlocked_modify_order(t, order, new_size, shares);
                } else {
                  m_order_book->unlocked_remove_order(t, order, shares);
                }
              }
            }

            if(order_found) {
              m_trade.set_time(t);
              m_trade.m_size = shares;
              send_trade_update();

              if(inside_changed) {
                m_quote.set_time(t);
                m_quote.m_id = m_trade.m_id;
                send_quote_update();
              }
            }
          }
        }
        break;

      case 0x43: // order executed with price
        {
          SIZE_CHECK(lse_millennium::order_executed_with_price);
          const lse_millennium::order_executed_with_price& msg = reinterpret_cast<const lse_millennium::order_executed_with_price&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          uint64_t order_id = msg.order_id;
          uint32_t shares =   msg.executed_shares;
          uint32_t remaining_shares = msg.remaining_shares;
          MDPrice price = MDPrice::from_iprice64(msg.execution_price);

          Time t = ci.timestamp + Time_Duration(ns);
          if(m_order_book) {
            bool order_found = false;
            bool inside_changed = false;
            {
              MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
              MDOrder* order = m_order_book->unlocked_find_order(order_id);
              if(order) {
                if(m_qbdf_builder) {
                  if(msg.printable == 'Y') {
                    if(remaining_shares + shares != order->size) {
                      gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                                   m_micros_recv_time, m_micros_exch_time_delta, 'L',
                                                   order->side(), remaining_shares+shares, price);
                    }
                    gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                               m_micros_recv_time, m_micros_exch_time_delta,
                                               'L', order->flip_side(), msg.printable,
                                               shares, price);
                  } else {
                    gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                                 m_micros_recv_time, m_micros_exch_time_delta, 'L',
                                                 order->side(), remaining_shares, price);
                  }
                }
                order_found = true;
                inside_changed = order->is_top_level();
                m_trade.m_id = order->product_id;
                m_trade.set_side(order->flip_side());
                memcpy(m_trade.m_trade_qualifier, "C   ", 4);
                MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
                if(remaining_shares > 0) {
                  m_order_book->unlocked_modify_order(t, order, remaining_shares, shares);
                } else {
                  m_order_book->unlocked_remove_order(t, order, shares);
                }
              }
            }

            if(order_found) {
              if(msg.printable == 'Y') {
                m_trade.set_time(t);
                m_trade.m_price = price.fprice();
                m_trade.m_size = shares;
                send_trade_update();
              }
            }
            if(inside_changed) {
              m_quote.set_time(t);
              m_quote.m_id = m_trade.m_id;
              send_quote_update();
            }
          }
        }
        break;
      case 0x50: // trade
        {
          SIZE_CHECK(lse_millennium::trade);
          const lse_millennium::trade& msg = reinterpret_cast<const lse_millennium::trade&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          m_trade.m_id = product_id_of_symbol(msg.instrument_id);
          if(m_trade.m_id != Product::invalid_product_id) {
            MDPrice price = MDPrice::from_iprice64(msg.price);
            m_trade.set_time(ci.timestamp + Time_Duration(ns));
            m_trade.m_price = price.fprice();
            m_trade.m_size = msg.executed;
            m_trade.set_side(0); // side is obfuscated
            
            switch(msg.cross_type) {
            default:
            case 0: memcpy(m_trade.m_trade_qualifier, "P   ", 4); break;
            case 5: memcpy(m_trade.m_trade_qualifier, "XI  ", 4); break;  // internal cross
            case 6: memcpy(m_trade.m_trade_qualifier, "BI  ", 4); break;  // internal BTF
            case 7: memcpy(m_trade.m_trade_qualifier, "XC  ", 4); break;  // committed cross
            case 8: memcpy(m_trade.m_trade_qualifier, "BI  ", 4); break;  // committed BTF
            }
            
            if (m_qbdf_builder) {
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, 'L', ' ', '0',
                                    string(m_trade.m_trade_qualifier, 4), msg.executed, price);
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x51: // auction_trade
        {
          SIZE_CHECK(lse_millennium::auction_trade);
          const lse_millennium::auction_trade& msg = reinterpret_cast<const lse_millennium::auction_trade&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          m_trade.m_id = product_id_of_symbol(msg.instrument_id);
          if(m_trade.m_id != Product::invalid_product_id) {
            MDPrice price = MDPrice::from_iprice64(msg.price);
            m_trade.set_time(ci.timestamp + Time_Duration(ns));
            m_trade.m_price = price.fprice();
            m_trade.m_size = msg.executed;
            m_trade.set_side(0); // side is obfuscated
            switch(msg.auction_type) {
            case 'C':
              memcpy(m_trade.m_trade_qualifier, "QC  ", 4);
              break;
            case 'O':
              memcpy(m_trade.m_trade_qualifier, "QO  ", 4);
              break;
            default:
              memcpy(m_trade.m_trade_qualifier, "    ", 4);
              m_trade.m_trade_qualifier[0] = msg.auction_type;
            }
            
            if (m_qbdf_builder) {
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, 'L', ' ', '0',
                                    string(m_trade.m_trade_qualifier, 4), msg.executed, price);
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x78: // off_book_trade
        {
          SIZE_CHECK(lse_millennium::off_book_trade);

          const lse_millennium::off_book_trade& msg = reinterpret_cast<const lse_millennium::off_book_trade&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          m_trade.m_id = product_id_of_symbol(msg.instrument_id);
          if(m_trade.m_id != Product::invalid_product_id) {
            MDPrice price = MDPrice::from_iprice64(msg.price);
            m_trade.set_time(ci.timestamp + Time_Duration(ns));
            m_trade.m_price = price.fprice();
            m_trade.m_size = msg.executed;
            m_trade.set_side(0); // side is obfuscated
            memcpy(m_trade.m_trade_qualifier, msg.trade_type, 4);
            if (m_qbdf_builder) {
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, 'L', ' ', '0',
                                    string(m_trade.m_trade_qualifier, 4), msg.executed, price);
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x42: // broken trade
        {
          // As of 20120430 LSE uses a 19 byte packet which includes the instrument_id, previously it was 15 bytes
          // Right now we ignore the entire message
          if(len != 19 && len != 15) {
            send_alert("%s %s called with lse_millennium::broken_trade len=%d, expecting 19 or 15", where, m_name.c_str(), len);
            continue;
          }
          //const lse_millennium::broken_trade_message& msg = reinterpret_cast<const lse_millennium::broken_trade_message&>(*buf);
        }
        break;
      case 0x49: // auction_info
        {
          SIZE_CHECK(lse_millennium::auction_info);
          const lse_millennium::auction_info& msg = reinterpret_cast<const lse_millennium::auction_info&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_micros_exch_time = m_micros_exch_time_marker + (ns / Time_Constants::ticks_per_micro);
          m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

          Time t = ci.timestamp + Time_Duration(ns);

          ProductID id = product_id_of_symbol(msg.instrument_id);
          if(id != Product::invalid_product_id) {
            MDPrice near_price = MDPrice::from_iprice64(msg.indicative_price);

            m_auction.set_time(t);
            m_auction.m_id = id;
            m_auction.m_ref_price = -1.0;
            m_auction.m_far_price = -1.0;
            m_auction.m_near_price = near_price.fprice();
            m_auction.m_paired_size = msg.paired_quantity;
            m_auction.m_imbalance_size = 0;
            m_auction.m_imbalance_direction = '0';
            m_auction.m_indicative_price_flag = msg.auction_type;

            m_msu.m_market_status = m_security_status[id];
            m_feed_rules->apply_auction_conditions(m_auction, m_msu);
            if(m_msu.m_market_status != m_security_status[id]) {
              m_msu.m_id = id;
              m_security_status[id] = m_msu.m_market_status;
              if(m_subscribers) {
                m_subscribers->update_subscribers(m_msu);
              }
            }

            if (m_qbdf_builder && !m_exclude_imbalance) {
              gen_qbdf_imbalance_micros(m_qbdf_builder, id, m_micros_recv_time,
                                        m_micros_exch_time_delta, 'L',
                                        MDPrice(), near_price, MDPrice(),
                                        msg.paired_quantity, 0, '0', msg.auction_type);
            } else if(m_subscribers) {
              m_subscribers->update_subscribers(m_auction);
            }

            if(m_provider) {
              m_provider->check_auction(m_auction);
            }
          }
        }
        break;
      case 0x77: // statistics
        {
          SIZE_CHECK(lse_millennium::statistics);

        }
        break;
      case 0x71: // top_of_book
	{
	  SIZE_CHECK(lse_millennium::top_of_book);
	}
	break;
      default:
        send_alert("%s %s called with unknown message type '%c'", where, m_name.c_str(), message_type);
        break;
      }
    }
    return packet_len;
  }
  
  void
  LSE_Millennium_Handler::send_retransmit_request(channel_info& b)
  {
    
    
  }
  
  size_t
  LSE_Millennium_Handler::read(size_t context, const char* buf, size_t len)
  {
    
    return 0;
  }
  
  void
  LSE_Millennium_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, 'L', -1);
      }

      m_order_book->clear(false);
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }

  void
  LSE_Millennium_Handler::reset(size_t context, const char* msg)
  {
    const char* where = "LSE_Millennium_Handler::reset";

    if (context >= m_channel_info_map.size()) {
      m_logger->log_printf(Log::ERROR, "%s: Bad context (%ld), skipping reset", where, context);
      return;
    }

    channel_info& ci = m_channel_info_map[context];

    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    if(m_subscribers) {
      for(vector<ProductID>::const_iterator p = ci.clear_products.begin(), p_end = ci.clear_products.end();
          p != p_end; ++p) {
        m_subscribers->post_invalid_quote(*p, m_quote.m_exch);
      }
    }

    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, 'L', context);
      }

      for(vector<ProductID>::const_iterator p = ci.clear_products.begin(), p_end = ci.clear_products.end();
          p != p_end; ++p) {
        m_order_book->clear(*p, false);
        m_seen_product.reset(*p);
      }
    }

    m_logger->log_printf(Log::WARN, "LSE_Millennium_Handler::reset %s called with %zd products", m_name.c_str(), ci.clear_products.size());

    ci.clear_products.clear();
  }

  size_t
  LSE_Millennium_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* packet_buf, size_t packet_len)
  {
    const lse_millennium::message_header& header = reinterpret_cast<const lse_millennium::message_header&>(*packet_buf);
    
    log.printf(" seq_no:%u, \"hdr_count\":%u\n", header.hdr_sequence, header.hdr_count);
    
    if(header.hdr_count == 0) {
      return packet_len;
    }
        
    const char* buf = packet_buf + sizeof(lse_millennium::message_header);

    uint8_t len = buf[0];
    for(uint8_t i = 0; i < header.hdr_count; ++i, buf += len) {
      len =               buf[0];
      char message_type = buf[1];
      
      switch(message_type) {
      case 0x54: // timestamp
        {
          const lse_millennium::timestamp_message& msg = reinterpret_cast<const lse_millennium::timestamp_message&>(*buf);
          
          Time_Duration ts = seconds(msg.seconds);
          
          char time_buf[32];
          Time_Utils::print_time(time_buf, ts, Timestamp::MILI);
          
          log.printf("  timestamp %s\n", time_buf);
        }
        break;
      case 0x52: // symbol_directory
        {
          const lse_millennium::symbol_directory& msg = reinterpret_cast<const lse_millennium::symbol_directory&>(*buf);
          log.printf("  symbol_directory ns:%.9u stock:%d status:%d isin:%.12s sedol:%.12s underlying:%.6s currency:%.3s\n",
                     msg.nanoseconds, msg.instrument_id, msg.status, msg.isin, msg.sedol, msg.underlying, msg.currency);
        }
        break;
      case 0x48: // symbol_status
        {
          const lse_millennium::symbol_status& msg = reinterpret_cast<const lse_millennium::symbol_status&>(*buf);
          log.printf("  symbol_status ns:%.9u stock:%d trading_status:%d flags:%d halt_reason:%.4s session_change_reason:%d new_end_time:%.8s book_type:%d\n",
                     msg.nanoseconds, msg.instrument_id, msg.trading_status, msg.flags, msg.halt_reason, msg.session_change_reason, msg.new_end_time, msg.book_type);
        }
        break;
      case 0x41:  // add_order
        {
          const lse_millennium::add_order& msg = reinterpret_cast<const lse_millennium::add_order&>(*buf);
          
          IPrice64 price = IPrice64::from_iprice64(msg.price);
          
          log.printf("  add_order ns:%.9u order_id:%lu side:%c shares:%u stock:%d price:%0.4f flags:%d\n",
                     msg.nanoseconds, msg.order_id, msg.side, msg.shares, msg.instrument_id, price.fprice(), msg.flags);
        }
        break;
      case 0x46:  // add_attributed_order
        {
          const lse_millennium::add_attributed_order& msg = reinterpret_cast<const lse_millennium::add_attributed_order&>(*buf);
          
          IPrice64 price = IPrice64::from_iprice64(msg.price);
          
          log.printf("  add_attributed_order ns:%.9u order_id:%lu side:%c shares:%u stock:%d price:%0.4f attribution:%.11s flags:%d\n",
                     msg.nanoseconds, msg.order_id, msg.side, msg.shares, msg.instrument_id, price.fprice(), msg.attribution, msg.flags);
        }
        break;
      case 0x50: // trade
        {
          const lse_millennium::trade& msg = reinterpret_cast<const lse_millennium::trade&>(*buf);
          MDPrice price = MDPrice::from_iprice64(msg.price);
          log.printf("  trade ns:%.9u executed:%d stock:%d price:%0.4f match_number:%ld cross_type:%d\n",
                     msg.nanoseconds, msg.executed, msg.instrument_id, price.fprice(), msg.match_number, msg.cross_type);
        }
        break;
      case 0x51: // auction_trade
        {
          const lse_millennium::auction_trade& msg = reinterpret_cast<const lse_millennium::auction_trade&>(*buf);
          MDPrice price = MDPrice::from_iprice64(msg.price);
          log.printf("  auction_trade ns:%.9u executed:%d stock:%d price:%0.4f match_number:%ld auction_type:%c\n",
                     msg.nanoseconds, msg.executed, msg.instrument_id, price.fprice(), msg.match_number, msg.auction_type);
	}
	break;
      case 0x71: // top_of_book
	{
          const lse_millennium::top_of_book& msg = reinterpret_cast<const lse_millennium::top_of_book&>(*buf);
	  MDPrice buy_limit_price = MDPrice::from_iprice64(msg.buy_limit_price);
	  MDPrice sell_limit_price = MDPrice::from_iprice64(msg.sell_limit_price);
	  log.printf("  top_of_book ns:%.9u buy_limit_price:%0.4f sell_limit_price:%0.4f buy_limit_size:%d sell_limit_size:%d\n",
		     msg.nanoseconds, buy_limit_price.fprice(), sell_limit_price.fprice(), msg.buy_limit_size, msg.sell_limit_size);
	}
        break;
      case 0x79: // order_book_clear
        {
          const lse_millennium::order_book_clear& msg = reinterpret_cast<const lse_millennium::order_book_clear&>(*buf);
	  log.printf("  order_book_clear ns:%.9u stock:%d flags:0x%02x\n", msg.nanoseconds, msg.instrument_id, msg.flags);
        }
        break;
      default:
        {
	  log.printf("  unhandled message type 0x%02x\n", message_type);
        }
        break;
      }
    }
    
    return len;
  }

  void
  LSE_Millennium_Handler::send_quote_update()
  {
    m_order_book->fill_quote_update(m_quote.m_id, m_quote);
    m_msu.m_market_status = m_security_status[m_quote.m_id];
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);
    if(m_msu.m_market_status != m_security_status[m_quote.m_id]) {
      m_msu.m_id = m_quote.m_id;
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
  LSE_Millennium_Handler::send_trade_update()
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
  LSE_Millennium_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);

    m_name = name;

    m_midnight = Time::today();

    bool build_book = params.getDefaultBool("build_book", true);
    if(build_book) {
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_books.push_back(m_order_book);
      m_order_book->init(params);
    }

    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;

    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';

    m_seen_product.resize(m_app->pm()->max_product_id(), false);

    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);

    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);
  }

  void
  LSE_Millennium_Handler::start()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  LSE_Millennium_Handler::stop()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  LSE_Millennium_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
                                            const string& mdSymbol_,const string& mdExch_)
  {
    mutex_t::scoped_lock lock(m_mutex);

    char* end = 0;

    uint32_t inst_id = strtol(mdSymbol_.c_str(), &end, 10);

    if(inst_id != 0 && *end == '\0') {
      m_prod_map.insert(make_pair(inst_id, id_));
    } else {
      m_logger->log_printf(Log::ERROR, "Received invalid submap symbol for %s '%s'",
                           m_app->pm()->find_by_id(id_).symbol().c_str(), mdSymbol_.c_str());
      return;
    }

  }

  LSE_Millennium_Handler::LSE_Millennium_Handler(Application* app) :
    Handler(app, "LSE_Millennium"),
    m_micros_exch_time_marker(0)
  {
  }

  LSE_Millennium_Handler::~LSE_Millennium_Handler()
  {
  }


}
