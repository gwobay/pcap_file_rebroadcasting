#include <Data/QBDF_Util.h>
#include <Data/DirectEdge_0_9_Handler.h>
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

  namespace edge_09 {

    struct message_header {
      uint16_t hdr_length;
      uint8_t  hdr_count;
      uint8_t  hdr_unit;
      uint32_t hdr_sequence;
    } __attribute__ ((packed));

    struct timestamp_message {
      uint8_t  length;
      char     message_type;
      uint64_t seconds;
    }  __attribute__ ((packed));

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
    } __attribute__ ((packed));

    struct add_order_attributed {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint64_t price;
      char     flags;
      char     attribution[4];
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
      uint64_t execution_price;
    } __attribute__ ((packed));

    struct reduce_size_long {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t canceled_shares;
    } __attribute__ ((packed));

    struct reduce_size_short {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint16_t canceled_shares;
    } __attribute__ ((packed));

    struct modify_order_long {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t shares;
      uint64_t price;
      char     flags;
    } __attribute__ ((packed));

    struct modify_order_short {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint16_t shares;
      uint16_t price;
      char     flags;
    } __attribute__ ((packed));

    struct delete_order {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
    } __attribute__ ((packed));

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
    } __attribute__ ((packed));

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

    struct broken_trade_message {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      uint64_t match_number;
    } __attribute__ ((packed));

    struct end_of_session {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
    } __attribute__ ((packed));

    struct security_status {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     issue_type;
      uint8_t  minimum_order_qty;
      uint8_t  round_lot_qty;
      char     tape_type;
      uint8_t  orderbook_number;
      char     halt_status;
      char     flags;
    } __attribute__ ((packed));

    struct system_status {
      uint8_t  length;
      char     message_type;
      uint32_t nanoseconds;
      char     status;
    } __attribute__ ((packed));

  }

  /*
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

  static
  float
  raw_price_to_float(uint32_t p_n)
  {
    float price = ((float) p_n) / 10000.0;
    return price;
  }
  */
  
  ProductID
  DirectEdge_0_9_Handler::product_id_of_symbol(const char* symbol, int len)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol, len);
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string.c_str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
                             "DirectEdge_0_9_Handler::product_id_of_symbol", symbol);
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }

      return symbol_id;
    }
    
    if(len == 6) {
      return m_prod_map.find6(symbol);
    } else {
      return m_prod_map.find8(symbol);
    }
  }


  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  if(len != sizeof(type)) {                                             \
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), len, sizeof(type)); \
    return len;                                                         \
  }                                                                     \

  size_t
  DirectEdge_0_9_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "DirectEdge_0_9_Handler::parse";

    if(len < sizeof(edge_09::message_header)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd", where, m_name.c_str(), len, sizeof(edge_09::message_header));
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

    const edge_09::message_header& header = reinterpret_cast<const edge_09::message_header&>(*buf);

    if(header.hdr_unit >= m_channel_info_map.size()) {
      m_channel_info_map.resize(header.hdr_unit + 1);
      m_channel_info_map[header.hdr_unit].context = context;
      m_logger->log_printf(Log::INFO, "%s: %s increasing channel_info_map due to new unit %d", where, m_name.c_str(), header.hdr_unit);
    }
    
    channel_info_t& ci = m_channel_info_map[header.hdr_unit];

    uint32_t seq_no = header.hdr_sequence;
    uint32_t last_seq_no = seq_no + header.hdr_count;

    if(seq_no == 0) {
      parse2(context, buf, len);
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
  DirectEdge_0_9_Handler::add_order(size_t context, Time t, uint64_t order_id, ProductID id, char side, uint64_t price32, uint32_t shares)
  {
    SEEN();
    if(m_order_book) {
      MDPrice price = MDPrice::from_iprice32(price32);
      bool inside_changed = m_order_book->add_order_ip(t, order_id, id, side, price, shares);

      if(m_qbdf_builder) {
        m_micros_exch_time = t.total_usec() - Time::today().total_usec();
        m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
        gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                  m_micros_exch_time_delta, m_exch_letter, side,
                                  shares, price);
      }

      if(inside_changed) {
        m_quote.set_time(t);
        m_quote.m_id = id;
        if(! m_qbdf_builder) {
          send_quote_update();
        }
      }
    }
  }

  void
  DirectEdge_0_9_Handler::reduce_size(size_t context, Time t, uint64_t order_id, uint32_t shares)
  {
    //SEEN();
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
                                           m_exch_letter, order->side(), new_size, order->price);
            } else {
              gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                           m_micros_recv_time, m_micros_exch_time_delta,
                                           m_exch_letter, order->side(), 0, MDPrice());
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
  DirectEdge_0_9_Handler::modify_order(size_t context, Time t, uint64_t order_id, uint32_t shares, uint64_t price32)
  {
    if(m_order_book) {
      ProductID id = Product::invalid_product_id;
      bool inside_changed = false;
      {
        MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

        MDOrder* order = m_order_book->unlocked_find_order(order_id);
        if(order) {
          MDPrice price = MDPrice::from_iprice32(price32);
          if(m_qbdf_builder) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                         m_micros_recv_time, m_micros_exch_time_delta,
                                         m_exch_letter, order->side(), shares, price);
          }

          id = order->product_id;
          MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
          inside_changed = m_order_book->unlocked_modify_order_ip(t, order, order->side(), price, shares);
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
  DirectEdge_0_9_Handler::parse2(size_t context, const char* packet_buf, size_t packet_len)
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

    const char* where = "DirectEdge_0_9_Handler::parse2";

    const edge_09::message_header& header = reinterpret_cast<const edge_09::message_header&>(*packet_buf);

    if(header.hdr_count == 0) {
      return packet_len;
    }
    
    uint32_t seq_no = header.hdr_sequence;
    int      message_count = header.hdr_count;
    
    channel_info_t& ci = m_channel_info_map[context];
    const char* buf = packet_buf + sizeof(edge_09::message_header);
    
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
    for(int i = 0; i < message_count; ++i, buf += len) {
      len =               buf[0];
      char message_type = buf[1];

      switch(buf[1]) {
      case 0x20: // timestamp
        {
          SIZE_CHECK(edge_09::timestamp_message);
          const edge_09::timestamp_message& msg = reinterpret_cast<const edge_09::timestamp_message&>(*buf);
          uint64_t sec = msg.seconds;
          ci.timestamp = Time(sec * Time_Constants::ticks_per_second);

          if (m_qbdf_builder) {
            m_micros_exch_time = ci.timestamp.total_usec() - Time::today().total_usec();
            m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
          }
        }
        break;
      case 0x21:  // add_order_long
        {
          SIZE_CHECK(edge_09::add_order_long);
          const edge_09::add_order_long& msg = reinterpret_cast<const edge_09::add_order_long&>(*buf);

          uint32_t ns = msg.nanoseconds;
          ProductID id = product_id_of_symbol(msg.stock, 6);
          if(id != Product::invalid_product_id) {
            uint64_t order_id = msg.order_id;
            uint32_t shares =   msg.shares;
            uint64_t price =    msg.price;
            Time t = ci.timestamp + Time_Duration(ns);
            add_order(context, t, order_id, id, msg.side, price, shares);
          }
        }
        break;
      case 0x22:  // add_order_short
        {
          SIZE_CHECK(edge_09::add_order_short);
          const edge_09::add_order_short& msg = reinterpret_cast<const edge_09::add_order_short&>(*buf);

          uint32_t ns = msg.nanoseconds;
          ProductID id = product_id_of_symbol(msg.stock, 6);
          if(id != Product::invalid_product_id) {
            uint64_t order_id = msg.order_id;
            uint16_t shares =   msg.shares;
            uint32_t price =    msg.price;
            price *= 100;
            Time t = ci.timestamp + Time_Duration(ns);
            add_order(context, t, order_id, id, msg.side, price, shares);
          }
        }
        break;
      case 0x2F:  // add_order_expanded
        {
          SIZE_CHECK(edge_09::add_order_expanded);
          const edge_09::add_order_expanded& msg = reinterpret_cast<const edge_09::add_order_expanded&>(*buf);

          uint32_t ns = msg.nanoseconds;
          ProductID id = product_id_of_symbol(msg.stock, 8);
          if(id != Product::invalid_product_id) {
            uint64_t order_id = msg.order_id;
            uint32_t shares =   msg.shares;
            uint64_t price =    msg.price;
            Time t = ci.timestamp + Time_Duration(ns);
            add_order(context, t, order_id, id, msg.side, price, shares);
          }
        }
        break;
      case 0x34:
        {
          SIZE_CHECK(edge_09::add_order_attributed);
          //const edge_09::add_order_attributed& msg = reinterpret_cast<const edge_09::add_order_attributed&>(*buf);
          /*
            Skip this message. it's currently a duplicate
          */
        }
        break;
      case 0x23:  // order executed
        {
          SIZE_CHECK(edge_09::order_executed);
          const edge_09::order_executed& msg = reinterpret_cast<const edge_09::order_executed&>(*buf);
          
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
                                             m_exch_letter, order->flip_side(), ' ',
                                             shares, order->price);
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
          SIZE_CHECK(edge_09::order_executed_with_price);
          const edge_09::order_executed_with_price& msg = reinterpret_cast<const edge_09::order_executed_with_price&>(*buf);

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
                  if(remaining_shares + shares != order->size) {
                    gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                                 m_micros_recv_time, m_micros_exch_time_delta,
                                                 m_exch_letter, order->side(),
                                                 remaining_shares + shares, price);
                  }
                  gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                             m_micros_recv_time, m_micros_exch_time_delta,
                                             m_exch_letter, order->flip_side(), ' ', shares, price);
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
          SIZE_CHECK(edge_09::reduce_size_long);
          const edge_09::reduce_size_long& msg = reinterpret_cast<const edge_09::reduce_size_long&>(*buf);

          uint32_t ns = msg.nanoseconds;
          uint64_t order_id = msg.order_id;
          uint32_t shares =   msg.canceled_shares;
          Time t = ci.timestamp + Time_Duration(ns);
          reduce_size(context, t, order_id, shares);
        }
        break;
      case 0x26: // reduce_size_short
        {
          SIZE_CHECK(edge_09::reduce_size_short);
          const edge_09::reduce_size_short& msg = reinterpret_cast<const edge_09::reduce_size_short&>(*buf);

          uint32_t ns = msg.nanoseconds;
          uint64_t order_id = msg.order_id;
          uint16_t shares =   msg.canceled_shares;
          Time t = ci.timestamp + Time_Duration(ns);
          reduce_size(context, t, order_id, shares);
        }
        break;
      case 0x27: // modify_order_long
        {
          SIZE_CHECK(edge_09::modify_order_long);
          const edge_09::modify_order_long& msg = reinterpret_cast<const edge_09::modify_order_long&>(*buf);

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
          SIZE_CHECK(edge_09::modify_order_short);
          const edge_09::modify_order_short& msg = reinterpret_cast<const edge_09::modify_order_short&>(*buf);

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
          SIZE_CHECK(edge_09::delete_order);
          const edge_09::delete_order& msg = reinterpret_cast<const edge_09::delete_order&>(*buf);

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
                                               m_exch_letter, order->side(), 0, MDPrice());
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
          SIZE_CHECK(edge_09::trade_long);
          const edge_09::trade_long& msg = reinterpret_cast<const edge_09::trade_long&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_trade.m_id = product_id_of_symbol(msg.stock, 6);
          if(m_trade.m_id != Product::invalid_product_id) {
            Time t = ci.timestamp + Time_Duration(ns);
            m_trade.set_time(t);
            MDPrice price = MDPrice::from_iprice32(msg.price);
            m_trade.m_price = price.fprice();
            m_trade.m_size = msg.shares;
            m_trade.set_side(0); // side is obfuscated
            m_trade.clear_flags();
            m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
            
            if (m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, m_exch_letter, ' ', '0',
                                    "    ", msg.shares, price);
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x2b: // trade_short
        {
          SIZE_CHECK(edge_09::trade_short);
          const edge_09::trade_short& msg = reinterpret_cast<const edge_09::trade_short&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_trade.m_id = product_id_of_symbol(msg.stock, 6);
          if(m_trade.m_id != Product::invalid_product_id) {
            Time t = ci.timestamp + Time_Duration(ns);
            m_trade.set_time(t);
            MDPrice price = MDPrice::from_iprice32((uint32_t)msg.price * 100);
            m_trade.m_price = price.fprice();
            m_trade.m_size = msg.shares;
            m_trade.set_side(0); // side is obfuscated
            m_trade.clear_flags();
            m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
            
            if (m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, m_exch_letter, ' ', '0',
                                    "    ", msg.shares, price);
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x30: // trade_expanded
        {
          SIZE_CHECK(edge_09::trade_expanded);
          const edge_09::trade_expanded& msg = reinterpret_cast<const edge_09::trade_expanded&>(*buf);

          uint32_t ns = msg.nanoseconds;
          m_trade.m_id = product_id_of_symbol(msg.stock, 8);
          if(m_trade.m_id != Product::invalid_product_id) {
            Time t = ci.timestamp + Time_Duration(ns);
            m_trade.set_time(t);
            MDPrice price = MDPrice::from_iprice32(msg.price);
            m_trade.m_price = price.fprice();
            m_trade.m_size = msg.shares;
            m_trade.set_side(0); // side is obfuscated
            m_trade.clear_flags();
            m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
            
            if (m_qbdf_builder) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, m_micros_recv_time,
                                    m_micros_exch_time_delta, m_exch_letter, ' ', '0',
                                    "    ", msg.shares, price);
            } else {
              send_trade_update();
            }
          }
        }
        break;
      case 0x2c: // broken trade
        {
          SIZE_CHECK(edge_09::broken_trade_message);
          //const edge_09::broken_trade_message& msg = reinterpret_cast<const edge_09::broken_trade_message&>(*buf);
        }
        break;
      case 0x2d: // end of session message
        {
          SIZE_CHECK(edge_09::end_of_session);
          send_alert("%s %s end of session", where, m_name.c_str());
        }
        break;
      case 0x2e:// security_status
        {
          SIZE_CHECK(edge_09::security_status);
          const edge_09::security_status& msg = reinterpret_cast<const edge_09::security_status&>(*buf);

          ProductID id = product_id_of_symbol(msg.stock, 8);
          if(id != Product::invalid_product_id) {
            m_msu.m_id = id;

            switch(msg.halt_status) {
            case 'H': m_msu.m_market_status = MarketStatus::Halted; break;
            case 'T': m_msu.m_market_status = MarketStatus::Open; break;
            default:
              break;
            }
            m_security_status[id] = m_msu.m_market_status;
            if(m_subscribers) {
              m_subscribers->update_subscribers(m_msu);
            }
          }
        }
        break;
      case 0x31: // system_status
        {
          SIZE_CHECK(edge_09::system_status);
          const edge_09::system_status& msg = reinterpret_cast<const edge_09::system_status&>(*buf);

          const char* text = 0;
          switch(msg.status) {
          case 'E': text = "Exchange is Open for Pre-Market Trading"; break;
          case 'N': text = "Exchange is Open for Normal Market Session"; break;
          case 'O': text = "Exchange is Open for Post-Market Trading"; break;
          case 'H': text = "Exchange is in Emergency Halt"; break;
          default:
            text = "Unknown message";
          }

          m_logger->log_printf(Log::INFO, "%s: %s  received system_status message %c - %s", where, m_name.c_str(), msg.status, text);
          if(msg.status == 'H') {
            send_alert("%s: %s  received system_status message %c - %s", where, m_name.c_str(), msg.status, text);
          }
        }
        break;
      default:
        send_alert("%s %s called with unknown message type '%c'", where, m_name.c_str(), message_type);
        break;
      }
    }
    return packet_len;
  }

  size_t
  DirectEdge_0_9_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* packet_buf, size_t packet_len)
  {
    const edge_09::message_header& header = reinterpret_cast<const edge_09::message_header&>(*packet_buf);

    if(header.hdr_count == 0) {
      return packet_len;
    }

    const char* buf = packet_buf + sizeof(edge_09::message_header);

    uint8_t len = buf[0];
    for(uint8_t i = 0; i < header.hdr_count; ++i, buf += len) {
      len =               buf[0];
      char message_type = buf[1];

      switch(buf[1]) {
      case 0x20: // timestamp
        {
          const edge_09::timestamp_message& msg = reinterpret_cast<const edge_09::timestamp_message&>(*buf);

          Time t(msg.seconds * Time_Constants::ticks_per_second);

          char buf[32];
          Time_Utils::print_time(buf, t, Timestamp::MICRO);

          log.printf("  timestamp              sec:%lu  %s\n", msg.seconds, buf);
        }
        break;
      case 0x21:  // add_order_long
        {
          const edge_09::add_order_long& msg = reinterpret_cast<const edge_09::add_order_long&>(*buf);
          IPrice64 price = IPrice64::from_iprice32(msg.price);

          log.printf("  add_order_long         ns:%09d order_id:%lu side:%c shares:%u stock:%.6s price:%0.4f display:%u replenish:%u\n",
                     msg.nanoseconds, msg.order_id, msg.side, msg.shares, msg.stock, price.fprice(), msg.flags & 0x1, msg.flags & 0x1<<2);
        }
        break;
      case 0x22:  // add_order_short
        {
          const edge_09::add_order_short& msg = reinterpret_cast<const edge_09::add_order_short&>(*buf);
          IPrice64 price = IPrice64::from_iprice32((uint32_t)msg.price*100);

          log.printf("  add_order_short        ns:%09d order_id:%lu side:%c shares:%u stock:%.6s price:%0.4f display:%u replenish:%u\n",
                     msg.nanoseconds, msg.order_id, msg.side, msg.shares, msg.stock, price.fprice(), msg.flags & 0x1, msg.flags & 0x1<<2);
        }
        break;
      case 0x2F:  // add_order_expanded
        {
          const edge_09::add_order_expanded& msg = reinterpret_cast<const edge_09::add_order_expanded&>(*buf);
          IPrice64 price = IPrice64::from_iprice32(msg.price);
          log.printf("  add_order_expanded     ns:%09d order_id:%lu side:%c shares:%u stock:%.8s price:%0.4f display:%u replenish:%u\n",
                     msg.nanoseconds, msg.order_id, msg.side, msg.shares, msg.stock, price.fprice(), msg.flags & 0x1, msg.flags & 0x1<<2);
        }
        break;
      case 0x34:
        {
          const edge_09::add_order_attributed& msg = reinterpret_cast<const edge_09::add_order_attributed&>(*buf);
          IPrice64 price = IPrice64::from_iprice32(msg.price);

          log.printf("  add_order_attributed   ns:%09d order_id:%lu side:%c shares:%u stock:%.8s price:%0.4f attr:%.4s display:%u replenish:%u\n",
                     msg.nanoseconds, msg.order_id, msg.side, msg.shares, msg.stock, price.fprice(), msg.attribution, msg.flags & 0x1, msg.flags & 0x1<<2);
        }
        break;
      case 0x23:  // order executed
        {
          const edge_09::order_executed& msg = reinterpret_cast<const edge_09::order_executed&>(*buf);
          log.printf("  executed               ns:%09d order_id:%lu exec_shares:%u match_number:%lu\n",
                     msg.nanoseconds, msg.order_id, msg.executed_shares, msg.match_number);
        }
        break;
      case 0x24: // order executed with price
        {
          const edge_09::order_executed_with_price& msg = reinterpret_cast<const edge_09::order_executed_with_price&>(*buf);
          IPrice64 price = IPrice64::from_iprice32(msg.execution_price);
          log.printf("  executed_with_price    ns:%09d order_id:%lu exec_shares:%u remaining_shares:%u match_number:%lu execution_price:%0.4f\n",
                     msg.nanoseconds, msg.order_id, msg.executed_shares, msg.remaining_shares, msg.match_number, price.fprice());
        }
        break;
      case 0x25: // reduce_size_long
        {
          const edge_09::reduce_size_long& msg = reinterpret_cast<const edge_09::reduce_size_long&>(*buf);
          log.printf("  reduce_size_long       ns:%09d order_id:%lu canceled_shares:%u\n",
                     msg.nanoseconds, msg.order_id, msg.canceled_shares);
        }
        break;
      case 0x26: // reduce_size_short
        {
          const edge_09::reduce_size_short& msg = reinterpret_cast<const edge_09::reduce_size_short&>(*buf);
          log.printf("  reduce_size_long       ns:%09d order_id:%lu canceled_shares:%u\n",
                     msg.nanoseconds, msg.order_id, msg.canceled_shares);
        }
        break;
      case 0x27: // modify_order_long
        {
          const edge_09::modify_order_long& msg = reinterpret_cast<const edge_09::modify_order_long&>(*buf);
          IPrice64 price = IPrice64::from_iprice32(msg.price);
          log.printf("  modify_order_long      ns:%09d order_id:%lu shares:%u price:%0.4f priority:%u\n",
                     msg.nanoseconds, msg.order_id, msg.shares, price.fprice(), msg.flags&0x1);
        }
        break;
      case 0x28: // modify_order_short
        {
          const edge_09::modify_order_short& msg = reinterpret_cast<const edge_09::modify_order_short&>(*buf);
          IPrice64 price = IPrice64::from_iprice32((uint32_t)msg.price * 100);
          log.printf("  modify_order_short     ns:%09d order_id:%lu shares:%u price:%0.4f priority:%u\n",
                     msg.nanoseconds, msg.order_id, msg.shares, price.fprice(), msg.flags&0x1);
        }
        break;
      case 0x29:  // delete_order
        {
          const edge_09::delete_order& msg = reinterpret_cast<const edge_09::delete_order&>(*buf);
          log.printf("  delete_order           ns:%09d order_id:%lu\n",
                     msg.nanoseconds, msg.order_id);
        }
        break;
      case 0x2a: // trade_long
        {
          const edge_09::trade_long& msg = reinterpret_cast<const edge_09::trade_long&>(*buf);
          IPrice64 price = IPrice64::from_iprice32((uint64_t)msg.price);
          log.printf("  trade_long             ns:%09d order_ref_number:%lu side:%c shares:%u stock:%.6s price:%0.4f match_number:%lu\n",
                     msg.nanoseconds, msg.order_ref_number, msg.side, msg.shares, msg.stock, price.fprice(), msg.match_number);
        }
        break;
      case 0x2b: // trade_short
        {
          const edge_09::trade_short& msg = reinterpret_cast<const edge_09::trade_short&>(*buf);
          IPrice64 price = IPrice64::from_iprice32((uint32_t)msg.price * 100);
          log.printf("  trade_short            ns:%09d order_ref_number:%lu side:%c shares:%u stock:%.6s price:%0.4f match_number:%lu\n",
                     msg.nanoseconds, msg.order_ref_number, msg.side, msg.shares, msg.stock, price.fprice(), msg.match_number);
        }
        break;
      case 0x30: // trade_expanded
        {
          const edge_09::trade_expanded& msg = reinterpret_cast<const edge_09::trade_expanded&>(*buf);
          IPrice64 price = IPrice64::from_iprice32((uint64_t)msg.price);
          log.printf("  trade_expanded         ns:%09d order_ref_number:%lu side:%c shares:%u stock:%.8s price:%0.4f match_number:%lu\n",
                     msg.nanoseconds, msg.order_ref_number, msg.side, msg.shares, msg.stock, price.fprice(), msg.match_number);
        }
        break;
      case 0x2c: // broken trade
        {
          const edge_09::broken_trade_message& msg = reinterpret_cast<const edge_09::broken_trade_message&>(*buf);
          log.printf("  broke_trade_message     ns:%09d match_number:%lu\n",
                     msg.nanoseconds, msg.match_number);
        }
        break;
      case 0x2d: // end of session message
        {
          log.printf("  end of session");
        }
        break;
      case 0x2e:// security_status
        {
          const edge_09::security_status& msg = reinterpret_cast<const edge_09::security_status&>(*buf);
          log.printf("  security_status       ns:%09d stock:%.8s issue_type:%c minimum_order_qty:%u round_lot_qty:%u tape_type:%c orderbook_number:%u halt_status:%c flags:%u\n",
                     msg.nanoseconds, msg.stock, msg.issue_type, msg.minimum_order_qty, msg.round_lot_qty, msg.tape_type, msg.orderbook_number, msg.halt_status, msg.flags);
        }
        break;
      case 0x31: // system_status
        {
          const edge_09::system_status& msg = reinterpret_cast<const edge_09::system_status&>(*buf);
          log.printf("  system_status         ns:%09d status:%c\n",
                     msg.nanoseconds, msg.status);
        }
        break;
      default:
        {
          log.printf("Unknown message type '%x'\n", message_type);
        }
        break;
      }
    }
    return packet_len;
  }

  void
  DirectEdge_0_9_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_letter, -1);
      }

      m_order_book->clear(false);
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }

  void
  DirectEdge_0_9_Handler::reset(size_t context, const char* msg)
  {
    channel_info_t& ci = m_channel_info_map[context];

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
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_letter, context);
      }

      for(vector<ProductID>::const_iterator p = ci.clear_products.begin(), p_end = ci.clear_products.end();
          p != p_end; ++p) {
        m_order_book->clear(*p, false);
        m_seen_product.reset(*p);
      }
    }

    m_logger->log_printf(Log::WARN, "DirectEdge_0_9_Handler::reset %s called with %zd products", m_name.c_str(), ci.clear_products.size());

    ci.clear_products.clear();
  }

  void
  DirectEdge_0_9_Handler::send_quote_update()
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
  DirectEdge_0_9_Handler::send_trade_update()
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
  DirectEdge_0_9_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
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
    if (exch == ExchangeID::EDGA) {
      m_exch_letter = 'J';
    } else {
      m_exch_letter = 'K';
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
  DirectEdge_0_9_Handler::start()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  DirectEdge_0_9_Handler::stop()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  DirectEdge_0_9_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
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

  DirectEdge_0_9_Handler::DirectEdge_0_9_Handler(Application* app) :
    Handler(app, "DirectEdge_0_9"),
    m_prod_map(262000),
    m_micros_exch_time_marker(0)
  {
  }

  DirectEdge_0_9_Handler::~DirectEdge_0_9_Handler()
  {
  }


}
