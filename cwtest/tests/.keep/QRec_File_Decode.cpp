#include <Data/Handler.h>
#include <Data/MDBufferQueue.h>
#include <Data/MoldUDP64.h>
#include <Data/TVITCH_5_Handler.h>
#include <Data/NYSE_Handler.h>
#include <Data/NYSE_XDP_Handler.h>
#include <Data/NYSE_XDP_Integrated_Handler.h>
#include <Data/ARCA_Handler.h>
#include <Data/DirectEdge_0_9_Handler.h>
#include <Data/LSE_Millennium_Handler.h>
#include <Data/PITCH_2_9_Handler.h>
#include <Data/CQS_CTS_Handler.h>
#include <Data/TSE_FLEX_Handler.h>
#include <ExchangeSession/LSE_Native_Session.h>
#include <Session/QMessage_Session.h>
#include <ExchangeSession/EDGE_XPRS_Session.h>
#include <ExchangeSession/NYSE_UTP_Session.h>
#include <ExchangeSession/BATS_BOE_Session.h>
#include <ExchangeSession/BATS_BOE_Nomura_Session.h>
#include <ExchangeSession/OUCH_Exchange_Session.h>
#include <ExchangeSession/ARCA_DIRECT_Session.h>
#include <ExchangeSession/Eurex_ETI_Session.h>
#include <Data/TMX_QL2_Handler.h>
#include <Data/NXT_Book_Handler.h>
#include <Data/XETRA_Handler.h>
#include <Data/Euronext_XDP_Handler.h>
#include <Data/ASX_ITCH_1_1_Handler.h>

#include <Data/Handler.h>
#include <Data/Decode.h>

#include <Util/File_Utils.h>

using namespace std;


using namespace hf_tools;
using namespace hf_core;

struct rec {
  struct record_header rh;
  char buf[128 * 1024];
}  __attribute__ ((packed));

struct long_rec {
  struct long_record_header rh;
  char buf[128 * 1024];
}  __attribute__ ((packed));



class Decode_Handler : public Handler {
public:
  
  Dump_Filters m_filters;
  
  Decoder m_decoder;
  
  string m_name;

  bool  m_suppress_qrec;
  
  boost::scoped_ptr<Dump_Logger> m_ilogger;
  
public:
  Decode_Handler(Application* app, Decoder dec, const Dump_Filters& filters) :
    Handler(app, ""),
    m_filters(filters),
    m_decoder(dec),
    m_suppress_qrec(false),
    m_ilogger(new Dump_Logger())
  {
    m_filters.skip_heartbeats = true;
  }

  Decode_Handler(Application* app, Decoder dec, const Dump_Filters& filters, FILE *out) :
    Handler(app, ""),
    m_filters(filters),
    m_decoder(dec),
    m_suppress_qrec(false),
    m_ilogger(new Dump_Logger(out))
  {
    m_filters.skip_heartbeats = true;
  }

  virtual const string& name() const { return m_name; }
  
  virtual size_t parse(size_t context, const char* buf, size_t len) {
    throw runtime_error("Decode_Handler::parse(context, buf, len) called");
  }
  
  size_t parse_qRec(size_t context, const char* buf, size_t len, int fileno, bool short_form)
  {
    const record_header* rh = reinterpret_cast<const record_header*>(buf);
    buf += sizeof(record_header);
    len -= sizeof(record_header);
    
    if(m_filters.start.is_set()) {
      if(Time(rh->ticks) < m_filters.start) {
        return len;
      }
    }
    
    Time::set_current_time(Time(rh->ticks));
    
    char time_buf[32];
    Time_Utils::print_time(time_buf, Time(rh->ticks), Timestamp::MICRO);
    
    if(!m_suppress_qrec) {
      if(short_form)
      {
        //m_ilogger->printf("%s,%lu", time_buf, rh->context);
      }
      else
      {
        m_ilogger->printf("qrec_record_time=%s,context=%lu,qrec_len=%u", time_buf, rh->context, rh->len);
      }
    }
    
    return m_decoder(*m_ilogger, m_filters, rh->context, buf, rh->len);
  }

  size_t parse_qRel(size_t context, const char* buf, size_t len, int fileno)
  {
    const long_record_header* rh = reinterpret_cast<const long_record_header*>(buf);
    buf += sizeof(long_record_header);
    len -= sizeof(long_record_header);
    
    if(m_filters.start.is_set()) {
      if(Time(rh->ticks) < m_filters.start) {
        return len;
      }
    }
    
    Time::set_current_time(Time(rh->ticks));
    
    char time_buf[32];
    Time_Utils::print_time(time_buf, Time(rh->ticks), Timestamp::MICRO);
    
    if(!m_suppress_qrec) {
      m_ilogger->printf("qrel_record_time=%s,context=%lu,qrec_len=%u", time_buf, rh->context, rh->len);
    }
    
    return m_decoder(*m_ilogger, m_filters, rh->context, buf, rh->len);
  }
  
  virtual size_t parse2(size_t context, const char* buf, size_t len)
  {
    return 0;
  }

  virtual size_t read(size_t context, const char* buf, size_t len)
  {
    return len;
  }

  virtual void reset(const char* msg)
  {
    m_ilogger->printf("ERROR:  %s\n", msg);
  }

  virtual void send_alert(const char* fmt, ...)
  {
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");
    va_end(va);
  }

  virtual void start() { }
  virtual void stop() { }
  virtual void subscribe_product(ProductID id_, ExchangeID exch_, const string& mdSymbol_,const string& mdExch_) { }
};


bool
check_record_header(FILE* file_stream, char* buffer, uint64_t max_context)
{
  record_header* rec = reinterpret_cast<record_header*>(buffer);
  
  if((strncmp(buffer, record_header_title, 4) != 0 && strncmp(buffer, long_record_header_title, 4) != 0) || rec->context > max_context) {
    fprintf(stderr, "Mangled record header detected... '%.4s'  \n", buffer);
    size_t r;
    int byte_counter = 0;
    while (1) {
      int i = 0;
      for (; i < 4; i++) {
        r = fread(buffer+i, 1, 1, file_stream);
        ++byte_counter;
        if (!r || (*(buffer+i) != record_header_title[i])) {
          break;
        }
      }
      
      if (!r) {
        fprintf(stderr, "Error while looking for good record header title, possibly EOF\n");
        break;
      } else if (i == 4) {
        fprintf(stderr, "Found good record header title, reading rest of record header\n");
        r = fread(buffer+4, sizeof(record_header)-4, 1, file_stream);
        byte_counter += (sizeof(record_header)-4);
        if (rec->context > max_context) {
          fprintf(stderr, "Context looks bad on this record, searching for new one\n");
          break;
        }
        fprintf(stderr, "Context looks good on new record header title, returning to normal parsing\n");
        return true;
      }
      fprintf(stderr, "Searching forward for good record header title (bytes searched: %d)\n", byte_counter);
    }
    return false;
  }
  return true;
}


int
main(int argc, const char* argv[])
{
  
  decoder_options_t decoder_options[] = {
    { "arca", ARCA_Handler::dump, "Older pre-XDP ARCX recordings" },
    { "asx_itch", ASX_ITCH_1_1_Handler::dump, "Australia" },
    { "cta", CQS_CTS_Handler::dump, "CTA/UTP  Consolidated level-1 US exchanges" },
    { "edge", DirectEdge_0_9_Handler::dump, "EDGX,EDGA  older pre-merge recordings,  20150109 and prior" },
    { "euronext_xdp", Euronext_XDP_Handler::dump, "XPAR,XBRU,XLIS,XAMS" },
    { "euronext_xdp_refdata", Euronext_XDP_Handler::dump_refdata, "XPAR,XBRU,XLIS,XAMS" },
    { "itch4", MoldUDP64::dump, "XNAS,XBOS" },
    { "itch5", TVITCH_5_Handler::dump, "XNAS,XBOS" },
    { "lse", LSE_Millennium_Handler::dump, "XLON LSE Millennium" },
    { "nyse", NYSE_Handler::dump, "XNYS Older pre-XDP recorindgs" },
    { "nyse_xdp", NYSE_XDP_Handler::dump, "XNYS,ARCX" },
    { "nyse_xdp_integrated", NYSE_XDP_Integrated_Handler::dump, "XNYS 201511 and after" },
    { "nxt_book", NXT_Book_Handler::dump, "Normura NXT  consolidated level2 recordings for all EU exchanges" },
    { "pitch", PITCH_2_9_Handler::dump, "BATS,BATY,EDGX,EDGA,BATE,CHIX   (EDGX,EDGA after 20150112 merger)"  },
    { "tse_flex", TSE_FLEX_Handler::dump, "XTKS  Tokyo" },
    { "tmx_ql2", TMX_QL2_Handler::dump, "XTSE  Toronto" },
    { "xetra_ebs14", XETRA_Handler::dump, "XETR  Germany" },
    
    { "arca_direct", ARCA_DIRECT_Session::dump, "ARCX  direct order entry" },
    { "bats_boe", BATS_BOE_Session::dump, "BATS,BATY,EDGX,EDGA  direct order entry" },
    { "bats_boe_nomura", BATS_BOE_Nomura_Session::dump, "BATS,BATY,EDGX,EDGA  direct order entry" },
    { "edge_xprs", EDGE_XPRS_Session::dump, "EDGX.EDGA direct order entry retired after 20150109" },
    { "eurex_eti", EUREX_ETI_Session::dump, "EUREX ETI direct order entry" },
    { "lse_native", LSE_Native_Session::dump, "XLON direct order entry" },
    { "nyse_utp",NYSE_UTP_Session::dump, "XNYS,ARCX  direct order entry" },
    { "ouch", OUCH_Exchange_Session::dump, "XNAS,XBOS direct order entry" },
    { "qmessage", QMessage_Session::dump, "Quantbot Internal direct order entry" },
    //{ "arca_direct_short", ARCA_DIRECT_Session::dump, "ARCX  direct order entry" },
    //{ "bats_boe_short", BATS_BOE_Session::dump, "BATS,BATY,EDGX,EDGA  direct order entry" },
    //{ "bats_boe_nomura_short", BATS_BOE_Nomura_Session::dump, "BATS,BATY,EDGX,EDGA  direct order entry" },
    //{ "nyse_utp_nomura",NYSE_UTP_Session::dump, "XNYS,ARCX  direct order entry" },
    { "ouch_short", OUCH_Exchange_Session::dump_short, "XNAS,XBOS direct order entry" },
    { "bats_boe_nomura_short", BATS_BOE_Nomura_Session::dump_short, "BATS,BATY,EDGX,EDGA  direct order entry" },
    { "nyse_utp_short",NYSE_UTP_Session::dump_short, "XNYS,ARCX  direct order entry" },
    { "arca_direct_short", ARCA_DIRECT_Session::dump_short, "ARCX  direct order entry" },
    { 0, 0, 0 }
  };
  
  if(argc < 3) {
    printf("Usage: %s --<decoder> [--start YYYY-MM-DD HH:MM:SS.mmm] infile [outfile]\n", argv[0]);
    printf(" Decoders:\n");
    for(int i = 0; decoder_options[i].name; ++i) {
      printf("  %-20s   %s\n", decoder_options[i].name, decoder_options[i].help_str);
    }
    return 1;
  }
  
  Decoder dec;
  
  if(strlen(argv[1]) < 4) {
    printf("Unknown decoder %s\n", argv[1]);
    return 2;
  } else if(argv[1][0] != '-' || argv[1][1] != '-') {
    printf("Invalid decoder option %s,  should start with --\n", argv[1]);
    return 2;
  }
  const char* decoder_name = argv[1] + 2;
  for(int i = 0; decoder_options[i].name; ++i) {
    if(0 == strcmp(decoder_name, decoder_options[i].name)) {
      dec = decoder_options[i].dec;
      break;
    }
  }
  if(!dec) {
    printf("Unknown decoder %s\n", argv[1]);
    return 2;
  }
  
  Time::set_simulation_mode(true);
  
  int current_arg = 2;

  Dump_Filters filters;
  do {
    if(current_arg >= argc) {
      printf("Input file not specified\n");
      return 2;
    }
    
    if(argv[current_arg][0] != '-') {
      break;
    }
    
    string option = argv[current_arg];
    ++current_arg;
    
    if(option == "--start") {
      filters.start = Time(string(argv[current_arg]));
      ++current_arg;
    }
        
  } while(true);
  
  string input = argv[current_arg];
  ++current_arg;
  
  FILE* file1 = 0;
  
  if(has_gzip_extension(input)) {
    string file_cmd("/bin/zcat ");
    file_cmd += input;
    file1 = popen(file_cmd.c_str(), "r");
  } else if(has_7zip_extension(input)) {
    string file_cmd("/tools/p7zip/redhat/x_64/9.20.1/bin/7z x -so -bd -y ");
    file_cmd += input;
    file1 = popen(file_cmd.c_str(), "r");
  } else if(has_xz_extension(input)) {
    string file_cmd("/usr/bin/xzcat ");
    file_cmd += input;
    file1 = popen(file_cmd.c_str(), "r");
  } else if (input == "-") {
    file1 = stdin;
  } else {
    file1 = fopen(input.c_str(), "r");
  }
  
  if(!file1) {
    printf("%s not found\n", input.c_str());
    return 2;
  }
  
  setvbuf(file1, 0, _IOFBF, 65536);
  
  FILE* outfile = stdout;
  
  if(argc > current_arg) {
    string output = argv[current_arg];
    ++current_arg;
    
    if(output == "-") {
      outfile = stdout;
    } else {
      outfile = fopen(output.c_str(), "w");
      if (!outfile) {
        fprintf(stderr, "output file %s not found\n", output.c_str());
        return 3;
      }
    }
  }
  
  Application app;
  //app.start();
  
  Decode_Handler *m = new Decode_Handler(&app, dec, filters, outfile);
  
  if(0 == strcmp(decoder_name, "euronext_xdp_refdata")) {
    m->m_suppress_qrec = true;
    Euronext_XDP_Handler::dump_refdata_headers(*m->m_ilogger);
  }

  bool use_short_dump = string(decoder_name).find("_short") != string::npos;
 
  if(use_short_dump)
  {
    m->m_ilogger->printf("qrec_time,dir,msg_type,symbol,order_id,internal_id,side,shares,price,orig_order_id,orig_internal_id,exec_shares,exec_price,liq_flag\n");
  }
 
  uint64_t max_context = 128;
  
  m->m_channel_info_map.resize(64);
  for(int i = 0; i < 64; ++i) {
    char buf[64];
    snprintf(buf, 64, "#%d", i);
    m->m_channel_info_map[i].name = buf;
    m->m_channel_info_map[i].context = i;
  }
  
  char buffer[1 * 1024 * 1024];
  
  while(true) {
    bool   long_header_mode = false;
    size_t header_len = sizeof(record_header);
    
    size_t r = fread(buffer, header_len, 1, file1);
    if(!r) {
      break;
    }
    
  reread:
    if(strncmp(buffer, record_header_title, 4) != 0) {
      if(strncmp(buffer, long_record_header_title, 4) == 0) {
        long_header_mode = true;
        header_len = sizeof(long_record_header);
      } else {
        if(!check_record_header(file1, buffer, max_context)) {
          fprintf(stderr, "file1 - Problem in record header title\n");
          return 10;
        }
        goto reread;
      }
    }
    
    if(long_header_mode) {
      r = fread(buffer+sizeof(record_header), sizeof(long_record_header) - sizeof(record_header), 1, file1);
      if(!r) {
        break;
      }
    }
    
    size_t msg_len = 0;
    if (long_header_mode) {
      long_record_header *p = reinterpret_cast<long_record_header*>(buffer);
      msg_len = p->len;
    } else {
      record_header *p = reinterpret_cast<record_header*>(buffer);
      msg_len = p->len;
    }
    
    r = fread(buffer + header_len, msg_len, 1, file1);
    if(!r) {
      fprintf(stderr, "Short last record in file\n");
      return 12;
    }

    if(!long_header_mode) {
      m->parse_qRec(0, buffer, header_len + msg_len, 1, use_short_dump);
    } else {
      m->parse_qRel(0, buffer, header_len + msg_len, 1);
    }
  }
  
  fclose(file1);
  file1 = 0;
  
  delete m;
  m = 0;

  return 0;
}

