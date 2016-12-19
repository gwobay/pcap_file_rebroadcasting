#include <math.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>



#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>


#include <ASIO/IO_Service.h>
#include <Logger/Logger_Manager.h>
#include "NYSE_TAQ_99_QBDF_Builder.h"
#include <MD/MDManager.h>
#include <Util/File_Utils.h>
#include <Util/Network_Utils.h>
#include <Util/Parameter.h>
#include <FIX/FIX_Message.h>
#include <Data/QBDF_Util.h>


namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  using namespace boost::iostreams;


  class NYSE_TAQ_99_Quote_Rec_Event_Sequence : public Event_Sequence
  {
  public:
    NYSE_TAQ_99_Quote_Rec_Event_Sequence(string symbol, int n_messages, quote_message* quote_messages, Time date, NYSE_TAQ_99_QBDF_Builder* qbdf_builder)
      : m_symbol(symbol), m_n_messages(n_messages), m_quote_messages(quote_messages), m_current_offset(0), m_date(date), m_qbdf_builder(qbdf_builder)
    {

      quote_message& qmsg = m_quote_messages[m_current_offset];
      m_time = m_date + seconds(qmsg.qtim);
    }

    virtual string str() const
    {
      return "";
    }

    bool process_current_message()
    {
      return true;
    }

    virtual bool next(Event_Hub& hub)
    {
      quote_message& qmsg = m_quote_messages[m_current_offset];

      //Time exch_time = m_date + seconds(qmsg.qtim);
      m_qbdf_builder->handle_quote(qmsg, m_symbol);
      if(m_current_offset >= (m_n_messages - 1)) { return false; }
      ++m_current_offset;
      m_time = m_date + seconds((m_quote_messages[m_current_offset]).qtim);
      return true;
    }

    virtual Event* event()
    {
      return NULL;
    }

    INTRUSIVE_IMPL;

  private:
    string m_symbol;
    int m_n_messages;
    quote_message* m_quote_messages;
    int m_current_offset;
    Time m_date;
    NYSE_TAQ_99_QBDF_Builder* m_qbdf_builder;
    hf_tools::atomic_counter m_intrusive_ptr_base_counter;
  };

  class NYSE_TAQ_99_Trade_Rec_Event_Sequence : public Event_Sequence
  {
  public:
    NYSE_TAQ_99_Trade_Rec_Event_Sequence(string symbol, int n_messages, trade_message* trade_messages, Time date, NYSE_TAQ_99_QBDF_Builder* qbdf_builder)
      : m_symbol(symbol), m_n_messages(n_messages), m_trade_messages(trade_messages), m_current_offset(0), m_date(date), m_qbdf_builder(qbdf_builder)
    {

      trade_message& tmsg = m_trade_messages[m_current_offset];
      m_time = m_date + seconds(tmsg.ttim);
    }

    virtual string str() const
    {
      return "";
    }

    bool process_current_message()
    {
      return true;
    }

    virtual bool next(Event_Hub& hub)
    {
      trade_message& tmsg = m_trade_messages[m_current_offset];

      //Time exch_time = m_date + seconds(tmsg.ttim);
      m_qbdf_builder->handle_trade(tmsg, m_symbol);
      if(m_current_offset >= (m_n_messages - 1)) { return false; }
      ++m_current_offset;
      m_time = m_date + seconds((m_trade_messages[m_current_offset]).ttim);
      return true;
    }

    virtual Event* event()
    {
      return NULL;
    }

    INTRUSIVE_IMPL;

  private:
    string m_symbol;
    int m_n_messages;
    trade_message* m_trade_messages;
    int m_current_offset;
    Time m_date;
    NYSE_TAQ_99_QBDF_Builder* m_qbdf_builder;
    hf_tools::atomic_counter m_intrusive_ptr_base_counter;
  };

  char* NYSE_TAQ_99_QBDF_Builder::load_bin_file(string& idx_file_path, string& bin_file_path, bool is_quote_file, Event_Player& event_player, vector<Event_SequenceRP>& files_list)
  {
    uint32_t date_int = boost::lexical_cast<uint32_t>(m_date);
    ifstream idx_file(idx_file_path.c_str(), ios::in | ios::binary );
    index_message idx_msg;
    bool loaded_file = false;
    char* buffer = 0;
    while(idx_file.read(reinterpret_cast<char*>(&idx_msg), sizeof(index_message))) {
      if(idx_msg.date == date_int) {
        if(!loaded_file) {
          FILE * pFile = fopen(bin_file_path.c_str() , "rb");
          if (pFile==NULL) { // error 
          }

          // obtain file size:
          fseek (pFile , 0 , SEEK_END);
          size_t lSize = ftell (pFile);
          rewind (pFile);

          // allocate memory to contain the whole file:
          buffer = (char*) malloc (sizeof(char)*lSize);
          if (buffer == NULL) { // error
          }

          // copy the file into the buffer:
          size_t result = fread (buffer,1,lSize,pFile);
          if (result != lSize) { // error 
          }

          fclose (pFile);
          loaded_file = true;
        }

        string symbol(idx_msg.symbol, sizeof(idx_msg.symbol));
        boost::algorithm::trim_right(symbol);
        int n_messages = (idx_msg.endrec - idx_msg.begrec) + 1;
        if(is_quote_file) {
          quote_message* quote_message_buffer = reinterpret_cast<quote_message*>(buffer + (idx_msg.begrec-1)*sizeof(quote_message));
          Event_SequenceRP seq(new NYSE_TAQ_99_Quote_Rec_Event_Sequence(symbol, n_messages, quote_message_buffer, Time(m_date, "%Y%m%d"), this));
          files_list.push_back(seq);
          event_player.add_sequence(seq);
        } else {
          trade_message* trade_message_buffer = reinterpret_cast<trade_message*>(buffer + (idx_msg.begrec-1)*sizeof(trade_message));
          Event_SequenceRP seq(new NYSE_TAQ_99_Trade_Rec_Event_Sequence(symbol, n_messages, trade_message_buffer, Time(m_date, "%Y%m%d"), this));
          files_list.push_back(seq);
          event_player.add_sequence(seq);
        }
      }
      else { 
        break; 
      }
    }
    return buffer;
  }

  void NYSE_TAQ_99_QBDF_Builder::handle_trade(trade_message& trade_msg, string& symbol)
  {
      uint32_t symbol_id = process_taq_symbol(symbol);
      uint64_t exch_time = trade_msg.ttim * static_cast<uint64_t>(1000000);
      MDPrice price = MDPrice::from_fprice(trade_msg.price / 100000000.0 );
      uint32_t size = trade_msg.size;
      char correct = (char)trade_msg.corr;
      string cond = string(trade_msg.cond, 4);
      char exch = trade_msg.ex;
      if (exch == 'T')
        exch = 'Q';
      //uint32_t seq_no = trade_msg.tseq;

      //cout << Time::current_time().to_string() << " exch_time=" << exch_time << " symbol_id=" << symbol_id << " seq_no=" << seq_no << " price=" << price.fprice() << endl;
      //gen_qbdf_trade_micros_seq_qbdf_time(this, symbol_id, exch_time, 0, exch, ' ', correct, cond, size, price, seq_no);
      gen_qbdf_trade_micros(this, symbol_id, exch_time, 0, exch, ' ', correct, cond, size, price);

  }

  void NYSE_TAQ_99_QBDF_Builder::handle_quote(quote_message& quote_msg, string& symbol)
  {
      uint32_t symbol_id = process_taq_symbol(symbol);
      uint64_t exch_time = quote_msg.qtim * static_cast<uint64_t>(1000000);
      MDPrice bid_price = MDPrice::from_fprice(quote_msg.bid / 100000000.0 );
      uint32_t bid_size = quote_msg.bidsiz;
      MDPrice ask_price = MDPrice::from_fprice(quote_msg.ofr / 100000000.0 );
      uint32_t ask_size = quote_msg.ofrsiz;

      char cond = (char)quote_msg.mode;
      char exch = quote_msg.ex;
      if (exch == 'T')
        exch = 'Q';
      //uint32_t seq_no = quote_msg.qseq;

      //cout << Time::current_time().to_string() << " exch_time=" << exch_time << " symbol_id=" << symbol_id << " seq_no=" << seq_no << " bid_price=" << bid_price.fprice() << endl;
      //gen_qbdf_quote_micros_seq_qbdf_time(this, symbol_id, exch_time, 0, exch, cond, bid_price, ask_price, bid_size, ask_size, seq_no);
      gen_qbdf_quote_micros(this, symbol_id, exch_time, 0, exch, cond, bid_price, ask_price, bid_size, ask_size);
  }

  int NYSE_TAQ_99_QBDF_Builder::process_raw_file(const string& file_name)
  {
    // file_name="/data_mnt/nyse_taq/2003/02/d200302.tab"
  
    vector<Event_SequenceRP> files_list;
    Event_Player_Config event_player_config;
    Event_Player event_player(NULL, event_player_config);
 
    boost::filesystem::path p(file_name.c_str());
    boost::filesystem::path dir = p.parent_path();
    boost::filesystem::directory_iterator end_it;
    list<char*> buffers_to_clean_up;
    for(boost::filesystem::directory_iterator it(dir); it != end_it; ++it)
    {
      if(boost::filesystem::is_regular_file(it->path()))
      {
        string cur_file_name = it->path().filename().string();
        if(boost::algorithm::ends_with(cur_file_name, "idx")) {
          string bin_file_name = boost::replace_all_copy(cur_file_name, "idx", "bin");
          bool is_quote_file = boost::starts_with(bin_file_name, "q");
          string idx_file_path = it->path().string();
          string bin_file_path = (dir / boost::filesystem::path(bin_file_name.c_str())).string();
          
          char* bin_file = load_bin_file(idx_file_path, bin_file_path, is_quote_file, event_player, files_list);
          if(bin_file) { buffers_to_clean_up.push_back(bin_file); }
          // ToDo: store bin_file if not null so we can free it later
        }
      }
    }
 
    event_player.run();
    for(list<char*>::iterator it = buffers_to_clean_up.begin(); it != buffers_to_clean_up.end(); ++it) {
      delete(*it);
    }
    return 0;
  }
}
