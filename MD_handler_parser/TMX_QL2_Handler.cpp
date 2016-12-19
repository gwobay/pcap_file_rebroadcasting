#include <Data/QBDF_Util.h>
#include <Data/TMX_QL2_Handler.h>
#include <Util/Network_Utils.h>

#include <Exchange/Exchange_Manager.h>
#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <MD/MDProvider.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>

namespace hf_core {

  namespace tmx_QL2 {

    struct protocol_framing {
      uint8_t  frame_start;
      char     protocol_name;
      char     protocol_version;
      uint16_t message_length;
    } __attribute__ ((packed));

    struct protocol_framing_header {
      uint32_t session_id;
      char     ack_req_poss_dup;
      uint8_t  num_body;
    } __attribute__ ((packed));

    struct generic_header {
      uint16_t length;
      uint8_t  message_type; 
      uint8_t  admin_id_msg_ver;

    } __attribute__ ((packed));

    struct admin_header {
      uint16_t length;
      uint8_t  message_type;
      uint8_t  admin_id;
    } __attribute__ ((packed));

    struct business_header {
      uint16_t length;
      uint8_t  message_type;
      uint8_t  message_version;
      char     source_id;
      uint16_t stream_id;
      uint8_t  sequence_0;   // for future expansion
      uint32_t sequence_1;   // max value == 4,000,000,000
    } __attribute__ ((packed));

    // admin messages

    struct heartbeat_message { // '0'
      admin_header header;
      uint16_t hb_msecs;
    } __attribute__ ((packed));

    struct operation_message { // '8'
      admin_header header;
      uint8_t operation_code;
      char message[100];
    } __attribute__ ((packed));

    // start of day messages

    struct broker_number_and_order_id
    {
      uint16_t broker_number;
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
    } __attribute__ ((packed));

    struct assign_cop_orders_message {  // 'A'
      business_header header;
      char symbol[9];
      uint64_t cop;     // px/1,000,000  calculated_opening_price
      uint8_t  order_side; // "B|S"
      broker_number_and_order_id orders[15];
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct assign_cop_no_orders_message {  // 'B'
      business_header header;
      char symbol[9];
      uint64_t cop;     // px/1,000,000  calculated_opening_price
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct assign_limit_entry
    {
      uint16_t broker_number;
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
      uint64_t price;
    } __attribute__ ((packed));

    struct assign_limit_message {  // 'C'
      business_header header;
      char symbol[9];
      uint64_t cop;     // px/1,000,000  calculated_opening_price
      uint8_t  order_side; // "B|S"
      assign_limit_entry orders[15];
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct symbol_status_message { // 'J'
      business_header header;
      char symbol[9];
      uint8_t stock_group;
      char cusip[12];
      uint16_t board_lot;
      uint8_t currency;
      uint64_t face_value;
      uint64_t last_sale;
    } __attribute__ ((packed));

    struct order_book_message {  // 'G'
      business_header header;
      char symbol[9];
      uint16_t broker_number;
      uint8_t order_side; // "B|S"
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint64_t priority_ts; //epoch in micros
    } __attribute__ ((packed));

    struct order_book_terms_message {  // 'j'
      business_header header;
      char symbol[9];
      uint16_t broker_number;
      uint8_t order_side; // "B|S"
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint8_t  non_resident; // "Y|N"
      uint8_t  settlement_terms; // "C|N|M|T|D"
      uint32_t settlement_date;   // YYYYMMDD
      uint64_t priority_ts; //epoch in micros
    } __attribute__ ((packed));

    // special market/stock state messages

    // intraday messages

    struct market_state_update_message {  // 'E'
      business_header header;
      uint8_t  market_state; // "P|O|S|C|R|F|N|M|A|E|L"
      uint8_t  stock_group;
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct moc_imbalance_message { // 'F'
      business_header header;
      char symbol[9];
      uint8_t  imbalance_side; // "B|S| "
      uint32_t imbalance_volume;
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct stock_status_message { // 'I'
      business_header header;
      char symbol[9];
      char comment[40];
      char stock_state[2];
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));


    struct order_booked_message { // 'P'
      business_header header;
      char symbol[9];
      uint16_t broker_number;
      char order_side; // 'B|S'
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint64_t priority_ts; //epoch in micros
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));


    struct order_cancelled_message { // 'Q'
      business_header header;
      char symbol[9];
      uint16_t broker_number;
      uint8_t order_side; // 'B|S'
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct order_price_time_assigned_message { // 'R'
      business_header header;
      char symbol[9];
      uint16_t broker_number;
      uint8_t order_side; // 'B|S'
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint64_t priority_ts; //epoch in micros
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));


    struct trade_report_message {  // 'S'
      business_header header;
      char symbol[9];
      uint32_t trade_number;
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint16_t buy_broker_number;
      uint64_t buy_order_id;  // YYYYMMDDnnnnnnnnn
      uint32_t buy_display_volume;
      uint16_t sell_broker_number;
      uint64_t sell_order_id;  // YYYYMMDDnnnnnnnnn
      uint32_t sell_display_volume;
      uint8_t  bypass; // "Y|N"
      uint32_t trade_ts;   // HHMMSS (manually set)
      uint8_t  cross_type; // "I|B|C|S|V"
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));


    struct trade_cancelled_message {  // 'T' 
      business_header header;
      char symbol[9];
      uint32_t trade_number;
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));


    struct trade_correction_message {  // 'U'
      business_header header;
      char symbol[9];
      uint32_t trade_number;
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint16_t buy_broker_number;
      uint16_t sell_broker_number;
      uint8_t  initiated_by; // "B|S|C"
      uint32_t orig_trade_number;
      uint8_t  bypass; // "Y|N"
      uint32_t trade_ts;   // HHMMSS (manually set)
      uint8_t  cross_type; // "I|B|C|S|V"
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));


    struct order_booked_terms_message { // 'm'
      business_header header;
      char symbol[9];
      uint16_t broker_number;
      uint8_t order_side; // 'B|S'
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint8_t  non_resident; // "Y|N"
      uint8_t  settlement_terms; // "C|N|M|T|D"
      uint32_t settlement_date;   // YYYYMMDD
      uint64_t priority_ts; //epoch in micros
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct order_cancelled_terms_message { // 'n'
      business_header header;
      char symbol[9];
      uint16_t broker_number;
      uint8_t order_side; // 'B|S'
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct order_price_time_assigned_terms_message { // 'o'
      business_header header;
      char symbol[9];
      uint16_t broker_number;
      uint8_t order_side; // 'B|S'
      uint64_t order_id;  // YYYYMMDDnnnnnnnnn
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint64_t priority_ts; //epoch in micros
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct trade_report_terms_message {  // 'p'
      business_header header;
      char symbol[9];
      uint32_t trade_number;
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint16_t buy_broker_number;
      uint64_t buy_order_id;  // YYYYMMDDnnnnnnnnn
      uint32_t buy_display_volume;
      uint16_t sell_broker_number;
      uint64_t sell_order_id;  // YYYYMMDDnnnnnnnnn
      uint32_t sell_display_volume;
      uint32_t trade_ts;   // HHMMSS (manually set)
      uint8_t  non_resident; // "Y|N"
      uint8_t  settlement_terms; // "C|N|M|T|D"
      uint32_t settlement_date;   // YYYYMMDD
      uint8_t  cross_type; // "I|B|C|S|V"
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct trade_cancelled_terms_message {  // 'q' 
      business_header header;
      char symbol[9];
      uint32_t trade_number;
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed));

    struct trade_correction_terms_message {  // 'r'
      business_header header;
      char symbol[9];
      uint32_t trade_number;
      uint64_t price;     // px/1,000,000
      uint32_t volume;
      uint16_t buy_broker_number;
      uint16_t sell_broker_number;
      uint8_t  initiated_by; // "B|S|C"
      uint32_t orig_trade_number;
      uint32_t trade_ts;   // HHMMSS (manually set)
      uint8_t  non_resident; // "Y|N"
      uint8_t  settlement_terms; // "C|N|M|T|D"
      uint32_t settlement_date;   // YYYYMMDD
      uint8_t  cross_type; // "I|B|C|S|V"
      uint64_t trading_system_ts;  //epoch in micros
    } __attribute__ ((packed)); 

 
   // START HERE 

    struct multicast_packet_header {
      uint32_t sequence;
      uint16_t message_count;
    } __attribute__ ((packed));

    struct multicast_data_header {
      uint16_t length;
    } __attribute__ ((packed));

    struct session_message_header {
      char      session_message_type;
    } __attribute__ ((packed));

    struct common_data_header {
      char timestamp[8];
      char message_type;
    } __attribute__ ((packed));

    struct system_event_message {
      char timestamp[8];
      char message_type;
      char event_code;
    } __attribute__ ((packed));

    struct order_add_message {
      common_data_header header;
      char order_ref[9];
      char side;
      char shares[6];
      char stock[6];
      char price[10];
      char display;
    } __attribute__ ((packed));

    struct order_add_message_long {
      common_data_header header;
      char order_ref[9];
      char side;
      char shares[10];
      char stock[6];
      char price[19];
      char display;
    } __attribute__ ((packed));

    struct order_exec_message {
      common_data_header header;
      char order_ref[9];
      char exec_shares[6];
      char trade_ref[9];
      char contra_order_ref[9];
      char tick_direction;
    } __attribute__ ((packed));

    struct order_exec_message_long {
      common_data_header header;
      char order_ref[9];
      char exec_shares[10];
      char trade_ref[9];
      char contra_order_ref[9];
      char tick_direction;
    } __attribute__ ((packed));

    struct order_cancel_message {
      common_data_header header;
      char order_ref[9];
      char cancelled_shares[6];
    } __attribute__ ((packed));

    struct order_cancel_message_long {
      common_data_header header;
      char order_ref[9];
      char cancelled_shares[10];
    } __attribute__ ((packed));

    struct trade_message {
      common_data_header header;
      char order_ref[9];
      char side;
      char shares[6];
      char stock[6];
      char price[10];
      char trade_ref[9];
      char contra_order_ref[9];
    } __attribute__ ((packed));

    struct trade_message_long {
      common_data_header header;
      char order_ref[9];
      char side;
      char shares[10];
      char stock[6];
      char price[19];
      char trade_ref[9];
      char contra_order_ref[9];
    } __attribute__ ((packed));

    struct broken_trade_message {
      common_data_header header;
      char trade_ref[9];
    } __attribute__ ((packed));

    /*
    struct stock_status_message {
      common_data_header header;
      char stock[6];
      char trading_state;
      char reserved;
    } __attribute__ ((packed));
    */
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
  TMX_QL2_Handler::product_id_of_symbol(const char* symbol, int len)
  {
    if (m_qbdf_builder) {
      string symbol_string(symbol, len);
      size_t whitespace_posn = symbol_string.find_last_not_of(" ");
      if (whitespace_posn != string::npos)
        symbol_string = symbol_string.substr(0, whitespace_posn+1);

      ProductID symbol_id = m_qbdf_builder->process_taq_symbol(symbol_string.c_str());
      if (symbol_id == 0) {
        m_logger->log_printf(Log::ERROR, "%s: Unable to create or find id for symbol [%s]\n",
                             "TMX_QL2_Handler::product_id_of_symbol", symbol);
        //cout << "symbol_string = '" << symbol_string << "' len=" << symbol_string.size() << endl;
        m_logger->sync_flush();
        return Product::invalid_product_id;
      }
      hash_map<ProductID, string>::iterator it = product_id_to_symbol.find(symbol_id);
      if(it == product_id_to_symbol.end())
      {
        product_id_to_symbol[symbol_id] = symbol_string;
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


  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  if(data_message_length != sizeof(type)) {                                             \
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), data_message_length, sizeof(type)); \
    return data_message_length;                                                         \
  }                                                                     \

#define LOGV(x)       \
  if(false) {         \
    x;                \
  }                   \

#define SEEN(id)                                                \
  if(!m_seen_product.test(id)) {                                \
    m_seen_product.set(id);                                     \
  }


  void TMX_QL2_Handler::publishImbalance(ProductID productId, Time& exchTimestamp, MDPrice& copPrice, uint32_t totalBuySize, uint32_t totalSellSize)
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
      string symbol = product_id_to_symbol[productId];
      //uint32_t bidSize = m_order_book->book(productId).price_level('B', copPrice)->total_shares;
      //uint32_t askSize = m_order_book->book(productId).price_level('S', copPrice)->total_shares;
      //cout << Time::current_time().to_string() << "," << symbol << "," << m_auction.m_ref_price << "," << m_auction.m_paired_size << ","
      //     << m_auction.m_imbalance_size << "," << m_auction.m_imbalance_direction << ","
      //     << bidSize << "," << askSize << endl;

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

  inline void TMX_QL2_Handler::parseInnerMessage(const char*buf)
  {
    //const char* where = "TMX_QL2_Handler::parse2";

    LOGV(cout << "parse2: buf=" << (const size_t)buf << endl);

    const tmx_QL2::business_header& businessHeader = reinterpret_cast<const tmx_QL2::business_header&>(*buf);
    const uint8_t msgType = businessHeader.message_type;

    LOGV(cout << "msgType=" << msgType << endl);
    // Might need to grab msgVersion if I see multiple versions for some messages
    switch(msgType)
    {
      // SOD messages
      case 'J': // Symbol Status
        {
	  const tmx_QL2::symbol_status_message& symbolStatus = reinterpret_cast<const tmx_QL2::symbol_status_message&>(*buf);
          //cout << "symbol = '" << symbolStatus.symbol << "'" << endl;
          const uint8_t stockGroup = symbolStatus.stock_group;
          //cout << "stockGroup=" << static_cast<unsigned>(stockGroup) << endl;
          const uint16_t boardLotSize = symbolStatus.board_lot;

          //m_stock_group_logger->printf("%lld,%s,%d,%d\n", Time::current_time().total_sec(), string(symbolStatus.symbol, sizeof(symbolStatus.symbol)).c_str(), stockGroup, boardLotSize);

          // ToDo: Persist these somewhere?
	  ProductID productId = product_id_of_symbol(symbolStatus.symbol, sizeof(symbolStatus.symbol));
          if (productId != Product::invalid_product_id)
          {
            //cout << "productId=" << productId << endl;
            product_id_to_board_lot_size[productId] = boardLotSize;

            hash_map<uint8_t, vector<ProductID> >::iterator sgIt = stock_group_to_product_ids.find(stockGroup);
            if(sgIt == stock_group_to_product_ids.end())
            {
              stock_group_to_product_ids[stockGroup] = vector<ProductID>();
            }
            
            hash_map<ProductID, uint8_t>::iterator productIt = product_id_to_stock_group.find(productId);
            if(productIt != product_id_to_stock_group.end())
            {//Check to see if it changed
              uint8_t existingStockGroup = productIt->second;
              if(existingStockGroup != stockGroup)
              {
                // Remove from old
                vector<ProductID>& products = stock_group_to_product_ids[existingStockGroup];
                vector<ProductID>::iterator it = products.begin();
                while(it != products.end())
                {
                  if (*it == productId)
                    it = products.erase(it);
                  else
                    ++it;
                }
                // Add to new
                stock_group_to_product_ids[stockGroup].push_back(productId);
                // Update map
                product_id_to_stock_group[productId] = stockGroup;
              }
            }
            else
            {// Need to add
              stock_group_to_product_ids[stockGroup].push_back(productId);
              product_id_to_stock_group[productId] = stockGroup;
            } 
          }
        }
         break;
      case 'G': // Order Book
         {
	    const tmx_QL2::order_book_message& orderBooked = reinterpret_cast<const tmx_QL2::order_book_message&>(*buf);
            LOGV(cout << "symbol = '" << orderBooked.symbol << "'" << endl);
	    ProductID productId = product_id_of_symbol(orderBooked.symbol, sizeof(orderBooked.symbol));
            if (productId != Product::invalid_product_id)
            {
              SEEN(productId);          
	      uint64_t exchOrderId = orderBooked.order_id;
              uint16_t brokerNumber = orderBooked.broker_number; 
              uint64_t orderId = getUniqueOrderId(exchOrderId, productId, brokerNumber);
              LOGV(cout << "OrderBook (SOD): orderId=" << orderId << endl);
              LOGV(cout << "exchOrderId=" << exchOrderId << " productId=" << productId << " brokerNumber=" << brokerNumber << endl);
              uint64_t priceRaw = orderBooked.price;
              uint32_t volume = orderBooked.volume;
              if(ignore_due_to_odd_lot(productId, volume)) 
              { 
                break; 
              }
              uint64_t priorityTs = orderBooked.priority_ts;  // Lack of trading_system_ts is what makes this SOD order_book message different from the intraday order_booked_message
              Time exchTimestamp(priorityTs * Time_Constants::ticks_per_micro);
              exchTimestamp = Time::current_time(); // Overriding the exchange timestamp since it could be extremely old
              char side = orderBooked.order_side;

              if(m_order_book) 
              {
                MDPrice price = MDPrice::from_iprice64(priceRaw*100); 
                LOGV(cout << "price=" << static_cast<double>(priceRaw) / 1000000.0 << " side=" << side << " size=" << volume << endl);
                bool inside_changed = m_order_book->add_order_ip(exchTimestamp, orderId, productId, side, price, volume);
                if(m_qbdf_builder && !m_exclude_level_2)
                {
                  m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
                  m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
                  m_micros_exch_time = Time::current_time().total_usec() - Time::today().total_usec() - 100; // Trying this
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                  gen_qbdf_order_add_micros(m_qbdf_builder, productId, orderId, m_micros_recv_time,
                                            m_micros_exch_time_delta, m_qbdf_exch_char,
                                            side, volume, price);
                }
                if(inside_changed) 
                {
                  m_quote.set_time(exchTimestamp);
                  m_quote.m_id = productId;
                    send_quote_update();
                }
              }
            }
            else
            {
              if(m_qbdf_builder)
              {
#ifdef CW_DEBUG
                cout << "Invalid product id" << endl;
#endif
              }
            }
         }
         break;
      case 'j': // Order Book - Terms
         {
#ifdef CW_DEBUG
           cout << "Order Book - Terms" << endl;
#endif
	   const tmx_QL2::order_book_terms_message& orderBooked = reinterpret_cast<const tmx_QL2::order_book_terms_message&>(*buf);
           LOGV(cout << "symbol = '" << orderBooked.symbol << "'" << endl);
	   ProductID productId = product_id_of_symbol(orderBooked.symbol, sizeof(orderBooked.symbol));
           if (productId != Product::invalid_product_id)
           {
#ifdef CW_DEBUG
             cout << "product is valid" << endl;
#endif
           }
         }
         break;
      // Special Market/Stock State Messages
      case 'A': // Assign COP - Orders
         {
           LOGV(cout << " Got AssignCop" << endl);
	   const tmx_QL2::assign_cop_orders_message& assignCopOrders = reinterpret_cast<const tmx_QL2::assign_cop_orders_message&>(*buf);
	   ProductID productId = product_id_of_symbol(assignCopOrders.symbol, sizeof(assignCopOrders.symbol));
           if (productId != Product::invalid_product_id)
           { 
             uint64_t copPriceRaw = assignCopOrders.cop;
	     MDPrice copPrice = MDPrice::from_iprice64(copPriceRaw * 100); 
             uint64_t tradingSystemTs = assignCopOrders.trading_system_ts;
	     Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
             uint32_t totalImbalanceSideSize = 0;
             char side = ' ';
             const size_t numOrderSlots = sizeof(assignCopOrders.orders) / sizeof(tmx_QL2::broker_number_and_order_id);
             for(size_t i = 0; i < numOrderSlots; ++i)
             {
               const tmx_QL2::broker_number_and_order_id& order = assignCopOrders.orders[i];
               if(order.order_id == 0)
               {
                 break; // Assuming once we see order_id == 0 there will be no more valid orders
               }
               if(i == numOrderSlots - 1)
               {
                 LOGV(cout << "Full message of AssignCopOrders" << endl);
               }
               LOGV(cout << " Order " << i << ": broker_number=" << order.broker_number << " exchId=" << order.order_id << endl);
               uint64_t orderId = getUniqueOrderId(order.order_id, productId, order.broker_number);
	       bool insideChanged(false);
	       LOGV(std::cout << "AssignCop orderId=" << orderId << std::endl);
	       {
		 MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
		 MDOrder* order = m_order_book->unlocked_find_order(orderId);
		 if(order)
		 {
		   LOGV(std::cout << "Found order" << std::endl);
                   totalImbalanceSideSize += order->size;
                   side = order->side();
                   insideChanged |= order->is_top_level();
		   if(m_qbdf_builder && !m_exclude_level_2)
		   {
		     m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
                     m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
		     m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
		     gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, orderId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
		       order->side(), order->size, copPrice);
		   }
		   insideChanged |= order->is_top_level();
                   {
                     MDOrderBook::mutex_t::scoped_lock bookLock(m_order_book->book(order->product_id).m_mutex, true);
		     LOGV(cout << "Modifying order: copPrice=" << copPrice.fprice() << " side=" << order->side() << " size=" << order->size << endl);
		     m_order_book->unlocked_modify_order_ip(exchTimestamp, order, order->side(), copPrice, order->size);
                   }
                   insideChanged |= order->is_top_level();
		   if(insideChanged)
		   {
		     m_quote.m_id = order->product_id;
		   }
	         }
	         else { LOGV(cout << "AssignCop: Did not find order" << endl); }
	       }
	       if(insideChanged)
	       {
	         m_quote.set_time(exchTimestamp);
                 send_quote_update();
	       }
	       if(m_order_book->orderbook_update())
	       {
	         m_order_book->post_orderbook_update();
	       }
             }

             LOGV(cout << "AssignCop_Orders: symbol=" << string(assignCopOrders.symbol, 9) << " copPrice=" << copPrice.fprice() 
                       << " side=" << side << " totalImbalanceSideSize=" << totalImbalanceSideSize
                       << " timestamp=" << exchTimestamp.to_string() << endl);

           }
         }
         break;

      case 'B': // Assign COP - No Orders
         {
	   const tmx_QL2::assign_cop_no_orders_message& assignCop = reinterpret_cast<const tmx_QL2::assign_cop_no_orders_message&>(*buf);
	   ProductID productId = product_id_of_symbol(assignCop.symbol, sizeof(assignCop.symbol));
           if (productId != Product::invalid_product_id)
           { 
             uint64_t copPriceRaw = assignCop.cop;
	     MDPrice copPrice = MDPrice::from_iprice64(copPriceRaw * 100); 
             uint64_t tradingSystemTs = assignCop.trading_system_ts;
             Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
             m_auction.set_time(exchTimestamp);
             m_auction.m_id = productId;
             //m_auction.m_imbalance_size = 0;
             //m_auction.m_imbalance_direction = 0;
             LOGV(cout << "AssignCop: symbol=" << string(assignCop.symbol, 9) << " copPrice=" << copPrice.fprice() << " timestamp=" << exchTimestamp.to_string() << endl);
             if(m_qbdf_builder && !m_exclude_imbalance)
             {
               m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
               m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
               m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
               gen_qbdf_imbalance_micros(m_qbdf_builder, productId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                                         copPrice, MDPrice(), MDPrice(), 0 ,0, ' ', ' '); //copPrice, copPrice, 0, 0, ' ', ' ');
             }

             if(m_subscribers)
             {
               m_subscribers->update_subscribers(m_auction);
             }
           }
         }
         break;
      case 'C': // Assign Limit
         {
           LOGV(cout << " Got AssignLimit" << endl);
	   const tmx_QL2::assign_limit_message& assignLimitOrders = reinterpret_cast<const tmx_QL2::assign_limit_message&>(*buf);
	   ProductID productId = product_id_of_symbol(assignLimitOrders.symbol, sizeof(assignLimitOrders.symbol));
           if (productId != Product::invalid_product_id)
           { 
             uint64_t tradingSystemTs = assignLimitOrders.trading_system_ts;
	     Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
             for(size_t i = 0; i < sizeof(assignLimitOrders.orders) / sizeof(tmx_QL2::assign_limit_entry); ++i)
             {
               const tmx_QL2::assign_limit_entry& order = assignLimitOrders.orders[i];
               if(order.order_id == 0)
               {
                 break; // Assuming once we see order_id == 0 there will be no more valid orders
               }
               uint64_t newPriceRaw = order.price;
	       MDPrice newPrice = MDPrice::from_iprice64(newPriceRaw * 100); 
               LOGV(cout << " Order " << i << ": broker_number=" << order.broker_number << " exchId=" << order.order_id << endl);
               uint64_t orderId = getUniqueOrderId(order.order_id, productId, order.broker_number);
	       bool insideChanged(false);
	       LOGV(cout << "AssignCop orderId=" << orderId << std::endl);
	       {
		 MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
		 MDOrder* order = m_order_book->unlocked_find_order(orderId);
		 if(order)
		 {
		   LOGV(cout << "Found order" << endl);
                   insideChanged |= order->is_top_level();
		   if(m_qbdf_builder && !m_exclude_level_2)
		   {
		     m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
                     m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
		     m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
		     gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, orderId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
		       order->side(), order->size, newPrice);
		   }
		   insideChanged |= order->is_top_level();
		   if(insideChanged)
		   {
		     m_quote.m_id = order->product_id;
		   }
                   {
                     MDOrderBook::mutex_t::scoped_lock bookLock(m_order_book->book(order->product_id).m_mutex, true);
		     m_order_book->unlocked_modify_order_ip(exchTimestamp, order, order->side(), newPrice, order->size);
                   }
	         }
	         else { cout << "AssignLimit: Did not find order" << endl; }
	       }
	       if(insideChanged)
	       {
	         m_quote.set_time(exchTimestamp);
	         send_quote_update();
	       }
	       if(m_order_book->orderbook_update())
	       {
	         m_order_book->post_orderbook_update();
	       }
             }
           }
         }
         break;
      // Intraday messages;
      case 'E': // Market State Update
        {
	    const tmx_QL2::market_state_update_message& marketStateUpdate = reinterpret_cast<const tmx_QL2::market_state_update_message&>(*buf);
#ifdef CW_DEBUG
            cout << "Market state update: marketState=" << marketStateUpdate.market_state << " stockGroup=" << static_cast<unsigned int>(marketStateUpdate.stock_group) << endl;
#endif
            char marketState = marketStateUpdate.market_state;

            bool publishUpdate = false;
            bool inAuctionState = false;
            switch(marketState)
            {
              case 'P': // Pre-open
                break;
              case 'O': // Opening
                inAuctionState = true;
                break;
              case 'S': // Open
                m_msu.m_market_status = MarketStatus::Open;
                m_qbdf_status = QBDFStatusType::Open;
                publishUpdate = true;
                break;
              case 'C': // Closed
                break;
              case 'R': // Extended Hours Open
                break;
              case 'F': // Extended Hours Close
                break;
              case 'N': // Extended Hours CXLs
                break;
              case 'M': // MOC Imbalance
                break;
              case 'A': // CCP Determination
                inAuctionState = true;
                break;
              case 'E': // Price Movement Extension
                break;
              case 'L': // Closing
                break;
              default:
#ifdef CW_DEBUG
                cout << "Got unrecognized market state type " << marketState << endl;
#endif
                break;
             }

             uint8_t stockGroup = marketStateUpdate.stock_group;
             uint64_t tradingSystemTs = marketStateUpdate.trading_system_ts;
             Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);

             hash_map<uint8_t, vector<ProductID> >::iterator it =  stock_group_to_product_ids.find(stockGroup);
             if(it != stock_group_to_product_ids.end())
             {
               vector<ProductID>& affectedProducts = it->second;
               for(vector<ProductID>::iterator prodIt = affectedProducts.begin(); prodIt != affectedProducts.end(); ++prodIt)
               {
                 ProductID& productId = *prodIt;
 
                 if(inAuctionState)
                 {
                   product_ids_in_auction_state[productId] = marketState;
                 }
                 else
                 {
                   hash_map<ProductID, char>::iterator auctionStateIt = product_ids_in_auction_state.find(productId);
                   if(auctionStateIt != product_ids_in_auction_state.end())
                   {
                     product_ids_in_auction_state.erase(auctionStateIt);
                   }
                 }

                 if(publishUpdate)
                 {
                  m_msu.m_id = productId;
                  if(m_qbdf_builder && !m_exclude_status)
                  {
                    m_msu.set_time(exchTimestamp);
                    m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
                    if(m_hist_mode)
                    {
                      m_micros_recv_time = m_micros_exch_time;
                      m_micros_exch_time_delta = 0;
                    }
                    else
                    {
                      m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                    }
                    m_qbdf_status_reason = string("TMX Market State ") + marketState;
                    gen_qbdf_status_micros(m_qbdf_builder, productId, m_micros_recv_time, m_micros_exch_time_delta, 
                                           m_qbdf_exch_char, m_qbdf_status.index(), m_qbdf_status_reason);
                  }

                  m_security_status[productId] = m_msu.m_market_status;
                  if(m_subscribers)
                  {
                    m_subscribers->update_subscribers(m_msu);
                  }
                  m_msu.m_reason.clear();
                  LOGV(cout << "  ProductId=" << productId << endl);
                 }
               }
             }
             else
             {
#ifdef CW_DEBUG
               cout << "Stock group " << stockGroup << " not found in map, so we don't know which products to apply the market state update to" << endl;
#endif
             }
        }
         break;
      case 'F': // MOC Imbalance
        {
	    const tmx_QL2::moc_imbalance_message& mocImbalance = reinterpret_cast<const tmx_QL2::moc_imbalance_message&>(*buf);
            uint64_t tradingSystemTs = mocImbalance.trading_system_ts;
            //cout << "moc_imbalance tradingSystemTs=" << tradingSystemTs << endl;
            Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
            ProductID productId = product_id_of_symbol(mocImbalance.symbol, sizeof(mocImbalance.symbol));
            if (productId != Product::invalid_product_id)
            {          
              SEEN(productId);          
              uint32_t volume = mocImbalance.imbalance_volume;   
              uint64_t tradingSystemTs = mocImbalance.trading_system_ts;
              Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
              LOGV(cout << "moc_imbalance: symbol=" << string(mocImbalance.symbol, 9) << " side=" << mocImbalance.imbalance_side << " volume=" << volume << endl);
              m_auction.set_time(exchTimestamp);
              m_auction.m_id = productId;
              m_auction.m_imbalance_size = volume;
              m_auction.m_imbalance_direction = mocImbalance.imbalance_side;

              if(m_qbdf_builder && !m_exclude_imbalance)
              {
                // Should we even do this?  The MOC imbalance msg only has size and side
                m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
                m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
                m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                gen_qbdf_imbalance_micros(m_qbdf_builder, productId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                                          MDPrice(), MDPrice(), MDPrice(), 0, volume, mocImbalance.imbalance_side, ' ');
              }

              if(m_subscribers)
              {
                m_subscribers->update_subscribers(m_auction);
              }
            }
          break;
        }
      case 'P': // Order Booked
         {
	    const tmx_QL2::order_booked_message& orderBooked = reinterpret_cast<const tmx_QL2::order_booked_message&>(*buf);
            LOGV(cout << "Order Booked: symbol = '" << orderBooked.symbol << "'" << endl);

            
	    ProductID productId = product_id_of_symbol(orderBooked.symbol, sizeof(orderBooked.symbol));
            if (productId != Product::invalid_product_id)
            {          
              SEEN(productId);          
	      uint64_t exchOrderId = orderBooked.order_id;
              uint16_t brokerNumber = orderBooked.broker_number; 
              uint64_t orderId = getUniqueOrderId(exchOrderId, productId, brokerNumber);
              LOGV(std::cout << "OrderBooked: orderId=" << orderId << std::endl);
              uint64_t priceRaw = orderBooked.price;
              uint32_t volume = orderBooked.volume ;
              if(ignore_due_to_odd_lot(productId, volume)) 
              { 
                break; 
              }
              uint64_t tradingSystemTs = orderBooked.trading_system_ts;  // Note that there's also priority_ts, which we'll need for SOD OrderBook msg
              Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
              LOGV(cout << "tradingSystemTs=" << tradingSystemTs << endl);
              char side = orderBooked.order_side;

              if(m_order_book) 
              {
                MDPrice price = MDPrice::from_iprice64(priceRaw * 100); 
                LOGV(cout << "fprice=" << price.fprice() << " rawPrice=" << priceRaw << " volume=" << volume << " side=" << side << endl);
                bool inside_changed = m_order_book->add_order_ip(exchTimestamp, orderId, productId, side, price, volume);
                if(m_qbdf_builder && !m_exclude_level_2)
                {
                  m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
                  m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
                  m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                  gen_qbdf_order_add_micros(m_qbdf_builder, productId, orderId, m_micros_recv_time,
                                            m_micros_exch_time_delta, m_qbdf_exch_char,
                                            side, volume, price);
                }
                if(inside_changed) 
                {
                  m_quote.set_time(exchTimestamp);
                  m_quote.m_id = productId;
                  send_quote_update();

                  LOGV(cout << "ProductId = " << productId << endl);
                  MDOrderBook& mdOrderBook = m_order_book->book(productId);
                  MDBidBook& bidBook = mdOrderBook.bid();
                  MDAskBook& askBook = mdOrderBook.ask();
                  MDPriceLevel* bidTop = bidBook.top();
                  MDPriceLevel* askTop = askBook.top();
                  if(bidTop && askTop)
                  {
                    LOGV(cout << bidTop->total_shares << " " << bidTop->price_as_double() << " x " << askTop->price_as_double() << " " << askTop->total_shares << endl);
                    if(bidTop->price_as_double() >= askTop->price_as_double())
                    {
                      LOGV(cout << "Market is locked or crossed" << endl);
                    }
                  }
                  else
                  {
                    LOGV(cout << "For some reason bidTop or askTop is null" << endl);
                  }
                }
              }
            }
            else
            {
              if(m_qbdf_builder)
              {
                std::cout << "Invalid product id" << endl;
              }
            }
         }
         break;
      case 'm': // Order Booked - Terms
       {
	 const tmx_QL2::order_booked_terms_message& orderBooked = reinterpret_cast<const tmx_QL2::order_booked_terms_message&>(*buf);
         LOGV(cout << "Order Booked - Terms: symbol = '" << orderBooked.symbol << "'" << endl);
         LOGV(cout << "  settlement_terms=" << (char)orderBooked.settlement_terms << endl);
         LOGV(cout << "  settlement_date=" << orderBooked.settlement_date << endl);
         ++m_number_of_terms_orders_seen;
         if(m_number_of_terms_orders_seen % 10 == 0)
         {
#ifdef CW_DEBUG
           cout << "Have seen " << m_number_of_terms_orders_seen << " term orders" << endl;
#endif
         }
       }
         break;
      case 'Q': // Order Cancelled
         {
           if(m_order_book)
           {
             bool insideChanged = false;
             const tmx_QL2::order_cancelled_message& orderCancelled = reinterpret_cast<const tmx_QL2::order_cancelled_message&>(*buf);
             uint64_t exchOrderId = orderCancelled.order_id;
             ProductID productId = product_id_of_symbol(orderCancelled.symbol, sizeof(orderCancelled.symbol));
             if(productId == Product::invalid_product_id)
             {
               if(m_qbdf_builder)
               {
                 LOGV(cout << "Invalid product id" << endl);
               }
               return;
             }
             uint16_t brokerNumber = orderCancelled.broker_number; 
             uint64_t orderId = getUniqueOrderId(exchOrderId, productId, brokerNumber);
             uint64_t tradingSystemTs = orderCancelled.trading_system_ts;
             Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
             LOGV(cout << "OrderCancelled: orderId=" << orderId << endl);
             {
               MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
               MDOrder* order = m_order_book->unlocked_find_order(orderId);
               if(order)
               {
                 LOGV(cout << "Found order" << endl);
                 if(m_qbdf_builder && !m_exclude_level_2)
                 {
                   m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
                   m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
                   
                   m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                   gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, orderId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                     order->side(), 0, MDPrice());
                 }
                 insideChanged = order->is_top_level();
                 if(insideChanged)
                 {
                   m_quote.m_id = order->product_id;
                 }
  
                 {
                   MDOrderBook::mutex_t::scoped_lock bookLock(m_order_book->book(order->product_id).m_mutex, true);
                   m_order_book->unlocked_remove_order(exchTimestamp, order);
                 }
               }
               else 
               { 
                 LOGV(cout << "Did not find order: exchOrderId= " << exchOrderId << " productId=" << productId << " symbol=" << orderCancelled.symbol << endl); 
               }
             }
             if(insideChanged)
             {
               m_quote.set_time(exchTimestamp);
               send_quote_update();
             }
             if(m_order_book->orderbook_update())
             {
               m_order_book->post_orderbook_update();
             }
           }
         }
         break;
      case 'n': // Order Cancelled - Terms
         LOGV(cout << "Order Cancelled - Terms" << endl);
         break;
      case 'R': // Order Price-Time Assigned
         {
           if(m_order_book)
           {
             const tmx_QL2::order_price_time_assigned_message& modified = reinterpret_cast<const tmx_QL2::order_price_time_assigned_message&>(*buf);
             uint64_t exchOrderId = modified.order_id;
             ProductID productId = product_id_of_symbol(modified.symbol, sizeof(modified.symbol));
             if(productId == Product::invalid_product_id)
             {
               LOGV(cout << "Invalid product id" << endl);
               return;
             }
             uint16_t brokerNumber = modified.broker_number; 
             uint64_t orderId = getUniqueOrderId(exchOrderId, productId, brokerNumber);
             uint64_t newPriceRaw = modified.price;
             uint32_t newVolume = modified.volume;
             if(ignore_due_to_odd_lot(productId, newVolume))
             {  
               newVolume = 0;
             }
             char newSide = modified.order_side;
             uint64_t tradingSystemTs = modified.trading_system_ts;
             MDPrice newPrice = MDPrice::from_iprice64(newPriceRaw * 100); 
             Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
             bool insideChanged(false);
             LOGV(cout << "OrderPriceTimeAssigned: orderId=" << orderId << endl);
             {
	       MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
               MDOrder* order = m_order_book->unlocked_find_order(orderId);
               if(order)
               {
                 LOGV(cout << "Found order" << endl);
                 insideChanged |= order->is_top_level();
                 if(m_qbdf_builder && !m_exclude_level_2)
                 {
                   m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
                   m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
                   m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
                   gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, orderId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                     newSide, newVolume, newPrice);
                 }
                 insideChanged |= order->is_top_level();
                 if(insideChanged)
                 {
                   m_quote.m_id = order->product_id;
                 }
  
                 if(newVolume == 0)
                 {
                   LOGV(cout << "Got an order_price_time_assigned with volume=0, which is weird." << std::endl);
                   MDOrderBook::mutex_t::scoped_lock bookLock(m_order_book->book(order->product_id).m_mutex, true);
                   m_order_book->unlocked_remove_order(exchTimestamp, order);
                 }
                 else
                 {
                   MDOrderBook::mutex_t::scoped_lock bookLock(m_order_book->book(order->product_id).m_mutex, true);
                   m_order_book->unlocked_modify_order_ip(exchTimestamp, order, newSide, newPrice, newVolume);
                 }
               }
               else { LOGV(cout << "Did not find order" << endl); }
             }
             if(insideChanged)
             {
               m_quote.set_time(exchTimestamp);
               send_quote_update();
             }
             if(m_order_book->orderbook_update())
             {
               m_order_book->post_orderbook_update();
             }
           }
         }
         break;
      case 'o': // Order Price-Time Assigned - Terms
         break;
      case 'I': // Stock Status
         {
            // ToDo: Handle properly.  What do these tags mean?
	    const tmx_QL2::stock_status_message& stateUpdate = reinterpret_cast<const tmx_QL2::stock_status_message&>(*buf);
#ifdef CW_DEBUG
            cout << "StateUpdate: symbol = '" << stateUpdate.symbol << "'" << endl;
#endif
	    ProductID productId = product_id_of_symbol(stateUpdate.symbol, sizeof(stateUpdate.symbol));
            if (productId != Product::invalid_product_id)
            {
#ifdef CW_DEBUG
              cout << "state updated to " << stateUpdate.stock_state[0] << stateUpdate.stock_state[1] << endl;
#endif
              // AR - AuthorizedDelayed
              // IR - InhibitedDelayed
              // AS - AuthorizedHalted
              // IS - InhibitedHalted
              // AG - AuthorizedFrozen
              // IG - InhibitedFrozen
              // AE - Authorized Price Movement Delayed
              // AF - Authorized Price Movement Frozen
              // IE - Inhibited Price Movement Delayed
              // IF - Inhibited Price Movement Frozen
              // A  - Authorized
              // I  - Inhibited
             
              uint64_t tradingSystemTs = stateUpdate.trading_system_ts;
              Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);

              m_msu.set_time(exchTimestamp);
              m_msu.m_id = productId;
              // Anything other than 'A ' indicates that we'll either get rejected if we 
              // try to send orders, or our orders will go through but won't be live until trading resumes.
              if(stateUpdate.stock_state[0] != 'A' || stateUpdate.stock_state[1] != ' ')
              {
                // ToDo: publish a halt
                //m_msu.m_market_status = MarketStatus::Halted;
                //m_qbdf_status = QBDFStatusType::Halted;
              }
              else
              {
                // ToDo: publish open (but only if it's actually open?)
                //m_msu.m_market_status = m_security_status[productId];
               
              }
            }          
         }
         break;
      case 'S': // Trade Report
         {
           if(m_order_book)
           {
             bool insideChanged = false;
             const tmx_QL2::trade_report_message& tradeReport = reinterpret_cast<const tmx_QL2::trade_report_message&>(*buf);
             uint64_t exchBuyOrderId = tradeReport.buy_order_id;
             uint64_t exchSellOrderId = tradeReport.sell_order_id;
             ProductID productId = product_id_of_symbol(tradeReport.symbol, sizeof(tradeReport.symbol));
             LOGV(cout << "Trade: symbol=" << string(tradeReport.symbol, 9) << " productId=" << productId << endl);
             if(productId == Product::invalid_product_id)
             {
               if(m_qbdf_builder)
               {
#ifdef CW_DEBUG
                 cout << "Invalid product id" << endl;
#endif
               }
               return;
             }
             uint16_t buyBrokerNumber = tradeReport.buy_broker_number; 
             uint16_t sellBrokerNumber = tradeReport.sell_broker_number; 
             uint64_t buyOrderId = getUniqueOrderId(exchBuyOrderId, productId, buyBrokerNumber);
             uint64_t sellOrderId = getUniqueOrderId(exchSellOrderId, productId, sellBrokerNumber);
             char crossType = tradeReport.cross_type; 
             LOGV(cout << "symbol=" << string(tradeReport.symbol, 9) << endl);
             LOGV(cout << "Trade: buyOrderId=" << buyOrderId << " sellOrderId=" << sellOrderId << " crossType=" << crossType  << endl);
             uint32_t tradeVolume = tradeReport.volume;
             uint64_t tradingSystemTs = tradeReport.trading_system_ts;
             uint64_t tradePriceRaw = tradeReport.price;
             MDPrice tradePrice = MDPrice::from_iprice64(tradePriceRaw * 100); 
/*
             if(tradeReport.symbol[0] == 'I' && tradeReport.symbol[1] == 'F' && tradeReport.symbol[2] =='P')
             {
#ifdef CW_DEBUG
               cout << "Trade: symbol=" << string(tradeReport.symbol, 9) << " productId=" << productId << "tradePrice=" << tradePrice.fprice() << endl;
#endif
             }
*/
             bool orderFound(false);
             Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
             m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
             m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
             m_micros_exch_time_delta = m_micros_exch_time - m_micros_recv_time;
             uint32_t buyVisibleSharesExecuted(0);
             uint32_t sellVisibleSharesExecuted(0);
             //cout << "symbol=" << string(tradeReport.symbol, 9) << endl;
             //cout << "Trade: buyOrderId=" << buyOrderId << " sellOrderId=" << sellOrderId << " crossType=" << crossType  << " price=" << tradePrice.fprice() << " size=" << tradeVolume << endl;
             //cout << "timestamp=" << exchTimestamp.to_string() << endl;
             if(crossType != ' ')
             {
               LOGV(cout << "Abnormal cross type buyBrokerNumber=" << buyBrokerNumber << " sellBrokerNumber=" << sellBrokerNumber << endl);
             }

             char auctionStatus = ' ';
             hash_map<ProductID, char>::iterator auctionIt = product_ids_in_auction_state.find(productId);
             if(auctionIt != product_ids_in_auction_state.end())
             {
               auctionStatus = auctionIt->second;
             }
             //cout << "auctionStatus=" << auctionStatus << endl;

             // ToDo: try to ignore odd lot orders?  Do we need to put these odd lots in a separate book?
             //inline uint16_t getBoardLotSize(ProductID productId, MDPrice refPrice);

             // ToDo: ignore some non-standard cross trades?

             {
	       MDOrderBookHandler::mutex_t::scoped_lock lock(m_order_book->m_mutex);
               MDOrder* order = m_order_book->unlocked_find_order(buyOrderId);
               if(order)
               {
                 orderFound = true;
                 uint32_t buyDisplayVolume = tradeReport.buy_display_volume;
                 LOGV(cout << "Found order" << endl);
                 if (order->size >= buyDisplayVolume)
                 {
                   buyVisibleSharesExecuted = order->size - buyDisplayVolume;
                   if(m_qbdf_builder && buyVisibleSharesExecuted > 0) // && !m_exclude_level_2 ?
                   {
                     gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, buyOrderId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                       order->flip_side(), auctionStatus, buyVisibleSharesExecuted, tradePrice);
                   }
                   insideChanged = order->is_top_level();
                   if(insideChanged)
                   {
                     m_quote.m_id = order->product_id;
                   }
                 }
                 else
                 {
                   LOGV(cout << "Very strange: orderSize=" << order->size << " < buyDisplayVolume=" << buyDisplayVolume << endl);
                   if(m_qbdf_builder)
                   {
                     gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, buyOrderId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                       order->flip_side(), buyDisplayVolume, order->price);
                   }
                 }
                 if(buyDisplayVolume == 0)// || ignore_due_to_odd_lot(productId, buyDisplayVolume))
                 {
                   MDOrderBook::mutex_t::scoped_lock bookLock(m_order_book->book(order->product_id).m_mutex, true);
                   m_order_book->unlocked_remove_order(exchTimestamp, order);
                 }
                 else
                 {
                   MDOrderBook::mutex_t::scoped_lock bookLock(m_order_book->book(order->product_id).m_mutex, true);
                   m_order_book->unlocked_modify_order(exchTimestamp, order, buyDisplayVolume, tradeVolume);
                 }
               }

               order = m_order_book->unlocked_find_order(sellOrderId);
               if(order)
               {
                 uint32_t sellDisplayVolume = tradeReport.sell_display_volume;
                 LOGV(cout << "Found order sellDisplayVolume=" << sellDisplayVolume << endl);
                 if (!orderFound && (order->size >= sellDisplayVolume))
                 {
                   sellVisibleSharesExecuted = order->size - sellDisplayVolume;
                   if(m_qbdf_builder && sellVisibleSharesExecuted > 0) // && !m_exclude_level_2 ?
                   {
                     gen_qbdf_order_exec_micros(m_qbdf_builder, order->product_id, sellOrderId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                       order->flip_side(), auctionStatus, sellVisibleSharesExecuted, tradePrice);
                   }
                   insideChanged = insideChanged || order->is_top_level();
                   if(insideChanged)
                   {
                     m_quote.m_id = order->product_id;
                   }
                 }
                 else
                 {
                   if(order->size < sellDisplayVolume)
                   {
                     LOGV(cout << "Very strange: orderSize=" << order->size << " < sellDisplayVolume=" << sellDisplayVolume << endl);
                   }
                   if(m_qbdf_builder)
                   {
                     gen_qbdf_order_modify_micros(m_qbdf_builder, order->product_id, sellOrderId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char,
                       order->side(), sellDisplayVolume, order->price);
                   }
                 }
                 orderFound = true;
                 if (sellDisplayVolume == 0)// || ignore_due_to_odd_lot(productId, sellDisplayVolume))
                 {
                   MDOrderBook::mutex_t::scoped_lock bookLock(m_order_book->book(order->product_id).m_mutex, true);
                   m_order_book->unlocked_remove_order(exchTimestamp, order);
                 }
                 else
                 {
                   MDOrderBook::mutex_t::scoped_lock bookLock(m_order_book->book(order->product_id).m_mutex, true);
                   m_order_book->unlocked_modify_order(exchTimestamp, order, sellDisplayVolume, tradeVolume);
                 }
               }
             }

             if(auctionStatus != ' ')
             {
               LOGV(cout << "Auction trade: " << tradeVolume << "@" << tradePrice.fprice() 
                    << " buyVisibleSharesExecuted=" << buyVisibleSharesExecuted << " sellVisibleSharesExecuted=" << sellVisibleSharesExecuted << endl);
             }

             if(buyVisibleSharesExecuted && sellVisibleSharesExecuted)
             {
               LOGV(cout << "buyVisibleSharesExecuted=" << buyVisibleSharesExecuted << " sellVisibleSharesExecuted=" << sellVisibleSharesExecuted << endl);
             }
             uint32_t visibleSharesExecuted = buyVisibleSharesExecuted > sellVisibleSharesExecuted ? buyVisibleSharesExecuted : sellVisibleSharesExecuted;
             if(tradeVolume > visibleSharesExecuted)
             {// This means we got some non-visible size executed
               uint32_t nonVisibleSharesExecuted = tradeVolume - visibleSharesExecuted;
               if(m_qbdf_builder)                                                                                                 
               {                                                                                                                  // side, corr, cond
                 string auctionStatusStr("    ");
                 if(auctionStatus != ' ')
                 {
                   auctionStatusStr[0] = auctionStatus;
                 }
                 gen_qbdf_trade_micros(m_qbdf_builder, productId, m_micros_recv_time, m_micros_exch_time_delta, m_qbdf_exch_char, ' ', '0', auctionStatusStr,
                                       nonVisibleSharesExecuted, tradePrice);
               }
             }
             // ToDo: try to pass along a valid trade_condition?
             m_trade.m_id = productId;
             m_trade.clear_flags();
             m_trade.set_exec_flag(TradeUpdate::exec_flag_exec);
             m_trade.set_time(exchTimestamp);
             m_trade.m_price = tradePrice.fprice();
             m_trade.m_size = tradeVolume;
             m_trade.m_trade_qualifier[0] = auctionStatus;
             if(!m_qbdf_builder)
             {
               send_trade_update();
             }
             if(!orderFound)
             {
               LOGV(cout << "Did not find order. productId=" << productId << " buyExchId = " << exchBuyOrderId << " buyBrokerNumber=" << buyBrokerNumber << endl);
               LOGV(cout << "Did not find order. productId=" << productId << " sellExchId = " << exchSellOrderId << " sellBrokerNumber=" << sellBrokerNumber << endl);
             }

             if(insideChanged)
             {
               m_quote.set_time(exchTimestamp);
               send_quote_update();
             }
             if(m_order_book->orderbook_update())
             {
               m_order_book->post_orderbook_update();
             }
           }
         }
         break;
      case 'p': // Trade Report - Terms
         break;
      case 'T': // Trade Cancelled
        {
           if (m_qbdf_builder) 
           {
             const tmx_QL2::trade_cancelled_message& tradeCanceled = reinterpret_cast<const tmx_QL2::trade_cancelled_message&>(*buf);
             ProductID productId = product_id_of_symbol(tradeCanceled.symbol, sizeof(tradeCanceled.symbol));
             if(productId == Product::invalid_product_id)
             {
               if(m_qbdf_builder)
               {
#ifdef CW_DEBUG
                 cout << "Invalid product id" << endl;
#endif
               }
               return;
             }

             uint64_t tradingSystemTs = tradeCanceled.trading_system_ts;
             Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
             m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();

             gen_qbdf_cancel_correct(m_qbdf_builder, productId,
                                     m_micros_exch_time / Time_Constants::micros_per_mili,
                                     0, string(tradeCanceled.symbol, sizeof(tradeCanceled.symbol)), "CORRECTION", tradeCanceled.trade_number, 0.0,
                                     0.0, "   ", 0, 0, "    ");
           }
         }
         break;
      case 'q': // Trade Cancelled - Terms
         break;
      case 'U': // Trade Correction
         {
           if (m_qbdf_builder) 
           {
             const tmx_QL2::trade_correction_message& tradeCorrection = reinterpret_cast<const tmx_QL2::trade_correction_message&>(*buf);
             ProductID productId = product_id_of_symbol(tradeCorrection.symbol, sizeof(tradeCorrection.symbol));
             if(productId == Product::invalid_product_id)
             {
               if(m_qbdf_builder)
               {
#ifdef CW_DEBUG
                 cout << "Invalid product id" << endl;
#endif
               }
               return;
             }

             uint64_t tradingSystemTs = tradeCorrection.trading_system_ts;
             Time exchTimestamp(tradingSystemTs * Time_Constants::ticks_per_micro);
             m_micros_exch_time = exchTimestamp.total_usec() - Time::today().total_usec();
             double corr_price = MDPrice::from_iprice64(tradeCorrection.price * 100).fprice();

             gen_qbdf_cancel_correct(m_qbdf_builder, productId,
                                     m_micros_exch_time / Time_Constants::micros_per_mili,
                                     0, string(tradeCorrection.symbol, sizeof(tradeCorrection.symbol)), "CORRECTION", tradeCorrection.orig_trade_number, 0.0,
                                     0.0, "   ", corr_price, tradeCorrection.volume, "    ");
           }
         }
         break;
      case 'r': // Trade Correction - Terms
         break;
      default:
#ifdef CW_DEBUG
         cout << "Unrecognized message type '" << msgType << "'" << endl;
#endif
	 break;
    }

    return;
  }

  size_t
  TMX_QL2_Handler::parse(size_t context, const char* buf, size_t len)
  {
    // Only need the protocol frame if we want to check for packet completeness/cohesion / sanity checking    
    //const tmx_QL2::protocol_framing& frame = reinterpret_cast<const tmx_QL2::protocol_framing&>(*buf);
    //cout << "frame.message_length=" << frame.message_length << " len=" << len << endl;

    const char* bufPtr = buf + sizeof(tmx_QL2::protocol_framing);
    const tmx_QL2::protocol_framing_header& frameHeader = reinterpret_cast<const tmx_QL2::protocol_framing_header&>(*bufPtr);
    bufPtr += sizeof(tmx_QL2::protocol_framing_header);
    const tmx_QL2::generic_header& genericHeader = reinterpret_cast<const tmx_QL2::generic_header&>(*bufPtr);
    const uint8_t numBody = frameHeader.num_body;
    const uint8_t firstMsgType = genericHeader.message_type;
    switch(firstMsgType)
    {
      case 0x30: // Heartbeat
      {
         break;
      }
      case 0x31: // Login Request
      case 0x32: // Login Response
      case 0x33: // Logout message
      case 0x34: // Ack message
      case 0x35: // Replay request
      case 0x37: // Reserved
      case 0x38: // Operation message
      case 0x39: // Reject
      {
         //std::cout << "Admin msg: firstMsgType = " << static_cast<unsigned>(firstMsgType) << std::endl;  
         break;
      }
      case 0x36: // Sequence Jump : we probably care about this
      {
#ifdef CW_DEBUG
         cout << "Got a sequence jump msg!" << endl;
#endif
         break;
      }
      default:	// If it's not one of the above message types, then it's not an administrative message
      {
        const tmx_QL2::business_header& firstBusinessHeader = reinterpret_cast<const tmx_QL2::business_header&>(*bufPtr);
        const uint32_t seqNum = firstBusinessHeader.sequence_1;
        const uint16_t streamId = firstBusinessHeader.stream_id;
        const uint32_t nextSeqNum = seqNum + numBody;
       
        if(streamId > m_channel_info_map.size())
        {
          // Do something
#ifdef CW_DEBUG
          cout << "Stream ID " << streamId << " exceeds max " << m_channel_info_map.size() << endl;
#endif
        } 
        channel_info& ci = m_channel_info_map[streamId];
        
        if (ci.seq_no == 0)
        {
          // This is the first message received on this stream
        }
        else
        {
          uint32_t expectedSeqNum = ci.seq_no;
          if(seqNum < expectedSeqNum)
          {
            //cout << "Duplicate msg: seqNum " << seqNum << " streamId " << streamId << endl;
            return len;
          }
          else if (seqNum > expectedSeqNum)
          {
#ifdef CW_DEBUG
            cout << "Gap on streamId=" << streamId << " : expecting " << expectedSeqNum << " got " << seqNum << endl;
#endif
            if(ci.begin_recovery(this, buf, len, seqNum, nextSeqNum))
            {
              return len;
            }
          }
        }
        LOGV(cout << endl);
        ci.seq_no = nextSeqNum;
        ci.last_update = Time::currentish_time();
        LOGV(cout << "seqNum=" << seqNum << " context=" << context << " streamId=" << firstBusinessHeader.stream_id << " source_id=" << firstBusinessHeader.source_id << endl);

        if(m_recorder) {
          Logger::mutex_t::scoped_lock l(m_recorder->mutex());
          record_header h(Time::current_time(), context, len);
          m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
          m_recorder->unlocked_write(buf, len);
        }
        if(m_record_only) {
          break;
        }

        for(size_t businessBodyIndex = 0; businessBodyIndex < numBody; ++businessBodyIndex)
        {
          const tmx_QL2::business_header& businessHeader = reinterpret_cast<const tmx_QL2::business_header&>(*bufPtr);
          uint16_t businessMsgLen = businessHeader.length;
          parseInnerMessage(bufPtr); 
          bufPtr += businessMsgLen;
        }
        if(!ci.queue.empty())
        {
#ifdef CW_DEBUG
          cout << "processing queued messages" << endl;
#endif
          ci.process_recovery(this);
        }
        break;
      }
    }
    
    return len;
  }

  size_t
  TMX_QL2_Handler::parse2(size_t context, const char* buf, size_t len)
  {
#ifdef CW_DEBUG
    cout << "in parse2" << endl;
#endif
    const char* bufPtr = buf + sizeof(tmx_QL2::protocol_framing);
    const tmx_QL2::protocol_framing_header& frameHeader = reinterpret_cast<const tmx_QL2::protocol_framing_header&>(*bufPtr);
    const uint8_t numBody = frameHeader.num_body;
    bufPtr += sizeof(tmx_QL2::protocol_framing_header);

    for(size_t businessBodyIndex = 0; businessBodyIndex < numBody; ++businessBodyIndex)
    {
      const tmx_QL2::business_header& businessHeader = reinterpret_cast<const tmx_QL2::business_header&>(*bufPtr);
      uint16_t businessMsgLen = businessHeader.length;
      parseInnerMessage(bufPtr);
      bufPtr += businessMsgLen;
    }
    return len;
  }

  size_t
  TMX_QL2_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
  {
    const tmx_QL2::protocol_framing& frame = reinterpret_cast<const tmx_QL2::protocol_framing&>(*buf);
    uint8_t start = frame.frame_start;
    char proto_n = frame.protocol_name;
    char proto_v = frame.protocol_version;
    uint16_t msg_len = frame.message_length;
    
    log.printf("    frame = start: %u proto_n: %c proto_v: %c msg_len: %u\n", start, proto_n, proto_v, msg_len);

    const tmx_QL2::protocol_framing_header& frame_header = reinterpret_cast<const tmx_QL2::protocol_framing_header&>(*(buf+5));
    uint32_t session_id = frame_header.session_id;
    char ac_re_po_du = frame_header.ack_req_poss_dup;
    uint8_t num_body = frame_header.num_body;
    
    log.printf("    frame_header = session_id: %u ack_req_poss_dup: %c num_body: %u\n", session_id, ac_re_po_du, num_body);
   
    if (num_body >= 0) {
      const tmx_QL2::generic_header& generic_hdr = reinterpret_cast<const tmx_QL2::generic_header&>(*(buf+5+6));
      uint16_t generic_msg_len = generic_hdr.length;
      uint8_t  msg_type = generic_hdr.message_type;
      uint8_t  ad_id_ms_ve = generic_hdr.admin_id_msg_ver;
      
      //log.printf("generic_hdr: msg_len: %d msg_type: [%c] (hex: %x) ad_id_ms_ve: %u\n", generic_msg_len, msg_type, msg_type, ad_id_ms_ve);
      log.printf("generic_hdr: session_id: %u msg_len: %d msg=%c\n",session_id, generic_msg_len, msg_type);
      
      log.printf("%s : ", Time::current_time().to_string().c_str()); 
      switch(msg_type) {
      case '0': //Heartbeat
	{
	  //SIZE_CHECK(tmx_QL2::heartbeat_message);
	  const tmx_QL2::heartbeat_message& adm = reinterpret_cast<const tmx_QL2::heartbeat_message&>(*(buf+5+6));
	  log.printf("heartbeat_msg [%c] = hb_msecs: %u\n", adm.header.message_type, adm.hb_msecs);
	}
	break;
      case '8': //Operation
	{
	  //SIZE_CHECK(tmx_QL2::operation_message);
	  const tmx_QL2::operation_message& adm = reinterpret_cast<const tmx_QL2::operation_message&>(*(buf+5+6));
	  log.printf("operation_msg [%c] = operation_code: %x message: %.100s\n", adm.header.message_type, adm.operation_code, adm.message);
	}
	break;
      case 'A': //Assign COP - Orders
	{
	  //SIZE_CHECK(tmx_QL2::assign_cop_orders_message);
	  const tmx_QL2::assign_cop_orders_message& msg = reinterpret_cast<const tmx_QL2::assign_cop_orders_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("assign_cop_orders_msg [%c] = sym: %.9s cop: %lu cop: %12.6f order_side: %c trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.cop, (msg.cop/1000000.0),  msg.order_side, msg.trading_system_ts, trading_system_dt_buf);

          for(size_t i = 0; i < sizeof(msg.orders) / sizeof(tmx_QL2::broker_number_and_order_id); ++i)
          {
            log.printf("%s : ", Time::current_time().to_string().c_str()); 
	    log.printf("assign_cop_orders_msg [%c] = sym: %.9s broker_number_%lu: %u order_id_%lu: %lu\n", msg_type, msg.symbol, i, msg.orders[i].broker_number, i, msg.orders[i].order_id);
          }
	}
	break;
      case 'B': //Assign COP - No Orders
	{
	  //SIZE_CHECK(tmx_QL2::assign_cop_no_orders_message);
	  const tmx_QL2::assign_cop_no_orders_message& msg = reinterpret_cast<const tmx_QL2::assign_cop_no_orders_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("assign_cop_no_orders_msg [%c] = sym: %.9s cop: %lu cop: %12.6f trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.cop, (msg.cop/1000000.0), msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'C': //Assign Limit
	{
	  //SIZE_CHECK(tmx_QL2::assign_limit_message);
	  const tmx_QL2::assign_limit_message& msg = reinterpret_cast<const tmx_QL2::assign_limit_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("assign_limit_msg [%c] = sym: %.9s cop: %lu cop: %12.6f order_side: %c trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.cop, (msg.cop/1000000.0),  msg.order_side, msg.trading_system_ts, trading_system_dt_buf);
          for(size_t i = 0; i < sizeof(msg.orders) / sizeof(tmx_QL2::assign_limit_entry); ++i)
          {
            const tmx_QL2::assign_limit_entry& order = msg.orders[i];
	    log.printf("assign_limit_msg [%c] = sym: %.9s broker_number_%lu: %u order_id_%lu: %lu price_%lu: %12.6f\n", msg_type, msg.symbol, i, order.broker_number, i, order.order_id, i, (order.price/1000000.0));
          }
	}
	break;
      case 'E': //Market State Update
	{
	  //SIZE_CHECK(tmx_QL2::market_state_update_message);
	  const tmx_QL2::market_state_update_message& msg = reinterpret_cast<const tmx_QL2::market_state_update_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);
	  
	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("market_state_update_msg [%c] = market_state: [%c]  stock_group: %u trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.market_state, msg.stock_group, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'F': //MOC Imbalance
	{
	  //SIZE_CHECK(tmx_QL2::moc_imbalance_message);
	  const tmx_QL2::moc_imbalance_message& msg = reinterpret_cast<const tmx_QL2::moc_imbalance_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("moc_imbalance_msg [%c] = sym: %.9s imbalance_side: [%c] imbalance_volume: %u\n", msg_type, msg.symbol, msg.imbalance_side, msg.imbalance_volume);
	  log.printf("moc_imbalance_msg [%c] = sym: %.9s trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'G': //Order Book
	{
	  //SIZE_CHECK(tmx_QL2::order_book_message);
	  const tmx_QL2::order_book_message& msg = reinterpret_cast<const tmx_QL2::order_book_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char priority_dt_buf[32];
	  Time_Utils::print_datetime(priority_dt_buf, Time(msg.priority_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);

	  log.printf("order_book_msg [%c] = sym: %.9s broker_number: %u order_side: %c order_id: %lu price: %lu price: %12.6f volume: %u priority_ts %lu priority_timestamp: %s\n", msg_type, msg.symbol, msg.broker_number, msg.order_side, msg.order_id, msg.price, (msg.price/1000000.0), msg.volume, msg.priority_ts, priority_dt_buf);
	}
	break;
      case 'I': //Stock Status
	{
	  //SIZE_CHECK(tmx_QL2::stock_status_message);
	  const tmx_QL2::stock_status_message& msg = reinterpret_cast<const tmx_QL2::stock_status_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);
	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("stock_status_msg [%c] = sym: %.9s comment [%.40s] stock_state [%.2s] trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.comment, msg.stock_state, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'J': //Symbol Status
	{
	  //SIZE_CHECK(tmx_QL2::symbol_status_message);
	  const tmx_QL2::symbol_status_message& msg = reinterpret_cast<const tmx_QL2::symbol_status_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);
	  log.printf("symbol_status_msg [%c] = sym: %.9s stock_group: %u cusip: %.12s board_lot: %u currency: %c\n", msg_type, msg.symbol, msg.stock_group, msg.cusip, msg.board_lot, msg.currency);
	}
	break;
      case 'P': //Order Booked
	{
	  //SIZE_CHECK(tmx_QL2::order_booked_message);
	  const tmx_QL2::order_booked_message& msg = reinterpret_cast<const tmx_QL2::order_booked_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char priority_dt_buf[32];
	  Time_Utils::print_datetime(priority_dt_buf, Time(msg.priority_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("order_booked_msg [%c] = sym: %.9s broker_number: %u order_side: %c order_id: %lu price: %lu price: %12.6f volume: %u\n", msg_type, msg.symbol, msg.broker_number, msg.order_side, msg.order_id, msg.price, (msg.price/1000000.0), msg.volume);
	  log.printf("order_booked_msg [%c] = sym: %.9s priority_ts       %lu priority_timestamp:       %s\n", msg_type, msg.symbol, msg.priority_ts, priority_dt_buf);
	  log.printf("order_booked_msg [%c] = sym: %.9s trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'Q': //Order Cancelled
	{
	  //SIZE_CHECK(tmx_QL2::order_cancelled_message);
	  const tmx_QL2::order_cancelled_message& msg = reinterpret_cast<const tmx_QL2::order_cancelled_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("order_cancelled_msg [%c] = sym: %.9s broker_number: %u order_side: %c order_id: %lu\n", msg_type, msg.symbol, msg.broker_number, msg.order_side, msg.order_id);
	  log.printf("order_cancelled_msg [%c] = sym: %.9s trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'R': //Order Price-Time Assigned
	{
	  //SIZE_CHECK(tmx_QL2::order_price_time_assigned_message);
	  const tmx_QL2::order_price_time_assigned_message& msg = reinterpret_cast<const tmx_QL2::order_price_time_assigned_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char priority_dt_buf[32];
	  Time_Utils::print_datetime(priority_dt_buf, Time(msg.priority_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("order_price_time_assigned_msg [%c] = sym: %.9s broker_number: %u order_side: %c order_id: %lu price: %lu price: %12.6f volume: %u\n", msg_type, msg.symbol, msg.broker_number, msg.order_side, msg.order_id, msg.price, (msg.price/1000000.0), msg.volume);
	  log.printf("order_price_time_assigned_msg [%c] = sym: %.9s priority_ts       %lu priority_timestamp:       %s\n", msg_type, msg.symbol, msg.priority_ts, priority_dt_buf);
	  log.printf("order_price_time_assigned_msg [%c] = sym: %.9s trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'S': //Trade Report
	{
	  //SIZE_CHECK(tmx_QL2::trade_report_message);
	  const tmx_QL2::trade_report_message& msg = reinterpret_cast<const tmx_QL2::trade_report_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);		 
	  log.printf("trade_report [%c] = sym: %.9s trade_ts: %06u trading_system_ts: %lu [%s]\n", msg_type, msg.symbol, msg.trade_ts, msg.trading_system_ts, trading_system_dt_buf);
	  log.printf("trade_report [%c] = sym: %.9s trade_number: %u price: %12.6f volume: %u bypass: [%c] cross_type: [%c]\n", msg_type, msg.symbol, msg.trade_number, (msg.price/1000000.0), msg.volume, msg.bypass, msg.cross_type);
	  log.printf("trade_report [%c] = sym: %.9s buy_broker_number : %u  buy_order_id : %lu  buy_display_volume : %u\n", msg_type, msg.symbol, msg.buy_broker_number, msg.buy_order_id, msg.buy_display_volume);
	  log.printf("trade_report [%c] = sym: %.9s sell_broker_number: %u sell_order_id: %lu sell_display_volume: %u\n", msg_type, msg.symbol, msg.sell_broker_number, msg.sell_order_id, msg.sell_display_volume);
	}
	break;
      case 'T': //Trade Cancelled
	{
	  //SIZE_CHECK(tmx_QL2::trade_cancelled_message);
	  const tmx_QL2::trade_cancelled_message& msg = reinterpret_cast<const tmx_QL2::trade_cancelled_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);
	  char  trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);		 
	  log.printf("trade_cancelled [%c] = sym: %.9s trade_number: %u trading_system_ts: %lu [%s]\n", msg_type, msg.symbol, msg.trade_number, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'U': //Trade Correction
	{
	  //SIZE_CHECK(tmx_QL2::trade_correction_message);
	  const tmx_QL2::trade_correction_message& msg = reinterpret_cast<const tmx_QL2::trade_correction_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);		 
	  log.printf("trade_correction [%c] = sym: %.9s trade_ts: %06u trading_system_ts: %lu [%s]\n", msg_type, msg.symbol, msg.trade_ts, msg.trading_system_ts, trading_system_dt_buf);

	  log.printf("trade_correction [%c] = sym: %.9s trade_number: %u price: %12.6f volume: %u\n", msg_type, msg.symbol, msg.trade_number, (msg.price/1000000.0), msg.volume);
	  log.printf("trade_correction [%c] = sym: %.9s orig_trade_number: %u bypass: [%c] cross_type: [%c]\n", msg_type, msg.symbol, msg.orig_trade_number, msg.bypass, msg.cross_type);
	  log.printf("trade_correction [%c] = sym: %.9s buy_broker_number : %u sell_broker_number : %u initiated_by [%c]\n", msg_type, msg.symbol, msg.buy_broker_number, msg.sell_broker_number, msg.initiated_by);

	}
	break;
      case 'j': //Order Book - Terms
	{
	  //SIZE_CHECK(tmx_QL2::order_book_terms_message);
	  const tmx_QL2::order_book_terms_message& msg = reinterpret_cast<const tmx_QL2::order_book_terms_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char priority_dt_buf[32];
	  Time_Utils::print_datetime(priority_dt_buf, Time(msg.priority_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("order_book_terms_msg [%c] = sym: %.9s broker_number: %u order_side: %c order_id: %lu price: %lu price: %12.6f volume: %u\n", msg_type, msg.symbol, msg.broker_number, msg.order_side, msg.order_id, msg.price, (msg.price/1000000.0), msg.volume);
	  log.printf("order_book_terms_msg [%c] = sym: %.9s non_resident: [%c] settlement terms: [%c] settlement_date: %06u\n", msg_type, msg.symbol, msg.non_resident, msg.settlement_terms, msg.settlement_date);
	  log.printf("order_book_terms_msg [%c] = sym: %.9s priority_ts       %lu priority_timestamp:       %s\n", msg_type, msg.symbol, msg.priority_ts, priority_dt_buf);
	}
	break;
      case 'm': //Order Booked - Terms
	{
	  //SIZE_CHECK(tmx_QL2::order_booked_terms_message);
	  const tmx_QL2::order_booked_terms_message& msg = reinterpret_cast<const tmx_QL2::order_booked_terms_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char priority_dt_buf[32];
	  Time_Utils::print_datetime(priority_dt_buf, Time(msg.priority_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("order_booked_terms_msg [%c] = sym: %.9s broker_number: %u order_side: %c order_id: %lu price: %lu price: %12.6f volume: %u\n", msg_type, msg.symbol, msg.broker_number, msg.order_side, msg.order_id, msg.price, (msg.price/1000000.0), msg.volume);
	  log.printf("order_booked_terms_msg [%c] = sym: %.9s non_resident: [%c] settlement terms: [%c] settlement_date: %06u\n", msg_type, msg.symbol, msg.non_resident, msg.settlement_terms, msg.settlement_date);
	  log.printf("order_booked_terms_msg [%c] = sym: %.9s priority_ts       %lu priority_timestamp:       %s\n", msg_type, msg.symbol, msg.priority_ts, priority_dt_buf);
	  log.printf("order_booked_terms_msg [%c] = sym: %.9s trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'n': //Order Cancelled - Terms
	{
	  //SIZE_CHECK(tmx_QL2::order_cancelled_terms_message);
	  const tmx_QL2::order_cancelled_terms_message& msg = reinterpret_cast<const tmx_QL2::order_cancelled_terms_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("order_cancelled_terms_msg [%c] = sym: %.9s broker_number: %u order_side: %c order_id: %lu\n", msg_type, msg.symbol, msg.broker_number, msg.order_side, msg.order_id);
	  log.printf("order_cancelled_terms_msg [%c] = sym: %.9s trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'o': //Order Price-Time Assigned - Terms
	{
	  //SIZE_CHECK(tmx_QL2::order_price_time_assigned_terms_message);
	  const tmx_QL2::order_price_time_assigned_terms_message& msg = reinterpret_cast<const tmx_QL2::order_price_time_assigned_terms_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char priority_dt_buf[32];
	  Time_Utils::print_datetime(priority_dt_buf, Time(msg.priority_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);
	  log.printf("order_price_time_assigned_terms_msg [%c] = sym: %.9s broker_number: %u order_side: %c order_id: %lu price: %lu price: %12.6f volume: %u\n", msg_type, msg.symbol, msg.broker_number, msg.order_side, msg.order_id, msg.price, (msg.price/1000000.0), msg.volume);
	  log.printf("order_price_time_assigned_terms_msg [%c] = sym: %.9s priority_ts       %lu priority_timestamp:       %s\n", msg_type, msg.symbol, msg.priority_ts, priority_dt_buf);
	  log.printf("order_price_time_assigned_terms_msg [%c] = sym: %.9s trading_system_ts %lu trading_system_timestamp: %s\n", msg_type, msg.symbol, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'p': //Trade Report - Terms
	{
	  //SIZE_CHECK(tmx_QL2::trade_report_terms_message);
	  const tmx_QL2::trade_report_terms_message& msg = reinterpret_cast<const tmx_QL2::trade_report_terms_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);		 
	  log.printf("trade_report_terms_msg [%c] = sym: %.9s trade_ts: %06u trading_system_ts: %lu [%s]\n", msg_type, msg.symbol, msg.trade_ts, msg.trading_system_ts, trading_system_dt_buf);
	  log.printf("trade_report_terms_msg [%c] = sym: %.9s trade_number: %u price: %12.6f volume: %u\n", msg_type, msg.symbol, msg.trade_number, (msg.price/1000000.0), msg.volume);
	  log.printf("trade_report_terms_msg [%c] = sym: %.9s buy_broker_number : %u  buy_order_id : %lu  buy_display_volume : %u\n", msg_type, msg.symbol, msg.buy_broker_number, msg.buy_order_id, msg.buy_display_volume);
	  log.printf("trade_report_terms_msg [%c] = sym: %.9s sell_broker_number: %u sell_order_id: %lu sell_display_volume: %u\n", msg_type, msg.symbol, msg.sell_broker_number, msg.sell_order_id, msg.sell_display_volume);
	  log.printf("trade_report_terms_msg [%c] = sym: %.9s non_resident: [%c] settlement terms: [%c] settlement_date: %06u cross_type: [%c]\n", msg_type, msg.symbol, msg.non_resident, msg.settlement_terms, msg.settlement_date, msg.cross_type);
	}
	break;
      case 'q': //Trade Cancelled - Terms
	{
	  //SIZE_CHECK(tmx_QL2::trade_cancelled_terms_message);
	  const tmx_QL2::trade_cancelled_terms_message& msg = reinterpret_cast<const tmx_QL2::trade_cancelled_terms_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char  trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);		 
	  log.printf("trade_cancelled_terms [%c] = sym: %.9s trade_number: %u trading_system_ts: %lu [%s]\n", msg_type, msg.symbol, msg.trade_number, msg.trading_system_ts, trading_system_dt_buf);
	}
	break;
      case 'r': //Trade Correction - Terms
	{
	  //SIZE_CHECK(tmx_QL2::trade_correction_terms_message);
	  const tmx_QL2::trade_correction_terms_message& msg = reinterpret_cast<const tmx_QL2::trade_correction_terms_message&>(*(buf+5+6));
	  //uint32_t channel = ( (uint32_t)msg.header.source_id << 16 ) | msg.header.stream_id;
	  //log.printf("session_id: %u source_id: %c stream_id: %u sequence: %u channel: %u (%u) msg=%c", session_id, msg.header.source_id, msg.header.stream_id, msg.header.sequence_1, channel, channel, msg_type);

	  char trading_system_dt_buf[32];
	  Time_Utils::print_datetime(trading_system_dt_buf, Time(msg.trading_system_ts * Time_Constants::ticks_per_micro), Timestamp::MICRO);		 
	  log.printf("trade_correction_terms [%c] = sym: %.9s trade_number: %u price: %12.6f volume: %u\n", msg_type, msg.symbol, msg.trade_number, (msg.price/1000000.0), msg.volume);
	  log.printf("trade_correction_terms [%c] = sym: %.9s buy_broker_number : %u sell_broker_number : %u initiated_by [%c]\n", msg_type, msg.symbol, msg.buy_broker_number, msg.sell_broker_number, msg.initiated_by);
	  log.printf("trade_correction_terms [%c] = sym: %.9s orig_trade_number: %u trade_ts: %06u \n", msg_type, msg.symbol, msg.orig_trade_number, msg.trade_ts);
	  log.printf("trade_correction_terms [%c] = sym: %.9s non_resident: [%c] settlement terms: [%c] settlement_date: %06u cross_type: [%c]\n", msg_type, msg.symbol, msg.non_resident, msg.settlement_terms, msg.settlement_date, msg.cross_type);
	  log.printf("trade_correction_terms [%c] = sym: %.9s trading_system_ts: %lu [%s]\n", msg_type, msg.symbol, msg.trading_system_ts, trading_system_dt_buf);

	}
	break;
      default:
	log.printf("generic_hdr = msg_len: %d msg_type: [%c] (hex: %x) ad_id_ms_ve: %u\n", generic_msg_len, msg_type, msg_type, ad_id_ms_ve);
	//case 'a':order_add_message_long
	break;
      }
    }
    return len;
  }

  void
  TMX_QL2_Handler::reset(const char* msg)
  {
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_qbdf_exch_char, -1);
      }
#ifdef CW_DEBUG
      cout << "clearing order book" << endl;
#endif
      m_order_book->clear(true);
    }
    if(m_subscribers) {
      m_subscribers->post_invalid_quote(m_quote.m_exch);
    }
  }

  void
  TMX_QL2_Handler::reset(size_t context, const char* msg)
  {
    //const char* where = "TMX_QL2_Handler::reset";

#ifdef CW_DEBUG
    cout << "reset2. context=" << context << endl;
#endif
#ifdef CW_DEBUG
    cout << "msg=" << msg << endl;
#endif

    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }

    if(m_subscribers) {
      // nothing for now
    }

    if(m_order_book) {
      if(m_qbdf_builder) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, m_qbdf_exch_char, context);
      }
#ifdef CW_DEBUG
      cout << "clearing order book 2" << endl;
#endif
      m_order_book->clear(true);
    }
  }

  void
  TMX_QL2_Handler::send_quote_update()
  {
    if(!m_qbdf_builder)
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

    // ToDo: only do this is the symbol hasn't had an open trade and if it's before ~9:40 or so
    MDPriceLevel* topBidLevel = m_order_book->book(m_quote.m_id).bid().top();
    MDPriceLevel* topAskLevel = m_order_book->book(m_quote.m_id).ask().top();

    Time nowTime = Time::current_time();
    m_quote.set_time(nowTime);
    string symbol = product_id_to_symbol[m_quote.m_id];
    //if(symbol.size() == 2 && symbol[0] == 'B' && symbol[1] == 'B')
    //{
    //  cout << m_quote.time().to_string();
    if(topBidLevel && topAskLevel)
    {
      double bestBid = topBidLevel->price_as_double();
      double bestAsk = topAskLevel->price_as_double();
      uint32_t totalBuySize = topBidLevel->total_shares;
      uint32_t totalSellSize = topAskLevel->total_shares;
      //if(bestBid > bestAsk)
      //{
      //  cout << Time::current_time().to_string() << "|" << symbol << "|" << bestBid << "|" << bestAsk << "|" << totalBuySize << "|" << totalSellSize << endl;
      //}
      //cout << "|bestBid=" << bestBid << "|bestAsk=" << bestAsk;
      //cout << "|totalBuySize=" << totalBuySize << "|totalSellSize=" << totalSellSize;
      Time exchangeTime = m_quote.time();
      if(bestBid > 0 && bestBid == bestAsk)
      {
        MDPrice auctionPrice = MDPrice::from_fprice(bestBid);
        //cout << "|publishImbalance";
        publishImbalance(m_quote.m_id, exchangeTime, auctionPrice, totalBuySize, totalSellSize);
        product_id_to_imbalance_state[m_quote.m_id] = true; 
      }
      else if(bestBid < bestAsk)
      {
        hash_map<ProductID, bool>::iterator it = product_id_to_imbalance_state.find(m_quote.m_id);
        if(it != product_id_to_imbalance_state.end() && it->second == true)
        {
          // No imbalance
          MDPrice emptyPrice;
          publishImbalance(m_quote.m_id, exchangeTime, emptyPrice, 0, 0);
          product_id_to_imbalance_state[m_quote.m_id] = false;
        }
      }
      //else { cout << "|NotLocked"; }
    }
    //else 
    //{
      //if(!(topBidLevel))
      //{
      //  cout << "|No bid";
      //}
      //if(!(topAskLevel))
      //{
      //  cout << "|No ask";
      //}
    //}
    //cout << endl;
    //}
  }

/*
  void TMX_QL2_Handler::print_imbalance()
  {
    MDPriceLevel* topBidLevel = m_order_book->book(m_quote.m_id).bid().top();
    MDPriceLevel* topAskLevel = m_order_book->book(m_quote.m_id).ask().top();
    double bestBid = topBidLevel->price_as_double();
    double bestAsk = topAskLevel->price_as_double();
 
    // Might have to null check?
    if(topBidLevel && topAskLevel)
    {
      if(bestBid > 0 && bestBid == bestASk)
      {
        string symbol = product_id_to_symbol[m_quote.m_id];
        uint32_t bidSize = topBidLevel->total_shares;
        uint32_t askSize = topAskLevel->total_shares;
#ifdef CW_DEBUG
        cout << "method2,"
             << Time::current_time().to_string() << ","
             << bidSize << "," << askSize << endl;
#endif
      }
    }
  }
*/

  void
  TMX_QL2_Handler::send_trade_update()
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
  TMX_QL2_Handler::init(const string& name, ExchangeID exch, const Parameter& params)
  {
    Handler::init(name, params);

    for(size_t streamId = 0; streamId < 1024; ++streamId) // 1024 is just a guess at the max # stream ID
    {
      m_channel_info_map.push_back(channel_info("", streamId));
    }

    // ToDo: load symbol status mappings (two maps) from persistence
    // stock_group_to_product_ids
    // product_id_to_stock_group

    m_name = name;

    m_midnight = Time::today();

    if (exch == ExchangeID::XTSE) {
      m_qbdf_exch_char = 't';
    } else if (exch == ExchangeID::CHIJ) {
      m_qbdf_exch_char = 'j';
    } else if (exch == ExchangeID::CHIX) {
      m_qbdf_exch_char = 'Z';
    } else {
      m_logger->log_printf(Log::ERROR, "TMX_QL2_Handler::init: Unrecognized ExchangeID");
      m_qbdf_exch_char = ' ';
    }

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

    m_ignore_odd_lots = params.getDefaultBool("ignore_odd_lots", true);

    // ToDo: if m_qbdf_builder, write this to the qbdf output dir?
    //m_stock_group_logger = m_app->lm()->get_logger("STOCK_GROUP", "csv"); 
    //m_stock_group_logger->printf("timestamp_seconds,symbol,stock_group,board_lot_size\n");
  }

  void
  TMX_QL2_Handler::start()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  TMX_QL2_Handler::stop()
  {
    if(m_order_book) {
      m_order_book->clear(false);
    }
  }

  void
  TMX_QL2_Handler::subscribe_product(ProductID id_,ExchangeID exch_,
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

  TMX_QL2_Handler::TMX_QL2_Handler(Application* app) :
    Handler(app, "TMX_QL2"),
    m_hist_mode(false),
    m_next_date_id(0),
    m_number_of_terms_orders_seen(0)
  {
  }

  TMX_QL2_Handler::~TMX_QL2_Handler()
  {
  }


}
