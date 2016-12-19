#include <boost/lexical_cast.hpp>

#include <Data/Eurex_Handler.h>
#include <Util/Time.h>

namespace hf_core {

  static FactoryRegistrar1<std::string, Handler, Application*, Eurex_Handler> r1("Eurex_Handler");

  static inline char char_side(Eurex::EnumSide::Side s) {
    return s == Eurex::EnumSide::Buy ? 'B' : 'S';
  }

  void Eurex_Handler::set_times(uint64_t exch_time_nanos) {
    if (exch_time_nanos < m_midnight.total_nsec()) {
      // TODO: is this ok?
      // this is kind of a weird case -- OrderAdd whose time priority is from a previous date.
      // for OrderAdd whose time priority is from a previous date, set to 1 micro sec after midnight
      m_micros_exch_time = 1;
    } else {
      m_micros_exch_time = (exch_time_nanos - m_midnight.total_nsec()) / Time_Constants::ticks_per_micro;
    }
    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
  }

  Eurex_Handler::Eurex_Handler(Application* app) :
  Handler(app, "Eurex"), m_name("Eurex"),
    m_qbdf_exch_char('e'),
    m_ref_data_mode(false),
    m_last_packet_complete(1),
    m_order_id_hash(static_cast<size_t>(10000000UL)),
    m_order_id(0)
  { }

    size_t Eurex_Handler::parse(size_t context, const char* buf, size_t len)
    {
      if (m_qbdf_builder) {
        m_micros_recv_time = Time::current_time().total_usec() - m_midnight.total_usec();
      }

      size_t sofar = 0;

      if(m_drop_packets) {
        --m_drop_packets;
        return 0;
      }

      // ref data mode does not work at the moment.
      if (m_ref_data_mode) {
      } else {
        const Eurex::PacketHeader& head = reinterpret_cast<const Eurex::PacketHeader&>(*buf);

        if(context >= m_channel_info_map.size()) {
          m_logger->log_printf(Log::INFO, "%s: %s increasing channel_info_map due to new unit %lu", __PRETTY_FUNCTION__, m_name.c_str(), context);
          while(context >= m_channel_info_map.size()) {
            m_channel_info_map.push_back(channel_info("", m_channel_info_map.size()));
          }
        }

        channel_info& ci = m_channel_info_map[context];



        uint32_t seq_no = head.ApplSeqNum;
        uint32_t next_seq_no = seq_no + 1;

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

        m_market_segment_id = head.MarketSegmentID;

        sofar += parse2(context, buf, len);

        ci.seq_no = next_seq_no;
        ci.last_update = Time::currentish_time();

        if(!ci.queue.empty()) {
          ci.process_recovery(this);
        }
      }
      return sofar;
    }

    size_t Eurex_Handler::parse2(size_t context, const char* packet_buf, size_t packet_len)
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

      size_t sofar = 0;
      const char* buf = packet_buf + sizeof(Eurex::PacketHeader);
      size_t len = packet_len - sizeof(Eurex::PacketHeader);

      channel_info& ci = m_channel_info_map[context];

      while (sofar < len) {
        const Eurex::MessageHeader& head = reinterpret_cast<const Eurex::MessageHeader&>(*(buf + sofar));

      switch (head.TemplateID) {
        case Eurex::EnumMessageType::OrderAdd:
          {
            const Eurex::OrderAdd& msg = reinterpret_cast<const Eurex::OrderAdd&>(*(buf + sofar));
            ProductID pid = product_id_of_symbol(msg.SecurityID);
            if(pid != Product::invalid_product_id) {
              set_times(msg.TrdRegTSTimeIn);

              uint32_t shares = msg.DisplayQty;
              MDPrice price = MDPrice::from_iprice64(msg.Price);
              char side = char_side(msg.Side);
              uint64_t order_id = get_unique_order_id(msg.TrdRegTSTimePriority, msg.SecurityID, side);

              Time t(msg.TrdRegTSTimeIn);
              ci.timestamp = t;

              if(m_order_book) {
                if(m_qbdf_builder) {
                  gen_qbdf_order_add_micros(m_qbdf_builder, pid, order_id, m_micros_recv_time,
                                            m_micros_exch_time_delta, m_qbdf_exch_char, side, shares, price);
                }

                bool inside_changed = m_order_book->add_order_ip(t, order_id, pid, side, price, shares);

                if(inside_changed) {
                  m_quote.set_time(t);
                  m_quote.m_id = pid;
                  send_quote_update();
                }
              }
            }
          }
          break;
        case Eurex::EnumMessageType::OrderModify:
          {
            const Eurex::OrderModify& msg = reinterpret_cast<const Eurex::OrderModify&>(*(buf + sofar));
            modify_order(context, msg.TrdRegTSTimeIn, msg.SecurityID, msg.Side, msg.TrdRegTSPrevTimePriority, msg.TrdRegTSTimePriority, msg.Price, msg.DisplayQty, ci);
          }

          break;
        case Eurex::EnumMessageType::OrderModifySamePrio:
          {
            const Eurex::OrderModifySamePrio& msg = reinterpret_cast<const Eurex::OrderModifySamePrio&>(*(buf + sofar));
            modify_order(context, msg.TrdRegTSTimeIn, msg.SecurityID, msg.Side, msg.TrdRegTSTimePriority, 0, msg.Price, msg.DisplayQty, ci);
          }
          break;
        case Eurex::EnumMessageType::OrderDelete:
          {


            uint64_t TrdRegTSTimeIn;
            int64_t SecurityID;
            Eurex::EnumSide::Side Side;
            uint64_t TrdRegTSTimePriority;

            if (m_qbdf_builder) {
              const Eurex::MessageHeader& head = reinterpret_cast<const Eurex::MessageHeader&>(*(buf + sofar));
              if (head.BodyLen == 56) { // 2.1+
                const Eurex::OrderDelete& msg = reinterpret_cast<const Eurex::OrderDelete&>(*(buf + sofar));
                TrdRegTSTimeIn = msg.TrdRegTSTimeIn;
                SecurityID = msg.SecurityID;
                Side = msg.Side;
                TrdRegTSTimePriority = msg.TrdRegTSTimePriority;
              } else {
                const Eurex::OrderDelete10& msg = reinterpret_cast<const Eurex::OrderDelete10&>(*(buf + sofar));
                TrdRegTSTimeIn = msg.TrdRegTSTimeIn;
                SecurityID = msg.SecurityID;
                Side = msg.Side;
                TrdRegTSTimePriority = msg.TrdRegTSTimePriority;
              }
            } else {
              const Eurex::OrderDelete& msg = reinterpret_cast<const Eurex::OrderDelete&>(*(buf + sofar));
              TrdRegTSTimeIn = msg.TrdRegTSTimeIn;
              SecurityID = msg.SecurityID;
              Side = msg.Side;
              TrdRegTSTimePriority = msg.TrdRegTSTimePriority;
            }

            set_times(TrdRegTSTimeIn);

            Time t(TrdRegTSTimeIn);
            ci.timestamp = t;

            if(m_order_book) {
              char side = char_side(Side);
              uint64_t order_id = get_unique_order_id(TrdRegTSTimePriority, SecurityID, side);
              bool inside_changed = false;
              ProductID pid = Product::invalid_product_id;
              {
                MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
                MDOrder* order = m_order_book->unlocked_find_order(order_id);
                if(order) {
                  pid = order->product_id;
                  if(m_qbdf_builder) {
                    gen_qbdf_order_modify_micros(m_qbdf_builder, pid, order_id,
                                                 m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                                                 order->side(), 0, MDPrice());
                  }

                  inside_changed = order->is_top_level();
                  if(inside_changed) {
                    m_quote.m_id = pid;
                  }
                  MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(pid).m_mutex, true);
                  m_order_book->unlocked_remove_order(t, order);
                } else {
                  send_alert("couldn't find order %lld to delete (made from %lu %lu %c)", order_id, TrdRegTSTimePriority, SecurityID, side);
                }
              }
              if(inside_changed) {
                m_quote.set_time(t);
                send_quote_update();
              }
            }
          }
          break;
        // partial and full order execution are handled the same
        case Eurex::EnumMessageType::FullOrderExecution:
        case Eurex::EnumMessageType::PartialOrderExecution:
          {
            // note: order execution does not contain the time of the execution: just the "time priority" of the
            // original order, which is part of the unique identifier of that order

            int64_t msg_SecurityID;
            Eurex::EnumSide::Side msg_Side;
            uint64_t msg_TrdRegTSTimePriority;
            int32_t msg_LastQty;

            if (head.TemplateID == Eurex::EnumMessageType::FullOrderExecution) {
              const Eurex::FullOrderExecution& msg = reinterpret_cast<const Eurex::FullOrderExecution&>(*(buf + sofar));
              msg_SecurityID = msg.SecurityID;
              msg_Side = msg.Side;
              msg_TrdRegTSTimePriority = msg.TrdRegTSTimePriority;
              msg_LastQty = msg.LastQty;
            } else { // case Eurex::EnumMessageType::PartialOrderExecution:
              const Eurex::PartialOrderExecution& msg = reinterpret_cast<const Eurex::PartialOrderExecution&>(*(buf + sofar));
              msg_SecurityID = msg.SecurityID;
              msg_Side = msg.Side;
              msg_TrdRegTSTimePriority = msg.TrdRegTSTimePriority;
              msg_LastQty = msg.LastQty;
            }

            m_micros_exch_time = m_micros_recv_time;
            m_micros_exch_time_delta = 0;
            ProductID pid = product_id_of_symbol(msg_SecurityID);
            if (pid != Product::invalid_product_id) {
              char side = char_side(msg_Side);
              uint64_t order_id = get_unique_order_id(msg_TrdRegTSTimePriority, msg_SecurityID, side);
              int64_t shares = msg_LastQty;

              //Time t(m_micros_exch_time * Time_Constants::ticks_per_micro); // maybe make this Time::current_time() or something
              Time t = Time::current_time();
              ci.timestamp = t;

              if(m_order_book) {
                bool order_found = false;
                bool inside_changed = false;
                {
                  MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
                  MDOrder* order = m_order_book->unlocked_find_order(order_id);
                  if(order) {
                    if (order->product_id != pid)
                      send_alert("exec product id mismatch %lu %lu %c (%lu), %d != %d", msg_TrdRegTSTimePriority, msg_SecurityID, side, order_id, order->product_id, pid);

                    if(m_qbdf_builder) {
                      gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
                                                 m_micros_recv_time, m_micros_exch_time_delta,
                                                 m_qbdf_exch_char, order->flip_side(), ' ', shares, order->price);
                    }

                    order_found = true;
                    m_trade.m_id = order->product_id;
                    m_trade.m_price = order->price_as_double();
                    m_trade.set_side(order->flip_side());
                    memcpy(m_trade.m_trade_qualifier, "    ", 4);

                    // order->size is uint32_t, up-cast to signed 64-bit number to compare to the quantity in the message, which can be
                    // negative (e.g., for spreads)
                    if (static_cast<int64_t>(order->size) < shares)
                      send_alert("exec size is bigger than the actual number of shares!! %d %f %u %u", m_trade.m_id, m_trade.m_price, shares, order->size);
                    uint32_t new_size = order->size - shares;
                    inside_changed = order->is_top_level();
                    MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
                    if(new_size > 0) {
                      m_order_book->unlocked_modify_order(t, order, new_size, shares);
                    } else {
                      m_order_book->unlocked_remove_order(t, order, shares);
                    }
                  } else {
                    send_alert("couldn't find order %lld to execute (made from %lu %lu %c)", order_id, msg_TrdRegTSTimePriority, msg_SecurityID, side);
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
          }
          break;
        case Eurex::EnumMessageType::InstrumentStateChange:
          {
            const Eurex::InstrumentStateChange& msg = reinterpret_cast<const Eurex::InstrumentStateChange&>(*(buf + sofar));
            ProductID pid = product_id_of_symbol(msg.SecurityID);
            if (pid != Product::invalid_product_id) {
              set_times(msg.TransactTime);
              MarketStatus ms = m_security_status[pid];
              QBDFStatusType qbdf_stat = QBDFStatusType::Invalid;
              if (msg.SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::Closed) {
                m_security_status[pid] = MarketStatus::Closed;
                qbdf_stat = QBDFStatusType::Closed;
              } else if (msg.SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::Continuous) {
                m_security_status[pid] = MarketStatus::Open;
                qbdf_stat = QBDFStatusType::Open;
              } else if (msg.SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::OpeningAuction) {
                m_security_status[pid] = MarketStatus::Auction;
                qbdf_stat = QBDFStatusType::Auction;
              } else if (msg.SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::IntradayAuction) {
                m_security_status[pid] = MarketStatus::Auction;
                qbdf_stat = QBDFStatusType::Auction;
              } else if (msg.SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::CircuitBreakerAuction) {
                m_security_status[pid] = MarketStatus::Auction;
                qbdf_stat = QBDFStatusType::Auction;
              } else if (msg.SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::ClosingAuction) {
                m_security_status[pid] = MarketStatus::Auction;
                qbdf_stat = QBDFStatusType::Auction;
              }

              /* // others - don't exactly map to one of our market states
              Eurex::EnumSecurityTradingStatus::Restricted = 201 ,
              Eurex::EnumSecurityTradingStatus::Book = 202 ,
              Eurex::EnumSecurityTradingStatus::OpeningAuctionFreeze = 205 ,
              Eurex::EnumSecurityTradingStatus::IntradayAuctionFreeze = 207 ,
              Eurex::EnumSecurityTradingStatus::CircuitBreakerAuctionFreeze = 209 ,
              Eurex::EnumSecurityTradingStatus::ClosingAuctionFreeze = 211
              */

              if (m_qbdf_builder) {
                gen_qbdf_status_micros(m_qbdf_builder, pid,
                              m_micros_recv_time, m_micros_exch_time_delta,
                              m_qbdf_exch_char, qbdf_stat.index(), boost::lexical_cast<string>(msg.SecurityTradingStatus));
              }
              if (ms != m_security_status[pid]) {
                m_msu.m_id = pid;
                m_msu.m_exch = m_exch;
                m_msu.m_market_status = m_security_status[pid];
                m_msu.m_ssr = SSRestriction::None;
                m_msu.m_rpi = RetailPriceImprovement::None;
                if(m_subscribers) {
                  m_subscribers->update_subscribers(m_msu);
                }
              }
            }
          }
          break;
        // case Eurex::EnumMessageType::ProductStateChange:
        //   {
        //     const Eurex::ProductStateChange& msg = reinterpret_cast<const Eurex::ProductStateChange&>(*(buf + sofar));
        //   }
        //   break;
        case Eurex::EnumMessageType::OrderMassDelete:
          {
            if(m_qbdf_builder) {
              const Eurex::MessageHeader& head = reinterpret_cast<const Eurex::MessageHeader&>(*(buf + sofar));
              ProductID pid = Product::invalid_product_id;
              if (head.BodyLen == 24) { // 2.1+
                const Eurex::OrderMassDelete& msg = reinterpret_cast<const Eurex::OrderMassDelete&>(*(buf + sofar));
                set_times(msg.TransactTime);
                pid = product_id_of_symbol(msg.SecurityID);
              } else {
                const Eurex::OrderMassDelete10& msg = reinterpret_cast<const Eurex::OrderMassDelete10&>(*(buf + sofar));
                pid = product_id_of_symbol(msg.SecurityID);
              }

              if (pid != Product::invalid_product_id) {
                if (m_order_book) {
                  m_order_book->clear(pid, true);
                }
                gen_qbdf_gap_marker(m_qbdf_builder, (m_micros_recv_time * Time_Constants::ticks_per_micro) / Time_Constants::ticks_per_mili, pid, context);
              }
            } else {
              const Eurex::OrderMassDelete& msg = reinterpret_cast<const Eurex::OrderMassDelete&>(*(buf + sofar));
              ProductID pid = product_id_of_symbol(msg.SecurityID);
              if (pid != Product::invalid_product_id) {
                set_times(msg.TransactTime);
                if (m_order_book) {
                  m_order_book->clear(pid, true);

                  if(m_subscribers) {
                    m_subscribers->post_invalid_quote(pid, m_exch);
                  }
                }
              }
            }

          }
          break;
        // case Eurex::EnumMessageType::Heartbeat:
        //   {
        //     const Eurex::Heartbeat& msg = reinterpret_cast<const Eurex::Heartbeat&>(*(buf + sofar));
        //   }
        //   break;
        case Eurex::EnumMessageType::ExecutionSummary:
          {
            if (m_qbdf_builder) {
              const Eurex::MessageHeader& head = reinterpret_cast<const Eurex::MessageHeader&>(*(buf + sofar));

              ProductID pid = Product::invalid_product_id;
              uint64_t exch_time_nanos = 0;
              uint64_t security_id = 0;
              int32_t qty = 0;
              MDPrice price;
              char side = '0';

              if (head.BodyLen == 64) { // more recent (2.1+)
                const Eurex::ExecutionSummary& msg = reinterpret_cast<const Eurex::ExecutionSummary&>(*(buf + sofar));
                security_id = msg.SecurityID;
                exch_time_nanos = msg.ExecID;
                qty = msg.LastQty;
                price = MDPrice::from_iprice64(msg.LastPx);
                side = msg.AggressorSide == Eurex::EnumAggressorSide::Buy ? 'B' : 'S';
              } else {
                const Eurex::ExecutionSummary10& msg = reinterpret_cast<const Eurex::ExecutionSummary10&>(*(buf + sofar));
                security_id = msg.SecurityID;
                exch_time_nanos = msg.ExecID;
                qty = msg.LastQty;
                price = MDPrice::from_iprice64(msg.LastPx);
                side = msg.AggressorSide == Eurex::EnumAggressorSide::Buy ? 'B' : 'S';
              }

              pid = product_id_of_symbol(security_id);
              if (pid != Product::invalid_product_id) {
                set_times(exch_time_nanos);
                string cond = "    ";
                gen_qbdf_trade_micros(m_qbdf_builder, pid,
                           m_micros_exch_time, m_micros_exch_time_delta,
                           m_qbdf_exch_char, side, ' ', cond,
                           qty, price);
              }
            }
          }
          break;
        case Eurex::EnumMessageType::InstrumentSummary:
          {
            const Eurex::InstrumentSummary& msg = reinterpret_cast<const Eurex::InstrumentSummary&>(*(buf + sofar));
            set_times(msg.LastUpdateTime);
            ProductID pid = product_id_of_symbol(msg.SecurityID);
            if (pid != Product::invalid_product_id) {
              // m_MarketSegmentID_ProductID[m_market_segment_id].insert(pid);
              if (msg.SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::IntradayAuction ||
                  msg.SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::CircuitBreakerAuction) {
                if (m_qbdf_builder) {
                  gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, pid, m_qbdf_exch_char, context);
                }

                if (m_order_book) {
                  m_order_book->clear(pid, true);

                  if(m_subscribers) {
                    m_subscribers->post_invalid_quote(pid, m_exch);
                  }
                }
              }
            }
          }
          break;
        case Eurex::EnumMessageType::AuctionBBO:
          {
            const Eurex::AuctionBBO& msg = reinterpret_cast<const Eurex::AuctionBBO&>(*(buf + sofar));
            ProductID pid = product_id_of_symbol(msg.SecurityID);
            if (pid != Product::invalid_product_id) {
              MDPrice bid_price = MDPrice::from_iprice64(msg.BidPx);
              MDPrice ask_price = MDPrice::from_iprice64(msg.OfferPx);

              Time t(m_micros_exch_time * Time_Constants::ticks_per_micro); // maybe make this Time::current_time() or something
              if(m_qbdf_builder)
              {
                set_times(msg.TransactTime);
                int32_t exch_time_delta = 0;
                char indicativePriceFlag = ' ';
                bool suppress64 = false;
                MDPrice emptyPrice;
                gen_qbdf_imbalance_micros(m_qbdf_builder, pid, m_micros_recv_time, exch_time_delta, m_qbdf_exch_char,
                  emptyPrice, // ref_price
                  bid_price,// near_price
                  ask_price, // far_price
                  0, 0, 0, indicativePriceFlag, suppress64);
              }

              Time exch_time(msg.TransactTime);
              AuctionUpdate auction(exch_time, pid, m_exch);
              auction.m_near_price = 0;
              if(m_subscribers) {
                m_subscribers->update_subscribers(auction);
                m_quote.m_bid = bid_price.fprice();
                m_quote.m_ask = ask_price.fprice();
                m_quote.m_bid_size = 0;
                m_quote.m_ask_size = 0;
                m_quote.m_quote_status = QuoteStatus::Normal;
                m_quote.m_id = pid;
                m_quote.set_time(exch_time);
                send_quote_update();
              }
              if(m_provider) {
                m_provider->check_auction(auction);
              }
            }
          }
          break;
        case Eurex::EnumMessageType::AuctionClearingPrice:
          {
            const Eurex::AuctionClearingPrice& msg = reinterpret_cast<const Eurex::AuctionClearingPrice&>(*(buf + sofar));
            ProductID pid = product_id_of_symbol(msg.SecurityID);
            if (pid != Product::invalid_product_id) {
              AuctionUpdate auction(Time(msg.TransactTime), pid, m_exch);
              MDPrice px = MDPrice::from_iprice64(msg.LastPx);
              auction.m_ref_price = px.fprice();
              set_times(msg.TransactTime);
              if(m_qbdf_builder)
              {
                int32_t exch_time_delta = 0;
                char indicativePriceFlag = ' ';
                bool suppress64 = false;
                MDPrice emptyPrice;
                gen_qbdf_imbalance_micros(m_qbdf_builder, pid, m_micros_recv_time, exch_time_delta, m_qbdf_exch_char,
                  px, // ref_price
                  emptyPrice,// near_price
                  emptyPrice, // far_price
                  0, 0, 0, indicativePriceFlag, suppress64);
              }
              if(m_subscribers) {
                m_subscribers->update_subscribers(auction);
              }
            }
          }
          break;
        case Eurex::EnumMessageType::TopOfBook:
          {
            const Eurex::TopOfBook& msg = reinterpret_cast<const Eurex::TopOfBook&>(*(buf + sofar));
            ProductID pid = product_id_of_symbol(msg.SecurityID);
            if (pid != Product::invalid_product_id) {
              MDPrice bid_price = MDPrice::from_iprice64(msg.BidPx);
              MDPrice ask_price = MDPrice::from_iprice64(msg.OfferPx);
              Time exch_time(msg.TransactTime);
              if (m_qbdf_builder) {
                set_times(msg.TransactTime);
                gen_qbdf_quote_micros(m_qbdf_builder, pid,
                             m_micros_recv_time, m_micros_exch_time_delta,
                             m_qbdf_exch_char, ' ', bid_price, ask_price,
                             0, 0);
              }
              if(m_subscribers) {
                m_quote.set_time(exch_time);
                m_quote.m_bid = bid_price.fprice();
                m_quote.m_ask = ask_price.fprice();
                m_quote.m_bid_size = 0;
                m_quote.m_ask_size = 0;
                m_quote.m_quote_status = QuoteStatus::Normal;
                m_quote.m_id = pid;
                send_quote_update();
              }
            }
          }
          break;
        case Eurex::EnumMessageType::TradeReport:
          {
            const Eurex::TradeReport& msg = reinterpret_cast<const Eurex::TradeReport&>(*(buf + sofar));
            ProductID pid = product_id_of_symbol(msg.SecurityID);
            if (pid != Product::invalid_product_id) {
              set_times(msg.TransactTime);
              MDPrice price = MDPrice::from_iprice64(msg.LastPx);
              int32_t qty = msg.LastQty;
              char side = '0';
              char cond[] = {' ', ' ', ' ', ' '};
              Time exch_time(msg.TransactTime);

              if (msg.MatchType == Eurex::EnumMatchType::CallAuction) {
                cond[0] = boost::lexical_cast<char>(msg.MatchType);
                cond[1] = boost::lexical_cast<char>(msg.MatchSubType);
              } else if (msg.MatchType == Eurex::EnumMatchType::ConfirmedTradeReport || msg.MatchType == Eurex::EnumMatchType::CrossAuction) {
                cond[0] = boost::lexical_cast<char>(msg.MatchType);
              }

              m_trade.set_time(exch_time);
              m_trade.m_id = pid;
              m_trade.m_price = price.fprice();
              m_trade.m_size = qty;
              m_trade.m_side = 0;

              send_trade_update();

              if (m_qbdf_builder) {
                gen_qbdf_trade_micros(m_qbdf_builder, pid,
                           m_micros_recv_time, m_micros_exch_time_delta,
                           m_qbdf_exch_char, side, ' ', string(cond, 4),
                           qty, price);
              }
            }
          }
          break;
        default:
          break;
      }
        sofar += head.BodyLen;
      }

      return packet_len;
    }

  void Eurex_Handler::subscribe_product(hf_core::ProductID product_id, hf_core::ExchangeID exchange_id, const string& mdSymbol, const string& mdExch)
  {
    if(exchange_id == ExchangeID::XEUR) {
      int64_t eurex_id = boost::lexical_cast<int64_t>(mdSymbol);
      m_eurex_id_to_product_id[eurex_id] = product_id;
    }
  }

  ProductID
  Eurex_Handler::product_id_of_symbol(int64_t instrument_id)
  {
    if (m_qbdf_builder) {
      stringstream ss;
      ss << instrument_id;

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(ss.str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
                             __PRETTY_FUNCTION__, ss.str().c_str());
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      return symbol_id;
    } else {
      hash_map<int64_t, ProductID>::iterator it = m_eurex_id_to_product_id.find(instrument_id);
      if(it == m_eurex_id_to_product_id.end()) {
        return Product::invalid_product_id;
      }
      return it->second;
    }
  }

  void
  Eurex_Handler::send_quote_update()
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
  Eurex_Handler::send_trade_update()
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
  Eurex_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
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
    m_exch = exch;
    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_msu.m_exch = exch;
    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, m_midnight.strftime("%Y%m%d"));
    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);

    for (int i = 0; i != m_app->pm()->max_product_id(); ++i) {
      const Product p = m_app->pm()->find_by_id(i);
      p.get("");
    }


    for (size_t i = m_channel_info_map.size(); i < 32; ++i)
      m_channel_info_map.push_back(channel_info("", i));
  }

  uint64_t
  Eurex_Handler::get_unique_order_id(uint64_t eurex_time_priority, int64_t security_id, char side)
  {
    Eurex::OrderIDKey oihk(eurex_time_priority, security_id, side);
    Eurex::order_id_hash_table::iterator i = m_order_id_hash.find(oihk);
    if (i == m_order_id_hash.end()) {
      uint64_t order_id = ++m_order_id;
      m_order_id_hash[oihk] = order_id;
      return order_id;
    } else {
      return i->second;
    }
  }

  void
  Eurex_Handler::modify_order(size_t context, uint64_t exch_time_nanos, uint64_t security_id, Eurex::EnumSide::Side s, uint64_t existing_time_prio, uint64_t new_time_prio, int64_t px, int32_t shares, channel_info& ci)
  {
    set_times(exch_time_nanos);
    char side = char_side(s);
    uint64_t order_id = get_unique_order_id(existing_time_prio, security_id, side);
    uint64_t new_order_id = 0;

    MDPrice price = MDPrice::from_iprice64(px);

    Time t(exch_time_nanos);
    ci.timestamp = t;

    // mostly copied from LSE handler

    if(m_order_book) {
      ProductID pid = Product::invalid_product_id;
      bool inside_changed = false;
      MDOrder* order = NULL;
      {
        MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
        order = m_order_book->unlocked_find_order(order_id);
      }
      if(order) {
        pid = order->product_id;

        if (new_time_prio != 0) { // replace logic
          new_order_id = get_unique_order_id(new_time_prio, security_id, side);
          {
            MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
            inside_changed = m_order_book->unlocked_replace_order_ip(t, order, order->side(), price, shares, new_order_id, false);
          }
        } else { // modify logic
          MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(order->product_id).m_mutex, true);
          inside_changed = m_order_book->unlocked_modify_order_ip(t, order, order->side(), price, shares);
        }

        if(m_qbdf_builder) {
          if (new_time_prio != 0) { // replace logic
            gen_qbdf_order_replace_micros(m_qbdf_builder, order->product_id, order_id,
                                          m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                                          order->side(), new_order_id, shares, price);
          } else { // modify logic
            gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
                                         m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                                         order->side(), shares, price);
          }
        }
      } else {
        send_alert("couldn't find order %lld to modify (made from %lu %lu %c)", order_id, existing_time_prio, security_id, side);
      }

      if(inside_changed) {
        m_quote.set_time(t);
        m_quote.m_id = pid;
        send_quote_update();
      }
    }
  }

  static std::string get_security_trading_status(Eurex::EnumSecurityTradingStatus::SecurityTradingStatus SecurityTradingStatus) {
    std::string security_trading_status = "";
    if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::Closed) {
      security_trading_status = "Closed";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::Restricted) {
      security_trading_status = "Restricted";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::Book) {
      security_trading_status = "Book";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::Continuous) {
      security_trading_status = "Continuous";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::OpeningAuction) {
      security_trading_status = "OpeningAuction";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::OpeningAuctionFreeze) {
      security_trading_status = "OpeningAuctionFreeze";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::IntradayAuction) {
      security_trading_status = "IntradayAuction";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::IntradayAuctionFreeze) {
      security_trading_status = "IntradayAuctionFreeze";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::CircuitBreakerAuction) {
      security_trading_status = "CircuitBreakerAuction";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::CircuitBreakerAuctionFreeze) {
      security_trading_status = "CircuitBreakerAuctionFreeze";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::ClosingAuction) {
      security_trading_status = "ClosingAuction";
    } else if (SecurityTradingStatus == Eurex::EnumSecurityTradingStatus::ClosingAuctionFreeze) {
      security_trading_status = "ClosingAuctionFreeze";
    } else {
      security_trading_status = "";
    }
    return security_trading_status;
  }



  static size_t
  dump_parse2(ILogger& log, Dump_Filters& filter, size_t context, int32_t MarketSegmentID, const char* buf, size_t len) {
    const Eurex::MessageHeader& head = reinterpret_cast<const Eurex::MessageHeader&>(*buf);
    log.printf("segment %6d len %3hu msg_type %5u seq_num %10d", MarketSegmentID, head.BodyLen, head.TemplateID, head.MsgSeqNum);
    size_t processed = head.BodyLen;

    switch (head.TemplateID) {
      case Eurex::EnumMessageType::AuctionBBO:
        {
          const Eurex::AuctionBBO& msg = reinterpret_cast<const Eurex::AuctionBBO&>(*buf);
          log.printf(" AuctionBBO sec_id %7ld trans_time %lu bid %ld offer %ld", msg.SecurityID, msg.TransactTime, msg.BidPx, msg.OfferPx);
        }
        break;
      case Eurex::EnumMessageType::AuctionClearingPrice:
        {
          const Eurex::AuctionClearingPrice& msg = reinterpret_cast<const Eurex::AuctionClearingPrice&>(*buf);
          log.printf(" AuctionClearingPrice sec_id %7ld trans_time %lu last px %ld", msg.SecurityID, msg.TransactTime, msg.LastPx);
        }
        break;
      case Eurex::EnumMessageType::CrossRequest:
        {
          const Eurex::CrossRequest& msg = reinterpret_cast<const Eurex::CrossRequest&>(*buf);
          log.printf(" CrossRequest sec_id %7ld trans_time %lu last qty %d", msg.SecurityID, msg.TransactTime, msg.LastQty);
        }
        break;
      case Eurex::EnumMessageType::ExecutionSummary:
        {
          const Eurex::ExecutionSummary& msg = reinterpret_cast<const Eurex::ExecutionSummary&>(*buf);
          char aggr_side = 'X';
          if (msg.AggressorSide == Eurex::EnumAggressorSide::Buy)
            aggr_side = 'B';
          else if (msg.AggressorSide == Eurex::EnumAggressorSide::Sell)
            aggr_side = 'S';
          log.printf(" ExecutionSummary sec_id %7ld aggressor_time %lu req_time %lu exec_id %lu last qty %d last px %ld aggr_side %d trade_cond %d",
                     msg.SecurityID, msg.AggressorTimestamp, msg.RequestTime, msg.ExecID, msg.LastQty, msg.LastPx, aggr_side,
                     msg.TradeCondition);
        }
        break;
      case Eurex::EnumMessageType::FullOrderExecution:
        {
          const Eurex::FullOrderExecution& msg = reinterpret_cast<const Eurex::FullOrderExecution&>(*buf);
          char side = 'X';
          if (msg.Side == Eurex::EnumSide::Buy)
            side = 'B';
          else if (msg.Side == Eurex::EnumSide::Sell)
            side = 'S';

          log.printf(" FullOrderExecution sec_id %7ld side %c last qty %d last px %ld px %ld time_priority %lu match_id %u",
                     msg.SecurityID, side, msg.LastQty, msg.LastPx, msg.Price, msg.TrdRegTSTimePriority, msg.TrdMatchID);
        }
        break;
      case Eurex::EnumMessageType::Heartbeat:
        {
          const Eurex::Heartbeat& msg = reinterpret_cast<const Eurex::Heartbeat&>(*buf);
          log.printf(" Heartbeat last_msg_seq_num_processed %u", msg.LastMsgSeqNumProcessed);
        }
        break;
      case Eurex::EnumMessageType::OrderAdd:
        {
          const Eurex::OrderAdd& msg = reinterpret_cast<const Eurex::OrderAdd&>(*buf);
          char side = 'X';
          if (msg.Side == Eurex::EnumSide::Buy)
            side = 'B';
          else if (msg.Side == Eurex::EnumSide::Sell)
            side = 'S';

          log.printf(" OrderAdd sec_id %7ld time_in %lu time_prio %lu side %c price %ld disp_qty %d",
                     msg.SecurityID, msg.TrdRegTSTimeIn, msg.TrdRegTSTimePriority, side, msg.Price, msg.DisplayQty);
        }
        break;
      case Eurex::EnumMessageType::OrderDelete:
        {
          const Eurex::MessageHeader head = reinterpret_cast<const Eurex::MessageHeader&>(*buf);
          if (head.BodyLen == 56) { // 2.1+
            const Eurex::OrderDelete& msg = reinterpret_cast<const Eurex::OrderDelete&>(*buf);
            char side = 'X';
            if (msg.Side == Eurex::EnumSide::Buy)
              side = 'B';
            else if (msg.Side == Eurex::EnumSide::Sell)
              side = 'S';

            log.printf(" OrderDelete sec_id %7ld time_in %lu trans_time %lu time_prio %lu side %c price %ld disp_qty %d",
                       msg.SecurityID, msg.TrdRegTSTimeIn, msg.TransactTime, msg.TrdRegTSTimePriority, side, msg.Price, msg.DisplayQty);
          } else {
            const Eurex::OrderDelete10& msg = reinterpret_cast<const Eurex::OrderDelete10&>(*buf);
            char side = 'X';
            if (msg.Side == Eurex::EnumSide::Buy)
              side = 'B';
            else if (msg.Side == Eurex::EnumSide::Sell)
              side = 'S';

            log.printf(" OrderDelete10 sec_id %7ld time_in %lu time_prio %lu side %c price %ld disp_qty %d",
                       msg.SecurityID, msg.TrdRegTSTimeIn, msg.TrdRegTSTimePriority, side, msg.Price, msg.DisplayQty);
          }
        }
        break;
      case Eurex::EnumMessageType::OrderMassDelete:
        {
          const Eurex::MessageHeader head = reinterpret_cast<const Eurex::MessageHeader&>(*buf);
          if (head.BodyLen == 24) { // 2.1+
            const Eurex::OrderMassDelete& msg = reinterpret_cast<const Eurex::OrderMassDelete&>(*buf);
            log.printf(" OrderMassDelete sec_id %7ld trans_time %lu", msg.SecurityID, msg.TransactTime);
          } else {
            const Eurex::OrderMassDelete10& msg = reinterpret_cast<const Eurex::OrderMassDelete10&>(*buf);
            log.printf(" OrderMassDelete10 sec_id %7ld", msg.SecurityID);
          }
        }
        break;
      case Eurex::EnumMessageType::OrderModify:
        {
          const Eurex::OrderModify& msg = reinterpret_cast<const Eurex::OrderModify&>(*buf);
          char side = 'X';
          if (msg.Side == Eurex::EnumSide::Buy)
            side = 'B';
          else if (msg.Side == Eurex::EnumSide::Sell)
            side = 'S';
          log.printf(" OrderModify sec_id %7ld time_in %lu prev_time_prio %lu prev_price %ld prev_qty %d time_prio %lu side %c price %ld disp_qty %d",
                     msg.SecurityID, msg.TrdRegTSTimeIn, msg.TrdRegTSPrevTimePriority, msg.PrevPrice, msg.PrevDisplayQty, msg.TrdRegTSTimePriority, side, msg.Price, msg.DisplayQty);
        }
        break;
      case Eurex::EnumMessageType::OrderModifySamePrio:
        {
          const Eurex::OrderModifySamePrio& msg = reinterpret_cast<const Eurex::OrderModifySamePrio&>(*buf);
          char side = 'X';
          if (msg.Side == Eurex::EnumSide::Buy)
            side = 'B';
          else if (msg.Side == Eurex::EnumSide::Sell)
            side = 'S';

          log.printf(" OrderModifySamePrio sec_id %7ld time_in %lu trans_time %lu prev_qty %d time_prio %lu side %c price %ld disp_qty %d",
                     msg.SecurityID, msg.TrdRegTSTimeIn, msg.TransactTime, msg.PrevDisplayQty, msg.TrdRegTSTimePriority, side, msg.Price, msg.DisplayQty);
        }
        break;
      case Eurex::EnumMessageType::PartialOrderExecution:
        {
          const Eurex::PartialOrderExecution& msg = reinterpret_cast<const Eurex::PartialOrderExecution&>(*buf);
          char side = 'X';
          if (msg.Side == Eurex::EnumSide::Buy)
            side = 'B';
          else if (msg.Side == Eurex::EnumSide::Sell)
            side = 'S';
          log.printf(" PartialOrderExecution sec_id %7ld time_prio %lu match_id %u last_qty %d last_px %ld price %ld side %c",
                     msg.SecurityID, msg.TrdRegTSTimePriority, msg.TrdMatchID, msg.LastQty, msg.LastPx, msg.Price, side);
        }
        break;
      case Eurex::EnumMessageType::TopOfBook:
        {
          const Eurex::TopOfBook& msg = reinterpret_cast<const Eurex::TopOfBook&>(*buf);
          log.printf(" TopOfBook sec_id %7ld trans_time %lu bid %ld offer %ld",
                     msg.SecurityID, msg.TransactTime, msg.BidPx, msg.OfferPx);
        }
        break;

      case Eurex::EnumMessageType::InstrumentStateChange:
        {
          const Eurex::InstrumentStateChange& msg = reinterpret_cast<const Eurex::InstrumentStateChange&>(*buf);
          string security_status = "";
          if (msg.SecurityStatus == Eurex::EnumSecurityStatus::Active) {
            security_status = "Active";
          } else if (msg.SecurityStatus == Eurex::EnumSecurityStatus::Inactive) {
            security_status = "InActive";
          } else if (msg.SecurityStatus == Eurex::EnumSecurityStatus::Expired) {
            security_status = "Expired";
          } else if (msg.SecurityStatus == Eurex::EnumSecurityStatus::Suspended) {
            security_status = "Suspended";
          }

          string security_trading_status = get_security_trading_status(msg.SecurityTradingStatus);

          log.printf(" InstrumentStateChange sec_id %7ld trans_time %lu sec_status %s trading_status %s fast_market %c",
                     msg.SecurityID, msg.TransactTime, security_status.c_str(), security_trading_status.c_str(), msg.FastMarketIndicator ? 'T' : 'F');
        }
        break;
      case Eurex::EnumMessageType::ProductStateChange:
        {
          const Eurex::ProductStateChange& msg = reinterpret_cast<const Eurex::ProductStateChange&>(*buf);
          string session_id = "";
          if (msg.TradingSessionID == Eurex::EnumTradingSessionID::Day) {
            session_id = "Day";
          } else if (msg.TradingSessionID == Eurex::EnumTradingSessionID::Morning) {
            session_id = "Morning";
          } else if (msg.TradingSessionID == Eurex::EnumTradingSessionID::Evening) {
            session_id = "Evening";
          } else if (msg.TradingSessionID == Eurex::EnumTradingSessionID::Holiday) {
            session_id = "Holiday";
          }

          string session_sub_id = "";
          if (msg.TradingSessionSubID == Eurex::EnumTradingSessionSubID::PreTrading) {
            session_sub_id = "PreTrading";
          } else if (msg.TradingSessionSubID == Eurex::EnumTradingSessionSubID::Trading) {
            session_sub_id = "Trading";
          } else if (msg.TradingSessionSubID == Eurex::EnumTradingSessionSubID::Closing) {
            session_sub_id = "Closing";
          } else if (msg.TradingSessionSubID == Eurex::EnumTradingSessionSubID::PostTrading) {
            session_sub_id = "PostTrading";
          } else if (msg.TradingSessionSubID == Eurex::EnumTradingSessionSubID::Quiescent) {
            session_sub_id = "Quiescent";
          }

          string session_status = "";
          if (msg.TradSesStatus == Eurex::EnumTradSesStatus::Halted) {
            session_status = "Halted";
          } else if (msg.TradSesStatus == Eurex::EnumTradSesStatus::Open) {
            session_status = "Open";
          } else if (msg.TradSesStatus == Eurex::EnumTradSesStatus::Closed) {
            session_status = "Closed";
          }


          log.printf(" ProductStateChange trans_time %lu session_id %s session_sub_id %s session_status %s fast_market %c",
                     msg.TransactTime, session_id.c_str(), session_sub_id.c_str(), session_status.c_str(), msg.FastMarketIndicator ? 'T' : 'F');

        }
        break;
      case Eurex::EnumMessageType::InstrumentSummary:
        {
          const Eurex::InstrumentSummary& msg = reinterpret_cast<const Eurex::InstrumentSummary&>(*buf);
          string security_trading_status = get_security_trading_status(msg.SecurityTradingStatus);
          log.printf(" InstrumentSummary sec_id %7ld last_update_time %lu reg_exec_time %lu tot # orders %hu sec_status %u trading_status %s fast_market %u md_entries %u entry_px %lu entry_size %d entry_type %u",
                     msg.SecurityID, msg.LastUpdateTime, msg.TrdRegTSExecutionTime, msg.TotNoOrders, msg.SecurityStatus, security_trading_status.c_str(),
                     msg.FastMarketIndicator, msg.NoMDEntries, msg.MDEntryPx, msg.MDEntrySize, msg.MDEntryType);
        }
        break;
      case Eurex::EnumMessageType::ProductSummary:
        {
          const Eurex::ProductSummary& msg = reinterpret_cast<const Eurex::ProductSummary&>(*buf);
          log.printf(" ProductSummary last_seq %u sess_id %u sub_sess_id %u sess_status %u fast_market %u",
                     msg.LastMsgSeqNumProcessed, msg.TradingSessionID, msg.TradingSessionSubID, msg.TradSesStatus, msg.FastMarketIndicator);
        }
        break;



        /*
// TODO
struct AddComplexInstrument {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t TransactTime;
   int32_t SecuritySubType;
  EnumProductComplex::ProductComplex ProductComplex:8;
  EnumImpliedMarketIndicator::ImpliedMarketIndicator ImpliedMarketIndicator:8;
   uint8_t NoLegs;
      char Pad1;
   int32_t LegSymbol;
      char Pad4[4];
   int64_t LegSecurityID;
   int32_t LegRatioQty;
  EnumLegSide::LegSide LegSide:8;
      char Pad3[3];
} __attribute__((packed));


struct PacketHeader {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint32_t ApplSeqNum;
   int32_t MarketSegmentID;
   uint8_t PartitionID;
  EnumCompletionIndicator::CompletionIndicator CompletionIndicator:8;
  EnumApplSeqResetIndicator::ApplSeqResetIndicator ApplSeqResetIndicator:8;
      char Pad5[5];
  uint64_t TransactTime;
} __attribute__((packed));

struct QuoteRequest {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
   int32_t LastQty;
  EnumSide::Side Side:8;
      char Pad3[3];
  uint64_t TransactTime;
} __attribute__((packed));

struct SnapshotOrder {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
  uint64_t TrdRegTSTimePriority;
   int32_t DisplayQty;
  EnumSide::Side Side:8;
      char Pad3[3];
   int64_t Price;
} __attribute__((packed));

struct TradeReversal {
  uint16_t BodyLen;
  uint16_t TemplateID;
  uint32_t MsgSeqNum;
   int64_t SecurityID;
  uint64_t TransactTime;
  uint32_t TrdMatchID;
   int32_t LastQty;
   int64_t LastPx;
  uint64_t TrdRegTSExecutionTime;
   uint8_t NoMDEntries;
      char Pad7[7];
   int64_t MDEntryPx;
   int32_t MDEntrySize;
  EnumMDEntryType::MDEntryType MDEntryType:8;
      char Pad3[3];
} __attribute__((packed));
        */

      case Eurex::EnumMessageType::TradeReport:
        {
          const Eurex::TradeReport& msg = reinterpret_cast<const Eurex::TradeReport&>(*buf);
          string match_type = "";
          string match_sub_type = "";
          if (msg.MatchType == Eurex::EnumMatchType::CallAuction) {
              match_type = "CallAuction";

            if (msg.MatchSubType == Eurex::EnumMatchSubType::OpeningAuction) {
              match_sub_type = "OpeningAuction";
            } else if (msg.MatchSubType == Eurex::EnumMatchSubType::ClosingAuction) {
              match_sub_type = "ClosingAuction";
            } else if (msg.MatchSubType == Eurex::EnumMatchSubType::IntradayAuction) {
              match_sub_type = "IntradayAuction";
            } else if (msg.MatchSubType == Eurex::EnumMatchSubType::CircuitBreakerAuction) {
              match_sub_type = "CircuitBreakerAuction";
            }
          } else {
            match_sub_type = "NA";
            if (msg.MatchType == Eurex::EnumMatchType::CrossAuction) {
              match_type = "CrossAuction";
            } else if (msg.MatchType == Eurex::EnumMatchType::ConfirmedTradeReport) {
              match_type = "ConfirmedTradeReport";
            }
          }
          log.printf(" TradeReport sec_id %7ld trans_time %lu match_type %s match_sub_type %s trd_match_id %u qty %d px %ld",
                     msg.SecurityID, msg.TransactTime, match_type.c_str(), match_sub_type.c_str(), msg.TrdMatchID, msg.LastQty, msg.LastPx);
        }
        break;
      default:
        {
          log.printf(" unhandled message %d", head.TemplateID);
        }
        break;
    }

    log.printf("\n");
    return processed;
  }

  size_t
  Eurex_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
  {
    const Eurex::PacketHeader& head = reinterpret_cast<const Eurex::PacketHeader&>(*buf);

    size_t sofar = 0;
    sofar += sizeof(Eurex::PacketHeader);

    log.printf(" trans_time %lu appl_seq %10u segment %6d partition %2u complete %d appl_seq_reset %d len %lu\n",
               head.TransactTime, head.ApplSeqNum, head.MarketSegmentID, head.PartitionID, head.CompletionIndicator, head.ApplSeqResetIndicator, len);

    while (sofar < len) {
      const char *sub_buf = buf + sofar;
      sofar += dump_parse2(log, filter, context, head.MarketSegmentID, sub_buf, len - sofar);
    }
    return sofar;
  }

  void
  Eurex_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    if(m_order_book) {
      m_order_book->clear(false);
      if(m_qbdf_builder && m_micros_recv_time > 0) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_qbdf_exch_char, -1);
      }
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }
}
