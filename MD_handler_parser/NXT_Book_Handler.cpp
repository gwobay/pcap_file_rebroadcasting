#include <Data/QBDF_Util.h>
#include <Data/NXT_Book_Handler.h>
#include <Util/Network_Utils.h>
#include <Util/CSV_Parser.h>

#include <Exchange/Exchange_Manager.h>
#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <MD/MDProvider.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <boost/foreach.hpp>

#include <stdarg.h>

namespace hf_core {

  static FactoryRegistrar1<std::string, Handler, Application*, NXT_Book_Handler> r1("NXT_Book_Handler");
  
  NXT_Book_Shared_TCP_Handler* NXT_Book_Handler::m_tcp_handler = 0;


  static bool
  get_exch_time_delta(uint64_t exch_time, uint64_t recv_time,
                      int& exch_time_delta, bool& delta_in_millis)
  {
    int64_t temp_delta =  exch_time - recv_time;
    
    if (labs(temp_delta) < numeric_limits<int>::max()) {
      exch_time_delta = (int)temp_delta;
      delta_in_millis = false;
    } else {
      exch_time_delta = (int)(temp_delta / 1000L);
      delta_in_millis = true;
    }
    
    return true;
  }
  
  
  // Common check to insure that sizeof(message_type) is the same as what NASDAQ sent, send alert if sizes do not match
#define SIZE_CHECK(type)                                                \
  if(len != sizeof(type)) {                                             \
    send_alert("%s %s called with " #type " len=%d, expecting %d", where, m_name.c_str(), len, sizeof(type)); \
    continue;                                                           \
  }

  size_t
  NXT_Book_Handler::parse(size_t context, const char* buf, size_t len)
  {
    const char* where = "NXT_Book_Handler::parse";
    
    mutex_t::scoped_lock lock(m_mutex);
    
    if (m_qbdf_builder) {
      m_micros_recv_time = Time::current_time().total_usec() - Time::today().total_usec();
      m_micros_recv_qbdf_time = m_qbdf_builder->micros_to_qbdf_time(m_micros_recv_time);
      
      if (m_micros_recv_time / Time_Constants::micros_per_min > m_last_min_reported) {
        m_last_min_reported = m_micros_recv_time / Time_Constants::micros_per_min;
        m_logger->log_printf(Log::INFO, "%s: Got time %lu, equivalent to %lu", where,
                             m_micros_recv_time, m_micros_recv_qbdf_time);
        m_logger->sync_flush();
      }
    }
    
    const nxt_book::multicast_packet_header_common* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_common*>(buf);

    if(m_not_logged_in && m_dispatcher) {
      if(packet_header->protocol_version >= 5) {
        const nxt_book::multicast_packet_header_v06* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_v06*>(buf);
        if(packet_header->packet_type == 'H') {
          if(m_recorder) {
            Logger::mutex_t::scoped_lock l(m_recorder->mutex());
            record_header h(Time::current_time(), context, len);
            m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
            m_recorder->unlocked_write(buf, len);
          }
          const size_t mph_len = sizeof(nxt_book::multicast_packet_header_v06);
          handle_heartbeat(context, buf + mph_len, len - mph_len, packet_header->sender_id);
        }
      } else {
        const nxt_book::multicast_packet_header_v01* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_v01*>(buf);
        if(packet_header->fragment_indicator == 0) {
          if(m_recorder) {
            Logger::mutex_t::scoped_lock l(m_recorder->mutex());
            record_header h(Time::current_time(), context, len);
            m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
            m_recorder->unlocked_write(buf, len);
          }
          
          look_for_heartbeat(context, buf, len);
        }
        return len;
      }
    }
    
    if(m_qbdf_builder && (context == 1 || context == 2)) {
      handle_message(context, buf);
      return len;
    }
    
    if(m_dispatcher && packet_header->sender_id != m_chosen_sender_id) {
      if(packet_header->protocol_version >= 5) {
        const nxt_book::multicast_packet_header_v06* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_v06*>(buf);
        if(packet_header->packet_type == 'H') {
          ++m_other_heartbeat;
          if(m_other_heartbeat > 5) {
            // Drop current publisher and choose another
            send_alert("NXT_Book_Handler::parse  %s  missed %d heartbeats from sender id %d, reselecting publisher",
                       m_name.c_str(), m_other_heartbeat, m_chosen_sender_id);
            reselect_publisher();
            
            m_other_heartbeat = 0;
          }
        }
      }
      return len;
    }
    
    m_other_heartbeat = 0;
    
    if(unlikely(m_drop_packets)) {
      --m_drop_packets;
      return 0;
    }
    
    channel_info& ci = m_channel_info_map[0];
    
    uint32_t seq_no = packet_header->sequence_number;
    uint32_t next_seq_no = seq_no + 1;
    
    if(seq_no == (uint32_t)-1) {
      parse2(context, buf, len);
      return len;
    }
    
    if(seq_no != ci.seq_no) {
      if(seq_no < ci.seq_no) {
        // duplicate
        if(!m_qbdf_builder) {
          return len;
        } else {
          m_logger->log_printf(Log::INFO, "%s: Got seq_no %u, expected %lu, resetting", where, seq_no, ci.seq_no);
        }
      } else {
        if(ci.begin_recovery(this, buf, len, seq_no, next_seq_no)) {
          return len;
        }
      }
    }
    
    parse2(context, buf, len);
    ci.seq_no = next_seq_no;
    ci.last_update = Time::currentish_time();
    
    if(!ci.queue.empty()) {
      ci.process_recovery(this);
    }
    
    return len;
  }
  
  void
  NXT_Book_Handler::look_for_heartbeat(size_t context, const char* packet_buf, size_t packet_len)
  {
    const nxt_book::multicast_packet_header_v01* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_v01*>(packet_buf);
    
    const char* buf = packet_buf + sizeof(nxt_book::multicast_packet_header_v01);
    ssize_t len = packet_len - sizeof(nxt_book::multicast_packet_header_v01);
    
    for(int pkt = 0; pkt < packet_header->message_count; ++pkt) {
      const nxt_book::header_only* message_header = reinterpret_cast<const nxt_book::header_only*>(buf);
      if(message_header->message_size < len) {
        send_alert("NXT_Book_Handler::look_for_heartbeat  %s  buffer len %u  too short for message %ld  message_type:%c",
                   m_name.c_str(), message_header->message_size, len, message_header->message_type);
        return;
      }
      
      if(message_header->message_type == 'h') {
        handle_heartbeat(context, buf + 2, message_header->message_size - 2, packet_header->sender_id);
      }
      
      buf += message_header->message_size;
      len -= message_header->message_size;
    }
  }
  
  void
  NXT_Book_Handler::handle_heartbeat(size_t context, const char* buf, size_t buf_len, uint8_t sender_id)
  {
    const nxt_book::heartbeat* heartbeat = reinterpret_cast<const nxt_book::heartbeat*>(buf);
    
    bool sender_found = false;
    for(vector<nxt_book::publisher_info>::iterator i = m_nxt_publishers.begin(), i_end = m_nxt_publishers.end(); i != i_end; ++i) {
      if(i->sender_id == sender_id) {
        sender_found = true;
        break;
      }
    }
    
    if(sender_found) {
      // Once we've seen the second message from a sender, it's time to choose a sender and connect
      sort(m_nxt_publishers.begin(), m_nxt_publishers.end());
      for(unsigned int i = 0; i < m_nxt_publishers.size(); ++i) {
        m_chosen_sender_id = m_nxt_publishers[i].sender_id;
        m_logger->log_printf(Log::INFO, "NXT_Book_Handler::handler_heartbeat %s:  subscribing to sender id: %d", 
                             m_name.c_str(), m_chosen_sender_id);
        
        int context = m_tcp_handler->establish_connection(m_nxt_publishers[i]);
        if(context != -1) {
          m_not_logged_in = false;
          for(map<string, subscription_t>::iterator sub = m_subscriptions.begin(), sub_end = m_subscriptions.end(); sub != sub_end; ++sub) {
            if(!m_tcp_handler->request_symbol_id(this, context, sub->first)) {
              break;
            }
          }
          return;
        }
      }
    } else {
      nxt_book::publisher_info pub;
      pub.sender_id = sender_id;
      pub.priority = heartbeat->priority;
      pub.major_data_version = heartbeat->major_data_version;
      pub.minor_data_version = heartbeat->minor_data_version;
      pub.tcp_service_ip = heartbeat->tcp_service_ip;
      pub.tcp_service_port = heartbeat->tcp_service_port;
      m_nxt_publishers.push_back(pub);
    }
    
  }
  
  
  void
  NXT_Book_Shared_TCP_Handler::init(const string& name, const Parameter& p)
  {
    Handler::init(name, p);
    m_name = name;
    m_client_id = p["client_id"].getString();
    m_password  = p["password"].getString();

  }
  
  bool
  NXT_Book_Shared_TCP_Handler::send_msg(NXT_Book_Handler* h, NXT_Book_Shared_TCP_Handler::publisher_t& pub, const void* msg, size_t len)
  {
    Logger* recorder = h->recorder().get();
    if(recorder) {
      size_t context = 2;
      Logger::mutex_t::scoped_lock l(recorder->mutex());
      record_header h(Time::current_time(), context, len);
      recorder->unlocked_write((const char*)&h, sizeof(record_header));
      recorder->unlocked_write((const char*)msg, len);
    }
    
    if(pub.writes_pending) {
      pub.pending_writes.push_back(pending_write((const char*)msg, len, 0));
      return true;
    }
    
    ssize_t ret = ::write(pub.tcp_fd, msg, len);
    if((ret >= 0 && ret < (ssize_t)len) || (ret == -1 && errno == EAGAIN)) {
      int already_written = ret;
      if(already_written < 0) {
        already_written = 0;
      }
      pub.pending_writes.push_back(pending_write((const char*)msg, len, already_written));
      pub.writes_pending = true;
      return true;
    }
    
    if(ret < 0) {
      string err = errno_to_string();
      send_alert("NXT_Book_Shared_TCP_Handler::send_msg return errno %d - %s", errno, err.c_str());
      return false;
    }
    
    return true;
  }
  
  int
  NXT_Book_Shared_TCP_Handler::establish_connection(nxt_book::publisher_info& publisher_)
  {
    mutex_t::scoped_lock lock(m_mutex);
   
    size_t context = m_publishers.size();
    publisher_t* pub = 0;
    
    for(deque<publisher_t>::iterator i = m_publishers.begin(); i != m_publishers.end(); ++i) {
      if(i->tcp_service_ip == publisher_.tcp_service_ip && i->tcp_service_port == publisher_.tcp_service_port) {
        context = i - m_publishers.begin();
        pub = &*i;
        if(i->connection_state == 2) {
          return context;
        }
      }
    }
    
    if(!pub) {
      context = m_publishers.size();
      m_publishers.push_back(publisher_);
      pub = &m_publishers.back();
    }
    
    char host[32];
    char port[16];
    
    union { uint8_t quad[4]; uint32_t i; }  host_i;
    host_i.i = pub->tcp_service_ip;
    
    snprintf(host, 32, "%d.%d.%d.%d", host_i.quad[0], host_i.quad[1], host_i.quad[2], host_i.quad[3]);
    snprintf(port, 16, "%d", pub->tcp_service_port);
    
    m_logger->log_printf(Log::INFO, "NXT_Book_Handler::establish_connection: %s  connecting to %s:%s", m_name.c_str(), host, port);
    
    bool ret = pub->tcp_fd.sync_connect_tcp(host, port);
    
    m_logger->log_printf(Log::INFO, "NXT_Book_Handler::establish_connection: %s  connection status = %s", m_name.c_str(), ret ? "connected": "error");
    
    if(ret) {
      pub->tcp_fd.set_nonblocking();
      
      m_dispatcher->add_tcp_fd(this, "sub", context, pub->tcp_fd, 256 * 1024);
      m_dispatcher->start_sync(this, "sub");
      
      struct sockaddr_in adr_inet;
      socklen_t  len_inet = sizeof(adr_inet);
      
      if(getsockname(pub->tcp_fd, (struct sockaddr*)&adr_inet, &len_inet)) {
        m_logger->log_printf(Log::ERROR, "NXT_Book_Handler::establish_connection: %s  error calling getsockname on fd", m_name.c_str());
        m_client_ip = 0;
      }
      m_client_ip = adr_inet.sin_addr.s_addr;
      
      /* Client Hello is no longer part of the spec as of version 0.6
      nxt_book::client_hello hello;
      memset(&hello, 0, sizeof(nxt_book::client_hello));
      hello.message_type = 'c';
      hello.message_size = sizeof(nxt_book::client_hello);
      memcpy(hello.client_id, m_client_id.c_str(), std::min(m_client_id.size(), (size_t)15));
      ::write(pub->tcp_fd, &hello, sizeof(nxt_book::client_hello));
      */
      
      pub->connection_state = 2;
    } else {
      pub->connection_state = 3;
      return -1;
    }
    
    return context;
  }
  
  bool
  NXT_Book_Shared_TCP_Handler::request_symbol_id(NXT_Book_Handler* h, int context, const string& symbol)
  {
    mutex_t::scoped_lock lock(m_mutex);
    
    publisher_t& pub = m_publishers[context];
    
    pub.symbol_to_handler[symbol] = h;
    
    nxt_book::get_symbol_id msg;
    memset(&msg, 0, sizeof(msg));
    msg.message_type = 'a';
    msg.message_size = sizeof(msg);
    strncpy(msg.symbol, symbol.c_str(), 31);
    return send_msg(h, pub, &msg, sizeof(msg));
  }
  
  
  void
  NXT_Book_Shared_TCP_Handler::subscribe(NXT_Book_Handler* h, publisher_t& pub, uint32_t symbol_id)
  {
    nxt_book::subscription  sub;
    memset(&sub, 0, sizeof(nxt_book::subscription));
    sub.message_type = 's';
    sub.message_size = sizeof(nxt_book::subscription);
    sub.symbol_id = symbol_id;
    sub.md_type = 0;
    memcpy(sub.client_id, m_client_id.c_str(), std::min(m_client_id.size(), (size_t)15));
    sub.client_ip = m_client_ip;
    memcpy(sub.password, m_password.c_str(), std::min(m_password.size(), (size_t)11));
    send_msg(h, pub, &sub, sizeof(sub));
    sub.md_type = 1;
    send_msg(h, pub, &sub, sizeof(sub));
    sub.md_type = 2;
    send_msg(h, pub, &sub, sizeof(sub));
    sub.md_type = 3;
    send_msg(h, pub, &sub, sizeof(sub));
  }
    
  size_t
  NXT_Book_Shared_TCP_Handler::read(size_t context, const char* buf, size_t len)
  {
    mutex_t::scoped_lock lock(m_mutex);
    
    publisher_t& pub = m_publishers[context];
    
    //m_logger->log_printf(Log::INFO, "NXT_Book_Shared_TCP_Handler::read  len=%zd", len);
    
    size_t total_read = 0;
    while(len >= sizeof(nxt_book::header_only)) {
      const nxt_book::header_only* message_header = reinterpret_cast<const nxt_book::header_only*>(buf);
      if(message_header->message_size > len ) {
        break;
      }
      
      switch(message_header->message_type) {
      case 'b':  // Symbol ID
        {
          const nxt_book::symbol_id* msg = reinterpret_cast<const nxt_book::symbol_id*>(buf);
          
          string symbol(msg->symbol, strnlen(msg->symbol, 31));
          hash_map<string, NXT_Book_Handler*>::iterator i = pub.symbol_to_handler.find(symbol);
          if(i != pub.symbol_to_handler.end()) {
            NXT_Book_Handler* h = i->second;
            Logger* recorder = h->recorder().get();
            if(recorder) {
              Logger::mutex_t::scoped_lock l(recorder->mutex());
              record_header h(Time::current_time(), 1, message_header->message_size);
              recorder->unlocked_write((const char*)&h, sizeof(record_header));
              recorder->unlocked_write(buf, message_header->message_size);
            }
            pub.symbol_id_to_handler[msg->symbol_id] = h;
            subscribe(h, pub, msg->symbol_id);
            h->handle_message(1, buf);
          } else {
            send_alert("NXT_Book_Handler::handle_message %s symbol_id called with unknown symbol %s", m_name.c_str(), symbol.c_str());
          }
        }
        break;
      case 'e': // Notification
        {
          const nxt_book::notification* msg = reinterpret_cast<const nxt_book::notification*>(buf);
          switch(msg->notification) {
          case 1:
            send_alert("NXT_Book_Shared_TCP_Handler notification -- Soft limit on number of outstanding requests");
            break;
          case 2:
            send_alert("NXT_Book_Shared_TCP_Handler notification -- Hard limit on number of outstanding requests");
            break;
          default:
            send_alert("NXT_Book_Shared_TCP_Handler notification -- Unknown notification %u", msg->notification);
            break;
          }
        }
        break;
      default:
        {
          const nxt_book::header_w_symbol_id_only* msg = reinterpret_cast<const nxt_book::header_w_symbol_id_only*>(buf);
          uint32_t symbol_id = msg->symbol_id;
          hash_map<uint32_t, NXT_Book_Handler*>::iterator i = pub.symbol_id_to_handler.find(symbol_id);
          if(i != pub.symbol_id_to_handler.end()) {
            NXT_Book_Handler* h = i->second;
            Logger* recorder = h->recorder().get();
            if(recorder) {
              Logger::mutex_t::scoped_lock l(recorder->mutex());
              if(message_header->message_size <= std::numeric_limits<uint16_t>::max()) {
                record_header h(Time::current_time(), 1, message_header->message_size);
                recorder->unlocked_write((const char*)&h, sizeof(record_header));
                recorder->unlocked_write(buf, message_header->message_size);
              } else {
                long_record_header h(Time::current_time(), 1, message_header->message_size);
                recorder->unlocked_write((const char*)&h, sizeof(long_record_header));
                recorder->unlocked_write(buf, message_header->message_size);
              }
            }
            h->handle_message(1, buf);
          } else {
            send_alert("NXT_Book_Handler::handle_message %s called with unknown symbol_id  %d", m_name.c_str(), symbol_id);
          }
        }
      }
      len -= message_header->message_size;
      buf += message_header->message_size;
      total_read += message_header->message_size;
    }
    
    if(pub.writes_pending) {
      //m_logger->log_printf(Log::INFO, "NXT_Book_Shared_TCP_Handler::read start  writes_pending size = %zd", pub.pending_writes.size());
      
      while(!pub.pending_writes.empty()) {
        pending_write& pw = pub.pending_writes.front();
        size_t len = pw.data.size() - pw.already_written;
        ssize_t ret = ::write(pub.tcp_fd, pw.data.c_str() + pw.already_written, len);
        if((ret >= 0 && ret < (ssize_t)len) || (ret == -1 && errno == EAGAIN)) {
          int already_written = ret;
          if(already_written < 0) {
            already_written = 0;
          }
          pw.already_written += already_written;
          break;
        } else if(ret == -1) {
          string err = errno_to_string();
          send_alert("NXT_Book_Shared_TCP_Handler::send_msg return errno %d - %s", errno, err.c_str());
          pub.pending_writes.clear();
          pub.writes_pending = false;
        } else {
          pub.pending_writes.pop_front();
        }
      }
      
      //m_logger->log_printf(Log::INFO, "NXT_Book_Shared_TCP_Handler::read finish  writes_pending size = %zd", pub.pending_writes.size());

      pub.writes_pending = !pub.pending_writes.empty();
    }
    return total_read;
  }
  
  
  void
  NXT_Book_Handler::subscribe_product(ProductID id, ExchangeID exch, const string& mdsymbol, const string& mdexch)
  {
    mutex_t::scoped_lock lock(m_mutex);
    
    subscription_t sub;
    sub.id = id;
    sub.exch = exch;
    sub.nxt_symbol_id = -1;
    if (m_qbdf_builder) {
      sub.exch_char = m_qbdf_builder->get_qbdf_exch_by_hf_exch_id(exch);
    }
    
    int idx = 0;
    for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator i = m_order_books.begin(), i_end = m_order_books.end(); i != i_end; ++i, ++idx) {
      if((*i)->exch() == exch)  break;
    }
    if(idx == (int)m_order_books.size())
      idx = -1;
    sub.book_idx = idx;
    
    m_subscriptions.insert(make_pair(mdsymbol, sub));
  }
  
  size_t
  NXT_Book_Handler::parse2(size_t context, const char* packet_buf, size_t packet_len)
  {
    if(m_recorder) {
      Logger::mutex_t::scoped_lock l(m_recorder->mutex());
      if(packet_len <= std::numeric_limits<uint16_t>::max()) {
        record_header h(Time::current_time(), context, packet_len);
        m_recorder->unlocked_write((const char*)&h, sizeof(record_header));
        m_recorder->unlocked_write(packet_buf, packet_len);
      } else {
        long_record_header h(Time::current_time(), context, packet_len);
        m_recorder->unlocked_write((const char*)&h, sizeof(long_record_header));
        m_recorder->unlocked_write(packet_buf, packet_len);
      }
    }

    if(m_record_only) {
      return packet_len;
    }
    
    uint8_t fragment_indicator = 0;
    int     message_count = 0;
    size_t  header_size = 0;
    
    const nxt_book::multicast_packet_header_common* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_common*>(packet_buf);
    
    if(packet_header->protocol_version >= 5) {
      const nxt_book::multicast_packet_header_v06* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_v06*>(packet_buf);
      if(packet_header->packet_type == 'D') {
        const nxt_book::data_packet* dp = reinterpret_cast<const nxt_book::data_packet*>(packet_buf + sizeof(nxt_book::multicast_packet_header_v06));
        fragment_indicator = dp->fragment_indicator;
        message_count = dp->message_count;
        header_size = sizeof(nxt_book::multicast_packet_header_v06) + sizeof(nxt_book::data_packet);
      } else if(packet_header->packet_type == 'H') {
        return packet_len;
      } else {
        return packet_len;
      }
    } else {
      const nxt_book::multicast_packet_header_v01* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_v01*>(packet_buf);
      fragment_indicator = packet_header->fragment_indicator;
      message_count = packet_header->message_count;
      header_size = sizeof(nxt_book::multicast_packet_header_v01);
    }
    
    switch(fragment_indicator) {
    case 0: break;
    case 1: 
      m_fragment_buffer.clear();
      m_fragment_buffer.insert(m_fragment_buffer.end(), packet_buf, packet_buf + packet_len);
      return packet_len;
    case 2:
      if(m_fragment_buffer.empty()) {
        send_alert("NXT_Book_Handler::parse2 recieved fragment_indicicator 2 when fragment_buffer empty -- skipping");
        return packet_len;
      }
      m_fragment_buffer.insert(m_fragment_buffer.end(), packet_buf + header_size, packet_buf + packet_len);
      return packet_len;
    case 3:
      if(m_fragment_buffer.empty()) {
        send_alert("NXT_Book_Handler::parse2 recieved fragment_indicicator 3 when fragment_buffer empty -- skipping");
        return packet_len;
      }
      m_fragment_buffer.insert(m_fragment_buffer.end(), packet_buf + header_size, packet_buf + packet_len);
      
      packet_buf = &m_fragment_buffer[0];
      packet_len += m_fragment_buffer.size();
    }
    
    const char* buf = packet_buf + header_size;
    ssize_t len = packet_len - header_size;
    
    for(int pkt = 0; pkt < message_count; ++pkt) {
      const nxt_book::header_only* message_header = reinterpret_cast<const nxt_book::header_only*>(buf);
      if(message_header->message_size > len) {
        send_alert("NXT_Book_Handler::parse2  %s  buffer len %u too short for message len %ld   message_type:%c",
                   m_name.c_str(), message_header->message_size, len, message_header->message_type);
        return packet_len;
      }
      
      handle_message(context, buf);
      buf += message_header->message_size;
      len -= message_header->message_size;
    }
    
    return packet_len;
  }
  
  void
  NXT_Book_Handler::handle_message(size_t context, const char* buf)
  {
    const nxt_book::header_w_symbol_seq* message_header = reinterpret_cast<const nxt_book::header_w_symbol_seq*>(buf);
    
    if(message_header->message_type == 'h')
      return;
    else if(message_header->message_type == 'b') {
      const nxt_book::symbol_id* msg = reinterpret_cast<const nxt_book::symbol_id*>(buf);
      
      string symbol(msg->symbol, strnlen(msg->symbol, 31));
      if(!m_qbdf_builder) {
        map<string, subscription_t>::iterator i = m_subscriptions.find(symbol);
        if(i != m_subscriptions.end()) {
          subscription_t& sub = i->second;
          m_nxt_subscriptions.insert(make_pair(msg->symbol_id, sub));
          if((int)m_waiting_snapshot.size() <= sub.id) {
            m_waiting_snapshot.resize(sub.id*2, false);
          }
          m_waiting_snapshot[sub.id] = true;
        } else {
          send_alert("NXT_Book_Handler::handle_message %s symbol_id called with unknown symbol %s", m_name.c_str(), symbol.c_str());
        }
      } else {
        ProductID id = m_qbdf_builder->process_taq_symbol(symbol);
        if((int)m_waiting_snapshot.size() <= id) {
          m_waiting_snapshot.resize(id*2, false);
        }
        m_waiting_snapshot[id] = true;
        
        size_t pos = symbol.rfind('.');
        
        char exch_char = ' ';
        
        if(pos != string::npos) {
          string mic = symbol.substr(pos+1);
          exch_char = m_qbdf_builder->get_qbdf_exch_by_hf_exch_name(mic);
        } else {
          m_logger->log_printf(Log::ERROR, "%s: Could not find exchange MIC separator in symbol %s",
                               "NXT_Book_Handler::handle_message", symbol.c_str());
        }
        
        subscription_t& sub = m_nxt_subscriptions[msg->symbol_id];
        
        sub.id = id;
        sub.exch = m_qbdf_builder->get_hf_exch_id_by_qbdf_exch(exch_char);
        sub.nxt_symbol_id = msg->symbol_id;
        sub.exch_char = exch_char;
        
        int idx = 0;
        for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator i = m_order_books.begin(), i_end = m_order_books.end(); i != i_end; ++i, ++idx) {
          if((*i)->exch() == sub.exch)  break;
        }
        if(idx == (int)m_order_books.size())
          idx = -1;
        sub.book_idx = idx;
        
        if(m_qbdf_exchange_chars.end() == find(m_qbdf_exchange_chars.begin(), m_qbdf_exchange_chars.end(), exch_char)) {
          m_qbdf_exchange_chars.push_back(exch_char);
        }
      }
      return;
    } else if(message_header->message_type == 's') {
      return;
    } else if(message_header->message_type == 'a') {
      return;
    }
    

    hash_map<uint32_t, subscription_t>::const_iterator sub_itr = m_nxt_subscriptions.find(message_header->symbol_id);
    if(sub_itr == m_nxt_subscriptions.end()) {
      return;
    }
    const subscription_t& sub_info = sub_itr->second;
    ProductID id = sub_info.id;
    char exch_char = sub_info.exch_char;
    ExchangeID exch = sub_info.exch;
    
    MDOrderBookHandler* order_book = (sub_info.book_idx != -1) ? m_order_books[sub_info.book_idx].get() : 0;
    
    if(m_waiting_snapshot.test(id) &&
       (message_header->message_type != 'A' && message_header->message_type != 'B')) {
      m_symbol_seq[id].queue_push(message_header->symbol_seq_number, message_header->symbol_seq_number + 1, buf, message_header->message_size);
      return;
    }
    
    switch(message_header->message_type) {
    case 'A': // Price Book Initial Snapshot
    case 'C': // Price Book Snapshot
    case 'F': // Price book Update
      {
        const char* price_buf = buf;
        const nxt_book::price_book* pb = reinterpret_cast<const nxt_book::price_book*>(price_buf);
        price_buf += sizeof(nxt_book::price_book);
        
        if(order_book) {
          if(message_header->message_type != 'F') {
            order_book->clear(id, true);
            if(m_qbdf_builder) {
              gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, id, exch_char, context);
            }
          }

          bool inside_changed = false;
          
          for(uint16_t i = 0; i < pb->pl_count; ++i) {
            const nxt_book::price_book_bid_ask* ba = reinterpret_cast<const nxt_book::price_book_bid_ask*>(price_buf);
            price_buf += sizeof(nxt_book::price_book_bid_ask);
            
            char side = ba->side ? 'B' : 'S';
            MDPrice iprice = MDPrice::from_fprice(ba->price);
            Time ts = m_midnight + convert_timestamp(ba->timestamp);
            
            if(side == 'S' && iprice.invalid()) {
              iprice.clear();
            }
            
	    //cout << "check" << endl;
	    //cout << ba->timestamp << endl;
	    //cout << convert_micros(ba->timestamp) << endl;
	    //cout << m_tz_offset << endl;
	    //cout << convert_micros(ba->timestamp) + m_tz_offset << endl;
	    
            uint64_t exch_time_micros = convert_micros(ba->timestamp) - m_tz_offset;
	    //cout <<  convert_micros(ba->timestamp) << "," << m_tz_offset << "," << exch_time_micros << endl;
            
            if(ba->attached_order_count == 0) {
              if(ba->size > 0) {
                inside_changed |= order_book->modify_level_ip(ts, id, side, iprice, ba->size, ba->pl_order_count, false);
              } else {
                inside_changed |= order_book->remove_level_ip(ts, id, side, iprice, false);
              }
              
              if(m_qbdf_builder) {
		int32_t record_time_delta = 0;
                bool delta_in_millis = false;
                get_exch_time_delta(exch_time_micros, m_micros_recv_time, record_time_delta, delta_in_millis);
                
                bool last_in_group = i == (pb->pl_count - 1);
                bool executed = false;
                
                gen_qbdf_level_iprice64_qbdf_time(m_qbdf_builder, id, m_micros_recv_qbdf_time,
                                                  record_time_delta, exch_char, side,
                                                  '0', '0', '0',
                                                  ba->pl_order_count, ba->size, iprice, last_in_group, executed, delta_in_millis);
              }
              
              price_buf += ba->attached_order_count * sizeof(nxt_book::attached_order);
            } else {
              MDOrderBook& ob = order_book->book(id);
              MDOrderBookHandler::mutex_t::scoped_lock lock(order_book->m_mutex);
              MDOrderBook::mutex_t::scoped_lock book_lock(ob.m_mutex, true);
              
              for(uint16_t j = 0; j < ba->attached_order_count; ++j) {
                const nxt_book::attached_order* ao = reinterpret_cast<const nxt_book::attached_order*>(price_buf);
                price_buf += sizeof(nxt_book::attached_order);
                
                MDOrder* order = order_book->unlocked_find_order(ao->order_id);
                if(order) {
                  if(ao->order_size > 0) {
                    if(iprice == order->price) {
                      inside_changed |= order_book->unlocked_modify_order(ts, order, ao->order_size, false);
                    } else {
                      inside_changed |= order_book->unlocked_modify_order_ip(ts, order, side, iprice, ao->order_size);
                    }
                  } else {
                    inside_changed |= order_book->unlocked_remove_order(ts, order, false);
                  }
                  if(m_qbdf_builder) {
                    int32_t record_time_delta = 0; bool delta_in_millis = false;
                    get_exch_time_delta(exch_time_micros, m_micros_recv_time, record_time_delta, delta_in_millis);
                    gen_qbdf_order_modify_micros_qbdf_time(m_qbdf_builder, id, ao->order_id, m_micros_recv_qbdf_time,
                                                           record_time_delta, exch_char, side, ao->order_size, iprice, delta_in_millis);
                  }
                } else {
                  if(ao->order_size > 0) {
                    inside_changed |= order_book->unlocked_add_order_ip(ts, ao->order_id, id, side, iprice, ao->order_size);
                    if(m_qbdf_builder) {
                      int32_t record_time_delta = 0; bool delta_in_millis = false;
                      get_exch_time_delta(exch_time_micros, m_micros_recv_time, record_time_delta, delta_in_millis);
                      gen_qbdf_order_add_micros_qbdf_time(m_qbdf_builder, id, ao->order_id, m_micros_recv_qbdf_time,
                                                          record_time_delta, exch_char, side, ao->order_size, iprice, delta_in_millis);
                    }
                  }
                }
              }
              
              if(ba->size == 0) {
                MDPriceLevel* pl = ob.find_price_level(side, iprice);
                if(pl) {
                  if(m_qbdf_builder) {
                    int32_t record_time_delta = 0;
                    bool delta_in_millis = false;
                    get_exch_time_delta(exch_time_micros, m_micros_recv_time, record_time_delta, delta_in_millis);
                    
                    // Decide whether to send out a single level message to wipe out the level or iterate over the orders
                    if(true) {
                      for(MDOrder* order = pl->m_orders; order; order = order->next) {
                        gen_qbdf_order_modify_micros_qbdf_time(m_qbdf_builder, id, order->order_id, m_micros_recv_qbdf_time,
                                                               record_time_delta, exch_char, side, 0, iprice, delta_in_millis);
                      }
                    } else {
                      bool last_in_group = true;
                      bool executed = false;
                      gen_qbdf_level_iprice64_qbdf_time(m_qbdf_builder, id, m_micros_recv_qbdf_time,
                                                        record_time_delta, exch_char, side,
                                                        '0', '0', '0',
                                                        ba->pl_order_count, ba->size, iprice, last_in_group, executed, delta_in_millis);
                    }
                  }
                  
                  m_logger->log_printf(Log::INFO, "Price level %0.4f for product_id %d had to be cleared with %d pending orders and %d remaining shares",
                                       ba->price, id, pl->num_orders, pl->total_shares);
                  // The level wasn't adequately cleared
                  inside_changed |= ob.remove_price_level(side, pl);
                }
              }
            }
          }
          
          if(inside_changed) {
            m_quote.set_time(Time::current_time());
            m_quote.m_id = id;
            m_quote.m_exch = exch;
            
            order_book->fill_quote_update(id, m_quote);
            
            send_quote_update();
          }
        }
      }
    break;
    case 'I': // Trade Initial Snapshot
      // Not sure we want to do anything with the snapshot
      break;
    case 'J': // Trade Update
      {
        const nxt_book::trade* t = reinterpret_cast<const nxt_book::trade*>(buf);
        
        m_trade.set_time(m_midnight + convert_timestamp(t->trade_timestamp));
        m_trade.m_id = id;
        m_trade.m_exch = exch;
        m_trade.m_price = t->price;
        m_trade.m_size = t->trade_size;
        m_trade.m_size_adjustment_to_cum = t->trade_size;
        switch(t->side) {
        case 0:  m_trade.m_side = 'S'; break;
        case 2:  m_trade.m_side = 'B'; break;
        default: m_trade.m_side = 'U'; break;
        }
        
        m_trade.m_is_clean = true;
        m_trade.m_misc[0] = ' ';

        if(t->trade_flags.OpeningAuction || t->trade_flags.OpeningPrint) {
          m_trade.m_trade_status = TradeStatus::OpenTrade;
          m_trade.m_misc[0] = 'A';
        } else if(t->trade_flags.ClosingAuction || t->trade_flags.ClosingPrint) {
          m_trade.m_trade_status = TradeStatus::CloseTrade;
          m_trade.m_misc[0] = 'A';
        } else if(t->trade_flags.UndefinedAuction || t->trade_flags.IntradayAuction ||
                  t->trade_flags.UnscheduledAuction || t->trade_flags.Cross) {
          m_trade.m_trade_status = TradeStatus::Cross;
        } else {
          m_trade.m_trade_status = TradeStatus::New;
        }
        
        if(t->trade_flags.OffExchange || t->trade_flags.Cancel) {
          m_trade.m_trade_status = TradeStatus::Invalid;
          m_trade.m_size_adjustment_to_cum = 0;
          m_trade.m_is_clean = false;
        }

        if(m_qbdf_builder) {
          const char* trade_flags = "    ";
          switch(m_trade.m_trade_status.index()) {
          case TradeStatus::New:
            break;
          case TradeStatus::Cross:
            trade_flags = "X   ";
            break;
          case TradeStatus::OpenTrade:
            trade_flags = "O   ";
            break;
          case TradeStatus::CloseTrade:
            trade_flags = "C   ";
            break;
          case TradeStatus::Invalid:
            trade_flags = "?   ";
            break;
          }
	  uint64_t exch_time_micros = convert_micros(t->trade_timestamp) - m_tz_offset;
          
          int32_t record_time_delta = 0; bool delta_in_millis = false;
          get_exch_time_delta(exch_time_micros, m_micros_recv_time, record_time_delta, delta_in_millis);
          gen_qbdf_trade_micros_qbdf_time(m_qbdf_builder, id,
                                          m_micros_recv_qbdf_time, record_time_delta,
                                          exch_char, m_trade.m_side, '0', trade_flags,
                                          m_trade.m_size, MDPrice::from_fprice(m_trade.m_price), delta_in_millis);
        }
        
        send_trade_update();        
      }
    break;
    case 'M':
    case 'N':
      {
        const nxt_book::imbalance* msg = reinterpret_cast<const nxt_book::imbalance*>(buf);
        
        m_auction.set_time(m_midnight + convert_timestamp(msg->timestamp));
        
        m_auction.m_id = id;
        m_auction.m_exch = exch;
        
        m_auction.m_ref_price = msg->ref_price;
        m_auction.m_far_price = msg->far_price;
        if(msg->near_price != 0) {
          m_auction.m_near_price = msg->near_price;
        } else {
          m_auction.m_near_price = msg->ref_price;
        }
        m_auction.m_paired_size = msg->match_volume;
        m_auction.m_imbalance_size = msg->imbalance_volume;
        
        switch(msg->imbalance_status) {
        case 0 : m_auction.m_imbalance_direction = 'B'; break;
        case 1 : m_auction.m_imbalance_direction = 'S'; break;
        case 2 : m_auction.m_imbalance_direction = 'I'; break;
        case 3 : m_auction.m_imbalance_direction = 'O'; break;
        case 4 : m_auction.m_imbalance_direction = 'I'; break;
        default: m_auction.m_imbalance_direction = 'I'; break;
        }
        
        //m_auction.m_indicative_price_flag = msg.cross_type;
        
        if(m_qbdf_builder) {
	  uint64_t exch_time_micros = convert_micros(msg->timestamp) - m_tz_offset;
          int32_t record_time_delta = 0; bool delta_in_millis = false;
          get_exch_time_delta(exch_time_micros, m_micros_recv_time, record_time_delta, delta_in_millis);
          
          MDPrice ref_price = MDPrice::from_fprice(msg->ref_price);
	  MDPrice near_price;
	  if(msg->near_price != 0) {
	    near_price = MDPrice::from_fprice(msg->near_price);
	  } else {
	    near_price = ref_price;
	  }
          
          MDPrice far_price = MDPrice::from_fprice(msg->far_price);
          gen_qbdf_imbalance_micros_qbdf_time(m_qbdf_builder, id, m_micros_recv_qbdf_time, record_time_delta, exch_char,
                                              ref_price, near_price, far_price,
                                              m_auction.m_paired_size, m_auction.m_imbalance_size,
                                              m_auction.m_imbalance_direction, m_auction.m_indicative_price_flag, delta_in_millis);
        }
        
        if(m_provider) {
          m_provider->check_auction(m_auction);
        }        
        if(m_subscribers) {
          m_subscribers->update_subscribers(m_auction);
        }
      }
    break;
    case 'Q':
    case 'R':
      {
        const nxt_book::trading_status* msg = reinterpret_cast<const nxt_book::trading_status*>(buf);
        
        m_msu.set_time(m_midnight + convert_timestamp(msg->timestamp));
        
        m_msu.m_id = id;
        m_msu.m_exch = exch;

        QBDFStatusType qbdf_status = QBDFStatusType::Invalid;

        switch(msg->trading_phase) {
        case 'C': m_msu.m_market_status = MarketStatus::Closed; qbdf_status = QBDFStatusType::Closed; break;
        case 'O': m_msu.m_market_status = MarketStatus::Open; qbdf_status = QBDFStatusType::Open; break;
        case 'M':  // No reference price available
        case 'H': m_msu.m_market_status = MarketStatus::Halted; qbdf_status = QBDFStatusType::Halted; break;
        case 'A': // Mid-day Auction
        case 'B': // Opening Auction
        case 'D': // Closing Auction
        case 'E': // Cross period
        case 'F': // Opening cross period
        case 'G': // Closing cross period
          m_msu.m_market_status = MarketStatus::Auction;
          qbdf_status = QBDFStatusType::Auction;
        break; 
        
        case 'I': // Pre-open period
        case 'J': // Post-close period
        case 'K': // Trading at closing price
        case 'L': // Closed for Day
          m_msu.m_market_status = MarketStatus::Closed;
          qbdf_status = QBDFStatusType::Closed;
        break;
        case 'U': // Unknown
        default:
          m_msu.m_market_status = MarketStatus::Unknown;
          qbdf_status = QBDFStatusType::Invalid;
        break;
        }
        m_msu.m_ssr = msg->short_sale_restriction ? SSRestriction::Activated : SSRestriction::None;
        
        if(m_qbdf_builder) {
	  uint64_t exch_time_micros = convert_micros(msg->timestamp) - m_tz_offset;
          int32_t record_time_delta = 0; bool delta_in_millis = false;
          get_exch_time_delta(exch_time_micros, m_micros_recv_time, record_time_delta, delta_in_millis);
          if(delta_in_millis) {
            record_time_delta = numeric_limits<int>::min();
          }
          
          gen_qbdf_status_micros_qbdf_time(m_qbdf_builder, id, m_micros_recv_qbdf_time, record_time_delta, exch_char, qbdf_status.index(), "");
          
          QBDFStatusType ssr = msg->short_sale_restriction ? QBDFStatusType::ShortSaleRestrictActivated :  QBDFStatusType::NoShortSaleRestrictionInEffect;
          gen_qbdf_status_micros_qbdf_time(m_qbdf_builder, id, m_micros_recv_qbdf_time, record_time_delta, exch_char, ssr.index(), "");
        }
        
        if(m_subscribers) {
          m_subscribers->update_subscribers(m_msu);
        }
        
      }
    break;
    default:
      break;
      
    }
    
    if(m_waiting_snapshot.test(id) && (message_header->message_type == 'A' || message_header->message_type == 'B')) {
      m_waiting_snapshot.set(id, false);
      MDBufferQueue& q = m_symbol_seq[id];
      while(!q.empty()) {
        if(q.top().seq_num >= message_header->symbol_seq_number) {
          handle_message(context, q.top().msg);
        }
        q.queue_pop();
      }
    }
    
  }

  void
  NXT_Book_Handler::send_quote_update()
  {
    /*
    m_msu.m_market_status = m_security_status[m_quote.m_id];
    m_feed_rules->apply_quote_conditions(m_quote, m_msu);
    if(m_msu.m_market_status != m_security_status[m_quote.m_id]) {
      m_msu.m_id = m_quote.m_id;
      m_security_status[m_quote.m_id] = m_msu.m_market_status;
      if(m_subscribers) {
        m_subscribers->update_subscribers(m_msu);
      }
    }
    */
    if(m_provider) {
      m_provider->check_quote(m_quote);
    }
    update_subscribers(m_quote);
  }

  void
  NXT_Book_Handler::send_trade_update()
  {
    /*
    m_msu.m_market_status = m_security_status[m_trade.m_id];
    m_feed_rules->apply_trade_conditions(m_trade, m_msu);
    if(m_msu.m_market_status != m_security_status[m_trade.m_id]) {
      m_msu.m_id = m_trade.m_id;
      m_security_status[m_trade.m_id] = m_msu.m_market_status;
      if(m_subscribers) {
        m_subscribers->update_subscribers(m_msu);
      }
    }
    */
    if(m_provider) {
      m_provider->check_trade(m_trade);
    }
    update_subscribers(m_trade);
  }
  
  void
  NXT_Book_Handler::reset(const char* msg)
  {
    m_fragment_buffer.clear();
    
    if(msg != 0 && msg[0] != '\0') {
      send_alert(msg);
    }
    
    clear_order_books(false);
    
    if(m_qbdf_builder) {
      BOOST_FOREACH(char exch_char, m_qbdf_exchange_chars) {
        gen_qbdf_gap_marker_micros(m_qbdf_builder, m_micros_recv_time, 0, exch_char, -1);
      }
    }
    
    if(m_subscribers) {
      BOOST_FOREACH(ExchangeID eid, m_supported_exchanges) {
        m_subscribers->post_invalid_quote(eid);
      }
    }
  }
  
  void
  NXT_Book_Shared_TCP_Handler::send_retransmit_request(channel_info& b)
  {
    nxt_book::packet_recovery_request msg;
    msg.message_type = 'r';
    msg.message_size = sizeof(msg);
    msg.packet_sequence_number = 0;
    msg.packet_count = 0;
  }
  
  void
  NXT_Book_Handler::send_retransmit_request(channel_info& b)
  {
    
  }

  
  
  size_t
  NXT_Book_Handler::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* packet_buf, size_t packet_len)
  {
    if(context == 0) { // multicast stream
      const nxt_book::multicast_packet_header_common* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_common*>(packet_buf);
      
      uint8_t fragment_indicator = 0;
      int     message_count = 0;
      size_t  header_size = 0;
      
      if(packet_header->protocol_version >= 5) {
        const nxt_book::multicast_packet_header_v06* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_v06*>(packet_buf);
        packet_header->dump(log, filter);
        if(packet_header->packet_type == 'D') {
          const nxt_book::data_packet* dp = reinterpret_cast<const nxt_book::data_packet*>(packet_buf + sizeof(nxt_book::multicast_packet_header_v06));
          fragment_indicator = dp->fragment_indicator;
          message_count = dp->message_count;
          header_size = sizeof(nxt_book::multicast_packet_header_v06) + sizeof(nxt_book::data_packet);
        } else if(packet_header->packet_type == 'H') {
          const nxt_book::heartbeat* msg = reinterpret_cast<const nxt_book::heartbeat*>(packet_buf + sizeof(nxt_book::multicast_packet_header_v06));
          msg->dump(log, filter);
          return packet_len;
        } else {
          return packet_len;
        }
      } else {
        const nxt_book::multicast_packet_header_v01* packet_header = reinterpret_cast<const nxt_book::multicast_packet_header_v01*>(packet_buf);
        packet_header->dump(log, filter);
        fragment_indicator = packet_header->fragment_indicator;
        message_count = packet_header->message_count;
        header_size = sizeof(nxt_book::multicast_packet_header_v01);
      }
      
      if(fragment_indicator != 0) {
        log.printf("fragmented packet dumping not implemented\n");
        return packet_len;
      }
      
      const char* buf = packet_buf + header_size;
      ssize_t len = packet_len - header_size;
      
      for(int pkt = 0; pkt < message_count; ++pkt) {
        const nxt_book::header_only* message_header = reinterpret_cast<const nxt_book::header_only*>(buf);
        if(message_header->message_size > len) {
          log.printf("NXT_Book_Handler::dump  buffer len %u too short for message %ld   message_type:%c\n",
                     message_header->message_size, len, message_header->message_type);
          //return packet_len;
        }
        
        dump_message(log, filter, context, buf);
        buf += message_header->message_size;
        len -= message_header->message_size;
      }
    } else {
      dump_message(log, filter, context, packet_buf);
    }
    
    return packet_len;
  }
  
  size_t
  NXT_Book_Handler::dump_message(ILogger& log, Dump_Filters& filter, size_t context, const char* buf)
  {
    const nxt_book::header_only* message_header = reinterpret_cast<const nxt_book::header_only*>(buf);
    
    log.printf("  message_type:%c  message_size:%d  ", message_header->message_type, message_header->message_size);
    
    switch(message_header->message_type) {
    case 'a': // Get Symbol ID
      {
        const nxt_book::get_symbol_id* msg = reinterpret_cast<const nxt_book::get_symbol_id*>(buf);
        msg->dump(log, filter);
      }
      break;
    case 'b':  // Symbol ID
      {
        const nxt_book::symbol_id* msg = reinterpret_cast<const nxt_book::symbol_id*>(buf);
        msg->dump(log, filter);
      }
      break;
    case 'c': // Client Hello
      {
        const nxt_book::client_hello* msg = reinterpret_cast<const nxt_book::client_hello*>(buf);
        log.printf("client_hello  client_id:%.15s\n", msg->client_id);
      }
      break;
    case 'h': // heartbeat
      {
        const nxt_book::heartbeat* msg = reinterpret_cast<const nxt_book::heartbeat*>(buf);
        
        union { uint8_t quad[4]; uint32_t i; }  host_i;
        host_i.i = msg->tcp_service_ip;
        char host[32];
        snprintf(host, 32, "%d.%d.%d.%d", host_i.quad[0], host_i.quad[1], host_i.quad[2], host_i.quad[3]);
        
        log.printf("heartbeat  priority:%d  major_data_version:%d  minor_data_version:%d  heartbeat_interval:%d  tcp_service_ip:%s  tcp_service_port:%d\n",
                   msg->priority, msg->major_data_version, msg->minor_data_version, msg->heartbeat_interval, host, msg->tcp_service_port);
      }
      break;
    case 's': // subscription
      {
        const nxt_book::subscription* msg = reinterpret_cast<const nxt_book::subscription*>(buf);
        log.printf("subscription  symbol_id:%u  md_type:%d\n", msg->symbol_id, msg->md_type);
      }
      break;
    case 'A': // Price Book Initial Snapshot
    case 'C': // Price Book Snapshot
    case 'F': // Price book Update
      {
        const char* price_buf = buf;
        const nxt_book::price_book* pb = reinterpret_cast<const nxt_book::price_book*>(price_buf);
        log.printf("  price_book  symbol_id:%u  symbol_seq_number:%u  symbol_status:%u  pl_count:%u\n", pb->symbol_id,
                   pb->symbol_seq_number, pb->symbol_status, pb->pl_count);
        price_buf += sizeof(nxt_book::price_book);
        for(uint16_t i = 0; i < pb->pl_count; ++i) {
          const nxt_book::price_book_bid_ask* ba = reinterpret_cast<const nxt_book::price_book_bid_ask*>(price_buf);
          char ts[64];
          Time_Utils::print_time(ts, convert_timestamp(ba->timestamp), Timestamp::MICRO);
          
          log.printf("    price_book_bid_ask  side:%c  price:%0.4f  size:%u  timestamp:%s  pl_order_count:%u  attached_order_count:%u\n",
                     ba->side ? 'B' : 'S', ba->price, ba->size, ts, ba->pl_order_count, ba->attached_order_count);
          price_buf += sizeof(nxt_book::price_book_bid_ask);
          
          for(uint16_t j = 0; j < ba->attached_order_count; ++j) {
            const nxt_book::attached_order* ao = reinterpret_cast<const nxt_book::attached_order*>(price_buf);
            char ts[64];
            Time_Utils::print_time(ts, convert_timestamp(ao->order_timestamp), Timestamp::MICRO);
            log.printf("      attached_order  order_id:%lu  order_size:%u  order_timestamp:%s\n",
                       ao->order_id, ao->order_size, ts);
            price_buf += sizeof(nxt_book::attached_order);
          }
        }
      }
      break;
    case 'I':  // Trade Initial Snapshot
    case 'J':  // Trade Update
      {
        const nxt_book::trade* msg = reinterpret_cast<const nxt_book::trade*>(buf);
        char ts[64];
        Time_Utils::print_time(ts, convert_timestamp(msg->trade_timestamp), Timestamp::MICRO);
        
        char side;
        switch(msg->side) {
        case 0:  side = 'S'; break;
        case 2:  side = 'B'; break;
        default: side = 'U'; break;
        }
        
        const char* msg_type = "";
        if(message_header->message_type == 'I') {
          msg_type = "_initial_snapshot";
        }
        
        log.printf("  trade%s  symbol_id:%d  symbol_seq_number:%u  symbol_status:%u  price:%0.4f  high_price:%0.4f  low_price:%0.4f  trade_id:%lu  order_id:%lu  trade_size:%u  timestamp:%s  side:%c  trade_flags:(",
                   msg_type, msg->symbol_id, msg->symbol_seq_number, msg->symbol_status, msg->price, msg->high_price,
                   msg->low_price, msg->trade_id, msg->order_id, msg->trade_size, ts, side);
        msg->trade_flags.print(log);
        log.printf(")\n");
      }
    break;
    case 'B': // Price Book Initial Status
    case 'D': // Price Book Status
    case 'G': // Trade Initial Status
    case 'H': // Trade Status
    case 'K': // Imbalance Initial Status
    case 'L': // Imbalance Status
    case 'O': // Trading Status Initial Status
    case 'P': // Trading Status Status
      {
        const nxt_book::status_message* msg = reinterpret_cast<const nxt_book::status_message*>(buf);
        
        const char* msg_type = 0;
        switch(message_header->message_type) {
        case 'B': msg_type = "Price Book Initial Status"; break;
        case 'D': msg_type = "Price Book Status"; break;
        case 'G': msg_type = "Trade Initial Status"; break;
        case 'H': msg_type = "Trade Status"; break;
        case 'K': msg_type = "Imbalance Initial Status"; break;
        case 'L': msg_type = "Imbalance Status"; break;
        case 'O': msg_type = "Trading Status Initial Status"; break;
        case 'P': msg_type = "Trading Status Status"; break;
        default:  msg_type = "Unknown"; break;
        }
        
        const char* status_msg = "";
        switch(msg->symbol_status) {
        case 1: status_msg = "No data available"; break;
        case 16: status_msg = "Subscription failed"; break;
        case 32: status_msg = "Not entitled"; break;
        case 64: status_msg = "Invalid password"; break;
        default: break;
        };
        
        log.printf("  status_message  %s  symbol_id:%d  symbol_seq_number:%u  symbol_status:%d %s\n",
                   msg_type, msg->symbol_id, msg->symbol_seq_number, msg->symbol_status, status_msg);
      }
    break;
    case 'M':
    case 'N':
      {
        const nxt_book::imbalance* msg = reinterpret_cast<const nxt_book::imbalance*>(buf);
        msg->dump(log, filter);
      }
      break;
    case 'Q':
    case 'R':
      {
        const nxt_book::trading_status* msg = reinterpret_cast<const nxt_book::trading_status*>(buf);
        msg->dump(log, filter);
      }
    break;
    default :
      log.printf("Unknown msg type %c\n", message_header->message_type);
    }
    
    return message_header->message_size;
  }
  
  namespace nxt_book {
    void trading_status::dump(ILogger& log, Dump_Filters& filter) const {
      const char* phase;
      switch(trading_phase) {
      case 'C': phase = "Closed"; break;
      case 'O': phase = "Open"; break;
      case 'M': phase = "No reference price available"; break;
      case 'H': phase = "Trading halted"; break;
      case 'A': phase = "Mid-day auction call"; break;
      case 'B': phase = "Opening auction call"; break;
      case 'D' : phase = "Closing auction call"; break;
      case 'E' : phase = "Cross period"; break;
      case 'F' : phase = "Opening cross period"; break;
      case 'G' : phase = "Closing cross period"; break;
      case 'I' : phase = "Pre-open period"; break;
      case 'J' : phase = "Post-close period"; break;
      case 'K' : phase = "Trading at closing price"; break;
      case 'L' : phase = "Closed for the day"; break;
      case 'U' : phase = "Trading phase unknown"; break;
      default:   phase = "Unknown"; break;
      }
      
      char ts[64];
      Time_Utils::print_time(ts, convert_timestamp(timestamp), Timestamp::MICRO);
      
      log.printf("  trading_status   symbol_id:%d  symbol_seq_number:%u  symbol_status:%d  trading_phase:%d - %s SSR:%d  timestamp:%s\n",
                 symbol_id, symbol_seq_number, symbol_status, trading_phase, phase, short_sale_restriction, ts);
    }
  }
  
  
  void
  NXT_Book_Handler::init(const string& name, const vector<ExchangeID>& exchanges, const Parameter& params)
  {
    m_supported_exchanges = exchanges;
    
    Handler::init(name, params);
    
    if(m_dispatcher && !m_tcp_handler) {
      MDDispatcher* tcp_dispatcher = m_provider->md_manager().get_dispatcher(params["tcp_dispatcher"].getString());
      m_tcp_handler = new NXT_Book_Shared_TCP_Handler(m_app);
      if(tcp_dispatcher) {
        m_tcp_handler->set_dispatcher(tcp_dispatcher);
      } else {
        m_tcp_handler->set_dispatcher(m_dispatcher);
      }
      m_tcp_handler->init("nxt_book_tcp", params);
    }
    
    m_name = name;
    m_client_id = params["client_id"].getString();
    m_password = params["password"].getString();
    
    m_midnight = Time::utc_today();
    
    bool build_book = params.getDefaultBool("build_book", true);
    if(build_book) {
      for(vector<ExchangeID>::const_iterator eid = exchanges.begin(); eid != exchanges.end(); ++eid) {
        boost::shared_ptr<MDOrderBookHandler> ob(new MDOrderBookHandler(m_app, *eid, m_logger));
        ob->init(params);
        m_order_books.push_back(ob);
      }
    }
    
    //m_quote.m_exch = exch;
    //m_trade.m_exch = exch;
    //m_auction.m_exch = exch;
    //m_msu.m_exch = exch;
    
    memset(m_trade.m_trade_qualifier, ' ', sizeof(m_trade.m_trade_qualifier));
    m_trade.m_correct = '\0';
    
    const string& feed_rules_name = params["feed_rules_name"].getString();
    m_feed_rules = ObjectFactory<string,Feed_Rules>::allocate(feed_rules_name);
    m_feed_rules->init(m_app, Time::today().strftime("%Y%m%d"));

    if (m_qbdf_builder) {
      m_tz_offset = m_qbdf_builder->get_bin_tz_adjust() * Time_Constants::micros_per_second;
      if (params.has("symbol_map_file")) {
        string symbol_map = params["symbol_map_file"].getString();
        CSV_Parser parser(symbol_map);
        parser.set_ignore_errors(true);
        while(CSV_Parser::OK == parser.read_row()) {
          string symbol = parser.get_column(0);
          size_t start = std::max(static_cast<int>(symbol.size() - 4), 0);
          string mic = symbol.substr(start);
          boost::optional<ExchangeID> symbol_eid = ExchangeID::get_by_name(mic.c_str());
          if (!symbol_eid) {
            continue;
          }

          if (std::find(m_supported_exchanges.begin(), m_supported_exchanges.end(), symbol_eid.get()) == m_supported_exchanges.end()) {
            continue;
          }

          string nxt_id = parser.get_column(1);

          ProductID id = m_qbdf_builder->process_taq_symbol(nxt_id);
          this->subscribe_product(id, symbol_eid.get(), nxt_id, "");

          map<string, subscription_t>::iterator i = m_subscriptions.find(nxt_id);
          if(i != m_subscriptions.end()) {
            uint32_t nxt_id_num = strtoul(nxt_id.c_str(), NULL, 10);
            subscription_t& sub = i->second;
            m_nxt_subscriptions.insert(make_pair(nxt_id_num, sub));
          }
        }
      }

      //m_tz_offset = 0;
    }
    
    m_security_status.resize(m_app->pm()->max_product_id(), MarketStatus::Unknown);
    m_waiting_snapshot.resize(m_app->pm()->max_product_id(), false);
    m_symbol_seq.resize(m_app->pm()->max_product_id());
  }
  
  void
  NXT_Book_Handler::reselect_publisher()
  {
    m_fragment_buffer.clear();
    m_nxt_publishers.clear();
    m_nxt_subscriptions.clear();
    
    m_not_logged_in = true;
    m_chosen_sender_id = -1;
    
    m_waiting_snapshot.reset();
    
    for(ProductID p = 0; p < m_app->pm()->max_product_id(); ++p) {
      m_security_status[p] = MarketStatus::Unknown;
      m_symbol_seq[p].queue_clear();
    }
    
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
  }
  
  void
  NXT_Book_Handler::start()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }
    
    clear_order_books(false);
  }
  
  void
  NXT_Book_Handler::stop()
  {
    reselect_publisher();
    
    clear_order_books(false);
  }
  
  NXT_Book_Shared_TCP_Handler::NXT_Book_Shared_TCP_Handler(Application* app)
    : Handler(app, "NXT_Book_Shared_TCP_Handler")
  {
  }
  
  NXT_Book_Shared_TCP_Handler::~NXT_Book_Shared_TCP_Handler()
  {
  }
  
  
  NXT_Book_Handler::NXT_Book_Handler(Application* app) :
    Handler(app, "NXT_Book"),
    m_not_logged_in(true),
    m_chosen_sender_id(-1),
    m_other_heartbeat(0)
  {
  }

  NXT_Book_Handler::~NXT_Book_Handler()
  {
  }

  void NXT_Book_Handler::create_qbdf_channel_info()
  {
    m_channel_info_map.push_back(channel_info("", 0));
    m_channel_info_map.push_back(channel_info("", 1));
    m_channel_info_map.push_back(channel_info("", 2));
  }
  
  namespace nxt_book {
    
    void trade::TradeFlags::print(ILogger& log) const
    {
      if(OffExchange) { log.puts("OffExchange "); }
      if(OffBook) { log.puts("OffBook "); }
      if(Hidden) { log.puts("Hidden "); }
      if(Matched) { log.puts("Matched "); }
      if(UndefinedAuction) { log.puts("UndefinedAuction "); }
      if(OpeningAuction) { log.puts("OpeningAuction "); }
      if(ClosingAuction) { log.puts("ClosingAuction "); }
      if(IntradayAuction) { log.puts("IntradayAuction "); }
      if(UnscheduledAuction) { log.puts("UnscheduledAuction "); }
      if(Cross) { log.puts("Cross "); }
      if(Dark) { log.puts("Dark "); }
      if(Cancel) { log.puts("Cancel "); }
      if(OTC) { log.puts("OTC "); }
      if(AtMarketClose) { log.puts("AtMarketClose "); }
      if(OutOfMainSession) { log.puts("OutOfMainSession "); }
      if(TradeThroughExempt) { log.puts("TradeThroughExempt "); }
      if(Acquisition) { log.puts("Acquisition "); }
      if(Bunched) { log.puts("Bunched "); }
      if(Cash) { log.puts("Cash "); }
      if(Distribution) { log.puts("Distribution "); }
      if(IntermarketSweep) { log.puts("IntermarketSweep "); }
      if(BunchedSold) { log.puts("BunchedSold "); }
      if(OddLot) { log.puts("OddLot "); }
      if(PriceVariation) { log.puts("PriceVariation "); }
      if(Rule127_155) { log.puts("Rule127_155 "); }
      if(SoldLast) { log.puts("SoldLast "); }
      if(NextDay) { log.puts("NextDay "); }
      if(MCOpen) { log.puts("MCOpen "); }
      if(OpeningPrint) { log.puts("OpeningPrint "); }
      if(MCClose) { log.puts("MCClose "); }
      if(ClosingPrint) { log.puts("ClosingPrint "); }
      if(CorrectedConsClose) { log.puts("CorrectedConsClose "); }
      if(PriorRefPrice) { log.puts("PriorRefPrice "); }
      if(Seller) { log.puts("Seller "); }
      if(Split) { log.puts("Split "); }
      if(FormT) { log.puts("FormT "); }
      if(ExtOutOfSeq) { log.puts("ExtOutOfSeq "); }
      if(StockOption) { log.puts("StockOption "); }
      if(AvgPrice) { log.puts("AvgPrice "); }
      if(YellowFlag) { log.puts("YellowFlag "); }
      if(SoldOutOfSeq) { log.puts("SoldOutOfSeq "); }
      if(StoppedStock) { log.puts("StoppedStock "); }
      if(DerivPriced) { log.puts("DerivPriced "); }
      if(MCReOpen) { log.puts("MCReOpen "); }
      if(AutoExec) { log.puts("AutoExec "); }
      if(Unused) { log.puts("Unused "); }
    }
    
  }
  
}
