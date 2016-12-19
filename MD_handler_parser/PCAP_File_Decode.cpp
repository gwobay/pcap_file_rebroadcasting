#include <stdio.h>

#include <Data/PCAP_Parser.h>
#include <Data/Decode.h>

#include <Data/Eurex_Handler.h>
#include <Data/Euronext_XDP_Handler.h>
#include <Data/PITCH_2_9_Handler.h>
#include <Data/LSE_Millennium_Handler.h>


using namespace std;
using namespace hf_core;





// this is kind of a hack b/c the "read" method wants a Handler pointer
class PCAP_Decode_Handler : public Handler {
public:
  PCAP_Decode_Handler(Decoder dec, Dump_Filters filters) :
    Handler(NULL, ""),
    m_name("PCAP_Decode_Handler"),
    m_decoder(dec),
    m_filters(filters),
    m_ilogger(new Dump_Logger())
  { }

  virtual size_t parse(size_t context, const char* buf, size_t len) {
    char time_buf[32];
    Time_Utils::print_time(time_buf, Time::current_time(), Timestamp::MILI);
    
    m_ilogger->printf("pcap time %s context %2lu", time_buf, context);
    m_decoder(*m_ilogger, m_filters, context, buf, len);
    return len;
  }

  virtual const string& name() const { return m_name; }
  virtual size_t parse2(size_t context, const char* buf, size_t len) { return 0; }
  virtual size_t read(size_t context, const char* buf, size_t len) { return 0; }
  virtual void reset(const char* msg) { }
  virtual void subscribe_product(ProductID id, ExchangeID exch, const string& mdSymbol,const string& mdExch) { }
  virtual void start() { }
  virtual void stop() { }
private:
  std::string m_name;
  Decoder m_decoder;
  Dump_Filters m_filters;
  boost::scoped_ptr<Dump_Logger> m_ilogger;
};


int
main(int argc, const char* argv[])
{
  decoder_options_t decoder_options[] = {
    { "eurex", Eurex_Handler::dump, "Eurex EOBI 3.0"},
    { "euronext_xdp", Euronext_XDP_Handler::dump, "XPAR,XBRU,XLIS,XAMS" },
    { "euronext_xdp_refdata", Euronext_XDP_Handler::dump_refdata, "XPAR,XBRU,XLIS,XAMS" },
    { "lse", LSE_Millennium_Handler::dump, "XLON LSE Millennium" },
    { "pitch", PITCH_2_9_Handler::dump, "BATS,BATY,EDGX,EDGA,BATE,CHIX   (EDGX,EDGA after 20150112 merger)"  },
    { "binary", binary_dump, "dump binary as 0s and 1s" },
    { 0, 0, 0 }
  };

  if(argc < 3) {
    //printf("Usage: %s --<decoder> [--start YYYY-MM-DD HH:MM:SS.mmm] infile [outfile]\n", argv[0]);
#ifdef CW_DEBUG
    printf("Usage: %s --<decoder> infile\n", argv[0]);
#endif
#ifdef CW_DEBUG
    printf(" Decoders:\n");
#endif
    for(int i = 0; decoder_options[i].name; ++i) {
#ifdef CW_DEBUG
      printf("  %-20s   %s\n", decoder_options[i].name, decoder_options[i].help_str);
#endif
    }
    return 1;
  }

  if(strlen(argv[1]) < 4) {
#ifdef CW_DEBUG
    printf("Unknown decoder %s\n", argv[1]);
#endif
    return 2;
  } else if(argv[1][0] != '-' || argv[1][1] != '-') {
#ifdef CW_DEBUG
    printf("Invalid decoder option %s,  should start with --\n", argv[1]);
#endif
    return 2;
  }
  Decoder dec;
  const char* decoder_name = argv[1] + 2;
  for(int i = 0; decoder_options[i].name; ++i) {
    if(0 == strcmp(decoder_name, decoder_options[i].name)) {
      dec = decoder_options[i].dec;
      break;
    }
  }

  if(!dec) {
#ifdef CW_DEBUG
    printf("Unknown decoder %s\n", argv[1]);
#endif
    return 2;
  }

  string filename = argv[2];

  Dump_Filters filters;

  
  Time::set_simulation_mode(true);

  PCAP_Decode_Handler handler(dec, filters);
  PCAP_Parser p;
  p.read(filename, &handler);


  return 0;
}
