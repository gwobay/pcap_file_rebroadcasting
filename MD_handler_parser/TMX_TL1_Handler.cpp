#include <Data/QBDF_Util.h>
#include <Data/TMX_TL1_Handler.h>
#include <Util/Network_Utils.h>

#include <Exchange/Exchange_Manager.h>
#include <MD/MDManager.h>
#include <MD/MDProvider.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>
#include <boost/lexical_cast.hpp>

namespace hf_core {

  namespace tmx_TL1 {

    struct packet_frame_header 
    {
      char stx;
      char length[4]; // length of whole packet excluding STX and ETX
      char sequence_number[9];
      char service_id[3];
      char recovery_identifier;
      char continuation_identifier;
      char message_type[2];
      char exchange_identifier[2];
      // packet ends with ETX (0x03) after contents
    } __attribute__ ((packed));

    struct heartbeat_message
    {
      char separator_1; //'['
      char section_identifier_1[10]; // "HEARTBEAT"
      char date[10]; // YYYY-MM-DD
      char separator_2;
      char time_of_day[8]; // HH:MM:SS
      char separator_3;
      char seconds_since_1970[19]; // "%012d.%06d"
      char separator_4[2]; // ']['
      char section_identifier_2[10]; // "LAST SENT"
      char seq_nbr_of_last_msg_sent[9];
      char separator_5;
      char time_last_msg_sent[8]; //HH:MM:SS
      char separator_6;
      char seconds_since_1970_last_msg[19]; // "%012d.%06d"
      char separator_7[2]; //']['
      char section_identifier_3[10]; // "LAST HB  "
      char seq_nbr_of_last_heartbeat_sent[9];
      char separator_8;
      char time_last_heartbeat_sent[8]; 
      char separator_9;
      char seconds_since_1970_last_heartbeat[19];
      char separator_10; //']'
      char feed_name[20];
      char reserved[2];
      char hostname[8]; // "Primary" or "DR"
      char version[4];
    } __attribute__ ((packed));

    struct equity_trade_message
    {
      packet_frame_header header;
      char symbol[8];
      char volume[9];
      char trade_price[11];
      char buyer_id[3];
      char seller_id[3];
      char trade_time_stamp[6];
      char last_sale[11];
      char trade_id[9];
      char cross_type;
      char moc;
      char bypass;
      char opening_trade;
      char settlement_terms;
      char trading_system_time_stamp[20];
    } __attribute__ ((packed));

    struct symbol_status_message
    {
      packet_frame_header header;
      char symbol[8];
      char trading_system_time_stamp[20];
      char exchange_id[3];
      char cusip[12];
      char board_lot[9];
      char currency;
      char face_value[9];
      char last_sale[11];
      char moc_eligible;
      char product_type;
      char symbol_name[40];
      char stock_group[2];
      char stock_state[2];
    } __attribute__ ((packed));

    struct moc_imbalance_notification_message
    {
      packet_frame_header header;
      char symbol[8];
      char imbalance_side;
      char imbalance[9];
    } __attribute__ ((packed));

    struct moc_price_movement_delay_message
    {
      packet_frame_header header;
      char symbol[8];
      char trading_system_time_stamp[20];
      char stock_state[2];
      char ccp[11];
      char vwap[11];
    } __attribute__ ((packed));

    struct stock_state_message
    {
      packet_frame_header header;
      char symbol[8];
      char trading_system_time_stamp[20];
      char comment[40];
      char stock_state[2];
      char opening_time[6];
    } __attribute__ ((packed));

    struct equity_quote_message
    {
      packet_frame_header header;
      char symbol[8];
      char bid_price[9];
      char bid_size[9];
      char ask_price[9];
      char ask_size[9];
      char trading_system_time_stamp[20];
    } __attribute__ ((packed));

    struct general_message
    {
      packet_frame_header header;
      char trading_system_time_stamp[20];
      char bulletin_indicator;
      char message_text[80];
    } __attribute__ ((packed));

    struct equity_trade_cancellation_message
    {
      packet_frame_header header;
      char symbol[8];
      char volume[9];
      char trade_price[11];
      char buyer_id[3];
      char seller_id[3];
      char trade_time_stamp[6];
      char original_trade_id[9];
      char last_sale[11];
      char trading_system_time_stamp[20];
    } __attribute__ ((packed));

    struct market_state_message
    {
      packet_frame_header header;
      char trading_system_time_stamp[20];
      char stock_group[2];
      char market_state[1];
    } __attribute__ ((packed));

    struct trading_tier_status_message
    {
      packet_frame_header header;
      char exchange_id[3];
      char total_number_of_symbols[5];
      char total_number_of_groups[3];
      char trading_system_time_stamp[20];
      char trading_tier_id[6];
    } __attribute__ ((packed));

    struct equity_trade_correction_message
    {
      packet_frame_header header;
      char symbol[8];
      char volume[9];
      char trade_price[11];
      char buyer_id[3];
      char seller_id[3];
      char trade_time_stamp[6];
      char last_sale[11];
      char trade_id[9];
      char cross_type;
      char moc;
      char bypass;
      char opening_trade;
      char settlement_terms;
      char original_trade_id[9];
      char trading_system_time_stamp[20];
    } __attribute__ ((packed));

  }

  static
  uint64_t
  int_of_symbol(const char* symbol, int len)
  {
    uint64_t r = 0;
    for(int i = 0; i < len; ++i) {
      if(symbol[i] == ' ' || symbol[i] == '\0') {
        break;
      }
      r = (r << 7) + symbol[i];
    }
    return r;
  }


  ProductID
  TMX_TL1_Handler::product_id_of_symbol(const char* symbol, int len)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol, len);
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string.c_str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
                             "TMX_TL1_Handler::product_id_of_symbol", symbol);
        //cout << "symbol_string = '" << symbol_string << "' len=" << symbol_string.size() << endl;
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }

      return symbol_id;
    }

    uint64_t h = int_of_symbol(symbol, len);
    hash_map<uint64_t, ProductID>::iterator i = m_prod_map.find(h);
    if(i == m_prod_map.end()) {
      return Product::invalid_product_id;
    } else {
      return i->second;
    }
  }

#define LOGV(x)       \
  if(false) {         \
    x;                \
  }                   \

#define SEEN(id)                                                \
  if(!m_seen_product.test(id)) {                                \
    m_seen_product.set(id);                                     \
  }

template<size_t STR_LENGTH, typename OUT_TYPE>
inline static OUT_TYPE ascii_to_number(const char* ascii)
{
  string s(ascii, STR_LENGTH);
  return boost::lexical_cast<OUT_TYPE>(s);
}

#define A_TO_U32(x) ascii_to_number<sizeof(x), uint32_t>(x)

#define MSG_TYPE_TO_U32(x) static_cast<uint32_t>(((static_cast<uint32_t>(x[0]) & 0x0ff) << 8) | (static_cast<uint32_t>(x[1]) & 0x0ff))
#define MSG_TYPE_TO_U32_CONST(x, y) static_cast<uint32_t>(((static_cast<uint32_t>(x) & 0x0ff) << 8) | (static_cast<uint32_t>(y) & 0x0ff))

template<size_t STR_LENGTH>
inline static double price_to_double(const char* ascii, double divisor)
{
  uint32_t i_price = ascii_to_number<STR_LENGTH, uint32_t>(ascii);
  return i_price / divisor;
}

#define F_PRICE(x, d) price_to_double<sizeof(x)>(x, d)


inline static uint64_t get_us_since_midnight(const char* ascii)
{
  return (ascii_to_number<2, uint64_t>(ascii + 8) * 60 * 60 * 1000000) + 
         (ascii_to_number<2, uint64_t>(ascii + 10) * 60 * 1000000) +
         (ascii_to_number<2, uint64_t>(ascii + 12) * 1000000) + 
         ascii_to_number<6, uint64_t>(ascii + 14);
}

  void TMX_TL1_Handler::publishImbalance(ProductID productId, Time& exchTimestamp, MDPrice& copPrice, uint32_t totalBuySize, uint32_t totalSellSize)
  {
    m_auction.m_id = productId;
    uint32_t pairedSize = totalBuySize > totalSellSize ? totalSellSize : totalBuySize; // paired is the min of the two
    uint32_t imbalanceSize = totalBuySize > totalSellSize ? (totalBuySize - totalSellSize) : (totalSellSize - totalBuySize); // imbalance is the difference
    char imbalanceDirection = totalBuySize > totalSellSize ? 'B' : totalBuySize == totalSellSize ? ' ' : 'S';

    m_auction.m_ref_price = copPrice.fprice();
    m_auction.m_far_price = copPrice.fprice();
    m_auction.m_near_price = copPrice.fprice();
    m_auction.m_paired_size = pairedSize;
    m_auction.m_imbalance_size = imbalanceSize;
    m_auction.m_imbalance_direction = imbalanceDirection;
    if(m_qbdf_builder && !m_exclude_imbalance)
    {
      m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
      m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
      gen_qbdf_imbalance_micros(m_qbdf_builder, productId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                                copPrice, copPrice, copPrice, pairedSize, imbalanceSize, imbalanceDirection, ' ');
      LOGV(cout << "Publishing imbalance: price=" << copPrice.fprice() << " pairedSize=" << pairedSize << " productId=" << productId << " exchTimestamp=" << exchTimestamp.to_string()
           << " imbalanceSize=" << imbalanceSize << " imbalanceDir=" << imbalanceDirection << endl);
    }
    if(m_subscribers)
    {
      m_subscribers->update_subscribers(m_auction);
    }
  }

  size_t
  TMX_TL1_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const tmx_TL1::packet_frame_header& frameHeader = reinterpret_cast<const tmx_TL1::packet_frame_header&>(*buf);
    //cout << "context=" << context << " message_type='" << frameHeader.message_type[0] << frameHeader.message_type[1] << "'" << endl;

    if(frameHeader.stx != 0x02)
    {
      m_logger->log_printf(Log::WARN, "TMX_TL1_Handler::parse frameHeader.stx is not STX");
      return len;
    }

    //cout << "msg='";
    //for(size_t i=0; i < len; ++i)
    //{
    //  cout << buf[i];
    //}
    //cout << endl << endl;

    // If the message isn't a heartbeat (or some admin message - don't think I've seen those in the spec)
    if(MSG_TYPE_TO_U32(frameHeader.message_type) != MSG_TYPE_TO_U32_CONST('V', ' '))
    {
       uint32_t seqNum = A_TO_U32(frameHeader.sequence_number);
       //cout << "seqNum=" << seqNum << endl;
       // Track sequence number, distinguishing mulitcast groups by context
      if (context > m_channel_info_map.size())
      {
        m_logger->log_printf(Log::WARN, "TMX_TL1_Handler::parse context %lu exceeds max %lu", context, m_channel_info_map.size());
        // Do something
        return len;
      }
      channel_info& ci = m_channel_info_map[context];
      if (ci.seq_no == 0)
      {
        //This is the first message received on this group
      }
      else
      {
        uint32_t expectedSeqNum = ci.seq_no;
        if(seqNum < expectedSeqNum)
        {
          //cout << "Duplicate msg: seqNum " << seqNum << " context " << context << endl;
          return len;
        }
        else if (seqNum > expectedSeqNum)
        {
          m_logger->log_printf(Log::WARN, "TMX_TL1_Handler::parse gap on context %lu : expecting %d got %d", context, expectedSeqNum, seqNum);
          if(ci.begin_recovery(this, buf, len, seqNum, seqNum + 1))
          {
            return len;
          }
        }
      }
      LOGV(cout << endl);
      ci.seq_no = seqNum + 1;
      ci.last_update = Time::currentish_time();
      LOGV(cout << "seqNum=" << seqNum << " context=" << context << " context=" << context << " source_id=" << endl);

      if(m_recorder) {
        Logger::mutex_t::scoped_lock l(m_recorder->mutex());
        record_header h(Time::current_time(), context, len);
        m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
        m_recorder->unlocked_write(buf, len);
      }
      if(m_record_only) {
        return len;
      }

      parse2(context, buf, len);

      if(!ci.queue.empty())
      {
        ci.process_recovery(this);
      }
    }

    return len;
  }

  size_t
  TMX_TL1_Handler::parse2(size_t context, const char* buf, size_t len)
  { 
    const tmx_TL1::packet_frame_header& frameHeader = reinterpret_cast<const tmx_TL1::packet_frame_header&>(*buf);
    uint32_t msgTypeU = MSG_TYPE_TO_U32(frameHeader.message_type);
    switch(msgTypeU)
    {
      case MSG_TYPE_TO_U32_CONST('A', ' '):
        {
          const tmx_TL1::equity_trade_message& msg = reinterpret_cast<const tmx_TL1::equity_trade_message&>(*buf);
          ProductID productId = product_id_of_symbol(msg.symbol, sizeof(msg.symbol));
          if(productId == Product::invalid_product_id)
          {
            LOGV(cout << "Invalid product id" << endl);
            break;
          }
          m_trade.m_id = productId;
          m_trade.clear_flags();
          m_trade.set_exec_flag(TradeUpdate::exec_flag_exec);
          m_trade.m_price = F_PRICE(msg.trade_price, 100000.0);
          m_trade.m_size = A_TO_U32(msg.volume);

          m_trade.m_trade_qualifier[0] = msg.cross_type;
          m_trade.m_trade_qualifier[1] = msg.moc;
          m_trade.m_trade_qualifier[2] = msg.opening_trade;
          m_trade.m_trade_qualifier[3] = msg.settlement_terms;

          m_trade.set_side(0);
          m_trade.strat_recv_time = Time::current_time();
          uint64_t usSinceMidnight = get_us_since_midnight(msg.trading_system_time_stamp);
          Time time = Time(Time::today().ticks() + usSinceMidnight * 1000);
          m_trade.set_time(time);
          //cout << time.to_string() << " " << string(msg.symbol, sizeof(msg.symbol)) << " '" << m_trade.m_trade_qualifier << "'" << endl;
          
          m_msu.m_market_status = m_security_status[productId];
          m_feed_rules->apply_trade_conditions(m_trade, m_msu);
          if(m_msu.m_market_status != m_security_status[m_trade.m_id]) {
            m_msu.m_id = m_trade.m_id;
            m_msu.m_exch = m_trade.m_exch;
            m_security_status[m_trade.m_id] = m_msu.m_market_status;
            if(m_subscribers) {
              m_subscribers->update_subscribers(m_msu);
            }
          }

          if(m_qbdf_builder)
          {
            int32_t exch_time_delta = 0;
            char side = ' ';
            char corr = ' ';
            string cond(m_trade.m_trade_qualifier, sizeof(m_trade.m_trade_qualifier));
            gen_qbdf_trade_micros(m_qbdf_builder, m_trade.m_id, usSinceMidnight, exch_time_delta,
                m_qbdf_exch_char, side, corr, cond, m_trade.m_size, MDPrice::from_fprice(m_trade.m_price));
          }
          if(m_provider) 
          {
            m_provider->check_trade(m_trade);
          }

          update_subscribers(m_trade);
        }
        break;
      case MSG_TYPE_TO_U32_CONST('B', ' '):
        {
          const tmx_TL1::symbol_status_message& msg = reinterpret_cast<const tmx_TL1::symbol_status_message&>(*buf);
          ProductID productId = product_id_of_symbol(msg.symbol, sizeof(msg.symbol));
          if(productId == Product::invalid_product_id)
          {
            break;
          }
          bool is_halted = msg.stock_state[0] == 'I' || msg.stock_state[1] != ' ';
        
          Time t = Time::current_time(); // ToDo: fix
          m_msu.set_time(t);  
          m_msu.m_id = productId;
          m_msu.m_reason = string(msg.stock_state, sizeof(msg.stock_state));
          QBDFStatusType qbdf_status;
          if(is_halted) {
            m_msu.m_market_status = MarketStatus::Halted;
            qbdf_status = QBDFStatusType::Halted;
          }
          else {
            m_msu.m_market_status = MarketStatus::Open;
            qbdf_status = QBDFStatusType::Open;
          }
          if(m_subscribers) {
            m_subscribers->update_subscribers(m_msu);
          }
          if(m_qbdf_builder) {
            m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
            uint64_t micros_exch_time_delta = 0;
            gen_qbdf_status_micros(m_qbdf_builder, productId, m_micros_recv_time,
                                       micros_exch_time_delta, m_qbdf_exch_char,
                                       qbdf_status.index(), m_msu.m_reason);
          }
          m_msu.m_reason.clear();
          m_security_status[productId] = m_msu.m_market_status;
      
          // ToDo: publish a market status update (see m_msu in TMX_QL2)
        }
        break;
      case MSG_TYPE_TO_U32_CONST('C', ' '):
        {
          const tmx_TL1::moc_imbalance_notification_message& msg = reinterpret_cast<const tmx_TL1::moc_imbalance_notification_message&>(*buf);
          ProductID productId = product_id_of_symbol(msg.symbol, sizeof(msg.symbol));
          if(productId == Product::invalid_product_id)
          {
            LOGV(cout << "Invalid product id" << endl);
            break;
          }
          m_auction.m_id = productId;
          m_auction.m_imbalance_size = A_TO_U32(msg.imbalance);
          m_auction.m_imbalance_direction = msg.imbalance_side;
          //uint64_t m_micros_exch_time = Time::current_time().total_usec() - Time::today().total_usec();
          //uint64_t m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
          uint64_t m_micros_exch_time_delta = 0;
          if(m_qbdf_builder)
          {
            gen_qbdf_imbalance_micros(m_qbdf_builder, productId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                                MDPrice(), MDPrice(), MDPrice(), 0, m_auction.m_imbalance_size, m_auction.m_imbalance_direction, ' ');
          }
          if(m_subscribers)
          {
            m_subscribers->update_subscribers(m_auction);
          }
        }
        break;
      case MSG_TYPE_TO_U32_CONST('C', 'A'):
        {
          //const tmx_TL1::moc_price_movement_delay_message& msg = reinterpret_cast<const tmx_TL1::moc_price_movement_delay_message&>(*buf);
          // ToDo: publish a market status update (maybe) and an m_auction with the CCP (maybe)
        }
        break;
      case MSG_TYPE_TO_U32_CONST('D', ' '):
        {
          //const tmx_TL1::stock_state_message& msg = reinterpret_cast<const tmx_TL1::stock_state_message&>(*buf);
          // ToDo: publish a market status update
        }
        break;
      case MSG_TYPE_TO_U32_CONST('E', ' '):
        {
          const tmx_TL1::equity_quote_message& msg = reinterpret_cast<const tmx_TL1::equity_quote_message&>(*buf);
          ProductID productId = product_id_of_symbol(msg.symbol, sizeof(msg.symbol));
          if(productId == Product::invalid_product_id)
          {
            LOGV(cout << "Invalid product id" << endl);
            break;
          }
          m_quote.m_id = productId;
          m_quote.m_bid = F_PRICE(msg.bid_price, 1000.0);
          m_quote.m_bid_size = A_TO_U32(msg.bid_size); 
          m_quote.m_ask = F_PRICE(msg.ask_price, 1000.0);
          m_quote.m_ask_size = A_TO_U32(msg.ask_size);
          m_quote.m_quote_status = QuoteStatus::Normal;
          m_quote.strat_recv_time = Time::current_time();
          uint64_t usSinceMidnight = get_us_since_midnight(msg.trading_system_time_stamp);
          Time time = Time(Time::today().ticks() + usSinceMidnight * 1000);
          m_quote.set_time(time);

          m_msu.m_market_status = m_security_status[productId];
          m_feed_rules->apply_quote_conditions(m_quote, m_msu);
          if(m_msu.m_market_status != m_security_status[m_quote.m_id]) {
            m_msu.m_id = m_quote.m_id;
            m_msu.m_exch = m_quote.m_exch;
            m_security_status[m_quote.m_id] = m_msu.m_market_status;
            if(m_subscribers) {
              m_subscribers->update_subscribers(m_msu);
            }
          }

          if(m_qbdf_builder)
          {
            int32_t exch_time_delta = 0; // ToDo: populate
            char cond = ' ';
            gen_qbdf_quote_micros(m_qbdf_builder, m_quote.m_id, usSinceMidnight, exch_time_delta,
              m_qbdf_exch_char, cond, MDPrice::from_fprice(m_quote.m_bid), MDPrice::from_fprice(m_quote.m_ask),
              m_quote.m_bid_size, m_quote.m_ask_size);
          }
          if(m_provider)
          {
            m_provider->check_quote(m_quote);
          }
          update_subscribers(m_quote);

          // Also update imbalance
          if(m_quote.m_bid > 0)
          {
            if( m_quote.m_bid == m_quote.m_ask)
            {
              if(true) // ToDo: only continue if we're in a period of time where there should be an auction going on 
              {
                m_auction.m_id = productId;
                m_auction.m_ref_price = m_quote.m_bid;
                m_auction.m_near_price = m_quote.m_bid;
                m_auction.m_far_price = m_quote.m_bid;
                m_auction.m_paired_size = (m_quote.m_bid_size > m_quote.m_ask_size) ? m_quote.m_ask_size : m_quote.m_bid_size;
                m_auction.m_imbalance_size = (m_quote.m_bid_size > m_quote.m_ask_size) ? m_quote.m_bid_size - m_quote.m_ask_size : m_quote.m_ask_size - m_quote.m_bid_size;
                m_auction.m_imbalance_direction = (m_quote.m_bid_size > m_quote.m_ask_size) ? 'B' : m_quote.m_bid_size == m_quote.m_ask_size ? ' ' : 'S';
                if(m_qbdf_builder && !m_exclude_imbalance)
                {
                   /*
#ifdef CW_DEBUG
                   cout << Time::current_time().to_string() << "," 
                        << string(msg.symbol, sizeof(msg.symbol)) << "," 
                        << m_auction.m_ref_price << "," 
                        << m_auction.m_paired_size << ","
                        << m_auction.m_imbalance_size << "," 
                        << m_auction.m_imbalance_direction << ","
                        << m_quote.m_bid_size << ","
                        << m_quote.m_ask_size << endl;
#endif
                   */
                   uint64_t micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
                   int64_t micros_exch_time_delta = static_cast<int64_t>(micros_recv_time) - usSinceMidnight;
                   MDPrice mdPrice = MDPrice::from_fprice(m_quote.m_bid);
                   gen_qbdf_imbalance_micros(m_qbdf_builder, productId, micros_recv_time, micros_exch_time_delta, m_qbdf_exch_char,
                     mdPrice, mdPrice, mdPrice, m_auction.m_paired_size, m_auction.m_imbalance_size, m_auction.m_imbalance_direction, ' ');
                }
                if(m_subscribers)
                {
                  m_subscribers->update_subscribers(m_auction);
                }
                product_id_to_imbalance_state[productId] = true;
              }
            }
            else if (m_quote.m_bid > m_quote.m_ask && m_quote.m_ask != 0)
            {
              string symbol(msg.symbol, sizeof(msg.symbol));
              m_logger->log_printf(Log::WARN, "TMX_TL1_Handler::parse market is crossed for %s bid=%f ask=%f", symbol.c_str(), m_quote.m_bid, m_quote.m_ask);
            }
            else
            {
              hash_map<ProductID, bool>::iterator it = product_id_to_imbalance_state.find(m_quote.m_id);
              if(it != product_id_to_imbalance_state.end() && it->second == true)
              {
                // No imbalance
                MDPrice emptyPrice;
                m_auction.m_id = productId;
                m_auction.m_ref_price = 0.0;
                m_auction.m_near_price = 0.0;
                m_auction.m_far_price = 0.0;
                m_auction.m_paired_size = 0.0;
                m_auction.m_imbalance_size = 0;
                m_auction.m_imbalance_direction = ' ';
                if(m_qbdf_builder && !m_exclude_imbalance)
                {
                   uint64_t micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
                   int64_t micros_exch_time_delta = static_cast<int64_t>(micros_recv_time) - usSinceMidnight;
                   gen_qbdf_imbalance_micros(m_qbdf_builder, productId, micros_recv_time, micros_exch_time_delta, m_qbdf_exch_char,
                     emptyPrice, emptyPrice, emptyPrice, 0, 0, ' ', ' ');
                }
                if(m_subscribers)
                {
                  m_subscribers->update_subscribers(m_auction);
                }
                product_id_to_imbalance_state[m_quote.m_id] = false;
              }
            } 
          } 
          
        }
        break;
      case MSG_TYPE_TO_U32_CONST('G', ' '):
        {
          const tmx_TL1::general_message& msg = reinterpret_cast<const tmx_TL1::general_message&>(*buf);
          string text(msg.message_text, sizeof(msg.message_text));
          m_logger->log_printf(Log::WARN, "TMX_TL1_Handler::general message: bulletin_indicator=%c text=%s", msg.bulletin_indicator, text.c_str());
        }
        break;
      case MSG_TYPE_TO_U32_CONST('H', ' '):
        {
          if(m_qbdf_builder)
          {
            const tmx_TL1::equity_trade_cancellation_message& msg = reinterpret_cast<const tmx_TL1::equity_trade_cancellation_message&>(*buf);
            ProductID productId = product_id_of_symbol(msg.symbol, sizeof(msg.symbol));
            if(productId == Product::invalid_product_id)
            {
              LOGV(cout << "Invalid product id" << endl);
              break;
            }
            uint64_t usSinceMidnight = get_us_since_midnight(msg.trading_system_time_stamp);
            uint32_t originalTradeId = A_TO_U32(msg.original_trade_id);
            gen_qbdf_cancel_correct(m_qbdf_builder, productId,
                                     usSinceMidnight / Time_Constants::micros_per_mili,
                                     0, string(msg.symbol, sizeof(msg.symbol)), "CORRECTION", originalTradeId, 0.0,
                                     0.0, "   ", 0, 0, "    ");
          }
        }
        break;
      case MSG_TYPE_TO_U32_CONST('S', ' '):
        {
          //const tmx_TL1::market_state_message& msg = reinterpret_cast<const tmx_TL1::market_state_message&>(*buf);
          // If anything, I think just log this 
        }
        break;
      case MSG_TYPE_TO_U32_CONST('T', ' '):
        {
          //const tmx_TL1::trading_tier_status_message& msg = reinterpret_cast<const tmx_TL1::trading_tier_status_message&>(*buf);
          // If anything, just log
        }
        break;
      case MSG_TYPE_TO_U32_CONST('X', ' '):
        {
          if(m_qbdf_builder)
          {
            const tmx_TL1::equity_trade_correction_message& msg = reinterpret_cast<const tmx_TL1::equity_trade_correction_message&>(*buf);
            ProductID productId = product_id_of_symbol(msg.symbol, sizeof(msg.symbol));
            if(productId == Product::invalid_product_id)
            {
              LOGV(cout << "Invalid product id" << endl);
              break;
            }
            uint64_t usSinceMidnight = get_us_since_midnight(msg.trading_system_time_stamp);
            uint32_t originalTradeId = A_TO_U32(msg.trade_id);
            double correctTradePrice = F_PRICE(msg.trade_price, 10000.0);
            uint32_t correctTradeVolume = A_TO_U32(msg.volume);
            char tradeQualifiers[4];
            tradeQualifiers[0] = msg.cross_type;
            tradeQualifiers[1] = msg.moc;
            tradeQualifiers[2] = msg.opening_trade;
            tradeQualifiers[3] = msg.settlement_terms;
            string correctTradeConditions = string(tradeQualifiers, sizeof(tradeQualifiers));

            gen_qbdf_cancel_correct(m_qbdf_builder, productId,
                                     usSinceMidnight / Time_Constants::micros_per_mili,
                                     0, string(msg.symbol, sizeof(msg.symbol)), "CORRECTION", originalTradeId, 0.0,
                                     0.0, "   ", correctTradePrice, correctTradeVolume, correctTradeConditions);
          }
        }
        break;
      default:
        {
          
        }
        break;
    }
    return len;
  }

  size_t
  TMX_TL1_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
  {
    /*
    const tmx_TL1::protocol_framing& frame = reinterpret_cast<const tmx_TL1::protocol_framing&>(*buf);
    uint8_t start = frame.frame_start;
    char proto_n = frame.protocol_name;
    char proto_v = frame.protocol_version;
    uint16_t msg_len = frame.message_length;
    
    log.printf("    frame = start: %u proto_n: %c proto_v: %c msg_len: %u\n", start, proto_n, proto_v, msg_len);

    const tmx_TL1::protocol_framing_header& frame_header = reinterpret_cast<const tmx_TL1::protocol_framing_header&>(*(buf+5));
    uint32_t session_id = frame_header.session_id;
    char ac_re_po_du = frame_header.ack_req_poss_dup;
    uint8_t num_body = frame_header.num_body;
    
    log.printf("    frame_header = session_id: %u ack_req_poss_dup: %c num_body: %u\n", session_id, ac_re_po_du, num_body);
   
    if (num_body >= 0) {
      const tmx_TL1::generic_header& generic_hdr = reinterpret_cast<const tmx_TL1::generic_header&>(*(buf+5+6));
      uint16_t generic_msg_len = generic_hdr.length;
      uint8_t  msg_type = generic_hdr.message_type;
      uint8_t  ad_id_ms_ve = generic_hdr.admin_id_msg_ver;
      
      //log.printf("generic_hdr: msg_len: %d msg_type: [%c] (hex: %x) ad_id_ms_ve: %u\n", generic_msg_len, msg_type, msg_type, ad_id_ms_ve);
      log.printf("generic_hdr: session_id: %u msg_len: %d msg=%c\n",session_id, generic_msg_len, msg_type);
      
      log.printf("%s : ", Time::current_time().to_string().c_str()); 
      switch(msg_type) {
      }
    }
    return len;
    */ return 0;
  }

  void
  TMX_TL1_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_qbdf_exch_char, -1);
      }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }

  void
  TMX_TL1_Handler::reset(size_t context, const char* msg)
  {
    m_logger->log_printf(Log::WARN, "TMX_TL1_Handler::reset");

    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    if(m_subscribers) {
      // nothing for now
    }

      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_qbdf_exch_char, context);
      }
  }

  void
  TMX_TL1_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);

    for(size_t channelNo = 0; channelNo  < 12; ++channelNo) // Really there are only two TMX TL1 channels
    {
      m_channel_info_map.push_back(channel_info("", channelNo));
    }

    m_name = name;

    m_midnight = Time::today();

    if (exch == ExchangeID::XTSE) {
      m_qbdf_exch_char = 't';
    } else {
      m_logger->log_printf(Log::ERROR, "TMX_TL1_Handler::init: Unrecognized ExchangeID");
      m_qbdf_exch_char = ' ';
    }

    m_quote.m_exch = exch;
    m_trade.m_exch = exch;
    m_auction.m_exch = exch;
    m_msu.m_exch = exch;

    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';

    m_seen_product.resize(m_app->pm()->max_product_id(), false);

    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));
    //m_feed_rules = m_app->fm()->feed_rule(feed_rules_name);

    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);

    if(params.has("record_file")) {
      m_recorder = m_app->lm()->get_logger(params["record_file"].getString());
    }
    m_record_only = params.getDefaultBool("record_only", false);

    // ToDo: if m_qbdf_builder, write this to the qbdf output dir?
    //m_stock_group_logger = m_app->lm()->get_logger("STOCK_GROUP", "csv"); 
    //m_stock_group_logger->printf("timestamp_seconds,symbol,stock_group,board_lot_size\n");
  }

  void
  TMX_TL1_Handler::start()
  {
  }

  void
  TMX_TL1_Handler::stop()
  {
  }

  void
  TMX_TL1_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
                                       const string& mdSymbol_,const string& mdExch_)
  {
    mutex_t::scoped_lock lock(m_mutex);

    char symbol[9];
    if(mdExch_.empty()) {
      snprintf(symbol, 9, "%s", mdSymbol_.c_str());
    } else {
      snprintf(symbol, 9, "%s.%s", mdSymbol_.c_str(), mdExch_.c_str());
    }

    uint64_t s = int_of_symbol(symbol, 8);

    if(m_prod_map.count(s) > 0) {
      m_logger->log_printf(Log::ERROR, "Mapping %s to %zd COLLIDES", symbol, s);
    }

    m_prod_map.insert(make_pair(s, id_));
  }

  TMX_TL1_Handler::TMX_TL1_Handler(Application* app) :
    Handler(app, "TMX_TL1"),
    m_hist_mode(false),
    m_next_date_id(0),
    m_number_of_terms_orders_seen(0)
  {
  }

  TMX_TL1_Handler::~TMX_TL1_Handler()
  {
  }


}
