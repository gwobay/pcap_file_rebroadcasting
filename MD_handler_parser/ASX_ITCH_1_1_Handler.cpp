#include <Data/QBDF_Util.h>
#include <Data/Handler.h>
#include <Data/ASX_ITCH_1_1_Handler.h>
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
#include <thread>

#define LOGV_MARCO(X) if(false) { X  };

namespace hf_core {

  static FactoryRegistrar1<std::string, Handler, Application*, ASX_ITCH_1_1_Handler> r1("ASX_ITCH_1_1_Handler");

  enum ASX_MESSAGE_TYPE {
    TIME_MESSAGE='T',
    END_OF_BUSINESS_TRADE_DATE='S',
    EQUITY_SYMBOL_DIRECTORY='R',
    FUTURE_SYMBOL_DIRECTORY='f',
    OPTION_SYMBOL_DIRECTORY='h',
    COMBINATION_SYMBOL_DIRECTORY='M',
    BUNDLES_SYMBOL_DIRECTORY='m',
    TICK_SIZE_TABLE='L',
    ORDER_BOOK_STATE='O',
    ORDER_ADDED='A',
    ORDER_ADDED_WITH_PARTICIPANT_ID='F',
    ORDER_VOLUME_CANCELLED='X',
    ORDER_REPLACED='U',
    ORDER_DELETED='D',
    ORDER_EXECUTED='E',
    AUCTION_ORDER_EXECUTED='C',
    COMBINATION_EXECUTED='e',
    IMPLIED_ORDER_ADDED='j',
    IMPLIED_ORDER_REPLACED='l',
    IMPLIED_ORDER_DELETED='k',
    TRADE_EXECUTED='P',
    COMBINATION_TRADE_EXECUTED='p',
    TRADE_REPORT='Q',
    TRADE_CANCELLATION='B',
    EQUILIBRIUM_PRICE_AUCTION_INFO='Z',
    OPEN_HIGH_LOW_LAST_TRADE_ADJUSTMENT='t',
    MARKET_SETTLEMENT='Y',
    TEXT_MESSAGE='x',
    REQUEST_FOR_QUOTE='q',
    ANOMALOUS_ORDER_THRESHOLD_PUBLISH='W',
    VOLUME_AND_OPEN_INTEREST='V',
    SNAPSHOT_COMPLETE='G',
  };
  
  uint64_t
  ASX_ITCH_1_1_Handler::get_unique_order_id(uint64_t asx_order_id, uint32_t order_book_id, char side)
  {
    asx_order_id_hash_key oihk(asx_order_id, order_book_id, side);
    asx_order_id_hash_t::iterator i = m_order_id_hash.find(oihk);
    if (i == m_order_id_hash.end()) {
      uint64_t order_id = ++m_order_id;
      m_order_id_hash[oihk] = order_id;
      return order_id;
    } else {
      return i->second;
    }
  }
  
  ProductID
  ASX_ITCH_1_1_Handler::get_product_id(uint32_t order_book_id, const string& symbol)
  {
    if (m_qbdf_builder) {
      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol);
      if (symbol_id == 0) {
	m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for order_book_id [%u]",
			     "ASX_ITCH_1_1_Handler::product_id_of_order_book_id", order_book_id);
	m_logger->sync_flush();
	return Product::invalid_product_id;
      }
      return symbol_id;
    }
    
    map<uint32_t, symbol_info>::iterator i = m_symbol_info_map.find(order_book_id);
    if (i == m_symbol_info_map.end()) {
      return Product::invalid_product_id;
    } else {
      i->second.product_id = m_symbol_info_map.size();
      return i->second.product_id;
    }
    ProductID id = m_prod_map.find8(symbol.c_str());
    return id;
  }
  
  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)						\
  if(len != sizeof(type)) {						\
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), len, sizeof(type)); \
    return len;								\
  } 
  
  inline size_t ASX_ITCH_1_1_Handler::parseTIME_MESSAGE_T(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
  
    SIZE_CHECK(asx_itch_1_1::timestamp_message);
    const asx_itch_1_1::timestamp_message& msg = reinterpret_cast<const asx_itch_1_1::timestamp_message&>(*buf);

    uint32_t sec = ntohl(msg.seconds);
    m_timestamp = Time(Time_Constants::ticks_per_second * (uint64_t)sec);
        
    if (m_qbdf_builder) {
      if (m_timestamp.total_usec() <  Time::today().total_usec()) {
	// ASX Feed starts up the day before, mark all times as minutes past midnight to prevent previous day timestamps
	m_timestamp = Time::today() + Time_Duration(m_timestamp.ticks() % Time_Constants::ticks_per_hr);
      }

      m_micros_exch_time = m_timestamp.total_usec() - Time::today().total_usec();
      if (m_hist_mode) {
	m_micros_recv_time = m_micros_exch_time;
	m_micros_exch_time_delta = 0;
      } else {
	m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
      }
      
    }
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseEND_OF_BUSINESS_TRADE_DATE_S(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
    //SIZE_CHECK(asx_itch_1_1::system_event_message);
    const asx_itch_1_1::system_event_message& msg = reinterpret_cast<const asx_itch_1_1::system_event_message&>(*buf);
    
    //m_logger->log_printf(Log::INFO, "%s: system_event event_code=%c", where, msg.event_code);
    
    if(msg.event_code == 'O') {
      send_alert("%s %s received system event message 'O' -- Start of Messages", where, m_name.c_str());
    } else if(msg.event_code == 'C') {
      send_alert("%s %s received system event message 'C' -- End of Messages", where, m_name.c_str());
      channel_info* ci = get_channel_info(context);
      if (ci->context == context){
	      ci->final_message_received=true;
      }
    }
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseEQUITY_SYMBOL_DIRECTORY_R(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
    
    SIZE_CHECK(asx_itch_1_1::order_book_directory);
    const asx_itch_1_1::order_book_directory& msg = reinterpret_cast<const asx_itch_1_1::order_book_directory&>(*buf);
	
    uint32_t order_book_id = ntohl(msg.order_book_id);
    
    //m_logger->log_printf(Log::INFO, "%s: msg_type=%c order_book_id=%u symbol=%s",
    //where, msg.message_type, order_book_id, msg.symbol);
    
    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id == Product::invalid_product_id) {
      if (msg.financial_product == 5) { // (1=Option/Warrant; 3=Future; 5=Cash; 11=Standard combination)
	string symbol = string(msg.symbol, sizeof(msg.symbol));
	size_t found = symbol.find_last_not_of(" \t\f\v\n\r");
	if (found != string::npos)
	  symbol.erase(found+1);
	else
	  symbol.clear();
	
	si->product_id = get_product_id(order_book_id, symbol);
	memcpy(si->symbol, &msg.symbol, sizeof(si->symbol));
	memcpy(si->isin, &msg.isin, sizeof(si->isin));
	memcpy(si->currency, &msg.currency, sizeof(si->currency));
	si->num_decimals_in_price = ntohs(msg.num_decimals_in_price);
	si->num_decimals_in_nominal = ntohs(msg.num_decimals_in_nominal);
	si->odd_lot_size = ntohl(msg.odd_lot_size);
	si->round_lot_size = ntohl(msg.round_lot_size);
	si->block_lot_size = ntohl(msg.block_lot_size);
	si->nominal_value = ntohll(msg.nominal_value);
	si->price_multiplier = pow(10, 2-si->num_decimals_in_price);
      }
    }
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseFUTURE_SYMBOL_DIRECTORY_f(size_t context, const char* buf, size_t len){
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseOPTION_SYMBOL_DIRECTORY_h(size_t context, const char* buf, size_t len){
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseCOMBINATION_SYMBOL_DIRECTORY_M(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
    
    SIZE_CHECK(asx_itch_1_1::combination_order_book_directory);
    
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseBUNDLES_SYMBOL_DIRECTORY_m(size_t context, const char* buf, size_t len){
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseTICK_SIZE_TABLE_L(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;

    SIZE_CHECK(asx_itch_1_1::tick_size_message);
    const asx_itch_1_1::tick_size_message& msg = reinterpret_cast<const asx_itch_1_1::tick_size_message&>(*buf);
    
    uint32_t order_book_id = ntohl(msg.order_book_id);
    
    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id != Product::invalid_product_id) {
      //uint32_t ns = ntohl(msg.nanoseconds);
      //Time t = m_timestamp + Time_Duration(ns);
      
      //uint64_t tick_size = ntohll(msg.tick_size);
      //uint32_t price_from = ntohl(msg.price_from);
      //uint32_t price_to = ntohl(msg.price_to);
      //m_logger->log_printf(Log::INFO, "%s: tick_size id=%d %d-%d=%lu m_timestamp=%s ns=%u",
      //where, si->product_id, price_from, price_to, tick_size, m_timestamp.to_string().c_str(), ns);
    }
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseORDER_BOOK_STATE_O(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
    
    SIZE_CHECK(asx_itch_1_1::order_book_state_message);
    const asx_itch_1_1::order_book_state_message& msg = reinterpret_cast<const asx_itch_1_1::order_book_state_message&>(*buf);
	
    uint32_t order_book_id = ntohl(msg.order_book_id);
    symbol_info* si = get_symbol_info(order_book_id);
    
    if(si->product_id != Product::invalid_product_id) {
      uint32_t ns = ntohl(msg.nanoseconds);
      Time t = m_timestamp + Time_Duration(ns);

      m_msu.set_time(t);
      m_msu.m_id = si->product_id;
	  
      if (m_qbdf_builder and !m_exclude_status) {
	if (t.total_usec() <  Time::today().total_usec()) {
	  // ASX Feed starts up the day before, mark all times as midnight to prevent previous day timestamps
	  t = Time::today();
	}

	m_micros_exch_time = t.total_usec() - Time::today().total_usec();
	if (m_hist_mode) {
	  m_micros_recv_time = m_micros_exch_time;
	  m_micros_exch_time_delta = 0;
	} else {
	  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	}
	
	//            m_logger->log_printf(Log::INFO, "%s: message=%c t=%s m_micros_recv_time=%lu",
	//				 where, buf[0], t.to_string().c_str(), m_micros_recv_time);
	m_qbdf_status_reason = string("ASX Trading State ") + msg.state_name;
	
	gen_qbdf_status_micros(m_qbdf_builder, si->product_id, m_micros_recv_time,
			       m_micros_exch_time_delta, m_exch_char,
			       m_qbdf_status.index(), m_qbdf_status_reason);
      }
      
      
      //m_msu.m_reason.assign(msg.reason, 4);
      //m_security_status[id] = m_msu.m_market_status;
      //if(m_subscribers) {
      //  m_subscribers->update_subscribers(m_msu);
      //}
      //m_msu.m_reason.clear();
    }
    return len;
  }
  
  inline size_t ASX_ITCH_1_1_Handler::parseORDER_ADDED_A(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
    
    SIZE_CHECK(asx_itch_1_1::add_order_no_mpid);
    const asx_itch_1_1::add_order_no_mpid& msg = reinterpret_cast<const asx_itch_1_1::add_order_no_mpid&>(*buf);
    
    uint32_t order_book_id = ntohl(msg.order_book_id);
    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id != Product::invalid_product_id) {
      uint32_t ns = ntohl(msg.nanoseconds);
      Time t = m_timestamp + Time_Duration(ns);
      
      uint64_t asx_order_id = ntohll(msg.order_id);
      uint64_t shares = ntohll(msg.quantity);
      MDPrice price = MDPrice::from_iprice32(ntohl(msg.price) * si->price_multiplier);
      char side = msg.side;
      
      if(m_order_book) {
	uint64_t order_id = get_unique_order_id(asx_order_id, order_book_id, side);
	
	if(m_qbdf_builder && !m_exclude_level_2) {
	  if (t.total_usec() <  Time::today().total_usec()) {
	    // ASX Feed starts up the day before, mark all times as midnight to prevent previous day timestamps
	    t = Time::today();
	  }
	  
	  m_micros_exch_time = t.total_usec() - Time::today().total_usec();
	  if (m_hist_mode) {
	    m_micros_recv_time = m_micros_exch_time;
	    m_micros_exch_time_delta = 0;
	  } else {
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	  }
	  
	  gen_qbdf_order_add_micros(m_qbdf_builder, si->product_id, order_id, m_micros_recv_time,
				    m_micros_exch_time_delta, m_exch_char, side, shares, price);
	}

	bool inside_changed = m_order_book->add_order_ip(t, order_id, si->product_id, side, price, shares);

	if(inside_changed) {
	  m_quote.set_time(t);
	  m_quote.m_id = si->product_id;
	  if (!m_qbdf_builder) {
	    send_quote_update();
	  }
	}
      }
    }
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseORDER_ADDED_WITH_PARTICIPANT_ID_F(size_t context, const char* buf, size_t len)
  {
    
    const char* where=__func__;

    SIZE_CHECK(asx_itch_1_1::add_order_mpid);
    const asx_itch_1_1::add_order_mpid& msg = reinterpret_cast<const asx_itch_1_1::add_order_mpid&>(*buf);
    
    uint32_t order_book_id = ntohl(msg.order_book_id);
    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id != Product::invalid_product_id) {
      uint32_t ns = ntohl(msg.nanoseconds);
      Time t = m_timestamp + Time_Duration(ns);
      
      uint64_t asx_order_id = ntohll(msg.order_id);
      uint64_t shares = ntohll(msg.quantity);
      MDPrice price = MDPrice::from_iprice32(ntohl(msg.price) * si->price_multiplier);
      char side = msg.side;
	  
      if(m_order_book) {
	uint64_t order_id = get_unique_order_id(asx_order_id, order_book_id, side);
	
	if(m_qbdf_builder && !m_exclude_level_2) {
	  if (t.total_usec() <  Time::today().total_usec()) {
	    // ASX Feed starts up the day before, mark all times as midnight to prevent previous day timestamps
	    t = Time::today();
	  }

	  m_micros_exch_time = t.total_usec() - Time::today().total_usec();
	  if (m_hist_mode) {
	    m_micros_recv_time = m_micros_exch_time;
	    m_micros_exch_time_delta = 0;
	  } else {
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	  }
	  
	  gen_qbdf_order_add_micros(m_qbdf_builder, si->product_id, order_id, m_micros_recv_time,
				    m_micros_exch_time_delta, m_exch_char, side, shares, price);
	}

	bool inside_changed = m_order_book->add_order_ip(t, order_id, si->product_id, msg.side, price, shares);
	
	if(inside_changed) {
	  m_quote.set_time(t);
	  m_quote.m_id = si->product_id;
	  if (!m_qbdf_builder) {
	    send_quote_update();
	  }
	}
      }
    }
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseORDER_VOLUME_CANCELLED_X(size_t context, const char* buf, size_t len){
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseORDER_DELETED_D(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
    
    SIZE_CHECK(asx_itch_1_1::order_delete_message);
    const asx_itch_1_1::order_delete_message& msg = reinterpret_cast<const asx_itch_1_1::order_delete_message&>(*buf);
    
    uint32_t order_book_id = ntohl(msg.order_book_id);

    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id != Product::invalid_product_id) {
      uint32_t ns = ntohl(msg.nanoseconds);
      Time t = m_timestamp + Time_Duration(ns);

      uint64_t asx_order_id = ntohll(msg.order_id);
      char side = msg.side;
	  
      if(m_order_book) {
	uint64_t order_id = get_unique_order_id(asx_order_id, order_book_id, side);
	
	bool inside_changed = false;
	{
	  MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
	  
	  MDOrder* order = m_order_book->unlocked_find_order(order_id);
	  if(order) {
	    if(m_qbdf_builder && !m_exclude_level_2) {
	      if (t.total_usec() <  Time::today().total_usec()) {
		// ASX Feed starts up the day before, mark all times as midnight to prevent previous day timestamps
		t = Time::today();
	      }

	      m_micros_exch_time = t.total_usec() - Time::today().total_usec();
	      if (m_hist_mode) {
		m_micros_recv_time = m_micros_exch_time;
		m_micros_exch_time_delta = 0;
	      } else {
		m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	      }

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
	  if (!m_qbdf_builder) {
	    send_quote_update();
	  }
	}
	if(m_order_book->orderbook_update()) {
	  m_order_book->post_orderbook_update();
	}
      }
    }
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseORDER_REPLACED_U(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;

    SIZE_CHECK(asx_itch_1_1::order_replace_message);
    const asx_itch_1_1::order_replace_message& msg = reinterpret_cast<const asx_itch_1_1::order_replace_message&>(*buf);
    
    uint32_t order_book_id = ntohl(msg.order_book_id);
    
    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id != Product::invalid_product_id) {
      uint32_t ns = ntohl(msg.nanoseconds);
      Time t = m_timestamp + Time_Duration(ns);
      
      uint64_t asx_order_id = ntohll(msg.order_id);
      uint64_t shares = ntohll(msg.quantity);
      MDPrice price = MDPrice::from_iprice32(ntohl(msg.price) * si->price_multiplier);
      char side = msg.side;
      
      if(m_order_book) {
	uint64_t order_id = get_unique_order_id(asx_order_id, order_book_id, side);
	
	ProductID id = Product::invalid_product_id;
	bool inside_changed = false;
	{
	  MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);

	  MDOrder* orig_order = m_order_book->unlocked_find_order(order_id);
	  if(orig_order) {
	    if(m_qbdf_builder && !m_exclude_level_2) {
	      if (t.total_usec() <  Time::today().total_usec()) {
		// ASX Feed starts up the day before, mark all times as midnight to prevent previous day timestamps
		t = Time::today();
	      }
	      
	      m_micros_exch_time = t.total_usec() - Time::today().total_usec();
	      if (m_hist_mode) {
		m_micros_recv_time = m_micros_exch_time;
		m_micros_exch_time_delta = 0;
	      } else {
		m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	      }

	      gen_qbdf_order_replace_micros(m_qbdf_builder, orig_order->product_id, order_id,
					    m_micros_recv_time, m_micros_exch_time_delta,
					    m_exch_char, orig_order->side(), order_id,
					    shares, price);
	    }
	    
	    id = orig_order->product_id;
	    MDOrderBook::mutex_t::scoped_lock book_lock(m_order_book->book(id).m_mutex, true);
	    inside_changed = m_order_book->unlocked_replace_order_ip(t, orig_order, orig_order->side(),
								     price, shares, order_id, false);
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
    return len;
  }


  inline size_t ASX_ITCH_1_1_Handler::parseORDER_EXECUTED_E(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
    
    SIZE_CHECK(asx_itch_1_1::order_executed);
    const asx_itch_1_1::order_executed& msg = reinterpret_cast<const asx_itch_1_1::order_executed&>(*buf);

    uint32_t order_book_id = ntohl(msg.order_book_id);
	
    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id != Product::invalid_product_id) {
      uint32_t ns = ntohl(msg.nanoseconds);
      Time t = m_timestamp + Time_Duration(ns);

      uint64_t asx_order_id = ntohll(msg.order_id);
      uint64_t shares = ntohll(msg.executed_quantity);
      char side = msg.side;
      
      if(m_order_book) {
	uint64_t order_id = get_unique_order_id(asx_order_id, order_book_id, side);
	
	bool order_found = false;
	bool inside_changed = false;
	{
	  MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
          
	  MDOrder* order = m_order_book->unlocked_find_order(order_id);
	  if(order) {
	    if(m_qbdf_builder && !m_exclude_level_2) {
	      if (t.total_usec() <  Time::today().total_usec()) {
		// ASX Feed starts up the day before, mark all times as midnight to prevent previous day timestamps
		t = Time::today();
	      }

	      m_micros_exch_time = t.total_usec() - Time::today().total_usec();
	      if (m_hist_mode) {
		m_micros_recv_time = m_micros_exch_time;
		m_micros_exch_time_delta = 0;
	      } else {
		m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	      }
              
	      bool skip_order = false;
	      if (t < m_new_auction_date) {
		asx_match_id mi = asx_match_id(ntohll(msg.match_id_part_1), ntohl(msg.match_id_part_2));
		set<asx_match_id>& match_ids = m_match_id_lookup[si->product_id];
		if (match_ids.find(mi) == match_ids.end()) {
		  match_ids.insert(mi);
		} else {
		  gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
					       m_micros_recv_time, m_micros_exch_time_delta,
					       m_exch_char, order->side(), order->size - shares, order->price);
		  skip_order = true;
		}
	      } 
	      
	      if (! skip_order) {
		gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
					   m_micros_recv_time, m_micros_exch_time_delta, m_exch_char,
					   order->flip_side(), ' ', shares, order->price);
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
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseAUCTION_ORDER_EXECUTED_C(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
    
    SIZE_CHECK(asx_itch_1_1::order_executed_with_price);
    const asx_itch_1_1::order_executed_with_price& msg = reinterpret_cast<const asx_itch_1_1::order_executed_with_price&>(*buf);
    
    uint32_t order_book_id = ntohl(msg.order_book_id);
    
    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id != Product::invalid_product_id) {
      uint32_t ns = ntohl(msg.nanoseconds);
      Time t = m_timestamp + Time_Duration(ns);
      
      uint64_t asx_order_id = ntohll(msg.order_id);
      uint64_t shares = ntohll(msg.executed_quantity);
      MDPrice price = MDPrice::from_iprice32(ntohl(msg.execution_price) * si->price_multiplier);
      char side = msg.side;
      
      if(m_order_book) {
	uint64_t order_id = get_unique_order_id(asx_order_id, order_book_id, side);
	
	bool order_found = false;
	bool inside_changed = false;
	{
	  MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
	  
	  MDOrder* order = m_order_book->unlocked_find_order(order_id);
	  if(order) {
	    uint32_t new_size = order->size - shares;
	    if(m_qbdf_builder && !m_exclude_level_2) {
	      if (t.total_usec() <  Time::today().total_usec()) {
		// ASX Feed starts up the day before, mark all times as midnight to prevent previous day timestamps
		t = Time::today();
	      }
	      
	      m_micros_exch_time = t.total_usec() - Time::today().total_usec();
	      if (m_hist_mode) {
		m_micros_recv_time = m_micros_exch_time;
		m_micros_exch_time_delta = 0;
	      } else {
		m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	      }
	      
	      bool skip_order = false;
	      if (t < m_new_auction_date) {
		asx_match_id mi = asx_match_id(ntohll(msg.match_id_part_1), ntohl(msg.match_id_part_2));
		set<asx_match_id>& match_ids = m_match_id_lookup[si->product_id];
		if (match_ids.find(mi) == match_ids.end()) {
		  match_ids.insert(mi);
		} else {
		  gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
					       m_micros_recv_time, m_micros_exch_time_delta,
					       m_exch_char, order->side(), new_size, price);
		  skip_order = true;
		}
	      } 
	      
	      if (! skip_order) {
		if(msg.printable == 'Y') {
		  gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, order_id,
					     m_micros_recv_time, m_micros_exch_time_delta,
					     m_exch_char, order->flip_side(), msg.cross_trade,
					     shares, price);
		} else {
		  gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, order_id,
					       m_micros_recv_time, m_micros_exch_time_delta,
					       m_exch_char, order->side(), new_size, price);
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
    return len;
  }
  
  inline size_t ASX_ITCH_1_1_Handler::parseCOMBINATION_EXECUTED_e(size_t context, const char* buf, size_t len){
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseIMPLIED_ORDER_ADDED_j(size_t context, const char* buf, size_t len){
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseIMPLIED_ORDER_REPLACED_l(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseIMPLIED_ORDER_DELETED_k(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseTRADE_EXECUTED_P(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;
    
    SIZE_CHECK(asx_itch_1_1::trade_message);
    const asx_itch_1_1::trade_message& msg = reinterpret_cast<const asx_itch_1_1::trade_message&>(*buf);
    
    uint32_t order_book_id = ntohl(msg.order_book_id);
    
    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id != Product::invalid_product_id) {
      uint32_t ns = ntohl(msg.nanoseconds);
      Time t = m_timestamp + Time_Duration(ns);
      
      m_trade.m_id = si->product_id;
      m_trade.set_time(t);
      MDPrice price = MDPrice::from_iprice32(ntohl(msg.trade_price) * si->price_multiplier);
          
      m_trade.m_price = price.fprice();
      uint64_t qty = ntohll(msg.quantity); //quantity is uint64_t;
      m_trade.m_size = (int)qty; //m_size defined as int
      char side_c = msg.side == 'B' ? 'S' : 'B';
      m_trade.set_side(side_c);
      m_trade.clear_flags();
      m_trade.set_exec_flag(TradeUpdate::exec_flag_trade);
      
      if (m_qbdf_builder && !m_exclude_trades) {
	if (t.total_usec() <  Time::today().total_usec()) {
	  // ASX Feed starts up the day before, mark all times as midnight to prevent previous day timestamps
	  t = Time::today();
	}
	
	m_micros_exch_time = t.total_usec() - Time::today().total_usec();
	if (m_hist_mode) {
	  m_micros_recv_time = m_micros_exch_time;
	  m_micros_exch_time_delta = 0;
	} else {
	  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	}
	
	if(msg.printable == 'Y' || msg.cross_trade == 'Y') {
	  string cond;
	  if (msg.cross_trade == 'Y') {
	    cond = "Y    ";
	  } else {
	    cond = "     ";
	  }
	  gen_qbdf_trade_micros(m_qbdf_builder, si->product_id, m_micros_recv_time,
				m_micros_exch_time_delta, m_exch_char, side_c, '0', cond,
				ntohll(msg.quantity), price);
	}
      } else {
	send_trade_update();
      }
    }
    return len;
  }

  inline size_t ASX_ITCH_1_1_Handler::parseCOMBINATION_TRADE_EXECUTED_p(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseTRADE_REPORT_Q(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseTRADE_CANCELLATION_B(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseEQUILIBRIUM_PRICE_AUCTION_INFO_Z(size_t context, const char* buf, size_t len)
  {
    const char* where=__func__;

    SIZE_CHECK(asx_itch_1_1::equilibrium_price_message);
    const asx_itch_1_1::equilibrium_price_message& msg = reinterpret_cast<const asx_itch_1_1::equilibrium_price_message&>(*buf);

    uint32_t order_book_id = ntohl(msg.order_book_id);

    symbol_info* si = get_symbol_info(order_book_id);
    if(si->product_id != Product::invalid_product_id) {
      uint32_t ns = ntohl(msg.nanoseconds);
      Time t = m_timestamp + Time_Duration(ns);
      
      m_auction.set_time(t);
      m_auction.m_id = si->product_id;
      MDPrice price = MDPrice::from_iprice32(ntohl(msg.equilibrium_price) * si->price_multiplier);
      uint64_t bid_quantity = ntohll(msg.bid_quantity);
      uint64_t ask_quantity = ntohll(msg.ask_quantity);
      uint64_t paired_shares = min(bid_quantity, ask_quantity);
      uint64_t imbalance_size = 0;
      char imbalance_side = ' ';
      
      if (bid_quantity > ask_quantity) {
	imbalance_size = bid_quantity - ask_quantity;
	imbalance_side = 'B';
      } else if (bid_quantity < ask_quantity) {
	imbalance_side = 'S';
	imbalance_size = ask_quantity - bid_quantity;
      }
      
      m_auction.m_ref_price = price.fprice();
      m_auction.m_far_price = price.fprice();
      m_auction.m_near_price = price.fprice();
      m_auction.m_paired_size = paired_shares;
      m_auction.m_imbalance_size = imbalance_size;
      m_auction.m_imbalance_direction = imbalance_side;
      m_auction.m_indicative_price_flag = ' ';
	  
      if (m_qbdf_builder && !m_exclude_imbalance) {
	if (t.total_usec() <  Time::today().total_usec()) {
	  // ASX Feed starts up the day before, mark all times as midnight to prevent previous day timestamps
	  t = Time::today();
	}
	
	m_micros_exch_time = t.total_usec() - Time::today().total_usec();
	if (m_hist_mode) {
	  m_micros_recv_time = m_micros_exch_time;
	  m_micros_exch_time_delta = 0;
	} else {
	  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	}
	
	gen_qbdf_imbalance_micros(m_qbdf_builder, si->product_id, m_micros_recv_time,
				  m_micros_exch_time_delta, m_exch_char, price, price, price,
				  m_auction.m_paired_size, m_auction.m_imbalance_size,
				  imbalance_side, ' ');
	
      }

      if(m_subscribers) {
	m_subscribers->update_subscribers(m_auction);
      }
    }
    return len;
  }
  
  inline size_t ASX_ITCH_1_1_Handler::parseOPEN_HIGH_LOW_LAST_TRADE_ADJUSTMENT_t(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseMARKET_SETTLEMENT_Y(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseTEXT_MESSAGE_x(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseREQUEST_FOR_QUOTE_q(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseANOMALOUS_ORDER_THRESHOLD_PUBLISH_W(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseVOLUME_AND_OPEN_INTEREST_V(size_t context, const char* buf, size_t len){
    return len;
  }
  inline size_t ASX_ITCH_1_1_Handler::parseSNAPSHOT_COMPLETE_G(size_t context, const char* buf, size_t len){
    return len;
}

size_t ASX_ITCH_1_1_Handler::get_itch_packet_length(const char* buffer, size_t len, size_t& realCount)
{
    const asx_itch_1_1::downstream_header_t* header = reinterpret_cast<const asx_itch_1_1::downstream_header_t*>(buffer);
    
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    //uint64_t next_seq_num = seq_num + message_count;

    size_t parsed_len=sizeof(*header);
    const char* block=buffer+parsed_len;
    for( ; message_count > 0; --message_count) {
      uint16_t message_length = ntohs(reinterpret_cast<const asx_itch_1_1::message_block_t*>(block)->message_length);
      parsed_len += (2 + message_length);
      if (parsed_len > len){
	      parsed_len -= (2 + message_length);
      }
      block += (2 + message_length);
    }
    realCount -= message_count;
    return parsed_len;
}
  
  void
ASX_ITCH_1_1_Handler::print_message(const char* buf)
{
	bool show_msg=false;
	char prtBuf[360];
	prtBuf[0]='\n';
	prtBuf[1]=0x00;
    switch(buf[0]) {
    case TIME_MESSAGE://'T':
      {
        const asx_itch_1_1::timestamp_message& msg = reinterpret_cast<const asx_itch_1_1::timestamp_message&>(*buf);
	
        uint32_t sec = ntohl(msg.seconds);
        Time_Duration ts = seconds(sec);
	
        char time_buf[32];
        Time_Utils::print_time(time_buf, ts, Timestamp::MILI);
	
#ifdef CW_DEBUG
        printf("  timestamp %s\n", time_buf);
#endif
        sprintf(prtBuf,"  timestamp %s\n", time_buf);
      }
      break;
    case EQUITY_SYMBOL_DIRECTORY://'R':
      {
	      show_msg=true;
        const asx_itch_1_1::order_book_directory& msg = reinterpret_cast<const asx_itch_1_1::order_book_directory&>(*buf);
        uint32_t ns = ntohl(msg.nanoseconds);
	uint32_t book_id=ntohl(msg.order_book_id);
#ifdef CW_DEBUG
	printf("  order_book_directory ns=%u symbol=%-.32s isin=%-.12s currency=%-.3s financial_product=%hhd num_decimals_in_price=%hu\n", ns, msg.symbol, msg.isin, msg.currency, msg.financial_product, ntohs(msg.num_decimals_in_price));
#endif
	
	sprintf(prtBuf, "order book id:%lu symbol:%-.32s isin:%-.12s $$:%-.3s prod.type:%hhd-\n", book_id, msg.symbol, msg.isin, msg.currency, msg.financial_product); 
	
      }
      break;
    case COMBINATION_SYMBOL_DIRECTORY://'M':
      {
#ifdef CW_DEBUG
	printf("  combination_order_book_directory");
#endif
        sprintf(prtBuf,"  combination_order_book_directory");
      }
      break;
    case END_OF_BUSINESS_TRADE_DATE://'S':
      {
        const asx_itch_1_1::system_event_message& msg = reinterpret_cast<const asx_itch_1_1::system_event_message&>(*buf);
	
#ifdef CW_DEBUG
	printf("  system_event_message code=%c\n", msg.event_code);
#endif
	sprintf(prtBuf,"system_event_message code=%c\n", msg.event_code);
      }
      break;
    case ORDER_ADDED://'A':
      {
        const asx_itch_1_1::add_order_no_mpid& msg = reinterpret_cast<const asx_itch_1_1::add_order_no_mpid&>(*buf);
	
        uint32_t ns = ntohl(msg.nanoseconds);
        uint64_t order_id = ntohll(msg.order_id);
        uint64_t qty = ntohll(msg.quantity);
        uint32_t book_id = ntohl(msg.order_book_id);
        uint32_t book_pos = ntohl(msg.order_book_posn);
        uint32_t shares = (uint32_t)qty;
	MDPrice price = MDPrice::from_fprice(ntohl(msg.price));
        
#ifdef CW_DEBUG
        printf("  add_order ns:%u order_id:%lu side:%c shares:%u stock:%u price:%0.4f\n", ns, order_id, msg.side, shares, book_id, price.fprice());
#endif
	sprintf(prtBuf, " add order_id:%lu side:%c shares:%u stock:%u price:%0.4f\n", order_id, msg.side, shares, book_id, price.fprice());
      }
      break;
    default:
#ifdef CW_DEBUG
      printf("  message_type:%c not supported\n", buf[0]);
#endif
	sprintf(prtBuf, " message_type:%c not processed\n", buf[0]);
	break;
    }
    m_logger->log_printf(Log::INFO, "%s", prtBuf);

	if (m_current_logger){
		m_current_logger->printf(prtBuf);
		//m_current_logger->flush(); 
	}
    return;
}
  
#ifdef USE_MY
  size_t
  ASX_ITCH_1_1_Handler::parse2(size_t context, const char* buf, size_t len)
{
	/*
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
      if(m_record_only) {
        return len;
      }
    } moved to the end of parse_messge
    */
    
    const asx_itch_1_1::downstream_header_t* header = reinterpret_cast<const asx_itch_1_1::downstream_header_t*>(buf);
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    uint16_t counter=message_count;
    
    size_t off_set = sizeof(asx_itch_1_1::downstream_header_t);
    size_t parsed_len = off_set;
    const char* block = buf + off_set;
    
	channel_info* ci;
    asx_channel* asxCh = get_asx_channel(context);
    if (asxCh) ci=(asxCh->m_channel_info);
    else ci = get_channel_info(context);//m_channel_info_map[context];
    if (!ci) {
#ifdef CW_DEBUG
	    cout << __func__ <<  " UNKNOWN channel " << context << endl;
#endif
    }
    
    if(seq_num < ci->seq_no && message_count > 0) {
      //m_logger->log_printf(Log::INFO, "ASX_ITCH_1_1_Handler::parse2: %s received partially-new packet seq_num=%zd expected_seq_num=%zd msg_count=%d",
      //                     m_name.c_str(), seq_num, ci->seq_no, message_count);
      while(seq_num < ci->seq_no && counter > 0) {
        // a rare case, this message contains some things we've already seen and some
        // things we haven't, skip what we've already seen
        uint16_t message_length = ntohs(reinterpret_cast<const asx_itch_1_1::message_block_t*>(block)->message_length);
        parsed_len += 2 + message_length;
        if (unlikely(parsed_len > len)) {
          m_logger->log_printf(Log::ERROR, "ASX_ITCH_1_1_Handler::parse: void bytes more than len (%zd)", len);
          return len; //return the len so it got thrown away
        }
        block += 2 + message_length;
        --counter;
        ++seq_num;
      }
      if (block != buf+off_set){
#ifdef CW_DEBUG
	      printf("!!! Parse from middle of block %u != %u\n",
			      block, buf+off_set);
#endif
      }
    }
    
    for(size_t kk=0 ; kk < counter ; kk++) {
      uint16_t message_length = ntohs(reinterpret_cast<const asx_itch_1_1::message_block_t*>(block)->message_length);
      parsed_len += 2 + message_length;
      if (unlikely(parsed_len > len)) {
        m_logger->log_printf(Log::ERROR, "ASX_ITCH_1_1_Handler::parse: (%zd) too short for message count(%zd)", len, message_count);
	parsed_len -= (2 + message_length);
        break;
      }
#ifdef CW_DEBUG
	    if (m_test_mode){
  printf("ch.%d parse seqno: %06d \n", ci->context, ci->seq_no);
	    }
#endif
	    if (asxCh && asxCh->m_loggerRP){
		    m_current_logger=asxCh->m_loggerRP.get();
		    m_current_logger->printf( "ch.%d parse seqno: %06d ", ci->context, ci->seq_no);
	    }
      size_t parser_result = parse_message(context, block+2, message_length);
      if (unlikely(parser_result != message_length)) {
        m_logger->log_printf(Log::ERROR, "ASX_ITCH_1_1_Handler::parse: ERROR message parser returned %zu bytes parsed, expected %u\n",
                             parser_result, message_length);
#ifdef CW_DEBUG
cout << "Bad msg shown with inconsistent len " << endl;
#endif
        //return 0;
      }
      ci->seq_no++;
      block += 2 + message_length;
    }
    
    if(unlikely(parsed_len != len)) {
      m_logger->log_printf(Log::ERROR, "ASX_ITCH_1_1_Handler::parse: Received buffer of size %zd but parsed %zd, message_count=%d",
                           len, parsed_len, (int) ntohs(header->message_count));
      //return parsed_len;
    }
    
    return parsed_len;
}

#endif

#ifdef USE_MY
size_t
ASX_ITCH_1_1_Handler::parse(size_t context, const char* buffer, size_t len)
{
	if (len < sizeof(asx_itch_1_1::downstream_header_t)){
#ifdef CW_DEBUG
cout << "Ch. " << context << " data len=" << len << " too short"<< endl;
#endif
m_logger->log_printf(Log::WARN, "%s: ch. %u data len=%lu < header", __func__, context, len);
	}
	//this message might come from a Q !!
    const asx_itch_1_1::downstream_header_t* header = reinterpret_cast<const asx_itch_1_1::downstream_header_t*>(buffer);
    
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    size_t realCount = message_count;
    //uint64_t next_seq_num = seq_num + message_count;
    
    if (seq_num < 1) return len;
#ifdef CW_DEBUG
    if (m_test_mode){
    printf("ch.%d got seqno %lu count %u ", context, seq_num, message_count);
    }
#endif
    if(unlikely(m_drop_packets)) {
      --m_drop_packets;
      return 0;
    }
m_logger->log_printf(Log::INFO, "%s: ch.%u data len=%lu seqno=%llu", __func__, context, len, seq_num);

    /*all channel should be set before the program started----
      while(context >= m_channel_info_map.size()) {
      m_channel_info_map.push_back(channel_info("", m_channel_info_map.size() + 1));
      m_logger->log_printf(Log::INFO, "%s: %s increasing channel_info_map due to new context %lu", "ASX_ITCH_1_1_Handler::parse", m_name.c_str(), context);
      }
    */
    
    asx_channel* asxCh=get_asx_channel(context);
    if (!asxCh)
  {
#ifdef CW_DEBUG
	  printf("unknown channel and cannot find the state of channel");
#endif
	  return len;
  }

    Channel_State& service_state = asxCh->m_channel_state; 

    channel_info& wk_channel = *(asxCh->m_channel_info);
    if (wk_channel.seq_no < 1) wk_channel.seq_no=1;
    
#ifdef CW_DEBUG
    if (m_test_mode){
    printf(" expecting seqno %u\n", wk_channel.seq_no);
    }
#endif

    if(seq_num <  wk_channel.seq_no) {
      return len; //ask dispatcher to keep going
    }
    
    size_t return_len=get_itch_packet_length(buffer, len, realCount);
    size_t next_seq_num=seq_num+message_count;
    //if (return_len > len) return_len=len;
    if (wk_channel.seq_no==1 &&
		    seq_num > 1 &&
		    //in initial state
		    service_state.m_in_refresh_state &&
		    service_state.m_snapshot_timeout_time.ticks()==0)
    {
	    //need to connect the SoupBinTcp server
	    //
	    //first put in Q
	 wk_channel.queue.queue_push(seq_num, next_seq_num, buffer, return_len);
	    //
	    try_connect_snapshot(asxCh);
	    //
	    return return_len;
    }
    if (wk_channel.seq_no==1 && seq_num == 1){
	    //ok in sync; no need to connect;
	    if (wk_channel.snapshot_socket.fd < 1)
		    service_state.m_in_refresh_state=false;
		    service_state.m_refresh_rejected=true;
    }
    mutex_t::scoped_lock lock;
    if(context != 0) {
      lock.acquire(m_mutex);
    }

//in the following wk_channel.seq_no always represents the correct next sequence number
    if (service_state.m_in_refresh_state){
	    //TO-DO
	    //if service_state.m_in_retransmit_state
	    //put in service_state.m_retransmission_queue
	    //udp data put into service_state.m_refresh_queue
	    //make sure to process the 'G' and update channel state
	    //and check if to deQ the retransm Q
	service_state.m_in_retransmit_state=false;
	    //
	    //service_state.m_retransmission_queue.Enqueue(return_len, context, buffer);
	    //
	    wk_channel.queue.queue_push(seq_num, next_seq_num, buffer, return_len);
#ifdef CW_DEBUG
	    if (m_test_mode){
	    printf("Snapshot mode seqno:%d Qed-> Qsize:%d\n", seq_num, wk_channel.queue.size());
	    }
#endif
	    //now check if the recovery line is dropped
	    if (service_state.m_snapshot_timeout_time < Time::current_time())
	    {
		    process_snapshot_reject(asxCh);
	    }
	    return return_len;
    } else if (service_state.m_in_retransmit_state){
	    //m_recovery_end_seqno = gap_start -1
	    //if seqno > m_recovery_end_seqno put in m_retransmission_queue
	    //if a gap found in the m_in_retransmit_state
	    //just send another retransmission and 
	    //discard this message by returning return_len;
	    //otherwise process it
	    if (seq_num > wk_channel.seq_no){
		    if (seq_num > service_state.m_recovery_end_seqno+1)
		    {
	    wk_channel.queue.queue_push(seq_num, next_seq_num, buffer, return_len);
#ifdef CW_DEBUG
	    		if (m_test_mode){
	    printf("Recovery mode mcast msg seqno:%d Qed-> Qsize:%d\n", seq_num, wk_channel.queue.size());
	    				}
#endif
		    //service_state.m_retransmission_queue.Enqueue(return_len, context, buffer);
		    return return_len;
		  }
#ifdef CW_DEBUG
  printf("Recovery mode gap seqno:%d expect %d -> new recovery request sent\n", seq_num, wk_channel.seq_no);
#endif
	    send_retransmit_request(asxCh, wk_channel.seq_no, service_state.m_recovery_end_seqno - wk_channel.seq_no+1);
		    return return_len;
	    }
    } else if (wk_channel.seq_no<=1){
	    //so beginning of the day I gave up the snapshot session
#ifdef CW_DEBUG
cout << __func__ << "ch." << context << " Gave up snapshot session " << endl;
#endif
	    wk_channel.seq_no=seq_num;
    }
    
    if (seq_num > wk_channel.seq_no){
	    //create re_trans_mitt request
		//put in service_state.m_retransmission_queue
	    //update all channel states
	    
	    //service_state.m_retransmission_queue.Enqueue(return_len, context, buffer);
	    	bool ok;
	    	ok=send_retransmit_request(asxCh, wk_channel.seq_no, 
				seq_num - wk_channel.seq_no);
	    if (ok){
	    service_state.m_in_retransmit_state = true;
	    service_state.m_recovery_seqno=wk_channel.seq_no;
	    service_state.m_recovery_end_seqno=seq_num-1;
	    wk_channel.queue.queue_push(seq_num, next_seq_num, buffer, return_len);
#ifdef CW_DEBUG
	    if (m_test_mode){
  printf("Sync mode gap seqno:%d expect %d -> Qed(%d) and recovery request sent\n", seq_num, wk_channel.seq_no, wk_channel.queue.size());
	    }
#endif

	    }
	    return return_len;
    }

//start to process the in sync message
#ifdef CW_DEBUG
	    if (m_test_mode){
  printf("process in sync seqno:%d =? %d \n", seq_num, wk_channel.seq_no);
	    }
#endif
//--------------- start in sync mode here --------------
	if (message_count==0 || message_count==0xFFFF)
	{
#ifdef CW_DEBUG
		if (m_test_mode)
			printf("Go heartbeat");
#endif
		return return_len;
	}
      memcpy(asxCh->session, header->session, 10);
      //m_logger->log_printf(Log::INFO, "ASX_ITCH_1_1_Handler: %s Session = %10s", m_name.c_str(), m_session);
      wk_channel.seq_no = seq_num;
    /*
     * copy to the channel's session
       if(unlikely(strncmp(m_session, header->session, 10))) {
      return 0;
    }
    */
    
    size_t parsed_len =parse2(context, buffer, return_len);
    
    wk_channel.seq_no = next_seq_num;
    wk_channel.last_update = Time::currentish_time();
    
    if (service_state.m_in_retransmit_state &&
		    wk_channel.seq_no >  service_state.m_recovery_end_seqno){
	    //update the in_transmission_state to false
	    service_state.m_in_retransmit_state=false;
	    service_state.m_recovery_end_seqno=0;
	    //
	    process_MDQ(asxCh);
    }
      
  //if (wk_channel.seq_no % 50 == 0 &&
  if (asxCh && asxCh->m_loggerRP){
                      asxCh->m_loggerRP.get()->sync_flush();
  }
    
    return parsed_len;
}
#endif
  
  size_t
  ASX_ITCH_1_1_Handler::parse(size_t context, const char* buffer, size_t len)
{
  
    const asx_itch_1_1::downstream_header_t* header = reinterpret_cast<const asx_itch_1_1::downstream_header_t*>(buffer);
    
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    uint64_t next_seq_num = seq_num + message_count;
    
    if(unlikely(m_drop_packets)) {
      --m_drop_packets;
      return 0;
    }
    
    asx_channel* asxCh=get_asx_channel(context);
    if (!asxCh)
    {
    	m_logger->log_printf(Log::WARN, "%s: unknown channel %u\n", __func__, context);
    }

    channel_info* wk_channel= asxCh->m_channel_info;
    if(next_seq_num <= wk_channel->seq_no) {
      return 0;
    }
    
    mutex_t::scoped_lock lock;
    if(context != 0) {
      lock.acquire(m_mutex);
    }
    
    if(unlikely(!wk_channel->last_update.is_set())) {
      memcpy(asxCh->session, header->session, 10);
      m_logger->log_printf(Log::INFO, "ASX_ITCH_1_1_Handler: %s Session = %10s", m_name.c_str(), asxCh->session);
      wk_channel->seq_no = seq_num;
    }
    /*
    if(unlikely(strncmp(m_session, header->session, 10))) {
      return 0;
    }
    */
    
    if(next_seq_num <= wk_channel->seq_no) {
      return 0;
    } else 
	    if (asxCh->m_channel_state.m_in_refresh_state==true){
    wk_channel->queue.queue_push(seq_num, next_seq_num, buffer, len);
		    return len;
	    }
	    if(seq_num > wk_channel->seq_no) {
      if(wk_channel->begin_recovery(this, buffer, len, seq_num, next_seq_num)) {
        return 0;
      }
    }
    
    size_t parsed_len = parse2(context, buffer, len);
    
    wk_channel->seq_no = next_seq_num;
    wk_channel->last_update = Time::currentish_time();
    
    if(!wk_channel->queue.empty()) {
      wk_channel->process_recovery(this);
    }
    
    return parsed_len;
}

  size_t
  ASX_ITCH_1_1_Handler::parse2(size_t context, const char* buffer, size_t len)
{
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buffer, len);
      if(m_record_only) {
        return len;
      }
    }
    
    const asx_itch_1_1::downstream_header_t* header = reinterpret_cast<const asx_itch_1_1::downstream_header_t*>(buffer);
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    
    size_t parsed_len = sizeof(asx_itch_1_1::downstream_header_t);
    const char* block = buffer + sizeof(asx_itch_1_1::downstream_header_t);
    
    asx_channel* asxCh=get_asx_channel(context);
    channel_info* wk_channel= asxCh->m_channel_info;

    if(seq_num < wk_channel->seq_no && message_count > 0) {
      //m_logger->log_printf(Log::INFO, "ASX_ITCH_1_1_Handler::parse2: %s received partially-new packet seq_num=%zd expected_seq_num=%zd msg_count=%d",
      //                     m_name.c_str(), seq_num, wk_channel->seq_no, message_count);
      while(seq_num < wk_channel->seq_no && message_count > 0) {
        // a rare case, this message contains some things we've already seen and some
        // things we haven't, skip what we've already seen
        uint16_t message_length = ntohs(reinterpret_cast<const asx_itch_1_1::message_block_t*>(block)->message_length);
        parsed_len += 2 + message_length;
        if (unlikely(parsed_len > len)) {
          m_logger->log_printf(Log::ERROR, "ASX_ITCH_1_1_Handler::parse: tried to more bytes than len (%zd)", len);
          return 0;
        }
        block += 2 + message_length;
        --message_count;
        ++seq_num;
      }
    }
   size_t last_seqno=seq_num+message_count; 
    for( ; message_count > 0; --message_count) {
      uint16_t message_length = ntohs(reinterpret_cast<const asx_itch_1_1::message_block_t*>(block)->message_length);
      parsed_len += 2 + message_length;
      if (unlikely(parsed_len > len)) {
        m_logger->log_printf(Log::ERROR, "ASX_ITCH_1_1_Handler::parse: tried to more bytes than len (%zd)", len);
        return 0;
      }
        m_logger->log_printf(Log::INFO, "ch. %u processing seqno %u \n",
			context, last_seqno - message_count );
      size_t parser_result = parse_message(0, block+2, message_length);
      if (unlikely(parser_result != message_length)) {
        m_logger->log_printf(Log::ERROR, "ASX_ITCH_1_1_Handler::parse: ERROR message parser returned %zu bytes parsed, expected %u\n",
                             parser_result, message_length);
        return 0;
      }
      block += 2 + message_length;
    }
    
    if(unlikely(parsed_len != len)) {
      m_logger->log_printf(Log::ERROR, "ASX_ITCH_1_1_Handler::parse: Received buffer of size %zd but parsed %zd, message_count=%d",
                           len, parsed_len, (int) ntohs(header->message_count));
      return 0;
    }
    
    return parsed_len;
}
  
#ifdef USE_HEADER1
void ASX_ITCH_1_1_Handler::prepend_tcp_to_parse(const char* session, size_t context, size_t seqno, const char*buf, size_t len)
{
	char new_msg[len+sizeof(asx_itch_1_1::downstream_header_t)];
	asx_itch_1_1::downstream_header_t* header = reinterpret_cast<asx_itch_1_1::downstream_header_t*>(new_msg);
	memset(header->session, ' ', sizeof(header->session));
	memcpy(header->session, session, sizeof(header->session));
	header->seq_num = htonll(seqno);
	header->message_count = htons(1);
	char *p=&new_msg[sizeof(asx_itch_1_1::downstream_header_t)];
	memcpy(p, buf, len);
	parse2(context, new_msg, len+sizeof(asx_itch_1_1::downstream_header_t));
}
#endif

  size_t
  ASX_ITCH_1_1_Handler::parse_message(size_t context, const char* buf, size_t len)
{
    const char* where = "ASX_ITCH_1_1_Handler::parse_message";
    
    if(len == 0) {
      m_logger->log_printf(Log::ERROR, "%s %s called with len=0", where, m_name.c_str());
      return len;
    }
    
	    if (m_test_mode){
  print_message(buf);
	    }
    switch(buf[0]) { // this index is the first one
    case TIME_MESSAGE://'T':
      parseTIME_MESSAGE_T(context, buf, len); 
      break;
    case  TICK_SIZE_TABLE://'L':
      parseTICK_SIZE_TABLE_L(context, buf, len); 
      break;
    case EQUITY_SYMBOL_DIRECTORY://'R':
      parseEQUITY_SYMBOL_DIRECTORY_R(context, buf, len);
      break;
    case COMBINATION_SYMBOL_DIRECTORY://'M':
      parseCOMBINATION_SYMBOL_DIRECTORY_M(context, buf, len);
      break;
    case END_OF_BUSINESS_TRADE_DATE://'S':
      parseEND_OF_BUSINESS_TRADE_DATE_S(context, buf, len);
      flush_all_loggers();
      break;
    case ORDER_BOOK_STATE://'O':
      parseORDER_BOOK_STATE_O(context, buf, len);
      break;
    case ORDER_ADDED://'A':
      parseORDER_ADDED_A(context, buf, len); 
      break;
    case ORDER_ADDED_WITH_PARTICIPANT_ID://'F':
      parseORDER_ADDED_WITH_PARTICIPANT_ID_F(context, buf, len);
      break;
    case ORDER_EXECUTED://'E':
      parseORDER_EXECUTED_E(context, buf, len);
      break;
    case AUCTION_ORDER_EXECUTED://'C':
      parseAUCTION_ORDER_EXECUTED_C(context, buf, len);
      break;
    case ORDER_DELETED://'D':
      parseORDER_DELETED_D(context, buf, len);
      break;
    case ORDER_REPLACED: //'U':
      parseORDER_REPLACED_U(context, buf, len);
      break;
    case TRADE_EXECUTED://'P':
      parseTRADE_EXECUTED_P(context, buf, len);
      break;
    case EQUILIBRIUM_PRICE_AUCTION_INFO://'Z':
      parseEQUILIBRIUM_PRICE_AUCTION_INFO_Z(context, buf, len);
      break;
    default:
      send_alert("%s %s called with unknown message type '%c'", where, m_name.c_str(), buf[0]);
      break;
    };
    
    if (m_qbdf_builder && m_hist_mode) {
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where, m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }
#ifdef USE_MY
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
      if(m_record_only) {
        return len;
      }
    }
#endif
    
    return len;
}

void ASX_ITCH_1_1_Handler::process_MDQ(ASX_ITCH_1_1_Handler::asx_channel* asxCh)
{
	channel_info& wk_channel=*(asxCh->m_channel_info);
	Channel_State& service_state=asxCh->m_channel_state;
	service_state.m_in_refresh_state = false;
#ifdef CW_DEBUG
#ifdef CW_DEBUG
printf("\nChannel %d DeQ: %d ",wk_channel.context, wk_channel.queue.size());
#endif
#endif
	while (!wk_channel.queue.empty()){
		MDBufferQueue::queued_message aMsg=wk_channel.queue.top();
#ifdef CW_DEBUG
printf(" first msgNo:%d count:%d\n\n",aMsg.seq_num, aMsg.next_seq_num-aMsg.seq_num);
#endif
		if (aMsg.seq_num>wk_channel.seq_no){
#ifdef CW_DEBUG
			printf("DeQ : GAP !!!");
#endif
			bool ok;

    		ok=send_retransmit_request(asxCh, wk_channel.seq_no, 
				    aMsg.seq_num - wk_channel.seq_no);
		if (ok){
	    		service_state.m_in_retransmit_state = true;
	    		service_state.m_recovery_seqno=wk_channel.seq_no;
	    		service_state.m_recovery_end_seqno=aMsg.seq_num-1;
			}
			else service_state.m_in_retransmit_state = false;
	    		return;
	    	}
		if (aMsg.seq_num==wk_channel.seq_no){
		    parse2(wk_channel.context, aMsg.msg, aMsg.msg_len);
		} else if (wk_channel.seq_no > aMsg.seq_num && 
				wk_channel.seq_no < aMsg.next_seq_num){
			//create a new packet for partial
			//or send to parse_message directly
	asx_itch_1_1::downstream_header_t* header = reinterpret_cast<asx_itch_1_1::downstream_header_t*>(aMsg.msg);
    			uint64_t seq_num = aMsg.seq_num;
			/*
    			uint16_t message_count = ntohs(header->message_count);
    			uint64_t next_seq_num = seq_num + message_count;
			header->seq_num=htonll(wk_channel.seq_no);
    			uint16_t new_count=next_seq_num - wk_channel.seq_no;
    			header->message_count=htons(new_count);
			*/

			char* pNewH=aMsg.msg+sizeof(*header);
			while (seq_num++ < wk_channel.seq_no){
        			uint16_t message_length = ntohs(reinterpret_cast<const asx_itch_1_1::message_block_t*>(pNewH)->message_length);
        			pNewH += (2 + message_length);
			}
			/*
			pNewH -= sizeof(*header);
			memcpy(pNewH, header, sizeof(*header));
	    		parse2(wk_channel.context, pNewH, aMsg.msg_len - (pNewH - aMsg.msg));
			*/
			while (wk_channel.seq_no < aMsg.next_seq_num){
        			uint16_t message_length = ntohs(reinterpret_cast<const asx_itch_1_1::message_block_t*>(pNewH)->message_length);
#ifdef CW_DEBUG
  printf("ch.%d parse seqno: %06d partial \n", wk_channel.context, wk_channel.seq_no);
#endif
  if (asxCh && asxCh->m_loggerRP){
                      m_current_logger=asxCh->m_loggerRP.get();
		    m_current_logger->printf( "ch.%d parse seqno: %06d partial ", wk_channel.context, wk_channel.seq_no);
	    }
		parse_message(wk_channel.context, pNewH+2, message_length);
		wk_channel.seq_no++;
        			pNewH += (2 + message_length);
			}
      		}
		wk_channel.queue.queue_pop();
	    }
    //service_state.m_refresh_queue.Reset();
    service_state.m_in_refresh_state = false;
    service_state.m_in_retransmit_state = false;
}

  void
ASX_ITCH_1_1_Handler::process_queued_messages(size_t last_msg_seq_num, size_t context)
{
#ifdef USE_MY_Q
  asx_channel* asxCh=get_asx_channel(context);
    if (!asxCh) return;
    Channel_State& service_state = *(asxCh->m_channel_state);
    
    size_t cur_queue_offset = 0;
    while(service_state.m_refresh_queue.HasMore(cur_queue_offset))
      {
	size_t cur_len;
	size_t cur_context;
	const char* buf = service_state.m_refresh_queue.GetPayloadAt(cur_queue_offset, cur_len, cur_context);
	//Queued_Message_Header* queued_msg_header = reinterpret_cast<Queued_Message_Header*>(&(service_state.m_queued_message_buffer[service_state.m_queued_message_offset]));
	const asx_itch_1_1::downstream_header_t& packet = reinterpret_cast<const asx_itch_1_1::downstream_header_t&>(*buf);
	
	uint32_t seq_no = ntohl(packet.seq_num);
	if(seq_no > last_msg_seq_num)
	  {
	    m_channel_info_map[cur_context].seq_no = seq_no + 1;
	    parse2(cur_context, buf, cur_len);
	  } // Else it was already included in the snapshot
      }
    service_state.m_refresh_queue.Reset();
    service_state.m_in_refresh_state = false;
    m_logger->log_printf(Log::INFO, "ASX_ITCH Snapshot process_queued_messages - done");
#endif
}
  
  static char last_session[11]="1234567890";
  static uint64_t next_seqno=1;
  static size_t msg_context=0;
  size_t 
  ASX_ITCH_1_1_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
  {
    const asx_itch_1_1::downstream_header_t* header = reinterpret_cast<const asx_itch_1_1::downstream_header_t*>(buf);
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    /*for test
    if (strncmp(last_session, header->session, sizeof(m_tcp_session))!=0)
    {
      memcpy(last_session, header->session, sizeof(m_tcp_session));
#ifdef CW_DEBUG
      cout << "New Session :" << last_session << endl;
#endif
    }
    if (msg_context != context){
#ifdef CW_DEBUG
      cout << "New Context :" << context << " != " << msg_context << endl;
#endif
      msg_context=context;
    }
    
    if (next_seqno != seq_num)
    {
#ifdef CW_DEBUG
	cout << context << " >>msg gap :" << seq_num << " != " << next_seqno << endl;
#endif
    }
    */
    next_seqno=seq_num+message_count;
    
    log.printf(" context:%lu  seq_num:%lu  message_count:%u  \n", context, seq_num, message_count);
    
    const char* block = buf + sizeof(asx_itch_1_1::downstream_header_t);
    
    for(uint16_t i = 0; i < message_count; ++i) {
      uint16_t message_length = ntohs(reinterpret_cast<const asx_itch_1_1::message_block_t*>(block)->message_length);
      log.printf("  message_length:%u", message_length);
      dump_message(log, filter, context, block + 2, len);
      block += 2 + message_length;
    }
    
    return len;
  }
  
  size_t
  ASX_ITCH_1_1_Handler::dump_message(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
{
    switch(buf[0]) {
    case TIME_MESSAGE://'T':
      {
        const asx_itch_1_1::timestamp_message& msg = reinterpret_cast<const asx_itch_1_1::timestamp_message&>(*buf);
	
        uint32_t sec = ntohl(msg.seconds);
        Time_Duration ts = seconds(sec);
	
        char time_buf[32];
        Time_Utils::print_time(time_buf, ts, Timestamp::MILI);
	
        log.printf("  timestamp %s\n", time_buf);
      }
      break;
    case EQUITY_SYMBOL_DIRECTORY://'R':
      {
        const asx_itch_1_1::order_book_directory& msg = reinterpret_cast<const asx_itch_1_1::order_book_directory&>(*buf);
        uint32_t ns = ntohl(msg.nanoseconds);
	log.printf("  order_book_directory ns=%u symbol=%-.32s isin=%-.12s currency=%-.3s financial_product=%hhd num_decimals_in_price=%hu\n",
                   ns, msg.symbol, msg.isin, msg.currency, msg.financial_product, ntohs(msg.num_decimals_in_price));
      }
      break;
    case COMBINATION_SYMBOL_DIRECTORY://'M':
      {
	log.printf("  combination_order_book_directory");
      }
      break;
    case END_OF_BUSINESS_TRADE_DATE://'S':
      {
        const asx_itch_1_1::system_event_message& msg = reinterpret_cast<const asx_itch_1_1::system_event_message&>(*buf);
	
	log.printf("  system_event_message code=%c\n", msg.event_code);
      }
      break;
    case ORDER_ADDED://'A':
      {
        const asx_itch_1_1::add_order_no_mpid& msg = reinterpret_cast<const asx_itch_1_1::add_order_no_mpid&>(*buf);
	
        uint32_t ns = ntohl(msg.nanoseconds);
        uint64_t order_id = ntohll(msg.order_id);
        uint64_t qty = ntohll(msg.quantity);
        uint32_t shares = (uint32_t)qty;
	MDPrice price = MDPrice::from_fprice(ntohl(msg.price));
        
        log.printf("  add_order ns:%u order_id:%lu side:%c shares:%u stock:%u price:%0.4f\n",
                   ns, order_id, msg.side, shares, ntohl(msg.order_book_id), price.fprice());
      }
      break;
    default:
      log.printf("  message_type:%c not supported\n", buf[0]);
      break;
    }
    return len;
}
  
bool
ASX_ITCH_1_1_Handler::establish_refresh_connection(channel_info* wk_channel)
{
    string where = __func__;
    
    if (wk_channel->snapshot_socket.connected()) return true;
    wk_channel->snapshot_is_on=false;
    m_logger->log_printf(Log::INFO, "%s ch. %d", where.c_str(), wk_channel->context);
    int ret = wk_channel->snapshot_socket.connect_tcp(wk_channel->snapshot_addr);
    if(ret == 0) {
      send_alert("%s: Unable to connect to refresh host, line %d", where, wk_channel->context);
#ifdef CW_DEBUG
cout << where << " - refresh connection failed" << endl;
#endif
      return false;
    }
    wk_channel->snapshot_socket.set_nodelay();
    
    if(fcntl(wk_channel->snapshot_socket, F_SETFL, O_NONBLOCK) < 0) 
    {
//don't throw, snapshot is not critical
      //throw errno_exception((where +": fcntl(O_NONBLOCK) failed ").c_str(), errno);
    m_logger->log_printf(Log::WARN, "snapshot no-block failed ch.%d fd=%d", where.c_str(), wk_channel->context, wk_channel->snapshot_socket.fd);
	    wk_channel->snapshot_socket.close();
	    return false;
    }
    asx_channel* asxCh=get_asx_channel(wk_channel->context);
    if (!asxCh) return true;//because the dispatcher not start yet
    //wk_channel->snapshot_is_on=true;
    if (m_dispatcher){
	    m_dispatcher->add_tcp_fd(this, wk_channel->name, wk_channel->context,wk_channel->snapshot_socket.fd, 2048);
	    m_dispatcher->start_sync(this, wk_channel->name);
    }
    m_logger->log_printf(Log::INFO, "%s connected at %d", where.c_str(), wk_channel->snapshot_socket.fd);
    return true;
}

  void
ASX_ITCH_1_1_Handler::establish_refresh_connection(size_t channel_id)
{
    string where = __func__;
    
    //cout << where << endl;
    channel_info* wk_channel = get_channel_info(channel_id);
    if (!wk_channel){
#ifdef CW_DEBUG
	    printf("%s found no channel for id=%d\n", __func__, channel_id);
#endif
	    return ;
    }
    if (wk_channel->context != channel_id) return ;

    establish_refresh_connection(wk_channel);
}
  
void
  ASX_ITCH_1_1_Handler::try_connect_snapshot(asx_channel* asxCh)
{
	if (asxCh->m_channel_state.m_refresh_rejected)
		return;
	bool try1=establish_refresh_connection(asxCh->m_channel_info);
	if (!try1) {
		//if the mcast session alredy started
		//then no try if Qsize > 5
		//reason : if mcast ok means tcp hiccups
		//otherwise I should get connected immediately
		if (asxCh->m_channel_info->queue.size() > 5){
			process_snapshot_reject(asxCh);
		}
		//otherwise, still early and let's try again
	return;
	}
	if (send_snapshot_login(asxCh)){
	asxCh->m_channel_state.m_in_refresh_state=true;
	asxCh->m_channel_state.m_snapshot_timeout_time=Time::current_time()+msec(10*1000);
	}
}

  void
  ASX_ITCH_1_1_Handler::establish_retransmission_connection(size_t channel_id)
  {
	  /*
    string where = __func__;
    
#ifdef CW_DEBUG
    cout << where << endl;
#endif
    //TO DO :: use the same udp socket to send udp diagram to "request server" 
    //not TCP; 
    int ret = m_retransmission_socket.connect_tcp(m_retransmission_addr);
    if(ret == 0) {
      send_alert("%s: %s Unable to connect to retransmission host", where);
#ifdef CW_DEBUG
      cout << where << " - retransmission connection failed" << endl;
#endif
    }
    m_retransmission_socket.set_nodelay();
    
    if(fcntl(m_retransmission_socket, F_SETFL, O_NONBLOCK) < 0) {
      throw errno_exception("ASX_ITCH_1_1_Handler::establish_retransmission_connection: fcntl(O_NONBLOCK) failed ", errno);
    }
    
#ifdef CW_DEBUG
    cout << "ASX_ITCH::establish_retransmission_connection - m_retransmission_socket.connected()=" << m_retransmission_socket.connected() << endl;
#endif
    
#ifdef CW_DEBUG
    cout << Time::current_time().to_string() << " about to sleep" << endl;
#endif
    ::sleep(1);
#ifdef CW_DEBUG
    cout << Time::current_time().to_string() << " finished sleep" << endl;
#endif
    */
  }
  
void ASX_ITCH_1_1_Handler::process_snapshot_reject(asx_channel* asxCh){

	if (asxCh->m_channel_info->snapshot_socket.connected())
	send_tcp_logout(asxCh->m_channel_info->context);
	//*(asxCh->m_channel_info).seq_no=service_state.m_refresh_start_seqno;
	//process_queued_messages(service_state.m_refresh_start_seqno, context);
	asxCh->m_channel_state.m_in_refresh_state=false;
	//asxCh->m_channel_state.m_snapshot_timeout_time=0;
	asxCh->m_channel_state.m_refresh_rejected=true;
#ifdef CW_DEBUG
	cout << __func__ << endl;
#endif
	process_MDQ(asxCh);
}

  void
  ASX_ITCH_1_1_Handler::execute_tcp_data(size_t context, const char* buf, size_t len)
{
    const asx_itch_1_1::SoupBinTcpBeat& data = reinterpret_cast<const asx_itch_1_1::SoupBinTcpBeat&>(*buf);
    uint16_t claim_len=ntohs(data.packet_length);
    /* moved to the end of parse_message
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
      if(m_record_only) {
	return ;
      }
    }
    */

    //if (buf[2]=='T') {
	    //cout << "Time message throw away" << endl;
    //}
//printf("TCP %d data type(%c):<%-8s>\n", claim_len, buf[2], &buf[3]);
    asx_channel* asxCh=get_asx_channel(context);
    if (!asxCh) return;
    Channel_State& service_state = asxCh->m_channel_state;
    channel_info* wk_channel = asxCh->m_channel_info;
    size_t processed_len=0;
    
    //switch (buf[2]) { 
    if ( 'A' == buf[2]) { 
    m_logger->log_printf(Log::INFO, "channel %d get login accept at %d", context, wk_channel->snapshot_socket);
      // case 'A': 
      if (len < sizeof(asx_itch_1_1::SoupBinTcpAccept))
      {
#ifdef CW_DEBUG
	cout << __func__ << " socket data " << len << " too short for A type expecting " << sizeof(asx_itch_1_1::SoupBinTcpAccept) << endl; 
#endif
	return ;
      }
      const asx_itch_1_1::SoupBinTcpAccept& msg = reinterpret_cast<const asx_itch_1_1::SoupBinTcpAccept&>(*buf);
      memcpy(m_tcp_session, &msg.session, sizeof(m_tcp_session)); 
      memcpy(asxCh->session, &msg.session, sizeof(m_tcp_session)); 
      //int iLen=sizeof(asx_itch_1_1::SoupBinTcpAccept::seqno);
      char* p0=msg.seqno;
      char* p9=p0+sizeof(asx_itch_1_1::SoupBinTcpAccept::seqno)-1;
      while (p0 < p9 && *p0 < '0') p0++;
      while (p0 < p9 && *p9 < '0') p9--;
      int seqno=1; //stoi(msgCount);
      if (p9>p0){
	     seqno=stoi(string(p0, p9-p0+1));
      } else if (*p0 > ' ') seqno=*p0-48;
      /*while (msg.seqno[iLen-1]!=' ') iLen--;
      string msgCount(&msg.seqno[iLen], sizeof(asx_itch_1_1::SoupBinTcpAccept::seqno)-iLen);
      */
#ifdef CW_DEBUG
      cout << __func__ << " login accepted with seqno=" << seqno << endl;
#endif
      wk_channel->seq_no=seqno;
    	service_state.m_received_refresh_start=true;
	service_state.m_snapshot_timeout_time=Time::current_time()+msec(3000);
      //break;
    }  else if (buf[2] =='J') { 
      //case 'J':
      const string& reasonA="Not Authorized";
      const string& reasonS="Session not available";
#ifdef CW_DEBUG
      cout << __func__ << "got Login Reject: " << (buf[3]=='A'?reasonA:reasonS) << endl; 
#endif
    m_logger->log_printf(Log::INFO, "channel %u get login reject %s", context, buf[3]=='A'?reasonA.c_str():reasonS.c_str());
      process_snapshot_reject(asxCh);
      //break;
    } else if (buf[2] == 'S') {
      //case 'S':
      if (buf[3]=='G'){
	send_tcp_logout(context);
	char seqnoS[21] ;
	strncpy(seqnoS, &buf[4], 20);
	char* pp;
	uint64_t seqno=strtoll(seqnoS, &pp, 10);
	wk_channel->seq_no=seqno>0?seqno:1;
	service_state.m_in_refresh_state=false;
	asxCh->m_channel_state.m_refresh_rejected=true;
#ifdef CW_DEBUG
	cout << "get end of snapshot and set seqno=" << wk_channel->seq_no << endl;
#endif
    m_logger->log_printf(Log::INFO, "channel %u get end of snapshot and set seqno= %d", context, wk_channel->seq_no);
	//process_queued_messages(wk_channel->seq_no, context);
  if (asxCh && asxCh->m_loggerRP){
                      asxCh->m_loggerRP.get()->sync_flush();
  }
#ifdef CW_DEBUG
	cout << __func__ << " got End of SS" << endl;
#endif
#ifdef USE_MY
	process_MDQ(asxCh);//wk_channel->seq_no, context);
#endif

      }
      else
	{
	  if (asxCh) asxCh->snapshot_msg_received++;
  if (asxCh && asxCh->m_loggerRP){
                      m_current_logger=asxCh->m_loggerRP.get();
		    m_current_logger->printf( "ch.%d parse seqno: %06u tcp: ", context, asxCh->snapshot_msg_received);
	  //print_message(&buf[3]);
	    }
  if (len-3 > 0)
  {
	char new_msg[360];//
	//claim_len-1+sizeof(asx_itch_1_1::downstream_header_t)];
	asx_itch_1_1::downstream_header_t* header = reinterpret_cast<asx_itch_1_1::downstream_header_t*>(new_msg);
	memset(header->session, ' ', sizeof(header->session));
	memcpy(header->session, asxCh->session, sizeof(header->session));
	uint64_t seqNo=asxCh->snapshot_msg_received;
        header->seq_num=htonll(seqNo);
        uint16_t msgCount=1;
	header->message_count = htons(msgCount);
	char *p=&new_msg[sizeof(asx_itch_1_1::downstream_header_t)];
	asx_itch_1_1::message_block_t* bb=reinterpret_cast<const asx_itch_1_1::message_block_t*>(p);
	bb->message_length=htons(claim_len-1);
	p += sizeof(*bb);
	memcpy(p, &buf[3], claim_len-1);
	parse2(context, new_msg, claim_len-1+sizeof(*header)+sizeof(*bb));
	//prepend_tcp_to_parse(asxCh->session, context, asxCh->snapshot_msg_received, &buf[3], len-3);
	  
	  //parse_message(context, &buf[3], len-3);
  }
  else m_current_logger->printf( "\n" );
	}
      //break;
    } else if (buf[2] == 'Z') {
      //case 'Z':
      m_tcp_got_session_end=true;
      //break;
    } else if (buf[2] == '+') { 
      //case '+':
      const asx_itch_1_1::SoupBinTcpDebug& dbg = reinterpret_cast<const asx_itch_1_1::SoupBinTcpDebug&>(*buf);
#ifdef CW_DEBUG
      cout << string(&buf[3], claim_len-1) << endl;
#endif
      //break;
    } else {
      //default:
#ifdef CW_DEBUG
      cout << __func__ << " should not get unrecognized signal " << '\n';
#endif
      //break;
    }
    processed_len=claim_len+2;
    //return processed_len;
}
  
  size_t
  ASX_ITCH_1_1_Handler::parse_tcp_data(size_t context, const char* buf, size_t len)
  {
    if (len < 3) {
#ifdef CW_DEBUG
      cout << __func__ << " data too short len=" << len << endl;
#endif
      return 0;
    }
    asx_channel* asxCh=get_asx_channel(context);
    if (!asxCh)
    {
#ifdef CW_DEBUG
	    printf("%s from unknow channel %d\n", __func__, context);
#endif
	    return len;
    }
    channel_info& wk_channel=*(asxCh->m_channel_info);
    wk_channel.snapshot_is_on=true;
    
    const asx_itch_1_1::SoupBinTcpBeat& data = reinterpret_cast<const asx_itch_1_1::SoupBinTcpBeat&>(*buf);
    uint16_t claim_len=ntohs(data.packet_length);
    if (len < claim_len+2)
    {
#ifdef CW_DEBUG
      cout << __func__ << " socket data len=" << len << " too short expecting " << claim_len << endl; 
#endif
      return 0;
    }
    /*
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
      if(m_record_only) {
        return len;
      }
    }*/
    execute_tcp_data(context, buf, len);
    size_t processed_len=claim_len+2;
    if (Time::current_time() > asxCh->next_heartbeat_time)
    {
      send_tcp_heartbeat(asxCh);
    }
    return processed_len;
  }

  size_t
  ASX_ITCH_1_1_Handler::read(size_t context, const char* buf, size_t len)
{
	string where = __func__;
	asx_channel* asxCh=get_asx_channel(context);
	//if (!buf) process_pending_outbound_Q();
	//add a trick here so that this handler can be informed that the 
	//tcp socket is closed
	if (len > UINT_MAX - 150)
	{
		//close this tcp socket based on context
		if (!asxCh) return 0;
		channel_info& wk_channel = *(asxCh->m_channel_info);
		if (wk_channel.snapshot_socket.connected())
			wk_channel.snapshot_socket.close();
		try_connect_snapshot(asxCh);
		//retry if condition is right
		//or give up tcp session and deQ
		return 0;
	}
	if(len ==0) {
		if (!asxCh) return 0;
		channel_info& wk_channel = *(asxCh->m_channel_info);

		if (asxCh->m_channel_state.m_in_refresh_state &&
		asxCh->m_channel_state.m_snapshot_timeout_time.ticks() > 0 && 
		Time::current_time() > asxCh->m_channel_state.m_snapshot_timeout_time){
			process_snapshot_reject(asxCh);	
			return 0;
		}

		if (!wk_channel.snapshot_is_on) {
			if (asxCh->m_channel_state.m_refresh_rejected)
				return 0;
			if (wk_channel.snapshot_socket.connected())
				send_snapshot_login(asxCh);
			return 0 ;
		}

		if (Time::current_time() > asxCh->next_heartbeat_time)
		{
			send_tcp_heartbeat(asxCh);
		}

		return 0;
	}
	return parse_tcp_data(context, buf, len);
}
  
  void
  ASX_ITCH_1_1_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    
    if(m_order_book) {
      m_order_book->clear(false);
      if(m_qbdf_builder && m_micros_recv_time > 0) {
	m_logger->log_printf(Log::ERROR, "GAP MESSAGE");
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_exch_char, -1);
      }
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }

  void
  ASX_ITCH_1_1_Handler::reset(size_t context, const char* msg)
  {
    if(context == 3) { // quote context
      reset(msg);
      return;
    }
    
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    
  }

  size_t
  ASX_ITCH_1_1_Handler::dump_refdata_headers(ILogger& log)
  {
    log.printf("instrument_code,symbol_index,instrument_name,trading_code,mic,financial_market_code,mic_list,lot_size,fix_price_tick,type_of_market_admission,issuing_country_code,event_date,instrument_group,instrument_category,type_of_instrument,mnemo\n");
    return 0;
  }

  size_t
  ASX_ITCH_1_1_Handler::dump_refdata(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len)
  {
    const asx_itch_1_1::downstream_header_t& packet = reinterpret_cast<const asx_itch_1_1::downstream_header_t&>(*buf);
   /* 
    uint8_t  num_msgs = packet.number_msgs;
    Time_Duration send_time = Time_Duration(Time_Constants::ticks_per_mili * (uint64_t)ntohl(packet.send_time));
    
    char time_buf[32];
    Time_Utils::print_time(time_buf, send_time, Timestamp::MICRO);
    
    buf += sizeof(asx_itch_1_1::downstream_header_t);
    
    char decompressed_buf[ZLIB_BUF_LEN];
    if(packet.delivery_flag == 17) {
      if(packet_len <= sizeof(asx_itch_1_1::downstream_header_t)) {
        return packet_len;
      }
      // must decompress
      //unsigned new_len =
      decompress_buffer_one_shot(buf, packet_len-sizeof(asx_itch_1_1::downstream_header_t), decompressed_buf);
      buf = decompressed_buf;
    }
    
    for(int msg_idx = 0; msg_idx < num_msgs; ++msg_idx) {
      const message_header& header = reinterpret_cast<const message_header&>(*buf);
      
      uint16_t msg_size = ntohs(header.msg_size);
      uint16_t msg_type = ntohs(header.msg_type);
      
      const char* msg_buf = buf;
      buf += 2 + msg_size;
      switch(msg_type) {
      case 553:
        {
          const reference_data_message& msg = reinterpret_cast<const reference_data_message&>(*msg_buf);
          SIZE_CHECK(reference_data_message);
          
          uint32_t symbol_index = ntohl(msg.symbol_index);
          Time_Duration source_time = GET_TIME_DURATION(msg);
          Time_Utils::print_time(time_buf, source_time, Timestamp::MICRO);
          
          char instrument_name[18];
          for(int i = 0; i < 18; ++i) {
            char c = msg.instrument_name[i];
            if(!(c == ',' || c == '\'' || c == '"' || c == '\n')) {
              instrument_name[i] = c;
            } else {
              instrument_name[i] = '_';
            }
          }
          
          string mnemo = strlen(msg.mnemo) > sizeof(msg.mnemo) ? string(msg.mnemo, sizeof(msg.mnemo)) : string(msg.mnemo);
 
          log.printf("%.12s,%u,%.18s,%.12s,%.4s,%.3s,%.24s,%lu,%u,%c,%.3s,%.8s,%.3s,%c,%hu,%s\n",
                     msg.instrument_code, symbol_index, instrument_name, msg.trading_code,
                     msg.mic, msg.financial_market_code, msg.mic_list,
                     ntohll(msg.lot_size), ntohl(msg.fix_price_tick),
                     msg.type_of_market_admission, msg.issuing_country_code, msg.event_date, msg.instrument_group_code, msg.instrument_category,
                     ntohs(msg.type_of_instrument), mnemo.c_str());
          
        }
        break;
      }
    }
    */
    return packet_len;
  }

  ProductID
  ASX_ITCH_1_1_Handler::product_id_of_symbol_index(uint32_t symbol_index)
  {
    //hash_map<uint32_t, symbol_info>::iterator it = m_symbol_info_map.find(symbol_index);
    map<uint32_t, symbol_info>::iterator it = m_symbol_info_map.find(symbol_index);
    if(it != m_symbol_info_map.end())
    {
      return it->second.product_id;
    }
    return Product::invalid_product_id;
  }
  
  void
  ASX_ITCH_1_1_Handler::send_quote_update()
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
    if(m_subscribers) {
      m_subscribers->update_subscribers(m_quote);
    }
  }
  
  void
  ASX_ITCH_1_1_Handler::send_quote_update(MDOrderBookHandler* order_book)
  {
    //if (order_book != m_order_book)
#ifdef CW_DEBUG
    cout<< "using m_orer_book" << endl;
#endif
    send_quote_update();
  }
  
  void
  ASX_ITCH_1_1_Handler::send_trade_update()
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
    if(m_subscribers) {
      m_subscribers->update_subscribers(m_trade);
    }
  }

  size_t
  ASX_ITCH_1_1_Handler::dump_sym_dir_data_header(ILogger& log)
  {
    log.printf("channel_id,order_book_id,symbol,isin\n");
    return 0;
  }

  size_t
  ASX_ITCH_1_1_Handler::dump_sym_dir_data(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len)
  {
  	const char* where = "ASX_ITCH_1_1_Handler::dump_sym_dir_data";
    const asx_itch_1_1::downstream_header_t* header = reinterpret_cast<const asx_itch_1_1::downstream_header_t*>(buf);
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    
    const char* block = buf + sizeof(asx_itch_1_1::downstream_header_t);
    
    for(uint16_t i = 0; i < message_count; ++i) {
      uint16_t message_length = ntohs(reinterpret_cast<const asx_itch_1_1::message_block_t*>(block)->message_length);
      block += 2;
      switch(block[0]) {
	    case EQUITY_SYMBOL_DIRECTORY://'R':
        {
          const asx_itch_1_1::order_book_directory& msg = reinterpret_cast<const asx_itch_1_1::order_book_directory&>(*block);

          if (msg.financial_product == 5) {
        	string symbol = string(msg.symbol, sizeof(msg.symbol));
	    	size_t found = symbol.find_last_not_of(" \t\f\v\n\r");
	    	if (found != string::npos)
	    	  symbol.erase(found+1);
	    	else
	    	  symbol.clear();

            log.printf("%lu,%lu,%s,%12s\n",context,msg.order_book_id,symbol.c_str(),msg.isin);
	  	  }
        }
        break;
      }
      block +=  message_length;
    }
    return packet_len;
  }

  bool
  ASX_ITCH_1_1_Handler::send_tcp_heartbeat(asx_channel* asxCh)
  {
    string where = __func__;
    channel_info& wk_channel = *(asxCh->m_channel_info);
    if (!wk_channel.snapshot_is_on) return false;
    if (!wk_channel.snapshot_socket.connected())
    {
      m_logger->log_printf(Log::WARN, (where+" socket not connected").c_str());
      return false;
    }

    int ret = ::send(wk_channel.snapshot_socket, &m_soup_heartbeat, sizeof(m_soup_heartbeat), MSG_DONTWAIT);

    if(ret < 0) {
      m_logger->log_printf(Log::WARN, (where+" got negative=%d").c_str(), ret);
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert((where + " send returned %d - %s").c_str(), errno, err_msg);
      return false;
    }
    asxCh->next_heartbeat_time = Time::current_time()+m_heartbeat_interval;
#ifdef CW_DEBUG
    if (m_test_mode){
	    printf("-Heartbeat-->> ");
    }
#endif
    			//spec interval is 1 second
    return true;
}
  
  bool
  ASX_ITCH_1_1_Handler::send_tcp_logout(size_t channel_id)
{
    string where = __func__;
#ifdef CW_DEBUG
    cout << __func__ << " for channel " << channel_id << endl;
#endif
    channel_info* wk_channel = get_channel_info(channel_id);
    if (!wk_channel->snapshot_is_on) return false;
    if (!wk_channel->snapshot_socket.connected())
    {
#ifdef CW_DEBUG
      cout << std::boolalpha ;
#endif
#ifdef CW_DEBUG
      cout << where << " m_snapshot_socket " << false << endl;
#endif
      return false;
    }
    
    int ret = ::send(wk_channel->snapshot_socket, &m_soup_logout, sizeof(m_soup_logout), MSG_DONTWAIT);
    
    if(ret < 0) {
      m_logger->log_printf(Log::WARN, (where+" got negative=%d").c_str(), ret);
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert((where + " send returned %d - %s").c_str(), errno, err_msg);
      return false;
    }
    m_last_beat_time=Time::current_time();
    wk_channel->snapshot_is_on=false;
    return true;
}
  
void ASX_ITCH_1_1_Handler::flush_all_loggers()
{
    for (size_t i=0; i< m_asx_channels.size(); i++) {
	    if (m_asx_channels[i].get()->m_loggerRP){
		    m_asx_channels[i].get()->m_loggerRP.get()->sync_flush();
	    }
    }
}
void ASX_ITCH_1_1_Handler::update_channel_state()
{
	
    for (size_t i=0; i< m_channel_info_map.size(); i++) {
	    if (get_asx_channel( m_channel_info_map[i].context ))
		    continue;
	    boost::shared_ptr<ASX_ITCH_1_1_Handler::asx_channel>  asxCh(new asx_channel());
	    m_asx_channels.push_back(asxCh);
	    if (m_test_mode){
	    string logg_name = m_channel_log_base+
		    std::to_string(m_channel_info_map[i].context);
 m_logger->log_printf(Log::INFO, "create channel log %s ", logg_name.c_str() );
    m_asx_channels.back().get()->m_loggerRP=m_app->lm()->get_logger(logg_name);
	    }
	    else
	    m_asx_channels.back().get()->m_loggerRP=0;
    m_asx_channels.back().get()->m_channel_info=&m_channel_info_map[i];
	    m_channel_info_map[i].seq_no=1;
	    m_asx_channels.back().get()->udp_recovery_send_fd=m_channel_info_map[i].last_timestamp_ms;
	    m_channel_info_map[i].last_timestamp_ms=0;
	    m_asx_channels.back().get()->udp_recovery_read_fd=m_channel_info_map[i].last_msg_ms;
	    m_channel_info_map[i].last_msg_ms=0;
	    m_asx_channels.back().get()->snapshot_msg_received=0;
	    m_asx_channels.back().get()->m_channel_state.m_snapshot_timeout_time=Time(0);
	    
	    try_connect_snapshot(m_asx_channels.back().get());

    }
}

ASX_ITCH_1_1_Handler::asx_channel* ASX_ITCH_1_1_Handler::get_asx_channel(size_t channel_id)
{
    for (int i=0; i< m_asx_channels.size(); i++) {
      if (m_asx_channels[i].get()->m_channel_info->context != channel_id)
	continue;
      return m_asx_channels[i].get();
    }
    return 0;
}


channel_info* ASX_ITCH_1_1_Handler::get_channel_info(size_t channel_id)
{
	asx_channel* asxCh=get_asx_channel(channel_id);
	if (!asxCh) return 0;
    return asxCh->m_channel_info;
}

  //snapshot is started in ASX_ITCH_Session
  bool
ASX_ITCH_1_1_Handler::send_snapshot_login(asx_channel* asxCh)
{
    const char* where = __func__;
    //asx_channel* asxCh=get_asx_channel(channel_id);
    //if (!asxCh) return true;
    if (asxCh->m_channel_state.m_refresh_rejected)
	    return false;
    channel_info& wk_channel = *(asxCh->m_channel_info);

#ifdef CW_DEBUG
   cout << where << " Channel snapshot flage:" << (wk_channel.snapshot_is_on?"on":"off") << endl;
#endif
    if (wk_channel.snapshot_is_on) return true;
    if (!wk_channel.snapshot_socket.connected())
    {
      m_logger->log_printf(Log::WARN, "%s no connection", where );
	    
#ifdef CW_DEBUG
      cout << where << " snapshot_socket not connected" << endl;
#endif
      return false;
    }
 
    //int write_flag=0;//MSG_DONTWAIT;
      m_logger->log_printf(Log::INFO, "%s", where );
    int ret = ::send(wk_channel.snapshot_socket, &m_soup_login, sizeof(m_soup_login), MSG_DONTWAIT);
    
    if(ret < 0) {
      m_logger->log_printf(Log::WARN, "%s send got negative=%d", where, ret);
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert("%s %s sendto returned %d - %s", where, m_name.c_str(), errno, err_msg);
#ifdef CW_DEBUG
   printf("%s %s sendto returned %d - %s\n", where, m_name.c_str(), errno, err_msg);
#endif
      return false;
    }
#ifdef CW_DEBUG
    printf("channel %d %s sent %d bytes to socket %d\n",wk_channel.context, __func__, ret, wk_channel.snapshot_socket.fd);
#endif
    
    wk_channel.snapshot_is_on=true;
    asxCh->next_heartbeat_time=Time::current_time()+m_heartbeat_interval;
    //cout << where << " login send time " << Time::current_time();
    asxCh->m_channel_state.m_has_sent_refresh_request=true;
    asxCh->m_channel_state.m_in_refresh_state=true;
    return true;
}

  bool
ASX_ITCH_1_1_Handler::send_snapshot_login(size_t channel_id)
{
    const char* where = __func__;
    asx_channel* asxCh = get_asx_channel(channel_id);
    if (!asxCh) return false;
    //channel_info* wk_channel=*(asxCh->m_channel_info);

    return send_snapshot_login(asxCh);
}
  
  bool
  ASX_ITCH_1_1_Handler::send_snapshot_request(size_t channel_id)
  {
    return send_snapshot_login(channel_id);
  }

LoggerRP ASX_ITCH_1_1_Handler::G_logger=0;
void
ASX_ITCH_1_1_Handler::set_Glogger(LoggerRP& alg){
	G_logger=alg.get();
}

  void
  ASX_ITCH_1_1_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);
    m_name = name;
    
    //m_midnight = Time::today();
    //m_timestamp = Time::currentish_time();
    m_heartbeat_interval=msec(1000);
    //m_logger->log_printf(Log::INFO, "m_midnight=%s m_timestamp=%s", m_midnight.to_string().c_str(), m_timestamp.to_string().c_str());
    
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

    m_exch_char = 'a';
    
    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';
    
    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);
    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);
    
    m_new_auction_date = Time("20121119 00:00:00", "%Y%m%d %H:%M:%S");

    /* using channel
       if (params.has("retransmission_ip"))
       m_recovery_channel_info.name= params["retransmission_ip"].getString();
    */ 
    m_test_mode=false;
    uint16_t one=1;
    m_soup_logout.packet_length=htons(one);
    m_soup_logout.packet_type='O';
    m_soup_heartbeat.packet_length=htons(one);
    m_soup_heartbeat.packet_type='R';
    m_soup_debug.packet_type='+';
    memset(&m_soup_login, ' ', sizeof(asx_itch_1_1::SoupBinTcpLogin));
    memcpy(&m_soup_login.seqno, "1", 1);
    m_soup_login.packet_type='L';
    uint16_t logLen=47;
    m_soup_login.packet_length=htons(logLen);
    char* loggDir=getenv("PRODUCT_LOG_DIR");
    if (!loggDir) loggDir=getenv("PWD");
    if (loggDir) {
	    m_channel_log_base=loggDir;
    }
    else m_channel_log_base="ASX_LOG";
    if (params.has("channel_logger_base")){
	   m_channel_log_base=params["channel_logger_base"].getString(); 
    }
    if (params.has("create_channel_log")){
	   string tmp=params["create_channel_log"].getString(); 
	   if (tmp.length() > 0 && (tmp[0]=='Y' || tmp[0]=='y'))
		   m_test_mode=true;
    }
    if (!params.has("username")){
	    m_logger->log_printf(Log::ERROR, "No username!!!");
	    throw errno_exception("No username defined in config!!!", ENXIO);
    }
      string tmpS=params["username"].getString();
      memcpy(m_soup_login.username, tmpS.c_str(), tmpS.length());
    if (!params.has("password")){
	    m_logger->log_printf(Log::ERROR, "No password!!!");
	    throw errno_exception("No password defined in config!!!", ENXIO);
    }
      tmpS=params["password"].getString();
      memcpy(m_soup_login.password, tmpS.c_str(), tmpS.length()); 

    if (!m_recorder){
	    m_logger->log_printf(Log::ERROR, "No m_recorder!!!");
      throw errno_exception("No m_recorder!!!",  ENXIO);
    }
    cout << "m_recorder->" << params["record_file"].getString() << endl;
  }
  
  void ASX_ITCH_1_1_Handler::set_channel_ID(size_t context, uint16_t channel_ID, const string& channel_type_str)
  {
  }
  
  void ASX_ITCH_1_1_Handler::set_num_contexts(size_t nc) {
    //m_context_to_product_ids.resize(nc);
  }
  
  void 
  ASX_ITCH_1_1_Handler::set_up_channels(const Parameter& params)
  {
    //channel infor was built by ASX_ITCH_Session init_
  }
  
  void 
  ASX_ITCH_1_1_Handler::sync_udp_channel(string name, size_t context, int  recovery_socket)
  { 
      channel_info* pCh=Handler::get_channel_info(name, context);
      if (pCh && pCh->context==context && pCh->recovery_socket.fd < 1)
          pCh->recovery_socket.fd=recovery_socket;
  }
  
  void 
  ASX_ITCH_1_1_Handler::sync_tcp_channel(string name, size_t context, int  snapshot_socket)
  { 
          channel_info* pCh=Handler::get_channel_info(name, context);
          if (pCh && pCh->context==context && pCh->snapshot_socket.fd < 1)
          pCh->snapshot_socket.fd=snapshot_socket;
  }
  
  void
  ASX_ITCH_1_1_Handler::start()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }
  
  void
  ASX_ITCH_1_1_Handler::stop()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
    Handler::stop();
  }
  
  bool
  ASX_ITCH_1_1_Handler::send_retransmit_request(asx_channel* asxCh,  int64_t for_seqno, uint16_t need_total)
{
    string where=__func__;
	int fd=asxCh->udp_recovery_send_fd;
	channel_info& wk_channel=*(asxCh->m_channel_info);
#ifdef CW_DEBUG
	if (!m_test_mode) 
#endif
		fd=wk_channel.recovery_socket.fd;
    uint64_t from_seq_no = for_seqno;
    uint16_t message_count = need_total;
    
    send_alert((where+" from_seq_no=%zd, message_count=%d").c_str(), from_seq_no, (int) message_count);
    
    asx_itch_1_1::downstream_header_t req;
    
    memcpy(req.session, asxCh->session, sizeof(req.session));
    req.seq_num = htonll(from_seq_no);
    req.message_count = htons(message_count);
    
    int ret;
#ifdef CW_DEBUG
    if (m_test_mode) 
    {
    printf("%s fd=%d from_seq_no=%d, message_count=%d\n", __func__, fd, from_seq_no, need_total);
    size_t z=sizeof(req);
    char tmpBuf[z+4];
    	memcpy(tmpBuf, &req, z);
	memset(tmpBuf+z, 0x01, 3);
	    ret = ::write(fd, tmpBuf, z+3);
    } else 
#endif
    ret = sendto(fd, &req, sizeof(req), 0, (struct sockaddr *)&(wk_channel.recovery_addr), sizeof(wk_channel.recovery_addr));
    bool result=true;
    if(ret <= 0) {
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert((where+" %s sendto returned %d - %s").c_str(), m_name.c_str(), errno, err_msg);
      result=false;
    }
    return result;
}
  
  bool
  ASX_ITCH_1_1_Handler::send_retransmit_request(channel_info& wk_channel,  int64_t for_seqno, uint16_t need_total)
{
    string where=__func__;
    uint64_t from_seq_no = for_seqno;
    uint16_t message_count = need_total;
    
    send_alert((where+" from_seq_no=%zd, message_count=%d").c_str(), from_seq_no, (int) message_count);
    
    asx_itch_1_1::downstream_header_t req;
    
    memcpy(req.session, m_tcp_session, sizeof(m_tcp_session));
    req.seq_num = htonll(from_seq_no);
    req.message_count = htons(message_count);
    
    int ret;
#ifdef CW_DEBUG
    if (m_test_mode) 
    {
    printf("%s ch %d from_seq_no=%d, message_count=%d", __func__, wk_channel.context, from_seq_no, need_total);
    size_t z=sizeof(req);
    char tmpBuf[z+4];
    	memcpy(tmpBuf, &req, z);
	memset(tmpBuf+z, 0x01, 3);
	    ret = ::write(wk_channel.recovery_socket, tmpBuf, z+3);
	    //char udpMB[4];
	    //memset(udpMB, 0x01, 3);//UDP_BOUNDARY, 3);
	    //::write(wk_channel->recovery_socket, udpMB, 3);
    } else 
#endif
    ret = sendto(wk_channel.recovery_socket, &req, sizeof(req), 0, (struct sockaddr *)&(wk_channel.recovery_addr), sizeof(wk_channel.recovery_addr));
    bool result=true;
    if(ret <= 0) {
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert((where+" %s sendto returned %d - %s").c_str(), m_name.c_str(), errno, err_msg);
      result=false;
    }
    return result;
}
  
  void
  ASX_ITCH_1_1_Handler::send_retransmit_request(channel_info& wk_channel)
{
    string where=__func__;
    if(wk_channel.recovery_socket == -1) {
      return ;//false;
    }
    
    uint64_t from_seq_no = wk_channel.seq_no;
    uint16_t message_count = wk_channel.queue.top().seq_num - from_seq_no;

    send_alert((where+" from_seq_no=%zd, message_count=%d").c_str(), from_seq_no, (int) message_count);

    asx_itch_1_1::downstream_header_t req;

    memcpy(req.session, m_tcp_session, sizeof(m_tcp_session));
    req.seq_num = htonll(from_seq_no);
    req.message_count = htons(message_count);
    socket_fd m_retransmission_fd=wk_channel.recovery_socket;

    //int ret = send(m_retransmission_socket, &req, sizeof(req), MSG_DONTWAIT);
    int ret ;
#ifdef CW_DEBUG
    if (m_test_mode) 
    {
    printf("%s ch %d from_seq_no=%d, message_count=%d", __func__, wk_channel.context, from_seq_no, message_count);
	    ret = ::write(wk_channel.recovery_socket, &req, sizeof(req));
	    char udpMB[4];
	    memset(udpMB, 0x01, 3);//UDP_BOUNDARY, 3);
	    ::write(wk_channel.recovery_socket, udpMB, 3);
    } else 
#endif
     ret= sendto(wk_channel.recovery_socket, &req, sizeof(req), 0, 
		     (struct sockaddr *)&wk_channel.recovery_addr, 
		     sizeof(wk_channel.recovery_addr));
    
    bool result=true;
    if(ret < 0) {
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert((where+" %s sendto returned %d - %s").c_str(), m_name.c_str(), errno, err_msg);
    	result=false;
    }
    //return result;
}
  
  void
  ASX_ITCH_1_1_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
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

  //void ASX_ITCH_1_1_Handler::SetTcpHeartbeatResponseServiceId(uint16_t channel_ID) { m_tcp_heartbeat_response_channel_ID = channel_ID; }
  
  void ASX_ITCH_1_1_Handler::SetTcpContexts(size_t refresh_tcp_context, size_t retransmission_tcp_context) { 
    m_refresh_tcp_context = refresh_tcp_context; 
    m_retransmission_tcp_context = retransmission_tcp_context; 
  }
  void ASX_ITCH_1_1_Handler::add_context_product_id(size_t context, ProductID id) { 
    if(m_context_to_product_ids.size() <= context) { 
      m_context_to_product_ids.resize(context+1); 
    } 
    m_context_to_product_ids[context].push_back(id); 
  }
  
  
  ASX_ITCH_1_1_Handler::ASX_ITCH_1_1_Handler(Application* app) :
    Handler(app, "ASX_ITCH_1_1"),
    m_prod_map(262000),
    m_micros_exch_time_marker(0),
    m_test_mode(false),
    m_hist_mode(false),
    m_soup_login(),
    m_soup_logout(),
    m_soup_heartbeat(),
    m_soup_debug()
  {
    //m_symbol_info_map.resize(4194304); //m_symbol_info_map.resize(65536);
    m_match_id_lookup.resize(65536, set<asx_match_id>());
  }
  
  ASX_ITCH_1_1_Handler::~ASX_ITCH_1_1_Handler()
  {
  }
  
}
