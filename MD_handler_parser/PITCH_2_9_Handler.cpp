#include <Data/QBDF_Util.h>
#include <Data/PITCH_2_9_Handler.h>
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

  static FactoryRegistrar1<std::string, Handler, Application*, PITCH_2_9_Handler> r1("PITCH_2_9_Handler");

  namespace pitch_29 {
    const uint64_t exec_id_msb = 2821109907456;

    struct message_header {
      uint16_t hdr_length;
      uint8_t  hdr_count;
      uint8_t  hdr_unit;
      uint32_t hdr_sequence;
    } __attribute__ ((packed));

    // 0x20
    struct timestamp_message {
      uint8_t  length;
      char     message_type;
      uint32_t seconds;
    }  __attribute__ ((packed));

    // 0x21
    struct add_order_long {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      char     stock[6];
      uint64_t price;
      char     flags;
    } __attribute__ ((packed));

    // 0x40
    struct add_order_long_eu {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint64_t price;
    } __attribute__ ((packed));

    // 0x22
    struct add_order_short {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     side;
      uint16_t shares;
      char     stock[6];
      uint16_t price;
      char     flags;
    } __attribute__ ((packed));

    // 0x2f
    struct add_order_expanded {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint64_t price;
      char     flags;
      char     participant_id[4];
      char     customer_indicator;
    } __attribute__ ((packed));

    // 0x23
    struct order_executed {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t executed_shares;
      uint64_t match_number;
      char     flags[3];
    } __attribute__ ((packed));

    // 0x24
    struct order_executed_with_price {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t executed_shares;
      uint32_t remaining_shares;
      uint64_t match_number;
      uint64_t execution_price;
      char     flags[3];
    } __attribute__ ((packed));

    // 0x25
    struct reduce_size_long {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t canceled_shares;
    } __attribute__ ((packed));

    // 0x26
    struct reduce_size_short {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint16_t canceled_shares;
    } __attribute__ ((packed));

    // 0x27
    struct modify_order_long {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t shares;
      uint64_t price;
      char     flags;
    } __attribute__ ((packed));

    // 0x28
    struct modify_order_short {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint16_t shares;
      uint16_t price;
      char     flags;
    } __attribute__ ((packed));

    // 0x29
    struct delete_order {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
    } __attribute__ ((packed));

    // 0x32
    struct trade_report {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t shares;
      char     stock[8];
      uint64_t price;
      uint64_t match_number;
      uint64_t timestamp;
      char     execution_venue[4];
      char     currency[3];
      char     flags[11];
    } __attribute__ ((packed));

    // 0x34
    struct statistics_message {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      uint64_t price;
      char     statistic_type;
      char     price_determination;
    } __attribute__ ((packed));

    // 0x2a
    struct trade_long {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_ref_number;
      char     side;
      uint32_t shares;
      char     stock[6];
      uint64_t price;
      uint64_t match_number;
    } __attribute__ ((packed));

    // 0x41
    struct trade_long_eu {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_ref_number;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint64_t price;
      uint64_t match_number;
      char     flags[4];
    } __attribute__ ((packed));

    // 0x2b
    struct trade_short {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_ref_number;
      char     side;
      uint16_t shares;
      char     stock[6];
      uint16_t price;
      uint64_t match_number;
      char     flags[4];
    } __attribute__ ((packed));

    // 0x30
    struct trade_expanded {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_ref_number;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint64_t price;
      uint64_t match_number;
    } __attribute__ ((packed));

    // 0x2c
    struct broken_trade_message {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t match_number;
    } __attribute__ ((packed));

    // 0x2d
    struct end_of_session {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
    } __attribute__ ((packed));

    // 0x31
    struct trading_status {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     halt_status;
      char     reg_SHO_auction;
      char     reserved1;
      char     reserved2;
    } __attribute__ ((packed));

    // 0x95
    struct auction_update {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     auction_type;
      uint64_t reference_price;
      uint32_t buy_shares;
      uint32_t sell_shares;
      uint64_t indicative_price;
      uint64_t reserved;
    } __attribute__ ((packed));

    // 0xac
    struct auction_update_eu {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     auction_type;
      uint64_t reference_price;
      uint64_t indicative_price;
      uint32_t indicative_shares;
      char     outside_tolerance;
      char     includes_primary;
    } __attribute__ ((packed));


    // 0x96
    struct auction_summary {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     auction_type;
      uint64_t price;
      uint32_t shares;
    } __attribute__ ((packed));

    // 0x97
    struct clear_unit {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
    } __attribute__ ((packed));
    
    // 0x98
    struct retail_price_improvement {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     rpi;
    } __attribute__ ((packed));
    
  }


  static
  double
  raw_price_to_double(uint32_t p_n)
  {
    double price = ((double) p_n) / 10000.0;
    return price;
  }

  static
  double
  raw_short_price_to_double(uint16_t p_n)
  {
    double price = ((double) p_n) / 100.0;
    return price;
  }
    
  ProductID
  PITCH_2_9_Handler::product_id_of_symbol(const char* symbol, int len)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol, len);
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string.c_str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
                             "PITCH_2_9_Handler::product_id_of_symbol", symbol);
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      
      return symbol_id;
    }
    
    return -1;
  }


  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  if(len > sizeof(type)) {                                              \
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), len, sizeof(type)); \
    return len;                                                         \
  }                                                                     \

  size_t
  PITCH_2_9_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "PITCH_2_9_Handler::parse";

    if(len < sizeof(pitch_29::message_header)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd", where, m_name.c_str(), len, sizeof(pitch_29::message_header));
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

    const pitch_29::message_header& header = reinterpret_cast<const pitch_29::message_header&>(*buf);
    
    while(header.hdr_unit >= m_channel_info_map.size()) {
      m_channel_info_map.push_back(channel_info("", m_channel_info_map.size() + 1));
      m_logger->log_printf(Log::INFO, "%s: %s increasing channel_info_map due to new unit %d (context=%lu)", where, m_name.c_str(), header.hdr_unit, context);
    }

    channel_info& ci = m_channel_info_map[header.hdr_unit];

    uint32_t seq_no = header.hdr_sequence;
    uint32_t last_seq_no = seq_no + header.hdr_count;

    if(seq_no == 0) {
      parse2(header.hdr_unit, buf, len);
      return len;
    }

    if(seq_no > ci.seq_no) {
      if(ci.begin_recovery(this, buf, len, seq_no, last_seq_no)) {
        return len;
      }
    } else if(last_seq_no <= ci.seq_no) {
      // duplicate
      return len;
    }
    
    parse2(header.hdr_unit, buf, len);
    ci.seq_no = last_seq_no;
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

  void
  PITCH_2_9_Handler::add_order(size_t context, Time t, uint64_t order_id, ProductID id, char side, uint64_t price32, uint32_t shares)
  {
    SEEN();
    if(m_order_book) {
      MDPrice price = MDPrice::from_iprice32(price32);
      bool inside_changed = m_order_book->add_order_ip(t, order_id, id, side, price, shares);

      if(inside_changed) {
        m_quote.set_time(t);
        m_quote.m_id = id;
        send_quote_update();
      }
    }
  }

  void
  PITCH_2_9_Handler::reduce_size(size_t context, Time t, uint64_t order_id, uint32_t shares)
  {
    if(m_order_book) {
      bool inside_changed = false;
      {
        MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

        MDOrder* order = m_order_book->unlocked_find_order(order_id);
        if(order) {
          uint32_t new_size = order->size - shares;

          if(m_qbdf_builder) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

            if (new_size > 0) {
              gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                           m_micros_recv_time, m_micros_exch_time_delta,
                                           m_exch_char, order->side(), new_size, order->price);
            } else {
              gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                           m_micros_recv_time, m_micros_exch_time_delta,
                                           m_exch_char, order->side(), 0, MDPrice());
            }
          }

          inside_changed = order->is_top_level();
          if(inside_changed) {
            m_quote.m_id = order->product_id;
          }
          MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
          if(new_size > 0) {
            m_order_book->unlocked_modify_order(t, order, new_size);
          } else {
            m_order_book->unlocked_remove_order(t, order);
          }
        }
      }

      if(inside_changed) {
        m_quote.set_time(t);
        send_quote_update();
      }
    }
  }

  void
  PITCH_2_9_Handler::modify_order(size_t context, Time t, uint64_t order_id, uint32_t shares, uint64_t price32)
  {
    if(m_order_book) {
      MDPrice iprice = MDPrice::from_iprice32(price32);

      ProductID id = Product::invalid_product_id;
      bool inside_changed = false;
      {
        MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

        MDOrder* order = m_order_book->unlocked_find_order(order_id);
        if(order) {

          if(m_qbdf_builder) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                         m_micros_recv_time, m_micros_exch_time_delta,
                                         m_exch_char, order->side(), shares, iprice);
          }

          id = order->product_id;
          MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
          inside_changed = m_order_book->unlocked_modify_order_ip(t, order, order->side(), iprice, shares);
          SEEN();
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
  PITCH_2_9_Handler::parse2(size_t context, const char* packet_buf, size_t packet_len)
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
    
    const char* where = "PITCH_2_9_Handler::parse2";

    const pitch_29::message_header& header = reinterpret_cast<const pitch_29::message_header&>(*packet_buf);

    if(header.hdr_count == 0) {
      return packet_len;
    }
    
    uint32_t seq_no = header.hdr_sequence;
    int      message_count = header.hdr_count;
    
    channel_info& ci = m_channel_info_map[context];
    const char* buf = packet_buf + sizeof(pitch_29::message_header);

    if(seq_no < ci.seq_no) {
      //m_logger->log_printf(Log::INFO, "%s: %s %s #%zd received partially-new packet seq_num=%u expected_seq_num=%zd msg_count=%d",
      //                     where, m_name.c_str(), ci.name.c_str(), ci.context, seq_no, ci.seq_no, message_count);
      while(seq_no < ci.seq_no && message_count > 0) {
        uint8_t len = buf[0];
        buf += len;
        --message_count;
        ++seq_no;
      }
    }

    uint8_t len = buf[0];
    for(int i = 0 ; i < message_count; ++i, buf += len) {
      len                  = buf[0];
      uint8_t message_type = buf[1];
      
      switch(message_type) {
      case 0x20: // timestamp
        {
          SIZE_CHECK(pitch_29::timestamp_message);
          const pitch_29::timestamp_message& msg = reinterpret_cast<const pitch_29::timestamp_message&>(*buf);
          uint32_t sec = msg.seconds;
          ci.timestamp = m_midnight + seconds(sec);
          ci.last_timestamp_ms = sec * Time_Constants::millis_per_second;
        }
        break;
      case 0x21:  // add_order_long
        {
          SIZE_CHECK(pitch_29::add_order_long);
          const pitch_29::add_order_long& msg = reinterpret_cast<const pitch_29::add_order_long&>(*buf);

          uint32_t ns = msg.nanoseconds;
          ProductID id = product_id_of_symbol6(msg.stock);
          if(id != Product::invalid_product_id) {
            uint64_t order_id = msg.order_id;
            uint32_t shares =   msg.shares;
            uint64_t price =    msg.price;
            Time t = ci.timestamp + Time_Duration(ns);
            add_order(context, t, order_id, id, msg.side, price, shares);
            if(m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                        m_micros_exch_time_delta, m_exch_char, msg.side, shares,
                                        MDPrice::from_iprice32(price));
            }
          }
        }
        break;
      case 0x40: // add_order_long_eu. most code copy/pasted from previous block
        {
          SIZE_CHECK(pitch_29::add_order_long_eu);
          const pitch_29::add_order_long_eu& msg = reinterpret_cast<const pitch_29::add_order_long_eu&>(*buf);
          
          uint32_t ns = msg.nanoseconds;
          ProductID id = product_id_of_symbol8(msg.stock);
          if(id != Product::invalid_product_id) {
            uint64_t order_id = msg.order_id;
            uint32_t shares =   msg.shares;
            uint64_t price =    msg.price;
            Time t = ci.timestamp + Time_Duration(ns);
            add_order(context, t, order_id, id, msg.side, price, shares);
            if(m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                        m_micros_exch_time_delta, m_exch_char, msg.side, shares,
                                        MDPrice::from_iprice32(price));
            }
          }
        }
        break;
      case 0x22:  // add_order_short
        {
          SIZE_CHECK(pitch_29::add_order_short);
          const pitch_29::add_order_short& msg = reinterpret_cast<const pitch_29::add_order_short&>(*buf);

          uint32_t ns = msg.nanoseconds;
          ProductID id = product_id_of_symbol6(msg.stock);
          if(id != Product::invalid_product_id) {
            uint64_t order_id = msg.order_id;
            uint16_t shares =   msg.shares;
            uint32_t price =    msg.price;
            price *= 100;
            Time t = ci.timestamp + Time_Duration(ns);
            add_order(context, t, order_id, id, msg.side, price, shares);
            if(m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                        m_micros_exch_time_delta, m_exch_char, msg.side, shares,
                                        MDPrice::from_iprice32(price));
            }
          }
        }
        break;
      case 0x23:  // order executed
        {
          SIZE_CHECK(pitch_29::order_executed);
          const pitch_29::order_executed& msg = reinterpret_cast<const pitch_29::order_executed&>(*buf);

          uint32_t ns = msg.nanoseconds;
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
                  m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                  gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                             m_micros_recv_time, m_micros_exch_time_delta,
                                             m_exch_char, order->flip_side(), ' ', shares, order->price);
                }

                order_found = true;
                m_trade.m_id = order->product_id;
                m_trade.m_price = order->price_as_double();
                m_trade.set_side(order->flip_side());
                m_trade.clear_flags();
                m_trade.set_exec_flag(TradeUpdate::exec_flag_exec);
                
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
      case 0x24: // order executed with price
        {
          SIZE_CHECK(pitch_29::order_executed_with_price);
          const pitch_29::order_executed_with_price& msg = reinterpret_cast<const pitch_29::order_executed_with_price&>(*buf);
          
          uint32_t ns = msg.nanoseconds;
          uint64_t order_id = msg.order_id;
          uint32_t shares =   msg.executed_shares;
          uint32_t remaining_shares = msg.remaining_shares;
          MDPrice price = MDPrice::from_iprice32(msg.execution_price);

          Time t = ci.timestamp + Time_Duration(ns);
          if(m_order_book) {
            bool order_found = false;
            bool inside_changed = false;
            {
              MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

              MDOrder* order = m_order_book->unlocked_find_order(order_id);
              if(order) {
                if(m_qbdf_builder) {
                  m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                  if(remaining_shares + shares > order->size) {
                    gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                                 m_micros_recv_time, m_micros_exch_time_delta, m_exch_char,
                                                 order->side(), remaining_shares+shares, price);
                  }
                  gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                             m_micros_recv_time, m_micros_exch_time_delta,
                                             m_exch_char, order->flip_side(), ' ', shares, price);
                }
                order_found = true;
                inside_changed = order->is_top_level();
                m_trade.m_id = order->product_id;
                m_trade.set_side(order->flip_side());
                m_trade.clear_flags();
                m_trade.set_exec_flag(TradeUpdate::exec_flag_exec_w_price);
                
                MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
                if(remaining_shares > 0) {
                  m_order_book->unlocked_modify_order(t, order, remaining_shares, shares);
                } else {
                  m_order_book->unlocked_remove_order(t, order, shares);
                }
              }
            }

            if(order_found) {
              m_trade.set_time(t);
              m_trade.m_price = price.fprice();
              m_trade.m_size = shares;
              send_trade_update();
            }
            if(inside_changed) {
              m_quote.set_time(t);
              m_quote.m_id = m_trade.m_id;
              send_quote_update();
            }
          }

        }
        break;
      case 0x25: // reduce_size_long
        {
          SIZE_CHECK(pitch_29::reduce_size_long);
          const pitch_29::reduce_size_long& msg = reinterpret_cast<const pitch_29::reduce_size_long&>(*buf);

          uint32_t ns = msg.nanoseconds;
          uint64_t order_id = msg.order_id;
          uint32_t shares =   msg.canceled_shares;
          Time t = ci.timestamp + Time_Duration(ns);
          reduce_size(context, t, order_id, shares);
        }
        break;
      case 0x26: // reduce_size_short
        {
          SIZE_CHECK(pitch_29::reduce_size_short);
          const pitch_29::reduce_size_short& msg = reinterpret_cast<const pitch_29::reduce_size_short&>(*buf);

          uint32_t ns = msg.nanoseconds;
          uint64_t order_id = msg.order_id;
          uint16_t shares =   msg.canceled_shares;
          Time t = ci.timestamp + Time_Duration(ns);
          reduce_size(context, t, order_id, shares);
        }
        break;
      case 0x27: // modify_order_long
        {
          SIZE_CHECK(pitch_29::modify_order_long);
          const pitch_29::modify_order_long& msg = reinterpret_cast<const pitch_29::modify_order_long&>(*buf);

          uint32_t ns = msg.nanoseconds;
          uint64_t order_id = msg.order_id;
          uint32_t shares =   msg.shares;
          uint64_t price =    msg.price;
          Time t = ci.timestamp + Time_Duration(ns);
          modify_order(context, t, order_id, shares, price);
        }
        break;
      case 0x28: // modify_order_short
        {
          SIZE_CHECK(pitch_29::modify_order_short);
          const pitch_29::modify_order_short& msg = reinterpret_cast<const pitch_29::modify_order_short&>(*buf);

          uint32_t ns = msg.nanoseconds;
          uint64_t order_id = msg.order_id;
          uint16_t shares =   msg.shares;
          uint32_t price =    msg.price;
          price *= 100;
          Time t = ci.timestamp + Time_Duration(ns);
          modify_order(context, t, order_id, shares, price);
        }
        break;
      case 0x29:  // delete_order
        {
          SIZE_CHECK(pitch_29::delete_order);
          const pitch_29::delete_order& msg = reinterpret_cast<const pitch_29::delete_order&>(*buf);

          uint32_t ns = msg.nanoseconds;
          uint64_t order_id = msg.order_id;
          Time t = ci.timestamp + Time_Duration(ns);
          if(m_order_book) {
            bool inside_changed = false;
            {
              MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

              MDOrder* order = m_order_book->unlocked_find_order(order_id);
              if(order) {
                if(m_qbdf_builder) {
                  m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                  gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                               m_micros_recv_time, m_micros_exch_time_delta,
                                               m_exch_char, order->side(), 0, MDPrice());
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
      case 0x2a: // trade_long
        {
          SIZE_CHECK(pitch_29::trade_long);
          const pitch_29::trade_long& msg = reinterpret_cast<const pitch_29::trade_long&>(*buf);

          uint32_t ns = msg.nanoseconds;

          // If first char of exec id (match number) is 'R' (R=27), then skip (routed trade)
          if ((msg.match_number / pitch_29::exec_id_msb) == 27)
            break;

          m_trade.m_id = product_id_of_symbol6(msg.stock);
          if(m_trade.m_id != Product::invalid_product_id) {
            Time t = ci.timestamp + Time_Duration(ns);
            m_trade.set_time(t);
            m_trade.m_price = raw_price_to_double(msg.price);
            m_trade.m_size = msg.shares;
            m_trade.set_side(0); // side is obfuscated
            m_trade.clear_flags();
            m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
            
            if (m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, m_exch_char, ' ', '0', "    ",
                                    msg.shares, MDPrice::from_iprice32(msg.price));
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x41: // trade_long_eu
        {
          SIZE_CHECK(pitch_29::trade_long_eu);
          const pitch_29::trade_long_eu& msg = reinterpret_cast<const pitch_29::trade_long_eu&>(*buf);

          // bats/chi-x EU does not use 'R' as the first character to indicate
          // routing, so we don't have the conditional "break" here like in
          // the US case

          m_trade.m_id = product_id_of_symbol8(msg.stock);
          if(m_trade.m_id != Product::invalid_product_id) {
            uint32_t ns = msg.nanoseconds;

            Time t = ci.timestamp + Time_Duration(ns);
            m_trade.set_time(t);
            m_trade.m_price = raw_price_to_double(msg.price);
            m_trade.m_size = msg.shares;
            m_trade.set_side(0); // side is obfuscated
            m_trade.clear_flags();
            m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);

            if (m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, m_exch_char, ' ', '0', "    ",
                                    msg.shares, MDPrice::from_iprice32(msg.price));
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x2b: // trade_short
        {
          SIZE_CHECK(pitch_29::trade_short);
          const pitch_29::trade_short& msg = reinterpret_cast<const pitch_29::trade_short&>(*buf);

          uint32_t ns = msg.nanoseconds;

          // If first char of exec id (match number) is 'R' (R=27), then skip (routed trade)
          if ((msg.match_number / pitch_29::exec_id_msb) == 27)
            break;

          m_trade.m_id = product_id_of_symbol6(msg.stock);
          if(m_trade.m_id != Product::invalid_product_id) {
            Time t = ci.timestamp + Time_Duration(ns);
            m_trade.set_time(t);
            m_trade.m_price = raw_short_price_to_double(msg.price);
            m_trade.m_size = msg.shares;
            m_trade.set_side(0);  // side is obfuscated
            m_trade.clear_flags();
            m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
            
            if (m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              uint32_t price = msg.price;
              price *= 100;
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, m_exch_char, ' ', '0', "    ",
                                    msg.shares, MDPrice::from_iprice32(price));
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x30: // trade_expanded
        {
          SIZE_CHECK(pitch_29::trade_expanded);
          const pitch_29::trade_expanded& msg = reinterpret_cast<const pitch_29::trade_expanded&>(*buf);

          uint32_t ns = msg.nanoseconds;

          // If first char of exec id (match number) is 'R' (R=27), then skip (routed trade)
          if ((msg.match_number / pitch_29::exec_id_msb) == 27)
            break;

          m_trade.m_id = product_id_of_symbol8(msg.stock);
          if(m_trade.m_id != Product::invalid_product_id) {
            Time t = ci.timestamp + Time_Duration(ns);
            m_trade.set_time(t);
            m_trade.m_price = raw_price_to_double(msg.price);
            m_trade.m_size = msg.shares;
            m_trade.set_side(0);  // side is obfuscated
            m_trade.clear_flags();
            m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
            
            if (m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, m_exch_char, ' ', '0', "    ",
                                    msg.shares, MDPrice::from_iprice32(msg.price));
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x2c: // broken trade
        {
          SIZE_CHECK(pitch_29::broken_trade_message);
          //const pitch_29::broken_trade_message& msg = reinterpret_cast<const pitch_29::broken_trade_message&>(*buf);

          // broken_trade_message has no identifier so it is useless unless we keep a
          // list of trade sequence numbers; so just skip
        }
        break;
      case 0x2d: // end of session message
        {
          SIZE_CHECK(pitch_29::end_of_session);
          send_alert("%s %s end of session", where, m_name.c_str());
        }
        break;
      case 0x2f:  // add_order_expanded
        {
          // As of 20120517 BATS uses a 40 byte packet including partipant_id, previously it was 36 bytes
          // As of 20160301 BATS uses a 41 byte packet including customer_indicator, previously it was 40 bytes
          // Right now we ignore the participant id
          if(len != 41 && len != 40 && len != 36) {
            send_alert("%s %s called with pitch29:add_order_expanded len=%d, expecting 41 or 40 or 36", where, m_name.c_str(), len);
            return len;
          }

          const pitch_29::add_order_expanded& msg = reinterpret_cast<const pitch_29::add_order_expanded&>(*buf);

          uint32_t ns = msg.nanoseconds;
          ProductID id = product_id_of_symbol8(msg.stock);
          if(id != Product::invalid_product_id) {
            uint64_t order_id = msg.order_id;
            uint32_t shares =   msg.shares;
            uint64_t price =    msg.price;
            Time t = ci.timestamp + Time_Duration(ns);
            add_order(context, t, order_id, id, msg.side, price, shares);
            if(m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                        m_micros_exch_time_delta, m_exch_char, msg.side, shares,
                                        MDPrice::from_iprice32(price));
            }
          }
        }
        break;
      case 0x31: // trading_status
        {
          SIZE_CHECK(pitch_29::trading_status);
          const pitch_29::trading_status& msg = reinterpret_cast<const pitch_29::trading_status&>(*buf);
          
          //uint32_t ns = msg.nanoseconds;
          
          if(m_qbdf_builder) {
            
          } else {
            ProductID id = product_id_of_symbol8(msg.stock);
            if(id != Product::invalid_product_id) {
              m_msu.m_id = id;
              
              QBDFStatusType qbdf_status;
              
              switch(msg.halt_status) {
              case 'A':
                m_msu.m_market_status = MarketStatus::Auction;
                qbdf_status = QBDFStatusType::Auction;
                break;
              case 'H':
                m_msu.m_market_status = MarketStatus::Halted;
                qbdf_status = QBDFStatusType::Halted;
                break;
              case 'Q':
                m_msu.m_market_status = MarketStatus::Auction;
                qbdf_status = QBDFStatusType::Auction;
                break;
              case 'S':
                m_msu.m_market_status = MarketStatus::Suspended;
                qbdf_status = QBDFStatusType::Suspended;
                break;
              case 'T':
                m_msu.m_market_status = MarketStatus::Open;
                qbdf_status = QBDFStatusType::Open;
                break;
              default:
                break;
              }
              m_security_status[id] = m_msu.m_market_status;
              if(m_subscribers) {
                m_subscribers->update_subscribers(m_msu);
              }
              if(m_qbdf_builder) {
                if(!m_exclude_status) {
                  string qbdf_status_reason;
                  gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time,
                                         m_micros_exch_time_delta, m_exch_char,
                                         qbdf_status.index(), qbdf_status_reason);
                }
              }
            }
          }
        }
        break;
      case 0x95: // auction update
        {
          // this is copied & slightly modified in handling of auction_update_eu (0xac below)
          SIZE_CHECK(pitch_29::auction_update);
          const pitch_29::auction_update& msg = reinterpret_cast<const pitch_29::auction_update&>(*buf);

          uint32_t ns = msg.nanoseconds;
          Time t = ci.timestamp + Time_Duration(ns);

          ProductID id = product_id_of_symbol8(msg.stock);
          if(id != Product::invalid_product_id) {
            m_auction.set_time(t);
            m_auction.m_id = id;
            m_auction.m_ref_price = raw_price_to_double(msg.reference_price);
            m_auction.m_far_price = 0;
            m_auction.m_near_price = raw_price_to_double(msg.indicative_price);

            m_auction.m_paired_size = min(msg.buy_shares, msg.sell_shares);
            m_auction.m_imbalance_size = abs((int)(msg.buy_shares - msg.sell_shares));

            if(msg.buy_shares == msg.sell_shares) {
              m_auction.m_imbalance_direction = '0';
            } else if(msg.buy_shares > msg.sell_shares) {
              m_auction.m_imbalance_direction = 'B';
            } else {
              m_auction.m_imbalance_direction = 'S';
            }
            m_auction.m_indicative_price_flag = msg.auction_type;

            if (m_qbdf_builder && !m_exclude_imbalance) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

              gen_qbdf_imbalance_micros(m_qbdf_builder, id, m_micros_recv_time,
                                        m_micros_exch_time_delta, m_exch_char,
                                        MDPrice::from_iprice32(msg.reference_price),
                                        MDPrice::from_iprice32(msg.indicative_price),
                                        MDPrice::from_iprice32(0),
                                        min(msg.buy_shares, msg.sell_shares),
                                        abs((int)(msg.buy_shares - msg.sell_shares)),
                                        m_auction.m_imbalance_direction, msg.auction_type);
            }
          }
        }
        break;
      case 0xac: // auction update_eu
        {
          // this is copied & slightly modified from handling of auction_update (0x95 above)
          SIZE_CHECK(pitch_29::auction_update_eu);
          const pitch_29::auction_update_eu& msg = reinterpret_cast<const pitch_29::auction_update_eu&>(*buf);

          uint32_t ns = msg.nanoseconds;
          Time t = ci.timestamp + Time_Duration(ns);

          ProductID id = product_id_of_symbol8(msg.stock);
          if(id != Product::invalid_product_id) {
            m_auction.set_time(t);
            m_auction.m_id = id;
            m_auction.m_ref_price = raw_price_to_double(msg.reference_price);
            m_auction.m_far_price = 0;
            m_auction.m_near_price = raw_price_to_double(msg.indicative_price);

            m_auction.m_paired_size = msg.indicative_shares;
            m_auction.m_imbalance_size = 0;

            m_auction.m_imbalance_direction = '0';
            m_auction.m_indicative_price_flag = msg.auction_type;

            if (m_qbdf_builder && !m_exclude_imbalance) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

              gen_qbdf_imbalance_micros(m_qbdf_builder, id, m_micros_recv_time,
                                        m_micros_exch_time_delta, m_exch_char,
                                        MDPrice::from_iprice32(msg.reference_price),
                                        MDPrice::from_iprice32(msg.indicative_price),
                                        MDPrice::from_iprice32(0),
                                        msg.indicative_shares,
                                        0,
                                        m_auction.m_imbalance_direction, msg.auction_type);
            }
          }
        }
        break;
      case 0x96: // auction summary
        {
          SIZE_CHECK(pitch_29::auction_summary);
          const pitch_29::auction_summary& msg = reinterpret_cast<const pitch_29::auction_summary&>(*buf);

          uint32_t ns = msg.nanoseconds;
          Time t = ci.timestamp + Time_Duration(ns);

          ProductID id = product_id_of_symbol8(msg.stock);
          if(id != Product::invalid_product_id) {
            m_trade.set_time(t);
            m_trade.m_id = id;
            m_msu.m_id = id;
            m_trade.m_price = raw_price_to_double(msg.price);
            m_trade.m_size = msg.shares;
            m_trade.set_side(0);
            m_trade.clear_flags();
            m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
            
            bool send_msu = true;
            switch(msg.auction_type) {
            case 'O':
              m_trade.m_trade_status = TradeStatus::OpenTrade;
              m_msu.m_market_status = MarketStatus::Open;
              break;
            case 'C':
              m_trade.m_trade_status = TradeStatus::CloseTrade;
              m_msu.m_market_status = MarketStatus::Closed;
              break;
            case 'H':
              m_trade.m_trade_status = TradeStatus::Cross;
              m_msu.m_market_status = MarketStatus::Open;
              break;
            case 'I':
            default:
              m_trade.m_trade_status = TradeStatus::Cross;
              send_msu = false;
              break;
            }

            if (m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              if (!m_exclude_trades) {
                gen_qbdf_cross_trade_micros(m_qbdf_builder, id, m_micros_recv_time,
                                            m_micros_exch_time_delta, m_exch_char, ' ',
                                            '0', msg.auction_type, msg.shares,
                                            MDPrice::from_iprice32(msg.price));
              }
            }

            if(send_msu) {
              if(m_subscribers) {
                m_subscribers->update_subscribers(m_msu);
              }
            }
            if(m_provider) {
              m_provider->check_trade(m_trade);
            }
            if(m_subscribers) {
              m_trade.m_is_clean = true;
              m_trade.m_size_adjustment_to_cum = m_trade.m_size;
              update_subscribers(m_trade);
            }
          }
        }
        break;
      case 0x97: // Unit Clear Message
        {
          SIZE_CHECK(pitch_29::clear_unit);
          //const pitch_29::clear_unit& msg = reinterpret_cast<const pitch_29::clear_unit&>(*buf);
          //m_logger->log_printf(Log::WARN, "PITCH_2_9_Handler::parse2 %s clear_unit", m_name.c_str(), context);
          this->reset(context, "Unit Clear Message received");
        }
        break;
      case 0x98: // Retail Price Improvement
        {
          SIZE_CHECK(pitch_29::retail_price_improvement);
          //const pitch_29::retail_price_improvement& msg = reinterpret_cast<const pitch_29::retail_price_improvement&>(*buf);
          //m_logger->log_printf(Log::WARN, "PITCH_2_9_Handler::parse2 %s clear_unit", m_name.c_str(), context);
        }
        break;
      case 0x32:
        {
            // this is like Europe's version of ADF, which is sent inline with the BATS multicast feed.
            // we ignore these messages for now, but may want to classify them as a new exchange.
            // the code below is copied & slightly adapted from the code that handles regular trade messages.
            // but check it over before just uncommenting & using it.

          // SIZE_CHECK(pitch_29::trade_report);
          // const pitch_29::trade_report& msg = reinterpret_cast<const pitch_29::trade_report&>(*buf);

          // m_trade.m_id = product_id_of_symbol(msg.stock, 8);
          // if(m_trade.m_id != Product::invalid_product_id) {
          //   uint32_t ns = msg.nanoseconds;

          //   Time t = ci.timestamp + Time_Duration(ns);
          //   m_trade.set_time(t);
          //   m_trade.m_price = raw_price_to_double(msg.price);
          //   m_trade.m_size = msg.shares;
          //   m_trade.set_side(0); // side is not sent
          //   m_trade.clear_flags();
          //   m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
          //   if (m_qbdf_builder) {
          //     m_micros_exch_time = t.total_usec() - Time::today().total_usec();
          //     m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
          //     gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
          //                           m_micros_exch_time_delta, m_exch_char, ' ', '0', "    ",
          //                           msg.shares, MDPrice::from_iprice32(msg.price));
          //   } else {
          //     send_trade_update();
          //   }
          // }
        }
        break;
      case 0x34:
        {
          // nothing really to do with these messages
          // const pitch_29::statistics_message& msg = reinterpret_cast<const pitch_29::statistics_message&>(*buf);
        }
        break;
      default:
        send_alert("%s %s called with unknown message type '%x'", where, m_name.c_str(), message_type);
        break;
      }
    }
    return packet_len;
  }

  size_t
  PITCH_2_9_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* packet_buf, size_t packet_len)
  {
    const pitch_29::message_header& header = reinterpret_cast<const pitch_29::message_header&>(*packet_buf);
    
    log.printf(" seq_num:%u  next_seq_num:%u   message_count:%u  unit:%u  length:%u\n",
               header.hdr_sequence, header.hdr_sequence + header.hdr_count, header.hdr_count, header.hdr_unit, header.hdr_length);
    
    int      message_count = header.hdr_count;
    
    const char* buf = packet_buf + sizeof(pitch_29::message_header);
    uint8_t len = buf[0];
    for(int i = 0 ; i < message_count; ++i, buf += len) {
      len                  = buf[0];
      uint8_t message_type = buf[1];
      
      switch(message_type) {
      case 0x20: // timestamp
        {
          const pitch_29::timestamp_message& msg = reinterpret_cast<const pitch_29::timestamp_message&>(*buf);
          uint32_t sec = msg.seconds;
          Time_Duration ts = seconds(sec);
          
          char time_buf[32];
          Time_Utils::print_time(time_buf, ts, Timestamp::MILI);
          
          log.printf("  timestamp %s\n", time_buf);
        }
        break;
      case 0x21:  // add_order_long
        {
          //SIZE_CHECK(pitch_29::add_order_long);
          const pitch_29::add_order_long& msg = reinterpret_cast<const pitch_29::add_order_long&>(*buf);
          
          log.printf("  add_order_long  ns:%.9u  stock:%.6s  side:%c  shares:%d  price:%0.4f  order_id:%lu  flags:%u\n",
                     msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.order_id, msg.flags);
        }
        break;
      case 0x22:  // add_order_short
        {
          //SIZE_CHECK(pitch_29::add_order_short);
          const pitch_29::add_order_short& msg = reinterpret_cast<const pitch_29::add_order_short&>(*buf);
          
          log.printf("  add_order_short  ns:%.9u  stock:%.6s  side:%c  shares:%d  price:%0.4f  order_id:%lu  flags:%u\n",
                     msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price*100).fprice(), msg.order_id, msg.flags);
        }
        break;
      case 0x23:  // order executed
        {
          //SIZE_CHECK(pitch_29::order_executed);
          const pitch_29::order_executed& msg = reinterpret_cast<const pitch_29::order_executed&>(*buf);
          
          log.printf("  order_executed  ns:%.9u  order_id:%lu  executed_shares:%u  match_number:%lu\n",
                     msg.nanoseconds, msg.order_id, msg.executed_shares, msg.match_number);
        }
        break;
      case 0x24:  // order executed with price
        {
          //SIZE_CHECK(pitch_29::order_executed);
          const pitch_29::order_executed_with_price& msg = reinterpret_cast<const pitch_29::order_executed_with_price&>(*buf);
          
          log.printf("  order_executed_with_price  ns:%.9u  order_id:%lu  executed_shares:%u  remaining_shares:%u  match_number:%lu  execution_price:%0.4f\n",
                     msg.nanoseconds, msg.order_id, msg.executed_shares, msg.remaining_shares, msg.match_number, MDPrice::from_iprice32(msg.execution_price).fprice());
        }
        break;
      case 0x25: // reduce_size_long
        {
          //SIZE_CHECK(pitch_29::reduce_size_long);
          const pitch_29::reduce_size_long& msg = reinterpret_cast<const pitch_29::reduce_size_long&>(*buf);
          
          log.printf("  reduce_size_long  ns:%.9u  order_id:%lu  cancelled_shares:%u\n",
                     msg.nanoseconds, msg.order_id, msg.canceled_shares);
        }
        break;
      case 0x26: // reduce_size_short
        {
          //SIZE_CHECK(pitch_29::reduce_size_short);
          const pitch_29::reduce_size_short& msg = reinterpret_cast<const pitch_29::reduce_size_short&>(*buf);
          
          log.printf("  reduce_size_short  ns:%.9u  order_id:%lu  cancelled_shares:%u\n",
                     msg.nanoseconds, msg.order_id, msg.canceled_shares);
        }
        break;
      case 0x27: // modify_order_long
        {
          //SIZE_CHECK(pitch_29::modify_order_long);
          const pitch_29::modify_order_long& msg = reinterpret_cast<const pitch_29::modify_order_long&>(*buf);
          
          log.printf("  modify_order_long  ns:%.9u  order_id:%lu  shares:%u  price:%0.4f  flags:%u\n",
                     msg.nanoseconds, msg.order_id, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.flags);
        }
        break;
      case 0x28: // modify_order_short
        {
          //SIZE_CHECK(pitch_29::modify_order_short);
          const pitch_29::modify_order_short& msg = reinterpret_cast<const pitch_29::modify_order_short&>(*buf);
          
          log.printf("  modify_order_long  ns:%.9u  order_id:%lu  shares:%u  price:%0.4f  flags:%u\n",
                     msg.nanoseconds, msg.order_id, msg.shares, MDPrice::from_iprice32(msg.price*100).fprice(), msg.flags);
        }
        break;
      case 0x29:  // delete_order
        {
          //SIZE_CHECK(pitch_29::delete_order);
          const pitch_29::delete_order& msg = reinterpret_cast<const pitch_29::delete_order&>(*buf);
          log.printf("  delete_order  ns:%.9u  order_id:%lu\n",
                     msg.nanoseconds, msg.order_id);
        }
        break;
      case 0x2a: // trade_long
        {
          //SIZE_CHECK(pitch_29::trade_long);
          const pitch_29::trade_long& msg = reinterpret_cast<const pitch_29::trade_long&>(*buf);
          
          log.printf("  trade_long  ns:%.9u  stock:%.6s  side:%c  shares:%u  price:%0.4f  order_ref_number:%lu  match_number:%lu\n",
                     msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.order_ref_number, msg.match_number);
        }
        break;
      case 0x2b: // trade_short
        {
          //SIZE_CHECK(pitch_29::trade_short);
          const pitch_29::trade_short& msg = reinterpret_cast<const pitch_29::trade_short&>(*buf);

          log.printf("  trade_short  ns:%.9u  stock:%.6s  side:%c  shares:%u  price:%0.4f  order_ref_number:%lu  match_number:%lu\n",
                     msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price*100).fprice(), msg.order_ref_number, msg.match_number);
        }
        break;
      case 0x30: // trade_expanded
        {
          //SIZE_CHECK(pitch_29::trade_expanded);
          const pitch_29::trade_expanded& msg = reinterpret_cast<const pitch_29::trade_expanded&>(*buf);
          
          log.printf("  trade_short  ns:%.9u  stock:%.8s  side:%c  shares:%u  price:%0.4f  order_ref_number:%lu  match_number:%lu\n",
                     msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.order_ref_number, msg.match_number);
        }
        break;
      case 0x2c: // broken trade
        {
          //SIZE_CHECK(pitch_29::broken_trade_message);
          //const pitch_29::broken_trade_message& msg = reinterpret_cast<const pitch_29::broken_trade_message&>(*buf);

          // broken_trade_message has no identifier so it is useless unless we keep a
          // list of trade sequence numbers; so just skip
        }
        break;
      case 0x2d: // end of session message
        {
          //SIZE_CHECK(pitch_29::end_of_session);
          const pitch_29::end_of_session& msg = reinterpret_cast<const pitch_29::end_of_session&>(*buf);
          log.printf("  end_of_session  ns:%9u\n",  msg.nanoseconds);
        }
        break;
      case 0x2f:  // add_order_expanded
        {
          // As of 20120517 BATS uses a 40 byte packet including partipant_id, previously it was 36 bytes
          // Right now we ignore the participant id
          const pitch_29::add_order_expanded& msg = reinterpret_cast<const pitch_29::add_order_expanded&>(*buf);
          
          if(len == 36) {
            log.printf("  add_order_expanded  ns:%.9u  stock:%.8s  side:%c  shares:%d  price:%0.4f  order_id:%lu  flags:%u\n",
                       msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.order_id, msg.flags);
          } else if(len == 40) {
            log.printf("  add_order_expanded  ns:%.9u  stock:%.8s  side:%c  shares:%d  price:%0.4f  order_id:%lu  flags:%u  participant_id:%.4s\n",
                       msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.order_id, msg.flags, msg.participant_id);
          } else if(len == 41) {
            log.printf("  add_order_expanded  ns:%.9u  stock:%.8s  side:%c  shares:%d  price:%0.4f  order_id:%lu  flags:%u  participant_id:%.4s customer_indicator:%c\n",
                       msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.order_id, msg.flags, msg.participant_id, 
                       msg.customer_indicator);
          } else {
            log.printf("  add_order_expanded  unknown len %u\n", len);
          }            
        }
        break;
      case 0x31: // trading_status
        {
          //SIZE_CHECK(pitch_29::trading_status);
          const pitch_29::trading_status& msg = reinterpret_cast<const pitch_29::trading_status&>(*buf);
          
          log.printf("  trading_status  ns:%.9u  stock:%.8s  halt_status:%c  reg_SHO:%c\n", msg.nanoseconds, msg.stock, msg.halt_status, msg.reg_SHO_auction);
          
        }
        break;
      case 0x95: // auction update
        {
          const pitch_29::auction_update& msg = reinterpret_cast<const pitch_29::auction_update&>(*buf);
          log.printf("  auction_update  ns:%.9u  stock:%.8s  auction_type:%c  reference_price:%0.4f  buy_shares:%d  sell_shares:%d  indicative_price:%0.4f\n",
              msg.nanoseconds, msg.stock, msg.auction_type, MDPrice::from_iprice32(msg.reference_price).fprice(),
              msg.buy_shares, msg.sell_shares, MDPrice::from_iprice32(msg.indicative_price).fprice());
        }
        break;
      case 0xac: // auction update eu
        {
          const pitch_29::auction_update_eu& msg = reinterpret_cast<const pitch_29::auction_update_eu&>(*buf);
          log.printf("  auction_update_eu  ns:%.9u  stock:%.8s  auction_type:%c  reference_price:%0.4f  indicative_price:%0.4f indicative_shares:%d  outside_tolerance:%c includes_primary:%c\n",
              msg.nanoseconds, msg.stock, msg.auction_type, MDPrice::from_iprice32(msg.reference_price).fprice(),
              MDPrice::from_iprice32(msg.indicative_price).fprice(),
              msg.indicative_shares, msg.outside_tolerance, msg.includes_primary);
        }
        break;
      case 0x96: // auction summary
        {
          //SIZE_CHECK(pitch_29::auction_summary);
          //const pitch_29::auction_summary& msg = reinterpret_cast<const pitch_29::auction_summary&>(*buf);
        }
        break;
      case 0x97: // Unit Clear Message
        {
          //SIZE_CHECK(pitch_29::clear_unit);
          //const pitch_29::clear_unit& msg = reinterpret_cast<const pitch_29::clear_unit&>(*buf);
          //m_logger->log_printf(Log::WARN, "PITCH_2_9_Handler::parse2 %s clear_unit", m_name.c_str(), context);
        }
        break;
      case 0x98: // Retail Price Improvement
        {
          //SIZE_CHECK(pitch_29::retail_price_improvement);
          const pitch_29::retail_price_improvement& msg = reinterpret_cast<const pitch_29::retail_price_improvement&>(*buf);
          log.printf("  retail_price_improvement  ns:%.9u  stock:%.8s  rpi:%c\n", msg.nanoseconds, msg.stock, msg.rpi);
        }
        break;
      case 0x40:
        {
            // should it be from_price64 ?
          const pitch_29::add_order_long_eu& msg = reinterpret_cast<const pitch_29::add_order_long_eu&>(*buf);
          log.printf("  add_order_long_eu  ns:%.9u  stock:%.8s  side:%c  shares:%d  price:%0.4f  order_id:%lu\n",
                     msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.order_id);
        }
        break;
      case 0x41:
        {
          const pitch_29::trade_long_eu& msg = reinterpret_cast<const pitch_29::trade_long_eu&>(*buf);
          log.printf("  trade_long_eu  ns:%.9u  stock:%.8s  side:%c  shares:%u  price:%0.4f  order_ref_number:%lu  match_number:%lu flags:%.4s\n",
                     msg.nanoseconds, msg.stock, msg.side, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.order_ref_number, msg.match_number, msg.flags);
        }
        break;
      case 0x32:
        {
          const pitch_29::trade_report& msg = reinterpret_cast<const pitch_29::trade_report&>(*buf);

          log.printf("  trade_report  ns:%.9u  stock:%.8s  shares:%lu  price:%0.4f  match_number:%lu timestamp:%lu venue:%.4s currency:%.3s flags:%.11s\n",
                     msg.nanoseconds, msg.stock, msg.shares, MDPrice::from_iprice32(msg.price).fprice(), msg.match_number, msg.timestamp, msg.execution_venue, msg.currency, msg.flags);
        }
        break;
      case 0x34:
        {
          const pitch_29::statistics_message& msg = reinterpret_cast<const pitch_29::statistics_message&>(*buf);
          log.printf("  statistics_message  ns:%.9u  stock:%.8s  price:%0.4f stat_type:%c price_determination:%c\n",
                     msg.nanoseconds, msg.stock, MDPrice::from_iprice32(msg.price).fprice(), msg.statistic_type, msg.price_determination);
        }
        break;
      default:
        break;
      }
    }
    return packet_len;
  }

  void
  PITCH_2_9_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_char, -1);
      }

      m_order_book->clear(false);
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }

  void
  PITCH_2_9_Handler::reset(size_t context, const char* msg)
  {
    const char* where = "PITCH_2_9_Handler::reset";

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
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_char, context);
      }

      for(vector<ProductID>::const_iterator p = ci.clear_products.begin(), p_end = ci.clear_products.end();
          p != p_end; ++p) {
        m_order_book->clear(*p, false);
        m_seen_product.reset(*p);
      }
    }

    m_logger->log_printf(Log::WARN, "PITCH_2_9_Handler::reset %s called with %zd products", m_name.c_str(), ci.clear_products.size());

    ci.clear_products.clear();
  }

  void
  PITCH_2_9_Handler::send_quote_update()
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
  PITCH_2_9_Handler::send_trade_update()
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
  PITCH_2_9_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
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

    if (m_qbdf_builder) {
      m_exch_char = m_qbdf_builder->get_qbdf_exch_by_hf_exch_id(exch);
      m_logger->log_printf(Log::INFO, "PITCH_2_9_Handler::init looked up m_exch_char=%c", m_exch_char);
    }
	
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
  PITCH_2_9_Handler::start()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  PITCH_2_9_Handler::stop()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  PITCH_2_9_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
                                       const string& mdSymbol_,const string& mdExch_)
  {
    mutex_t::scoped_lock lock(m_mutex);

    char symbol[9];
    size_t offset = 0;
    if(mdExch_.empty()) {
      offset = snprintf(symbol, 9, "%s", mdSymbol_.c_str());
    } else {
      offset = snprintf(symbol, 9, "%s.%s", mdSymbol_.c_str(), mdExch_.c_str());
    }
    memset(symbol+offset, ' ', 9 - offset);
    
    m_prod_map.insert8(symbol, id_);
  }

  PITCH_2_9_Handler::PITCH_2_9_Handler(Application* app) :
    Handler(app, "PITCH_2_9"),
    m_prod_map(262000)
  {
  }

  PITCH_2_9_Handler::~PITCH_2_9_Handler()
  {
  }


}
