#ifndef hf_core_Data_QBDF_Dump_h
#define hf_core_Data_QBDF_Dump_h

#include <Data/QBDF_Parser.h>

namespace hf_core {
  class Application;
  
  using namespace hf_tools;
  
  class QBDF_Dump {
  public:
    
    QBDF_Dump(Application* app, const Parameter& params);
    
    void set_logger(ILogger* logger) { m_logger = logger; }
    
    void run();
    void dump_qbdf_event();
    
    Time parse_time(const string& t);
    
  private:
    
    Application* m_app;
    QBDF_Parser m_parser;
    bool        m_csv;
    bool        m_json;
    char        m_date[9]; // 8 chars plus NUL
    
    boost::dynamic_bitset<> m_ids_to_print;
    
    Time m_start_time;
    Time m_end_time;
    
    ILogger* m_logger;
    
    LoggerRP m_default_logger;
  };
  
}

#endif /* ifndef hf_core_Data_QBDF_Dump_h */
