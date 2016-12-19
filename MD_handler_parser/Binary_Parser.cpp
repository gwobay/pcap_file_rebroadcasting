#include <Data/Binary_Data.h>
#include <Data/Binary_Parser.h>

#include <app/app.h>

#include <Product/Product_Manager.h>
#include <Exchange/Exchange_Manager.h>
#include <Logger/Logger_Manager.h>
#include <Util/CSV_Parser.h>

#include <Interface/QuoteUpdateSubscribe.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace hf_core {
  using namespace std;

  Binary_Parser::Binary_Parser(Application* app) :
    m_app(app),
    m_pm(app->pm()),
    m_em(app->em()),
    m_next_event_type(Event_Type::QuoteUpdate),
    m_cache(0),
    m_data_dir(""),
    m_start_time("09:30:00"),
    m_end_time("16:01:30"),
    m_midnight(Time::today()),
    m_log_updates_for_sid(0),
    m_shared_next_file(-1),
    m_max_buffers_to_cache(1),
    m_max_reader_threads(1),
    m_current_buffer(-1),
    m_shared_current_buffer(-1),
    m_current_binary_buffer(NULL),
    m_pending_event(false)
  {
    for (int i=0; i<2400; i++)
      m_shared_buffers[i] = NULL;
  }


  Binary_Parser::~Binary_Parser()
  {
    for (vector<BinaryIdRec*>::iterator iter = m_id_vector.begin(); iter !=  m_id_vector.end(); iter++) {
      BinaryIdRec * p = *iter;
      if (p) {
        for (int i=0; i< 256; i++) {
          if (p->cached_by_exch[i])
            delete p->cached_by_exch[i];
        }
        delete p;
      }
    }
    m_id_vector.clear();

    for (int i=0; i<m_max_reader_threads; i++) {
      m_buffer_readers[i].interrupt();
      m_buffer_readers[i].join();
    }

    for (int i=0; i<2400; i++) {
      if (m_shared_buffers[i]) {
	m_shared_buffers[i]->close();
	delete m_shared_buffers[i];
	m_shared_buffers[i]=NULL;
      }
    }
  }


  string Binary_Parser::str() const
  {
    char buf[2048];
    snprintf(buf, 2048, "Binary_Parser dir:'%s'",m_data_dir.c_str());
    return buf;
  }


  // ----------------------------------------------------------------
  // load in id's file that gives unique ID to each binary data symbol (for the
  // day being processed only) and then maps it to the real SID on that day
  // ----------------------------------------------------------------
  bool Binary_Parser::load_binary_id_file(const string & id_file)
  {
    const char* where = "Binary_Parser::load_binary_id_file";

    CSV_Parser parser(id_file);

    int bin_idx = parser.column_index("BinaryId");
    int symbol_idx = parser.column_index("Symbol");
    int sid_idx = parser.column_index("Sid");

    if (bin_idx == -1 || symbol_idx == -1 || sid_idx == -1) {
      m_logger->log_printf(Log::ERROR, "%s: unable to find required columns BinaryId, Symbol and Sid in file %s", where, id_file.c_str());
      return false;
    }

    m_id_vector.push_back(0);

    int lineNo = 1;
    CSV_Parser::state_t s = parser.read_row();
    while (s == CSV_Parser::OK) {
      BinaryIdRec *pRec = new BinaryIdRec();
      pRec->binaryId = strtol(parser.get_column(bin_idx), 0, 10);;
      pRec->sid = strtol(parser.get_column(sid_idx), 0, 10);
      pRec->symbol = string(parser.get_column(symbol_idx));
      for (int i=0; i<256; i++)
        pRec->cached_by_exch[i] = NULL;

      Product* prod = m_pm->find_by_sid(pRec->sid);
      if(!prod) {
        if (pRec->sid > 0)
          m_logger->log_printf(Log::WARN, "%s: read in unknown product SID '%d' symbol '%s'", where, pRec->sid,pRec->symbol.c_str());
        pRec->product_id = -1;
      } else {
        pRec->product_id = prod->product_id();
      }

      lineNo++;

      // Binary Id's are in sequential order but still need to handle any that are missing from mapping file
      for (int i=lineNo-1; i<pRec->binaryId; i++ )
        m_id_vector.push_back(NULL);

      m_id_vector.push_back(pRec);

      pair<map<int, BinaryIdRec*>::iterator, bool> id_result = m_id_map.insert(make_pair(pRec->binaryId,pRec));
      if (id_result.second == false) {
        m_logger->log_printf(Log::WARN, "%s: skipping duplicate TaqID [%d] at line %d of mapping file %s", where,pRec->binaryId,lineNo, id_file.c_str());
      }

      pair<map<string, BinaryIdRec*>::iterator, bool> symbol_result = m_symbol_map.insert(make_pair(pRec->symbol,pRec));
      if (symbol_result.second == false) {
        m_logger->log_printf(Log::WARN, "%s: skipping duplicate symbol [%s] at line %d of mapping file %s", where,pRec->symbol.c_str(),lineNo,id_file.c_str());
      }

      if (pRec->sid != 0) {
        pair<map<long, BinaryIdRec*>::iterator, bool> sid_result = m_sid_map.insert(make_pair(pRec->sid,pRec));
        if (sid_result.second == false) {
          m_logger->log_printf(Log::WARN, "%s: skipping duplicate SID [%ld] at line %d of mapping file %s", where,pRec->sid,lineNo,id_file.c_str());
        }
      }

      s = parser.read_row();
    }
    
    return true;
  }


  // ----------------------------------------------------------------
  // load in exch file that maps binary data exchange ids to product data
  // ----------------------------------------------------------------
  bool Binary_Parser::load_binary_exch_file(const string & exch_file)
  {
    const char* where = "Binary_Parser::load_binary_exch_file";

    CSV_Parser parser(exch_file);

    int bin_idx = parser.column_index("taq_exch_char");
    if (bin_idx == -1) {
      int bin_idx = parser.column_index("nyse_exch_char");
      if (bin_idx == -1) {
        bin_idx = parser.column_index("rdf_exch_char");
      }
    }

    int primary_exch_id_idx = parser.column_index("prim_exch_id");

    if (bin_idx == -1 || primary_exch_id_idx == -1) {
      m_logger->log_printf(Log::ERROR, "%s: unable to find required columns nyse_exch_char and prim_exch_id in file %s", where, exch_file.c_str());
      return false;
    }

    CSV_Parser::state_t s = parser.read_row();
    while (s == CSV_Parser::OK) {
      int id = (int) parser.get_column(bin_idx)[0];
      int exch = strtol(parser.get_column(primary_exch_id_idx), 0, 10);

      m_binary_exch_to_primary_exch[id] = m_em->find_by_primary_id(exch);
      if (!m_binary_exch_to_primary_exch[id]) {
        m_logger->log_printf(Log::ERROR, "%s: mapping binary exch id [%c] to primary exch id [%d] BUT cannot find that in ExchangeManavger",
                               where, (char) id , exch);
      } else {
        m_logger->log_printf(Log::INFO, "%s: mapping binary exch id [%c] to primary exch id [%d]", where, (char) id , exch);
      }
      s = parser.read_row();
    }
    
    return true;
  }


  // ----------------------------------------------------------------
  // Return the most recent event created by last read_row
  // ----------------------------------------------------------------
  ResponseEvent * Binary_Parser::get_event()
  {
    static const char* where = "Binary_Parser::get_event";
    if (!m_cache)
      return NULL;

    if (m_next_event_type == Event_Type::QuoteUpdate) {
      return &m_cache->m_quote;
    } else if (m_next_event_type == Event_Type::TradeUpdate) {
      return &m_cache->m_trade;
    } else if (m_next_event_type == Event_Type::AuctionUpdate) {
      return &m_cache->m_auction;
    } else if (m_next_event_type == Event_Type::MarketStatusUpdate) {
      return &m_cache->m_market_status;
    } else {
      m_logger->log_printf(Log::ERROR, "%s: invalid event type %s",
                           where, m_next_event_type.str());
    }
    return NULL;
  }


  // ----------------------------------------------------------------
  // Get the next 1 minute file, it may already be buffered!
  // ----------------------------------------------------------------
  bool Binary_Parser::getNextBuffer()
  {
    //const char where[] = "Binary_Parser::getNextBuffer";
    if (m_current_buffer != -1) {

      //m_logger->log_printf(Log::DEBUG, "%s: Waiting for shared_buffers[%d] lock", where, m_current_buffer);
      boost::mutex::scoped_lock lock(m_shared_buffers_mutex[m_current_buffer]);
      //m_logger->log_printf(Log::DEBUG, "%s: Got shared_buffers[%d] lock", where, m_current_buffer);

      // If we've finished with this buffer, then close it
      if (m_shared_buffers[m_current_buffer]) {
	m_shared_buffers_cond[m_current_buffer].notify_one();
	m_shared_buffers[m_current_buffer]->close();
	delete m_shared_buffers[m_current_buffer];
	m_shared_buffers[m_current_buffer]=NULL;
      }
    }

    // Now look for the next file/buffer
    m_current_buffer++;
    if (m_current_buffer < m_start_file)
      m_current_buffer = m_start_file;

    int hh = m_current_buffer / 100;
    int mm = m_current_buffer % 100;
    if (mm == 60) {
      mm = 0;
      hh++;
      m_current_buffer = 100 * hh;
    }

    if (m_current_buffer > m_end_file) {
      // we're done ...
      return false;
    }

    // Inform reader threads which buffer we're currently on
    {
      //m_logger->log_printf(Log::DEBUG, "%s: Waiting for current_buffer lock", where);
      boost::mutex::scoped_lock lock(m_shared_current_buffer_mutex);
      //m_logger->log_printf(Log::DEBUG, "%s: Got current_buffer lock",  where);
      m_shared_current_buffer = m_current_buffer;
      m_shared_current_buffer_cond.notify_all();
    }

    //m_logger->log_printf(Log::DEBUG, "%s: moving to buffer %d", where, m_current_buffer);

    {
      //m_logger->log_printf(Log::DEBUG, "%s: Waiting for shared_buffers[%d] lock", where, m_current_buffer);
      boost::mutex::scoped_lock lock(m_shared_buffers_mutex[m_current_buffer]);
      //m_logger->log_printf(Log::DEBUG, "%s: Got shared_buffers[%d] lock", where, m_current_buffer);
      while (m_shared_buffers[m_current_buffer] == 0) {
	//m_logger->log_printf(Log::DEBUG, "%s: Waiting for shared_buffers[%d]==0", where, m_current_buffer);
	m_shared_buffers_cond[m_current_buffer].wait(lock);
	//m_logger->log_printf(Log::DEBUG, "%s: Got shared_buffers[%d]!=0", where, m_current_buffer);
      }
    }

    if (!m_shared_buffers[m_current_buffer]->buffer_start) {
      // The buffer is empty, file must have been skipped, move on to the next
      //m_logger->log_printf(Log::INFO, "%s: Skipping empty buffer for [%04d]", where, m_current_buffer);
      return getNextBuffer();
    }

    m_current_binary_buffer = m_shared_buffers[m_current_buffer]->buffer_start;
    m_buffer_pos = m_current_binary_buffer;
    m_buffer_end = m_shared_buffers[m_current_buffer]->buffer_end;
    m_current_file_millis_since_midnight = m_shared_buffers[m_current_buffer]->millis_since_midnight;

    return true;
  }


  bool
  Binary_Parser::buffer::read(const char* filename, int hh, int mm)
  {
    file = hh*100 + mm;
    millis_since_midnight = (hh*3600 + mm*60) * 1000;

    fp = fopen(filename, "r");
    if (!fp) {
      return false;
    }
    fd = fileno(fp);
    struct stat fd_stat;
    fstat(fd, &fd_stat);
    buffer_start =  new char[fd_stat.st_size];
    if(!buffer_start) {
      return false;
    }
    // Note, moved away from mmap due to poor performance against the NAS
    size_t items = fread(buffer_start,fd_stat.st_size,1,fp);
    if (items != 1) {
      return false;
    }
    buffer_end = buffer_start + fd_stat.st_size;

    return true;
  }

  Binary_Parser::buffer::buffer() : fd(-1), buffer_start(0), buffer_end(0), millis_since_midnight(0) {}

  void Binary_Parser::buffer::close() {
    if(buffer_start) {
      delete [] buffer_start;

      buffer_start = 0;
      buffer_end = 0;
      ::fclose(fp);
      fd = -1;
    }
  }

  void Binary_Parser::buffer_reader() {
    const char where[] = "Binary_Parser::buffer_reader";
    bool done=false;
    int id=0;

    while (!done) {

      // Get next buffer to read
      int current_file=0;
      {
	//m_logger->log_printf(Log::DEBUG, "%s: %d Waiting for shared_next_file lock [%d]", where, id, m_shared_next_file);
	boost::mutex::scoped_lock lock(m_shared_next_file_mutex);
	//m_logger->log_printf(Log::DEBUG, "%s: %d Got shared_next_file lock [%d]", where, id, m_shared_next_file);

	if (m_shared_next_file > m_end_file) {
	  done=true;
	  continue;
	}

	if (id==0)
	  id=m_shared_next_file;


	// this is the file we will now read
	current_file = m_shared_next_file;

	// Increment shared_next_file, for next available thread
 	m_shared_next_file++;
	int hh = m_shared_next_file / 100;
	int mm = m_shared_next_file % 100;
	if (mm == 60) {
	  mm = 0;
	  hh++;
	  m_shared_next_file = 100 * hh;
	}
      }

      int hh = current_file / 100;
      int mm = current_file % 100;

      if (hh > 23 || hh < 0 || mm < 0 || mm > 59) {
	m_logger->log_printf(Log::ERROR, "%s: %d invalid file time=[%d] HH=[%d] MM=[%d]", where, id, current_file, hh, mm);
	done=true;
	continue;
      }

      if (current_file > m_end_file) {
	done=true;
	continue;
      }

      char filename[PATH_MAX];
      snprintf(filename, PATH_MAX, "%s/%02d%02d00.bin", m_data_dir.c_str(), hh, mm);

      buffer *buff = new buffer();
      if(!buff->read(filename, hh, mm)) {
	m_logger->log_printf(Log::INFO, "%s: %d Skipping empty or missing raw binary file [%s]", where, id, filename);
      }
      //else {
      //m_logger->log_printf(Log::INFO, "%s: %d Read in buffer %d from binary file [%s]", where, id, current_file, filename);
      //}

      // Let main process know this buffer is now available
      {
	//m_logger->log_printf(Log::DEBUG, "%s: %d Waiting for shared_buffers[%d] lock", where, id, current_file);
	boost::mutex::scoped_lock lock(m_shared_buffers_mutex[current_file]);
	//m_logger->log_printf(Log::DEBUG, "%s: %d Got shared_buffers[%d] lock",  where, id, current_file);

	m_shared_buffers[current_file] = buff;
	m_shared_buffers_cond[current_file].notify_one();
      }

      // See if we should read another buffer now or wait
      int target = int_add_minutes(current_file, -m_max_buffers_to_cache);

      //m_logger->log_printf(Log::DEBUG, "%s: %d Waiting for current_buffer lock", where, id);
      boost::mutex::scoped_lock lock(m_shared_current_buffer_mutex);
      //m_logger->log_printf(Log::DEBUG, "%s: %d Got current_buffer lock",  where, id);

      //m_logger->log_printf(Log::DEBUG, "%s: %d Waiting for current_buffer > %d, currently at %d", where, id, target, m_shared_current_buffer);
      while (m_shared_current_buffer <= target)
	m_shared_current_buffer_cond.wait(lock);

      //m_logger->log_printf(Log::DEBUG, "%s: %d Done waiting for current_buffer > %d, currently at %d", where, id, target, m_shared_current_buffer);
    }

  }

  int Binary_Parser::int_add_minutes(int start_hhmm, int add_mins)
  {
    int mins = (start_hhmm / 100)*60 + (start_hhmm % 100) + (add_mins / 60)*60 + (add_mins % 60);
    int hh = mins / 60;
    int mm = mins % 60;
    return (100*hh + mm);
  }


}
