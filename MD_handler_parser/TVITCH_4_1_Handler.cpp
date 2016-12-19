#include <Data/QBDF_Util.h>
#include <Data/TVITCH_4_1_Handler.h>
#include <Util/Network_Utils.h>

#include <Exchange/Exchange_Manager.h>
#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <MD/MDProvider.h>
#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>
#include <stdint.h>


#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)


namespace hf_core {

  namespace tvitch_41 {

    struct timestamp_message {
      char     message_type;
      uint32_t seconds;
    }  __attribute__ ((packed));

    struct system_event_message {
      char     message_type;
      uint32_t nanoseconds;
      char     event_code;
    } __attribute__ ((packed));

    struct stock_directory {
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     market_category;
      char     financial_status;
      uint32_t round_lot_size;
      char     route_lots_only;
    } __attribute__ ((packed));

    struct stock_trading_action {
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     trading_state;
      char     reserved;
      char     reason[4];
    } __attribute__ ((packed));

    struct reg_sho_restriction {
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     reg_sho_action;
    } __attribute__ ((packed));

    struct market_participant_position {
      char     message_type;
      uint32_t nanoseconds;
      char     mpid[4];
      char     stock[8];
      char     primary_market_maker;
      char     market_maker_mode;
      char     market_participant_state;
    } __attribute__ ((packed));


    struct add_order_no_mpid {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint32_t price;
    } __attribute__ ((packed));

    struct add_order_mpid {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint32_t price;
      char     attribution[4];
    } __attribute__ ((packed));

    struct order_executed {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t executed_shares;
      uint64_t match_number;
    } __attribute__ ((packed));

    struct order_executed_with_price {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t executed_shares;
      uint64_t match_number;
      char     printable;
      uint32_t execution_price;
    } __attribute__ ((packed));

    struct order_cancel_message {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
      uint32_t canceled_shares;
    } __attribute__ ((packed));

    struct order_delete_message {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_id;
    } __attribute__ ((packed));

    struct order_replace_message {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t orig_order_id;
      uint64_t new_order_id;
      uint32_t shares;
      uint32_t price;
    } __attribute__ ((packed));

    struct trade_message {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t order_ref_number;
      char     side;
      uint32_t shares;
      char     stock[8];
      uint32_t price;
      uint64_t match_number;
    } __attribute__ ((packed));

    struct cross_message {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t shares;
      char     stock[8];
      uint32_t cross_price;
      uint64_t match_number;
      char     cross_type;
    } __attribute__ ((packed));

    struct broken_trade_message {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t match_number;
    } __attribute__ ((packed));

    struct imbalance_message {
      char     message_type;
      uint32_t nanoseconds;
      uint64_t paired_shares;
      uint64_t imbalance_shares;
      char     imbalance_direction;
      char     stock[8];
      uint32_t far_price;
      uint32_t near_price;
      uint32_t ref_price;
      char     cross_type;
      char     price_variation_indicator;
    } __attribute__ ((packed));

    struct retail_price_improvement {
      char     message_type;
      uint32_t nanoseconds;
      char     stock[8];
      char     interest_flag;
    } __attribute__ ((packed));


  }

  /*
  static
  double
  raw_price_to_double(uint32_t p_n)
  {
    uint32_t p_h = ntohl(p_n);
    double price = ((double) p_h) / 10000.0;
    return price;
  }

  static
  float
  raw_price_to_float(uint32_t p_n)
  {
    uint32_t p_h = ntohl(p_n);
    float price = ((float) p_h) / 10000.0;
    return price;
  }
  */

  ProductID
  TVITCH_4_1_Handler::product_id_of_symbol(const char* symbol)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol, 8);
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string);
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol [%s]",
                             "TVITCH_4_1_Handler::product_id_of_symbol", symbol_string.c_str());
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      return symbol_id;
    }
    
    ProductID id = m_prod_map.find8(symbol);
    return id;
  }
  
  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  if(unlikely(len != sizeof(type))) {                                   \
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), len, sizeof(type)); \
    return len;                                                         \
  }                                                                     \
  
  size_t
  TVITCH_4_1_Handler::parse(size_t context, const char* buf, size_t len)
  {
    return parse_message(context, buf, len);
  }

  size_t
  TVITCH_4_1_Handler::parse_message(size_t, const char* buf, size_t len)
  {
    static const char* where = "TVITCH_4_1_Handler::parse";
    
    if(unlikely(len == 0)) {
      m_logger->log_printf(Log::ERROR, "%s %s called with len=0", where, m_name.c_str());
      return len;
    }
    
    if (m_qbdf_builder && !m_hist_mode) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                             m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }

    switch(buf[0]) {
    case 'T':
      {
        SIZE_CHECK(tvitch_41::timestamp_message);
        const tvitch_41::timestamp_message& msg = reinterpret_cast<const tvitch_41::timestamp_message&>(*buf);
        
        uint32_t sec = ntohl(msg.seconds);
        m_timestamp = m_midnight + seconds(sec);
        
        //compare_hw_time();
      }
      break;
    case 'S':
      {
        SIZE_CHECK(tvitch_41::system_event_message);
        const tvitch_41::system_event_message& msg = reinterpret_cast<const tvitch_41::system_event_message&>(*buf);

        if(msg.event_code == 'A') {
          send_alert("%s %s received system event message 'A' -- Emergency Market Condition - Halt", where, m_name.c_str());
        } else if(msg.event_code == 'R') {
          send_alert("%s %s received system event message 'A' -- Emergency Market Condition - Quote Only Period", where, m_name.c_str());
        } else if(msg.event_code == 'B') {
          send_alert("%s %s received system event message 'A' -- Emergency Market Condition - Resume", where, m_name.c_str());
        }
      }

      break;
    case 'R':
      {
        SIZE_CHECK(tvitch_41::stock_directory);
        //const tvitch_41::stock_directory& msg = reinterpret_cast<const tvitch_41::stock_directory&>(*buf);
      }
      break;
    case 'H':
      {
        SIZE_CHECK(tvitch_41::stock_trading_action);
        const tvitch_41::stock_trading_action& msg = reinterpret_cast<const tvitch_41::stock_trading_action&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        Time t = m_timestamp + Time_Duration(ns);

        ProductID id = product_id_of_symbol(msg.stock);
        if(id != Product::invalid_product_id) {
          m_msu.set_time(t);
          m_msu.m_id = id;

          switch(msg.trading_state) {
          case 'H':
            m_msu.m_market_status = MarketStatus::Halted;
            m_qbdf_status = QBDFStatusType::Halted;
            break;
          case 'R':
            m_msu.m_market_status = MarketStatus::Auction;
            m_qbdf_status = QBDFStatusType::Auction;
            break;
          case 'Q':
            m_msu.m_market_status = MarketStatus::Auction;
            m_qbdf_status = QBDFStatusType::Auction;
            break;
          case 'T':
            m_msu.m_market_status = MarketStatus::Open;
            m_qbdf_status = QBDFStatusType::Open;
            break;
          case 'V':
            m_msu.m_market_status = MarketStatus::Halted;
            m_qbdf_status = QBDFStatusType::Halted;
            break;
          default:
            m_msu.m_market_status = MarketStatus::Halted;
            m_qbdf_status = QBDFStatusType::Halted;
            break;
          }

          if (m_qbdf_builder and !m_exclude_status) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            if (m_hist_mode) {
              m_micros_recv_time = m_micros_exch_time;
              m_micros_exch_time_delta = 0;
            } else {
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            }

            if (m_search_orders.empty()) {
              m_qbdf_status_reason = string("NASDAQ Trading State ") + msg.trading_state;
              gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time,
                                     m_micros_exch_time_delta, m_exch_char,
                                     m_qbdf_status.index(), m_qbdf_status_reason);
            }
          }


          m_msu.m_reason.assign(msg.reason, 4);
          m_security_status[id] = m_msu.m_market_status;
          if(m_subscribers) {
            m_subscribers->update_subscribers(m_msu);
          }
          m_msu.m_reason.clear();
        }
      }
      break;
    case 'Y':
      {
        SIZE_CHECK(tvitch_41::reg_sho_restriction);
        const tvitch_41::reg_sho_restriction& msg = reinterpret_cast<const tvitch_41::reg_sho_restriction&>(*buf);

        ProductID id = product_id_of_symbol(msg.stock);
        if(id != Product::invalid_product_id) {
          SSRestriction rest;
          const char* action;
          switch(msg.reg_sho_action) {
          case '0':
            action = "No Short Sale Restriction";
            rest=SSRestriction::None;
            m_qbdf_status = QBDFStatusType::NoShortSaleRestrictionInEffect;
            break;
          case '1':
            action = "Short Sale Restriction Activated";
            rest=SSRestriction::Activated;
            m_qbdf_status = QBDFStatusType::ShortSaleRestrictActivated;
            break;
          case '2':
            action = "Short Sale Restriction Continued";
            rest=SSRestriction::Continued;
            m_qbdf_status = QBDFStatusType::ShortSaleRestrictContinued;
            break;
          default:
            action = "Unknown!";
            break;
          }

          if (m_qbdf_builder and !m_exclude_status) {
            uint32_t ns = ntohl(msg.nanoseconds);
            Time t = m_timestamp + Time_Duration(ns);
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            if (m_hist_mode) {
              m_micros_recv_time = m_micros_exch_time;
              m_micros_exch_time_delta = 0;
            } else {
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            }

            if (m_search_orders.empty()) {
              m_qbdf_status_reason = string("NASDAQ SSR Action ") + msg.reg_sho_action;
              gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time,
                                     m_micros_exch_time_delta, m_exch_char,
                                     m_qbdf_status.index(), m_qbdf_status_reason);
            }
          }

          const Product& prod = m_app->pm()->find_by_id(id);
          send_alert(AlertSeverity::LOW, "NASDAQ SSR symbol=%s %c %s",
                     prod.symbol().c_str(), msg.reg_sho_action, action);

          if (rest!=SSRestriction::Unknown) {
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
    case 'L':
      {
        SIZE_CHECK(tvitch_41::market_participant_position);
      }
      break;
    case 'A':
      {
        SIZE_CHECK(tvitch_41::add_order_no_mpid);
        const tvitch_41::add_order_no_mpid& msg = reinterpret_cast<const tvitch_41::add_order_no_mpid&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);

        ProductID id = product_id_of_symbol(msg.stock);
        if(likely(id != Product::invalid_product_id)) {
          uint64_t order_id = ntohll(msg.order_id);
          uint32_t shares = ntohl(msg.shares);
          MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));

          Time t = m_timestamp + Time_Duration(ns);
          if(likely(m_order_book)) {
            if(m_qbdf_builder && !m_exclude_level_2) {
              if (m_search_orders.empty()) {
                m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                if (m_hist_mode) {
                  m_micros_recv_time = m_micros_exch_time;
                  m_micros_exch_time_delta = 0;
                } else {
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                }

                gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                          m_micros_exch_time_delta, m_exch_char,
                                          msg.side, shares, price);
              } else {
                if (check_order_id(order_id)) {
#ifdef CW_DEBUG
                  cout << "Found order id " << order_id << " (add_order) received at " << Time::current_time().to_string() << " with exchange timestamp " << t.to_string() << endl;
#endif
                }
              }
            }
            
            bool inside_changed = m_order_book->add_order_ip(t, order_id, id, msg.side, price, shares);
            
            if(inside_changed) {
              m_quote.set_time(t);
              m_quote.m_id = id;
              if (!m_qbdf_builder) {
                send_quote_update();
              }
            }
          }
        }
      }
      break;
    case 'F':
      {
        SIZE_CHECK(tvitch_41::add_order_mpid);
        const tvitch_41::add_order_mpid& msg = reinterpret_cast<const tvitch_41::add_order_mpid&>(*buf);

        ProductID id = product_id_of_symbol(msg.stock);
        if(likely(id != Product::invalid_product_id)) {
          uint64_t order_id = ntohll(msg.order_id);
          uint32_t shares = ntohl(msg.shares);
          MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));

          uint32_t ns = ntohl(msg.nanoseconds);
          Time t = m_timestamp + Time_Duration(ns);
          
          if(likely(m_order_book)) {
            if(m_qbdf_builder && !m_exclude_level_2) {
              m_micros_exch_time = t.total_usec() - Time::today().total_usec();
              if (m_hist_mode) {
                m_micros_recv_time = m_micros_exch_time;
                m_micros_exch_time_delta = 0;
              } else {
                m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              }

              if (m_search_orders.empty()) {
                gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                          m_micros_exch_time_delta, m_exch_char,
                                          msg.side, shares, price);
              } else {
                if (check_order_id(order_id)) {
#ifdef CW_DEBUG
                  cout << "Found order id " << order_id << " (add_order_mpid) received at " << Time::current_time().to_string() << " with exchange timestamp " << t.to_string() << endl;
#endif
                }
              }
            }

            bool inside_changed = m_order_book->add_order_ip(t, order_id, id, msg.side, price, shares);

            if(inside_changed) {
              m_quote.set_time(t);
              m_quote.m_id = id;
              if (!m_qbdf_builder) {
                send_quote_update();
              }
            }
          }
        }
      }
      break;
    case 'E':
      {
        SIZE_CHECK(tvitch_41::order_executed);
        const tvitch_41::order_executed& msg = reinterpret_cast<const tvitch_41::order_executed&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        Time t = m_timestamp + Time_Duration(ns);

        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.executed_shares);

        if(m_order_book) {
          bool order_found = false;
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
            
            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              if(m_qbdf_builder && !m_exclude_level_2) {
                m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                if (m_hist_mode) {
                  m_micros_recv_time = m_micros_exch_time;
                  m_micros_exch_time_delta = 0;
                } else {
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                }

                if (m_search_orders.empty()) {
                  gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                             m_micros_recv_time, m_micros_exch_time_delta,
                                             m_exch_char, order->flip_side(), ' ',
                                             shares, order->price);
                } else {
                  if (check_order_id(order_id)) {
#ifdef CW_DEBUG
                    cout << "Found order id " << order_id << " (order_executed) received at " << Time::current_time().to_string() << " with exchange timestamp " << t.to_string() << endl;
#endif
                  }
                }
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
            if (!m_qbdf_builder) {
              send_trade_update();
            }

            if(inside_changed) {
              m_quote.set_time(t);
              m_quote.m_id = m_trade.m_id;
              if (!m_qbdf_builder) {
                send_quote_update();
              }
            }
            if(m_order_book->orderbook_update()) {
              m_order_book->post_orderbook_update();
            }
          }
        }
      }
      break;
    case 'C':
      {
        SIZE_CHECK(tvitch_41::order_executed_with_price);
        const tvitch_41::order_executed_with_price& msg = reinterpret_cast<const tvitch_41::order_executed_with_price&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        Time t = m_timestamp + Time_Duration(ns);

        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.executed_shares);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.execution_price));

        if(m_order_book) {
          bool order_found = false;
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              uint32_t new_size = order->size - shares;
              if(m_qbdf_builder && !m_exclude_level_2) {
                m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                if (m_hist_mode) {
                  m_micros_recv_time = m_micros_exch_time;
                  m_micros_exch_time_delta = 0;
                } else {
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                }

                if (m_search_orders.empty()) {
                  if(msg.printable == 'Y') {
                    gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                               m_micros_recv_time, m_micros_exch_time_delta,
                                               m_exch_char, order->flip_side(), msg.printable,
                                               shares, price);
                  } else {
                    gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                                 m_micros_recv_time, m_micros_exch_time_delta,
                                                 m_exch_char, order->side(), new_size, price);
                  }
                } else {
                  if (check_order_id(order_id)) {
#ifdef CW_DEBUG
                    cout << "Found order id " << order_id << " (order_executed_with_price) received at " << Time::current_time().to_string() << " with exchange timestamp " << t.to_string() << endl;
#endif
                  }
                }
              }
              
              order_found = true;
              inside_changed = order->is_top_level();
              m_trade.m_id = order->product_id;
              m_trade.set_side(order->flip_side());
              m_trade.clear_flags();
              m_trade.set_exec_flag(TradeUpdate::exec_flag_exec_w_price);
              
              MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
              if(new_size > 0) {
                m_order_book->unlocked_modify_order(t, order, new_size, shares);
              } else {
                m_order_book->unlocked_remove_order(t, order, shares);
              }
            }
          }

          if(order_found && msg.printable != 'N') {
            m_trade.set_time(t);
            m_trade.m_price = price.fprice();
            m_trade.m_size = shares;
            if (!m_qbdf_builder) {
              send_trade_update();
            }
          }
          if(inside_changed) {
            m_quote.set_time(t);
            m_quote.m_id = m_trade.m_id;
            if (!m_qbdf_builder) {
              send_quote_update();
            }
          }
          if(m_order_book->orderbook_update()) {
            m_order_book->post_orderbook_update();
          }
        }
      }
      break;
    case 'X':
      {
        SIZE_CHECK(tvitch_41::order_cancel_message);
        const tvitch_41::order_cancel_message& msg = reinterpret_cast<const tvitch_41::order_cancel_message&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        Time t = m_timestamp + Time_Duration(ns);

        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.canceled_shares);

        if(m_order_book) {
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              uint32_t new_size = order->size - shares;
              if(m_qbdf_builder && !m_exclude_level_2) {
                m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                if (m_hist_mode) {
                  m_micros_recv_time = m_micros_exch_time;
                  m_micros_exch_time_delta = 0;
                } else {
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                }

                if (m_search_orders.empty()) {
                  if (new_size > 0) {
                    gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                                 m_micros_recv_time, m_micros_exch_time_delta,
                                                 m_exch_char, order->side(), new_size,
                                                 order->price);
                  } else {
                    gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                                 m_micros_recv_time, m_micros_exch_time_delta,
                                                 m_exch_char, order->side(), 0, MDPrice());
                  }
                } else {
                  if (check_order_id(order_id)) {
#ifdef CW_DEBUG
                    cout << "Found order id " << order_id << " (order_cancel_message) received at " << Time::current_time().to_string() << " with exchange timestamp " << t.to_string() << endl;
#endif
                  }
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
            if (!m_qbdf_builder) {
              send_quote_update();
            }
          }
          if(m_order_book->orderbook_update()) {
            m_order_book->post_orderbook_update();
          }
        }
      }
      break;
    case 'D':
      {
        SIZE_CHECK(tvitch_41::order_delete_message);
        const tvitch_41::order_delete_message& msg = reinterpret_cast<const tvitch_41::order_delete_message&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        Time t = m_timestamp + Time_Duration(ns);

        uint64_t order_id = ntohll(msg.order_id);

        if(m_order_book) {
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

            MDOrder* order = m_order_book->unlocked_find_order(order_id);
            if(order) {
              if(m_qbdf_builder && !m_exclude_level_2) {
                m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                if (m_hist_mode) {
                  m_micros_recv_time = m_micros_exch_time;
                  m_micros_exch_time_delta = 0;
                } else {
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                }

                if (m_search_orders.empty()) {
                  gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                               m_micros_recv_time, m_micros_exch_time_delta,
                                               m_exch_char, order->side(), 0, MDPrice());
                } else {
                  if (check_order_id(order_id)) {
#ifdef CW_DEBUG
                    cout << "Found order id " << order_id << " (order_delete_message) received at " << Time::current_time().to_string() << " with exchange timestamp " << t.to_string() << endl;
#endif
                  }
                }
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
            if (!m_qbdf_builder) {
              send_quote_update();
            }
          }
          if(m_order_book->orderbook_update()) {
            m_order_book->post_orderbook_update();
          }
        }
      }
      break;
    case 'U':
      {
        SIZE_CHECK(tvitch_41::order_replace_message);
        const tvitch_41::order_replace_message& msg = reinterpret_cast<const tvitch_41::order_replace_message&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        Time t = m_timestamp + Time_Duration(ns);

        uint64_t orig_order_id = ntohll(msg.orig_order_id);
        uint64_t new_order_id = ntohll(msg.new_order_id);
        uint32_t shares = ntohl(msg.shares);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));

        if(m_order_book) {
          ProductID id = Product::invalid_product_id;
          bool inside_changed = false;
          {
            MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

            MDOrder* orig_order = m_order_book->unlocked_find_order(orig_order_id);
            if(orig_order) {
              if(m_qbdf_builder && !m_exclude_level_2) {
                m_micros_exch_time = t.total_usec() - Time::today().total_usec();
                if (m_hist_mode) {
                  m_micros_recv_time = m_micros_exch_time;
                  m_micros_exch_time_delta = 0;
                } else {
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                }

                if (m_search_orders.empty()) {
                  gen_qbdf_order_replace_micros(m_qbdf_builder, orig_order->product_id, orig_order_id,
                                                m_micros_recv_time, m_micros_exch_time_delta,
                                                m_exch_char, orig_order->side(), new_order_id,
                                                shares, price);
                } else {
                  if (check_order_id(orig_order_id)) {
#ifdef CW_DEBUG
                    cout << "Found orig order id " << orig_order_id << " (order_replace_message) at " << Time::current_time().to_string() << endl;
#endif
                  } else if (check_order_id(new_order_id)) {
#ifdef CW_DEBUG
                    cout << "Found new order id " << new_order_id << " (order_replace_message) at " << Time::current_time().to_string() << endl;
#endif
                  }
                }
              }
              
              id = orig_order->product_id;
              MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
              inside_changed = m_order_book->unlocked_replace_order_ip(t, orig_order, orig_order->side(),
                                                                       price, shares, new_order_id, false);
            }
          }

          if(inside_changed) {
            m_quote.set_time(t);
            m_quote.m_id = id;
            if (!m_qbdf_builder) {
              send_quote_update();
            }
          }
          if(m_order_book->orderbook_update()) {
            m_order_book->post_orderbook_update();
          }
        }
      }
      break;
    case 'P':
      {
        SIZE_CHECK(tvitch_41::trade_message);
        const tvitch_41::trade_message& msg = reinterpret_cast<const tvitch_41::trade_message&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        Time t = m_timestamp + Time_Duration(ns);

        m_trade.set_time(t);
        m_trade.m_id = product_id_of_symbol(msg.stock);

        if(m_trade.m_id != Product::invalid_product_id) {
          MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));
          m_trade.m_price = price.fprice();
          m_trade.m_size = ntohl(msg.shares);
          char side_c = ' ';
          if(!m_trade_has_no_side) {
            side_c = msg.side == 'B' ? 'S' : 'B';
          }
          m_trade.set_side(side_c);
          m_trade.clear_flags();
          m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);

          if (m_qbdf_builder && !m_exclude_trades) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            if (m_hist_mode) {
              m_micros_recv_time = m_micros_exch_time;
              m_micros_exch_time_delta = 0;
            } else {
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            }

            if (m_search_orders.empty()) {
              gen_qbdf_trade_micros(m_qbdf_builder, product_id_of_symbol(msg.stock),
                                    m_micros_recv_time, m_micros_exch_time_delta,
                                    m_exch_char, side_c, '0', "    ",
                                    ntohl(msg.shares), price);
            }
          } else {
            send_trade_update();
          }
        }
      }
      break;
    case 'Q':
      {
        SIZE_CHECK(tvitch_41::cross_message);
        const tvitch_41::cross_message& msg = reinterpret_cast<const tvitch_41::cross_message&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        Time t = m_timestamp + Time_Duration(ns);
        
        m_trade.set_time(t);
        m_trade.m_id = product_id_of_symbol(msg.stock);
        if(m_trade.m_id != Product::invalid_product_id) {
          m_msu.m_id = m_trade.m_id;
          MDPrice price = MDPrice::from_iprice32(ntohl(msg.cross_price));
          m_trade.m_price = price.fprice();
          m_trade.m_size = ntohll(msg.shares);
          m_trade.set_side(0);
          m_trade.clear_flags();
          m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
          
          bool send_msu = true;
          switch(msg.cross_type) {
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
          
          if (m_qbdf_builder && !m_exclude_trades) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            if (m_hist_mode) {
              m_micros_recv_time = m_micros_exch_time;
              m_micros_exch_time_delta = 0;
            } else {
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            }
            
            if (m_search_orders.empty()) {
              gen_qbdf_cross_trade_micros(m_qbdf_builder, product_id_of_symbol(msg.stock),
                                          m_micros_recv_time, m_micros_exch_time_delta,
                                          m_exch_char, ' ', ' ', msg.cross_type,
                                          ntohll(msg.shares), price);
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
          // use send_trade_update if we figure out how to do crosses
        }
      }
      break;
      case 'B':
      {
        SIZE_CHECK(tvitch_41::broken_trade_message);
        //const tvitch_41::broken_trade_message& msg = reinterpret_cast<const tvitch_41::broken_trade_message&>(*buf);

      }
      break;
    case 'I':
      {
        SIZE_CHECK(tvitch_41::imbalance_message);
        const tvitch_41::imbalance_message& msg = reinterpret_cast<const tvitch_41::imbalance_message&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        Time t = m_timestamp + Time_Duration(ns);

        ProductID id = product_id_of_symbol(msg.stock);
        if(id != Product::invalid_product_id) {
          m_auction.set_time(t);
          m_auction.m_id = id;

          MDPrice ref_price = MDPrice::from_iprice32(ntohl(msg.ref_price));
          MDPrice near_price = MDPrice::from_iprice32(ntohl(msg.near_price));
          MDPrice far_price = MDPrice::from_iprice32(ntohl(msg.far_price));

          m_auction.m_ref_price = ref_price.fprice();
          m_auction.m_far_price = far_price.fprice();
          m_auction.m_near_price = near_price.fprice();
          m_auction.m_paired_size = ntohll(msg.paired_shares);
          m_auction.m_imbalance_size = ntohll(msg.imbalance_shares);
          m_auction.m_imbalance_direction = msg.imbalance_direction;
          m_auction.m_indicative_price_flag = msg.cross_type;

          if (m_qbdf_builder && !m_exclude_imbalance) {
            m_micros_exch_time = t.total_usec() - Time::today().total_usec();
            if (m_hist_mode) {
              m_micros_recv_time = m_micros_exch_time;
              m_micros_exch_time_delta = 0;
            } else {
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            }

            if (m_search_orders.empty()) {
              gen_qbdf_imbalance_micros(m_qbdf_builder, id, m_micros_recv_time,
                                        m_micros_exch_time_delta, m_exch_char,
                                        ref_price, near_price, far_price,
                                        m_auction.m_paired_size, m_auction.m_imbalance_size,
                                        msg.imbalance_direction, msg.cross_type);
            }
          }

          if(m_subscribers) {
            m_subscribers->update_subscribers(m_auction);
          }
        }
      }
      break;
    case 'N':
      {
        //SIZE_CHECK(tvitch_41::retail_price_improvement);
        //const tvitch_41::retail_price_improvement& msg = reinterpret_cast<const tvitch_41::retail_price_improvement&>(*buf);
      }
      break;
    default:
      send_alert("%s %s called with unknown message type '%c'", where, m_name.c_str(), buf[0]);
      break;
    };

    if (m_qbdf_builder && m_hist_mode) {
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                             m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }

    return len;
  }

  size_t
  TVITCH_4_1_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
  {
    switch(buf[0]) {
    case 'T':
      {
        const tvitch_41::timestamp_message& msg = reinterpret_cast<const tvitch_41::timestamp_message&>(*buf);

        uint32_t sec = ntohl(msg.seconds);
        Time_Duration ts = seconds(sec);

        char time_buf[32];
        Time_Utils::print_time(time_buf, ts, Timestamp::MILI);

        log.printf("  timestamp %s\n", time_buf);
      }
      break;
    case 'P':
      {
        const tvitch_41::trade_message& msg = reinterpret_cast<const tvitch_41::trade_message&>(*buf);
        uint32_t ns = ntohl(msg.nanoseconds);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));

        log.printf("  trade  nanoseconds:%u  order_ref_num:%lu  side:%c  shares:%u  stock:%.8s  price:%0.4f  match_number:%lu\n",
                   ns, ntohll(msg.order_ref_number), msg.side, ntohl(msg.shares), msg.stock, price.fprice(), ntohll(msg.match_number));
        
      }
      break;
    case 'S':
      {
        const tvitch_41::system_event_message& msg = reinterpret_cast<const tvitch_41::system_event_message&>(*buf);
        log.printf("  system_event ns:%.9u event_code:%c\n", ntohl(msg.nanoseconds), msg.event_code);
      }
      break;
    case 'R':
      {
        //const tvitch_41::stock_directory& msg = reinterpret_cast<const tvitch_41::stock_directory&>(*buf);
      }
      break;
    case 'H':
      {

      }
      break;
    case 'A':
      {
        const tvitch_41::add_order_no_mpid& msg = reinterpret_cast<const tvitch_41::add_order_no_mpid&>(*buf);

        uint32_t ns = ntohl(msg.nanoseconds);
        uint64_t order_id = ntohll(msg.order_id);
        uint32_t shares = ntohl(msg.shares);
        MDPrice price = MDPrice::from_iprice32(ntohl(msg.price));

        log.printf("  add_order ns:%.9u order_id:%lu side:%c shares:%u stock:%.8s price:%0.4f\n",
                   ns, order_id, msg.side, shares, msg.stock, price.fprice());
      }
      break;
    case 'N':
      {
        const tvitch_41::retail_price_improvement& msg = reinterpret_cast<const tvitch_41::retail_price_improvement&>(*buf);
        uint32_t ns = ntohl(msg.nanoseconds);
        log.printf("  retail_price_improvement ns:%.9u stock:%.8s interest_flag:%c\n",
                   ns, msg.stock, msg.interest_flag);
      }
      break;
    default:
      log.printf("  message_type:%c not supported\n", buf[0]);
      break;
    }
    return len;
  }

  void
  TVITCH_4_1_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    if(m_order_book) {
      m_order_book->clear(false);
      if(m_qbdf_builder && m_micros_recv_time > 0) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_char, -1);
      }
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }
  
  void
  TVITCH_4_1_Handler::reset(size_t context, const char* msg, uint64_t expected_seq_no, uint64_t received_seq_no)
  {
    Handler::reset(context, msg, expected_seq_no, received_seq_no);
  }
  
  
  void
  TVITCH_4_1_Handler::send_quote_update()
  {
    ProductID id = m_quote.m_id;
    m_order_book->fill_quote_update(id, m_quote);
    m_msu.m_market_status = m_security_status[id];
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);
    if(m_msu.m_market_status != m_security_status[id]) {
      m_msu.m_id = id;
      m_security_status[id] = m_msu.m_market_status;
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
  TVITCH_4_1_Handler::send_trade_update()
  {
    ProductID id = m_trade.m_id;
    m_msu.m_market_status = m_security_status[id];
    m_feed_rules->apply_trade_conditions(m_trade, m_msu);
    if(m_msu.m_market_status != m_security_status[id]) {
      m_msu.m_id = id;
      m_security_status[id] = m_msu.m_market_status;
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
  TVITCH_4_1_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);
    m_name = name;
    
    m_midnight = Time::today();
    m_timestamp = Time::currentish_time();

    bool build_book = params.getDefaultBool("build_book", true);
    if(build_book) {
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_books.push_back(m_order_book);
      m_order_book->init(params);
    }
    
    string iso_date = Time::current_time().to_iso_date();
    if(iso_date >= "20140714") {
      m_trade_has_no_side = true;
      m_logger->log_printf(Log::INFO, "Date %s >= 20140714, trade_has_no_side enabled", iso_date.c_str());
    } else {
      m_trade_has_no_side = false;
      m_logger->log_printf(Log::INFO, "Date %s <  20140714, trade_has_no_side disabled", iso_date.c_str());
    }
    
    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;

    if (exch == ExchangeID::XBOS) {
      m_exch_char = 'B';
    } else {
      m_exch_char = 'Q';
    }

    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';
    
    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);
    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);
  }

  void
  TVITCH_4_1_Handler::start()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  TVITCH_4_1_Handler::stop()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }


  void
  TVITCH_4_1_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
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

  TVITCH_4_1_Handler::TVITCH_4_1_Handler(Application* app) :
    Handler(app, "TVITCH_4_1"),
    m_prod_map(262000),
    m_micros_exch_time_marker(0),
    m_hist_mode(false)
  {
  }

  TVITCH_4_1_Handler::~TVITCH_4_1_Handler()
  {
  }

}
