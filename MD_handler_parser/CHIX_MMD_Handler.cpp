#include <Data/QBDF_Util.h>
#include <Data/CHIX_MMD_Handler.h>
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

  static
  uint64_t
  parse_number(const char* buf, int len)
  {
    uint64_t n = 0;
    for(int i = 0; i < len; ++i) {
      char c = buf[i];
      if(c == ' ') continue;
      n *= 10;
      n += (c - '0');
    }
    return n;
  }

  namespace chix_MMD {

    struct multicast_packet_header {
      uint32_t sequence;
      uint16_t message_count;
    } __attribute__ ((packed));

    struct multicast_data_header {
      uint16_t length;
    } __attribute__ ((packed));

    struct session_message_header {
      char      session_message_type;
    } __attribute__ ((packed));

    struct common_data_header {
      char timestamp[8];
      char message_type;
    } __attribute__ ((packed));

    struct system_event_message {
      char timestamp[8];
      char message_type;
      char event_code;
    } __attribute__ ((packed));

    struct order_add_message {
      common_data_header header;
      char order_ref[9];
      char side;
      char shares[6];
      char stock[6];
      char price[10];
      char display;
    } __attribute__ ((packed));

    struct order_add_message_long {
      common_data_header header;
      char order_ref[9];
      char side;
      char shares[10];
      char stock[6];
      char price[19];
      char display;
    } __attribute__ ((packed));

    struct order_exec_message {
      common_data_header header;
      char order_ref[9];
      char exec_shares[6];
      char trade_ref[9];
      char contra_order_ref[9];
      char tick_direction;
    } __attribute__ ((packed));

    struct order_exec_message_long {
      common_data_header header;
      char order_ref[9];
      char exec_shares[10];
      char trade_ref[9];
      char contra_order_ref[9];
      char tick_direction;
    } __attribute__ ((packed));

    struct order_cancel_message {
      common_data_header header;
      char order_ref[9];
      char cancelled_shares[6];
    } __attribute__ ((packed));

    struct order_cancel_message_long {
      common_data_header header;
      char order_ref[9];
      char cancelled_shares[10];
    } __attribute__ ((packed));

    struct trade_message {
      common_data_header header;
      char order_ref[9];
      char side;
      char shares[6];
      char stock[6];
      char price[10];
      char trade_ref[9];
      char contra_order_ref[9];
    } __attribute__ ((packed));

    struct trade_message_long {
      common_data_header header;
      char order_ref[9];
      char side;
      char shares[10];
      char stock[6];
      char price[19];
      char trade_ref[9];
      char contra_order_ref[9];
    } __attribute__ ((packed));

    struct broken_trade_message {
      common_data_header header;
      char trade_ref[9];
    } __attribute__ ((packed));

    struct stock_status_message {
      common_data_header header;
      char stock[6];
      char trading_state;
      char reserved;
    } __attribute__ ((packed));

  }

  static
  uint64_t
  int_of_symbol(const char* symbol, int len)
  {
    uint64_t r = 0;
    for(int i = 0; i < len; ++i) {
      if(symbol[i] == ' ' || symbol[i] == '\0') {
        break;
      }
      r = (r << 7) + symbol[i];
    }
    return r;
  }


  ProductID
  CHIX_MMD_Handler::product_id_of_symbol(const char* symbol, int len)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol, len);
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string.c_str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
                             "CHIX_MMD_Handler::product_id_of_symbol", symbol);
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }

      return symbol_id;
    }

    uint64_t h = int_of_symbol(symbol, len);
    hash_map<uint64_t, ProductID>::iterator i = m_prod_map.find(h);
    if(i == m_prod_map.end()) {
      return Product::invalid_product_id;
    } else {
      return i->second;
    }
  }


  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  if(data_message_length != sizeof(type)) {                                             \
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), data_message_length, sizeof(type)); \
    return data_message_length;                                                         \
  }                                                                     \

  size_t
  CHIX_MMD_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "CHIX_MMD_Handler::parse";

    if(len < sizeof(chix_MMD::session_message_header)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd",
                 where, m_name.c_str(), len, sizeof(chix_MMD::session_message_header));
      return 0;
    }

    mutex_t::scoped_lock lock(m_mutex);

    if (m_qbdf_builder && !m_use_exch_time) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
    }

    if (m_hist_mode) {
      const chix_MMD::session_message_header& header = reinterpret_cast<const chix_MMD::session_message_header&>(*buf);
      if (header.session_message_type == 'S' and buf[len-1] == '\n') {
        parse2(0, buf+sizeof(chix_MMD::session_message_header), len-sizeof(chix_MMD::session_message_header)-1);
      }
    } else {
      channel_info& ci = m_channel_info_map[context];

      const chix_MMD::multicast_packet_header& header = reinterpret_cast<const chix_MMD::multicast_packet_header&>(*buf);
      uint32_t seq_no = ntohl(header.sequence);
      uint16_t mc = ntohs(header.message_count);
      uint32_t next_seq_no = seq_no + mc;

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

      parse2(0, buf, len);
      ci.seq_no = next_seq_no;
    }

    return len;
  }

#define SEEN()                                                  \
  if(!m_seen_product.test(id)) {                                \
    m_seen_product.set(id);                                     \
  }

  void
  CHIX_MMD_Handler::add_order(size_t context, Time t, uint64_t order_id, ProductID id, char side, uint64_t price32, uint32_t shares)
  {
    SEEN();
    if(m_order_book) {
      MDPrice price = MDPrice::from_iprice32(price32);
      bool inside_changed = m_order_book->add_order_ip(t, order_id, id, side, price, shares);

      if(m_qbdf_builder) {
        m_micros_exch_time = t.total_usec() - Time::today().total_usec();
        if (m_use_exch_time) {
          m_micros_recv_time = m_micros_exch_time;
        }
        m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
        gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                  m_micros_exch_time_delta, m_qbdf_exch_char,
                                  side, shares, price);
      }

      if(inside_changed) {
        m_quote.set_time(t);
        m_quote.m_id = id;
        send_quote_update();
      }
    }
  }

  void
  CHIX_MMD_Handler::reduce_size(size_t context, Time t, uint64_t order_id, uint32_t shares)
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
            if (m_use_exch_time) {
              m_micros_recv_time = m_micros_exch_time;
            }
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

            if (new_size > 0) {
              gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                           m_micros_recv_time, m_micros_exch_time_delta,
                                           m_qbdf_exch_char, order->side(), new_size, order->price);
            } else {
              gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                           m_micros_recv_time, m_micros_exch_time_delta,
                                           m_qbdf_exch_char, order->side(), 0, MDPrice());
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
  CHIX_MMD_Handler::modify_order(size_t context, Time t, uint64_t order_id, uint32_t shares, uint64_t price32)
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
            if (m_use_exch_time) {
              m_micros_recv_time = m_micros_exch_time;
            }
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                         m_micros_recv_time, m_micros_exch_time_delta,
                                         m_qbdf_exch_char, order->side(), shares, iprice);
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
  CHIX_MMD_Handler::parse2(size_t context, const char* buf, size_t len)
  {
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
    }

    if(m_record_only) {
      return len;
    }

    const char* where = "CHIX_MMD_Handler::parse2";

    size_t buf_posn = 0;
    size_t len_parsed = 0;
    uint16_t data_message_length = 0;
    uint16_t message_count = 1;
    if (!m_hist_mode) {
      //maybe add some handling for heartbeat (message_count = 0) here
      const chix_MMD::multicast_packet_header& header = reinterpret_cast<const chix_MMD::multicast_packet_header&>(*(buf+buf_posn));
      message_count = ntohs(header.message_count);
      buf_posn += sizeof(chix_MMD::multicast_packet_header);
      len_parsed += sizeof(chix_MMD::multicast_packet_header);
    }

    for (size_t i = 0; i < message_count; i++) {
      if (!m_hist_mode) {
        const chix_MMD::multicast_data_header& data_header = reinterpret_cast<const chix_MMD::multicast_data_header&>(*(buf+buf_posn));
        data_message_length = ntohs(data_header.length);
        buf_posn += sizeof(chix_MMD::multicast_data_header);
        len_parsed += sizeof(chix_MMD::multicast_data_header);
        len_parsed += data_message_length;
      }

      const chix_MMD::common_data_header& header = reinterpret_cast<const chix_MMD::common_data_header&>(*(buf+buf_posn));

      uint32_t millis_since_midnight = parse_number(header.timestamp, 8);
      Time t = m_midnight + Time_Duration(millis_since_midnight * Time_Constants::ticks_per_mili);

      //cout << "header.message_type=" << header.message_type << endl;
      switch (header.message_type) {
      case 'A': // order_add_message
      {
        SIZE_CHECK(chix_MMD::order_add_message);
        const chix_MMD::order_add_message& msg = reinterpret_cast<const chix_MMD::order_add_message&>(*(buf+buf_posn));

        ProductID id = product_id_of_symbol(msg.stock, 6);
        if(id != Product::invalid_product_id) {
          uint32_t order_id = parse_number(msg.order_ref, sizeof(msg.order_ref));
          uint32_t shares = parse_number(msg.shares, sizeof(msg.shares));
          uint32_t price = parse_number(msg.price, sizeof(msg.price));
          add_order(context, t, order_id, id, msg.side, price, shares);
        }
      }
      break;
      case 'a': // order_add_message_long
      {
        SIZE_CHECK(chix_MMD::order_add_message_long);
        const chix_MMD::order_add_message_long& msg = reinterpret_cast<const chix_MMD::order_add_message_long&>(*(buf+buf_posn));

        ProductID id = product_id_of_symbol(msg.stock, 6);
        if(id != Product::invalid_product_id) {
          uint32_t order_id = parse_number(msg.order_ref, sizeof(msg.order_ref));
          uint64_t shares = parse_number(msg.shares, sizeof(msg.shares));
          uint64_t price = parse_number(msg.price, sizeof(msg.price));

          if (shares > 4294967295) {
            m_logger->log_printf(Log::ERROR, "%s: (order_add_message_long) Order size too big for uint32_t: %lu",
                                 where, shares);
            shares = 4294967295;
          }

          add_order(context, t, order_id, id, msg.side, price, shares);
        }
      }
      break;
      case 'E': // order_exec_message
      {
        SIZE_CHECK(chix_MMD::order_exec_message);
        const chix_MMD::order_exec_message& msg = reinterpret_cast<const chix_MMD::order_exec_message&>(*(buf+buf_posn));

        uint32_t order_id = parse_number(msg.order_ref, sizeof(msg.order_ref));
        uint32_t shares = parse_number(msg.exec_shares, sizeof(msg.exec_shares));
        char tick_direction = msg.tick_direction;

        if(m_order_book) {
          bool order_found = false;
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              order_found = true;
              if(m_qbdf_builder) {
                m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                if (m_use_exch_time) {
                  m_micros_recv_time = m_micros_exch_time;
                }
                m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                           m_micros_recv_time, m_micros_exch_time_delta,
                                           m_qbdf_exch_char, order->flip_side(),
                                           tick_direction, shares, order->price);
              }

              m_trade.m_id = order->product_id;
              m_trade.m_price = order->price_as_double();
              m_trade.set_side(order->flip_side());

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
      case 'e': // order_exec_message_long
      {
        SIZE_CHECK(chix_MMD::order_exec_message_long);
        const chix_MMD::order_exec_message_long& msg = reinterpret_cast<const chix_MMD::order_exec_message_long&>(*(buf+buf_posn));

        uint32_t order_id = parse_number(msg.order_ref, sizeof(msg.order_ref));
        uint64_t shares = parse_number(msg.exec_shares, sizeof(msg.exec_shares));
        char tick_direction = msg.tick_direction;

        if (shares > 4294967295) {
          m_logger->log_printf(Log::ERROR, "%s: (order_exec_message_long) Order size too big for uint32_t: %lu",
                               where, shares);
          shares = 4294967295;
        }

        if(m_order_book) {
          bool order_found = false;
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              order_found = true;
              if(m_qbdf_builder) {
                m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                if (m_use_exch_time) {
                  m_micros_recv_time = m_micros_exch_time;
                }
                m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                           m_micros_recv_time, m_micros_exch_time_delta,
                                           m_qbdf_exch_char, order->flip_side(),
                                           tick_direction, shares, order->price);
              }

              m_trade.m_id = order->product_id;
              m_trade.m_price = order->price_as_double();
              m_trade.set_side(order->flip_side());

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
      case 'X': // order_cancel_message
      {
        SIZE_CHECK(chix_MMD::order_cancel_message);
        const chix_MMD::order_cancel_message& msg = reinterpret_cast<const chix_MMD::order_cancel_message&>(*(buf+buf_posn));

        uint32_t order_id = parse_number(msg.order_ref, sizeof(msg.order_ref));
        uint32_t cancelled_shares = parse_number(msg.cancelled_shares, sizeof(msg.cancelled_shares));

        reduce_size(context, t, order_id, cancelled_shares);
      }
      break;
      case 'x': // order_cancel_message_long
      {
        SIZE_CHECK(chix_MMD::order_cancel_message_long);
        const chix_MMD::order_cancel_message_long& msg = reinterpret_cast<const chix_MMD::order_cancel_message_long&>(*(buf+buf_posn));

        uint32_t order_id = parse_number(msg.order_ref, sizeof(msg.order_ref));
        uint64_t cancelled_shares = parse_number(msg.cancelled_shares, sizeof(msg.cancelled_shares));

        reduce_size(context, t, order_id, cancelled_shares);
      }
      break;
      case 'P': // trade_message
      {
        SIZE_CHECK(chix_MMD::trade_message);
        const chix_MMD::trade_message& msg = reinterpret_cast<const chix_MMD::trade_message&>(*(buf+buf_posn));

        ProductID id = product_id_of_symbol(msg.stock, 6);
        if(id != Product::invalid_product_id) {
          //uint32_t order_id = parse_number(msg.order_ref, sizeof(msg.order_ref));
          uint32_t shares = parse_number(msg.shares, sizeof(msg.shares));
          uint32_t price = parse_number(msg.price, sizeof(msg.price));
          MDPrice md_price = MDPrice::from_iprice32(price);

          m_trade.m_id = id;
          m_trade.set_time(t);
          m_trade.m_price = md_price.fprice();
          m_trade.m_size = shares;
          m_trade.set_side(0); // side is obfuscated

          if (m_qbdf_builder) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            if (m_use_exch_time) {
              m_micros_recv_time = m_micros_exch_time;
            }
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                  m_micros_exch_time_delta, m_qbdf_exch_char,
                                  ' ', '0', "    ", shares, md_price, false);
          } else {
            send_trade_update();
          }
        }
      }
      break;
      case 'p': // trade_message_long
      {
        SIZE_CHECK(chix_MMD::trade_message_long);
        const chix_MMD::trade_message_long& msg = reinterpret_cast<const chix_MMD::trade_message_long&>(*(buf+buf_posn));

        ProductID id = product_id_of_symbol(msg.stock, 6);
        if(id != Product::invalid_product_id) {
          //uint32_t order_id = parse_number(msg.order_ref, sizeof(msg.order_ref));
          uint64_t shares = parse_number(msg.shares, sizeof(msg.shares));
          uint64_t price = parse_number(msg.price, sizeof(msg.price));

          if (shares > 4294967295) {
            m_logger->log_printf(Log::ERROR, "%s: Trade size too big for uin32_t: %lu",
                                 where, shares);
            shares = 4294967295;
          }

          // add 8th decimal place, spec only has 7
          MDPrice md_price = MDPrice::from_iprice64(price * 10);

          m_trade.m_id = id;
          m_trade.set_time(t);
          m_trade.m_price = md_price.fprice();
          m_trade.m_size = shares;
          m_trade.set_side(0); // side is obfuscated

          if (m_qbdf_builder) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            if (m_use_exch_time) {
              m_micros_recv_time = m_micros_exch_time;
            }
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                  m_micros_exch_time_delta, m_qbdf_exch_char,
                                  ' ', '0', "    ", shares, md_price, false);
          } else {
            send_trade_update();
          }
        }
      }
      break;
      case 'B': // broken_trade_message
      {
        SIZE_CHECK(chix_MMD::broken_trade_message);
       // const chix_MMD::broken_trade_message& msg = reinterpret_cast<const chix_MMD::broken_trade_message&>(*(buf+buf_posn));

        // Need to track trade reference numbers to know what stock is affected
      }
      break;
      case 'H': // stock_status_message
      {
        SIZE_CHECK(chix_MMD::stock_status_message);
        const chix_MMD::stock_status_message& msg = reinterpret_cast<const chix_MMD::stock_status_message&>(*(buf+buf_posn));

        ProductID id = product_id_of_symbol(msg.stock, 6);
        if(id != Product::invalid_product_id) {
          m_msu.m_id = id;

          switch(msg.trading_state) {
          case 'H':
            m_msu.m_market_status = MarketStatus::Halted;
            m_qbdf_status = QBDFStatusType::Halted;
            break;
          case 'T':
            m_msu.m_market_status = MarketStatus::Open;
            m_qbdf_status = QBDFStatusType::Open;
            break;
          default:
            break;
          }

          m_qbdf_status_reason = "";

          if (m_qbdf_builder) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            if (m_use_exch_time) {
              m_micros_recv_time = m_micros_exch_time;
            }
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time,
                                   m_micros_exch_time_delta, m_qbdf_exch_char,
                                   m_qbdf_status.index(), m_qbdf_status_reason);
          }

          if(m_subscribers) {
            m_subscribers->update_subscribers(m_msu);
          }
        }
      }
      break;
      case 'S': // System event messsage
      {
        SIZE_CHECK(chix_MMD::system_event_message);
	// Ignore these messages for now
      }
      break;
      default:
        send_alert("%s %s called with unknown message type %c",
                   where, m_name.c_str(), header.message_type);
        break;
      }

      if (m_qbdf_builder) {
        if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
          m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
          m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                               m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
          m_logger->sync_flush();
        }
      }

     return len_parsed;
    }
    return len;
  }

  size_t
  CHIX_MMD_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* packet_buf, size_t packet_len)
  {
    return 0;
  }

  void
  CHIX_MMD_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_qbdf_exch_char, -1);
      }

      m_order_book->clear(false);
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }

  void
  CHIX_MMD_Handler::reset(size_t context, const char* msg)
  {
    const char* where = "CHIX_MMD_Handler::reset";

    if (context != 0) {
      m_logger->log_printf(Log::ERROR, "%s: Bad context (%ld), skipping reset", where, context);
      return;
    }

    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    if(m_subscribers) {
      // nothing for now
    }

    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_qbdf_exch_char, context);
      }
    }
  }

  void
  CHIX_MMD_Handler::send_quote_update()
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
  CHIX_MMD_Handler::send_trade_update()
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
  CHIX_MMD_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);

    m_name = name;

    m_midnight = Time::today();

    if (exch == ExchangeID::CHIJ) {
      m_qbdf_exch_char = 'j';
    } else if (exch == ExchangeID::CHIX) {
      m_qbdf_exch_char = 'Z';
    } else {
      m_logger->log_printf(Log::ERROR, "CHIX_MMD_Handler::init: Unrecognized ExchangeID");
      m_qbdf_exch_char = ' ';
    }

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
  CHIX_MMD_Handler::start()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  CHIX_MMD_Handler::stop()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  CHIX_MMD_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
                                       const string& mdSymbol_,const string& mdExch_)
  {
    mutex_t::scoped_lock lock(m_mutex);

    char symbol[9];
    if(mdExch_.empty()) {
      snprintf(symbol, 9, "%s", mdSymbol_.c_str());
    } else {
      snprintf(symbol, 9, "%s.%s", mdSymbol_.c_str(), mdExch_.c_str());
    }

    uint64_t s = int_of_symbol(symbol, 8);

    if(m_prod_map.count(s) > 0) {
      m_logger->log_printf(Log::ERROR, "Mapping %s to %zd COLLIDES", symbol, s);
    }

    m_prod_map.insert(make_pair(s, id_));
  }

  CHIX_MMD_Handler::CHIX_MMD_Handler(Application* app) :
    Handler(app, "CHIX_MMD"),
    m_hist_mode(false)
  {
  }

  CHIX_MMD_Handler::~CHIX_MMD_Handler()
  {
  }


}
