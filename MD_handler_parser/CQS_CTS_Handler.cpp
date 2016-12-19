#include <Data/QBDF_Util.h>
#include <Data/CQS_CTS_Handler.h>

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
    char network;
    char retression_requester[2];
    char message_header_identifier;
  }; 

  struct legacy_message_header {
    common_message_header header;
    char reserved[2];
    char seq_no[9];
    char participant_id;
    char timestamp[6];
  };

  struct message_header { 
    common_message_header header;
    char transaction_id_part_a[2];
    char seq_no[9];
    char participant_id;
    char cts_or_cqs_timestamp[6];
    char timestamp_1[6];
    char timestamp_2[6];
    char transaction_id_part_b[9];
  };

  struct short_quote {
    char symbol[3];
    char quote_condition;
    char reserved1[2];
    char bid_price_denominator;
    char bid_short_price[8];
    char bid_size_in_lots[3];
    char reserved2;
    char offer_price_denominator;
    char offer_short_price[8];
    char offer_size_in_lots[3];
    char reserved3;
    char nbbo_indicator;
    char finra_bbo_indicator;
  };


  struct long_quote {
    char symbol[11];
    char temp_suffix;
    char test_message_indicator;
    char primary_listing_market_participant_identifier;
    char reserved[2];
    char financial_status;
    char currency_indicator[3];
    char instrument_type;
    char cancel_correction_indicator;
    char settlement_condition;
    char market_condition;
    char quote_condition;
    char reserved1[2];
    char bid_price_denominator;
    char bid_price[12];
    char bid_size_in_lots[7];
    char offer_price_denominator;
    char offer_price[12];
    char offer_size_in_lots[7];
    char finra_market_maker_id[4];
    char finra_market_maker_geographica_location[2];
    char finra_market_maker_desk_location;
    char short_sale_restriction_indicator;
    char reserved2;
    char nbbo_indicator;
    char finra_bbo_indicator;
  };

  struct short_trade {
    char symbol[3];
    char sale_condition;
    char trade_volume[4];
    char price_denominator_indicator;
    char trade_price[8];
    char consolidated_high_low_indicator;
    char participant_open_high_low_indicator;
    char reserved;
  };

  struct long_trade {
    char symbol[11];
    char temporary_suffix;
    char test_message_indicator;
    char trade_reporting_facility_indicator;
    char primary_listing_market_participant_identifier;
    char reserved1;
    char financial_status;
    char currency_indicator[3];
    char held_trade_indicator;
    char instrument_type;
    char sellers_sale_days[3];
    char sale_condition[4];
    char trade_through_except_indicator;
    char short_sale_restriction_indicator;
    char reserved2;
    char price_denominator_indicator;
    char trade_price[12];
    char trade_volume[9];
    char consolidated_high_low_indicator;
    char participant_open_high_low_indicator;
    char reserved3;
    char stop_stock_indicator;
  };

  struct correction {
    char reserved1[5];
    char primary_listing_market_participant_identifier;
    char trade_reporting_facility_indicator;
    char symbol[11];
    char temporary_suffix;
    char financial_status;
    char currency_indicator[3];
    char instrument_type;
    char orig_seq_no[9];
    char reserved2[1];
    char orig_sellers_sale_days[3];
    char orig_sale_condition[4];
    char orig_price_denominator_indicator;
    char orig_trade_price[12];
    char orig_trade_volume[9];
    char orig_stop_stock_indicator;
    char orig_trade_through_except_indicator;
    char orig_short_sale_restriction_indicator;
    char reserved3[8];
    char corr_sellers_sale_days[3];
    char corr_sale_condition[4];
    char corr_price_denominator_indicator;
    char corr_trade_price[12];
    char corr_trade_volume[9];
    char corr_stop_stock_indicator;
    char corr_trade_through_except_indicator;
    char corr_short_sale_restriction_indicator;
    char reserved4[8];
    char consolidated_last_participant_id;
    char consolidated_last_price_denominator_indicator;
    char consolidated_last_price[12];
    char consolidated_prev_close_price_date[6];
    char consolidated_high_price_denominator_indicator;
    char consolidated_high_price[12];
    char consolidated_low_price_denominator_indicator;
    char consolidated_low_price[12];
    char consolidated_total_volume[11];
    char reserved5[11];
    char participant_last_price_denominator_indicator;
    char participant_last_price[12];
    char participant_prev_close_price_date[6];
    char participant_total_volume[11];
    char participant_tick;
    char participant_open_price_denominator_indicator;
    char participant_open_price[12];
    char participant_high_price_denominator_indicator;
    char participant_high_price[12];
    char participant_low_price_denominator_indicator;
    char participant_low_price[12];
    char reserved6[12];
  };

  struct cancel_error {
    char reserved1[5];
    char primary_listing_market_participant_identifier;
    char trade_reporting_facility_indicator;
    char symbol[11];
    char temporary_suffix;
    char financial_status;
    char currency_indicator[3];
    char instrument_type;
    char cancel_error_action;
    char orig_seq_no[9];
    char orig_sellers_sale_days[3];
    char orig_sale_condition[4];
    char orig_price_denominator_indicator;
    char orig_trade_price[12];
    char orig_trade_volume[9];
    char orig_stop_stock_indicator;
    char orig_trade_through_except_indicator;
    char orig_short_sale_restriction_indicator;
    char reserved2[8];
    char consolidated_last_participant_id;
    char consolidated_last_price_denominator_indicator;
    char consolidated_last_price[12];
    char consolidated_prev_close_price_date[6];
    char consolidated_high_price_denominator_indicator;
    char consolidated_high_price[12];
    char consolidated_low_price_denominator_indicator;
    char consolidated_low_price[12];
    char consolidated_total_volume[11];
    char reserved3[11];
    char participant_last_price_denominator_indicator;
    char participant_last_price[12];
    char participant_prev_close_price_date[6];
    char participant_total_volume[11];
    char participant_tick;
    char participant_open_price_denominator_indicator;
    char participant_open_price[12];
    char participant_high_price_denominator_indicator;
    char participant_high_price[12];
    char participant_low_price_denominator_indicator;
    char participant_low_price[12];
    char reserved4[12];
  };

  static uint64_t parse_seqno(const char *s)
  {
    uint64_t seqno = 0;
    // 9 digit seqno
    for(int i = 9; i; --i, ++s) {
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
    int h =   ts[0] - '0';
    int min = ts[1] - '0';
    int sec = ts[2] - '0';

    int milli = (ts[3] - '0') * 100;
    milli +=    (ts[4] - '0') * 10;
    milli +=    (ts[5] - '0');

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
                            (ts[4] - ' ') * 95 + 
                            (ts[5] - ' ');
    return Time::today() + usec(micros_since_midnight);
  }

  static inline void get_denom(char code, int& numerator, double& denom)
  {
    switch(code) {
    case '3' : numerator = 1; denom = 8.0; break;
    case '4' : numerator = 2; denom = 16.0; break;
    case '5' : numerator = 2; denom = 32.0; break;
    case '6' : numerator = 2; denom = 64.0; break;
    case '7' : numerator = 3; denom = 128.0; break;
    case '8' : numerator = 3; denom = 256.0; break;
    case 'A' : numerator = 1; denom = 10.0; break;
    case 'B' : numerator = 2; denom = 100.0; break;
    case 'C' : numerator = 3; denom = 1000.0; break;
    case 'D' : numerator = 4; denom = 10000.0; break;
    case 'E' : numerator = 5; denom = 100000.0; break;
    case 'F' : numerator = 6; denom = 1000000.0; break;
    case 'G' : numerator = 7; denom = 10000000.0; break;
    case 'H' : numerator = 8; denom = 100000000.0; break;
    case 'I' :
    default  : numerator = 0; denom = 1; break;
    }
  }

  static inline double parse_price(const char* price, char code, int len)
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
  CQS_CTS_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "CQS_CTS_Handler::parse";

    if(len < 20) {
      send_alert("%s: %s Runt packet received, len=%d", where, m_name.c_str(), len);
      return 0;
    }

    if(buf[0] != start_of_header) {
      send_alert("%s: %s First character not expected SOH char", where, m_name.c_str());
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

    channel_info_t& channel_info = m_channel_info_map[context];
    const common_message_header& header = reinterpret_cast<const common_message_header&>(*(buf+1));

    uint64_t seq_no;
    if(header.message_header_identifier == 'B')
    {
      const message_header& msg_header = reinterpret_cast<const message_header&>(*(buf+1));
      seq_no = parse_seqno(msg_header.seq_no);
    }
    else
    {
      const legacy_message_header& legacy_header = reinterpret_cast<const legacy_message_header&>(*(buf+1));
      seq_no = parse_seqno(legacy_header.seq_no);
    }

    if(header.category == 'C' && header.type == 'T') {
      ++seq_no; // treat it as the next seq_no
    }

    if(header.category == 'C' && header.type == 'L') {
      // Control, reset seq_no
      channel_info.seq_no = seq_no;
    } else if(channel_info.seq_no > seq_no) {
      // duplicate
      return len;
    } else if(channel_info.seq_no < seq_no) {
      if(channel_info.begin_recovery(this, buf, len, seq_no, 0)) {
        return len;
      }
    }

    channel_info.seq_no = seq_no;
    parse2(context, buf, len);
    // In CQS & UTP, it is parse2 that determines the next seq_no
    channel_info.last_update = Time::currentish_time();

    if(!channel_info.queue.empty()) {
      channel_info.process_recovery(this);
    }

    return len;
  }

  size_t
  CQS_CTS_Handler::parse2(size_t context, const char* buf, size_t len)
  {
    //const char* where = "CQS_CTS_Handler::parse2";

    channel_info_t& channel_info = m_channel_info_map[context];
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
      char participant_id;
      Time exch_timestamp;
      if(header.message_header_identifier == 'B')
      {
        const message_header& msg_header = reinterpret_cast<const message_header&>(*(buf+1));
        header_length = sizeof(msg_header);
        participant_id = msg_header.participant_id;
        exch_timestamp = parse_timestamp(msg_header.cts_or_cqs_timestamp, m_micros_exch_time);
      }
      else
      {
        const legacy_message_header& legacy_header = reinterpret_cast<const legacy_message_header&>(*(buf+1));
        header_length = sizeof(legacy_message_header);
        participant_id = legacy_header.participant_id;
        exch_timestamp = parse_timestamp_legacy(legacy_header.timestamp, m_micros_exch_time);
      }
      size_t message_length = header_length;

      m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;

      if(header.category == 'E') {
        // Listed Equity
        if(channel_info.cqs) {
          if(header.type == 'B') {
            // long quote
            const long_quote& quote = reinterpret_cast<const long_quote&>(*(buf+1+header_length));
            message_length += sizeof(long_quote);
            if     (quote.nbbo_indicator == '4') { message_length += 58; }
            else if(quote.nbbo_indicator == '6') { message_length += 28; }
            if     (quote.finra_bbo_indicator == '3') { message_length += 56; }

            symbol_copy(symbol, quote.symbol, 11);
            symbol_info_t si = product_id_of_symbol(participant_id, symbol, 11);

            if (si.id != Product::invalid_product_id) {
              //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
              m_quote.m_id = si.id;
              m_quote.m_exch = si.exch;
              m_quote.set_time(exch_timestamp);
              m_quote.m_bid = parse_price(quote.bid_price, quote.bid_price_denominator, 12);
              m_quote.m_ask = parse_price(quote.offer_price, quote.offer_price_denominator, 12);
              if(m_quote.m_ask == 0) {
                m_quote.m_ask = invalid_ask;
              }
              m_quote.m_bid_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.bid_size_in_lots, 7);
              m_quote.m_ask_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.offer_size_in_lots, 7);
              m_quote.m_cond = quote.quote_condition;

              if (m_qbdf_builder) {
                if (!m_exclude_quotes) {
                  gen_qbdf_quote_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                        m_micros_exch_time_delta, participant_id,
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
            // short quote
            const short_quote& quote = reinterpret_cast<const short_quote&>(*(buf+1+header_length));
            message_length += sizeof(short_quote);
            if     (quote.nbbo_indicator == '4') { message_length += 58; }
            else if(quote.nbbo_indicator == '6') { message_length += 28; }
            if     (quote.finra_bbo_indicator == '3') { message_length += 56; }

            symbol_copy(symbol, quote.symbol, 3);
            symbol_info_t si = product_id_of_symbol(participant_id, symbol, 3);

            if (si.id != Product::invalid_product_id) {
              //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
              m_quote.m_id = si.id;
              m_quote.m_exch = si.exch;
              m_quote.set_time(exch_timestamp);
              m_quote.m_bid = parse_price(quote.bid_short_price, quote.bid_price_denominator, 8);
              m_quote.m_ask = parse_price(quote.offer_short_price, quote.offer_price_denominator, 8);
              if(m_quote.m_ask == 0) {
                m_quote.m_ask = invalid_ask;
              }
              m_quote.m_bid_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.bid_size_in_lots, 3);
              m_quote.m_ask_size = m_exch_lot_size[m_quote.m_exch.index()] * parse_int(quote.offer_size_in_lots, 3);
              m_quote.m_cond = quote.quote_condition;

              if (m_qbdf_builder) {
                if (!m_exclude_quotes) {
                  gen_qbdf_quote_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                        m_micros_exch_time_delta, participant_id,
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
        } else {
          if(header.type == 'I') {
            // short trade
            const short_trade& trade = reinterpret_cast<const short_trade&>(*(buf+1+header_length));
            message_length += sizeof(short_trade);

            symbol_copy(symbol, trade.symbol, 3);
            symbol_info_t si = product_id_of_symbol(participant_id, symbol, 3);

            if (si.id != Product::invalid_product_id) {
              //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
              m_trade.m_id = si.id;
              m_trade.m_exch = si.exch;
              m_trade.set_time(exch_timestamp);
              m_trade.m_price = parse_price(trade.trade_price, trade.price_denominator_indicator, 8);
              m_trade.m_size = parse_int(trade.trade_volume, 4);
              m_trade.set_side(0);
              memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
              m_trade.m_trade_qualifier[0] = trade.sale_condition;
              m_trade.m_correct = '0';
              //m_logger->log_printf(Log::INFO, "Got short trade for %s  price=%f  vol=%d", symbol, m_trade.m_price, m_trade.m_size);

              if (m_qbdf_builder) {
                if (!m_exclude_trades) {
                  gen_qbdf_trade_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                        m_micros_exch_time_delta, participant_id, 0, '0',
                                        string(m_trade.m_trade_qualifier, 4), m_trade.m_size,
                                        MDPrice::from_fprice(m_trade.m_price));
                }
              } else {
                send_trade_update();
              }
            }
          } else if(header.type == 'B') {
            // long trade
            const long_trade& trade = reinterpret_cast<const long_trade&>(*(buf+1+header_length));
            message_length += sizeof(long_trade);

            symbol_copy(symbol, trade.symbol, 11);
            symbol_info_t si = product_id_of_symbol(participant_id, symbol, 11);

            if (si.id != Product::invalid_product_id) {
              //m_logger->log_printf(Log::INFO, "Got long quote for %s", symbol);
              m_trade.m_id = si.id;
              m_trade.m_exch = si.exch;
              m_trade.set_time(exch_timestamp);
              m_trade.m_price = parse_price(trade.trade_price, trade.price_denominator_indicator, 12);
              m_trade.m_size = parse_int(trade.trade_volume, 9);
              m_trade.set_side(0);
              memcpy(m_trade.m_trade_qualifier, trade.sale_condition, sizeof(m_trade.m_trade_qualifier));
              m_trade.m_correct = '0';
              //m_logger->log_printf(Log::INFO, "Got long trade for %s  price=%f  vol=%d", symbol, m_trade.m_price, m_trade.m_size);

              if (m_qbdf_builder) {
                if (!m_exclude_trades) {
                  gen_qbdf_trade_micros(m_qbdf_builder, si.id, m_micros_recv_time,
                                        m_micros_exch_time_delta, participant_id, 0, '0',
                                        string(m_trade.m_trade_qualifier, 4), m_trade.m_size,
                                        MDPrice::from_fprice(m_trade.m_price));
                }
              } else {
                send_trade_update();
              }
            }
          } else if (header.type == 'P') {
            // correction
            const correction& trade_corr = reinterpret_cast<const correction&>(*(buf+1+header_length));
            message_length += sizeof(correction);

            symbol_copy(symbol, trade_corr.symbol, 11);
            symbol_info_t si = product_id_of_symbol(participant_id, symbol, 11);

            if (si.id != Product::invalid_product_id) {
              m_logger->log_printf(Log::INFO, "Got correction for %s", symbol);
              m_logger->sync_flush();

              uint64_t orig_seq_no = parse_int(trade_corr.orig_seq_no, 9);

              double orig_price =  parse_price(trade_corr.orig_trade_price,
                                               trade_corr.orig_price_denominator_indicator, 12);
              int orig_size = parse_int(trade_corr.orig_trade_volume, 9);
              string orig_cond = string(trade_corr.orig_sale_condition, 4);

              double corr_price =  parse_price(trade_corr.corr_trade_price,
                                               trade_corr.corr_price_denominator_indicator, 12);
              int corr_size = parse_int(trade_corr.corr_trade_volume, 9);
              string corr_cond = string(trade_corr.corr_sale_condition, 4);

              if (m_qbdf_builder) {
                gen_qbdf_cancel_correct(m_qbdf_builder, si.id,
                                        m_micros_exch_time / Time_Constants::micros_per_mili,
                                        0, string(symbol), "CORRECTION", orig_seq_no, orig_price,
                                        orig_size, orig_cond, corr_price, corr_size, corr_cond);
              } else {
                //don't try to apply correction in real-time
                //send_trade_update();
              }
            }
          } else if (header.type == 'Q') {
            // cancel/error
            const cancel_error& trade_cancel = reinterpret_cast<const cancel_error&>(*(buf+1+header_length));
            message_length += sizeof(cancel_error);

            symbol_copy(symbol, trade_cancel.symbol, 11);
            symbol_info_t si = product_id_of_symbol(participant_id, symbol, 11);

            if (si.id != Product::invalid_product_id) {
              //m_logger->log_printf(Log::INFO, "Got cancel_error for %s", symbol);
              //m_logger->sync_flush();

              uint64_t orig_seq_no = parse_int(trade_cancel.orig_seq_no, 9);
              double orig_price =  parse_price(trade_cancel.orig_trade_price,
                                               trade_cancel.orig_price_denominator_indicator, 12);
              int orig_size = parse_int(trade_cancel.orig_trade_volume, 9);
              string orig_cond = string(trade_cancel.orig_sale_condition, 4);

              if (m_qbdf_builder) {
                gen_qbdf_cancel_correct(m_qbdf_builder, si.id,
                                        m_micros_exch_time / Time_Constants::micros_per_mili,
                                        0, string(symbol), "CANCEL", orig_seq_no, orig_price,
                                        orig_size, orig_cond, 0, 0, "");
              } else {
                //don't try to apply cancel_error in real-time
                //send_trade_update();
              }
            }
          }

        }
      } else {
        // We don't care about any other kind of message
        // skip
      }

      if(!(header.category == 'C' && (header.type == 'T' || header.type == 'L'))) {
        ++channel_info.seq_no;
      }

      buf += 1 + message_length;
      // search for next message
      //if(*buf != unit_separation && *buf != end_of_text && header.category != 'C' && header.category != 'A') {
      //  m_logger->log_printf(Log::WARN, "%s: %s Invalid computation of next message length for %c %c", "CQS_CTS_Handler::parse2", channel_info.name.c_str(), header.category, header.type);
      //}
      while(buf < buf_end && *buf != unit_separation && *buf != end_of_text ) {
        ++buf;
      }
      if(*buf == end_of_text || buf >= buf_end) {
        break;
      }
    }

    channel_info.last_update = Time::currentish_time();
    return len;
  }

  size_t
  CQS_CTS_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
  {
    //char symbol[16];
    const char* buf_end = buf + len;

    bool cqs = context <= 23;

    for(;;) {
      const common_message_header& header = reinterpret_cast<const common_message_header&>(*(buf+1));

      size_t header_length;
      char participant_id;
      Time exch_timestamp;
      const char* seq_no;
      uint64_t micros_exch_time;
      if(header.message_header_identifier == 'B')
      {
        const message_header& msg_header = reinterpret_cast<const message_header&>(*(buf+1));
        header_length = sizeof(msg_header);
        participant_id = msg_header.participant_id;
        exch_timestamp = parse_timestamp(msg_header.cts_or_cqs_timestamp, micros_exch_time);
        seq_no = msg_header.seq_no;
      }
      else
      {
        const legacy_message_header& legacy_header = reinterpret_cast<const legacy_message_header&>(*(buf+1));
        header_length = sizeof(legacy_message_header);
        participant_id = legacy_header.participant_id;
        exch_timestamp = parse_timestamp_legacy(legacy_header.timestamp, micros_exch_time);
        seq_no = legacy_header.seq_no;
      }
      size_t message_length = header_length;

      char time_buf[32];
      Time_Utils::print_time(time_buf, exch_timestamp, Timestamp::MICRO);

      log.printf(" timestamp:%s  seq_no:%.9s  exch:%c\n", time_buf, seq_no, participant_id);

      if(header.category == 'E') {
        // Listed Equity
        if(cqs) {
          if(header.type == 'B') {
            // long quote
            const long_quote& quote = reinterpret_cast<const long_quote&>(*(buf+1+message_length));
            message_length += sizeof(long_quote);
            if     (quote.nbbo_indicator == '4') { message_length += 58; }
            else if(quote.nbbo_indicator == '6') { message_length += 28; }
            if     (quote.finra_bbo_indicator == '3') { message_length += 56; }

          } else if(header.type == 'D') {
            // short quote
            const short_quote& quote = reinterpret_cast<const short_quote&>(*(buf+1+message_length));
            message_length += sizeof(short_quote);
            if     (quote.nbbo_indicator == '4') { message_length += 58; }
            else if(quote.nbbo_indicator == '6') { message_length += 28; }
            if     (quote.finra_bbo_indicator == '3') { message_length += 56; }

          }
        } else {
          if(header.type == 'I') {
            // short trade
            const short_trade& trade = reinterpret_cast<const short_trade&>(*(buf+1+message_length));
            message_length += sizeof(short_trade);

            log.printf("   short_trade symbol:%.3s exch:%c sale_cond:%c trade_volume:%.4s price_denom:%c trade_price:%.8s\n",
                       trade.symbol, participant_id, trade.sale_condition, trade.trade_volume, trade.price_denominator_indicator,
                       trade.trade_price);

          } else if(header.type == 'B') {
            // long trade
            const long_trade& trade = reinterpret_cast<const long_trade&>(*(buf+1+message_length));
            message_length += sizeof(long_trade);

            log.printf("   long_trade  symbol:%.11s exch:%c sale_cond:%.4s trade_volume:%.9s price_denom:%c trade_price:%.12s\n",
                       trade.symbol, participant_id, trade.sale_condition, trade.trade_volume, trade.price_denominator_indicator,
                       trade.trade_price);


          } else if (header.type == 'P') {
            // correction
            //const correction& trade_corr = reinterpret_cast<const correction&>(*(buf+1+message_length));
            message_length += sizeof(correction);

          } else if (header.type == 'Q') {
            // cancel/error
            //const cancel_error& trade_cancel = reinterpret_cast<const cancel_error&>(*(buf+1+message_length));
            message_length += sizeof(cancel_error);
          }

        }
      } else {
        // We don't care about any other kind of message
        // skip
      }

      buf += 1 + message_length;
      // search for next message
      //if(*buf != unit_separation && *buf != end_of_text && header.category != 'C' && header.category != 'A') {
      //  m_logger->log_printf(Log::WARN, "%s: %s Invalid computation of next message length for %c %c", "CQS_CTS_Handler::parse2", channel_info.name.c_str(), header.category, header.type);
      //}
      while(buf < buf_end && *buf != unit_separation && *buf != end_of_text ) {
        ++buf;
      }
      if(*buf == end_of_text || buf >= buf_end) {
        break;
      }
    }

    return len;
  }

  void
  CQS_CTS_Handler::send_quote_update()
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
  CQS_CTS_Handler::send_trade_update()
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

  void
  CQS_CTS_Handler::reset(const char *)
  {
    if (m_qbdf_builder) {
      gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, ' ', -1);
    }
  }

  void
  CQS_CTS_Handler::reset(size_t context, const char* msg)
  {
    if (m_qbdf_builder) {
      gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, ' ', context);
    }
  }

  CQS_CTS_Handler::symbol_info_t
  CQS_CTS_Handler::product_id_of_symbol(char participant_id, const char* symbol, int len)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol);
      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string);
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: unable to create or find id for symbol [%s]",
                             "CQS_CTS_Handler::product_id_of_symbol", symbol_string.c_str());
        m_logger->sync_flush();
        return CQS_CTS_Handler::symbol_info_t(Product::invalid_product_id, ExchangeID::MULTI);
      }
      ExchangeID exch = m_qbdf_builder->get_hf_exch_id_by_qbdf_exch(participant_id);
      return CQS_CTS_Handler::symbol_info_t(symbol_id, exch);
    }

    symbol_map_t* symbol_map = m_symbol_maps[participant_id];
    if(symbol_map) {
      symbol_map_t::const_iterator prod_i = symbol_map->find(symbol);
      if(prod_i != symbol_map->end()) {
        return CQS_CTS_Handler::symbol_info_t(prod_i->second.id, prod_i->second.exch);
      }
    }

    return CQS_CTS_Handler::symbol_info_t(Product::invalid_product_id, ExchangeID::MULTI);
  }


  void
  CQS_CTS_Handler::init(const string& name,const Parameter& params)
  {
    Handler::init(name, params);
    m_name = name;

    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);
    
    m_exch_lot_size.resize(ExchangeID::size, 100.0);

    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);
  }

  void
  CQS_CTS_Handler::start(void)
  {
  }

  void
  CQS_CTS_Handler::stop(void)
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
  }

  void
  CQS_CTS_Handler::subscribe_product(ProductID id, ExchangeID exch,
                                     const string& mdSymbol, const string& mdExch)
  {
    mutex_t::scoped_lock lock(m_mutex);
    if(mdExch.size() != 1) {
      m_logger->log_printf(Log::ERROR, "CQS_CTS_Handler::subscribe_product mdExch '%s' for symbol %s does not map to a valid participant id", mdExch.c_str(), mdSymbol.c_str());
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

  CQS_CTS_Handler::CQS_CTS_Handler(Application* app) :
    Handler(app, "CQS_CTS")
  {
    m_symbol_maps.resize(256, 0);
  }

  CQS_CTS_Handler::~CQS_CTS_Handler()
  {
  }
  
  
  static bool test_CQS_CTS(ILogger& logger)
  {
    bool pass = true;
    
    uint64_t micros_since_midnight = 0;
    parse_timestamp("!qkJrC", micros_since_midnight);
    UNIT_EXPECT_UINT64("parse_timestamp", 14400000000ULL, static_cast<unsigned long long>(micros_since_midnight));
    
    parse_timestamp("$Gt2a ", micros_since_midnight);
    UNIT_EXPECT_UINT64("parse_timestamp", 34200000000ULL, static_cast<unsigned long long>(micros_since_midnight));
    
    parse_timestamp("$fNx&O", micros_since_midnight);
    UNIT_EXPECT_UINT64("parse_timestamp", 36693015317ULL, static_cast<unsigned long long>(micros_since_midnight));
    
    parse_timestamp("%mMjWR", micros_since_midnight);
    UNIT_EXPECT_UINT64("parse_timestamp", 45000000000ULL, static_cast<unsigned long long>(micros_since_midnight));
    
    parse_timestamp("'J0lLM", micros_since_midnight);
    UNIT_EXPECT_UINT64("parse_timestamp", 57600000000ULL, static_cast<unsigned long long>(micros_since_midnight));
    
    return pass;
  }
  
  static bool r1 = UnitTestFactory::add_test("CQS_CTS_Handler", UnitTestFactory::TestFunction(&test_CQS_CTS));
  
}
