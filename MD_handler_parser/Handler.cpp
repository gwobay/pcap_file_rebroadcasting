#include <Data/Handler.h>
#include <Data/QBDF_Util.h>

#include <app/app.h>
#include <Event_Hub/Event_Hub.h>
#include <Logger/Logger_Manager.h>


#include <stdarg.h>

namespace hf_core {
  
  void
  Handler::init(const string& name, const vector<ExchangeID>& supported_exchanges, const Parameter& p)
  {
    if(supported_exchanges.size() == 1) {
      init(name, supported_exchanges[0], p);
    } else {
      init(name, p);
    }
  }
  
  void
  Handler::init(const string& name, const Parameter& params)
  {
    m_logger = m_app->lm()->get_logger("MD");
    if(params.has("record_file")) {
	    string rf=params["record_file"].getString();
      m_recorder = m_app->lm()->get_logger(rf);
    if (!m_recorder) m_logger->log_printf(Log::WARN, "missing m_recorder%s", rf.c_str());
    else m_logger->log_printf(Log::INFO, "m_recorder:%s", rf.c_str());
    }
    
    m_record_only = params.getDefaultBool("record_only", false);
    m_alert_frequency = params.getDefaultTimeDuration("alert_frequency", m_alert_frequency);
    m_exclude_level_2 = params.getDefaultBool("exclude_level_2", false);
    m_exclude_imbalance = params.getDefaultBool("exclude_imbalance", false);
    m_exclude_quotes = params.getDefaultBool("exclude_quotes", false);
    m_exclude_status = params.getDefaultBool("exclude_status", false);
    m_exclude_trades = params.getDefaultBool("exclude_trades", false);
    m_use_micros = params.getDefaultBool("use_micros", false);
    m_use_exch_time = params.getDefaultBool("use_exch_time", false);
    
    if (params.has("find_order_id_list")) {
      vector<string> find_order_id_list = params["find_order_id_list"].getStringVector();
      for (size_t i = 0; i < find_order_id_list.size(); i++) {
        uint64_t order_id = strtol(find_order_id_list[i].c_str(), 0, 10);
        m_search_orders.insert(order_id);
        m_logger->log_printf(Log::INFO, "Will search for order id %lu in %s", order_id, name.c_str());
      }
    } else {
      m_search_orders.clear();
    }
  }

  bool
  Handler::check_order_id(uint64_t order_id) {
    if (m_search_orders.find(order_id) != m_search_orders.end())
      return true;

    return false;
  }

  void
  Handler::drop_packets(int num_packets)
 {
    m_drop_packets = num_packets;
  }

  void
  Handler::send_alert(const char* fmt, ...)
  {
    Time_Duration since_last = Time::currentish_time() - m_last_alert_time;

    char buf[512];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, 512, fmt, va);
    va_end(va);

    if(since_last > m_alert_frequency) {
      AlertEvent alert(m_alert_severity, Time::currentish_time(), "MD", m_handler_type, buf);
      m_app->hub()->post(alert);
      m_last_alert_time = Time::currentish_time();
    }
    m_logger->error(buf);
  }

  void
  Handler::send_alert_forced(const char* fmt, ...)
  {
    //Time_Duration since_last = Time::currentish_time() - m_last_alert_time;

    char buf[512];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, 512, fmt, va);
    va_end(va);

    AlertEvent alert(m_alert_severity, Time::currentish_time(), "MD", m_handler_type, buf);
    m_app->hub()->post(alert);
    m_last_alert_time = Time::currentish_time();

    m_logger->error(buf);
  }

  void
  Handler::send_alert(AlertSeverity sev, const char* fmt, ...)
  {
    Time_Duration since_last = Time::currentish_time() - m_last_alert_time;

    char buf[512];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, 512, fmt, va);
    va_end(va);

    if(since_last > m_alert_frequency) {
      AlertEvent alert(sev, Time::currentish_time(), "MD", m_handler_type, buf);
      m_app->hub()->post(alert);
      m_last_alert_time = Time::currentish_time();
    }
    m_logger->error(buf);
  }
  
  void
  Handler::clear_order_books(bool delete_orders)
  {
    for(vector<boost::shared_ptr<MDOrderBookHandler> >::iterator i = m_order_books.begin(), i_end = m_order_books.end(); i != i_end; ++i) {
      (*i)->clear(delete_orders);
    }
  }

  channel_info* Handler::get_channel_info(string name, size_t context){
          for (size_t i=0; i<m_channel_info_map.size(); i++) {
                  if (m_channel_info_map[i].name != name || m_channel_info_map[i].context != context) continue;
                  return &m_channel_info_map[i];
	  } 	  
          return 0;
  }
  // this is a basic implementation, but the sub-class may want to override it e.g., to create a TCP
  // connection for recovery.
  void
  Handler::start()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }

    clear_order_books(false);
  }

  bool
  Handler::is_done()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      if (!i->final_message_received) return false;
    }
    return true;
  }

  void
  Handler::stop()
  {
    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
      i->clear();
    }

    clear_order_books(false);
  }

  void
  Handler::reset(size_t context, const char* msg, uint64_t expected_seq_no,
                 uint64_t received_seq_no)
  {
    if (m_qbdf_builder) {
      if (m_micros_recv_time > 0) {
	gen_qbdf_gap_summary(m_qbdf_builder, m_micros_recv_time / 1000, context, expected_seq_no, received_seq_no);
      } else if (m_ms_latest > 0) {
	gen_qbdf_gap_summary(m_qbdf_builder, m_ms_latest, context, expected_seq_no, received_seq_no);
      } else {
	gen_qbdf_gap_summary(m_qbdf_builder, 0, context, expected_seq_no, received_seq_no);
      }
    }
    reset(context, msg);
  }

  Handler::Handler(Application* app, const string& handler_type) :
    m_app(app),
    m_handler_type(handler_type),
    m_alert_frequency(minutes(1)),
    m_alert_severity(AlertSeverity::HIGH),
    m_qbdf_builder(0),
    m_ms_latest(0),
    m_ms_since_midnight(0),
    m_last_sec_reported(0),
    m_last_min_reported(0),
    m_micros_latest(0),
    m_micros_since_midnight(0),
    m_micros_recv_time(0),
    m_micros_exch_time(0),
    m_drop_packets(0),
    m_provider(0),
    m_dispatcher(0)
  {
  }
  
  void
  parse_qrec_file(Handler* handler, Logger* logger, FILE* f_in, size_t max_context)
  {
    Time::set_simulation_mode(true);

    const char* where = "parse_qrec_file";

    size_t buffer_size = 1024 * 1024;

    size_t total_parsed_len = 0;
    size_t parsed_len = 0;
    size_t bytes_to_parse = 0;
    char buffer[buffer_size];
    
    Time last_timestamp;
    
    bool read_more = true;
    while (read_more) {
      size_t unparsed_len = bytes_to_parse - parsed_len;
      if (parsed_len > 0) {
        // Move leftover bytes to beginning of buffer, fill remaining buffer
        memmove(buffer, buffer+parsed_len, unparsed_len);
        bytes_to_parse = fread(buffer+unparsed_len, 1, buffer_size-unparsed_len, f_in);
        if (bytes_to_parse < (buffer_size-unparsed_len))
          read_more = false;

        parsed_len = 0;
        bytes_to_parse += unparsed_len;
      } else {
        bytes_to_parse = fread(buffer, 1, buffer_size, f_in);
        if (bytes_to_parse < buffer_size)
          read_more = false;
      }
      
      int failed_count = 0;
      while ((parsed_len + sizeof(record_header)) < bytes_to_parse) {
        record_header* p_head = (record_header*)(buffer+parsed_len);
        if (((strncmp("qRec", p_head->title, 4) != 0) && (strncmp("qRel", p_head->title, 4) != 0)) || p_head->context > max_context) {
          logger->log_printf(Log::ERROR, "%s: Mangled record title at byte %d out of %d (total parsed_len = %zu)",
                             where, (int)parsed_len, (int)bytes_to_parse, total_parsed_len);
          logger->sync_flush();
          
          size_t i = parsed_len;
          for ( ; i < bytes_to_parse-sizeof(record_header); ++i) {
            p_head = (record_header*)(buffer+i);
            if ((strncmp("qRec", p_head->title, 4) == 0) || (strncmp("qRel", p_head->title, 4) == 0)) {
              logger->log_printf(Log::INFO, "%s: Found next good record title at byte %d, testing context", where, (int)i);
              logger->sync_flush();
              if (p_head->context > max_context) {
                logger->log_printf(Log::ERROR, "%s: Context=%lu, not possible so must still be corrupt...", where, p_head->context);
                logger->sync_flush();
              } else {
                logger->log_printf(Log::ERROR, "%s: Context=%lu, looks okay - breaking out of error handling", where, p_head->context);
                logger->sync_flush();
                break;
              }
            }
          }
          parsed_len = i;
          if (strncmp("qRec", p_head->title, 4) && strncmp("qRel", p_head->title, 4)) {
            logger->log_printf(Log::INFO, "%s: Could not find good 'qRec' in current buffer, need to read more bytes", where);
            logger->sync_flush();
            break;
          }
          failed_count++;
        }
        
        Time rec_time(p_head->ticks);
        if(rec_time >= last_timestamp) {
          last_timestamp = rec_time;
          Time::set_current_time(rec_time);
        } else {
          logger->log_printf(Log::INFO, "%s: Prevented time from going backwards", where);
        }
        
        if(0 == strncmp("qRec", p_head->title, 4)) {
          size_t packet_size = p_head->len;
          if ((parsed_len + sizeof(record_header) + packet_size) > bytes_to_parse)
            break;
          
          uint64_t context = p_head->context;
          parsed_len += sizeof(record_header);
          size_t handler_bytes_read = handler->parse(context, buffer+parsed_len, packet_size);
          
          if (handler_bytes_read != packet_size) {
            logger->log_printf(Log::ERROR, "%s: handler_bytes_read=%zd packet_size=%zu parsed_len=%zd bytes_to_parse=%zd",
                               where, handler_bytes_read, packet_size, parsed_len, bytes_to_parse);
          } else {
            parsed_len += packet_size;
          } 
        } else {
          long_record_header* p_head = (long_record_header*)(buffer+parsed_len);
          size_t packet_size = p_head->len;
          
          if ((parsed_len + sizeof(long_record_header) + packet_size) > bytes_to_parse)
            break;
          
          uint64_t context = p_head->context;
          parsed_len += sizeof(long_record_header);
          size_t handler_bytes_read = handler->parse(context, buffer+parsed_len, packet_size);
          
          if (handler_bytes_read != packet_size) {
            logger->log_printf(Log::ERROR, "%s: handler_bytes_read=%zd packet_size=%zu parsed_len=%zd bytes_to_parse=%zd",
                               where, handler_bytes_read, packet_size, parsed_len, bytes_to_parse);
          } else {
            parsed_len += packet_size;
          } 
        }
        
        if (failed_count >= 5)
          return;
      }
      total_parsed_len += parsed_len;
    }
    logger->sync_flush();
  }
  
  void
  Handler::compare_hw_time()
  {
    char time_hw[32];
    char time_sw[32];
    
    Time t = Time::system_time_no_update();
    
    Time_Utils::print_time(time_hw, m_hw_recv_time, Timestamp::NANO);
    Time_Utils::print_time(time_sw, t, Timestamp::NANO);
    
    m_logger->log_printf(Log::INFO, "%s  hw_recv_time = %s  sw_recv_time = %s  diff = %ld",
                         name().c_str(), 
                         time_hw,  time_sw,
                         (t - m_hw_recv_time).ticks());
    
  }
  

}
