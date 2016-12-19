#include <Data/QBDF_Util.h>
#include <Util/CSV_Parser.h>

namespace hf_core {

  vector<uint32_t>
  get_qbdf_message_sizes(string version)
  {
    string qbdf_spec_summary_file = "/q/coredata/exec/conf/qbdf/" + version + "/qbdf_spec_summary.csv";
    CSV_Parser csv_reader(qbdf_spec_summary_file);

    int enum_index_posn = csv_reader.column_index("enum_index");
    int size_posn = csv_reader.column_index("size");

    vector<uint32_t> qbdf_message_sizes(255, 0);
    CSV_Parser::state_t s = csv_reader.read_row();
    while (s == CSV_Parser::OK) {
      unsigned int enum_index = strtol(csv_reader.get_column(enum_index_posn), 0, 10);
      if(enum_index >= 255) {
        throw runtime_error("get_qbdf_message_sizes enum_index >= 255 from " + qbdf_spec_summary_file);
      }

      uint32_t msg_size = strtol(csv_reader.get_column(size_posn), 0, 10);
      qbdf_message_sizes[enum_index] = msg_size;
      s = csv_reader.read_row();
    }

    return qbdf_message_sizes;
  }

  bool gen_qbdf_quote(QBDF_Builder* p_builder, QuoteUpdate* p_quote,
                      uint32_t ms_since_midnight, char exch,
                      char source, char corr) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);
    return gen_qbdf_quote_rec_time(p_builder, p_quote, rec_time, exch, source, corr);
  }

  bool gen_qbdf_quote_rec_time(QBDF_Builder* p_builder, QuoteUpdate* p_quote,
                               uint32_t rec_time, char exch, char source, char corr) {
    uint32_t rec_size;
    void* p_quote_rec = NULL;

    qbdf_quote_small quote_small_rec;
    memset(&quote_small_rec, 0, sizeof(qbdf_quote_small));

    qbdf_quote_large quote_large_rec;
    memset(&quote_large_rec, 0, sizeof(qbdf_quote_large));

    qbdf_quote_price* p_quote_price_rec = NULL;

    if (p_quote->m_bid_size > USHRT_MAX || p_quote->m_ask_size > USHRT_MAX) {
      rec_size = sizeof(qbdf_quote_large);
      quote_large_rec.sizes.bidSize = p_quote->m_bid_size;
      quote_large_rec.sizes.askSize = p_quote->m_ask_size;
      quote_large_rec.hdr.format = QBDFMsgFmt::QuoteLarge;
      p_quote_rec = &quote_large_rec;
      p_quote_price_rec = &quote_large_rec.qr;
    } else {
      rec_size = sizeof(qbdf_quote_small);
      quote_small_rec.sizes.bidSize = p_quote->m_bid_size;
      quote_small_rec.sizes.askSize = p_quote->m_ask_size;
      quote_small_rec.hdr.format = QBDFMsgFmt::QuoteSmall;
      p_quote_rec = &quote_small_rec;
      p_quote_price_rec = &quote_small_rec.qr;
    }

    p_quote_price_rec->time = rec_time % 100000;
    p_quote_price_rec->symbolId = p_quote->m_id;
    p_quote_price_rec->exch = exch;
    p_quote_price_rec->bidPrice = p_quote->m_bid;
    p_quote_price_rec->askPrice = p_quote->m_ask;
    p_quote_price_rec->cond = p_quote->m_cond;
    p_quote_price_rec->source = source;
    p_quote_price_rec->cancelCorrect = corr;

    if (p_builder->write_binary_record("", p_quote->m_id, rec_time, p_quote_rec, rec_size)) {
      return false;
    }
    return true;

  }

  bool gen_qbdf_quote_micros(QBDF_Builder* p_builder, uint16_t id,
                             uint64_t micros_since_midnight_exch, int32_t exch_time_delta,
                             char exch, char cond, MDPrice bid_price, MDPrice ask_price,
                             uint32_t bid_size, uint32_t ask_size, bool suppress_64) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight_exch);
    return gen_qbdf_quote_micros_qbdf_time(p_builder, id, qbdf_time, exch_time_delta, exch,
                                           cond, bid_price, ask_price, bid_size, ask_size,
					   suppress_64);
  }


  bool gen_qbdf_quote_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
                                       uint64_t qbdf_time, int32_t exch_time_delta,
                                       char exch, char cond, MDPrice bid_price, MDPrice ask_price,
                                       uint32_t bid_size, uint32_t ask_size, bool suppress_64) {
    if (!suppress_64 && (ask_price.needs_iprice64() or bid_price.needs_iprice64())) {
      qbdf_quote_iprice64 quote_rec;
      memset(&quote_rec, 0, sizeof(qbdf_quote_iprice64));

      quote_rec.common.format = QBDFMsgFmt::quote_iprice64;
      quote_rec.common.time = qbdf_time % 100000000;
      quote_rec.common.exch_time_delta = exch_time_delta;
      quote_rec.common.exch = exch;
      quote_rec.common.id = id;
      quote_rec.cond = cond;
      quote_rec.bid_price = bid_price.iprice64();
      quote_rec.bid_size = bid_size;
      quote_rec.ask_price = ask_price.iprice64();
      quote_rec.ask_size = ask_size;
      // Write out this record to the correct 1 minute binary file
      if (p_builder->write_binary_record_micros(qbdf_time, &quote_rec, sizeof(qbdf_quote_iprice64))) {
        return false;
      }
    } else {
      if (bid_size > USHRT_MAX || ask_size > USHRT_MAX) {
	qbdf_quote_large_iprice32 quote_large_rec;
	memset(&quote_large_rec, 0, sizeof(qbdf_quote_large_iprice32));
	
	quote_large_rec.common.format = QBDFMsgFmt::quote_large_iprice32;
	quote_large_rec.common.time = qbdf_time % 100000000;
	quote_large_rec.common.exch_time_delta = exch_time_delta;
	quote_large_rec.common.exch = exch;
	quote_large_rec.common.id = id;
	quote_large_rec.cond = cond;
	quote_large_rec.bid_price = bid_price.iprice32();
	quote_large_rec.bid_size = bid_size;
	quote_large_rec.ask_price = ask_price.iprice32();
	quote_large_rec.ask_size = ask_size;
	// Write out this record to the correct 1 minute binary file
	if (p_builder->write_binary_record_micros(qbdf_time, &quote_large_rec, sizeof(qbdf_quote_large_iprice32))) {
	  return false;
	}
      } else {
	qbdf_quote_small_iprice32 quote_small_rec;
	memset(&quote_small_rec, 0, sizeof(qbdf_quote_small_iprice32));
	
	quote_small_rec.common.format = QBDFMsgFmt::quote_small_iprice32;
	quote_small_rec.common.time = qbdf_time % 100000000;
	quote_small_rec.common.exch_time_delta = exch_time_delta;
	quote_small_rec.common.exch = exch;
	quote_small_rec.common.id = id;
	quote_small_rec.cond = cond;
	quote_small_rec.bid_price = bid_price.iprice32();
	quote_small_rec.bid_size = bid_size;
	quote_small_rec.ask_price = ask_price.iprice32();
	quote_small_rec.ask_size = ask_size;
	// Write out this record to the correct 1 minute binary file
	if (p_builder->write_binary_record_micros(qbdf_time, &quote_small_rec, sizeof(qbdf_quote_small_iprice32))) {
	  return false;
	}
      }
    }
    return true;
  }


  bool gen_qbdf_trade(QBDF_Builder* p_builder, TradeUpdate* p_trade,
                      uint32_t ms_since_midnight, char exch,
                      char source, char corr, char stop, char side) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);
    return gen_qbdf_trade_rec_time(p_builder, p_trade, rec_time, exch, source, corr, stop, side);
  }


  bool gen_qbdf_trade_rec_time(QBDF_Builder* p_builder, TradeUpdate* p_trade,
                               uint32_t rec_time, char exch,
                               char source, char corr, char stop, char side) {
    qbdf_trade trade_rec;
    memset(&trade_rec, 0, sizeof(qbdf_trade));

    trade_rec.hdr.format = QBDFMsgFmt::Trade;
    trade_rec.time = rec_time % 100000;
    trade_rec.symbolId = p_trade->m_id;
    trade_rec.price = p_trade->m_price;
    trade_rec.size = p_trade->m_size;
    trade_rec.exch = exch;
    trade_rec.stop = stop;
    trade_rec.correct = corr;
    trade_rec.source = source;
    trade_rec.side = side;
    memcpy(trade_rec.cond, p_trade->m_trade_qualifier, sizeof(trade_rec.cond));

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record("", p_trade->m_id, rec_time, &trade_rec, sizeof(qbdf_trade))) {
      return false;
    }
    return true;

  }


  bool gen_qbdf_trade_micros(QBDF_Builder* p_builder, uint16_t id,
                             uint64_t micros_since_midnight_exch, int32_t exch_time_delta,
                             char exch, char side, char corr, string cond,
                             uint32_t size, MDPrice price, bool suppress_64) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight_exch);
    return gen_qbdf_trade_micros_qbdf_time(p_builder, id, qbdf_time, exch_time_delta, exch,
                                           side, corr, cond, size, price, suppress_64);
  }


  bool gen_qbdf_trade_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
                                       uint64_t qbdf_time, int32_t exch_time_delta,
                                       char exch, char side, char corr, string cond,
                                       uint32_t size, MDPrice price, bool suppress_64) {
    if (!suppress_64 && price.needs_iprice64()) {
      qbdf_trade_iprice64 trade_rec;
      memset(&trade_rec, 0, sizeof(qbdf_trade_iprice64));

      trade_rec.common.format = QBDFMsgFmt::trade_iprice64;
      trade_rec.common.time = qbdf_time % 100000000;
      trade_rec.common.exch_time_delta = exch_time_delta;
      trade_rec.common.exch = exch;
      trade_rec.common.id = id;
      set_side(trade_rec, side);
      trade_rec.correct = corr;

      memset(trade_rec.cond, ' ', 4);
      memcpy(trade_rec.cond, cond.c_str(), min((int)cond.size(), 4));

      trade_rec.size = size;
      trade_rec.price = price.iprice64();
      
      // Write out this record to the correct 1 minute binary file
      if (p_builder->write_binary_record_micros(qbdf_time, &trade_rec, sizeof(qbdf_trade_iprice64))) {
	return false;
      }
    } else {
      qbdf_trade_iprice32 trade_rec;
      memset(&trade_rec, 0, sizeof(qbdf_trade_iprice32));

      trade_rec.common.format = QBDFMsgFmt::trade_iprice32;
      trade_rec.common.time = qbdf_time % 100000000;
      trade_rec.common.exch_time_delta = exch_time_delta;
      trade_rec.common.exch = exch;
      trade_rec.common.id = id;
      set_side(trade_rec, side);
      trade_rec.correct = corr;

      memset(trade_rec.cond, ' ', 4);
      memcpy(trade_rec.cond, cond.c_str(), min((int)cond.size(), 4));

      trade_rec.size = size;
      trade_rec.price = price.iprice32();
      
      // Write out this record to the correct 1 minute binary file
      if (p_builder->write_binary_record_micros(qbdf_time, &trade_rec, sizeof(qbdf_trade_iprice32))) {
	return false;
      }
    }
    return true;
  }

  bool gen_qbdf_trade_augmented(QBDF_Builder* p_builder, uint16_t id,
				uint64_t micros_since_midnight_exch, int32_t exch_time_delta,
				char exch, char side, char exec_flag, char corr, const char* cond, const char* misc,
                                char trade_status, char market_status,
				uint32_t size, MDPrice price, uint64_t order_age, bool clean) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight_exch);
    return gen_qbdf_trade_augmented_qbdf_time(p_builder, id, qbdf_time, exch_time_delta, exch,
					      side, exec_flag, corr, cond, misc,
                                              trade_status, market_status,
                                              size, price, order_age, clean);
  }


  bool gen_qbdf_trade_augmented_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					  uint64_t qbdf_time, int32_t exch_time_delta,
					  char exch, char side, char exec_flag, char corr, const char* cond, const char* misc,
                                          char trade_status, char market_status,
					  uint32_t size, MDPrice price, uint64_t order_age, bool clean) {
    qbdf_trade_augmented trade_rec;
    memset(&trade_rec, 0, sizeof(qbdf_trade_augmented));
    
    trade_rec.common.format = QBDFMsgFmt::trade_augmented;
    trade_rec.common.time = qbdf_time % 100000000;
    trade_rec.common.exch_time_delta = exch_time_delta;
    trade_rec.common.exch = exch;
    trade_rec.common.id = id;
    set_side(trade_rec, side);
    trade_rec.set_exec_flag(exec_flag);
    trade_rec.set_clean_flag(clean);
    trade_rec.correct = corr;
    trade_rec.trade_status = trade_status;
    trade_rec.market_status = market_status;
    
    memcpy(trade_rec.cond, cond, 4);
    memcpy(trade_rec.misc, misc, 2);
    
    trade_rec.size = size;
    trade_rec.price = price.iprice32();
    trade_rec.order_age = order_age;
    
    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &trade_rec, sizeof(qbdf_trade_augmented))) {
      return false;
    }
    return true;
  }

  bool gen_qbdf_cross_trade_micros(QBDF_Builder* p_builder, uint16_t id,
                                   uint64_t micros_since_midnight, int32_t exch_time_delta,
                                   char exch, char side, char corr, char cross_type,
                                   uint32_t size, MDPrice price) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);

    qbdf_trade_iprice32 trade_rec;
    memset(&trade_rec, 0, sizeof(qbdf_trade_iprice32));

    trade_rec.common.format = QBDFMsgFmt::trade_iprice32;
    trade_rec.common.time = qbdf_time % 100000000;
    trade_rec.common.exch_time_delta = exch_time_delta;
    trade_rec.common.exch = exch;
    trade_rec.common.id = id;
    set_side(trade_rec, side);
    trade_rec.correct = corr;
    switch(cross_type) {
    case 'O':
      memcpy(trade_rec.cond, "O   ", 4);
      break;
    case 'C':
      memcpy(trade_rec.cond, "6   ", 4);
      break;
    case 'H':
    case 'I':
    default:
      memset(trade_rec.cond, ' ', 4);
      break;
    }

    trade_rec.size = size;
    trade_rec.price = price.iprice32();

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &trade_rec, sizeof(qbdf_trade_iprice32))) {
      return false;
    }
    return true;

  }


  bool gen_qbdf_cross_trade(QBDF_Builder* p_builder, TradeUpdate* p_trade,
                            uint32_t ms_since_midnight, char exch, char source,
                            char corr, char stop, char side, char cross_type) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);

    qbdf_trade trade_rec;
    memset(&trade_rec, 0, sizeof(qbdf_trade));

    trade_rec.hdr.format = QBDFMsgFmt::Trade;
    trade_rec.time = rec_time % 100000;
    trade_rec.symbolId = p_trade->m_id;
    trade_rec.price = p_trade->m_price;
    trade_rec.size = p_trade->m_size;
    trade_rec.exch = exch;
    trade_rec.stop = stop;
    trade_rec.correct = corr;
    trade_rec.source = source;
    trade_rec.side = side;
    switch(cross_type) {
    case 'O':
      memcpy(trade_rec.cond, "O   ", 4);
      break;
    case 'C':
      memcpy(trade_rec.cond, "6   ", 4);
      break;
    case 'H':
    case 'I':
    default:
      break;
    }

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record("", p_trade->m_id, rec_time, &trade_rec, sizeof(qbdf_trade))) {
      return false;
    }
    return true;

  }


  bool gen_qbdf_imbalance(QBDF_Builder* p_builder, AuctionUpdate* p_auction,
                          uint32_t ms_since_midnight, char exch) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);

    qbdf_imbalance imbalance_rec;
    memset(&imbalance_rec, 0, sizeof(qbdf_imbalance));

    imbalance_rec.hdr.format = QBDFMsgFmt::Imbalance;
    imbalance_rec.time = rec_time % 100000;
    imbalance_rec.symbolId = p_auction->m_id;
    imbalance_rec.ref_price = p_auction->m_ref_price;
    imbalance_rec.near_price = p_auction->m_near_price;
    imbalance_rec.far_price = p_auction->m_far_price;
    imbalance_rec.paired_size = p_auction->m_paired_size;
    imbalance_rec.imbalance_size = p_auction->m_imbalance_size;
    imbalance_rec.imbalance_side = p_auction->m_imbalance_direction;
    imbalance_rec.cross_type = p_auction->m_indicative_price_flag;
    imbalance_rec.exch = exch;

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record("", p_auction->m_id, rec_time, &imbalance_rec, sizeof(qbdf_imbalance))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_imbalance_micros(QBDF_Builder* p_builder, uint16_t id,
                                 uint64_t micros_since_midnight, int32_t exch_time_delta,
                                 char exch, MDPrice ref_price, MDPrice near_price,
                                 MDPrice far_price, uint32_t paired_size,
                                 uint32_t imbalance_size, char imbalance_side,
                                 char indicative_flag, bool suppress_64) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);
    return gen_qbdf_imbalance_micros_qbdf_time(p_builder, id, qbdf_time, exch_time_delta,
                                               exch, ref_price, near_price, far_price,
                                               paired_size, imbalance_size, imbalance_side,
                                               indicative_flag, suppress_64);
  }

  bool gen_qbdf_imbalance_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
                                           uint64_t qbdf_time, int32_t exch_time_delta,
                                           char exch, MDPrice ref_price, MDPrice near_price,
                                           MDPrice far_price, uint32_t paired_size,
                                           uint32_t imbalance_size, char imbalance_side,
                                           char indicative_flag, bool suppress_64) {

    if (!suppress_64 && (ref_price.needs_iprice64() or near_price.needs_iprice64() or far_price.needs_iprice64())) {
      qbdf_imbalance_iprice64 imbalance_rec;
      memset(&imbalance_rec, 0, sizeof(qbdf_imbalance_iprice64));

      imbalance_rec.common.format = QBDFMsgFmt::imbalance_iprice64;
      imbalance_rec.common.time = qbdf_time % 100000000;
      imbalance_rec.common.exch_time_delta = exch_time_delta;
      imbalance_rec.common.exch = exch;
      imbalance_rec.common.id = id;
      set_side(imbalance_rec, imbalance_side);

      imbalance_rec.cross_type = indicative_flag;
      imbalance_rec.ref_price = ref_price.iprice64();
      imbalance_rec.near_price = near_price.iprice64();
      imbalance_rec.far_price = far_price.iprice64();
      imbalance_rec.paired_size = paired_size;
      imbalance_rec.imbalance_size = imbalance_size;
      
      // Write out this record to the correct 1 minute binary file
      if (p_builder->write_binary_record_micros(qbdf_time, &imbalance_rec, sizeof(qbdf_imbalance_iprice64))) {
	return false;
      }
    } else {
      qbdf_imbalance_iprice32 imbalance_rec;
      memset(&imbalance_rec, 0, sizeof(qbdf_imbalance_iprice32));

      imbalance_rec.common.format = QBDFMsgFmt::imbalance_iprice32;
      imbalance_rec.common.time = qbdf_time % 100000000;
      imbalance_rec.common.exch_time_delta = exch_time_delta;
      imbalance_rec.common.exch = exch;
      imbalance_rec.common.id = id;
      set_side(imbalance_rec, imbalance_side);

      imbalance_rec.cross_type = indicative_flag;
      imbalance_rec.ref_price = ref_price.iprice32();
      imbalance_rec.near_price = near_price.iprice32();
      imbalance_rec.far_price = far_price.iprice32();
      imbalance_rec.paired_size = paired_size;
      imbalance_rec.imbalance_size = imbalance_size;
     
      // Write out this record to the correct 1 minute binary file
      if (p_builder->write_binary_record_micros(qbdf_time, &imbalance_rec, sizeof(qbdf_imbalance_iprice32))) {
	return false;
      }
    }
    return true;
  }


  bool gen_qbdf_order_add(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                          uint32_t ms_since_midnight, char exch,
                          char side, uint32_t size, float price) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);
    return gen_qbdf_order_add_rec_time(p_builder, id, order_id, rec_time, exch, side, size, price);
  }


  bool gen_qbdf_order_add_rec_time(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                                   uint32_t rec_time, char exch, char side, uint32_t size, float price) {

    qbdf_order_add_small small_order_rec;
    memset(&small_order_rec, 0, sizeof(qbdf_order_add_small));

    qbdf_order_add_large large_order_rec;
    memset(&large_order_rec, 0, sizeof(qbdf_order_add_large));

    qbdf_order_add_price* order_price_rec = NULL;

    size_t rec_size;
    void* p_data = NULL;

    if (size > USHRT_MAX) {
      large_order_rec.hdr.format = QBDFMsgFmt::OrderAddLarge;
      large_order_rec.sz.size = size;
      order_price_rec = &large_order_rec.pr;
      p_data = &large_order_rec;
      rec_size = sizeof(qbdf_order_add_large);
    } else {
      small_order_rec.hdr.format = QBDFMsgFmt::OrderAddSmall;
      small_order_rec.sz.size = size;
      order_price_rec = &small_order_rec.pr;
      p_data = &small_order_rec;
      rec_size = sizeof(qbdf_order_add_small);
    }

    order_price_rec->time = rec_time % 100000;
    order_price_rec->symbol_id = id;
    order_price_rec->order_id = order_id;
    order_price_rec->exch = exch;
    order_price_rec->side = side;
    order_price_rec->price = price;

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record("", id, rec_time, p_data, rec_size)) {
      return false;
    }
    return true;
  }

  bool gen_qbdf_order_add_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                 uint64_t micros_since_midnight, int32_t exch_time_delta,
                                 char exch, char side, uint32_t size, MDPrice price) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);
    return gen_qbdf_order_add_micros_qbdf_time(p_builder, id, order_id, qbdf_time, exch_time_delta,
                                               exch, side, size, price);
  }


  bool gen_qbdf_order_add_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                           uint64_t qbdf_time, int32_t exch_time_delta,
                                           char exch, char side, uint32_t size,
                                           MDPrice price, bool delta_in_millis) {
    qbdf_order_add_iprice32 order_rec;
    memset(&order_rec, 0, sizeof(qbdf_order_add_iprice32));

    order_rec.common.format = QBDFMsgFmt::order_add_iprice32;
    order_rec.common.time = qbdf_time % 100000000;
    order_rec.common.exch_time_delta = exch_time_delta;
    order_rec.common.exch = exch;
    order_rec.common.id = id;
    set_side(order_rec, side);
    set_delta_in_millis(order_rec, delta_in_millis);
    order_rec.order_id = order_id;
    order_rec.size = size;
    order_rec.price = price.iprice32();

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &order_rec, sizeof(qbdf_order_add_iprice32))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_order_exec(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                           uint32_t ms_since_midnight, char exch, uint32_t size, float price) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);
    return gen_qbdf_order_exec_rec_time(p_builder, id, order_id, rec_time, exch, size, price);
  }


  bool gen_qbdf_order_exec_rec_time(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                                    uint32_t rec_time, char exch, uint32_t size, float price) {
    qbdf_order_exec order_rec;
    memset(&order_rec, 0, sizeof(qbdf_order_exec));

    order_rec.hdr.format = QBDFMsgFmt::OrderExec;
    order_rec.time = rec_time % 100000;
    order_rec.symbol_id = id;
    order_rec.order_id = order_id;
    order_rec.exch = exch;
    order_rec.price = price;
    order_rec.size = size;

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record("", id, rec_time, &order_rec, sizeof(qbdf_order_exec))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_order_exec_cond_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                 uint64_t micros_since_midnight, int32_t exch_time_delta,
                                 char exch, char side, const char* cond,
                                 uint32_t size, MDPrice price) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);
    return gen_qbdf_order_exec_cond_micros_qbdf_time(p_builder, id, order_id, qbdf_time,
                                                exch_time_delta, exch, side, cond, size, price);
  }


  bool gen_qbdf_order_exec_cond_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                           uint64_t qbdf_time, int32_t exch_time_delta,
                                           char exch, char side, const char* cond,
                                           uint32_t size, MDPrice price, bool delta_in_millis) {
    qbdf_order_exec_iprice32_cond order_rec;
    memset(&order_rec, 0, sizeof(qbdf_order_exec_iprice32_cond));

    order_rec.common.format = QBDFMsgFmt::order_exec_iprice32_cond;
    order_rec.common.time = qbdf_time % 100000000;
    order_rec.common.exch_time_delta = exch_time_delta;
    order_rec.common.exch = exch;
    order_rec.common.id = id;
    set_side(order_rec, side);
    set_delta_in_millis(order_rec, delta_in_millis);
    memcpy(order_rec.cond, cond, sizeof(order_rec.cond));
    order_rec.order_id = order_id;
    order_rec.size = size;
    order_rec.price = price.iprice32();

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &order_rec, sizeof(qbdf_order_exec_iprice32_cond))) {
      return false;
    }
    return true;
  }

  bool gen_qbdf_order_exec_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                 uint64_t micros_since_midnight, int32_t exch_time_delta,
                                 char exch, char side, char cond,
                                 uint32_t size, MDPrice price) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);
    return gen_qbdf_order_exec_micros_qbdf_time(p_builder, id, order_id, qbdf_time,
                                                exch_time_delta, exch, side, cond, size, price);
  }


  bool gen_qbdf_order_exec_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                           uint64_t qbdf_time, int32_t exch_time_delta,
                                           char exch, char side, char cond,
                                           uint32_t size, MDPrice price, bool delta_in_millis) {
    qbdf_order_exec_iprice32 order_rec;
    memset(&order_rec, 0, sizeof(qbdf_order_exec_iprice32));

    order_rec.common.format = QBDFMsgFmt::order_exec_iprice32;
    order_rec.common.time = qbdf_time % 100000000;
    order_rec.common.exch_time_delta = exch_time_delta;
    order_rec.common.exch = exch;
    order_rec.common.id = id;
    set_side(order_rec, side);
    set_delta_in_millis(order_rec, delta_in_millis);
    order_rec.cond = cond;
    order_rec.order_id = order_id;
    order_rec.size = size;
    order_rec.price = price.iprice32();

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &order_rec, sizeof(qbdf_order_exec_iprice32))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_order_modify(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                             uint32_t ms_since_midnight, char exch,
                             char side, uint32_t size, float price) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);
    return gen_qbdf_order_modify_rec_time(p_builder, id, order_id, rec_time, exch, side, size, price);
  }


  bool gen_qbdf_order_modify_rec_time(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                                      uint32_t rec_time, char exch, char side, uint32_t size, float price) {

    qbdf_order_modify order_rec;
    memset(&order_rec, 0, sizeof(qbdf_order_modify));

    order_rec.hdr.format = QBDFMsgFmt::OrderModify;
    order_rec.time = rec_time % 100000;
    order_rec.symbol_id = id;
    order_rec.orig_order_id = order_id;
    order_rec.new_side = side;
    order_rec.new_price = price;
    order_rec.new_size = size;

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record("", id, rec_time, &order_rec, sizeof(qbdf_order_modify))) {
      return false;
    }
    return true;

  }


  bool gen_qbdf_order_modify_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                 uint64_t micros_since_midnight, int32_t exch_time_delta,
                                 char exch, char side, uint32_t size, MDPrice price) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);
    return gen_qbdf_order_modify_micros_qbdf_time(p_builder, id, order_id, qbdf_time,
                                                  exch_time_delta, exch, side, size, price);
  }


  bool gen_qbdf_order_modify_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                           uint64_t qbdf_time, int32_t exch_time_delta,
                                           char exch, char side, uint32_t size, MDPrice price,
                                           bool delta_in_millis) {

    qbdf_order_modify_iprice32 order_rec;
    memset(&order_rec, 0, sizeof(qbdf_order_modify_iprice32));

    order_rec.common.format = QBDFMsgFmt::order_modify_iprice32;
    order_rec.common.time = qbdf_time % 100000000;
    order_rec.common.exch_time_delta = exch_time_delta;
    order_rec.common.exch = exch;
    order_rec.common.id = id;
    order_rec.order_id = order_id;
    set_side(order_rec, side);
    set_delta_in_millis(order_rec, delta_in_millis);
    order_rec.size = size;
    order_rec.price = price.iprice32();

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &order_rec, sizeof(qbdf_order_modify_iprice32))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_order_replace(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                              uint32_t ms_since_midnight, char exch,
                              uint64_t new_order_id, uint32_t size, float price) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);
    return gen_qbdf_order_replace_rec_time(p_builder, id, order_id, rec_time, exch,
                                           new_order_id, size, price);
  }


  bool gen_qbdf_order_replace_rec_time(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                                       uint32_t rec_time, char exch, uint64_t new_order_id,
                                       uint32_t size, float price) {
    qbdf_order_replace order_rec;
    memset(&order_rec, 0, sizeof(qbdf_order_replace));

    order_rec.hdr.format = QBDFMsgFmt::OrderReplace;
    order_rec.time = rec_time % 100000;
    order_rec.symbol_id = id;
    order_rec.orig_order_id = order_id;
    order_rec.new_order_id = new_order_id;
    order_rec.price = price;
    order_rec.size = size;

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record("", id, rec_time, &order_rec, sizeof(qbdf_order_replace))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_order_replace_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                 uint64_t micros_since_midnight, int32_t exch_time_delta,
                                 char exch, char side, uint64_t new_order_id,
                                 uint32_t size, MDPrice price) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);
    return gen_qbdf_order_replace_micros_qbdf_time(p_builder, id, order_id, qbdf_time,
                                                   exch_time_delta, exch, side, new_order_id,
                                                   size, price);
  }


  bool gen_qbdf_order_replace_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                           uint64_t qbdf_time, int32_t exch_time_delta,
                                           char exch, char side, uint64_t new_order_id,
                                           uint32_t size, MDPrice price, bool delta_in_millis) {

    qbdf_order_replace_iprice32 order_rec;
    memset(&order_rec, 0, sizeof(qbdf_order_replace_iprice32));

    order_rec.common.format = QBDFMsgFmt::order_replace_iprice32;
    order_rec.common.time = qbdf_time % 100000000;
    order_rec.common.exch_time_delta = exch_time_delta;
    order_rec.common.exch = exch;
    order_rec.common.id = id;
    order_rec.orig_order_id = order_id;
    order_rec.new_order_id = new_order_id;
    set_side(order_rec, side);
    set_delta_in_millis(order_rec, delta_in_millis);
    order_rec.size = size;
    order_rec.price = price.iprice32();

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &order_rec, sizeof(qbdf_order_replace_iprice32))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_gap_marker(QBDF_Builder* p_builder, uint32_t ms_since_midnight,
                           ProductID id, size_t context) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);

    qbdf_gap_marker gap_marker_rec;
    memset(&gap_marker_rec, 0, sizeof(qbdf_gap_marker));

    gap_marker_rec.hdr.format = QBDFMsgFmt::GapMarker;
    gap_marker_rec.time = rec_time % 100000;
    gap_marker_rec.symbol_id = id;
    gap_marker_rec.context = context;

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record("", -1, rec_time, &gap_marker_rec, sizeof(qbdf_gap_marker))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_gap_marker_micros(QBDF_Builder* p_builder, uint64_t micros_since_midnight,
                                  uint16_t id, char exch, size_t context) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);

    return gen_qbdf_gap_marker_micros_qbdf_time(p_builder, qbdf_time, id, exch, context);
  }

  bool gen_qbdf_gap_marker_micros_qbdf_time(QBDF_Builder* p_builder, uint64_t qbdf_time,
					    uint16_t id, char exch, size_t context) {

    qbdf_gap_marker_micros gap_marker_rec;
    memset(&gap_marker_rec, 0, sizeof(qbdf_gap_marker_micros));

    gap_marker_rec.common.format = QBDFMsgFmt::gap_marker_micros;
    gap_marker_rec.common.time = qbdf_time % 100000000;
    gap_marker_rec.common.exch_time_delta = 0;
    gap_marker_rec.common.exch = exch;
    gap_marker_rec.common.id = id;
    gap_marker_rec.context = context;

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &gap_marker_rec, sizeof(qbdf_gap_marker_micros))) {
      return false;
    }
    return true;
  }

  bool gen_qbdf_gap_summary(QBDF_Builder* p_builder, uint32_t ms_since_midnight, size_t context,
                            uint64_t expected_seq_no, uint64_t received_seq_no) {

    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);

    if (p_builder->write_gap_summary_record(rec_time, context, expected_seq_no, received_seq_no)) {
      return false;
    }
    return true;
  }

  bool gen_qbdf_level(QBDF_Builder* p_builder, ProductID id, uint32_t ms_since_midnight,
                      char exch, char side, uint32_t size, float price, uint16_t num_orders,
                      char trade_status, char quote_cond, char reason_code,
                      uint32_t link_id_1, uint32_t link_id_2, uint32_t link_id_3, uint8_t last_in_group) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);

    return gen_qbdf_level_rec_time(p_builder, id, rec_time, exch, side, size, price, num_orders,
                                   trade_status, quote_cond, reason_code, link_id_1, link_id_2,
                                   link_id_3, last_in_group);
  }

  bool gen_qbdf_level_rec_time(QBDF_Builder* p_builder, ProductID id, uint32_t rec_time,
                               char exch, char side, uint32_t size, float price, uint16_t num_orders,
                               char trade_status, char quote_cond, char reason_code,
                               uint32_t link_id_1, uint32_t link_id_2, uint32_t link_id_3, uint8_t last_in_group) {
    qbdf_level level_rec;
    memset(&level_rec, 0, sizeof(qbdf_level));

    level_rec.hdr.format = QBDFMsgFmt::Level;
    level_rec.time = rec_time % 100000;
    level_rec.symbol_id = id;
    level_rec.side = side;
    level_rec.price = price;
    level_rec.size = size;
    level_rec.num_orders = num_orders;
    level_rec.trade_status = trade_status;
    level_rec.quote_cond = quote_cond;
    level_rec.reason_code = reason_code;
    level_rec.link_id_1 = link_id_1;
    level_rec.link_id_2 = link_id_2;
    level_rec.link_id_3 = link_id_3;
    level_rec.exch = exch;
    level_rec.last_in_group = last_in_group;

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record("", id, rec_time, &level_rec, sizeof(qbdf_level))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_level_iprice64(QBDF_Builder* p_builder, uint16_t id,
                               uint64_t micros_since_midnight, int32_t exch_time_delta,
                               char exch, char side, char trade_cond, char quote_cond,
                               char reason_code, uint16_t num_orders, uint32_t size, MDPrice md_price,
                               bool last_in_group, bool executed) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);
    return gen_qbdf_level_iprice64_qbdf_time(p_builder, id, qbdf_time, exch_time_delta,
                                             exch, side, trade_cond, quote_cond, reason_code,
                                             num_orders, size, md_price, last_in_group, executed);
  }

  bool gen_qbdf_level_iprice64_qbdf_time(QBDF_Builder* p_builder, ProductID id,
                                         uint64_t qbdf_time, int32_t exch_time_delta,
                                         char exch, char side, char trade_cond, char quote_cond,
                                         char reason_code, uint16_t num_orders,
                                         uint32_t size, MDPrice md_price, bool last_in_group,
                                         bool executed, bool delta_in_millis) {
    qbdf_level_iprice64 level_rec;
    memset(&level_rec, 0, sizeof(qbdf_level_iprice64));

    level_rec.common.format = QBDFMsgFmt::level_iprice64;
    level_rec.common.time = qbdf_time % 100000000;
    level_rec.common.exch_time_delta = exch_time_delta;
    level_rec.common.exch = exch;
    level_rec.common.id = id;
    level_rec.trade_cond = trade_cond;
    level_rec.quote_cond = quote_cond;
    level_rec.reason_code = reason_code;
    level_rec.num_orders = num_orders;
    level_rec.size = size;
    level_rec.price = md_price.iprice64();
    set_side(level_rec, side);
    set_last_in_group(level_rec, last_in_group);
    set_executed(level_rec, executed);
    set_delta_in_millis(level_rec, delta_in_millis);

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &level_rec, sizeof(qbdf_level_iprice64))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_link(QBDF_Builder* p_builder, ProductID id, string vendor_id,
                     string ticker, uint32_t link_id, char side) {
    if (p_builder->write_link_record(id, vendor_id, ticker, link_id, side)) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_cancel_correct(QBDF_Builder* p_builder, ProductID id, uint32_t ms_since_midnight,
                               uint32_t orig_ms_since_midnight, string vendor_id, string type,
                               uint64_t orig_seq_no, double orig_price, int orig_size,
                               string orig_cond, double new_price, int new_size, string new_cond) {

    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);
    uint32_t orig_rec_time = p_builder->millis_to_qbdf_time(orig_ms_since_midnight);

    return gen_qbdf_cancel_correct_rec_time(p_builder, id, rec_time, orig_rec_time, vendor_id, type,
                                            orig_seq_no, orig_price, orig_size, orig_cond,
                                            new_price, new_size, new_cond);
  }

  bool gen_qbdf_cancel_correct_rec_time(QBDF_Builder* p_builder, ProductID id, uint32_t rec_time,
                                        uint32_t orig_rec_time, string vendor_id, string type,
                                        uint64_t orig_seq_no, double orig_price, int orig_size,
                                        string orig_cond, double new_price, int new_size,
                                        string new_cond) {

    if (p_builder->write_cancel_correct_record(id, rec_time, orig_rec_time, vendor_id, type,
                                               orig_seq_no, orig_price, orig_size, orig_cond,
                                               new_price, new_size, new_cond)) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_status(QBDF_Builder* p_builder, ProductID id, uint32_t ms_since_midnight,
                       char exch, uint16_t status_type, const char* status_detail) {
    uint32_t rec_time = p_builder->millis_to_qbdf_time(ms_since_midnight);
    return gen_qbdf_status_rec_time(p_builder, id, rec_time, exch, status_type, status_detail);
  }


  bool gen_qbdf_status_rec_time(QBDF_Builder* p_builder, ProductID id, uint32_t rec_time,
                                char exch, uint16_t status_type, const char* status_detail) {
    qbdf_status status_rec;
    memset(&status_rec, 0, sizeof(qbdf_status));

    status_rec.hdr.format = QBDFMsgFmt::Status;
    status_rec.time = rec_time % 100000;
    status_rec.symbolId = id;
    status_rec.exch = exch;
    status_rec.status_type = status_type;
    strncpy(status_rec.status_detail, status_detail, sizeof(status_rec.status_detail));

    if (p_builder->write_binary_record("", id, rec_time, &status_rec, sizeof(qbdf_status))) {
      return false;
    }
    return true;
  }


  bool gen_qbdf_status_micros(QBDF_Builder* p_builder, uint16_t id,
                              uint64_t micros_since_midnight, int32_t exch_time_delta,
                              char exch, uint16_t status_type, string status_detail) {
    uint64_t qbdf_time = p_builder->micros_to_qbdf_time(micros_since_midnight);
    return gen_qbdf_status_micros_qbdf_time(p_builder, id, qbdf_time, exch_time_delta,
                                            exch, status_type, status_detail);
  }

  bool gen_qbdf_status_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
                                        uint64_t qbdf_time, int32_t exch_time_delta,
                                        char exch, uint16_t status_type, string status_detail) {
    qbdf_status_micros status_rec;
    memset(&status_rec, 0, sizeof(qbdf_status_micros));

    status_rec.common.format = QBDFMsgFmt::status_micros;
    status_rec.common.time = qbdf_time % 100000000;
    status_rec.common.exch_time_delta = exch_time_delta;
    status_rec.common.exch = exch;
    status_rec.common.id = id;
    status_rec.status_type = status_type;
    strncpy(status_rec.status_detail, status_detail.c_str(), sizeof(status_rec.status_detail));

    if (p_builder->write_binary_record_micros(qbdf_time, &status_rec, sizeof(qbdf_status_micros))) {
      return false;
    }
    return true;
  }

  bool gen_qbdf_quote_micros_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					   uint64_t qbdf_time, int32_t exch_time_delta,
					   char exch, char cond, MDPrice bid_price, MDPrice ask_price,
					   uint32_t bid_size, uint32_t ask_size, uint32_t seq_no) {
    if (bid_size > USHRT_MAX || ask_size > USHRT_MAX) {
      qbdf_quote_large_iprice32_seq quote_seq_rec;
      quote_seq_rec.seq = seq_no;
      quote_seq_rec.q.common.format = QBDFMsgFmt::quote_large_iprice32;
      quote_seq_rec.q.common.time = qbdf_time % 100000000;
      quote_seq_rec.q.common.exch_time_delta = exch_time_delta;
      quote_seq_rec.q.common.exch = exch;
      quote_seq_rec.q.common.id = id;
      quote_seq_rec.q.bid_price = bid_price.iprice32();
      quote_seq_rec.q.ask_price = ask_price.iprice32();
      quote_seq_rec.q.bid_size = bid_size;
      quote_seq_rec.q.ask_size = ask_size;
      // Write out this record to the correct 1 minute binary file
      if (p_builder->write_binary_record_micros(qbdf_time, &quote_seq_rec, sizeof(qbdf_quote_large_iprice32_seq))) {
	return false;
      }
    } else {
      qbdf_quote_small_iprice32_seq quote_seq_rec;
      quote_seq_rec.seq = seq_no;
      quote_seq_rec.q.common.format = QBDFMsgFmt::quote_small_iprice32;
      quote_seq_rec.q.common.time = qbdf_time % 100000000;
      quote_seq_rec.q.common.exch_time_delta = exch_time_delta;
      quote_seq_rec.q.common.exch = exch;
      quote_seq_rec.q.common.id = id;

      quote_seq_rec.q.cond = cond;
      quote_seq_rec.q.bid_price = bid_price.iprice32();
      quote_seq_rec.q.ask_price = ask_price.iprice32();
      quote_seq_rec.q.bid_size = bid_size;
      quote_seq_rec.q.ask_size = ask_size;

      // Write out this record to the correct 1 minute binary file
      if (p_builder->write_binary_record_micros(qbdf_time, &quote_seq_rec, sizeof(qbdf_quote_small_iprice32_seq))) {
	return false;
      }
    }
    return true;
  }
  
  //
  // introduced to handle the daily TAQ data format after Oct. 3, 2016
  //
  bool gen_qbdf_quote_nanos_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					  uint64_t qbdf_time, int32_t exch_time_delta,
					  char exch, char cond, MDPrice bid_price, MDPrice ask_price,
					  uint32_t bid_size, uint32_t ask_size, uint32_t seq_no) {
    if (bid_size > USHRT_MAX || ask_size > USHRT_MAX) {
      qbdf_quote_large_iprice32_seq quote_seq_rec;
      quote_seq_rec.seq = seq_no;
      quote_seq_rec.q.common.format = QBDFMsgFmt::quote_large_iprice32;
      quote_seq_rec.q.common.time = qbdf_time % 100000000000;
      quote_seq_rec.q.common.exch_time_delta = exch_time_delta;
      quote_seq_rec.q.common.exch = exch;
      quote_seq_rec.q.common.id = id;
      quote_seq_rec.q.bid_price = bid_price.iprice32();
      quote_seq_rec.q.ask_price = ask_price.iprice32();
      quote_seq_rec.q.bid_size = bid_size;
      quote_seq_rec.q.ask_size = ask_size;
      // Write out this record to the correct 1 minute binary file
      if (p_builder->write_binary_record_nanos(qbdf_time, &quote_seq_rec, sizeof(qbdf_quote_large_iprice32_seq))) {
	return false;
      }
    } else {
      qbdf_quote_small_iprice32_seq quote_seq_rec;
      quote_seq_rec.seq = seq_no;
      quote_seq_rec.q.common.format = QBDFMsgFmt::quote_small_iprice32;
      quote_seq_rec.q.common.time = qbdf_time % 100000000000;
      quote_seq_rec.q.common.exch_time_delta = exch_time_delta;
      quote_seq_rec.q.common.exch = exch;
      quote_seq_rec.q.common.id = id;

      quote_seq_rec.q.cond = cond;
      quote_seq_rec.q.bid_price = bid_price.iprice32();
      quote_seq_rec.q.ask_price = ask_price.iprice32();
      quote_seq_rec.q.bid_size = bid_size;
      quote_seq_rec.q.ask_size = ask_size;

      // Write out this record to the correct 1 minute binary file
      if (p_builder->write_binary_record_nanos(qbdf_time, &quote_seq_rec, sizeof(qbdf_quote_small_iprice32_seq))) {
	return false;
      }
    }
    return true;
  }


  bool gen_qbdf_trade_micros_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					   uint64_t qbdf_time, int32_t exch_time_delta,
					   char exch, char side, char corr, string cond,
					   uint32_t size, MDPrice price, uint32_t seq_no) {
    qbdf_trade_iprice32_seq trade_seq_rec;
    trade_seq_rec.seq = seq_no;
    trade_seq_rec.t.common.format = QBDFMsgFmt::trade_iprice32;
    trade_seq_rec.t.common.time = qbdf_time % 100000000;
    trade_seq_rec.t.common.exch_time_delta = exch_time_delta;
    trade_seq_rec.t.common.exch = exch;
    trade_seq_rec.t.common.id = id;
    
    set_side(trade_seq_rec.t, side);
    trade_seq_rec.t.correct = corr;

    memset(trade_seq_rec.t.cond, ' ', 4);
    memcpy(trade_seq_rec.t.cond, cond.c_str(), min((int)cond.size(), 4));
    
    trade_seq_rec.t.size = size;
    trade_seq_rec.t.price = price.iprice32();

    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &trade_seq_rec, sizeof(qbdf_trade_iprice32_seq))) {
      return false;
    }
    return true;
  }

  bool gen_qbdf_trade_nanos_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					   uint64_t qbdf_time, int32_t exch_time_delta,
					   char exch, char side, char corr, string cond,
					   uint32_t size, MDPrice price, uint32_t seq_no) {

    qbdf_trade_iprice32_seq trade_seq_rec; // should we use iprice64 ? 
    trade_seq_rec.seq = seq_no;
    trade_seq_rec.t.common.format = QBDFMsgFmt::trade_iprice32;
    trade_seq_rec.t.common.time = qbdf_time % 100000000000; // 15 digit time
    trade_seq_rec.t.common.exch_time_delta = exch_time_delta;
    trade_seq_rec.t.common.exch = exch;
    trade_seq_rec.t.common.id = id;
    
    set_side(trade_seq_rec.t, side);
    trade_seq_rec.t.correct = corr;
    
    memset(trade_seq_rec.t.cond, ' ', 4);
    memcpy(trade_seq_rec.t.cond, cond.c_str(), min((int)cond.size(), 4));
    
    trade_seq_rec.t.size = size;
    trade_seq_rec.t.price = price.iprice32();
    
    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_nanos(qbdf_time, &trade_seq_rec, sizeof(qbdf_trade_iprice32_seq))) {
	return false;
    }
    return true;
  }


  bool gen_qbdf_imbalance_micros_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					       uint64_t qbdf_time, int32_t exch_time_delta,
					       char exch, MDPrice ref_price, MDPrice near_price,
					       MDPrice far_price, uint32_t paired_size,
					       uint32_t imbalance_size, char imbalance_side,
					       char indicative_flag, uint32_t seq_no) {
    qbdf_imbalance_iprice32_seq imbalance_seq_rec;
    memset(&imbalance_seq_rec, 0, sizeof(qbdf_imbalance_iprice32_seq));
    
    imbalance_seq_rec.seq = seq_no;
    imbalance_seq_rec.i.common.format = QBDFMsgFmt::imbalance_iprice32;
    imbalance_seq_rec.i.common.time = qbdf_time % 100000000;
    imbalance_seq_rec.i.common.exch_time_delta = exch_time_delta;
    imbalance_seq_rec.i.common.exch = exch;
    imbalance_seq_rec.i.common.id = id;
    set_side(imbalance_seq_rec.i, imbalance_side);

    imbalance_seq_rec.i.cross_type = indicative_flag;
    imbalance_seq_rec.i.ref_price = ref_price.iprice32();
    imbalance_seq_rec.i.near_price = near_price.iprice32();
    imbalance_seq_rec.i.far_price = far_price.iprice32();
    imbalance_seq_rec.i.paired_size = paired_size;
    imbalance_seq_rec.i.imbalance_size = imbalance_size;
      
    // Write out this record to the correct 1 minute binary file
    if (p_builder->write_binary_record_micros(qbdf_time, &imbalance_seq_rec, sizeof(qbdf_imbalance_iprice32_seq))) {
      return false;
    }
    return true;
  }

}

