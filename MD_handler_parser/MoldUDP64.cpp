#include <Data/MoldUDP64.h>
#include <Data/TVITCH_4_1_Handler.h>
#include <MD/MDManager.h>

#include <app/app.h>
#include <Util/except.h>
#include <Util/Network_Utils.h>

#include <Logger/Logger_Manager.h>

#include <unistd.h>
#include <sys/types.h>

#include <stdarg.h>
#include <syscall.h>

#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)

namespace hf_core {

  using namespace hf_tools;
  
  struct message_block_t {
    uint16_t message_length;
    char     msg[0];
  } __attribute__ ((packed));
  
  struct downstream_header_t {
    char      session[10];
    uint64_t  seq_num;
    uint16_t  message_count;
    message_block_t message_block[0];
  } __attribute__ ((packed));
  
  struct request_packet_t {
    char      session[10];
    uint64_t  seq_num;
    uint16_t  message_count;
  } __attribute__ ((packed));


  void
  MoldUDP64::init(const string& name, const Parameter& params)
  {
    //const char* where = "MoldUDP64::init: ";

    Handler::init(name, params);
    m_name = name;

    if(params.has("record_file")) {
      m_recorder = m_app->lm()->get_logger(params["record_file"].getString());
    }
  }


  size_t
  MoldUDP64::parse(size_t context, const char* buffer, size_t len)
  {
    const downstream_header_t* header = reinterpret_cast<const downstream_header_t*>(buffer);
    
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    uint64_t next_seq_num = seq_num + message_count;
    
    if(unlikely(m_drop_packets)) {
      --m_drop_packets;
      return len;
    }
    
    if(next_seq_num <= m_channel_info.seq_no) {
      return len;
    }
    
    mutex_t::scoped_lock lock;
    if(context != 0) {
      lock.acquire(m_mutex);
    }
    
    m_message_parser->set_hw_recv_time(m_hw_recv_time);
    
    if(unlikely(!m_channel_info.last_update.is_set())) {
      memcpy(m_session, header->session, 10);
      m_logger->log_printf(Log::INFO, "MoldUDP64: %s Session = %10s", m_name.c_str(), m_session);
      m_channel_info.seq_no = seq_num;
    }
    /*
    if(unlikely(strncmp(m_session, header->session, 10))) {
      return len;
    }
    */
    
    if(next_seq_num <= m_channel_info.seq_no) {
      return len;
    } else if(seq_num > m_channel_info.seq_no) {
      if(m_channel_info.begin_recovery(this, buffer, len, seq_num, next_seq_num)) {
        return len;
      }
    }
    
    size_t parsed_len = MoldUDP64::parse2(context, buffer, len);
    
    m_channel_info.seq_no = next_seq_num;
    m_channel_info.last_update = Time::currentish_time();
    
    if(!m_channel_info.queue.empty()) {
      m_channel_info.process_recovery(this);
    }
    
    return parsed_len;
  }

  size_t
  MoldUDP64::parse2(size_t context, const char* buffer, size_t len)
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
    
    const downstream_header_t* header = reinterpret_cast<const downstream_header_t*>(buffer);
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    
    size_t parsed_len = sizeof(downstream_header_t);
    const message_block_t* block = &header->message_block[0];
    
    if(seq_num < m_channel_info.seq_no && message_count > 0) {
      //m_logger->log_printf(Log::INFO, "MoldUDP64::parse2: %s received partially-new packet seq_num=%zd expected_seq_num=%zd msg_count=%d",
      //                     m_name.c_str(), seq_num, m_channel_info.seq_no, message_count);
      while(seq_num < m_channel_info.seq_no && message_count > 0) {
        // a rare case, this message contains some things we've already seen and some
        // things we haven't, skip what we've already seen
        uint16_t message_length = ntohs(block->message_length);
        parsed_len += 2 + message_length;
        if (unlikely(parsed_len > len)) {
          m_logger->log_printf(Log::ERROR, "MoldUDP64::parse: tried to more bytes than len (%zd)", len);
          return len;
        }
        block = (message_block_t*) (((const char*) block) + 2 + message_length);
        --message_count;
        ++seq_num;
      }
    }
    
    for( ; message_count > 0; --message_count) {
      uint16_t message_length = ntohs(block->message_length);
      parsed_len += 2 + message_length;
      if (unlikely(parsed_len > len)) {
        m_logger->log_printf(Log::ERROR, "MoldUDP64::parse: tried to more bytes than len (%zd)", len);
        return len;
      }
      size_t parser_result = m_message_parser->parse_message(0, block->msg, message_length);
      if (unlikely(parser_result != message_length)) {
        m_logger->log_printf(Log::ERROR, "MoldUDP64::parse: ERROR message parser returned %zu bytes parsed, expected %u\n",
                             parser_result, message_length);
        return len;
      }
      block = (message_block_t*) (((const char*) block) + 2 + message_length);
    }
    if(unlikely(parsed_len != len)) {
      m_logger->log_printf(Log::ERROR, "MoldUDP64::parse: Received buffer of size %zd but parsed %zd, message_count=%d",
                           len, parsed_len, (int) ntohs(header->message_count));
      return len;
    }
    
    return parsed_len;
  }
  
  size_t
  MoldUDP64::dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buffer, size_t len)
  {
    const downstream_header_t* header = reinterpret_cast<const downstream_header_t*>(buffer);
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    
    log.printf(" seq_num:%lu  message_count:%u\n", seq_num, message_count);
    
    const message_block_t* block = &header->message_block[0];
    
    for(uint16_t i = 0; i < message_count; ++i) {
      uint16_t message_length = ntohs(block->message_length);
      TVITCH_4_1_Handler::dump(log, filter, context, block->msg, len);
      block = (message_block_t*) (((const char*) block) + 2 + message_length);
    }
    
    return len;
  }
  
  void
  MoldUDP64::send_retransmit_request(channel_info& channel_info)
  {
    if(channel_info.recovery_socket == -1) {
      return;
    }

    uint64_t from_seq_no = channel_info.seq_no;
    uint16_t message_count = channel_info.queue.top().seq_num - from_seq_no;

    send_alert("MoldUDP64::send_retransmit_request %s from_seq_no=%zd, message_count=%d", m_name.c_str(), from_seq_no, (int) message_count);

    request_packet_t req;

    memcpy(req.session, m_session, 10);
    req.seq_num = htonll(from_seq_no);
    req.message_count = htons(message_count);

    int ret = send(channel_info.recovery_socket, &req, sizeof(req), MSG_DONTWAIT);
    if(ret < 0) {
      char err_buf[512];
      char* err_msg = strerror_r(errno, err_buf, 512);
      send_alert("MoldUDP64::send_retransmit_request %s sendto returned %d - %s", m_name.c_str(), errno, err_msg);
    }
  }
  
  void
  MoldUDP64::start()
  {
    m_channel_info.clear();
    if(m_message_parser) {
      m_message_parser->start();
    }
  }

  void
  MoldUDP64::stop()
  {
    m_channel_info.clear();
    if(m_message_parser) {
      m_message_parser->stop();
    }
  }

  void
  MoldUDP64::reset(const char* msg)
  {
    if(m_message_parser) {
      m_message_parser->reset(msg);
    } else {
      m_logger->error(msg);
    }
  }

  void
  MoldUDP64::reset(size_t context, const char* msg, uint64_t expected_seq_no,
                   uint64_t received_seq_no)
  {
    if(m_message_parser) {
      m_message_parser->reset(context, msg, expected_seq_no, received_seq_no);
    } else {
      m_logger->error(msg);
    }
  }
  
  MoldUDP64::MoldUDP64(Application* app, TVITCH_4_1_Handler* p) :
    Handler(app, "MoldUDP64"),
    m_channel_info("", 1),
    m_message_parser(p)
  {

  }

  MoldUDP64::~MoldUDP64()
  {
  }



}
