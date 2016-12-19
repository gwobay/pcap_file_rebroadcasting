#ifndef hf_Data_Binary_Parser_H
#define hf_Data_Binary_Parser_H

#include <Simulator/Event_Sequence.h>
#include <MD/MDSubscriptionAcceptor.h>
#include <Interface/QuoteUpdate.h>
#include <Util/Parameter.h>
#include <Logger/Logger.h>
#include <Logger/Logger_Manager.h>
#include <Product/Product.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <map>

namespace hf_core {
  using namespace std;

  struct CacheRec {
    QuoteUpdate m_quote;
    TradeUpdate m_trade;
    AuctionUpdate m_auction;
    MarketStatusUpdate m_market_status;
    unsigned long long m_cum_clean_volume;
    int m_clean_trade_count;
    unsigned long long m_cum_volume;
    int m_trade_count;
    CacheRec() :  m_cum_clean_volume(0), m_clean_trade_count(0), m_cum_volume(0), m_trade_count(0) { ; }
  };

  typedef struct {
          int binaryId;
          long sid;
          string symbol;
          ProductID product_id;
          CacheRec *cached_by_exch[256];
  } BinaryIdRec;


  static const int max_thread_limit=20;


  class Binary_Parser {
  public:
    Binary_Parser(Application* app);
    ~Binary_Parser();
    virtual void init(const Parameter& params) = 0;
    virtual bool read_row() = 0;
    ResponseEvent* get_event();

    struct buffer {
      int   fd;
      FILE *fp;
      char *buffer_start;
      char *buffer_end;
      int   file;
      int   millis_since_midnight;

      buffer();
      bool read(const char* filename, int hh, int mm);
      void close();
      ~buffer() { close(); }
    };

  protected:
    bool load_binary_id_file(const string & id_file);
    bool load_binary_exch_file(const string & exch_file);
    bool getNextBuffer();
    void buffer_reader();
    string str() const;
    BinaryIdRec* getMappingByBinaryId(int id_) { return m_id_vector[id_]; }
    int int_add_minutes(int start_hhmm, int add_mins);

    Application* m_app;
    Product_Manager* m_pm;
    Exchange_Manager* m_em;

    LoggerRP m_logger;

    Event_Type m_next_event_type;

    CacheRec *m_cache;

    string m_data_dir;
    string m_start_time;
    string m_end_time;
    Time m_midnight;
    Time m_time;

    map<string, BinaryIdRec*> m_symbol_map;
    map<int, BinaryIdRec*> m_id_map;
    map<long, BinaryIdRec*> m_sid_map;
    vector<BinaryIdRec*> m_id_vector;

    int m_start_hhmmssii;
    int m_end_hhmmssiii;
    int m_start_file;
    int m_end_file;
    int m_current_file;
    int m_current_file_millis_since_midnight;

    int m_quotes_processed;
    int m_trades_processed;
    int m_indicatives_processed;
    int m_log_updates_for_sid;

    boost::mutex m_shared_next_file_mutex;
    boost::mutex m_shared_buffers_mutex[2400];
    boost::condition m_shared_buffers_cond[2400];
    buffer* m_shared_buffers[2400];
    boost::mutex m_shared_current_buffer_mutex;
    boost::condition m_shared_current_buffer_cond;

    int m_shared_next_file;
    boost::thread m_buffer_readers[max_thread_limit];

    int m_max_buffers_to_cache;
    int m_max_reader_threads;

    int    m_current_buffer;
    int m_shared_current_buffer;

    char  *m_current_binary_buffer;
    char  *m_buffer_pos;
    char  *m_buffer_end;
    const Exchange* m_binary_exch_to_primary_exch[256];

    bool m_pending_event;
    Event_Type m_pending_event_type;


  };


}

#endif // hf_Data_Binary_Parser_H
