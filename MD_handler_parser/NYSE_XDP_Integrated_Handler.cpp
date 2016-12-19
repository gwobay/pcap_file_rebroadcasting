#include <Data/QBDF_Util.h>
#include <Data/NYSE_XDP_Integrated_Handler.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>

#include <MD/MDProvider.h>

#include <Logger/Logger_Manager.h>

#include <Util/CSV_Parser.h>

#include <stdarg.h>

#define CLASS_NAME "NYSE_XDP_Integrated_Handler"
#define LIKELY(x) __builtin_expect(!!(x),1)
#define UNLIKELY(x) __builtin_expect(!!(x),0)
#define LOG(log_level_, format_string_, ...) QBT_LOG_WITH_FULL_PREFIX(m_logger, log_level_, m_name.c_str(), CLASS_NAME, __func__, format_string_, ##__VA_ARGS__)

namespace hf_core {
  
  static FactoryRegistrar1<std::string, Handler, Application*, NYSE_XDP_Integrated_Handler> r1("NYSE_XDP_Integrated_Handler");
  
  using namespace NYSE_XDP_Integrated_structs;


#define SET_SOURCE_TIME(obj)                                            \
  bool stale_msg __attribute__((unused))= false;                        \
  {                                                                     \
    Time source_time = Time((Time_Constants::ticks_per_second * (uint64_t)msg.source_time) + (uint64_t)msg.source_time_ns); \
    if(source_time.is_set()) {                                          \
      m_micros_exch_time = (source_time.total_usec() - m_micros_midnight); \
      m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;  \
      obj.set_time(source_time);					\
      Time_Duration diff = send_time - source_time;                     \
      if(diff.abs() > m_max_send_source_difference) {                   \
        Product& prod = m_app->pm()->find_by_id(id);                    \
        send_alert("%s --  %s::%s send_time %s is %s lagged from source_time %s for symbol %s", \
                   m_name.c_str(), CLASS_NAME, __func__, send_time.to_string().c_str(), \
                   diff.to_string().c_str(), source_time.to_string().c_str(), prod.symbol().c_str()); \
        stale_msg = true;                                               \
      }                                                                 \
    } else {                                                            \
      obj.set_time(Time::currentish_time());                            \
    }                                                                   \
  }

#define ORDER_ID(obj)                                   \
  obj.order_id + ((uint64_t)obj.symbol_index << 32)
  
#define NEW_ORDER_ID(obj)                               \
  obj.new_order_id + ((uint64_t)obj.symbol_index << 32)

#define GET_SYMBOL(msg)                                                \
  ProductID id = m_symbol_to_product_id.find_uint64(msg.symbol_index); \
  if(unlikely(m_qbdf_builder && id == -1)) { id = product_id_of_symbol_index(msg.symbol_index); } \
  symbol_info* si = (id != -1) ? &m_symbol_info_vector[id] : 0; 

  size_t
  NYSE_XDP_Integrated_Handler::parse(size_t context, const char* buf, size_t len)
  {
    if(len < sizeof(packet_header)) {
      send_alert("%s -- %s::%s:  Buffer of size %zd smaller than minimum packet length %zd", m_name.c_str(), CLASS_NAME, __func__, m_name.c_str(), len, sizeof(message_header));
      return 0;
    }

    if(m_drop_packets) {
      --m_drop_packets;
      return len;
    }

    if (m_qbdf_builder) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        LOG(INFO, "Got time %lu, equivalent to %lu", m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }
    
    channel_info& ci = m_channel_info_map[context];

    const packet_header& packet = reinterpret_cast<const packet_header&>(*buf);

    uint16_t pkt_size = packet.pkt_size;
    uint8_t  delivery_flag = packet.delivery_flag;
    uint8_t  num_msgs = packet.number_msgs;
    uint32_t seq_no = packet.seq_num;
    uint32_t next_seq_no = seq_no + num_msgs;

    if(pkt_size != len) {
      send_alert("%s -- %s::%s: %s Buffer of size %zd smaller than packet length %d", m_name.c_str(), CLASS_NAME, __func__, len, pkt_size);
      return 0;
    }

    if(delivery_flag != 21) {
      // Normal path
      if(seq_no < ci.seq_no) {
	return len; // duplicate
      } else if(seq_no > ci.seq_no) {
	if(ci.begin_recovery(this, buf, len, seq_no, next_seq_no)) {
	  return len;
	}
      }
    } else {
      // sequence number reset
      //const sequence_reset_message& msg = reinterpret_cast<const sequence_reset_message&>(*(buf + sizeof(message_header)));
      send_alert_forced("%s -- %s::%s received sequence reset message orig=%d  new=%d", m_name.c_str(), CLASS_NAME, __func__, seq_no, next_seq_no);
      ci.clear();
    }
    
    NYSE_XDP_Integrated_Handler::parse2(context, buf, len);
    
    ci.seq_no = next_seq_no;
    ci.last_update = Time::currentish_time();

    if(!ci.queue.empty()) {
      ci.process_recovery(this);
    }
    return len;
  }

  size_t
  NYSE_XDP_Integrated_Handler::parse2(size_t context, const char* orig_buf, size_t len)
  {
    const char* buf = orig_buf;
    const char* buf_end = orig_buf + len;

    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
    }
    if(m_record_only) {
      return len;
    }

    const packet_header& packet = reinterpret_cast<const packet_header&>(*buf);
    uint8_t  num_msgs = packet.number_msgs;

    //uint8_t delivery_flag = packet.delivery_flag;

    Time send_time = Time((Time_Constants::ticks_per_second * (uint64_t)packet.send_time) + (uint64_t)packet.send_time_ns);

    buf += sizeof(packet_header);

    for(int msg_idx = 0; msg_idx < num_msgs; ++msg_idx) {
      const message_header& header = reinterpret_cast<const message_header&>(*buf);

      uint16_t msg_size = header.msg_size;
      uint16_t msg_type = header.msg_type;

      const char* msg_buf = buf + sizeof(message_header);
      buf += msg_size;

      if(buf > buf_end) {
	send_alert("NYSE_XDP_Integrated: %s overran buffer length, len=%zd, msg_type=%d, msg_size=%d", m_name.c_str(), len, msg_type, msg_size);
	break;
      }

      switch(msg_type) {
      case 1: // sequence_number_reset
	break;

      case 2: // Time Reference message
	{
	  const source_time_reference_message& msg = reinterpret_cast<const source_time_reference_message&>(*msg_buf);
	  uint32_t id = msg.id;
          if(unlikely(id >= m_time_references.size())) {
            m_time_references.resize(id + 1);
          }
	  m_time_references[id] = Time(Time_Constants::ticks_per_second * (uint64_t)msg.time_reference);
          m_time_references[0]  = Time(Time_Constants::ticks_per_second * (uint64_t)msg.time_reference);
          
	}
	break;
        
      case 3: // Symbol index mapping message
	{
          /*
	  const symbol_index_mapping_message& msg = reinterpret_cast<const symbol_index_mapping_message&>(*msg_buf);
          
	  uint32_t symbol_index = msg.symbol_index;
	  symbol_info* si = get_symbol_info(context, symbol_index);
	  if(si->product_id == Product::invalid_product_id) {
	    // symbols should be only 10 characters, but I like to be double-sure and make sure we don't
	    // crash should a symbol not be null-terminated
	    //char symbol[12];
	    //strncpy(symbol, msg.symbol, 11);
	    //symbol[11] = '\0';
            product_id_of_symbol_index(symbol_index, si);
	  }

	  si->price_scale_code = msg.price_scale_code;
          */
	}
	break;
      case 5: // Option Series Index Mapping
	break;

      case 10: // Restransmission Request Message
	break;

      case 11: // Request Response Message
	break;

      case 12: // Heartbeat Response Message
	break;

      case 13: // Symbol Index Mapping Request Message
	break;

      case 15: // Refresh Request Message
	break;

      case 31: // Message Unavailable
	break;

      case 32: // Symbol Clear
        {
          const symbol_clear_message& msg = reinterpret_cast<const symbol_clear_message&>(*msg_buf);
          
          ProductID id = m_symbol_to_product_id.find_uint64(msg.symbol_index); \
	  if (m_order_book) {
	    if(id != Product::invalid_product_id) {
              m_order_book->clear(id, true);
              if (m_qbdf_builder) {
                gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, id, m_exch_char, context);
              }
            }
          }
        }
	break;
      case 33: // trading session change
	{
          const trading_session_change_message& msg = reinterpret_cast<const trading_session_change_message&>(*msg_buf);

	  if (m_order_book) {
            GET_SYMBOL(msg);
            if(id != Product::invalid_product_id) {
              uint8_t new_session = msg.trading_session;
	      if (new_session == 1) {
		break;
	      }
              
	      Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
	      m_micros_exch_time = t.total_usec() - m_micros_midnight;
	      m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

	      bool inside_changed = false;
              
	      unsigned int deleted_count = 0;
              unsigned int trigger_count = 0;
              
	      if(inside_changed) {
		m_quote.set_time(t);
		m_quote.m_id = id;
		send_quote_update();
	      }
              
	      if (m_qbdf_builder && deleted_count) {
                LOG(DEBUG, "Trade session change [product id=%u] [new_session=%u] obsolete orders: %u, deleted orders: %u", id, new_session, trigger_count, deleted_count);
	      }
	    }
          }
        }
	break;
        
      case 34: // security status
        {
          const security_status_message& msg = reinterpret_cast<const security_status_message&>(*msg_buf);
          ProductID id = m_symbol_to_product_id.find_uint64(msg.symbol_index); \
          if(id != Product::invalid_product_id) {
            Time t =  Time(Time_Constants::ticks_per_second * (uint64_t)msg.source_time) + Time_Duration(msg.source_time_ns);
	    m_micros_exch_time = t.total_usec() - m_micros_midnight;
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            
            m_msu.set_time(t);
            m_msu.m_id = id;
            m_msu.m_market_status = m_security_status[id];
            m_msu.m_ssr = SSRestriction::Unknown;
            m_msu.m_rpi = RetailPriceImprovement::Unknown;
            m_msu.m_reason.clear();
            
            QBDFStatusType qbdf_status;
            bool send_update = true;
            
            switch(msg.security_status) {
            case '3' :                                                qbdf_status = QBDFStatusType::OpeningDelay; break; 
            case '4' : m_msu.m_market_status =  MarketStatus::Halted; qbdf_status = QBDFStatusType::Halted; break;
            case '5' : m_msu.m_market_status =  MarketStatus::Open;   qbdf_status = QBDFStatusType::Open;   break;
            case '6' : m_msu.m_market_status =  MarketStatus::Halted; qbdf_status = QBDFStatusType::Halted; break;
              
            case 'A' : m_msu.m_ssr = SSRestriction::Activated; qbdf_status = QBDFStatusType::ShortSaleRestrictActivated; break;
            case 'C' : m_msu.m_ssr = SSRestriction::Continued; qbdf_status = QBDFStatusType::ShortSaleRestrictContinued; break;
            case 'D' : m_msu.m_ssr = SSRestriction::None;      qbdf_status = QBDFStatusType::NoShortSaleRestrictionInEffect; break;
              
            case 'O' : m_msu.m_market_status =  MarketStatus::Open;   qbdf_status = QBDFStatusType::Open;   break;
            case 'X' : m_msu.m_market_status =  MarketStatus::Closed; qbdf_status = QBDFStatusType::Closed;   break;
              
            case '~' : m_msu.m_ssr = SSRestriction::None;      qbdf_status = QBDFStatusType::NoShortSaleRestrictionInEffect; break;
            case 'E' : m_msu.m_ssr = SSRestriction::Activated; qbdf_status = QBDFStatusType::ShortSaleRestrictActivated; break;

            default:
              send_update = false;
              break;
            }
            if(send_update) {
              switch(msg.halt_condition) {
              case ' ': m_msu.m_reason = "Not applicable"; break;
              case '~': m_msu.m_reason = "Security not delayed/halted"; break;
              case 'D': m_msu.m_reason = "News dissemination"; break;
              case 'I': m_msu.m_reason = "Order imbalance"; break;
              case 'P': m_msu.m_reason = "News pending"; break;
              case 'M': m_msu.m_reason = "Volatility Trading Pause"; break;
              case 'S': m_msu.m_reason = "Reulated secuirty"; break;

              case 'Q': m_msu.m_reason = "Due to related security"; break;
              case 'V': m_msu.m_reason = "In view of common"; break;
              case 'X': m_msu.m_reason = "Equiptment changeover"; break;
              case 'Y': m_msu.m_reason = "Sub penny Trading"; break;
              case 'Z': m_msu.m_reason = "No open/No resume"; break;
              default:
                break;
              }
              
              if(!m_qbdf_builder) {
                if(m_subscribers) {
                  m_subscribers->update_subscribers(m_msu);
                }
              } else {
                gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time,
                                       m_micros_exch_time_delta, m_exch_char,
                                       qbdf_status.index(), m_msu.m_reason);
              }
            }
            m_security_status[id] = m_msu.m_market_status;
            
            m_msu.m_ssr = SSRestriction::Unknown;
            m_msu.m_rpi = RetailPriceImprovement::Unknown;
            m_msu.m_reason.clear();
          }
        }
        break;

      case 35: // Refresh Header Message
        {
          // We don't handle refreshes for now, skip
          //const refresh_header& msg = reinterpret_cast<const refresh_header&>(*msg_buf);
          return len;
        }
	break;
      case 100: // Add Order Message
	{
	  const add_order_message& msg = reinterpret_cast<const add_order_message&>(*msg_buf);
	  GET_SYMBOL(msg);
	  if(id != Product::invalid_product_id) {
	    Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
	    m_micros_exch_time = t.total_usec() - m_micros_midnight;
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            
	    if (m_order_book) {
	      MDPrice price = get_int_price(msg.price, si->price_scale_code);
	      if (!price.blank()) {
		uint64_t order_id = ORDER_ID(msg);
		uint32_t size = msg.volume;
                
		if (m_qbdf_builder && !m_exclude_level_2) {
		  gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
					    m_micros_exch_time_delta, m_exch_char, msg.side, size, price);
		}
                
		bool inside_changed = m_order_book->add_order_ip(t, order_id, id, msg.side, price, size);
                
		if(inside_changed) {
		  m_quote.set_time(t);
		  m_quote.m_id = id;
		  send_quote_update();
		}
	      }
	    }
	  }
        }
	break;
      case 101: // Modify Order Message
	{
	  const modify_order_message& msg = reinterpret_cast<const modify_order_message&>(*msg_buf);
          GET_SYMBOL(msg);
	  if(id != Product::invalid_product_id) {
	    Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
	    m_micros_exch_time = t.total_usec() - m_micros_midnight;
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            
	    bool inside_changed = false;
	    if (m_order_book) {
	      MDPrice price = get_int_price(msg.price, si->price_scale_code);
	      if (!price.blank()) {
		uint64_t order_id = ORDER_ID(msg);
		uint32_t size = msg.volume;
                
                MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
		MDOrder* order = m_order_book->unlocked_find_order(order_id);
                if(order) {
                  if(m_qbdf_builder && !m_exclude_level_2) {
                    gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                                 m_micros_exch_time_delta, m_exch_char, order->side(), size, price);
                  }
                  
                  MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
		  if(price == order->price && msg.position_change == 0) {
		    inside_changed = m_order_book->unlocked_modify_order(t, order, size);
		  } else {
		    inside_changed = m_order_book->unlocked_modify_order_ip(t, order, order->side(), price, size);
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
	}
	break;
      case 102: // Delete Order Message
	{
	  const delete_order_message& msg = reinterpret_cast<const delete_order_message&>(*msg_buf);
	  GET_SYMBOL(msg);
	  if(id != Product::invalid_product_id) {
	    if (m_order_book) {
	      Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
	      m_micros_exch_time = t.total_usec() - m_micros_midnight;
	      m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              
	      uint64_t order_id = ORDER_ID(msg);
	      if(m_qbdf_builder && !m_exclude_level_2) {
		gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
					     m_micros_exch_time_delta, m_exch_char, ' ', 0, MDPrice());
	      }

              bool inside_changed = false;
              {
                MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
                MDOrder* order = m_order_book->unlocked_find_order(order_id);
                if(order) {
                  MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
                  inside_changed = m_order_book->unlocked_remove_order(t, order);
                }
              }
              
	      if(inside_changed) {
		m_quote.set_time(t);
		m_quote.m_id = id;
		send_quote_update();
	      }
	    }
	  }
	}
	break;
      case 103: // Execution Message
	{
	  const execution_message& msg = reinterpret_cast<const execution_message&>(*msg_buf);
          GET_SYMBOL(msg);
          if(id != Product::invalid_product_id) {
	    if (m_order_book) {
	      Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
	      m_micros_exch_time = t.total_usec() - m_micros_midnight;
	      m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              
	      uint64_t order_id = ORDER_ID(msg);
              bool inside_changed = false;

              MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
              MDOrder* order = m_order_book->unlocked_find_order(order_id);
              
              char side = 'U';
              uint32_t new_size = 0;
              
              if(order) {
                side = order->flip_side();
              }
              
              MDPrice price = get_int_price(msg.price, si->price_scale_code);
              
              if(msg.printable_flag) {
                m_trade.set_time(t);
                m_trade.m_id = id;
                m_trade.m_price = price.fprice();
                m_trade.m_size = msg.volume;
                m_trade.clear_flags();
                m_trade.set_side(side);
                m_trade.m_trade_qualifier[0] = ' ';
                if(order && order->price != price) {
                  m_trade.set_exec_flag(TradeUpdate::exec_flag_exec_w_price);
                } else {
                  m_trade.set_exec_flag(TradeUpdate::exec_flag_exec);
                }
              }
              
              if(order) {
                MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
                if(msg.volume >= order->size) {
                  inside_changed = m_order_book->unlocked_remove_order(t, order, msg.volume);
                } else {
                  new_size = order->size - msg.volume;
                  inside_changed = m_order_book->unlocked_modify_order(t, order, new_size, msg.volume);
                }
              }
              
              if(!m_qbdf_builder) {
                if(msg.printable_flag) {
                  send_trade_update();
                }
                if(inside_changed) {
                  m_quote.set_time(t);
                  m_quote.m_id = id;
                  send_quote_update();
                }
              } else {
                if(msg.printable_flag) {
                  gen_qbdf_order_exec_micros(m_qbdf_builder, id, order_id,
                                             m_micros_recv_time, m_micros_exch_time_delta,
                                             m_exch_char, m_trade.side(), ' ',
                                             msg.volume, price);
                } else {
                  if(!m_exclude_level_2 && order) {
                    gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id,
                                                 m_micros_recv_time, m_micros_exch_time_delta,
                                                 m_exch_char, order->side(), new_size, price);
                    
                  }
                }
              }
              
              if(m_order_book->orderbook_update()) {
                m_order_book->post_orderbook_update();
              }
            }
	  }
	}
	break;
      case 104: // replace_order_message
        {
          const replace_order_message& msg = reinterpret_cast<const replace_order_message&>(*msg_buf);
          GET_SYMBOL(msg);
	  if(id != Product::invalid_product_id) {
            Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
	    m_micros_exch_time = t.total_usec() - m_micros_midnight;
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            
	    bool inside_changed = false;
	    if (m_order_book) {
	      MDPrice price = get_int_price(msg.price, si->price_scale_code);
	      if (!price.blank()) {
		uint64_t order_id = ORDER_ID(msg);
                uint64_t new_order_id = NEW_ORDER_ID(msg);
		uint32_t size = msg.volume;
                
                MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
		MDOrder* order = m_order_book->unlocked_find_order(order_id);
                if(order) {
                  if(m_qbdf_builder && !m_exclude_level_2) {
                    gen_qbdf_order_replace_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                                  m_micros_exch_time_delta, m_exch_char, order->side(),
                                                  new_order_id, size, price);
                  }
                  
                  MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
		  inside_changed = m_order_book->unlocked_replace_order_ip(t, order, order->side(), price, size, new_order_id, false);
		}
	      }
              
	      if(inside_changed) {
		m_quote.set_time(t);
		m_quote.m_id = id;
		send_quote_update();
	      }
	    }

            
          }
        }
        break;
      case 105: // Imbalance Message
	{
	  const imbalance_message& msg = reinterpret_cast<const imbalance_message&>(*msg_buf);
	  GET_SYMBOL(msg);
	  if(id != Product::invalid_product_id) {
	    m_auction.m_id = id;
	    SET_SOURCE_TIME(m_auction);

	    MDPrice ref_price = get_int_price(msg.reference_price, si->price_scale_code);
            MDPrice near_price = get_int_price(msg.continuous_book_clearing_price, si->price_scale_code);
            MDPrice far_price = get_int_price(msg.closing_only_clearing_price, si->price_scale_code);

	    // XDP has new fields that could be used as near/ far price (continuous_book_clearing_price, closing_only_clearing_price)
	    // but these are not currently implemented (values of 0 until then), continue using following logic for now
	    m_auction.m_ref_price = ref_price.fprice();
	    m_auction.m_far_price = far_price.fprice();
	    m_auction.m_near_price = near_price.fprice();
	    m_auction.m_paired_size = msg.paired_qty;
	    m_auction.m_imbalance_size = msg.total_imbalance_qty;
	    
            m_auction.m_imbalance_direction = msg.imbalance_side;
            
	    if (m_qbdf_builder && !m_exclude_imbalance) {
	      gen_qbdf_imbalance_micros(m_qbdf_builder, m_auction.m_id, m_micros_recv_time,
					m_micros_exch_time_delta, m_exch_char, ref_price, near_price, far_price,
					m_auction.m_paired_size, m_auction.m_imbalance_size,
					m_auction.m_imbalance_direction, ' ');
	    }

	    if(m_subscribers) {
	      m_subscribers->update_subscribers(m_auction);
	    }
	  }
	}
	break;
      case 106: // OrderBook Add Order Refresh Message
	{
	  const add_order_refresh_message& msg = reinterpret_cast<const add_order_refresh_message&>(*msg_buf);
	  GET_SYMBOL(msg);
	  if(id != Product::invalid_product_id) {
	    Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
	    m_micros_exch_time = t.total_usec() - m_micros_midnight;
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
            
	    if (m_order_book) {
	      MDPrice price = get_int_price(msg.price, si->price_scale_code);
	      if (!price.blank()) {
		uint64_t order_id = ORDER_ID(msg);
		uint32_t size = msg.volume;
                
		if (m_qbdf_builder && !m_exclude_level_2) {
		  gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
					    m_micros_exch_time_delta, m_exch_char, msg.side, size, price);
		}
                
		bool inside_changed = m_order_book->add_order_ip(t, order_id, id, msg.side, price, size);
                
		if(inside_changed) {
		  m_quote.set_time(t);
		  m_quote.m_id = id;
		  send_quote_update();
		}
	      }
	    }
	  }
	}
	break;
      case 110:
        {
          const non_displayed_trade_message& msg = reinterpret_cast<const non_displayed_trade_message&>(*msg_buf);
          GET_SYMBOL(msg);
	  if(id != Product::invalid_product_id) {
	    Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
	    m_micros_exch_time = t.total_usec() - m_micros_midnight;
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
        
            MDPrice price = get_int_price(msg.price, si->price_scale_code);
            
            if(msg.printable_flag) {
              m_trade.set_time(t);
              m_trade.m_id = id;
              m_trade.m_price = price.fprice();
              m_trade.m_size = msg.volume;
              m_trade.clear_flags();
              m_trade.set_side('U');
              m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
              m_trade.m_trade_qualifier[0] = ' ';
              
              if(!m_qbdf_builder) {
                send_trade_update();
              } else {
                gen_qbdf_trade_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                                      m_exch_char, 'U', '0', "    ", msg.volume, price);
              }
            }
          }
	}
	break;
      case 111:
        {
          const cross_trade_message& msg = reinterpret_cast<const cross_trade_message&>(*msg_buf);
          GET_SYMBOL(msg);
	  if(id != Product::invalid_product_id) {
            Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
	    m_micros_exch_time = t.total_usec() - m_micros_midnight;
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
        
            MDPrice price = get_int_price(msg.price, si->price_scale_code);
            
            m_msu.set_time(t);
            m_msu.m_id = id;
            
            m_trade.set_time(t);
            m_trade.m_id = id;
            m_trade.m_price = price.fprice();
            m_trade.m_size = msg.volume;
            m_trade.clear_flags();
            m_trade.set_side('U');
            m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
            m_trade.m_trade_qualifier[0] = msg.cross_type;
            
            if(!m_qbdf_builder) {
              send_trade_update();
            } else {
              char cond[5] = { msg.cross_type, ' ', ' ', ' ', '\0'}; 
              gen_qbdf_trade_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                                    m_exch_char, 'U', '0', cond, msg.volume, price);
            }
          }
        }
        break;
      case 112:
        {
          const trade_cancel_message& msg = reinterpret_cast<const trade_cancel_message&>(*msg_buf);
          GET_SYMBOL(msg);
          if (m_qbdf_builder) {
            if(id != Product::invalid_product_id) {
              Time t = m_time_references[si->matching_engine] + Time_Duration(msg.source_time_ns);
              m_micros_exch_time = t.total_usec() - m_micros_midnight;
              m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
              
              gen_qbdf_cancel_correct(m_qbdf_builder, id,
                                      m_micros_exch_time / Time_Constants::micros_per_mili,
                                      0, "", "CANCEL", msg.trade_id,
                                      0, 0, "", 0, 0, "");
            }
          }
        }
        break;
      default:
	m_logger->log_printf(Log::INFO, "Received unknown msg_type %d", (int)msg_type);
	break;
      }
    }
    
    return len;
  }

  size_t
  NYSE_XDP_Integrated_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len)
  {
    const packet_header& packet = reinterpret_cast<const packet_header&>(*buf);

    uint8_t  num_msgs = packet.number_msgs;
    Time send_time = Time((Time_Constants::ticks_per_second * (uint64_t)packet.send_time) + (uint64_t)packet.send_time_ns);

    char time_buf[32];
    Time_Utils::print_time(time_buf, send_time, Timestamp::MICRO);

    log.printf(" send_time:%s seq_num:%u entries:%u\n", time_buf, ntohl(packet.seq_num), num_msgs);

    buf += sizeof(packet_header);

    for(int msg_idx = 0; msg_idx < num_msgs; ++msg_idx) {
      const message_header& header = reinterpret_cast<const message_header&>(*buf);

      uint16_t msg_size = header.msg_size;
      uint16_t msg_type = header.msg_type;

      const char* msg_buf = buf + sizeof(message_header);
      buf += msg_size;

      switch(msg_type) {
      case 1: // sequence_number_reset
        log.printf("  sequence_number_reset\n");
        break;
      case 2: // Time Reference message
	{
	  const source_time_reference_message& msg = reinterpret_cast<const source_time_reference_message&>(*msg_buf);
	  
	  log.printf("  time_reference id:%u symbol_seq_num:%u time_reference:%u", msg.id, msg.symbol_seq_num, msg.time_reference);
	}
	break;
      case 3: // Symbol index mapping message
	{
	  const symbol_index_mapping_message& msg = reinterpret_cast<const symbol_index_mapping_message&>(*msg_buf);

          log.printf("  symbol_index_mapping  symbol_index:%u symbol:%.11s market_id:%u system_id:%u exch:%c price_scale_code:%u security_type:%c UOT:%u prev_close_price:%u prev_close_volume:%u price_resolution:%u round_lot:%c\n",
                     msg.symbol_index, msg.symbol, msg.market_id, msg.system_id, msg.exchange_code, msg.price_scale_code,
                     msg.security_type, msg.UOT, msg.prev_close_price, msg.prev_close_volume, msg.price_resolution, msg.round_lot);
        }
        break;
      case 32: // symbol clear mesasge
        {
          const symbol_clear_message& msg = reinterpret_cast<const symbol_clear_message&>(*msg_buf);
          log.printf("  symbol_clear  source_time:%u  source_time_ns:%u  symbol_index:%u  next_symbol_seq_num:%u\n",
                     msg.source_time, msg.source_time_ns, msg.symbol_index, msg.next_symbol_seq_num);
        }
        break;
      case 33: // Trading Session Change
        {
          const trading_session_change_message& msg = reinterpret_cast<const trading_session_change_message&>(*msg_buf);
          
          log.printf("  trading_session_change source_time:%u  source_time_ns:%u  symbol_index:%u  symbol_seq_num:%u  trading_session:%u\n",
                     msg.source_time, msg.source_time_ns, msg.symbol_index, msg.symbol_seq_num, msg.trading_session);
        }
        break;
      case 34: // security status
        {
          const security_status_message& msg = reinterpret_cast<const security_status_message&>(*msg_buf);
          log.printf("  security_status source_time:%u  source_time_ns:%u  symbol_index:%u  symbol_seq_num:%u  security_status:%c  halt_condition:%c\n",
                     msg.source_time, msg.source_time_ns, msg.symbol_index, msg.symbol_seq_num, msg.security_status, msg.halt_condition);
        }
        break;
      case 35: // refresh header
        {
          const refresh_header& msg = reinterpret_cast<const refresh_header&>(*msg_buf);
          
          log.printf("  refresh_header current_refresh_pkt:%hu  total_refresh_pkts:%hu  last_seq_num:%u  last_symbol_seq_num:%u\n",
                     msg.current_refresh_pkt, msg.total_refresh_pkts, msg.last_seq_num, msg.last_symbol_seq_num);
        }
        break;
      case 100: // Add Order Message
	{
	  const add_order_message& msg = reinterpret_cast<const add_order_message&>(*msg_buf);

          uint32_t symbol_index = msg.symbol_index;
          log.printf("  add_order source_time_ns:%u symbol_index:%u symbol_seq_num:%u order_id:%lu price:%u volume:%u side:%c firm_id:%.5s num_parity_splits:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.order_id, msg.price, msg.volume, msg.side, msg.firm_id, msg.num_parity_splits);
	}
	break;
      case 101: // Modify Order Message
	{
	  const modify_order_message& msg = reinterpret_cast<const modify_order_message&>(*msg_buf);

          uint32_t symbol_index = msg.symbol_index;
          log.printf("  modify_order source_time_ns:%u symbol_index:%u symbol_seq_num:%u order_id:%lu price:%u volume:%u position_change:%u prev_price_parity_splits:%u new_price_parity_splits:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.order_id, msg.price, msg.volume, msg.position_change, msg.prev_price_parity_splits, msg.new_price_parity_splits);
	}
	break;
      case 102: // Delete Order Message
	{
	  const delete_order_message& msg = reinterpret_cast<const delete_order_message&>(*msg_buf);

          uint32_t symbol_index = msg.symbol_index;
          log.printf("  delete_order source_time_ns:%u symbol_index:%u symbol_seq_num:%u order_id:%lu num_parity_splits:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.order_id, msg.num_parity_splits);
	}
	break;
      case 103: // Execution Message
	{
	  const execution_message& msg = reinterpret_cast<const execution_message&>(*msg_buf);

          uint32_t symbol_index = msg.symbol_index;
          log.printf("  execute_order source_time_ns:%u symbol_index:%u symbol_seq_num:%u order_id:%lu price:%u volume:%u printable_flag:%u num_parity_splits:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.order_id, msg.price, msg.volume, msg.printable_flag, msg.num_parity_splits);
	}
	break;
      case 104: // replace_order_message
        {
          const replace_order_message& msg = reinterpret_cast<const replace_order_message&>(*msg_buf);

          uint32_t symbol_index = msg.symbol_index;
          log.printf("  replace_order source_time_ns:%u symbol_index:%u symbol_seq_num:%u order_id:%lu new_order_id:%lu price:%u volume:%u prev_price_parity_splits:%u new_price_parity_splits:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.order_id, msg.new_order_id, msg.price, msg.volume, msg.prev_price_parity_splits, msg.new_price_parity_splits);
        }
        break;
      case 105:
	{
	  const imbalance_message& msg = reinterpret_cast<const imbalance_message&>(*msg_buf);
	  log.printf("  imbalance_message source_time_ns:%u symbol_index:%u symbol_seq_num:%u ref_price:%u paired_qty:%u total_imbalance_qty:%d market_imbalance_qty:%d side:%c\n",
		     msg.source_time_ns, msg.symbol_index, msg.symbol_seq_num, msg.reference_price, msg.paired_qty, msg.total_imbalance_qty, msg.market_imbalance_qty, msg.imbalance_side);
	}
	break;
      case 106:
        {
	  const add_order_refresh_message& msg = reinterpret_cast<const add_order_refresh_message&>(*msg_buf);
          
          uint32_t symbol_index = msg.symbol_index;
          log.printf("  add_order_refresh source_time_ns:%u symbol_index:%u symbol_seq_num:%u order_id:%lu price:%u volume:%u side:%c firm_id:%.5s num_parity_splits:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.order_id, msg.price, msg.volume, msg.side, msg.firm_id, msg.num_parity_splits);
        }
        break;
      case 110:
        {
          const non_displayed_trade_message& msg = reinterpret_cast<const non_displayed_trade_message&>(*msg_buf);
          
          uint32_t symbol_index = msg.symbol_index;
          log.printf("  non_displayed_trade_message source_time_ns:%u symbol_index:%u symbol_seq_num:%u trade_id:%u price:%u volume:%u printable_flag:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.trade_id, msg.price, msg.volume, msg.printable_flag);
        }
        break;
      case 111:
        {
          const cross_trade_message& msg = reinterpret_cast<const cross_trade_message&>(*msg_buf);
          
          uint32_t symbol_index = msg.symbol_index;
          log.printf("  cross_trade_message source_time_ns:%u symbol_index:%u symbol_seq_num:%u cross_id:%u price:%u volume:%u cross_type:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.cross_id, msg.price, msg.volume, msg.cross_type);
        }
        break;
      case 112:
        {
          const trade_cancel_message& msg = reinterpret_cast<const trade_cancel_message&>(*msg_buf);
          
          uint32_t symbol_index = msg.symbol_index;
          log.printf("  trade_cancel_message source_time_ns:%u symbol_index:%u symbol_seq_num:%u trade_id:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.trade_id);
        }
        break;
      case 113:
        {
          const cross_correction_message& msg = reinterpret_cast<const cross_correction_message&>(*msg_buf);
          
          uint32_t symbol_index = msg.symbol_index;
          log.printf("  cross_correction_message source_time_ns:%u symbol_index:%u symbol_seq_num:%u cross_id:%u volume:%u\n",
                     msg.source_time_ns, symbol_index, msg.symbol_seq_num, msg.cross_id, msg.volume);
        }
        break;
      case 223: // Stock Summary
	{
	  const stock_summary_message& msg = reinterpret_cast<const stock_summary_message&>(*msg_buf);

	  uint32_t symbol_index = msg.symbol_index;

	  Time source_time = Time((Time_Constants::ticks_per_second * (uint64_t)msg.source_time) + (uint64_t)msg.source_time_ns);
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
	  log.printf ( " stock_summary source_time:%s symbol_index:%u high_pirce:%u low_price:%u open:%u close:%u total_volume:%u\n",
		       time_buf, symbol_index, msg.high_price, msg.low_price, msg.open, msg.close, msg.total_volume );
	}
	break ;
      default : // we don't know the messsage type yet.
	log.printf ( "msg_type %d: not implemented\n", msg_type );
      }
      log.puts("\n");
    }

    return packet_len;
  }

  void
  NYSE_XDP_Integrated_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);

    m_name = name;
    
    if (params.getDefaultBool("build_book", false)) {
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_books.push_back(m_order_book);
      m_order_book->init(params);
      int prealloc_orders = params.getDefaultInt("prealloc_orders", 50000);
      m_order_book->prealloc_order_pool(prealloc_orders);
    }
    
    m_symbol_info_vector.resize(m_app->pm()->max_product_id());
        
    string symbol_mapping = params["symbol_mapping"].getString();
    {
      LOG(INFO, "Reading symbol mapping from %s", symbol_mapping.c_str());
      
      CSV_Parser parser(symbol_mapping);
      
      int symbol_index_idx = parser.column_index("symbol_index");
      int price_scale_code_idx = parser.column_index("price_scale_code");
      int system_id_idx = parser.column_index("system_id");
      
      while(CSV_Parser::OK == parser.read_row()) {
	int symbol_index = strtol(parser.get_column(symbol_index_idx), 0, 10);
	int price_scale_code = strtol(parser.get_column(price_scale_code_idx), 0, 10);
        int system_id = strtol(parser.get_column(system_id_idx), 0, 10);
        
	symbol_info* si = &m_symbol_info_map[symbol_index];
	si->price_scale_code = price_scale_code;
        si->matching_engine = system_id;
        
        if(si->matching_engine >= m_time_references.size()) {
          m_time_references.resize(si->matching_engine + 1);
        }
        
	//cout << symbol_index << "," << price_scale_code << "," << m_symbol_info_map[symbol_index].price_scale_code << endl;
      }
    }

    m_source_id = params.getDefaultString("source_id", "");

    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;

    if (exch == ExchangeID::XNYS) {
      m_exch_char = 'N';
    } else {
      m_exch_char = 'P';
    }
    
    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';

    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);
    
    m_micros_midnight = Time::today().ticks() / Time_Constants::ticks_per_micro;
    
    m_lot_size = params.getDefaultInt("lot_size", m_lot_size);
    
    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);
    
    m_max_send_source_difference = params.getDefaultTimeDuration("max_send_source_difference", m_max_send_source_difference);
    
    if(params.has("nyse_openbook")) {
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_book->init(params);
    }
    
    if(params.getDefaultBool("use_old_imbalance", false)) {
      m_use_old_imbalance = true;
    } else {
      m_use_old_imbalance = false;
    }
  }

  void
  NYSE_XDP_Integrated_Handler::start()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }

    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  NYSE_XDP_Integrated_Handler::stop()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }

    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  NYSE_XDP_Integrated_Handler::send_retransmit_request(channel_info& cib)
  {
    channel_info& ci = static_cast<channel_info&>(cib);

    if(!ci.recovery_socket.connected()) {
      ci.has_recovery = false;
      m_logger->log_printf(Log::INFO, "%s sent retransmit request called but socket =- -1", ci.name.c_str());
      return;
    }

    uint32_t begin = ci.seq_no;
    uint32_t end = ci.queue.top().seq_num;

    struct {
      packet_header  pkt_header;
      message_header msg_header;
      retransmission_request_message payload;
    } req;

    ++m_req_seq_num;

    Time t = Time::current_time();
    uint32_t send_time = t.sec();
    uint32_t send_time_ns = t.nsec();

    memset(&req, 0, sizeof(req));
    req.pkt_header.pkt_size = htons(sizeof(req));
    req.pkt_header.delivery_flag = 11;
    req.pkt_header.number_msgs = 1;
    req.pkt_header.seq_num = m_req_seq_num;
    req.pkt_header.send_time = htonl(send_time);
    req.pkt_header.send_time_ns = htonl(send_time_ns);
    req.msg_header.msg_size = htons(sizeof(message_header) + sizeof(retransmission_request_message));
    req.msg_header.msg_type = htons(11);

    req.payload.begin_seq_num = htonl(begin);
    req.payload.end_seq_num = htonl(end);
    strncpy(req.payload.source_id, m_source_id.c_str(), 10);
    //req.payload.product_id = m_product_id;
    //req.payload.channel_id = channel_id;

    int ret = 0;
    m_logger->log_printf(Log::INFO, "%s sent retransmit request via TCP %d to %d", ci.name.c_str(), begin, end);
    ret = ::send(ci.recovery_socket, &req, sizeof(req), MSG_DONTWAIT);

    if(ret < 0) {
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert("NYSE_XDP_Integrated_Handler::send_retransmit_request %s sendto returned %d - %s", m_name.c_str(), errno, err_msg);
    }

    ci.sent_resend_request = true;
  }

  void
  NYSE_XDP_Integrated_Handler::send_heartbeat_response(channel_info& ci)
  {
    if(!ci.recovery_socket.connected()) {
      return;
    }

    struct {
      packet_header  pkt_header;
      message_header msg_header;
      heartbeat_response_message payload;
    } resp;

    ++m_req_seq_num;

    Time t = Time::current_time();
    uint32_t send_time = t.sec();
    uint32_t send_time_ns = t.nsec();

    memset(&resp, 0, sizeof(resp));
    resp.pkt_header.pkt_size = htons(sizeof(resp));
    resp.pkt_header.delivery_flag = 11;
    resp.pkt_header.number_msgs = 1;
    resp.pkt_header.seq_num = m_req_seq_num;
    resp.pkt_header.send_time = htonl(send_time);
    resp.pkt_header.send_time_ns = htonl(send_time_ns);
    resp.msg_header.msg_size = htons(sizeof(message_header) + sizeof(heartbeat_response_message));
    resp.msg_header.msg_type = htons(12);
    strncpy(resp.payload.source_id, m_source_id.c_str(), 10);

    if(ci.recovery_socket.connected()) {
      if(send(ci.recovery_socket, &resp, sizeof(resp), 0) < 0) {
        ci.recovery_socket.close();
        send_alert("%s error sending heartbeat response, closing socket", m_name.c_str());
      }
    }
  }
  
  size_t
  NYSE_XDP_Integrated_Handler::read(size_t context, const char* buf, size_t len)
  {
    if(len < 2) {
      return 0;
    }
    
    const packet_header& pkt_header = reinterpret_cast<const packet_header&>(*buf);
    
    uint16_t pkt_size = pkt_header.pkt_size;
    if(len < pkt_size) {
      return 0;
    }
    
    return pkt_size;
  }
  
  void
  NYSE_XDP_Integrated_Handler::add_context_product_id(size_t context, ProductID id)
  {
    if(m_context_to_product_ids.size() <= context) {
      m_context_to_product_ids.resize(context+1);
    }
    m_context_to_product_ids[context].push_back(id);
  }

  void
  NYSE_XDP_Integrated_Handler::subscribe_product(ProductID id, ExchangeID exch, const string& mdSymbol, const string& mdExch)
  {
    mutex_t::scoped_lock lock(m_mutex);
    
    uint32_t symbol_index = strtol(mdSymbol.c_str(), 0, 10);
    
    if(symbol_index == 0) {
      m_logger->log_printf(Log::ERROR, "NYSE_XDP_Integrated_Handler::subscribe_product:  Called for invalid symbol_index '%s', product_id %d",
			   mdSymbol.c_str(), id);
      return;
    }
    
    symbol_info* si = &m_symbol_info_map[symbol_index];
    if(si->price_scale_code == -1) {
      m_logger->log_printf(Log::ERROR, "NYSE_XDP_Integrated_Handler::subscribe_product:  Unable to find symbol_index '%s', product_id %d in symbol_info_map (no price_scale_code)",
			   mdSymbol.c_str(), id);
      return;
    }
    
    m_symbol_to_product_id.insert_uint64(symbol_index, id);
    symbol_info* ssi = &m_symbol_info_vector[id];
    *ssi = *si;
  }

  void
  NYSE_XDP_Integrated_Handler::reset(size_t context, const char* msg)
  {
    reset(msg);
    return;
    
    if(context >= m_context_to_product_ids.size() || m_context_to_product_ids[context].empty()) {
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
      if (m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_char, context);
      }
      
      for(vector<ProductID>::const_iterator p = clear_products.begin(), p_end = clear_products.end();
          p != p_end; ++p) {
        m_order_book->clear(*p, false);
      }
    }
    m_logger->log_printf(Log::WARN, "NYSE_XDP_Integrated_Handler::reset %s called with %zd products", m_name.c_str(), clear_products.size());
  }

  void
  NYSE_XDP_Integrated_Handler::reset(const char* msg)
  {
    if(m_order_book) {
      if (m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_char, -1);
      }

      m_order_book->clear(false);
    }
    m_logger->log_printf(Log::WARN, "NYSE_XDP_Integrated_Handler::reset called %s -- %s", m_name.c_str(), msg);
  }

  void
  NYSE_XDP_Integrated_Handler::send_quote_update()
  {
    m_order_book->fill_quote_update(m_quote.m_id, m_quote);
    m_msu.m_market_status = m_security_status[m_quote.m_id];
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);

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
  NYSE_XDP_Integrated_Handler::send_trade_update()
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
  
  ProductID
  NYSE_XDP_Integrated_Handler::product_id_of_symbol_index(uint32_t symbol_index)
  {
    // If not qbdf_builder, all valid products have already been entered in the symbol_map
    if(m_qbdf_builder) {
      stringstream ss;
      ss << symbol_index;
      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(ss.str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol index [%d]",
                             "NYSE_Handler::product_id_of_symbol", symbol_index);
        m_logger->sync_flush();
        return -1;
      }
      
      m_symbol_to_product_id.insert_uint64(symbol_index, symbol_id);
      m_symbol_info_vector[symbol_id] = m_symbol_info_map[symbol_index];
      return symbol_id;
    }
    return -1;
  }
  

  NYSE_XDP_Integrated_Handler::NYSE_XDP_Integrated_Handler(Application* app) :
    Handler(app, "NYSE_XDP_Integrated"),
    m_symbol_to_product_id(32000),
    m_max_send_source_difference(msec(10000)),
    m_lot_size(100),
    m_nyse_product_id(4),
    m_req_seq_num(0),
    m_micros_midnight(0),
    m_exch_char(' ')
  {
    //m_symbol_info_map.resize(65536);
  }

  NYSE_XDP_Integrated_Handler::~NYSE_XDP_Integrated_Handler()
  {
  }



}
