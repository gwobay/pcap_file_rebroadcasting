#include <Data/TSE_FLEX_Handler.h>
#include <Data/QBDF_Util.h>

#include <MD/MDProvider.h>

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  const uint64_t MARKET_BID = 99999999900000000ULL;

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

  static
  uint64_t
  parse_time(const char* buf, int len)
  {
    size_t time_int = parse_number(buf, len);

    uint64_t micros = 0, sec = 0, min = 0, hr = 0;
    if (len == 12) {
      micros = (time_int % 1000000);
      sec = (time_int % 100000000) / 1000000;
      min = (time_int % 10000000000) / 100000000;
      hr = (time_int / 10000000000UL);
    } else if (len == 6) {
      sec = (time_int % 100);
      min = (time_int % 10000) / 100;
      hr = (time_int / 10000);
    } else {
      micros = (time_int % 1000) * 1000;
      sec = (time_int % 100000) / 1000;
      min = (time_int % 10000000) / 100000;
      hr = (time_int / 10000000);
    }
    
    return (micros + (sec*1000000) + (min*60000000) + (hr*3600000000));
  }
  
  size_t
  TSE_FLEX_Handler::parse(size_t context, const char* buf, size_t len)
  {
    if(len == 1) {
      return len;
    }
    if(len < 40) {
      m_logger->log_printf(Log::ERROR, "TSE_FLEX_Handler::parse: context %zd  message len %zd smaller than service-header size", context, len);
      return 0;
    }

    if(*buf != 0x11) {
      m_logger->log_printf(Log::ERROR, "TSE_FLEX_Handler::parse: context %zd invalid start character", context);
      return 0;
    }

    if (m_qbdf_builder && !m_use_exch_time) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
    }

    size_t msg_len = parse_number(buf + 1, 6);
    size_t seq_no  = parse_number(buf + 7, 11);
    size_t next_seq_no = seq_no + 1;

    if(msg_len != len) {
      m_logger->log_printf(Log::ERROR, "TSE_FLEX_Handler::parse: context %zd len=%zd and msg_len=%zd disagree", context, len, msg_len);
      return 0;
    }

    channel_info& ci = m_channel_info_map[context];

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

    parse2(context, buf, len);

    // Use m_use_exch_time as a proxy for historical mode
    // hist does not have multiple channels, so simply increment
    if (m_use_exch_time) {
      ++ci.seq_no;
    } else {
      ci.seq_no = next_seq_no;
    }
    ci.last_update = Time::currentish_time();

    if(!ci.queue.empty()) {
      ci.process_recovery(this);
    }

    return len;
  }

  size_t
  TSE_FLEX_Handler::parse2(size_t context, const char* buf, size_t len)
  {
    const char* where = "TSE_FLEX_Handler::parse2";

    if (m_qbdf_builder) {
      if (m_micros_midnight == 0) {
        m_micros_midnight = Time::today().ticks() / Time_Constants::ticks_per_micro;
        m_logger->log_printf(Log::INFO, "Using date %s", Time::today().strftime("%Y%m%d").c_str());
        if (Time::today().strftime("%Y%m%d") >= "20111121") {
          m_logger->log_printf(Log::INFO, "Using 11:30am close for morning session");
          m_micros_morning_close = 41400000000UL; // 11:30
        } else {
          m_micros_morning_close = 39600000000UL; // 11:00
          m_logger->log_printf(Log::INFO, "Using 11:00am close for morning session");
        }
        m_micros_afternoon_close = 54000000000UL; // 15:00
      }
    }

    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
    }

    if(m_record_only) {
      return len;
    }

    bool inside_changed = false;
    bool auction_inside_changed = false;
    bool last_in_group = true;
    bool special_quote = false;

    Time t_exch_time;

    buf += 18;  // Now we start skipping through the buffer since we've already determined it is valid

    //size_t msg_type = parse_number(buf, 3);
    buf += 3;
    char   exch_code = *buf; ++buf;
    if (exch_code != '1') {
      return len;
    }

    //size_t session = parse_number(buf, 2);
    buf += 2;

    //size_t issue_class = parse_number(buf, 4);
    if(buf[0] != '0' || buf[1] != '1') {
      // 01 is the issue classification code for stock
      return len;
    }
    buf += 4;

    buf += 7; // skip over spaces in stocks
    size_t stock_name = parse_number(buf, 5);
    ProductID id = product_id_of_symbol(stock_name);

    if(id == Product::invalid_product_id) {
      return len;
    }

    if ((size_t)id >= m_current_price.size()) {
      m_current_price.resize((size_t)id+1, MDPrice());
    }

    if ((size_t)id >= m_trading_volume.size()) {
      m_trading_volume.resize((size_t)id+1, 0);
    }

    if ((size_t)id >= m_morning_close_data.size()) {
      m_morning_close_data.resize((size_t)id+1, TradeState());
    }

    if ((size_t)id >= m_afternoon_close_data.size()) {
      m_afternoon_close_data.resize((size_t)id+1, TradeState());
    }

    if ((size_t)id >= m_special_quote_data.size()) {
      m_special_quote_data.resize((size_t)id+1, TradeState());
    }

    buf += 4;
    buf += 1; // reserved code

    if(*buf != 0x12) {
      m_logger->log_printf(Log::ERROR, "TSE_FLEX_Handler::parse: context %zd invalid header separation character", context);
      return 0;
    }

    int scan_posn = 0;
    int quotes_left = 0;
    int close_quotes_left = 0;
    while(buf[scan_posn] != 0x11) {
      ++scan_posn;  // skip control character

      if (buf[scan_posn] == 'Q')
        ++quotes_left;
      else if ((buf[scan_posn] == 'B' || (buf[scan_posn] == 'S')) && (buf[scan_posn+1] == 'C'))
        ++close_quotes_left;


      while(buf[scan_posn] != 0x13 && buf[scan_posn] != 0x11) { ++scan_posn; }
    }

    while(*buf != 0x11) {
      ++buf;  // skip control character

      // get user data message length
      size_t user_data_msg_len = 0;
      while(buf[user_data_msg_len] != 0x13 && buf[user_data_msg_len] != 0x11) { ++user_data_msg_len; }

      if(buf[0] == 'N' && buf[1] == 'O') { // NO
        size_t no = parse_number(buf+2, 8);
        size_t div_num = parse_number(buf+10, 5);
        size_t div_tot = parse_number(buf+15, 5);

        if (div_num != div_tot)
          last_in_group = false;

        if(m_quote_dump) {
          m_quote_dump->printf("%zd,NO,%zd,%zd,%zd\n", stock_name, no, div_num, div_tot);
        }
      } else if(buf[0] == 'B' && buf[1] == 'P') { // BP (TSE Flex Standard only)
	uint8_t tick_size_table;
	// New format effective 20150924 uses 68 bytes
	if (user_data_msg_len == 68) {
	  tick_size_table = parse_number(buf+61, 2);
	} else if (user_data_msg_len == 61) {
	  tick_size_table = parse_number(buf+2, 2);
	} else {
	  // This should not happen
#ifdef CW_DEBUG
	  cout << "BP user_data_msg_len=" << user_data_msg_len << endl;
#endif
	  return len;
	}

	string applicable_date = string(buf+4, 8);
	MDPrice base_price = MDPrice::from_iprice64(parse_number(buf+13, 14) * 10000UL);
	MDPrice limit_max_price = MDPrice::from_iprice64(parse_number(buf+29, 14) * 10000UL);
	MDPrice limit_min_price = MDPrice::from_iprice64(parse_number(buf+45, 14) * 10000UL);

	m_bp_data.push_back(bp_data_struct(id, stock_name, applicable_date, tick_size_table,
					   base_price, limit_max_price, limit_min_price));
      } else if(buf[0] == 'I' && buf[1] == 'I') { // II (TSE Flex Standard only)
	string applicable_date = string(buf+10, 8);
	uint32_t trade_unit = parse_number(buf+70, 14);

	m_ii_data.push_back(ii_data_struct(id, stock_name, applicable_date, trade_unit));
    
      } else if (buf[0] == 'S' && buf[1] == 'T') { // ST
	m_micros_exch_time = parse_time(buf+10, 12);
        if (m_use_exch_time) {
          m_micros_recv_time = m_micros_exch_time;
        }
        m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
        t_exch_time = Time::today() + usec(m_micros_exch_time);

        m_msu.m_id = id;

        bool send_update = true;
        QBDFStatusType qbdf_status;

        string state;
        if(buf[7] == 'A') {
          if(buf[8] == '0') {
            state = "halt";
            m_msu.m_market_status = MarketStatus::Halted;
            qbdf_status = QBDFStatusType::Halted;
          } else if(buf[8] == '1') {
            state = "resume";
            m_msu.m_market_status = MarketStatus::Open;
            qbdf_status = QBDFStatusType::Resume;
          }
        } else if(buf[7] == 'B') {
          state = "Itayose";
          m_msu.m_market_status = MarketStatus::Auction;
          qbdf_status = QBDFStatusType::Auction;
        } else if(buf[7] == 'C') {
          if(buf[8] == '0') {
            state = "suspension";
            m_msu.m_market_status = MarketStatus::Suspended;
            qbdf_status = QBDFStatusType::Suspended;
          } else if(buf[8] == '1') {
            state = "release of suspension";
            m_msu.m_market_status = MarketStatus::Open;
            qbdf_status = QBDFStatusType::Open;
          }
        } else if(buf[7] == 'D') {
          state = "Trading halt(not accepting order input)";
          m_msu.m_market_status = MarketStatus::Halted;
          qbdf_status = QBDFStatusType::Halted;
        } else {
          send_update = false;
        }

        if(send_update) {
          m_security_status[m_msu.m_id] = m_msu.m_market_status;

          if(m_qbdf_builder) {
            gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                                   'T', qbdf_status.index(), state);
          }
          if(m_subscribers) {
            m_subscribers->update_subscribers(m_msu);
          }
        }

        send_update = false;
        if (buf[4] == '1') {
          const char* action;
          QBDFStatusType qbdf_status;
          if (unlikely(m_micros_recv_time == 0))
              m_micros_recv_time = 28800000000UL; // 8am == 8 * 60 * 60 * 1e6 (h * m * s * microseconds)
          if (buf[9] == '1') {
            action = "SSR Activated";
            m_msu.m_ssr = SSRestriction::Activated;
            qbdf_status = QBDFStatusType::ShortSaleRestrictActivated;
            send_update = true;
          } else if (buf[9] == '0') {
            action = "SSR Deactivated";
            m_msu.m_ssr = SSRestriction::None;
            qbdf_status = QBDFStatusType::NoShortSaleRestrictionInEffect;
            send_update = true;
          }

          if(send_update) {
            string qbdf_status_reason = "ST TSE SSR Action: " + string(action);
            if (m_qbdf_builder) {
              gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                                     'T', qbdf_status.index(), qbdf_status_reason);
            }

            send_alert(AlertSeverity::LOW, "TSE SSR symbol=%zd %s", stock_name, qbdf_status_reason.c_str());

            m_msu.m_market_status = m_security_status[id];
            if(m_subscribers) {
              m_subscribers->update_subscribers(m_msu);
            }
            m_msu.m_ssr=SSRestriction::Unknown;
          }
        }

	
      } else if (buf[0] == 'Q' && buf[1] == '1') { // Q1 (TSE Flex Standard only)
        if(id != Product::invalid_product_id) {
	    m_micros_exch_time = parse_time(buf+21, 6);
	    if (m_use_exch_time) {
	      m_micros_recv_time = m_micros_exch_time;
	    }
	    m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	    t_exch_time = Time::today() + usec(m_micros_exch_time);
	    
	    MDPrice bid_price  = MDPrice::from_iprice64(parse_number(buf+46, 14) * 10000UL);
	    MDPrice ask_price  = MDPrice::from_iprice64(parse_number(buf+6, 14) * 10000UL);
	    
	    char bid_quote_flag = buf[67];
	    char ask_quote_flag = buf[27];

	    // silly logic to try and keep most important info from 2 char flags in a single char condition
	    char quote_cond;
	    if (bid_quote_flag == 'C' || ask_quote_flag == 'C') {
	      quote_cond = 'C';
	    } else if (bid_quote_flag > ask_quote_flag) {
	      quote_cond = bid_quote_flag;
	    } else {
	      quote_cond = ask_quote_flag;
	    }

	    //char bid_turnover_flag = buf[68];
	    //char ask_turnover_flag = buf[28];
	    
	    size_t bid_shares = parse_number(buf+69, 14);
	    size_t ask_shares = parse_number(buf+29, 14);
	    
	    if(m_qbdf_builder) {
	      gen_qbdf_quote_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
				    'T', quote_cond, bid_price, ask_price, bid_shares, ask_shares, false);
	    }

	    m_quote.set_time(t_exch_time);
	    m_quote.m_id = id;
	    m_quote.m_bid = bid_price.fprice();
	    m_quote.m_ask = ask_price.fprice();
	    m_quote.m_bid_size = bid_shares;
	    m_quote.m_ask_size = ask_shares;
	    m_quote.m_cond = quote_cond;

          if(m_subscribers) {
            m_msu.m_market_status = m_security_status[m_quote.m_id];
            m_feed_rules->apply_quote_conditions(m_quote, m_msu);
            if(m_msu.m_market_status != m_security_status[m_quote.m_id]) {
              m_msu.m_id = m_quote.m_id;
              m_security_status[m_quote.m_id] = m_msu.m_market_status;
              m_subscribers->update_subscribers(m_msu);
            }
            if(m_provider) {
              m_provider->check_quote(m_quote);
            }
            update_subscribers(m_quote);
          }
	}
      } else if (buf[0] == 'Q' && (buf[1] == 'S' || buf[1] == 'B')) { // QS and QB (TSE Flex Full only)
        if(m_quote_dump) {
          m_quote_dump->printf("%zd,%.2s,%.2s,%.1s,%.1s,%.14s,%.1s,%.9s,%.1s,%.1s,%.1s,%.14s,%.1s,%.1s,%.14s,%.1s,%.1s\n",
                               stock_name, buf+0, buf+2, buf+4, buf+5, buf+6, buf+20, buf+21, buf+30, buf+31, buf+32,
                               buf+33, buf+47, buf+48, buf+49, buf+63, buf+64);
        }

	// New format effective 20150924 uses 68 bytes
	if (user_data_msg_len == 68) {
	  m_micros_exch_time = parse_time(buf+21, 12);
	} else if (user_data_msg_len == 65) {
	  m_micros_exch_time = parse_time(buf+21, 9);
	} else {
	  // This should not happen
#ifdef CW_DEBUG
	  cout << "QS/QB user_data_msg_len=" << user_data_msg_len << endl;
#endif
	  return len;
	}

        if (m_use_exch_time) {
          m_micros_recv_time = m_micros_exch_time;
        }
        m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
        t_exch_time = Time::today() + usec(m_micros_exch_time);

        char side = buf[1];
        //char unit_flag = buf[5];

        MDPrice price = MDPrice::from_iprice64(parse_number(buf+6, 14) * 10000UL);
        if(price.blank() && side == 'B') {
          price.set_iprice64(MARKET_BID);
        }

	int new_format_offset = 0;
	if (user_data_msg_len == 68) {
	  new_format_offset = 3;
	}

        char quote_flag = buf[30+new_format_offset];
        //char matching_sign = buf[31+new_format_offset];
        char turnover_flag = buf[32+new_format_offset];
        char sign = buf[47+new_format_offset];
        size_t shares = parse_number(buf+33+new_format_offset, 14);
        size_t orders = parse_number(buf+49+new_format_offset, 14);

        //char middle_of_book = buf[64+new_format_offset];
        bool last_quote_in_packet = (quotes_left == 1);

        --quotes_left;

        if(id != Product::invalid_product_id) {
          if(m_qbdf_builder) {
            gen_qbdf_level_iprice64(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                                    'T', side, turnover_flag, quote_flag, sign, orders, shares, price,
                                    last_in_group && last_quote_in_packet, false);

            if (quote_flag == '3') {
              m_special_quote_data[id].id = stock_name;
              m_special_quote_data[id].exch = 'T';
              m_special_quote_data[id].recv_time = m_micros_recv_time;
              m_special_quote_data[id].exch_time = m_micros_exch_time;
              m_special_quote_data[id].side = side;
              m_special_quote_data[id].size = shares;
              m_special_quote_data[id].price = price;
            }
          }

          if(m_order_book) {
            if(quote_flag == '3') {
              special_quote = true;
              inside_changed = true;
            }

            if(shares != 0) {
              inside_changed |= m_order_book->modify_level_ip(t_exch_time, id, side, price, shares, orders, false);
            } else {
              inside_changed |= m_order_book->remove_level_ip(t_exch_time, id, side, price, false);
            }
          }
        }
      } else if (buf[0] == '4' && buf[1] == 'P') { // 4P (TSE Flex Standard only)
	m_micros_exch_time = parse_time(buf+91, 6);
        if (m_use_exch_time) {
          m_micros_recv_time = m_micros_exch_time;
        }
        m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
        t_exch_time = Time::today() + usec(m_micros_exch_time);

        MDPrice price = MDPrice::from_iprice64(parse_number(buf+76, 14) * 10000UL);
        m_current_price[id] = price;

	// pre-20150924 message length = 101, contained SSR/Halt info
	if (user_data_msg_len == 101) {
	  m_msu.m_id = id;
	  
	  bool send_update = true;
	  QBDFStatusType qbdf_status;
	  
	  string state;
	  if(buf[98] == 'A') {
	    if(buf[99] == '0') {
	      state = "halt";
	      m_msu.m_market_status = MarketStatus::Halted;
	      qbdf_status = QBDFStatusType::Halted;
	    } else if(buf[99] == '1') {
	      state = "resume";
	      m_msu.m_market_status = MarketStatus::Open;
	      qbdf_status = QBDFStatusType::Resume;
	    }
	  } else if(buf[98] == 'B') {
	    state = "Itayose";
	    m_msu.m_market_status = MarketStatus::Auction;
	    qbdf_status = QBDFStatusType::Auction;
	  } else if(buf[98] == 'C') {
	    if(buf[99] == '0') {
	      state = "suspension";
	      m_msu.m_market_status = MarketStatus::Suspended;
	      qbdf_status = QBDFStatusType::Suspended;
	    } else if(buf[99] == '1') {
	      state = "release of suspension";
	      m_msu.m_market_status = MarketStatus::Open;
	      qbdf_status = QBDFStatusType::Open;
	    }
	  } else if(buf[98] == 'D') {
	    state = "Trading halt(not accepting order input)";
	    m_msu.m_market_status = MarketStatus::Halted;
	    qbdf_status = QBDFStatusType::Halted;
	  } else {
	    send_update = false;
	  }
	  
	  if(send_update) {
	    m_security_status[m_msu.m_id] = m_msu.m_market_status;
	    
	    if(m_qbdf_builder) {
	      gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
				     'T', qbdf_status.index(), state);
	    }
	    if(m_subscribers) {
	      m_subscribers->update_subscribers(m_msu);
	    }
	  }
	  
	  send_update = false;
	  if (buf[3] == '1') {
	    const char* action;
	    QBDFStatusType qbdf_status;
	    if (buf[2] == '1') {
	      action = "SSR Activated";
	      m_msu.m_ssr = SSRestriction::Activated;
	      qbdf_status = QBDFStatusType::ShortSaleRestrictActivated;
	      send_update = true;
	    } else if (buf[2] == '0') {
	      action = "SSR Deactivated";
	      m_msu.m_ssr = SSRestriction::None;
	      qbdf_status = QBDFStatusType::NoShortSaleRestrictionInEffect;
	      send_update = true;
	    }
	    
	    if(send_update) {
	      string qbdf_status_reason = "4P TSE SSR Action: " + string(action);
	      if (m_qbdf_builder) {
		gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
				       'T', qbdf_status.index(), qbdf_status_reason);
	      }
	      
	      send_alert(AlertSeverity::LOW, "TSE SSR symbol=%zd %s", stock_name, qbdf_status_reason.c_str());
	      
	      m_msu.m_market_status = m_security_status[id];
	      if(m_subscribers) {
		m_subscribers->update_subscribers(m_msu);
	      }
	      m_msu.m_ssr=SSRestriction::Unknown;
	    }
	  }
	}

      } else if(buf[0] == '1' && buf[1] == 'P') {  // 1P

	// New format effective 20150924 uses 36 bytes
	if (user_data_msg_len == 36) {
	  m_micros_exch_time = parse_time(buf+20, 12);
	} else if (user_data_msg_len == 33) {
	  m_micros_exch_time = parse_time(buf+20, 9);
	} else {
	  // This should not happen
#ifdef CW_DEBUG
	  cout << "1P user_data_msg_len=" << user_data_msg_len << endl;
#endif
	  return len;
	}

	if (m_use_exch_time) {
	  m_micros_recv_time = m_micros_exch_time;
	}
	m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
	t_exch_time = Time::today() + usec(m_micros_exch_time);

        MDPrice price = MDPrice::from_iprice64(parse_number(buf+5, 14) * 10000UL);
        m_current_price[id] = price;

	// pre-20150924 message length = 33, contained SSR/Halt info
	if (user_data_msg_len == 33) {
	  m_msu.m_id = id;

	  bool send_update = true;
	  QBDFStatusType qbdf_status;

	  string state;
	  if(buf[30] == 'A') {
	    if(buf[31] == '0') {
	      state = "halt";
	      m_msu.m_market_status = MarketStatus::Halted;
	      qbdf_status = QBDFStatusType::Halted;
	    } else if(buf[31] == '1') {
	      state = "resume";
	      m_msu.m_market_status = MarketStatus::Open;
	      qbdf_status = QBDFStatusType::Resume;
	    }
	  } else if(buf[30] == 'B') {
	    state = "Itayose";
	    m_msu.m_market_status = MarketStatus::Auction;
	    qbdf_status = QBDFStatusType::Auction;
	  } else if(buf[30] == 'C') {
	    if(buf[31] == '0') {
	      state = "suspension";
	      m_msu.m_market_status = MarketStatus::Suspended;
	      qbdf_status = QBDFStatusType::Suspended;
	    } else if(buf[31] == '1') {
	      state = "release of suspension";
	      m_msu.m_market_status = MarketStatus::Open;
	      qbdf_status = QBDFStatusType::Open;
	    }
	  } else if(buf[30] == 'D') {
	    state = "Trading halt(not accepting order input)";
	    m_msu.m_market_status = MarketStatus::Halted;
	    qbdf_status = QBDFStatusType::Halted;
	  } else {
	    send_update = false;
	  }

	  if(send_update) {
	    m_security_status[m_msu.m_id] = m_msu.m_market_status;
	    
	    if(m_qbdf_builder) {
	      gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
				     'T', qbdf_status.index(), state);
	    }
	    if(m_subscribers) {
	      m_subscribers->update_subscribers(m_msu);
	    }
	  }

	  send_update = false;
	  if (buf[3] == '1') {
	    const char* action;
	    QBDFStatusType qbdf_status;
	    if (unlikely(m_micros_recv_time == 0))
              m_micros_recv_time = 28800000000UL; // 8am == 8 * 60 * 60 * 1e6 (h * m * s * microseconds)
	    if (buf[2] == '1') {
	      action = "SSR Activated";
	      m_msu.m_ssr = SSRestriction::Activated;
	      qbdf_status = QBDFStatusType::ShortSaleRestrictActivated;
	      send_update = true;
	    } else if (buf[2] == '0') {
	      action = "SSR Deactivated";
	      m_msu.m_ssr = SSRestriction::None;
	      qbdf_status = QBDFStatusType::NoShortSaleRestrictionInEffect;
	      send_update = true;
	    }

	    if(send_update) {
	      string qbdf_status_reason = "1P TSE SSR Action: " + string(action);
	      if (m_qbdf_builder) {
		gen_qbdf_status_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
				       'T', qbdf_status.index(), qbdf_status_reason);
	      }
	      
	      send_alert(AlertSeverity::LOW, "TSE SSR symbol=%zd %s", stock_name, qbdf_status_reason.c_str());
	      
	      m_msu.m_market_status = m_security_status[id];
	      if(m_subscribers) {
		m_subscribers->update_subscribers(m_msu);
	      }
	      m_msu.m_ssr=SSRestriction::Unknown;
	    }
	  }
	}

      } else if(buf[0] == 'V' && buf[1] == 'L') {  // VL
	if (m_is_standard_feed) {
	  m_micros_exch_time = parse_time(buf+20, 6);
	} else {
	  // New format effective 20150924 uses 33 bytes
	  if (user_data_msg_len == 33) {
	    m_micros_exch_time = parse_time(buf+20, 12);
	  } else if (user_data_msg_len == 30) {
	    m_micros_exch_time = parse_time(buf+20, 9);
	  } else {
	    // This should not happen
#ifdef CW_DEBUG
	    cout << "VL user_data_msg_len=" << user_data_msg_len << endl;
#endif
	    return len;
	  }
	}

        if (m_use_exch_time) {
          m_micros_recv_time = m_micros_exch_time;
        }
        m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
        t_exch_time = Time::today() + usec(m_micros_exch_time);

        //char unit = buf[5];
        size_t volume = parse_number(buf+6, 14);

        //Time_Utils::print_datetime(date_buf, Time(m_micros_recv_time * Time_Constants::ticks_per_micro), Timestamp::MILI);
        //m_logger->log_printf(Log::DEBUG, "   trading_volume %zd sym:%zd dt:%s", volume, stock_name, date_buf);
        size_t last_volume = m_trading_volume[id];
	if (last_volume > volume) {
	  m_logger->log_printf(Log::WARN, "Cumulative volume < previous volume; sym:%zd new_vol:%lu prev_vol:%lu",
			       stock_name, volume, last_volume);
	  continue;
	}

        size_t trade_size = volume - last_volume;

        if (trade_size > 0 && m_current_price[id].iprice64() > 0) {
          if(m_qbdf_builder) {
            gen_qbdf_trade_micros(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                                  'T', ' ', '0', " ", trade_size, m_current_price[id], false);

            // mark special quote price to 0 to show it is no longer effective
            m_special_quote_data[id].price = MDPrice();

            if (m_micros_exch_time <= m_micros_morning_close) {
              m_morning_close_data[id].id = id;
              m_morning_close_data[id].exch = 'T';
              m_morning_close_data[id].recv_time = m_micros_recv_time;
              m_morning_close_data[id].exch_time = m_micros_exch_time;
              m_morning_close_data[id].size = trade_size;
              m_morning_close_data[id].price = m_current_price[id];
            } else if (m_micros_exch_time <= m_micros_afternoon_close) {
              m_afternoon_close_data[id].id = id;
              m_afternoon_close_data[id].exch = 'T';
              m_afternoon_close_data[id].recv_time = m_micros_recv_time;
              m_afternoon_close_data[id].exch_time = m_micros_exch_time;
              m_afternoon_close_data[id].size = trade_size;
              m_afternoon_close_data[id].price = m_current_price[id];
            }
          }

          m_trade.set_time(t_exch_time);
          m_trade.m_id = id;
          m_trade.m_price = m_current_price[id].fprice();
          m_trade.m_size = trade_size;
          m_trade.set_side(0);
          memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
          m_trade.m_correct = '\0';

          if(m_subscribers) {
            m_msu.m_market_status = m_security_status[m_trade.m_id];
            m_feed_rules->apply_trade_conditions(m_trade, m_msu);
            if(m_msu.m_market_status != m_security_status[m_trade.m_id]) {
              m_msu.m_id = m_trade.m_id;
              m_security_status[m_trade.m_id] = m_msu.m_market_status;
              m_subscribers->update_subscribers(m_msu);
            }
            if(m_provider) {
              m_provider->check_trade(m_trade);
            }
            update_subscribers(m_trade);
          }
        }
        m_trading_volume[id] = volume;
      } else if(buf[0] == 'V' && buf[1] == 'A') {  // VA
        //char unit = buf[5];
        //size_t turnover = parse_number(buf+6, 14);
        //Time_Utils::print_datetime(date_buf, Time(m_micros_recv_time * Time_Constants::ticks_per_micro), Timestamp::MILI);
        //m_logger->log_printf(Log::DEBUG, "   turnover %zd sym:%zd dt:%s", turnover, stock_name, date_buf);

      } else if(buf[1] == 'C' && (buf[0] == 'B' || buf[0] == 'S')) {  // SC
	// New format effective 20150924 uses 66 bytes
	if (user_data_msg_len == 66) {
	  m_micros_exch_time = parse_time(buf+21, 12);
	} else if (user_data_msg_len == 63) {
	  m_micros_exch_time = parse_time(buf+21, 9);
	} else {
	  // This should not happen
#ifdef CW_DEBUG
	  cout << "BC/SC user_data_msg_len=" << user_data_msg_len << endl;
#endif
	  return len;
	}

        if (m_use_exch_time) {
          m_micros_recv_time = m_micros_exch_time;
        }
        m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
        t_exch_time = Time::today() + usec(m_micros_exch_time);

        char side = buf[0];
        //char unit_flag = buf[5];

        MDPrice price = MDPrice::from_iprice64(parse_number(buf+6, 14) * 10000UL);
        if(price.blank() && side == 'B') {
          price.set_iprice64(MARKET_BID);
        }

	int new_format_offset = 0;
	if (user_data_msg_len == 66) {
	  new_format_offset = 3;
	}

        char turnover_flag = buf[31+new_format_offset];
        char sign = buf[46+new_format_offset];
        size_t shares = parse_number(buf+32+new_format_offset, 14);
        size_t orders = parse_number(buf+48+new_format_offset, 14);

        bool last_quote_in_packet = (close_quotes_left == 1);
        --close_quotes_left;

        char quote_flag = 'C';

        if(id != Product::invalid_product_id) {
          if(m_qbdf_builder) {
            gen_qbdf_level_iprice64(m_qbdf_builder, id, m_micros_recv_time, m_micros_exch_time_delta,
                                    'T', side, turnover_flag, quote_flag, sign, orders, shares, price,
                                    last_in_group && last_quote_in_packet, false);
          }

          if(m_auction_order_book) {
            if(shares != 0) {
              auction_inside_changed |= m_auction_order_book->modify_level_ip(t_exch_time, id, side, price, shares, orders, false);
            } else {
              auction_inside_changed |= m_auction_order_book->remove_level_ip(t_exch_time, id, side, price, false);
            }
          }
        }

      }

      buf += user_data_msg_len; // SKIP to next
    }

    if(m_subscribers && id != Product::invalid_product_id && last_in_group && inside_changed) {
      m_quote.set_time(t_exch_time);
      m_quote.m_id = id;
      m_order_book->fill_quote_update(id, m_quote);
      m_quote.m_cond = special_quote ? '3' : '1';
      
      m_msu.m_market_status = m_security_status[id];

      m_feed_rules->apply_quote_conditions(m_quote, m_msu);

      if(m_msu.m_market_status != m_security_status[m_quote.m_id]) {
        m_msu.m_id = m_quote.m_id;
        m_msu.m_exch = m_quote.m_exch;
        m_security_status[id] = m_msu.m_market_status;
        m_subscribers->update_subscribers(m_msu);
      }
      if(m_provider) {
        m_provider->check_quote(m_quote);
      }

      update_subscribers(m_quote);
    }
    
    if (m_qbdf_builder) {
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
  TSE_FLEX_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t packet_len)
  {
    // observed lengths of TSE messages top out in the 1300 byte range (e.g., for 2014-11-12).
    char fmt_str[32];
    // -2 skips the beginning & trailing 0x11
    // -6 skips the 6-byte length string (we already printed it as part of the qrec header)
    snprintf(fmt_str, 32, "%%.%lus\n", packet_len - 2 - 6);
    log.printf(fmt_str, buf+7);
    return packet_len;

    // the code below is the original implementation of the dump() method for this feed. Comment
    // out the above code and re-compile if it's what you want. TSE FLEX is a mostly ASCII format
    // so just printing out the messages (as the above code does) can be helpful.
    //*******************************************************************************************



    buf += 18;  // Now we start skipping through the buffer since we've already determined it is valid

    size_t msg_type = parse_number(buf, 3);  buf += 3;
    char   exch_code = *buf; ++buf;
    //size_t session = parse_number(buf, 2);
    buf += 2;
    
    //size_t issue_class = parse_number(buf, 4);
    if(buf[0] != '0' || buf[1] != '1') {
      // 01 is the issue classification code for stock
      log.printf("\n");
      return packet_len;
    }
    buf += 4;

    buf += 7; // skip over spaces in stocks
    size_t stock_name = parse_number(buf, 5);

    buf += 4;
    buf += 1; // reserved code
    
    if(*buf != 0x12) {
      //m_logger->log_printf(Log::ERROR, "TSE_FLEX_Handler::parse: context %zd invalid header separation character", context);
      return 0;
    }

    int scan_posn = 0;
    while(buf[scan_posn] != 0x11) {
      ++scan_posn;  // skip control character

      while(buf[scan_posn] != 0x13 && buf[scan_posn] != 0x11) { ++scan_posn; }
    }

    while(*buf != 0x11) {
      ++buf;  // skip control character

      if(buf[0] == 'N' && buf[1] == 'O') { // NO
	log.printf(" msg_type:NO stock_name:%zd exch_code:%c header_msg_type=%zd\n", stock_name, exch_code, msg_type);
	log.printf(" actual_msg: %.20s\n", buf);

      } else if(buf[0] == '1' && buf[1] == 'P') {  // 1P
	log.printf(" msg_type:1P stock_name:%zd\n", stock_name);
	log.printf(" actual_msg: %.33s\n", buf);
      } else if(buf[0] == 'V' && buf[1] == 'A') {  // VA
	log.printf(" msg_type:VA stock_name:%zd\n", stock_name);
	log.printf(" actual_msg: %.30s\n", buf);
      } else if(buf[0] == 'V' && buf[1] == 'L') {  // VL
	log.printf(" msg_type:VL stock_name:%zd exch_code:%c header_msg_type=%zd\n", stock_name, exch_code, msg_type);
	log.printf(" actual_msg: %.30s\n", buf);
      } else if(buf[0] == 'L' && buf[1] == 'C') {  // LC
	log.printf(" msg_type:LC stock_name:%zd\n", stock_name);
	log.printf(" actual_msg: %.15s\n", buf);
      } else if(buf[0] == 'B' && buf[1] == 'P') {  // BP
	log.printf(" msg_type:BP stock_name:%zd\n", stock_name);
	log.printf(" actual_msg: %.61s\n", buf);
      } else if(buf[0] == 'I' && buf[1] == 'I') {  // II
	log.printf(" msg_type:II stock_name:%zd lot_size:%zd\n", stock_name, parse_number(buf+70, 14));
	log.printf(" actual_msg: %.125s\n", buf);
      }

      while(*buf != 0x13 && *buf != 0x11) { ++buf; } // SKIP to next
    }
    return packet_len;
  }


  void
  TSE_FLEX_Handler::write_morning_closes()
  {
    if (m_morning_closes_written)
      return;

    int counter = 0;
    for(size_t id = 1; id < m_morning_close_data.size(); id++) {
      if (m_morning_close_data[id].price.iprice64() > 0) {
        if (m_morning_close_data[id].exch_time < m_micros_morning_close) {
          m_morning_close_data[id].size = 0;
        }

        gen_qbdf_trade_micros(m_qbdf_builder, id,
                              m_morning_close_data[id].recv_time,
                              m_morning_close_data[id].exch_time - m_morning_close_data[id].recv_time,
                              m_morning_close_data[id].exch, ' ', '0', "AMCX", 0,
                              m_morning_close_data[id].price, false);

        m_morning_close_data[id].price = MDPrice();
        counter++;
      }
    }
    m_morning_closes_written = true;
    m_logger->log_printf(Log::INFO, "Wrote %d morning closes", counter);
  }


  void
  TSE_FLEX_Handler::write_afternoon_closes()
  {
    if (m_afternoon_closes_written)
      return;

    int counter = 0;
    for(size_t id = 1; id < m_afternoon_close_data.size(); id++) {
      if (!m_afternoon_close_data[id].price.blank()) {
        if (m_afternoon_close_data[id].exch_time < m_micros_afternoon_close) {
          m_afternoon_close_data[id].size = 0;
        }

        gen_qbdf_trade_micros(m_qbdf_builder, id,
                              m_afternoon_close_data[id].recv_time,
                              m_afternoon_close_data[id].exch_time - m_afternoon_close_data[id].recv_time,
                              m_afternoon_close_data[id].exch, ' ', '0', "PMCX", 0,
                              m_afternoon_close_data[id].price, false);

        m_afternoon_close_data[id].price = MDPrice();
        counter++;
      }
    }
    m_afternoon_closes_written = true;
    m_logger->log_printf(Log::INFO, "Wrote %d afternoon closes", counter);
  }


  void
  TSE_FLEX_Handler::write_bp_data()
  {
    const char* where = "TSE_FLEX_Handler::write_bp_data";

    string output_path = m_qbdf_builder->get_output_path(m_root_output_path, Time::today().strftime("%Y%m%d"));

    char out_file[1024];
    sprintf(out_file, "%s/bp_data.csv", output_path.c_str());
    m_logger->log_printf(Log::INFO, "%s: Writing BP data to %s", where, out_file);
    FILE *bp_data_file = fopen(out_file, "w");
    if (bp_data_file == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to create BP data file [%s] fopen retcode %d=[%s]",
                           where, out_file, errno, strerror(errno));
      m_logger->sync_flush();
      return;
    }
    fprintf(bp_data_file, "applicable_date,qbdf_id,vendor_id,tick_size_table,base_price,limit_max_price,limit_min_price\n");

    for(size_t i = 0; i < m_bp_data.size(); i++) {
      bp_data_struct bps = m_bp_data[i];
      fprintf(bp_data_file, "%s,%u,%u,%u,%f,%f,%f\n", bps.applicable_date.c_str(), bps.id, bps.stock_code,
	      bps.tick_size_table, bps.base_price.fprice(), bps.limit_max_price.fprice(), bps.limit_min_price.fprice());
    }

    fclose(bp_data_file);
    m_logger->log_printf(Log::INFO, "Wrote %lu BP data", m_bp_data.size());
  }

  void
  TSE_FLEX_Handler::write_ii_data()
  {
    const char* where = "TSE_FLEX_Handler::write_ii_data";

    string output_path = m_qbdf_builder->get_output_path(m_root_output_path, Time::today().strftime("%Y%m%d"));

    char out_file[1024];
    sprintf(out_file, "%s/ii_data.csv", output_path.c_str());
    m_logger->log_printf(Log::INFO, "%s: Writing II data to %s", where, out_file);
    FILE *ii_data_file = fopen(out_file, "w");
    if (ii_data_file == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to create II data file [%s] fopen retcode %d=[%s]",
                           where, out_file, errno, strerror(errno));
      m_logger->sync_flush();
      return;
    }
    fprintf(ii_data_file, "applicable_date,qbdf_id,vendor_id,trading_unit\n");

    for(size_t i = 0; i < m_ii_data.size(); i++) {
      ii_data_struct iis = m_ii_data[i];
      fprintf(ii_data_file, "%s,%u,%u,%u\n", iis.applicable_date.c_str(), iis.id, iis.stock_code, iis.trading_unit);
    }

    fclose(ii_data_file);
    m_logger->log_printf(Log::INFO, "Wrote %lu II data", m_ii_data.size());
  }

  void
  TSE_FLEX_Handler::write_special_quotes()
  {
    const char* where = "TSE_FLEX_Handler::write_special_quotes";

    string output_path = m_qbdf_builder->get_output_path(m_root_output_path, Time::today().strftime("%Y%m%d"));

    char out_file[1024];
    sprintf(out_file, "%s/special_quotes.csv", output_path.c_str());
    m_logger->log_printf(Log::INFO, "%s: Writing special quotes to %s", where, out_file);
    FILE *special_quotes_file = fopen(out_file, "w");
    if (special_quotes_file == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to create special quotes file [%s] fopen retcode %d=[%s]",
                           where, out_file, errno, strerror(errno));
      m_logger->sync_flush();
      return;
    }
    fprintf(special_quotes_file, "qbdf_id,vendor_id,price,size,side\n");

    int counter = 0;
    for(size_t id = 1; id < m_special_quote_data.size(); id++) {
      double special_quote_price = m_special_quote_data[id].price.fprice();
      if (special_quote_price > 0.0) {
        fprintf(special_quotes_file, "%zu,%u,%f,%d,%c\n",
                id, m_special_quote_data[id].id, special_quote_price,
                m_special_quote_data[id].size, m_special_quote_data[id].side);
        counter++;
      }
    }
    fclose(special_quotes_file);
    m_logger->log_printf(Log::INFO, "Wrote %d special quotes", counter);
  }


  void
  TSE_FLEX_Handler::reset(const char* msg)
  {

  }

  void
  TSE_FLEX_Handler::reset(size_t context, const char* msg)
  {

  }

  void
  TSE_FLEX_Handler:: subscribe_product(ProductID id_,ExchangeID exch_,
                                       const string& mdSymbol_,const string& mdExch_)
  {
    int symbol = strtol(mdSymbol_.c_str(), 0, 10);

    m_symbol_to_id.insert(make_pair(symbol, id_));
  }


  ProductID
  TSE_FLEX_Handler::product_id_of_symbol(int symbol)
  {
    if (m_qbdf_builder) {
      stringstream ss;
      ss << symbol;
      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(ss.str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol [%d]",
                             "NYSE_Handler::product_id_of_symbol", symbol);
        m_logger->sync_flush();
        return Product::invalid_product_id;
      } else {
        return symbol_id;
      }
    } else {
      map<int, ProductID>::const_iterator p_i = m_symbol_to_id.find(symbol);
      if(p_i != m_symbol_to_id.end()) {
        return p_i->second;
      } else {
        return Product::invalid_product_id;
      }
    }

  }



  void
  TSE_FLEX_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);
    
    bool build_book = params.getDefaultBool("build_book", true);
    if(build_book) {
      m_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_order_books.push_back(m_order_book);
      m_order_book->init(params);

      m_auction_order_book.reset(new MDOrderBookHandler(m_app, exch, m_logger));
      m_secondary_order_books.push_back(m_auction_order_book);
      m_auction_order_book->init(params, true);
    }

    m_root_output_path = params.getDefaultString("root_output_path", "");
    m_is_standard_feed = params.getDefaultBool("standard_feed", false);

    int max_product_id = m_app->pm()->max_product_id();

    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    m_security_status.resize(max_product_id+1, MarketStatus::Unknown);

    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;

    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';

    m_current_price.clear();
    m_trading_volume.clear();
    m_morning_close_data.clear();
    m_afternoon_close_data.clear();
    m_special_quote_data.clear();
    m_bp_data.clear();

    m_morning_closes_written = false;
    m_afternoon_closes_written = false;

    m_current_price.resize(max_product_id+1, MDPrice());
    m_trading_volume.resize(max_product_id+1, 0);

    if(params.getDefaultBool("quote_dump", false)) {
      m_quote_dump = m_app->lm()->get_logger("TSE_QUOTE_DUMP", "csv");
      m_quote_dump->printf("ticker,tag,flag,change_flag,unit_flag,price,sign,time,quote_flag,matching_sign,qty_volume,qty_quantity,qty_sign,order_volume,order_quantity,order_sign,mob_flag\n");
    }

  }


  void
  TSE_FLEX_Handler::start(void)
  {

  }

  void
  TSE_FLEX_Handler::stop(void)
  {
  }


  TSE_FLEX_Handler::TSE_FLEX_Handler(Application* app) :
    Handler(app, "TSE_FLEX"),
    m_micros_midnight(0),
    m_micros_morning_close(0),
    m_micros_afternoon_close(0)
  {
  }

  TSE_FLEX_Handler::~TSE_FLEX_Handler()
  {
  }



}
