#include <math.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
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
#include "CME_3_Hist_QBDF_Builder.h"
#include <MD/MDManager.h>
#include <Util/File_Utils.h>
#include <Util/Network_Utils.h>
#include <Util/Parameter.h>
#include <Simulator/Event_Player.h>
#include <FIX/FIX_Message.h>


namespace hf_core {
  using namespace std;
  using namespace hf_tools;
  using namespace boost::iostreams;

  class CME_3_Rec_Event_Sequence : public Event_Sequence
  {
  public:
    CME_3_Rec_Event_Sequence(boost::iostreams::filtering_stream<boost::iostreams::input>* fp_stream, string symbol_base,
                           CME_MDP_3_Channel_Handler_Base* channel_handler, Time cur_date, hash_map<int, bool>& security_ids, QBDF_Builder* qbdf_builder, bool is_for_sec_defs_only, 
                           Time_Duration time_offset)
      : m_infile(fp_stream), 
        m_symbol_base(symbol_base),
        m_channel_handler(channel_handler),
        m_date(cur_date),
        m_security_ids(security_ids),
        m_qbdf_builder(qbdf_builder),
        m_is_for_sec_defs_only(is_for_sec_defs_only),
        m_time_offset(time_offset),
        m_is_fall_back_week(false)
    {

      // This funky logic accounts for the possibility that today is the Sunday of DST
      Time t = Time::today() + hours(26);
      time_t secs = t.total_sec();
      struct tm tomorrow_tm;
      Time_Utils::clear_tm(tomorrow_tm);
      localtime_r(&secs, &tomorrow_tm);
      tomorrow_tm.tm_sec = 0;
      tomorrow_tm.tm_min = 0;
      tomorrow_tm.tm_hour = 0;
      secs = mktime(&tomorrow_tm);
      Time tomorrow(secs * ticks_per_second);
      Time_Duration day_diff = tomorrow - Time::today();
#ifdef CW_DEBUG
      cout << "day_diff.total_sec()=" << day_diff.total_sec() << endl;
#endif
      Time_Duration dst_offset = (hours(24) - seconds(day_diff.total_sec()));
      m_utc_offset = Time::today() - Time::utc_today() - dst_offset;
#ifdef CW_DEBUG
      cout << "m_utc_offset.total_sec()=" << m_utc_offset.total_sec() << endl;
#endif

      if(dst_offset == hours(-1)) {
        m_is_fall_back_week = true;
      }

      bool got_non_option_line = false;
      int num_tries = 1000;
      while(!got_non_option_line && getline(*m_infile, m_current_line) && num_tries > 0)
      {
        got_non_option_line = parse_current_line();
        m_channel_handler->set_qbdf_builder(NULL);
        --num_tries;
      }
      
      m_options_only = false;
      if(!got_non_option_line)
      {
        m_options_only = true;
        m_time = Time::current_time();
      }
    }

    virtual string str() const
    {
      return "";
    }

    bool process_current_message()
    {
      const char* raw_line = m_current_line.c_str();
      string msg_type;
      m_fix_field_parser.get_string(FIX_Field::MsgType, raw_line, &msg_type);
      //if(msg_type != "d" && m_is_for_sec_defs_only)
      //{
      //  return false;
      //}
      //cout << Time::current_time().to_string() << " process_current_message msg_type=" << msg_type << endl;
      if(msg_type == "d") // Security Definition
      {
        string symbol;
        int security_id;
        double display_factor;
        double min_price_increment;
        //string security_group;
        //strng maturity_month_year
        //double unit_of_measure_qty;
        int num_md_feed_types;        
        int outright_market_depth(0);
        int implied_market_depth(0);

        m_fix_field_parser.get_string(FIX_Field::Symbol, raw_line, &symbol);
        m_fix_field_parser.get_int(FIX_Field::SecurityID, raw_line, &security_id);
        m_fix_field_parser.get_double(FIX_Field::DisplayFactor, raw_line, &display_factor);
        m_fix_field_parser.get_double(FIX_Field::MinPriceIncrement, raw_line, &min_price_increment);
        m_fix_field_parser.get_int(FIX_Field::NoMdFeedTypes, raw_line, &num_md_feed_types);         

        if(symbol.size() > 5) // Ignore options and spreads
        {
          return true;
        }
        
        for(int i = 1; i <= num_md_feed_types; ++i)
        {
          string feed_type;
          string market_depth_str;
          
          m_fix_field_parser.find_nth_occurrence_of_string(FIX_Field::MDFeedType, raw_line, raw_line+m_current_line.size(), i, &feed_type);
          m_fix_field_parser.find_nth_occurrence_of_string(FIX_Field::MarketDepth, raw_line, raw_line+m_current_line.size(), i, &market_depth_str);
          if(feed_type == "GBX")
          {
            outright_market_depth = boost::lexical_cast<int>(market_depth_str);
          }
          else if(feed_type == "GBI")
          {
            implied_market_depth = boost::lexical_cast<int>(market_depth_str);
          }
        }
        //cout << Time::current_time().to_string() << " SEC: symbol=" << symbol << " security_id=" << security_id << " display_factor=" << display_factor
        //     << " min_price_increment=" << min_price_increment << " outright_market_depth=" << outright_market_depth
        //     << " implied_market_depth=" << implied_market_depth << endl;
        m_security_ids[security_id] = true;
#ifdef CW_DEBUG
        cout << "Added security id " << security_id << endl;
#endif
        unsigned maturity_month_year(0);
        uint32_t unit_of_measure_qty(0);
        string security_group("");
         
        string settle_price_type_str;
        m_fix_field_parser.get_string(FIX_Field::SettlPriceType, raw_line, &settle_price_type_str);
        uint32_t settle_price_type = 0;
        if(settle_price_type_str.size() == 8) {
          settle_price_type |= (settle_price_type_str[7] == '1') ? (0x1) : 0;
          settle_price_type |= (settle_price_type_str[6] == '1') ? (0x1 << 1) : 0;
          settle_price_type |= (settle_price_type_str[5] == '1') ? (0x1 << 2) : 0;
          settle_price_type |= (settle_price_type_str[4] == '1') ? (0x1 << 3) : 0;
        }

        m_channel_handler->set_qbdf_builder(m_qbdf_builder);
        m_channel_handler->AddInstrumentDefinition(security_id, symbol, maturity_month_year, display_factor, min_price_increment,
                                                  unit_of_measure_qty, outright_market_depth, implied_market_depth, 0, security_group, false, 0, 0.0, 0, 0, 
                                                  settle_price_type);

        m_channel_handler->set_qbdf_builder(NULL);
      }
      else if(msg_type == "X") // Incremental Refresh
      {
        int num_md_entries(0);
        
        m_fix_field_parser.get_int(FIX_Field::NoMDEntries, raw_line, &num_md_entries);
        string match_event_indicator_str;
        m_fix_field_parser.get_string(FIX_Field::MatchEventIndicator, raw_line, &match_event_indicator_str);
        uint32_t match_event_indicator(0);
        if(match_event_indicator_str.size() == 8) {
          match_event_indicator |= (match_event_indicator_str[7] == '1' ? (0x01 << 0) : 0);
          match_event_indicator |= (match_event_indicator_str[6] == '1' ? (0x01 << 1) : 0);
          match_event_indicator |= (match_event_indicator_str[5] == '1' ? (0x01 << 2) : 0);
          match_event_indicator |= (match_event_indicator_str[4] == '1' ? (0x01 << 3) : 0);
          match_event_indicator |= (match_event_indicator_str[3] == '1' ? (0x01 << 4) : 0);
          match_event_indicator |= (match_event_indicator_str[2] == '1' ? (0x01 << 5) : 0);
          match_event_indicator |= (match_event_indicator_str[1] == '1' ? (0x01 << 6) : 0);
          match_event_indicator |= (match_event_indicator_str[0] == '1' ? (0x01 << 7) : 0);
        }
 
/*
        if(m_symbol_base == "GE")
        {
#ifdef CW_DEBUG
          cout << "RawLine:" << m_current_line << endl;
#endif
#ifdef CW_DEBUG
          cout << "num_md_entries=" << num_md_entries << endl;
#endif
          
        }
*/
        size_t current_group_offset = m_current_line.find("279=");
        for(int i = 1; i <= num_md_entries; ++i)
        {
          int security_id;
          int update_action;
          char entry_type;
          int price_level(9999999);
          double price;
          int size;
          int rpt_seq;
          //int trade_volume(0);
          //char quote_condition(' ');
          bool last_in_group = i == num_md_entries;

          size_t group_end_offset = m_current_line.find("279=", current_group_offset + 1);
          //cout << "current_group_offset=" << current_group_offset << " group_end_offset=" << group_end_offset << endl;
          string group_string;
          if(group_end_offset == string::npos)
          {
            group_string = m_current_line.substr(current_group_offset);
          }
          else
          {
            group_string = m_current_line.substr(current_group_offset, group_end_offset - current_group_offset);
          }
          //cout << "Group_string='" << group_string << "'" << endl;
          group_string.push_back('\01');
          current_group_offset = group_end_offset;
          const char* raw_group = group_string.c_str();

          m_fix_field_parser.clear();
          m_fix_field_parser.parse_fields(raw_group, raw_group + group_string.size());

          m_fix_field_parser.get_int(FIX_Field::SecurityID, raw_group, &security_id);
/*
          if(m_symbol_base == "GE")
          {
#ifdef CW_DEBUG
            cout << "field " << i << ": security_id=" << security_id << endl;
#endif
          }
*/
          //cout << "security_id=" << security_id << endl;
          if(m_security_ids.find(security_id) == m_security_ids.end())
          {
            continue;
          }
          //cout << "Found security id " << security_id << endl;
          m_fix_field_parser.get_int(FIX_Field::MDUpdateAction, raw_group, &update_action);
          m_fix_field_parser.get_char(FIX_Field::MDEntryType, raw_group, &entry_type);
          m_fix_field_parser.get_int(FIX_Field::MDPriceLevel, raw_group, &price_level);
          m_fix_field_parser.get_double(FIX_Field::MDEntryPx, raw_group, &price);
          m_fix_field_parser.get_int(FIX_Field::MDEntrySize, raw_group, &size);
          m_fix_field_parser.get_int(FIX_Field::RptSeq, raw_group, &rpt_seq);

          if(entry_type == '2') { entry_type = 'T'; }

/*
          if(security_id == 801320)
          {
#ifdef CW_DEBUG
            cout <<  Time::current_time().to_string() << " INC: security_id=" << security_id << "  "
                 << i << ". update_action=" << update_action << " entry_type=" << entry_type << " price_level=" << price_level
                 << " price=" << price << " size=" << size << " rpt_seq=" << rpt_seq << " symbase=" << m_symbol_base 
                 << " trade_volume=" << trade_volume << " trade_condition=" << trade_condition << " quote_condition=" << quote_condition << endl;
#endif
#ifdef CW_DEBUG
            cout << "group_str='" << group_string << "'" << endl;
#endif
          }
*/
          // Implied prices aren't real trade prices, and they aren't in MDP 3.0 anyway, so to keep things consistent I'm zeroing them out

          uint64_t exch_time = Time::current_time().total_usec() * 1000;

          uint32_t settle_price_type = 0;

          if(entry_type == '6') {
            m_channel_handler->apply_daily_statistics(security_id, entry_type, price, rpt_seq, static_cast<uint8_t>(match_event_indicator), exch_time, 
                                                      settle_price_type);
          }
          else {
            m_channel_handler->apply_incremental_update(security_id, static_cast<uint8_t>(update_action), entry_type, static_cast<uint32_t>(price_level),
                                                        price, static_cast<int32_t>(size), last_in_group, rpt_seq, static_cast<uint8_t>(match_event_indicator), exch_time);
          }
        }
      }
      else if(msg_type == "f") // Security Status
      {
        const char* raw_line = m_current_line.c_str();
        int security_id;
        //int status;
#ifdef CW_DEBUG
        cout << Time::current_time().to_string() << "STAT" << endl;
#endif
        m_fix_field_parser.get_int(FIX_Field::SecurityID, raw_line, &security_id);
        //m_fix_field_parser.get_int(FIX_Field::SecurityTradingStatus, raw_line, &status);

        if(m_security_ids.find(security_id) != m_security_ids.end())
        {
#ifdef CW_DEBUG
          cout << "STAT: security_id=" << security_id << endl;
#endif
          //m_channel_handler->apply_security_status(security_id, status, rpt_seq);
        }
      }
      else
      {
#ifdef CW_DEBUG
        cout << "CME_3_Hist_QBDF_Builder: Unrecognized msg_type " << msg_type << endl;
#endif
      }
      return true;
    }

    virtual bool next(Event_Hub& hub)
    {
      if(m_options_only)
      {
        return false;
      }
      // Build QBDF for the existing message
      if(!process_current_message())
      {
        return false;
      }
      //cout << "Next: " << m_time.to_string() << " " << m_symbol_base << endl;
      bool got_non_option_line = false;
      while(!got_non_option_line)
      {
        if(!getline(*m_infile, m_current_line))
        {
          return false;
        }
        got_non_option_line = parse_current_line();
      }
      if(Time::current_time() < (m_date + minutes(1)))
      {
        //cout << m_time.to_string() << " - still too early" << endl;
      }
      else if(Time::current_time() > m_date + days(1))
      {
#ifdef CW_DEBUG
        cout << Time::current_time().to_string() << " - too late - ending" << endl;
#endif
        return false;
      }
      else if(m_channel_handler->get_qbdf_builder() == NULL)
      {
#ifdef CW_DEBUG
        cout << Time::current_time().to_string() << " - SOD - setting qbdf builder to " << m_qbdf_builder << endl;
#endif
        m_channel_handler->set_qbdf_builder(m_qbdf_builder);
        m_channel_handler->publish_current_state_to_qbdf_snap();
      }
      return true;
    }

    virtual Event* event()
    {
      return NULL;
    }

    INTRUSIVE_IMPL;

    bool m_options_only;
  private:
    bool parse_current_line()
    {
      m_fix_field_parser.clear();
      const char* raw_line = m_current_line.c_str();
      m_fix_field_parser.parse_fields(raw_line, raw_line + m_current_line.size());
      
      string symbol;
      m_fix_field_parser.get_string(FIX_Field::SecurityDesc, raw_line, &symbol);
      if(symbol.find(" ") != string::npos) // Ignore options
      {
        return false;
      }

      string sending_time_str;
      m_fix_field_parser.get_string(FIX_Field::SendingTime, raw_line, &sending_time_str);
      string time_substr = sending_time_str.substr(0, 14);
      string msec_substr = sending_time_str.substr(14, 3);
      int millis = boost::lexical_cast<int>(msec_substr);
      Time given_time = Time(time_substr, "%Y%m%d%H%M%S") + msec(millis) - m_utc_offset + m_time_offset;
/*
      if(m_symbol_base == "GE")
      {
#ifdef CW_DEBUG
        cout << sending_time_str << "->" << given_time.to_string() << " current_time=" << Time::current_time().to_string() << endl;
#endif
      }
*/
      if(given_time > m_date + days(1))
      {
        return false;
      } 
/*
      if(m_is_fall_back_week && given_time <= Time::today())
      {
        m_time = Time::current_time();
      } 
      else */if(given_time >= Time::current_time())
      {
        //cout << "AAA given_time=" << given_time.to_string() << " current_time=" << Time::current_time().to_string() << " symbolbase=" << m_symbol_base << endl;
        m_time = given_time;
      }
      else
      {
        //cout << "BBB given_time=" << given_time.to_string() << " current_time=" << Time::current_time().to_string() << " symbolbase=" << m_symbol_base << endl;
        m_time = Time::current_time();
      }
      return true;
    }

    string m_current_line;
    boost::iostreams::filtering_stream<boost::iostreams::input>* m_infile;
    string m_symbol_base;
    CME_MDP_3_Channel_Handler_Base* m_channel_handler;
    Time m_date;
    FIX_Field_Parser m_fix_field_parser;
    hf_tools::atomic_counter m_intrusive_ptr_base_counter;
    hash_map<int, bool>& m_security_ids;
    QBDF_Builder* m_qbdf_builder;
    bool m_is_for_sec_defs_only;
    Time_Duration m_time_offset;
    Time_Duration m_utc_offset;
    bool m_is_fall_back_week;
  };

  void get_this_month_files_3(const string& file_name, list<boost::filesystem::directory_entry>& result)
  {
    boost::filesystem::path p(file_name.c_str());
    boost::filesystem::path dir = p.parent_path(); 
    boost::filesystem::directory_iterator end_it;
    for(boost::filesystem::directory_iterator it(dir); it != end_it; ++it)
    {
      result.push_back(*it);
    }
  }

  void get_this_month_and_last_month_files_3(const string& file_name, list<boost::filesystem::directory_entry>& result, boost::gregorian::date cur_day)
  {
    get_this_month_files_3(file_name, result);
    boost::gregorian::date_duration dd(-7);
    boost::gregorian::date prev_date = cur_day + dd;
    boost::filesystem::path p(file_name.c_str());
    boost::filesystem::path dir = p.parent_path().parent_path().parent_path();
    string prev_date_str = boost::gregorian::to_iso_string(prev_date);
    string prev_year_str = prev_date_str.substr(0, 4);
    string prev_month_str = prev_date_str.substr(4, 2);

    string prev_month_dir = dir.string() + "/" + prev_year_str + "/" + prev_month_str;

    boost::filesystem::path prev_path(prev_month_dir.c_str());
    
    boost::filesystem::directory_iterator end_it;
    for(boost::filesystem::directory_iterator it(prev_path); it != end_it; ++it)
    {
      result.push_back(*it);
    }
  } 

  int CME_3_Hist_QBDF_Builder::process_raw_file(const string& file_name)
  {
    Time::set_simulation_mode(true);
    //Time::set_current_time(Time(m_date, "%Y%m%d"));
    ExchangeID exchange_id = m_hf_exch_ids[0];
   
    boost::scoped_ptr<CME_MDP_3_Handler> message_handler(new CME_MDP_3_Handler(m_app));
 
    //message_handler->set_qbdf_builder(this);
    message_handler->init(m_source_name, exchange_id, m_params);

    int hours_offset = 0;
    if(m_params.has("cme_hours_offset")) {
      hours_offset = m_params["cme_hours_offset"].getInt();
    }
    Time_Duration time_offset = hours(hours_offset); 

    if(!message_handler->sets_up_own_channel_contexts())
    {
      Handler::channel_info_map_t& cim = message_handler->m_channel_info_map;
      for (size_t i = 0; i < 256; ++i) {
        cim.push_back(channel_info("", i));
        cim.back().context = i; //
        cim.back().has_reordering = false;
      }
    }

/////////////

    boost::filesystem::path p(file_name.c_str());
    string file_name_without_path = p.filename().string();
    string exchange = file_name_without_path.substr(0, 4);
    vector<string> template_file_items;
    boost::split(template_file_items, file_name_without_path, boost::is_any_of("_.-"));

    list<boost::filesystem::directory_entry> possible_files;

    string file_start_date;
    string file_end_date;
    int file_start_index;
    int file_end_index;
    bool need_first_day_of_week_file = false;
    string first_day_of_week = "";
    if(template_file_items[3] == "FUT") 
    {
      need_first_day_of_week_file = true;
      // Daily
      file_start_date = template_file_items[4];
      file_end_date = template_file_items[4];
      file_start_index = 4;
      file_end_index = 4;

      // Get first day of the week
      boost::gregorian::date d = boost::gregorian::from_undelimited_string(file_start_date);
      get_this_month_and_last_month_files_3(file_name, possible_files, d);
      int prev_day_of_week = static_cast<int>(d.day_of_week());
#ifdef CW_DEBUG
      cout << "weekday=" << d.day_of_week() << endl;
#endif
#ifdef CW_DEBUG
      cout << "num_weekday=" << static_cast<int>(d.day_of_week()) << endl;
#endif
      first_day_of_week = file_start_date;
      boost::gregorian::date_duration dd(-1);
      d += dd;
#ifdef CW_DEBUG
      cout << "now d.day_of_week=" << static_cast<int>(d.day_of_week()) << endl;
#endif
      while(static_cast<int>(d.day_of_week()) < prev_day_of_week)
      { 
        for(list<boost::filesystem::directory_entry>::iterator it = possible_files.begin(); it != possible_files.end(); ++it)
        {
          if(boost::filesystem::is_regular_file(it->path()))
          {
            string cur_file_name = it->path().filename().string();
            vector<string> name_items;
            boost::split(name_items, cur_file_name, boost::is_any_of("_.-"));
            if(name_items.size() >= 6)
            {
               string cur_date_str = name_items[file_start_index];
               //cout << "AAA cur_date_str=" << cur_date_str << endl;
               
               if(cur_date_str.size() == 8) // Must be valid date
               {
                 boost::gregorian::date cur_date = boost::gregorian::from_undelimited_string(cur_date_str);
                 if(d == cur_date)
                 {
                   first_day_of_week = cur_date_str;
#ifdef CW_DEBUG
                   cout << "Setting first_day_of_week to " << first_day_of_week << endl;
#endif
                   break;
                 }
               }
            }
          }  
        }
        prev_day_of_week = static_cast<int>(d.day_of_week());
        d += dd;
      }
#ifdef CW_DEBUG
      cout << "first_day_of_week=" << first_day_of_week << endl;
#endif
      Time::set_current_time(Time(first_day_of_week, "%Y%m%d"));
    }
    else
    {
      get_this_month_files_3(file_name, possible_files);
      // Weekly
      file_start_date = template_file_items[3];
      file_end_date = template_file_items[4];
      file_start_index = 3;
      file_end_index = 4;
      Time::set_current_time(Time(file_start_date, "%Y%m%d") - hours(23));
    }
#ifdef CW_DEBUG
    cout << "file_start_date=" << file_start_date << " file_end_date=" << file_end_date << endl;
#endif


    if(template_file_items.size() < 6)
    {
      // ToDo: log error
    }

    m_only_one_open_file_at_a_time = true;

    hash_map<int, bool> security_ids;

    vector<Event_SequenceRP> files_list;
    Event_Player_Config event_player_config;
    Event_Player event_player(NULL, event_player_config);
    int file_count = 0;
    list<boost::shared_ptr<boost::iostreams::filtering_stream<boost::iostreams::input> > > fp_streams;
    for(list<boost::filesystem::directory_entry>::iterator it = possible_files.begin(); it != possible_files.end(); ++it)
    {
      if(boost::filesystem::is_regular_file(it->path()))
      {
        string cur_file_name = it->path().filename().string();
        vector<string> name_items;
        boost::split(name_items, cur_file_name, boost::is_any_of("_.-"));

        bool is_for_sec_defs_only = false;
        if(name_items.size() < 6 ||
           name_items[file_start_index] != file_start_date ||
           name_items[file_end_index] != file_end_date)
        {
          //cout << "name_items.size()=" << name_items.size() << endl;
          //cout << "name_items[file_start_index]=" << name_items[file_start_index] << endl;
          //cout << "name_items[file_end_index]=" << name_items[file_end_index] << endl;
          if(need_first_day_of_week_file)
          {
            if(name_items[0] == "secdef" || name_items[0] == "EnrichMD" || name_items[0] == "Marketdepth")
            {
              continue;
            }
            string cur_date_str = name_items[file_start_index];
/*
            if(cur_date_str != first_day_of_week)
            {
              continue;
            }
*/

            if(cur_date_str.size() == 8)
            {
               boost::gregorian::date cur_date = boost::gregorian::from_undelimited_string(cur_date_str);
               boost::gregorian::date_duration dd(1);
               if(cur_date >= boost::gregorian::from_undelimited_string(first_day_of_week) && cur_date <= (boost::gregorian::from_undelimited_string(file_start_date) + dd))
               {
                 is_for_sec_defs_only = true;
               }
               else
               {
                 continue;
               }
            }
            else
            {
              continue;
            }

          }
          else
          {
            continue;
          }
        }
        if(name_items[3] == "OPT")
        {
          continue;
        }
        string cur_file_exchange = name_items[0];
        string cur_symbol_base = name_items[2]; 

        // For NYMEX only, ignore symbols that have numbers in them.  This is mainly a hack to avoid opening too many files.
        if(exchange == "XNYM")
        {
          bool found_numeric_char = false;
          for(size_t i = 0; i < cur_symbol_base.size(); ++i)
          {
            char cur_char = cur_symbol_base[i];
            if(cur_char >= '0' && cur_char <= '9')
            {
              found_numeric_char = true;
            }
          }
          if(found_numeric_char)
          {
            continue;
          }
        }

        //if(cur_symbol_base != "CL") { continue; } 

        if(cur_file_exchange == exchange)
        {
          string cur_file_path = it->path().string();

          ++file_count;
#ifdef CW_DEBUG
          cout << "opening file " << file_count << " " << cur_file_path << endl;
#endif
          boost::shared_ptr<boost::iostreams::filtering_stream<boost::iostreams::input> > fp_stream(new boost::iostreams::filtering_stream<boost::iostreams::input>);
          fp_stream->push(gzip_decompressor());
          fp_stream->push(file_source(cur_file_path));

          fp_streams.push_back(fp_stream);
          Time midnight = Time(m_date, "%Y%m%d");
          CME_3_Rec_Event_Sequence* cme_seq = new CME_3_Rec_Event_Sequence(fp_stream.get(), cur_symbol_base, message_handler->my_one_channel_handler(), midnight, security_ids, this, is_for_sec_defs_only,
                                                                           time_offset);
          Event_SequenceRP seq(cme_seq);
          files_list.push_back(seq);
          if(!cme_seq->m_options_only)
          {
            event_player.add_sequence(seq);
          }
        }
      }
    }
    event_player.run();
    return 0;
  }
}
