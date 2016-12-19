#include <Data/QBDF_Util.h>
#include <Data/UTP_Handler.h>
#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <MD/MDProvider.h>
#include <Logger/Logger_Manager.h>
#include <Util/UnitTestFactory.h>

namespace hf_core {

  const char start_of_header = 0x01;
  const char end_of_text     = 0x03;
  const char unit_separation = 0x1F;

  struct common_message_header {
    char category;
    char type;
    char session_identifier;
    char retression_requester[2];
    char seq_no[8];
    char market_center_originator_id;
  };

  struct legacy_message_header {
    common_message_header common_header;
    char timestamp[9];
    char reserved;
  };

  struct message_header {
    common_message_header common_header;
    char sip_timestamp[6];
    char reserved[3];
    char sub_market_center_id;
    char timestamp_1[6];
    char timestamp_2[6];
    char transaction_id[7];
  };

  struct short_quote {
    char symbol[5];
    char quote_condition;
    char bid_price_denominator;
    char bid_short_price[6];
    char bid_size[2];
    char ask_price_denominator;
    char ask_short_price[6];
    char ask_size[2];
    char nbbo_indicator;
    char finra_bbo_indicator;
  };

  struct long_quote {
    char symbol[11];
    char quote_condition;
    char bid_price_denominator;
    char bid_price[10];
    char bid_size[7];
    char ask_price_denominator;
    char ask_price[10];
    char ask_size[7];
    char currency[3];
    char nbbo_indicator;
    char finra_bbo_indicator;
  };

  // As of October 1, 2012 short_quote/long_quote are retired and
  // quotes with retail interest are used
  struct short_quote_rlp {
    char symbol[5];
    char reserved;
    char sip_generated_updated;
    char quote_condition;
    char luld_bbo_indicator;
    char bid_price_denominator;
    char bid_short_price[6];
    char bid_size[2];
    char ask_price_denominator;
    char ask_short_price[6];
    char ask_size[2];
    char nbbo_indicator;
    char luld_national_bbo_indicator;
    char finra_bbo_indicator;
  };

  struct long_quote_rlp {
    char symbol[11];
    char reserved;
    char sip_generate_updated;
    char quote_condition;
    char luld_bbo_indcator;
    char retail_interest_indicator;
    char bid_price_denominator;
    char bid_price[10];
    char bid_size[7];
    char ask_price_denominator;
    char ask_price[10];
    char ask_size[7];
    char currency[3];
    char nbbo_indicator;
    char luld_national_bbo_indicator;
    char finra_bbo_indicator;
  };

  struct short_trade {
    char symbol[5];
    char sale_condition;
    char price_denominator;
    char trade_price[6];
    char report_volume[6];
    char consolidated_price_change_indicator;
  };

  struct long_trade {
    char symbol[11];
    char trade_through_excempt_flag;
    char sale_condition[4];
    char sellers_sale_days[2];
    char price_denominator;
    char trade_price[10];
    char currency[3];
    char report_volume[9];
    char consolidated_price_change_indicator;
  };

  struct trade_correction {
    char orig_seq_no[8];
    char symbol[11];
    char orig_trade_through_excempt_flag;
    char orig_sale_condition[4];
    char orig_sellers_sale_days[2];
    char orig_price_denominator;
    char orig_trade_price[10];
    char orig_currency[3];
    char orig_report_volume[9];
    char corr_trade_through_excempt_flag;
    char corr_sale_condition[4];
    char corr_sellers_sale_days[2];
    char corr_price_denominator;
    char corr_trade_price[10];
    char corr_currency[3];
    char corr_report_volume[9];
    char consolidated_high_price_denominator;
    char consolidated_high_price[10];
    char consolidated_low_price_denominator;
    char consolidated_low_price[10];
    char consolidated_last_price_denominator;
    char consolidated_last_price[10];
    char consolidated_last_price_market_center;
    char consolidated_currency[3];
    char consolidated_volume[11];
    char consolidated_price_change_indicator;
  };

  struct trade_cancel_error {
    char orig_seq_no[8];
    char symbol[11];
    char function;
    char orig_trade_through_excempt_flag;
    char orig_sale_condition[4];
    char orig_sellers_sale_days[2];
    char orig_price_denominator;
    char orig_trade_price[10];
    char orig_currency[3];
    char orig_report_volume[9];
    char consolidated_high_price_denominator;
    char consolidated_high_price[10];
    char consolidated_low_price_denominator;
    char consolidated_low_price[10];
    char consolidated_last_price_denominator;
    char consolidated_last_price[10];
    char consolidated_last_price_market_center;
    char consolidated_currency[3];
    char consolidated_volume[11];
    char consolidated_price_change_indicator;
  };

  static uint64_t parse_seqno(const char *s)
  {
    uint64_t seqno = 0;
    // 8 digit seqno
    for(int i = 8; i; --i, ++s) {
      seqno *= 10;
      seqno += (*s - '0');
    }
    return seqno;
  }

  static int parse_int(const char* s, int len)
  {
    int val = 0;
    for(int i = len; i; --i, ++s) {
      val *= 10;
      val += (*s - '0');
    }
    return val;

  }

  inline static char* symbol_copy(char* dest, const char* src, int len)
  {
    for( ; *src != ' ' && len > 0; ++src, ++dest, --len) {
      *dest = *src;
    }
    *dest = '\0';
    return dest;
  }

  inline static Time parse_timestamp_legacy(const char* ts, uint64_t& micros_since_midnight)
  {
    int h =   (ts[0] - '0') * 10 + (ts[1] - '0');
    int min = (ts[2] - '0') * 10 + (ts[3] - '0');
    int sec = (ts[4] - '0') * 10 + (ts[5] - '0');

    int milli = (ts[6] - '0') * 100;
    milli +=    (ts[7] - '0') * 10;
    milli +=    (ts[8] - '0');

    micros_since_midnight = (milli * 1000) + 
                            (sec * Time_Constants::micros_per_second) + 
                            (min * Time_Constants::micros_per_min) + 
                            (h * Time_Constants::micros_per_hr);

    return Time::today() + hours(h) + minutes(min) + seconds(sec) + msec(milli);
  }

  inline static Time parse_timestamp(const char* ts, uint64_t& micros_since_midnight)
  {
    micros_since_midnight = (ts[0] - ' ') * 95*95*95*95*95ULL +
                            (ts[1] - ' ') * 95*95*95*95ULL +
                            (ts[2] - ' ') * 95*95*95ULL +
                            (ts[3] - ' ') * 95*95ULL +
                            (ts[4] - ' ') * 95ULL +
                            (ts[5] - ' ');
    return Time::today() + usec(micros_since_midnight); 
  }

  inline void get_denom(char code, int& numerator, double& denom)
  {
    switch(code) {
    case 'B' : numerator = 2; denom = 100.0; break;
    case 'C' : numerator = 3; denom = 1000.0; break;
    case 'D' : numerator = 4; denom = 10000.0; break;
    default  : numerator = 0; denom = 1; break;
    }
  }

  inline double parse_price(const char* price, char code, int len)
  {
    int numerator;
    double denom;
    get_denom(code, numerator, denom);

    int whole_len = len - numerator;

    double price_w = (double) parse_int(price, whole_len);
    if(numerator > 0) {
      double price_f = (double) parse_int(price + whole_len, numerator);
      price_f /= denom;
      return(price_w + price_f);
    } else {
      return price_w;
    }
  }

  size_t
  UTP_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "UTP_Handler::parse";

    if(len < 20) {
      send_alert("%s: %s Runt packet received, len=%d", where, m_name.c_str(), len);
      return 0;
    }

    if(buf[0] != start_of_header) {
      send_alert("First character not expected SOH char");
      return 0;
    }

    if (m_qbdf_builder) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
      m_ms_latest = m_micros_recv_time / Time_Constants::micros_per_mili;
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                             m_micros_recv_time, m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time));
        m_logger->sync_flush();
      }
    }

    mutex_t::scoped_lock lock(m_mutex);

    channel_info& ci = m_channel_info_map[context];

    const common_message_header& header = reinterpret_cast<const common_message_header&>(*(buf+1));
    size_t seq_no = parse_seqno(header.seq_no);

    if(header.category == 'C' && header.type == 'T') {
      ++seq_no; // treat it as the next seq_no
    }

    if(header.category == 'C' && header.type == 'L') {
      // Control, reset seq_no
      m_logger->log_printf(Log::INFO, "Received control reset for context %ld, resetting seq_no to %ld", context, seq_no);
      ci.seq_no = seq_no;
    } else if(ci.seq_no > seq_no) {
      // duplicate
      return len;
    } else if(ci.seq_no < seq_no) {
      if (seq_no >= 99000000) {
        m_logger->log_printf(Log::WARN, "Received seq_no %ld > expected seq_no %ld, but past 99000000 threshold so will NOT gap forward", seq_no, ci.seq_no);
        return len;
      } else {
        if(ci.begin_recovery(this, buf, len, seq_no, 0)) {
          return len;
        }
      }
    }

    ci.seq_no = seq_no;
    parse2(context, buf, len);

    // In CQS & UTP, it is parse2 that determines the next seq_no
    ci.last_update = Time::currentish_time();
    if(!ci.queue.empty()) {
      ci.process_recovery(this);
    }

    return len;
  }

  size_t
  UTP_Handler::parse2(size_t context, const char* buf, size_t len)
  {
    //const char* where = "UTP_Handler::parse2";

    channel_info& ci = m_channel_info_map[context];
    char symbol[16];
    const char* buf_end = buf + len;

    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      record_header h(Time::current_time(), context, len);
      m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
      m_recorder->unlocked_write(buf, len);
    }

    for(;;) {
      const common_message_header& header = reinterpret_cast<const common_message_header&>(*(buf+1));

      size_t header_length;
      Time exch_timestamp;
      char market_center_originator_id;
      if(header.session_identifier == '1')
      {
        const message_header& msg_header = reinterpret_cast<const message_header&>(*(buf+1));
        header_length = sizeof(msg_header);
        exch_timestamp = parse_timestamp(msg_header.sip_timestamp, m_micros_exch_time);
        market_center_originator_id = msg_header.common_header.market_center_originator_id;
      }
      else
      {
        const legacy_message_header& msg_header = reinterpret_cast<const legacy_message_header&>(*(buf+1));
        header_length = sizeof(msg_header);
        exch_timestamp = parse_timestamp_legacy(msg_header.timestamp, m_micros_exch_time);
        market_center_originator_id = msg_header.common_header.market_center_originator_id;
      }

      m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
      size_t message_length = header_length;

      if(header.category == 'Q') {
        // Quote
        if(header.type == 'E') {
          // short quote retail liquidity interest -- starting October 1, 2012
          const short_quote_rlp& quote = reinterpret_cast<const short_quote_rlp&>(*(buf+1+header_length));
          message_length += sizeof(short_quote_rlp);
          if     (quote.nbbo_indicator == '2') { message_length += 22; }
          else if(quote.nbbo_indicator == '3') { message_length += 43; }
          if     (quote.finra_bbo_indicator == '2') { message_length += 4; }

          symbol_copy(symbol, quote.symbol, 5);
          symbol_info_t si = product_id_of_symbol(market_center_originator_id, symbol, 5);

          if (si.id != Product::invalid_product_id) {
            //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
            m_quote.m_id = si.id;
            m_quote.m_exch = si.exch;
            m_quote.set_time(exch_timestamp);
            m_quote.m_bid = parse_price(quote.bid_short_price, quote.bid_price_denominator, 6);
            m_quote.m_ask = parse_price(quote.ask_short_price, quote.ask_price_denominator, 6);
            m_quote.m_bid_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.bid_size, 2);
            m_quote.m_ask_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.ask_size, 2);
            m_quote.m_cond = quote.quote_condition;

            if (m_qbdf_builder) {
              if (!m_exclude_quotes) {
                gen_qbdf_quote_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                      m_micros_exch_time_delta, market_center_originator_id,
                                      quote.quote_condition,
                                      MDPrice::from_fprice(m_quote.m_bid),
                                      MDPrice::from_fprice(m_quote.m_ask),
                                      m_quote.m_bid_size, m_quote.m_ask_size);
              }
            } else {
              send_quote_update();
            }
          }
        } else if(header.type == 'F') {
          // long quote retail liquidity interest -- Starting October 1, 2012
          const long_quote_rlp& quote = reinterpret_cast<const long_quote_rlp&>(*(buf+1+header_length));
          message_length += sizeof(long_quote_rlp);
          if     (quote.nbbo_indicator == '2') { message_length += 22; }
          else if(quote.nbbo_indicator == '3') { message_length += 43; }
          if     (quote.finra_bbo_indicator == '2') { message_length += 4; }

          symbol_copy(symbol, quote.symbol, 11);
          symbol_info_t si = product_id_of_symbol(market_center_originator_id, symbol, 11);

          if (si.id != Product::invalid_product_id) {
            //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
            m_quote.m_id = si.id;
            m_quote.m_exch = si.exch;
            m_quote.set_time(exch_timestamp);
            m_quote.m_bid = parse_price(quote.bid_price, quote.bid_price_denominator, 10);
            m_quote.m_ask = parse_price(quote.ask_price, quote.ask_price_denominator, 10);
            m_quote.m_bid_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.bid_size, 7);
            m_quote.m_ask_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.ask_size, 7);
            m_quote.m_cond = quote.quote_condition;

            if (m_qbdf_builder) {
              if (!m_exclude_quotes) {
                gen_qbdf_quote_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                      m_micros_exch_time_delta, market_center_originator_id,
                                      quote.quote_condition,
                                      MDPrice::from_fprice(m_quote.m_bid),
                                      MDPrice::from_fprice(m_quote.m_ask),
                                      m_quote.m_bid_size, m_quote.m_ask_size);
              }
            } else {
              send_quote_update();
            }
          }
        } else if(header.type == 'C') {
          // short quote -- retired as of October 1, 2012
          const short_quote& quote = reinterpret_cast<const short_quote&>(*(buf+1+header_length));
          message_length += sizeof(short_quote);
          if     (quote.nbbo_indicator == '2') { message_length += 22; }
          else if(quote.nbbo_indicator == '3') { message_length += 43; }
          if     (quote.finra_bbo_indicator == '2') { message_length += 4; }

          symbol_copy(symbol, quote.symbol, 5);
          symbol_info_t si = product_id_of_symbol(market_center_originator_id, symbol, 5);

          if (si.id != Product::invalid_product_id) {
            //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
            m_quote.m_id = si.id;
            m_quote.m_exch = si.exch;
            m_quote.set_time(exch_timestamp);
            m_quote.m_bid = parse_price(quote.bid_short_price, quote.bid_price_denominator, 6);
            m_quote.m_ask = parse_price(quote.ask_short_price, quote.ask_price_denominator, 6);
            m_quote.m_bid_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.bid_size, 2);
            m_quote.m_ask_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.ask_size, 2);
            m_quote.m_cond = quote.quote_condition;

            if (m_qbdf_builder) {
              if (!m_exclude_quotes) {
                gen_qbdf_quote_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                      m_micros_exch_time_delta, market_center_originator_id,
                                      quote.quote_condition,
                                      MDPrice::from_fprice(m_quote.m_bid),
                                      MDPrice::from_fprice(m_quote.m_ask),
                                      m_quote.m_bid_size, m_quote.m_ask_size);
              }
            } else {
              send_quote_update();
            }
          }
        } else if(header.type == 'D') {
          // long quote  -- retired as of October 01, 2012
          const long_quote& quote = reinterpret_cast<const long_quote&>(*(buf+1+header_length));
          message_length += sizeof(long_quote);
          if     (quote.nbbo_indicator == '2') { message_length += 22; }
          else if(quote.nbbo_indicator == '3') { message_length += 43; }
          if     (quote.finra_bbo_indicator == '2') { message_length += 4; }

          symbol_copy(symbol, quote.symbol, 11);
          symbol_info_t si = product_id_of_symbol(market_center_originator_id, symbol, 11);

          if (si.id != Product::invalid_product_id) {
            //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
            m_quote.m_id = si.id;
            m_quote.m_exch = si.exch;
            m_quote.set_time(exch_timestamp);
            m_quote.m_bid = parse_price(quote.bid_price, quote.bid_price_denominator, 10);
            m_quote.m_ask = parse_price(quote.ask_price, quote.ask_price_denominator, 10);
            m_quote.m_bid_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.bid_size, 7);
            m_quote.m_ask_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.ask_size, 7);
            m_quote.m_cond = quote.quote_condition;

            if (m_qbdf_builder) {
              if (!m_exclude_quotes) {
                gen_qbdf_quote_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                      m_micros_exch_time_delta, market_center_originator_id,
                                      quote.quote_condition,
                                      MDPrice::from_fprice(m_quote.m_bid),
                                      MDPrice::from_fprice(m_quote.m_ask),
                                      m_quote.m_bid_size, m_quote.m_ask_size);
              }
            } else {
              send_quote_update();
            }
          }
        }
      } else if(header.category == 'T') {
        // Trade
        if(header.type == 'A') {
          // short trade
          const short_trade& trade = reinterpret_cast<const short_trade&>(*(buf+1+header_length));
          message_length += sizeof(short_trade);

          symbol_copy(symbol, trade.symbol, 5);
          symbol_info_t si = product_id_of_symbol(market_center_originator_id, symbol, 5);

          if (si.id != Product::invalid_product_id) {
            //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
            m_trade.m_id = si.id;
            m_trade.m_exch = si.exch;
            m_trade.set_time(exch_timestamp);
            m_trade.m_price = parse_price(trade.trade_price, trade.price_denominator, 6);
            m_trade.m_size = parse_int(trade.report_volume, 6);
            m_trade.set_side(0);
            memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
            m_trade.m_trade_qualifier[0] = trade.sale_condition;
            m_trade.m_correct = '0';

            //m_logger->log_printf(Log::INFO, "Got short trade for %s  price=%f  vol=%d", symbol, m_trade.m_price, m_trade.m_size);
            if (m_qbdf_builder) {
              if (!m_exclude_trades) {
                gen_qbdf_trade_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                      m_micros_exch_time_delta, market_center_originator_id,
                                      0, '0', string(m_trade.m_trade_qualifier, 4), m_trade.m_size,
                                      MDPrice::from_fprice(m_trade.m_price));
              }
            } else {
              send_trade_update();
            }
          }
        } else if(header.type == 'W') {
          // long trade
          const long_trade& trade = reinterpret_cast<const long_trade&>(*(buf+1+header_length));
          message_length += sizeof(long_trade);

          symbol_copy(symbol, trade.symbol, 11);
          symbol_info_t si = product_id_of_symbol(market_center_originator_id, symbol, 11);

          if (si.id != Product::invalid_product_id) {
            //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
            m_trade.m_id = si.id;
            m_trade.m_exch = si.exch;
            m_trade.set_time(exch_timestamp);
            m_trade.m_price = parse_price(trade.trade_price, trade.price_denominator, 10);
            m_trade.m_size = parse_int(trade.report_volume, 9);
            m_trade.set_side(0);
            memcpy(m_trade.m_trade_qualifier, trade.sale_condition, sizeof(m_trade.m_trade_qualifier));
            m_trade.m_correct = '0';

            //m_logger->log_printf(Log::INFO, "Got long trade for %s  price=%f  vol=%d", symbol, m_trade.m_price, m_trade.m_size);
            if (m_qbdf_builder) {
              if (!m_exclude_trades) {
                gen_qbdf_trade_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                      m_micros_exch_time_delta, market_center_originator_id,
                                      0, '0', string(m_trade.m_trade_qualifier, 4), m_trade.m_size,
                                      MDPrice::from_fprice(m_trade.m_price));
              }
            } else {
              send_trade_update();
            }
          }
        } else if (header.type == 'Y') {
          // trade_correction
          const trade_correction& trade_corr = reinterpret_cast<const trade_correction&>(*(buf+1+header_length));
          message_length += sizeof(trade_correction);

          symbol_copy(symbol, trade_corr.symbol, 11);
          symbol_info_t si = product_id_of_symbol(market_center_originator_id, symbol, 11);

          if (si.id != Product::invalid_product_id) {
            m_logger->log_printf(Log::INFO, "Got correction for %s", symbol);
            m_logger->sync_flush();

            uint64_t orig_seq_no = parse_int(trade_corr.orig_seq_no, 8);

            double orig_price =  parse_price(trade_corr.orig_trade_price,
                                             trade_corr.orig_price_denominator, 10);
            int orig_size = parse_int(trade_corr.orig_report_volume, 9);
            string orig_cond = string(trade_corr.orig_sale_condition, 4);

            double corr_price =  parse_price(trade_corr.corr_trade_price,
                                             trade_corr.corr_price_denominator, 10);
            int corr_size = parse_int(trade_corr.corr_report_volume, 9);
            string corr_cond = string(trade_corr.corr_sale_condition, 4);

            if (m_qbdf_builder) {
              gen_qbdf_cancel_correct(m_qbdf_builder, si.id,
                                      m_micros_exch_time / Time_Constants::micros_per_mili,
                                      0, string(symbol), "CORRECTION",
                                      orig_seq_no, orig_price, orig_size, orig_cond,
                                      corr_price, corr_size, corr_cond);
            } else {
              //don't try to apply correction in real-time
              //send_trade_update();
            }
          }
        } else if (header.type == 'Z') {
          // trade_cancel_error
          const trade_cancel_error& trade_cancel = reinterpret_cast<const trade_cancel_error&>(*(buf+1+header_length));
          message_length += sizeof(trade_cancel_error);

          symbol_copy(symbol, trade_cancel.symbol, 11);
          symbol_info_t si = product_id_of_symbol(market_center_originator_id, symbol, 11);

          if (si.id != Product::invalid_product_id) {
            //m_logger->log_printf(Log::INFO, "Got cancel_error for %s", symbol);
            //m_logger->sync_flush();

            uint64_t orig_seq_no = parse_int(trade_cancel.orig_seq_no, 8);
            double orig_price =  parse_price(trade_cancel.orig_trade_price,
                                             trade_cancel.orig_price_denominator, 10);
            int orig_size = parse_int(trade_cancel.orig_report_volume, 9);
            string orig_cond = string(trade_cancel.orig_sale_condition, 4);

            if (m_qbdf_builder) {
              gen_qbdf_cancel_correct(m_qbdf_builder, si.id,
                                      m_micros_exch_time / Time_Constants::micros_per_mili,
                                      0, string(symbol), "CANCEL", orig_seq_no,
                                      orig_price, orig_size, orig_cond, 0, 0, "");
            } else {
              //don't try to apply cancel_error in real-time
              //send_trade_update();
            }
          }
        }
      } else if(header.category == 'C') {
        if(header.type == 'W') {
          // Wipe-Out -- clear all quotes for exchange
          send_alert("%s: %s Received wipe-out message for exch ", "UTP_Handler::parse2", m_name.c_str());
        }
      } else{
        // We don't care about any other kind of message
      }

      if(!(header.category == 'C' && (header.type == 'T' || header.type == 'L'))) {
        ++ci.seq_no;
      }

      buf += 1 + message_length;
      // search for next message
      //if(*buf != unit_separation && *buf != end_of_text && header.category != 'C' && header.category != 'A') {
      //  m_logger->log_printf(Log::WARN, "%s: %s Invalid computation of next message length for %c %c", "UTP_Handler::parse2", ci.name.c_str(), header.category, header.type);
      //}
      while(buf < buf_end && *buf != unit_separation && *buf != end_of_text ) {
        ++buf;
      }
      if(*buf == end_of_text || buf >= buf_end) {
        break;
      }
    }

    ci.last_update = Time::currentish_time();
    return len;
  }

  void
  UTP_Handler::send_quote_update()
  {
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
  UTP_Handler::send_trade_update()
  {
    m_msu.m_market_status = m_security_status[m_trade.m_id];
    m_feed_rules->apply_trade_conditions(m_trade, m_msu);
    if(m_msu.m_market_status != m_security_status[m_trade.m_id]) {
      m_msu.m_id = m_trade.m_id;
      m_msu.m_exch = m_trade.m_exch;
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


  UTP_Handler::symbol_info_t
  UTP_Handler::product_id_of_symbol(char participant_id, const char* symbol, int len)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol);
      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string);
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol [%s]",
                             "UTP_Handler::product_id_of_symbol", symbol_string.c_str());
        m_logger->sync_flush();
        return UTP_Handler::symbol_info_t(Product::invalid_product_id, ExchangeID::MULTI);
      }
      ExchangeID exch = m_qbdf_builder->get_hf_exch_id_by_qbdf_exch(participant_id);
      return UTP_Handler::symbol_info_t(symbol_id, exch);
    }

    symbol_map_t* symbol_map = m_symbol_maps[participant_id];
    if(symbol_map) {
      symbol_map_t::const_iterator prod_i = symbol_map->find(symbol);
      if(prod_i != symbol_map->end()) {
        return UTP_Handler::symbol_info_t(prod_i->second.id, prod_i->second.exch);
      }
    }

    return UTP_Handler::symbol_info_t(Product::invalid_product_id, ExchangeID::MULTI);
  }

  void
  UTP_Handler::reset(const char *)
  {
    if (m_qbdf_builder) {
      gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, ' ', -1);
    }
  }

  void
  UTP_Handler::reset(size_t context, const char *)
  {
    if (m_qbdf_builder) {
      gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, ' ', context);
    }
  }

  void
  UTP_Handler::init(const string& name,const Parameter& params)
  {
    Handler::init(name, params);
    m_name = name;

#ifdef CW_DEBUG
    cout << "TODAY: " << Time::today().strftime("%Y%m%d") << endl;
#endif
    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);
    
    m_exch_lot_size.resize(ExchangeID::size, 100.0);

    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);
  }

  void
  UTP_Handler::start(void)
  {
  }

  void
  UTP_Handler::stop(void)
  {
  }

  void
  UTP_Handler::subscribe_product(ProductID id, ExchangeID exch,
                                 const string& mdSymbol, const string& mdExch)
  {
    mutex_t::scoped_lock lock(m_mutex);
    if(mdExch.size() != 1) {
      m_logger->log_printf(Log::ERROR, "UTP_Handler::subscribe_product mdExch '%s' for symbol %s does not map to a valid participant id", mdExch.c_str(), mdSymbol.c_str());
      return;
    }

    char p_id = mdExch[0];

    // This way we have a known-good const char*
    pair<set<string>::iterator, bool> s_i = m_symbols.insert(mdSymbol.c_str());

    const char* symbol = s_i.first->c_str();

    if(m_symbol_maps[p_id] == 0) {
      m_symbol_maps[p_id] = new symbol_map_t;
    }
    m_symbol_maps[p_id]->insert(make_pair(symbol, symbol_info_t(id, exch)));
  }

  UTP_Handler::UTP_Handler(Application* app) :
    Handler(app, "UTP")
  {
    m_symbol_maps.resize(256, 0);
  }

  UTP_Handler::~UTP_Handler()
  {
  }
  
  static bool test_UTP(ILogger& logger)
  {
    bool pass = true;
    
    uint64_t micros_since_midnight = 0;
    parse_timestamp("!qkJrC", micros_since_midnight);
    if(micros_since_midnight != 14400000000) {
      logger.printf("UTP_Handler parse_timestamp expected micros_since_midght 14400000000, received %lu\n", micros_since_midnight);
      pass = false;
    }
    
    return pass;
  }
  
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused"
  static bool r1 = UnitTestFactory::add_test("UTP_Handler", UnitTestFactory::TestFunction(&test_UTP));
#pragma GCC diagnostic pop
}
