#ifndef hf_Data_QBDF_Parser_H
#define hf_Data_QBDF_Parser_H

#include <Data/Binary_Data.h>
#include <Interface/QuoteUpdate.h>
#include <Exchange/Exchange.h>
#include <Exchange/Feed_Rules.h>
#include <Product/Product.h>
#include <Logger/Logger.h>
#include <Util/Parameter.h>


#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <Data/LZO_Util.h>
#include <Data/Snappy_Util.h>

#include <map>
#include <bitset>

namespace hf_core {
  using namespace std;

  struct QBDFIdRec {
    int binaryId;
    long sid;
    ProductID product_id;
    string symbol;

    QBDFIdRec() : binaryId(0), sid(0), product_id(-1) {}
    QBDFIdRec(int id) : binaryId(id), sid(0), product_id(-1) {}
    QBDFIdRec(int id, long sid, int product_id, const string& symbol) : binaryId(id), sid(sid), product_id(product_id), symbol(symbol) { }
  };

  static const int max_threads=20;
  
  class QBDF_Reader_Buffer {

  };
  
  class QBDF_Reader_Generator {
  public:
    virtual QBDF_Reader_Buffer * get_buffer(int hhmm) = 0;
    virtual ~QBDF_Reader_Generator() {}
  };
  
  
  class QBDF_Multi_File_Reader_Generator : public QBDF_Reader_Generator {
  public:
    
    
  };
  
  class QBDF_Single_File_Reader_Generator : public QBDF_Reader_Generator {
  public:
    
    
  };
  
  
  class QBDF_Parser {
  public:
    QBDF_Parser(Application* app);
    virtual ~QBDF_Parser();

    void init(const string& name, const Parameter& params);
    bool advance_buffer_pos_one_step();
    Time get_next_time();

    bool read_oc_summary_file();

    inline QBDFIdRec* getMappingByBinaryId(int id_) { return &m_id_vector[id_]; }

    string str() const;

    inline Time time() const { return m_time; };

    static const uint8_t exclude_gap_marker = 1;
    static const uint8_t exclude_imbalance = 2;
    static const uint8_t exclude_level2 = 4;
    static const uint8_t exclude_quotes = 8;
    static const uint8_t exclude_summary = 16;
    static const uint8_t exclude_status = 32;
    static const uint8_t exclude_trades = 64;

    struct buffer {
      Application* m_app;
      LoggerRP     m_logger;
      Compression_Util_RP m_compression_util;

      char   name[32];

      string filename;
      bool   use_compression;
      size_t memory_footprint;
      size_t processing_memory_footprint;
      volatile bool ready;

      int   hh;
      int   mm;
      int   millis_since_midnight;

      int   buffer_time() { return (hh*100) + mm; }

      char* buffer_start;
      char* buffer_end;

      buffer(Application* app, LoggerRP logger, const string& filename, bool use_compression, int hh, int mm);
      ~buffer();

      bool stat_file();
      bool read();
    };

  protected:
    bool load_binary_id_file(const string & id_file);
    bool load_binary_exch_file(const string & exch_file);
    bool getNextBuffer();
    void buffer_reader_loop();
    void single_file_reader_loop(string filename, int fd);
    void read_qbdf_version_file(const string& qbdf_version_file_name, string& qbdf_version);

  public:
    Application* m_app;
    Parameter m_params;

    string   m_name;
    LoggerRP m_logger;

    string m_data_dir;
    string m_start_time;
    string m_end_time;
    Time m_midnight;
    Time m_time;
    Time m_exch_time;
    Time m_order_time;
    Time_Duration m_delay_usec;
    bool m_use_exch_time;
    bool m_orders_use_exch_time;
    string m_qbdf_version;

    int m_start_file;
    int m_end_file;
    int m_minute_incr;
    int m_current_file_millis_since_midnight;
    uint64_t m_current_file_micros_since_midnight;

    int m_quote_count;
    int m_trade_count;
    int m_imbalance_count;
    int m_status_count;
    int m_level2_count;

    bool m_non_hf_mode;
    bool m_use_summary;

    boost::mutex        m_buffer_queue_mutex;
    /**  The following variables are synchronized by the above mutex */
    boost::condition    m_buffer_ready_condition;
    boost::condition    m_buffer_freed_condition;
    size_t              m_current_buffers_memory;
    bool                m_blocked_on_memory;
    
    buffer*             m_current_buffer;
    std::deque<buffer*> m_buffers;

    int m_next_file_to_read;

    /** end syncronized variables */

    boost::thread_group m_buffer_readers;

    size_t m_max_buffers_memory;
    int m_max_buffers_to_cache;
    int m_max_reader_threads;

    char *m_buffer_pos;
    char *m_buffer_end;
    vector<QBDFIdRec> m_id_vector;
    const Exchange* m_qbdf_exch_to_hf_exch[256];
    vector<uint32_t> m_qbdf_message_sizes;

    uint8_t       m_exclude_flags;
    bitset<128>   m_supported_exchange_letters;

    vector<ExchangeID> m_exchanges;
    map<string,char>   m_exch_name_to_qbdf_exch;

    bool m_check_delta_in_millis;
  };


}

#endif // hf_Data_QBDF_Parser_H
