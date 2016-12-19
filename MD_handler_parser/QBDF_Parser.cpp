// #pragma GCC optimize ("fast-math")

#include <Data/Binary_Data.h>
#include <Data/QBDF_Parser.h>
#include <Data/QBDF_Util.h>

#include <app/app.h>

#include <Product/Product_Manager.h>
#include <Exchange/Exchange_Manager.h>
#include <Logger/Logger_Manager.h>
#include <Util/CSV_Parser.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)

namespace hf_core {
  using namespace std;

  QBDF_Parser::QBDF_Parser(Application* app) :
    m_app(app),
    m_midnight(Time::today()),
    m_time(Time::today()),
    m_delay_usec(Time_Duration()),
    m_use_exch_time(false),
    m_orders_use_exch_time(true),
    m_start_file(0),
    m_end_file(0),
    m_minute_incr(1),
    m_current_file_millis_since_midnight(0),
    m_current_file_micros_since_midnight(0),
    m_quote_count(0),
    m_trade_count(0),
    m_imbalance_count(0),
    m_status_count(0),
    m_level2_count(0),
    m_non_hf_mode(0),
    m_use_summary(0),
    m_current_buffers_memory(0),
    m_blocked_on_memory(false),
    m_current_buffer(0),
    m_next_file_to_read(0),
    m_max_buffers_to_cache(1),
    m_max_reader_threads(1),
    m_exclude_flags(0),
    m_check_delta_in_millis(false)
  {
    memset(m_qbdf_exch_to_hf_exch, 0, sizeof(m_qbdf_exch_to_hf_exch));
  }


  QBDF_Parser::~QBDF_Parser()
  {
    m_buffer_readers.interrupt_all();
    m_buffer_readers.join_all();

    m_id_vector.clear();
  }


  void QBDF_Parser::read_qbdf_version_file(const string& qbdf_version_file_name, string& qbdf_version)
  {
    const char* where = "QBDF_Parser::read_qbdf_version_file";
    FILE* qbdf_version_file = fopen(qbdf_version_file_name.c_str(), "r");
    m_logger->log_printf(Log::INFO, "%s: Reading qbdf version from %s\n",
                         where, qbdf_version_file_name.c_str());
    if (qbdf_version_file == 0) {
      m_logger->log_printf(Log::DEBUG, "%s: Unable to open %s for reading, returning default\n",
                           where, qbdf_version_file_name.c_str());
      m_logger->sync_flush();
      qbdf_version = "1.0";
      return;
    }
    char input_buf[8];
    fgets(input_buf, 8, qbdf_version_file);
    qbdf_version = string(input_buf);
    if (!qbdf_version.empty() && qbdf_version[qbdf_version.length()-1] == '\n') {
      qbdf_version.erase(qbdf_version.length()-1);
    }
    fclose(qbdf_version_file);
  }

  // ----------------------------------------------------------------
  // Configure reader based on given parameters
  // ----------------------------------------------------------------
  void QBDF_Parser::init(const string& name, const Parameter& params)
  {
    m_name = name;

    const char* where = "QBDF_Parser::init";
    m_params = params;
    m_logger = m_app->lm()->get_logger(params.getDefaultString("log_file", "root"));
    
    Time unadjusted_time = Time::today();
    if (params.getDefaultBool("taq_time_gmt", false)) {
      tm today_tm;
      Time_Utils::clear_tm(today_tm);
      time_t secs = Time::currentish_time().total_sec();
      localtime_r(&secs, &today_tm);
      today_tm.tm_sec = 0;
      today_tm.tm_min = 0;
      today_tm.tm_hour = 0;
      secs = timegm(&today_tm);
      m_midnight =  Time(secs * ticks_per_second);

      m_logger->log_printf(Log::INFO, "%s %s: Normal midnight: %s, anchor midnight for taq offset: %s",
                           where, m_name.c_str(), unadjusted_time.to_string().c_str(), m_midnight.to_string().c_str());
    }
    int taq_hours_offset = params.getDefaultInt("taq_hours_offset", 0);
    if(taq_hours_offset != 0) {
      m_midnight += hours(taq_hours_offset);
    }

    m_delay_usec = usec(m_params.getDefaultInt("delay_usec", (int)m_delay_usec.total_usec()));
    if (m_delay_usec != Time_Duration()) {
      m_logger->log_printf(Log::INFO, "%s %s: Delay is set to %ldusec", where, m_name.c_str(),
			   m_delay_usec.total_usec());
    }

    m_use_exch_time = m_params.getDefaultBool("use_exch_time", false);
    m_non_hf_mode = params.getDefaultBool("non_hf_mode", false);
    m_orders_use_exch_time = params.getDefaultBool("orders_use_exch_time", m_orders_use_exch_time);

    if(params.getDefaultBool("exclude_gap_marker", false)) { m_exclude_flags |= exclude_gap_marker; }
    if(params.getDefaultBool("exclude_imbalance", false)) { m_exclude_flags |= exclude_imbalance; }
    if(params.getDefaultBool("exclude_level2", false)) { m_exclude_flags |= exclude_level2; }
    if(params.getDefaultBool("exclude_summary", false)) { m_exclude_flags |= exclude_summary; }
    if(params.getDefaultBool("exclude_quotes", false)) { m_exclude_flags |= exclude_quotes; }
    if(params.getDefaultBool("exclude_status", false)) { m_exclude_flags |= exclude_status; }
    if(params.getDefaultBool("exclude_trades", false)) { m_exclude_flags |= exclude_trades; }

    m_data_dir = params["data_dir"].getString();

    // Get QBDF version, retrieve message sizes accordingly
    read_qbdf_version_file(m_data_dir + "/qbdf_version", m_qbdf_version);
    m_qbdf_message_sizes = get_qbdf_message_sizes(m_qbdf_version);
    m_logger->log_printf(Log::INFO, "%s: Retrieved the following QBDF msg sizes using version %s:",
                         where, m_qbdf_version.c_str());
    for(size_t i = 0; i < m_qbdf_message_sizes.size(); i++) {
      size_t msg_size = m_qbdf_message_sizes[i];
      if (msg_size == 0)
        continue;

      m_logger->log_printf(Log::INFO, "%s:   msg_format=%lu msg_size=%lu", where, i, msg_size);
    }

    if (m_qbdf_version.compare("1.0") != 0) {
      m_check_delta_in_millis = true;
    }

    // load binary id to sid mapping file
    string id_file = params.getDefaultString("id_map", m_data_dir + "/ids_to_sids.csv");
    if (!load_binary_id_file(id_file)) {
      m_logger->log_printf(Log::ERROR, "%s %s: unable to load in binary id to sid mapping file %s",
                           where, m_name.c_str(), id_file.c_str());
      m_logger->sync_flush();
      return;
    }

    string exch_file = params["exch_map"].getString();
    if (!load_binary_exch_file(exch_file)) {
      m_logger->log_printf(Log::ERROR, "%s %s: unable to load in binary exchange mapping file %s",
                           where, m_name.c_str(), exch_file.c_str());
      m_logger->sync_flush();
      return;
    }

    bool use_oc_summary = m_params.getDefaultBool("use_oc_summary", false);
    m_use_summary = m_params.getDefaultBool("use_summary", false);
    m_minute_incr = 1;
    if(m_use_summary) {
      m_minute_incr = m_params.getDefaultInt("summary_increment", 1);
    }

    if(params.has("exchanges")) {
      vector<string> vec = params["exchanges"].getStringVector();
      Exchange_Manager* em = m_app->em();

      for(vector<string>::const_iterator itr = vec.begin(); itr!=vec.end(); ++itr) {
        boost::optional<ExchangeID> p=ExchangeID::get_by_name((*itr).c_str());
        if (p) {
          m_exchanges.push_back(*p);
          const Exchange& exch = em->find_by_primary(*p);
          uint8_t exch_letter = m_exch_name_to_qbdf_exch[exch.name()];
          m_supported_exchange_letters.set(exch_letter);
        } else {
          throw runtime_error("Invalid ExchangeID '" + *itr +"'");
        }
      }
    } else {
      m_supported_exchange_letters.set(); // set all exchanges to true
    }

    m_start_time = params["start_time"].getString();
    if (m_start_time.size() != 8) {
      char buf[512];
      snprintf(buf, 512, "%s %s: Invalid start_time entered [%s] use format HH:MM:SS", where, m_name.c_str(), m_start_time.c_str());
      throw runtime_error(buf);
    }

    m_end_time = params["end_time"].getString();
    if (m_end_time.size() != 8) {
      char buf[512];
      snprintf(buf, 512, "%s %s: Invalid end_time entered [%s] use format HH:MM:SS", where, m_name.c_str(), m_end_time.c_str());
      throw runtime_error(buf);
    }
    
    m_start_file=atoi(m_start_time.substr(0,2).c_str())*60 + atoi(m_start_time.substr(3,2).c_str());
    m_end_file=atoi(m_end_time.substr(0,2).c_str())*60 + atoi(m_end_time.substr(3,2).c_str());
    if(m_end_time.substr(5, 2) != "00") {
      m_end_file += 1;
    }
    
    if (m_end_file < m_start_file) {
      char buf[512];
      snprintf(buf, 512, "%s %s: end_time %s BEFORE start_time %s", where, m_name.c_str(), m_start_time.c_str(), m_end_time.c_str());
      throw runtime_error(buf);
    }

    m_next_file_to_read = m_start_file;

    // optimal default for mins to cache ahead is 20 for NAS or local disk
    m_max_buffers_to_cache = params.getDefaultInt("max_buffers_to_cache",20);

    m_max_buffers_memory = params.getDefaultInt("max_buffers_memory", 512 * 1024 * 1024);

    // optimal default for reading threads is 1 for local disk and 3 for NAS
    m_max_reader_threads = params.getDefaultInt("max_reader_threads",3);
    if (m_max_reader_threads > max_threads)
      m_max_reader_threads = max_threads;

    if(use_oc_summary) {
      m_logger->log_printf(Log::INFO, "%s %s: Open-Close Summary mode -- no buffer threads", where, m_name.c_str());
    } else {
      char all_day_filename[PATH_MAX];
      int fd = -1;

      if(!m_use_summary && params.getDefaultBool("use_allday", true)) {
        snprintf(all_day_filename, PATH_MAX, "%s/allday.bin", m_data_dir.c_str());
        fd = open(all_day_filename, 0);
      }
      if(fd < 0) {
        m_logger->log_printf(Log::INFO, "%s %s: Caching up to %d files and %zd MB ahead with %d threads",
                             where, m_name.c_str(), m_max_buffers_to_cache, m_max_buffers_memory / (1024*1024), m_max_reader_threads);

        for (int i=0; i< m_max_reader_threads; i++) {
          m_buffer_readers.create_thread(boost::bind(&QBDF_Parser::buffer_reader_loop, this));
        }
      } else {
        m_logger->log_printf(Log::INFO, "%s %s: Single-file mode caching up to %d minutes and %zd MB ahead with %d threads",
                             where, m_name.c_str(), m_max_buffers_to_cache, m_max_buffers_memory / (1024*1024), m_max_reader_threads);
        for (int i=0; i< m_max_reader_threads; i++) {
          m_buffer_readers.create_thread(boost::bind(&QBDF_Parser::single_file_reader_loop, this, all_day_filename, fd));
          if(i != m_max_reader_threads-1) {
            fd = open(all_day_filename, 0);
          }
        }
      }
    }

    m_logger->sync_flush();
  }


  string QBDF_Parser::str() const
  {
    char buf[2048];
    snprintf(buf, 2048, "QBDF_Parser '%s' dir:'%s'", m_name.c_str(), m_data_dir.c_str());
    return buf;
  }


  // ----------------------------------------------------------------
  // load in id's file that gives unique ID to each binary data symbol (for the
  // day being processed only) and then maps it to the real SID on that day
  // ----------------------------------------------------------------
  bool QBDF_Parser::load_binary_id_file(const string & id_file)
  {
    const char* where = "QBDF_Parser::load_binary_id_file";

    CSV_Parser parser(id_file);

    int bin_idx = parser.column_index("BinaryId");
    int symbol_idx = parser.column_index("Symbol");
    int sid_idx = parser.column_index("Sid");

    if (bin_idx == -1 || symbol_idx == -1 || sid_idx == -1) {
      m_logger->log_printf(Log::ERROR, "%s %s: Unable to find BinaryId, Symbol and/or Sid in file %s",
                           where, m_name.c_str(), id_file.c_str());
      m_logger->sync_flush();
      return false;
    }

    m_id_vector.reserve(10000);
    m_id_vector.push_back(QBDFIdRec(0));

    Product_Manager* pm = m_app->pm();

    while (parser.read_row() == CSV_Parser::OK) {
      size_t binaryId = strtol(parser.get_column(bin_idx), 0, 10);;
      long sid = strtol(parser.get_column(sid_idx), 0, 10);
      const char* symbol = parser.get_column(symbol_idx);
      int product_id = -1;

      if(pm) {
        const Product* prod = pm->find_by_sid(sid);
        if(prod) {
          product_id = prod->product_id();
        } else if(!m_non_hf_mode && sid > 0) {
          m_logger->log_printf(Log::DEBUG, "%s: read in unknown product SID '%lu' symbol '%s'", where, sid, symbol);
        }
      }

      // Binary Id's are in sequential order but still need to handle any that are missing from mapping file
      if(m_id_vector.size() != binaryId) {
        if(m_id_vector.size() > binaryId) {
          char buf[2048];
          snprintf(buf, 2048, "%s: While parsing '%s' found binaryId %zd smaller than current vector entry %zd",
                   where, id_file.c_str(), binaryId, m_id_vector.size());
          throw runtime_error(buf);
        } else {
          m_id_vector.resize(binaryId);
        }
      }
      
      m_id_vector.push_back(QBDFIdRec(binaryId, sid, product_id, symbol));
    }

    return true;
  }


  // ----------------------------------------------------------------
  // load in exch file that maps binary data exchange ids to product data
  // ----------------------------------------------------------------
  bool QBDF_Parser::load_binary_exch_file(const string & exch_file)
  {
    const char* where = "QBDF_Parser::load_binary_exch_file";

    CSV_Parser parser(exch_file);

    int taq_exch_char_posn = parser.column_index("taq_exch_char");
    int prim_exch_id_idx = parser.column_index("prim_exch_id");

    if (taq_exch_char_posn == -1 || prim_exch_id_idx == -1) {
      m_logger->log_printf(Log::ERROR, "%s: Unable to find taq_exch_char and/or prim_exch_id in file %s",
                           where, exch_file.c_str());
      m_logger->sync_flush();
      return false;
    }

    Exchange_Manager* em = m_app->em();
    while (parser.read_row() == CSV_Parser::OK) {
      char taq_exch_char = parser.get_column(taq_exch_char_posn)[0];
      int prim_exch_id = strtol(parser.get_column(prim_exch_id_idx), 0, 10);

      m_qbdf_exch_to_hf_exch[(int)taq_exch_char] = em->find_by_primary_id(prim_exch_id);
      if (!m_qbdf_exch_to_hf_exch[(int)taq_exch_char]) {
        if (m_non_hf_mode) {
          m_qbdf_exch_to_hf_exch[(int)taq_exch_char] = new Exchange(ExchangeID::XNYS, 99999, "BOGUS_EXCH");
        } else {
          m_logger->log_printf(Log::WARN, "%s: Unable to find HF exch for exch id %d (%c)",
                               where, prim_exch_id, taq_exch_char);
        }
      } else {
        m_logger->log_printf(Log::INFO, "%s: mapping qbdf exch [%c] to prim exch id [%d]",
                             where, taq_exch_char, prim_exch_id);
        m_exch_name_to_qbdf_exch[m_qbdf_exch_to_hf_exch[(int)taq_exch_char]->name()] = taq_exch_char;
      }
    }

    m_logger->sync_flush();
    return true;
  }


  // ----------------------------------------------------------------
  // Get the next 1 minute file, it may already be buffered!
  // ----------------------------------------------------------------
  bool QBDF_Parser::getNextBuffer()
  {
    const char where[] = "QBDF_Parser::getNextBuffer";

    boost::mutex::scoped_lock l(m_buffer_queue_mutex);
    
    if(m_current_buffer) {
      m_current_buffers_memory -= m_current_buffer->memory_footprint;
      delete m_current_buffer;
      m_current_buffer = 0;
      m_buffer_freed_condition.notify_all();
    }
    
    while(m_buffers.empty()) {
      if(m_buffer_readers.size() > 0) {
        m_logger->log_printf(Log::INFO, "%s %s: waiting for buffer object", where, m_name.c_str());
        m_buffer_ready_condition.wait(l);
      } else {
        return false;
      }
    }
    
    m_current_buffer = m_buffers.front();
    m_buffers.pop_front();
    
    m_buffer_freed_condition.notify_all();
    
    // Readers will push a NULL once they have finished reading
    if(!m_current_buffer) {
      return false;
    }
    
    // If we have to wait for the current buffer, we'll loop here
    if(!m_current_buffer->ready) {
      m_logger->log_printf(Log::INFO, "%s %s: waiting for buffer to be read", where, m_name.c_str());
      while(!m_current_buffer->ready) {
        m_buffer_ready_condition.wait(l);
      }
    }

    m_buffer_pos = m_current_buffer->buffer_start;
    m_buffer_end = m_current_buffer->buffer_end;
    m_current_file_millis_since_midnight = m_current_buffer->millis_since_midnight;
    m_current_file_micros_since_midnight = m_current_buffer->millis_since_midnight * 1000ULL;

    return true;
  }

  QBDF_Parser::buffer::buffer(Application* app, LoggerRP logger, const string& filename,
                              bool use_compression, int hh, int mm) :
    m_app(app), m_logger(logger), filename(filename), use_compression(use_compression),
    memory_footprint(0), processing_memory_footprint(0), ready(false),
    hh(hh), mm(mm), millis_since_midnight((hh*3600 + mm*60) * 1000),
    buffer_start(0), buffer_end(0)
  {
    snprintf(name, 32, "%02d:%02d", hh, mm);

    if (use_compression) {
      if (filename.compare(filename.length() - 4, 4, ".lzo") == 0) {
        m_compression_util.reset(new LZO_Util(m_app, m_logger));
      } else if (filename.compare(filename.length() - 4, 4, ".snz") == 0) {
        m_compression_util.reset(new Snappy_Util(m_app, m_logger));
      } else {
        m_compression_util.reset();
      }
    }
  }

  QBDF_Parser::buffer::~buffer()
  {
    if (!m_compression_util)
      delete [] buffer_start;
  }

  bool
  QBDF_Parser::buffer::stat_file()
  {
    const char where[] = "QBDF_Parser::buffer::stat_file";
    
    if(m_compression_util) {
      if(!m_compression_util->decompress_file_start(filename)) {
        m_logger->log_printf(Log::ERROR, "%s: Couldn't start decompression on file: [%s]", where, filename.c_str());
        return false;
      }
      if(!m_compression_util->decompress_file_get_next_block_size()) {
        m_logger->log_printf(Log::ERROR, "%s: Couldn't get decompression size on file: [%s]", where, filename.c_str());
        return false;
      }
      
      if(m_compression_util->in_buffer_len() > m_compression_util->out_buffer_len()) {
        memory_footprint = m_compression_util->in_buffer_len();
        processing_memory_footprint = 0;
      } else {
        memory_footprint = m_compression_util->out_buffer_len();
        processing_memory_footprint = m_compression_util->in_buffer_len();
      }
    } else {
      struct stat fd_stat;
      if(stat(filename.c_str(), &fd_stat)) {
        return false;
      }
      memory_footprint = fd_stat.st_size;
    }

    return true;
  }


  bool
  QBDF_Parser::buffer::read()
  {
    const char where[] = "QBDF_Parser::buffer::read";
    
    if(m_compression_util) {
      if (!m_compression_util->decompress_file_next_block(&buffer_start, &buffer_end)) {
        m_logger->log_printf(Log::ERROR, "%s: Couldn't decompress data file: [%s]", where, filename.c_str());
        return false;
      }
      m_compression_util->decompress_file_close();
    } else {
      int fd = open(filename.c_str(), O_RDONLY);
      if(fd < 0) {
        m_logger->log_printf(Log::FATAL, "%s: Unable to read file '%s'", where, filename.c_str());
        buffer_start = 0;
        return false;
      }
      
      buffer_start = new char[memory_footprint];
      if(!buffer_start) {
        m_logger->log_printf(Log::FATAL, "%s: Unable to allocate memory %zdMB to read file '%s'", where, memory_footprint / (1024*1024), filename.c_str());
        return false;
      }
      
      ssize_t items = ::read(fd, buffer_start, memory_footprint);
      if (items < 0) {
        m_logger->log_printf(Log::FATAL, "%s: Unable to read file, read() size did not match what stat() returned '%s'", where, filename.c_str());
        delete [] buffer_start;
        buffer_start = 0;
        return false;
      }
      buffer_end = buffer_start + memory_footprint;
      ::close(fd);
    }

    return true;
  }

  void QBDF_Parser::buffer_reader_loop() {
    const char* where = "QBDF_Parser::buffer_reader_loop";

    while (true) {
      boost::mutex::scoped_lock l(m_buffer_queue_mutex);

      bool looping = false;
      while(m_blocked_on_memory || (int)m_buffers.size() >= m_max_buffers_to_cache) {
        if(!looping && m_current_buffers_memory > 0) {
          const size_t MB = 1024 * 1024;
          m_logger->log_printf(Log::INFO, "%s %s: Waiting for freed buffers:  current_memory(MB): %zd   max_memory: %zd  num_buffers: %zd   max_buffers: %d",
                               where, m_name.c_str(), m_current_buffers_memory / MB, m_max_buffers_memory / MB, m_buffers.size(), m_max_buffers_to_cache);
        }
        looping = true;
        m_buffer_freed_condition.wait(l);
      }
      
      if(m_next_file_to_read >= m_end_file) {
        // end reader thread
        m_buffers.push_back(0);
        m_buffer_ready_condition.notify_all();
        break;
      }

      int hh = m_next_file_to_read / 60;
      int mm = m_next_file_to_read % 60;

      bool use_compression = !m_params.getDefaultBool("use_uncompressed", false);
      char filename[PATH_MAX];
      
      bool file_exists = false;

      if(m_use_summary) {
        use_compression = false;
        snprintf(filename, PATH_MAX, "%s/%02d%02d00.sum", m_data_dir.c_str(), hh, mm);
        file_exists = (0 == access(filename, R_OK));
      } else {
        if (use_compression) {
          snprintf(filename, PATH_MAX, "%s/%02d%02d00.bin.lzo", m_data_dir.c_str(), hh, mm);
          file_exists = (0 == access(filename, R_OK));
          
          if (!file_exists) {
            m_logger->log_printf(Log::DEBUG, "%s: %s No lzo compressed files found!", where, m_name.c_str());
            snprintf(filename, PATH_MAX, "%s/%02d%02d00.bin.snz", m_data_dir.c_str(), hh, mm);
            file_exists = (0 == access(filename, R_OK));
            if (!file_exists) {
              m_logger->log_printf(Log::DEBUG, "%s: %s No snappy compressed files found!", where, m_name.c_str());
              use_compression = false;
            }
          }
        }
        if(!use_compression) { // Can be set by failing to find compress files
          snprintf(filename, PATH_MAX, "%s/%02d%02d00.bin", m_data_dir.c_str(), hh, mm);
          file_exists = (0 == access(filename, R_OK));
          if(!file_exists) {
            m_logger->log_printf(Log::DEBUG, "%s: %s No uncompressed files found!", where, m_name.c_str());
          }
        }
      }
      
      if(!file_exists) {
        m_logger->log_printf(Log::DEBUG, "%s: %s No files found for %02d%02d00!", where, m_name.c_str(), hh, mm);
      }
      
      buffer* buf = new buffer(m_app, m_logger, filename, use_compression, hh, mm);
      m_buffers.push_back(buf);
      
      m_next_file_to_read += m_minute_incr;
      
      if(file_exists) {
        buf->stat_file();
        
        const size_t MB = 1024*1024;
        
        m_logger->log_printf(Log::INFO, "%s %s: [%s] memory_footprint %zdMB   processing_memory_footprint %zdMB", where, m_name.c_str(),
                             buf->name,  buf->memory_footprint / MB, buf->processing_memory_footprint / MB);
        
        m_current_buffers_memory += buf->memory_footprint + buf->processing_memory_footprint;
        
        // If buffers are completely empty, we're need to go ahead anyway regardless of memory
        while(!m_buffers.empty() && m_current_buffers_memory > m_max_buffers_memory) {
          const size_t MB = 1024 * 1024;
          m_logger->log_printf(Log::INFO, "%s %s: Blocked on memory:  current_memory(MB): %zd   max_memory: %zd  new buf memory footprint: %zd  new buf processing: %zd    num_buffers: %zd",
                               where, m_name.c_str(), m_current_buffers_memory / MB, m_max_buffers_memory / MB, buf->memory_footprint / MB, buf->processing_memory_footprint / MB, m_buffers.size());
          m_blocked_on_memory = true;
          m_buffer_freed_condition.wait(l);
        }
        m_blocked_on_memory = false;
        
        l.unlock();
        buf->read();
        l.lock();
      }
      
      buf->ready = true;
      m_buffer_ready_condition.notify_all();
      
      if(buf->processing_memory_footprint) {
        m_current_buffers_memory -= buf->processing_memory_footprint;
        if(m_blocked_on_memory && m_current_buffers_memory <= m_max_buffers_memory) {
          m_buffer_freed_condition.notify_all();
        }
      }
    }

    m_logger->log_printf(Log::INFO, "%s %s: ending", where, m_name.c_str());
  }

  void
  QBDF_Parser::single_file_reader_loop(string filename, int fd)
  {
    const char* where = "QBDF_Parser::single_file_reader_loop";

    boost::scoped_ptr<qbdf_allday_header> allday_header(new qbdf_allday_header);

    size_t r = read(fd, allday_header.get(), sizeof(qbdf_allday_header));
    if(r != sizeof(qbdf_allday_header)) {
      char buf[2048];
      snprintf(buf, 2048, "%s: %s Unable to read qbdf_allday_header in %s", where, m_name.c_str(), filename.c_str());
      throw runtime_error(buf);
    }

    /* Now we have the header, use it to index into the start time we wish to use */
    size_t current_location = r;

    while (true) {
      boost::mutex::scoped_lock l(m_buffer_queue_mutex);

      bool looping = false;
      while(m_blocked_on_memory || (int)m_buffers.size() >= m_max_buffers_to_cache) {
        if(!looping && m_current_buffers_memory > 0) {
          const size_t MB = 1024 * 1024;
          m_logger->log_printf(Log::INFO, "%s %s: Waiting for freed buffers:  current_memory(MB): %zd   max_memory: %zd  num_buffers: %zd   max_buffers: %d",
                               where, m_name.c_str(), m_current_buffers_memory / MB, m_max_buffers_memory / MB, m_buffers.size(), m_max_buffers_to_cache);
        }
        looping = true;
        m_buffer_freed_condition.wait(l);
      }

      if(m_next_file_to_read >= m_end_file) {
        // end reader thread
        m_buffers.push_back(0);
        m_buffer_ready_condition.notify_all();
        break;
      }

      qbdf_allday_index_item& index_elem = allday_header->index[m_next_file_to_read];
      uint8_t format = index_elem.format;
      size_t offset = index_elem.offset;
      size_t length = index_elem.length;

      int hh = m_next_file_to_read / 60;
      int mm = m_next_file_to_read % 60;

      m_next_file_to_read += m_minute_incr;

      m_logger->log_printf(Log::INFO, "%02d:%02d format=%d  offset=%zu  length=%zu", hh, mm, format, offset, length);

      if(!offset || !length) {
        continue;
      }

      if(offset != current_location) {
        m_logger->log_printf(Log::INFO, "%s: %s skipping to file location %zu", where, m_name.c_str(), offset);
        lseek64(fd, offset, SEEK_SET);
        current_location = offset;
      }

      if(format == '0') {
        buffer* buf = new buffer(m_app, m_logger, filename, /*use_lzo*/false, hh, mm);
        m_buffers.push_back(buf);
        
        buf->memory_footprint = length;
        buf->processing_memory_footprint = 0;
        
        const size_t MB = 1024*1024;
        
        m_logger->log_printf(Log::INFO, "%s %s: [%s] memory_footprint %zdMB   processing_memory_footprint %zdMB", where, m_name.c_str(),
                             buf->name,  buf->memory_footprint / MB, buf->processing_memory_footprint / MB);
        
        m_current_buffers_memory += buf->memory_footprint + buf->processing_memory_footprint;
        
        // If our buffer is the very next buffer, we're need to go ahead anyway regardless of memory
        while(!m_buffers.empty() && m_current_buffers_memory > m_max_buffers_memory) {
          m_logger->log_printf(Log::INFO, "%s %s: Blocked on memory:  current_memory(MB): %zd   max_memory: %zd  new buf memory footprint: %zd  new buf processing: %zd   num_buffers: %zd",
                               where, m_name.c_str(), m_current_buffers_memory / MB, m_max_buffers_memory / MB, buf->memory_footprint / MB, buf->processing_memory_footprint / MB, m_buffers.size());
          m_blocked_on_memory = true;
          m_buffer_freed_condition.wait(l);
        }
        m_blocked_on_memory = false;
        
        l.unlock();

        char* in_buffer = new char[length];

        r = read(fd, in_buffer, length);
        current_location += r;
        if(r != length) {
          char buf[2048];
          snprintf(buf, 2048, "%s: %s error reading block of size %zu, only read %zu bytes", where, m_name.c_str(), length, r);
          throw runtime_error(buf);
        }

        buf->buffer_start = in_buffer;
        buf->buffer_end = in_buffer + length;
        
        l.lock();
        
        buf->ready = true;
        m_buffer_ready_condition.notify_all();
        
        if(buf->processing_memory_footprint) {
          m_current_buffers_memory -= buf->processing_memory_footprint;
          if(m_blocked_on_memory && m_current_buffers_memory <= m_max_buffers_memory) {
            m_buffer_freed_condition.notify_all();
          }
        }
      } else if(format == '1') {
        struct {
          struct LZO_Header lzo_header;
          struct LZO_Block_Header block_header;
        } __attribute__((packed)) lzo_block;

        current_location += sizeof(lzo_block);
        if(sizeof(lzo_block) != read(fd, &lzo_block, sizeof(lzo_block))) {
          char buf[2048];
          snprintf(buf, 2048, "%s: %s Unable to read LZO block at offset %zd in file %s", where, m_name.c_str(), offset, filename.c_str());
          throw runtime_error(buf);
        }

        if(0 != memcmp(lzo_block.lzo_header.magic, lzo_magic, sizeof(lzo_magic))) {
          char buf[2048];
          snprintf(buf, 2048, "%s: %s file no LZO header found in file %s", where, m_name.c_str(), filename.c_str());
          throw runtime_error(buf);
        }
        
        size_t in_len = ntohl(lzo_block.block_header.in_len);
        size_t out_len = ntohl(lzo_block.block_header.out_len);
        
        // We set use_compression to false because we are not using LZO_Util
        buffer* buf = new buffer(m_app, m_logger, filename, /*use_compression*/false, hh, mm);
        m_buffers.push_back(buf);
        
        buf->memory_footprint = std::max(in_len, out_len);
        buf->processing_memory_footprint = in_len;
        
        const size_t MB = 1024*1024;
        
        m_logger->log_printf(Log::INFO, "%s %s: [%s] memory_footprint %zdMB   processing_memory_footprint %zdMB", where, m_name.c_str(),
                             buf->name,  buf->memory_footprint / MB, buf->processing_memory_footprint / MB);
        
        m_current_buffers_memory += buf->memory_footprint + buf->processing_memory_footprint;
        
        // If our buffer is the very next buffer, we're need to go ahead anyway regardless of memory
        while(!m_buffers.empty() && m_current_buffers_memory > m_max_buffers_memory) {
          m_logger->log_printf(Log::INFO, "%s %s: Blocked on memory:  current_memory(MB): %zd   max_memory: %zd  new buf memory footprint: %zd  new buf processing: %zd   num_buffers: %zd",
                               where, m_name.c_str(), m_current_buffers_memory / MB, m_max_buffers_memory / MB, buf->memory_footprint / MB, buf->processing_memory_footprint / MB, m_buffers.size());
          m_blocked_on_memory = true;
          m_buffer_freed_condition.wait(l);
        }
        m_blocked_on_memory = false;
        
        l.unlock();

        char* in_buffer = new char[in_len];

        r = read(fd, in_buffer, in_len);
        current_location += r;
        if(r != in_len) {
          char buf[2048];
          snprintf(buf, 2048, "%s: %s error reading block of size %zu, only read %zu bytes", where, m_name.c_str(), in_len, r);
          throw runtime_error(buf);
        }

        if(in_len >= out_len) {
          buf->buffer_start = in_buffer;
          buf->buffer_end = in_buffer + in_len;
        } else {
          char* out_buffer = new char[out_len];
          size_t new_len = out_len;
          int lzo_ret = lzo1x_decompress_safe((const uint8_t*)in_buffer, in_len, (uint8_t*)out_buffer, &new_len, NULL);
          delete[] in_buffer;
          in_buffer = 0;

          buf->buffer_start = out_buffer;
          buf->buffer_end = out_buffer + out_len;

          if(lzo_ret != LZO_E_OK || new_len != out_len) {
            char buf[2048];
            snprintf(buf, 2048, "%s: %s error decompressing LZO block, error code=%d", where, m_name.c_str(), lzo_ret);
            throw runtime_error(buf);
          }
        }
        
        l.lock();
        
        buf->ready = true;
        m_buffer_ready_condition.notify_all();
        
        if(buf->processing_memory_footprint) {
          m_current_buffers_memory -= buf->processing_memory_footprint;
          if(m_blocked_on_memory && m_current_buffers_memory <= m_max_buffers_memory) {
            m_buffer_freed_condition.notify_all();
          }
        }
      } else {
        char buf[2048];
        snprintf(buf, 2048, "%s: %s format %d not supported", where, m_name.c_str(), format);
        throw runtime_error(buf);
      }
    }

    close(fd);

    m_logger->log_printf(Log::INFO, "%s %s: ending", where, m_name.c_str());
  }

  bool
  QBDF_Parser::advance_buffer_pos_one_step()
  {
    const char* where = "QBDF_Parser::advance_buffer_pos";
    
    if (m_current_buffer) {
      if(m_buffer_pos && m_buffer_pos != m_buffer_end) {
        qbdf_format_header* pHdr = (qbdf_format_header*) m_buffer_pos;
        uint8_t format = pHdr->format;
        int message_size = m_qbdf_message_sizes[format];
        if(unlikely(message_size == 0)) {
          size_t offset = m_buffer_pos - m_current_buffer->buffer_start;
          size_t end = m_current_buffer->buffer_end - m_current_buffer->buffer_start;
          m_logger->log_printf(Log::FATAL, "%s %s: halting read invalid format [%d] offset [%zd] end [%zd] in file [%s]",
                               where, m_name.c_str(), format, offset, end,  m_current_buffer->filename.c_str());
          
          m_app->lm()->stdout()->log_printf(Log::FATAL, "%s %s: halting read invalid format [%d] offset [%zd] end [%zd] in file [%s]",
                                            where, m_name.c_str(), format, offset, end,  m_current_buffer->filename.c_str());
          
          return false;
        }
        
        m_buffer_pos += m_qbdf_message_sizes[format];
      }
      
      if (m_buffer_pos == m_buffer_end) {
        // This file is done ...
        if(m_buffer_pos) {
          uint64_t records_processed = m_level2_count + m_imbalance_count + m_trade_count + m_quote_count;
          m_logger->log_printf(Log::INFO, "%s: %4.1fM records, %4.1fM level2, %4.1fK imbalance, %4.1fM trades and %4.1fM quotes ",
                               m_name.c_str(), records_processed*0.000001, m_level2_count*0.000001,
                               m_imbalance_count*0.001, m_trade_count*0.000001, m_quote_count*0.000001);
        }
        m_buffer_pos = 0;
        m_buffer_end = 0;

        bool more_buffers = getNextBuffer();
        return more_buffers;
      } else if (m_buffer_pos > m_buffer_end) {
        m_logger->log_printf(Log::ERROR, "%s %s: binary buffer overrun pos=[%ld] end=[%ld] file=[%s]",
                             where, m_name.c_str(), (size_t)m_buffer_pos, (size_t)m_buffer_end, m_current_buffer->name);
        return false;
      } else {
        return true;
      }
    } else {
      bool more_buffers = getNextBuffer();
      return more_buffers;
    }
  }


  Time
  QBDF_Parser::get_next_time()
  {
    const char* where = "QBDF_Parser::get_next_time";

    if(unlikely(m_buffer_pos == 0)) {
      // File is empty
      m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight));
      return m_time;
    }

    qbdf_format_header *pHdr = (qbdf_format_header*) m_buffer_pos;

    bool skip = false;

    switch(pHdr->format) {
    case QBDFMsgFmt::QuoteSmall:
    case QBDFMsgFmt::QuoteLarge:
      {
        qbdf_quote_small* p_quote_small = (qbdf_quote_small*) m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_quote_small->qr.time));
        m_exch_time = m_time;
        skip = (m_exclude_flags & exclude_quotes) || !m_supported_exchange_letters[(uint8_t) p_quote_small->qr.exch];
      }
      break;
    case QBDFMsgFmt::Trade:
      {
        qbdf_trade *p_trade = (qbdf_trade*) m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_trade->time));
        m_exch_time = m_time;
        skip = (m_exclude_flags & exclude_trades) || !m_supported_exchange_letters[(uint8_t) p_trade->exch];
      }
      break;
    case QBDFMsgFmt::Imbalance:
      {
        qbdf_imbalance *p_imbalance = (qbdf_imbalance*) m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_imbalance->time));
        m_exch_time = m_time;
        skip = (m_exclude_flags & exclude_imbalance) || !m_supported_exchange_letters[(uint8_t) p_imbalance->exch];
      }
      break;
    case QBDFMsgFmt::Status:
      {
        qbdf_status* p_status = (qbdf_status*)m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_status->time));
        m_exch_time = m_time;
        skip = (m_exclude_flags & exclude_status) || !m_supported_exchange_letters[(uint8_t) p_status->exch];
      }
      break;
    case QBDFMsgFmt::OrderAddSmall:
    case QBDFMsgFmt::OrderAddLarge:
      {
        qbdf_order_add_small* p_order_add_small = (qbdf_order_add_small*)m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_order_add_small->pr.time));
        m_exch_time = m_time;
        skip = (m_exclude_flags & exclude_level2) || !m_supported_exchange_letters[(uint8_t) p_order_add_small->pr.exch];
      }
      break;
    case QBDFMsgFmt::OrderExec:
      {
        qbdf_order_exec* p_order_exec = (qbdf_order_exec*)m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_order_exec->time));
        m_exch_time = m_time;
        skip = (m_exclude_flags & exclude_level2) || !m_supported_exchange_letters[(uint8_t) p_order_exec->exch];
      }
      break;
    case QBDFMsgFmt::OrderReplace:
      {
        qbdf_order_replace* p_order_replace = (qbdf_order_replace*)m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_order_replace->time));
        m_exch_time = m_time;
        skip = m_exclude_flags & exclude_level2;
      }
      break;
    case QBDFMsgFmt::OrderModify:
      {
        qbdf_order_modify* p_order_modify = (qbdf_order_modify*)m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_order_modify->time));
        m_exch_time = m_time;
        skip = m_exclude_flags & exclude_level2;
      }
      break;
    case QBDFMsgFmt::GapMarker:
      {
        qbdf_gap_marker* p_gap_marker = (qbdf_gap_marker*)m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_gap_marker->time));
        m_exch_time = m_time;
        skip = m_exclude_flags & exclude_gap_marker;
      }
      break;
    case QBDFMsgFmt::Level:
      {
        qbdf_level* p_level = (qbdf_level*)m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight + p_level->time));
        m_exch_time = m_time;
        skip = (m_exclude_flags & exclude_level2) || !m_supported_exchange_letters[(uint8_t) p_level->exch];
      }
      break;
    case QBDFMsgFmt::Summary:
      {
        qbdf_summary *p_summary = (qbdf_summary*) m_buffer_pos;
        m_time = Time(m_midnight + msec(m_current_file_millis_since_midnight));
        m_exch_time = m_time;
        skip = (m_exclude_flags & exclude_summary) || !m_supported_exchange_letters[(uint8_t) p_summary->exch];
      }
      break;
    case QBDFMsgFmt::OpenCloseSummary:
      {
        /** needs special casing **/

        // This is a special case, we're going to rip through the data, and create a sorted array of open & close entries which
        // then take over the rest of the parser
        //qbdf_oc_summary *p_summary = (qbdf_oc_summary*) m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::order_add_iprice32:
      {
        qbdf_order_add_iprice32* p_order_add = (qbdf_order_add_iprice32*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_order_add->common.time));
        if (m_check_delta_in_millis && get_delta_in_millis(*p_order_add)) {
          m_exch_time = m_time + usec(p_order_add->common.exch_time_delta * 1000L);
        } else {
          m_exch_time = m_time + usec(p_order_add->common.exch_time_delta);
        }
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_level2) || !m_supported_exchange_letters[(uint8_t) p_order_add->common.exch];
      }
      break;
    case QBDFMsgFmt::order_replace_iprice32:
      {
        qbdf_order_replace_iprice32* p_order_replace = (qbdf_order_replace_iprice32*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_order_replace->common.time));
        if (m_check_delta_in_millis && get_delta_in_millis(*p_order_replace)) {
          m_exch_time = m_time + usec(p_order_replace->common.exch_time_delta * 1000L);
        } else {
          m_exch_time = m_time + usec(p_order_replace->common.exch_time_delta);
        }
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_level2) || !m_supported_exchange_letters[(uint8_t) p_order_replace->common.exch];
      }
      break;
    case QBDFMsgFmt::order_modify_iprice32:
      {
        qbdf_order_modify_iprice32* p_order_modify = (qbdf_order_modify_iprice32*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_order_modify->common.time));
        if (m_check_delta_in_millis && get_delta_in_millis(*p_order_modify)) {
          m_exch_time = m_time + usec(p_order_modify->common.exch_time_delta * 1000L);
        } else {
          m_exch_time = m_time + usec(p_order_modify->common.exch_time_delta);
        }
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_level2) || !m_supported_exchange_letters[(uint8_t) p_order_modify->common.exch];
      }
      break;
    case QBDFMsgFmt::order_exec_iprice32_cond:
      {
        qbdf_order_exec_iprice32_cond* p_order_exec = (qbdf_order_exec_iprice32_cond*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_order_exec->common.time));
        if (m_check_delta_in_millis && get_delta_in_millis(*p_order_exec)) {
          m_exch_time = m_time + usec(p_order_exec->common.exch_time_delta * 1000L);
        } else {
          m_exch_time = m_time + usec(p_order_exec->common.exch_time_delta);
        }
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_level2) || !m_supported_exchange_letters[(uint8_t) p_order_exec->common.exch];
      }
      break;
    case QBDFMsgFmt::order_exec_iprice32:
      {
        qbdf_order_exec_iprice32* p_order_exec = (qbdf_order_exec_iprice32*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_order_exec->common.time));
        if (m_check_delta_in_millis && get_delta_in_millis(*p_order_exec)) {
          m_exch_time = m_time + usec(p_order_exec->common.exch_time_delta * 1000L);
        } else {
          m_exch_time = m_time + usec(p_order_exec->common.exch_time_delta);
        }
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_level2) || !m_supported_exchange_letters[(uint8_t) p_order_exec->common.exch];
      }
      break;
    case QBDFMsgFmt::quote_small_iprice32:
      {
        qbdf_quote_small_iprice32* p_quote = (qbdf_quote_small_iprice32*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_quote->common.time));
        m_exch_time = m_time + usec(p_quote->common.exch_time_delta);
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_quotes) || !m_supported_exchange_letters[(uint8_t) p_quote->common.exch];
      }
      break;
    case QBDFMsgFmt::quote_large_iprice32:
      {
        qbdf_quote_large_iprice32* p_quote = (qbdf_quote_large_iprice32*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_quote->common.time));
        m_exch_time = m_time + usec(p_quote->common.exch_time_delta);
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_quotes) || !m_supported_exchange_letters[(uint8_t) p_quote->common.exch];
      }
      break;
    case QBDFMsgFmt::quote_iprice64:
      {
        qbdf_quote_iprice64* p_quote = (qbdf_quote_iprice64*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_quote->common.time));
        m_exch_time = m_time + usec(p_quote->common.exch_time_delta);
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_quotes) || !m_supported_exchange_letters[(uint8_t) p_quote->common.exch];
      }
      break;
    case QBDFMsgFmt::trade_iprice32:
      {
        qbdf_trade_iprice32* p_trade = (qbdf_trade_iprice32*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_trade->common.time));
        m_exch_time = m_time + usec(p_trade->common.exch_time_delta);
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_trades) || !m_supported_exchange_letters[(uint8_t) p_trade->common.exch];
      }
      break;
    case QBDFMsgFmt::trade_iprice64:
      {
        qbdf_trade_iprice64* p_trade = (qbdf_trade_iprice64*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_trade->common.time));
        m_exch_time = m_time + usec(p_trade->common.exch_time_delta);
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_trades) || !m_supported_exchange_letters[(uint8_t) p_trade->common.exch];
      }
      break;
    case QBDFMsgFmt::trade_augmented:
      {
        qbdf_trade_augmented* p_trade = (qbdf_trade_augmented*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_trade->common.time));
        m_exch_time = m_time + usec(p_trade->common.exch_time_delta);
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_trades) || !m_supported_exchange_letters[(uint8_t) p_trade->common.exch];
      }
      break;
    case QBDFMsgFmt::imbalance_iprice32:
      {
        qbdf_imbalance_iprice32* p_imbalance = (qbdf_imbalance_iprice32*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_imbalance->common.time));
        m_exch_time = m_time + usec(p_imbalance->common.exch_time_delta);
        if (m_check_delta_in_millis && get_delta_in_millis(*p_imbalance)) {
          m_exch_time = m_time + usec(p_imbalance->common.exch_time_delta * 1000L);
        } else {
          m_exch_time = m_time + usec(p_imbalance->common.exch_time_delta);
        }
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_imbalance) || !m_supported_exchange_letters[(uint8_t) p_imbalance->common.exch];
      }
      break;
    case QBDFMsgFmt::imbalance_iprice64:
      {
        qbdf_imbalance_iprice64* p_imbalance = (qbdf_imbalance_iprice64*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_imbalance->common.time));
        m_exch_time = m_time + usec(p_imbalance->common.exch_time_delta);
        if (m_check_delta_in_millis && get_delta_in_millis(*p_imbalance)) {
          m_exch_time = m_time + usec(p_imbalance->common.exch_time_delta * 1000L);
        } else {
          m_exch_time = m_time + usec(p_imbalance->common.exch_time_delta);
        }
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_imbalance) || !m_supported_exchange_letters[(uint8_t) p_imbalance->common.exch];
      }
      break;
    case QBDFMsgFmt::status_micros:
      {
        qbdf_status_micros* p_status = (qbdf_status_micros*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_status->common.time));
        m_exch_time = m_time + usec(p_status->common.exch_time_delta);
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = (m_exclude_flags & exclude_status) || !m_supported_exchange_letters[(uint8_t) p_status->common.exch];
      }
      break;
    case QBDFMsgFmt::gap_marker_micros:
      {
        qbdf_gap_marker_micros* p_gap_marker = (qbdf_gap_marker_micros*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_gap_marker->common.time));
        m_exch_time = m_time + usec(p_gap_marker->common.exch_time_delta);
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = m_exclude_flags & exclude_gap_marker;
      }
      break;
    case QBDFMsgFmt::level_iprice32:
      {
        qbdf_level_iprice32* p_level = (qbdf_level_iprice32*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_level->common.time));
        if (m_check_delta_in_millis && get_delta_in_millis(*p_level)) {
          m_exch_time = m_time + usec(p_level->common.exch_time_delta * 1000L);
        } else {
          m_exch_time = m_time + usec(p_level->common.exch_time_delta);
        }
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = m_exclude_flags & exclude_level2;
      }
      break;
    case QBDFMsgFmt::level_iprice64:
      {
        qbdf_level_iprice64* p_level = (qbdf_level_iprice64*)m_buffer_pos;
        m_time = Time(m_midnight + usec(m_current_file_micros_since_midnight + p_level->common.time));
        if (m_check_delta_in_millis && get_delta_in_millis(*p_level)) {
          m_exch_time = m_time + usec(p_level->common.exch_time_delta * 1000L);
        } else {
          m_exch_time = m_time + usec(p_level->common.exch_time_delta);
        }
        m_exch_time = m_time + usec(p_level->common.exch_time_delta);
        if(m_use_exch_time) { m_time = m_exch_time; }
        skip = m_exclude_flags & exclude_level2;
      }
      break;
    default:
      {
        skip = true;
        uint8_t format = pHdr->format;
        size_t offset = m_buffer_pos - m_current_buffer->buffer_start;
        size_t end = m_current_buffer->buffer_end - m_current_buffer->buffer_start;

        char buf[512];
        snprintf(buf, 512,  "%s %s: invalid format [%d] offset [%zd] end [%zd] in file [%s]",
                 where, m_name.c_str(), format, offset, end,  m_current_buffer->filename.c_str());

        m_logger->log_printf(Log::ERROR, "%s", buf);
        //throw runtime_error(buf);
        // A case we don't know about
      }
    }

    m_time += m_delay_usec;

    if(m_orders_use_exch_time) {
      m_order_time = m_exch_time;
    } else {
      m_order_time = m_time;
    }

    if(likely(!skip)) {
      return m_time;
    } else {
      return Time();
    }
  }

  bool QBDF_Parser::read_oc_summary_file()
  {
    const char* where = "QBDF_Parser::read_oc_summary_file";
    char filename[PATH_MAX];

    snprintf(filename, PATH_MAX, "%s/open_close.sum", m_data_dir.c_str());

    m_current_buffer = new buffer(m_app, m_logger, filename, false, 00, 00);

    if(!m_current_buffer->stat_file()) {
      m_logger->log_printf(Log::FATAL, "%s %s: Unable to stat file '%s'", where, m_name.c_str(), filename);
      return false;
    }

    if(!m_current_buffer->read()) {
      m_logger->log_printf(Log::FATAL, "%s %s: Unable to read file '%s'", where, m_name.c_str(), filename);
      return false;
    }
    
    m_buffer_pos = m_current_buffer->buffer_start;
    m_buffer_end = m_current_buffer->buffer_end;
    
    return true;
  }


}
