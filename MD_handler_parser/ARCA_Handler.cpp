#include <Data/QBDF_Util.h>
#include <Data/ARCA_Handler.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>

#include <MD/MDProvider.h>
#include <Logger/Logger_Manager.h>
#include <Util/Network_Utils.h>

#include <stdarg.h>

#include "AB_Fast.h"

namespace hf_core {

  using namespace ARCA_structs;

  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  size_t expect_len = sizeof(message_header) + (header.num_body_entries * sizeof(type)); \
    if(len != expect_len) {                                             \
      send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), len, expect_len); \
      return len;                                                       \
    }                                                                   \

#define BODY_SIZE_CHECK(type)                       \
  if(body_size < sizeof(type)) {                                        \
    send_alert("%s %s called with " #type " remaing msg body size=%d, less then expecting %d", where, m_name.c_str(), body_size, sizeof(type)); \
    return len;                                                         \
  }                                                                     \

#define SET_SOURCE_TIME(obj, source_time)                               \
  bool stale_msg __attribute__((unused)) = false;                       \
  {                                                                     \
    if(source_time.is_set()) {                                          \
      obj.set_time( m_midnight + source_time);                          \
      Time_Duration diff = send_time - source_time;                     \
      if(diff.abs() > m_max_send_source_difference) {                   \
        Product& prod = m_app->pm()->find_by_id(obj.m_id);              \
        send_alert("%s: %s send_time %s is %s lagged from source_time %s for symbol %s", \
                   where, m_name.c_str(), send_time.to_string().c_str(), \
                   diff.to_string().c_str(), source_time.to_string().c_str(), prod.symbol().c_str()); \
        stale_msg = true;                                               \
      }                                                                 \
    } else {                                                            \
      obj.set_time(Time::currentish_time());                            \
    }                                                                   \
  }

#define ORDER_ID(l2_obj)                        \
  ntohll(l2_obj.iOrderID.lDLong)
  //ntohl(l2_obj.iOrderID.in.iID) + ((uint64_t) ntohs(l2_obj.iOrderID.in.iMarketID) << 32) + ((uint64_t) l2_obj.iOrderID.in.iSystemID << 48);

#define SECURITY_INDEX(l2_obj)                                      \
  (uint32_t)l2_obj.iSessionID * 8192 + (uint32_t)ntohs(l2_obj.iSec)


  inline void ARCA_Handler::handle_symbol_clear(size_t context,const ArcaL2SymbolClear_t& clr)
  {
    static const char* where = "ARCA_Handler::handle_symbol_clear";
    uint32_t security_index = SECURITY_INDEX(clr);
    
    ProductID id = m_security_index_to_product_id[security_index];
    if(id != -1) {
      Product prod = m_app->pm()->find_by_id(id);
      
      m_logger->log_printf(Log::INFO, "%s: %s Symbol Clear received for %s", where, m_name.c_str(), prod.symbol().c_str());
      
      if(m_order_book) {
        if (m_qbdf_builder) {
          gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, id, 'P', context);
        }
        
        m_order_book->clear(id, false);
      }
    }
  }

  inline void ARCA_Handler::handle_symbol_update(size_t context,const ArcaL2SymbolUpdate_t& update)
  {
    static const char* where = "ARCA_Handler::handle_symbol_update";
    uint32_t security_index = SECURITY_INDEX(update);
    char symbol[17];
    strncpy(symbol, update.sSymbol, 16);
    symbol[16] = '\0';

    m_logger->log_printf(Log::INFO, "%s received symbol_index_mapping for symbol %s to %d", where, symbol, security_index);

    symbol_to_product_id_t::const_iterator s_i = m_symbol_to_product_id.find(symbol);
    if(s_i != m_symbol_to_product_id.end()) {
      m_security_index_to_product_id[security_index] = s_i->second;
      if(m_context_to_product_ids.size() <= context) {
        m_context_to_product_ids.resize(context+1);
      }
      m_context_to_product_ids[context].push_back(s_i->second);
    }
  }

  inline void ARCA_Handler::handle_add_order(const ArcaL2Add_t& add)
  {
    m_micros_exch_time = ntohl(add.iSourceTime) * Time_Constants::micros_per_mili;
    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
    uint32_t security_index = SECURITY_INDEX(add);
    
    ProductID id = m_security_index_to_product_id[security_index];
    if(id != -1) {
      Time t = m_midnight + usec(m_micros_exch_time);
      uint64_t  order_id = ORDER_ID(add);
      if(add.cSide != 'B' && add.cSide != 'S') {
        m_logger->log_printf(Log::ERROR, "cSide unknown '%c' - %d", add.cSide, add.cSide);
      }
      MDPrice price = get_int_price(add.iPrice, add.iPriceScale);
      if(!price.blank()) {
        uint32_t size = ntohl(add.iVolume);

        if(m_order_book) {
          if(m_qbdf_builder && !m_exclude_level_2) {
            gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                      m_micros_exch_time_delta, 'P', add.cSide, size, price);
          }

          bool inside_changed = m_order_book->add_order_ip(t, order_id, id, add.cSide, price, size);

          if(inside_changed) {
            m_quote.set_time(t);
            m_quote.m_id = id;
            send_quote_update();
          }
        }
      }
    }
  }

  inline void ARCA_Handler::handle_modify_order(const ArcaL2Modify_t& modify)
  {
    m_micros_exch_time = ntohl(modify.iSourceTime) * Time_Constants::micros_per_mili;
    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
    uint32_t security_index = SECURITY_INDEX(modify);
    ProductID id = m_security_index_to_product_id[security_index];
    if(id != -1) {
      Time t = m_midnight + usec(m_micros_exch_time);
      uint64_t  order_id = ORDER_ID(modify);
      char side = modify.cSide;
      MDPrice price = get_int_price(modify.iPrice, modify.iPriceScale);
      if(!price.blank()) {
        uint32_t size = ntohl(modify.iVolume);

        if(modify.cSide != 'B' && modify.cSide != 'S') {
          m_logger->log_printf(Log::ERROR, "modify cSide unknown '%c' - %d", modify.cSide, modify.cSide);
        }

        bool inside_changed = false;
        if(m_order_book) {

          MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
          MDOrder* order = m_order_book->unlocked_find_order(order_id);
          if(m_qbdf_builder && !m_exclude_level_2) {
            gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                         m_micros_exch_time_delta, 'P', side, size, price);
          }
          MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
          if(order) {
            if(price == order->price && side == order->side()) {
              m_order_book->unlocked_modify_order(t, order, size);
              inside_changed = order->is_top_level();
            } else {
              inside_changed = m_order_book->unlocked_modify_order_ip(t, order, side, price, size);
            }
          } else {
            inside_changed = m_order_book->unlocked_add_order_ip(t, order_id, id, side, price, size);
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

  inline void ARCA_Handler::handle_delete_order(const ArcaL2Delete_t& del)
  {
    m_micros_exch_time = ntohl(del.iSourceTime) * Time_Constants::micros_per_mili;
    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
    uint32_t security_index = SECURITY_INDEX(del);
    ProductID id = m_security_index_to_product_id[security_index];
    if(id != -1) {
      Time      t = m_midnight + usec(m_micros_exch_time);
      uint64_t  order_id = ORDER_ID(del);
      
      if(m_order_book) {
        if(m_qbdf_builder && !m_exclude_level_2) {
          gen_qbdf_order_modify_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                       m_micros_exch_time_delta, 'P', ' ', 0, MDPrice());
        }

        bool inside_changed = m_order_book->remove_order(t, order_id);
        if(inside_changed) {
          m_quote.set_time(t);
          m_quote.m_id = id;
          send_quote_update();
        }
      }
    }
  }

  inline void ARCA_Handler::handle_imbalance(const ArcaL2Imbalance_t& imb,
                                             const Time_Duration& send_time)
  {
    static const char* where = "ARCA_Handler::handle_imbalance";

    m_micros_exch_time = ntohl(imb.iSourceTime) * Time_Constants::micros_per_mili;
    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
    uint32_t security_index = SECURITY_INDEX(imb);
    ProductID id = m_security_index_to_product_id[security_index];
    if(id != -1) {
      m_auction.m_id = id;

      SET_SOURCE_TIME(m_auction, msec(ntohl(imb.iSourceTime)));

      //double psm = get_price_scale_mult(imb.iPriceScale);
      //double price = ntohl(imb.iPrice) * psm;
      MDPrice price = get_int_price(imb.iPrice, imb.iPriceScale);
      double fprice = price.fprice();

      int32_t total_imbalance = ntohl(imb.iTotalImbalance);

      m_auction.m_ref_price = fprice;
      m_auction.m_far_price = fprice;
      m_auction.m_near_price = fprice;
      m_auction.m_paired_size = ntohl(imb.iVolume);
      m_auction.m_imbalance_size = abs(total_imbalance);
      if(total_imbalance == 0) {
        m_auction.m_imbalance_direction = 'N';
      } else if(total_imbalance > 0) {
        m_auction.m_imbalance_direction = 'B';
      } else {
        m_auction.m_imbalance_direction = 'S';
      }

      if (m_qbdf_builder && !m_exclude_imbalance) {
        gen_qbdf_imbalance_micros(m_qbdf_builder, m_auction.m_id, m_micros_recv_time,
                                  m_micros_exch_time_delta, 'P', price, price, price,
                                  m_auction.m_paired_size, m_auction.m_imbalance_size,
                                  m_auction.m_imbalance_direction, ' ');
      }
      if(m_subscribers) {
        m_subscribers->update_subscribers(m_auction);
      }
    }
  }

  inline void ARCA_Handler::handle_book_refresh(const ArcaL2FreshHeader_t& fresh_header,
                                                const ArcaL2BookOrder_t& add)
  {
    m_micros_exch_time = ntohl(add.iSourceTime) * Time_Constants::micros_per_mili;
    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
    uint32_t security_index = SECURITY_INDEX(fresh_header);
    ProductID id = m_security_index_to_product_id[security_index];
    if(false && id != -1) {
      Time t = m_midnight + usec(m_micros_exch_time);
      uint64_t  order_id = ORDER_ID(add);
      if(add.cSide != 'B' && add.cSide != 'S') {
        m_logger->log_printf(Log::ERROR, "cSide unknown '%c' - %d", add.cSide, add.cSide);
      }

      MDPrice price = get_int_price(add.iPrice, add.iPriceScale);
      if(!price.blank()) {
        uint32_t size = ntohl(add.iVolume);

        if(m_order_book) {
          if(m_qbdf_builder && !m_exclude_level_2) {
            gen_qbdf_order_add_micros(m_qbdf_builder, id, order_id, m_micros_recv_time,
                                      m_micros_exch_time_delta, 'P', add.cSide, size, price);
          }

          bool inside_changed = false;
          if(!m_order_book->unlocked_find_order(order_id)) {
            inside_changed = m_order_book->add_order_ip(t, order_id, id, add.cSide, price, size);
          }

          if(inside_changed) {
            m_quote.set_time(t);
            m_quote.m_id = id;
            //send_quote_update();
          }
        }
      }
    }
  }

  inline void ARCA_Handler::handle_trade(const trade_message& msg,
                                         const Time_Duration& send_time)
  {
    static const char* where = "ARCA_Handler::handle_trade";
    
    m_micros_exch_time = ntohl(msg.source_time) * Time_Constants::micros_per_mili;
    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
    ProductID id = product_id_of_symbol(msg.symbol);
    if(id != Product::invalid_product_id) {
      m_trade.m_id = id;
      SET_SOURCE_TIME(m_trade, msec(ntohl(msg.source_time)));

      MDPrice price = get_int_price(msg.price_numerator, msg.price_scale_code);
      m_trade.m_price = price.fprice();
      m_trade.m_size = ntohl(msg.volume); // * m_lot_size;
      
      memcpy(m_trade.m_trade_qualifier, msg.trade_condition, sizeof(m_trade.m_trade_qualifier));

      uint32_t buy_side_order_id = ntohl(((OrderIDStruct&)(msg.buy_side_link_id)).iID);
      uint32_t sell_side_order_id = ntohl(((OrderIDStruct&)(msg.sell_side_link_id)).iID);
      // if either id == 0, make it -1 instead so that it will compare than the other side
      // if the order_id is 0 that means that side was the aggressor, a market or routed order
      
      char side = '\0';
      m_trade.clear_flags();
      if(buy_side_order_id == sell_side_order_id) {
        m_trade.set_side('\0');
        m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
      } else {
        side = (buy_side_order_id-1 > sell_side_order_id-1) ? 'B' : 'S';
        m_trade.set_side(side);
        m_trade.set_exec_flag(TradeUpdate::exec_flag_exec);
      }
      
      if (m_qbdf_builder && !m_exclude_trades) {
        gen_qbdf_trade_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                              'P', side, '0', string(msg.trade_condition, 4), m_trade.m_size, price);
      } else {
        send_trade_update();
      }
      
      /*
        char timestamp[64];
	Time_Utils::print_time(timestamp, m_trade.time(), Timestamp::MICRO);
	uint32_t buy_side_system_id = ((OrderIDStruct&)(msg.buy_side_link_id)).iSystemID;
	uint32_t buy_side_market_id = ntohs(((OrderIDStruct&)(msg.buy_side_link_id)).iMarketID);
	uint32_t sell_side_system_id = ((OrderIDStruct&)(msg.sell_side_link_id)).iSystemID;
	uint32_t sell_side_market_id = ntohs(((OrderIDStruct&)(msg.sell_side_link_id)).iMarketID);
	m_logger->printf("%s %s %0.4f %d buysidelinkid %lu %u %u %u  sellsidelinkid %lu %u %u %u  quotelinkid %ld\n",
	timestamp, msg.symbol, m_trade.m_price, m_trade.m_size,
	ntohll(msg.buy_side_link_id), buy_side_system_id, buy_side_market_id, buy_side_order_id,
	ntohll(msg.sell_side_link_id), sell_side_system_id, sell_side_market_id, sell_side_order_id,
	ntohll(msg.quote_link_id));
      */
      
      if(stale_msg) {
        m_logger->log_printf(Log::INFO, "%s: stale Trade message %s %0.2f %4s", where,
                             msg.symbol, m_trade.m_price, m_trade.m_trade_qualifier);
      }
    }
  }

  size_t
  ARCA_Handler::parse2(size_t context, const char* buf, size_t len)
  {
    static const char* where = "ARCA_Handler::parse2";

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

    const char* msg_buf = buf + sizeof(message_header);

    if(msg_type == 32) { // Book refresh
      const ArcaL2FreshHeader_t& fresh_header = reinterpret_cast<const ArcaL2FreshHeader_t&>(*buf);
      uint32_t security_index = SECURITY_INDEX(fresh_header);
      ProductID id = m_security_index_to_product_id[security_index];
      if(id == -1) {
        // See if we can add it
        char symbol[17];
        strncpy(symbol, fresh_header.sSymbol, 16);
        symbol[16] = '\0';
        symbol_to_product_id_t::const_iterator s_i = m_symbol_to_product_id.find(symbol);
        if(s_i != m_symbol_to_product_id.end()) {
          m_security_index_to_product_id[security_index] = s_i->second;
          if(m_context_to_product_ids.size() <= context) {
            m_context_to_product_ids.resize(context+1);
          }
          // From subscription, interval group always +1 to real group
          m_context_to_product_ids[context-1].push_back(s_i->second);
          /*
            m_logger->log_printf(Log::DEBUG, "%s received refresh mapping for symbol %s to %d + %d = %d",
                                 where, symbol, fresh_header.iSessionID,
                                 ntohs(fresh_header.iSec), security_index);
          */
        } else if (m_qbdf_builder) {
          ProductID new_pid = m_qbdf_builder->process_taq_symbol(symbol);
          if (new_pid == 0) {
            m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol [%s]",
                                 where, symbol);
          } else {
            subscribe_product(new_pid, m_exch, symbol, "");
            m_security_index_to_product_id[security_index] = new_pid;
            if(m_context_to_product_ids.size() <= context) {
              m_context_to_product_ids.resize(context+1);
            }
            // From subscription, interval group always +1 to real group
            m_context_to_product_ids[context-1].push_back(new_pid);
            /*
            m_logger->log_printf(Log::DEBUG, "%s received refresh mapping for symbol %s to %d + %d = %d",
                                 where, symbol, fresh_header.iSessionID,
                                 ntohs(fresh_header.iSec), security_index);
            */
          }
        }
      }
      msg_buf = buf + sizeof(ArcaL2FreshHeader_t);
    }

    if(m_compacted) {

      FAST_STATE fast_state[AB_MAX_FIELD];
      memcpy(&fast_state, fastStateInit, sizeof(fastStateInit));

      for(char i = 0; i < header.num_body_entries; ++i) {
        int32_t srcLen = len - (msg_buf - buf);
        uint16_t l2_msg_type = 0;

        ArcaL2MsgUnion l2_msg;
        memset(&l2_msg, 0, sizeof(ArcaL2MsgUnion));
        int ret = ABFastDecode(&l2_msg, (uint8_t*)msg_buf, &srcLen, &l2_msg_type, fast_state);
        if(ret != AB_OK) {
          channel_info& ci = m_channel_info_map[context];
          m_logger->log_printf(Log::ERROR, "%s: %s %s ABFastDecode returned %d i=%d, num_body_entries=%d len=%zd msg_type=%d",
                               where, m_name.c_str(), ci.name.c_str(), ret, i,
                               header.num_body_entries, len, l2_msg_type);
          return len;
        }
        msg_buf += srcLen;

        switch(l2_msg_type) {
        case 36: // clear symbol
          {
            const ArcaL2SymbolClear_t& clr = l2_msg.SymbolClear;
            handle_symbol_clear(context,clr);
          }
          break;
        case 32: // Book refresh
          {
            const ArcaL2FreshHeader_t& fresh_header = reinterpret_cast<const ArcaL2FreshHeader_t&>(*buf);
            const ArcaL2BookOrder_t& add = l2_msg.BookRefreshOrder;
            handle_book_refresh(fresh_header,add);
            break;
          }
        case 35:  // symbol_index_mapping
          {
            handle_symbol_update(context,l2_msg.SymbolUpdate);
            break;
          }
        case 100: // add order
          {
            const ArcaL2Add_t& add = l2_msg.Add;
            handle_add_order(add);
            break;
          }
        case 101: // modify order
          {
            const ArcaL2Modify_t& modify = l2_msg.Modify;
            handle_modify_order(modify);
            break;
          }
        case 102: // delete order
          {
            const ArcaL2Delete_t& del = l2_msg.Delete;
            handle_delete_order(del);
            break;
          }
        case 103: // imbalance
          {
            const ArcaL2Imbalance_t& imb = l2_msg.Imbalance;
            handle_imbalance(imb,send_time);
            break;
          }
        default:
          m_logger->log_printf(Log::ERROR, "got unknown msg type %d", l2_msg_type);
          break;
        }
      }
      if(msg_buf != buf + len) {
        m_logger->log_printf(Log::ERROR, "%s: decoded = %zd   len = %zd", where, msg_buf-buf, len);
      }
    } else {  // non m_compacted

      /** Handle non Level2 our usual way */
      switch(msg_type) {

      case 220: // Trades
        {
          SIZE_CHECK(trade_message);

          for(char i = 0; i < header.num_body_entries; ++i) {
            const trade_message& msg = reinterpret_cast<const trade_message&>(*(msg_buf + (i*sizeof(trade_message))));
            handle_trade(msg, send_time);
          }
          break;
        }

      case 36: // clear symbol
        {
          SIZE_CHECK(ArcaL2SymbolClear_t);
          for(char i = 0; i < header.num_body_entries; ++i) {
            const ArcaL2SymbolClear_t& clr = reinterpret_cast<const ArcaL2SymbolClear_t&>(*(msg_buf + (i*sizeof(ArcaL2SymbolClear_t))));
            handle_symbol_clear(context,clr);
          }
          break;
        }
      case 35:
        {
          SIZE_CHECK(ArcaL2SymbolUpdate_t);
          for(char i = 0; i < header.num_body_entries; ++i) {
            const ArcaL2SymbolUpdate_t& update = reinterpret_cast<const ArcaL2SymbolUpdate_t&>(*(msg_buf + (i*sizeof(ArcaL2SymbolUpdate_t))));
            handle_symbol_update(context,update);
          }
          break;
        }
      case 32:
        {
          const ArcaL2FreshHeader_t& fresh_header = reinterpret_cast<const ArcaL2FreshHeader_t&>(*buf);
          const ArcaL2BookOrder_t& add = reinterpret_cast<const ArcaL2BookOrder_t&>(*msg_buf);
          handle_book_refresh(fresh_header,add);
          break;
        }
      case 99: // generic l2 message: Order Add,Delete,Update and Imbalance Data
        {
          uint16_t msg_size=ntohs(header.msg_size);
          uint16_t body_size=msg_size-sizeof(message_header)+2;

          for(char i = 0; i < header.num_body_entries; ++i) {

            uint16_t l2_msg_type = ntohs(*((const uint16_t*)(msg_buf+2)));

            //m_logger->log_printf(Log::INFO, "ARCA_Handler found level message before type=%d,entries=%d,i=%d,msg_size=%d,body_size=%d,order_size=%d,update_size=%d,delete_size=%d,header_size=%d",
            //l2_msg_type,header.num_body_entries,i,msg_size,body_size,sizeof(ArcaL2Add_t),sizeof(ArcaL2Modify_t),sizeof(ArcaL2Delete_t),sizeof(message_header));

            switch (l2_msg_type) {

            case 100:  // add order
              {
                BODY_SIZE_CHECK(ArcaL2Add_t);
                const ArcaL2Add_t& add = reinterpret_cast<const ArcaL2Add_t&>(*msg_buf);
                msg_buf += sizeof(ArcaL2Add_t);
                body_size -= sizeof(ArcaL2Add_t);
                handle_add_order(add);
                break;
              }
            case 101: // modify order
              {
                BODY_SIZE_CHECK(ArcaL2Modify_t);
                const ArcaL2Modify_t& modify= reinterpret_cast<const ArcaL2Modify_t&>(*msg_buf);
                msg_buf += sizeof(ArcaL2Modify_t);
                body_size -= sizeof(ArcaL2Modify_t);
                handle_modify_order(modify);
                break;
              }
            case 102: // delete order
              {
                BODY_SIZE_CHECK(ArcaL2Delete_t);
                const ArcaL2Delete_t& del= reinterpret_cast<const ArcaL2Delete_t&>(*msg_buf);
                msg_buf += sizeof(ArcaL2Delete_t);
                body_size -= sizeof(ArcaL2Delete_t);
                handle_delete_order(del);
                break;
              }
            case 103: // imbalance
              {
                BODY_SIZE_CHECK(ArcaL2Imbalance_t);
                const ArcaL2Imbalance_t& imb= reinterpret_cast<const ArcaL2Imbalance_t&>(*msg_buf);
                msg_buf += sizeof(ArcaL2Imbalance_t);
                body_size -= sizeof(ArcaL2Imbalance_t);
                handle_imbalance(imb,send_time);
                break;
              }
            } // switch l2_msg_type
          } // for num_entries

          break;
        } // case 99
      default:
        break;
      }
    }

    return len;
  }

  size_t
  ARCA_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len)
  {
    const message_header& header = reinterpret_cast<const message_header&>(*buf);

    uint16_t msg_type = ntohs(header.msg_type);
    Time_Duration send_time = msec(ntohl(header.send_time));

    char time_buf[32];
    Time_Utils::print_time(time_buf, send_time, Timestamp::MILI);

    log.printf(" msg_type:%u send_time:%s seq_num:%u entries:%u product_id:%u retrans_flag:%u\n", msg_type, time_buf,
               ntohl(header.seq_num), header.num_body_entries, header.product_id, header.retrans_flag);

    const char* msg_buf = buf + sizeof(message_header);

    switch(msg_type) {
    case 220: // Trade
      {
        for(char i = 0; i < header.num_body_entries; ++i) {
          const trade_message& msg = reinterpret_cast<const trade_message&>(*(msg_buf + (i*sizeof(trade_message))));
          double psm = get_price_scale_mult(msg.price_scale_code);
          double price = ntohl(msg.price_numerator) * psm;
          uint32_t volume = ntohl(msg.volume);
          uint32_t buy_side_order_id = ntohl(((OrderIDStruct&)(msg.buy_side_link_id)).iID);
          uint32_t sell_side_order_id = ntohl(((OrderIDStruct&)(msg.sell_side_link_id)).iID);
          char side = (buy_side_order_id-1 > sell_side_order_id-1) ? 'B' : 'S';
          if(buy_side_order_id == sell_side_order_id) {
            side = 'U';
          }
          Time_Duration source_time = msec(ntohl(msg.source_time));
          Time_Utils::print_time(time_buf, source_time, Timestamp::MILI);
          uint32_t buy_side_system_id = ((OrderIDStruct&)(msg.buy_side_link_id)).iSystemID;
          uint32_t buy_side_market_id = ntohs(((OrderIDStruct&)(msg.buy_side_link_id)).iMarketID);
          uint32_t sell_side_system_id = ((OrderIDStruct&)(msg.sell_side_link_id)).iSystemID;
          uint32_t sell_side_market_id = ntohs(((OrderIDStruct&)(msg.sell_side_link_id)).iMarketID);

          log.printf("  trade source_time:%s symbol:%.16s price:%0.4f volume:%u side:%c bs_order_id:%u ss_order_id:%u bs_sys_id:%u ss_sys_id:%u bs_mkt_id:%u ss_mkt_id:%u price_num:%u source_seq_num:%lu source_sess_id:%u price_scale_code:%u exchange_id:%c security_type:%c trade_condition:%.4s quote_link_id:%lu\n",
                     time_buf, msg.symbol, price, volume, side, buy_side_order_id, sell_side_order_id, buy_side_system_id, sell_side_system_id, buy_side_market_id, sell_side_market_id,
                     ntohl(msg.price_numerator), ntohll(msg.source_seq_num), msg.source_session_id, msg.price_scale_code, msg.exchange_id, msg.security_type, msg.trade_condition, ntohll(msg.quote_link_id));
        }
      }
      break;
    case 32: // Book refresh
      {
        const ArcaL2FreshHeader_t& fresh_header = reinterpret_cast<const ArcaL2FreshHeader_t&>(*buf);
        msg_buf = buf + sizeof(ArcaL2FreshHeader_t);
        uint32_t security_index = SECURITY_INDEX(fresh_header);

        log.printf(" refresh %.16s security_index:%u session:%u iSec:%u cur_pkt:%u tot_pkt:%u last_sym_seq:%u last_pkt_seq:%u\n", fresh_header.sSymbol, security_index,
                   fresh_header.iSessionID, ntohs(fresh_header.iSec),
                   ntohs(fresh_header.iCurrentPktNum), ntohs(fresh_header.iTotalPktNum), ntohl(fresh_header.iLastSymbolSeqNum), ntohl(fresh_header.iLastPktSeqNum));

        for(int i = 0; i < header.num_body_entries; ++i) {
          const ArcaL2BookOrder_t& add = reinterpret_cast<const ArcaL2BookOrder_t&>(*msg_buf);

          uint64_t  order_id = ORDER_ID(add);
          Time_Utils::print_time(time_buf, msec(ntohl(add.iSourceTime)), Timestamp::MILI);

          double psm = get_price_scale_mult(add.iPriceScale);
          double price = ntohl(add.iPrice) * psm;

          log.printf("  add sym_seq:%u source_time:%s order_id:%lu volume:%u price:%0.4f price_num:%u price_scale:%u side:%c exchange:%c security_type:%c firm:%u\n",
                     ntohl(add.iSymbolSequence), time_buf, order_id, ntohl(add.iVolume), price, ntohl(add.iPrice), add.iPriceScale, add.cSide, add.cExchangeID, add.cSecurityType,
                     ntohs(add.iFirm));

          msg_buf += sizeof(ArcaL2BookOrder_t);
        }
      }
      break;
    case 35:
      {
        for(char i = 0; i < header.num_body_entries; ++i) {
          const ArcaL2SymbolUpdate_t& update = reinterpret_cast<const ArcaL2SymbolUpdate_t&>(*(msg_buf + (i*sizeof(ArcaL2SymbolUpdate_t))));
          uint32_t security_index = SECURITY_INDEX(update);
          log.printf("  symbol_update security_index:%u raw_security_index:%u session_id:%u sys_code:%c symbol:%.16s\n", security_index, ntohs(update.iSec), update.iSessionID, update.cSysCode, update.sSymbol);
        }
      }
      break;
    case 36: // clear symbol
      {
        for(char i = 0; i < header.num_body_entries; ++i) {
          const ArcaL2SymbolClear_t& clr = reinterpret_cast<const ArcaL2SymbolClear_t&>(*(msg_buf + (i*sizeof(ArcaL2SymbolClear_t))));
          uint32_t security_index = SECURITY_INDEX(clr);
          log.printf("  clear_symbol security_index:%u raw_security_index:%u session_id:%u\n", security_index, ntohs(clr.iSec), clr.iSessionID);
        }
      }
      break;
    case 99: // generic l2 message:
      {
        for(char i = 0; i < header.num_body_entries; ++i) {
          uint16_t l2_msg_type = ntohs(*((const uint16_t*)(msg_buf+2)));
          switch (l2_msg_type) {
          case 100: // add order
            {
              const ArcaL2Add_t& add = reinterpret_cast<const ArcaL2Add_t&>(*msg_buf);
              msg_buf += sizeof(ArcaL2Add_t);
              uint32_t security_index = SECURITY_INDEX(add);
              Time_Utils::print_time(time_buf, msec(ntohl(add.iSourceTime)), Timestamp::MILI);
              uint64_t order_id = ORDER_ID(add);
              double psm = get_price_scale_mult(add.iPriceScale);
              double price = ntohl(add.iPrice) * psm;
              
              log.printf("  add security_index:%u sym_seq:%u source_time:%s order_id:%lu volume:%u price:%0.4f price_num:%u price_scale:%u side:%c exchange:%c security_type:%c security_index_raw:%u session_id:%u firm:%u\n",
                         security_index, ntohl(add.iSymbolSequence), time_buf, order_id, ntohl(add.iVolume), price, ntohl(add.iPrice), add.iPriceScale, add.cSide, add.cExchangeID, add.cSecurityType,
                         ntohs(add.iSec), add.iSessionID, ntohs(add.iFirm));
            }
            break;
          case 101: // modify
            {
              const ArcaL2Modify_t& modify= reinterpret_cast<const ArcaL2Modify_t&>(*msg_buf);
              msg_buf += sizeof(ArcaL2Modify_t);
              uint32_t security_index = SECURITY_INDEX(modify);
              Time_Utils::print_time(time_buf, msec(ntohl(modify.iSourceTime)), Timestamp::MILI);
              uint64_t order_id = ORDER_ID(modify);
              double psm = get_price_scale_mult(modify.iPriceScale);
              double price = ntohl(modify.iPrice) * psm;

              log.printf("  modify security_index:%u sym_seq:%u source_time:%s order_id:%lu volume:%u price:%0.4f price_num:%u price_scale:%u side:%c exchange:%c security_type:%c security_index_raw:%u session_id:%u firm:%u reason_code:%d\n",
                         security_index, ntohl(modify.iSymbolSequence), time_buf, order_id, ntohl(modify.iVolume), price, ntohl(modify.iPrice), modify.iPriceScale, modify.cSide, modify.cExchangeID, modify.cSecurityType,
                         ntohs(modify.iSec), modify.iSessionID, ntohs(modify.iFirm), modify.iReasonCode);
              
            }
            break;
          case 102: // delete order
            {
              const ArcaL2Delete_t& del= reinterpret_cast<const ArcaL2Delete_t&>(*msg_buf);
              msg_buf += sizeof(ArcaL2Delete_t);

              uint32_t security_index = SECURITY_INDEX(del);
              Time_Utils::print_time(time_buf, msec(ntohl(del.iSourceTime)), Timestamp::MILI);
              uint64_t order_id = ORDER_ID(del);

              log.printf("  delete security_index:%u sym_seq:%u source_time:%s order_id:%lu side:%c exchange:%c security_type:%c security_index_raw:%u session_id:%u firm:%u reason_code:%d\n",
                         security_index, ntohl(del.iSymbolSequence), time_buf, order_id, del.cSide, del.cExchangeID, del.cSecurityType,
                         ntohs(del.iSec), del.iSessionID, ntohs(del.iFirm), del.iReasonCode);
            }
            break;
          case 103: // imbalance
            {
              const ArcaL2Imbalance_t& imb= reinterpret_cast<const ArcaL2Imbalance_t&>(*msg_buf);
              msg_buf += sizeof(ArcaL2Imbalance_t);

              uint32_t security_index = SECURITY_INDEX(imb);
              Time_Utils::print_time(time_buf, msec(ntohl(imb.iSourceTime)), Timestamp::MILI);
              double psm = get_price_scale_mult(imb.iPriceScale);
              double price = ntohl(imb.iPrice) * psm;

              log.printf("  imb security_index:%u sym_seq:%u source_time:%s volume:%u total_imbalance:%u market_imbalance:%u price:%0.4f price_num:%u price_scale:%u auction_type:%c exchange:%c security_type:%c security_index_raw:%u session_id:%u auction_time:%u\n",
                         security_index, ntohl(imb.iSymbolSequence), time_buf, ntohl(imb.iVolume), ntohl(imb.iTotalImbalance), ntohl(imb.iMarketImbalance), price, ntohl(imb.iPrice), imb.iPriceScale, imb.cAuctionType, imb.cExchangeID, imb.cSecurityType,
                         ntohs(imb.iSec), imb.iSessionID, ntohs(imb.iAuctionTime));

            }
            break;
          }
        }
      }
      break;
    }

    return packet_len;
  }

  void
  ARCA_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    NYSE_Handler::init(name, exch, params);

    m_compacted = params.getDefaultBool("compacted", false);

    if(params.has("arca_book") || params.getDefaultBool("build_book", false)) {
      m_exch = exch;
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_book->init(params);
      int prealloc_orders = params.getDefaultInt("prealloc_orders", 50000);
      m_order_book->prealloc_order_pool(prealloc_orders);
      m_nyse_product_id = 115;
    } else if(params.has("arca_trades")) {
      m_nyse_product_id = 113;
    }

  }

  void
  ARCA_Handler::add_context_product_id(size_t context, ProductID id)
  {
    if(m_context_to_product_ids.size() <= context) {
      m_context_to_product_ids.resize(context+1);
    }
    // may as well resize here, but we'll set the real values when we get the data
  }

  void
  ARCA_Handler::subscribe_product(ProductID id, ExchangeID exch,
                                 const string& mdSymbol, const string& mdExch)
  {
    mutex_t::scoped_lock lock(m_mutex);
    char ssymbol[32];
    if(mdExch.empty()) {
      snprintf(ssymbol, 32, "%s", mdSymbol.c_str());
    } else {
      snprintf(ssymbol, 32, "%s.%s", mdSymbol.c_str(), mdExch.c_str());
    }

    pair<set<string>::iterator, bool> s_i = m_symbols.insert(ssymbol);
    const char* symbol = s_i.first->c_str();
    m_symbol_to_product_id[symbol] = id;
  }

  void
  ARCA_Handler::send_quote_update()
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
  ARCA_Handler::send_trade_update()
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

  ARCA_Handler::ARCA_Handler(Application* app) :
    NYSE_Handler(app)
  {
    // Max session_id we seem to find is 20,  max security index tends to be in the hundreds
    m_security_index_to_product_id.resize(32 * 8192, -1);
  }

  ARCA_Handler::~ARCA_Handler()
  {
  }


}
