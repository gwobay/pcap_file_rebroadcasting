#ifndef hf_core_Solar_Pcap_To_QRec_h
#define hf_core_Solar_Pcap_To_QRec_h

#include <string>
#include <hash_map>
#include <Data/Handler.h>

namespace hf_core {
  using namespace std;

  class Solar_Pcap_To_QRec
  {
  private:
    struct Channel
    {
      ofstream* file_out;
      uint64_t context;

      Channel(ofstream* fileOut, uint64_t _context)
        : file_out(fileOut), context(_context)
      {
      }
    };

  public:
    Solar_Pcap_To_QRec();
    ~Solar_Pcap_To_QRec();

    void init(const string& input_file, const string& output_dir, const string& date_str);

    void map_channel(const string& mc_group, uint16_t mc_port, const string& channel_file_name);

    void run();

  private:
    string m_input_file;
    string m_output_dir;
    string m_date_str;
    uint64_t m_sod_ticks;
    hash_map<uint64_t, Channel> m_port_to_channel;
    hash_map<string, ofstream*> m_file_name_to_handle;
    hash_map<string, uint64_t> m_file_name_to_context;
  };

}

#endif
