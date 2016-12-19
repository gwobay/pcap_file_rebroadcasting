#include <Data/Handler.h>
#include <Data/MDBufferQueue.h>
#include <Data/NYSE_Handler.h>

#include <Util/File_Utils.h>

using namespace std;


using namespace hf_tools;
using namespace hf_core;

struct rec {
  struct record_header rh;
  char buf[32 * 1024];
}  __attribute__ ((packed));

//static bool searched = false;

class Merge_Handler : public Handler {
public:

  FILE* m_output;

public:
  Merge_Handler(Application* app, FILE* output, const string& handler_type) :
    Handler(app, handler_type), m_output(output)
  {
    Logger_Config lc;
    lc.name = "stderr";
    lc.filename = "/dev/stderr";

    m_logger = app->lm()->create_logger(lc);
  }

  virtual const string& name() const { return m_handler_type; }

  virtual size_t parse(size_t context, const char* buf, size_t len) {
    throw runtime_error("Merge_Handler::parse(context, buf, len) called");
  }

  virtual size_t parse(size_t context, const char* buf, size_t len, int fileno) = 0;

  virtual size_t parse2(size_t context, const char* buf, size_t len)
  {
    //record_header* rh = const_cast<record_header*>(reinterpret_cast<const record_header*>(buf));
    //rh->ticks = Time::current_time().ticks();

    fwrite(buf, len, 1, m_output);
    return 0;
  }

  virtual size_t read(size_t context, const char* buf, size_t len)
  {
    return len;
  }

  virtual void reset(const char* msg)
  {
    m_logger->log_printf(Log::ERROR, "%s", msg);
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


class NYSE_Merger : public Merge_Handler {
public:

  NYSE_Merger(Application* app, FILE* output) :
    Merge_Handler(app, output, "NYSE_Merger")
  {
  }

  size_t parse(size_t context, const char* buf, size_t len, int fileno)
  {
    const char* where = "NYSE_Handler::parse";

    const record_header* rh = reinterpret_cast<const record_header*>(buf);
    const NYSE_structs::message_header *header = reinterpret_cast<const NYSE_structs::message_header*>(buf+sizeof(record_header));

    Time::set_current_time(Time(rh->ticks));

    uint16_t msg_type = ntohs(header->msg_type);
    uint32_t seq_no = ntohl(header->seq_num);
    uint32_t next_seq_no = seq_no + 1;

    //fprintf(stderr, "fileno %d  context %zd, seqno %d\n", fileno, context, seq_no);

    channel_info& ci = m_channel_info_map[context];

    if(msg_type == 1) {
      // sequence number reset
      const NYSE_structs::sequence_reset_message& msg = reinterpret_cast<const NYSE_structs::sequence_reset_message&>(*(buf + sizeof(record_header) + sizeof(NYSE_structs::message_header)));
      uint32_t new_seq_no = ntohl(msg.new_seq_no);
      fprintf(stderr, "%s: received sequence reset message orig=%d  new=%d\n", where, seq_no, new_seq_no);
      ci.seq_no = new_seq_no;
      ci.clear();
      return len;
    }

    if(seq_no < ci.seq_no) {
      return 0; // duplicate
    } else if(seq_no > ci.seq_no) {
      if(ci.begin_recovery(this, buf, len, seq_no, next_seq_no)) {
        //m_logger->log_printf(Log::INFO,  "begin recovery  fileno %d line %s seq_no = %d  ci.seq_no = %zd", fileno, ci.name.c_str(), seq_no, ci.seq_no);
        return len;
      }
    }

    this->parse2(context, buf, len);

    ci.seq_no = next_seq_no;
    ci.last_update = Time::currentish_time();

    if(!ci.queue.empty()) {
      ci.process_recovery(this);
    }

    return len;
  }

};


class Mold_Merger : public Merge_Handler {
public:
  struct downstream_header_t {
    char      session[10];
    uint64_t  seq_num;
    uint16_t  message_count;
    //message_block_t message_block[0];
  } __attribute__ ((packed));

  channel_info m_channel_info;

  Mold_Merger(Application* app, FILE* output) :
    Merge_Handler(app, output, "Mold_Merger")
  {
  }

  size_t parse(size_t context, const char* buf, size_t len, int fileno)
  {
    //const char* where = "MoldUDP64_Handler::parse";

    const record_header* rh = reinterpret_cast<const record_header*>(buf);
    const downstream_header_t *header = reinterpret_cast<const downstream_header_t*>(buf+sizeof(record_header));

    Time::set_current_time(Time(rh->ticks));

    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    uint64_t next_seq_num = seq_num + message_count;
    if(message_count == 0) {
      ++next_seq_num;  // for heartbeats
    }

    if(seq_num > m_channel_info.seq_no) {
      if(m_channel_info.begin_recovery(this, buf, len, seq_num, next_seq_num)) {
        return 0;
      }
    } else if(next_seq_num <= m_channel_info.seq_no) {
      return 0;
    }

    size_t parsed_len __attribute__ ((unused)) = this->parse2(context, buf, len);
    m_channel_info.seq_no = next_seq_num;
    m_channel_info.last_update = Time::currentish_time();

    if(!m_channel_info.queue.empty()) {
      m_channel_info.process_recovery(this);
    }

    return len;
  }
};


class PITCH_Merger : public Merge_Handler {
public:

  struct message_header {
    uint16_t hdr_length;
    uint8_t  hdr_count;
    uint8_t  hdr_unit;
    uint32_t hdr_sequence;
  } __attribute__ ((packed));


  PITCH_Merger(Application* app, FILE* output) :
    Merge_Handler(app, output, "PITCH_Merger")
  {
    m_channel_info_map.resize(64);
  }

  size_t parse(size_t context, const char* buf, size_t len, int fileno)
  {
    const char* where = "PITCH_2_9_Handler::parse";

    if(len < sizeof(message_header)) {
      send_alert("%s: %s Buffer of size %zd smaller than minimum packet length %zd", where, "merger", len, sizeof(message_header));
      return 0;
    }

    const record_header* rh = reinterpret_cast<const record_header*>(buf);
    Time::set_current_time(Time(rh->ticks));

    const message_header& header = reinterpret_cast<const message_header&>(*(buf+sizeof(record_header)));

    while(header.hdr_unit >= m_channel_info_map.size()) {
      m_channel_info_map.push_back(channel_info("", m_channel_info_map.size() + 1));
      m_logger->log_printf(Log::INFO, "%s: %s increasing channel_info_map due to new unit %d", where, "merger", header.hdr_unit);
    }

    channel_info& ci = m_channel_info_map[header.hdr_unit];

    uint32_t seq_no = header.hdr_sequence;
    uint32_t last_seq_no = seq_no + header.hdr_count;

    if(seq_no == 0) {
      parse2(header.hdr_unit, buf, len);
      return len;
    }

    if(seq_no > ci.seq_no) {
      if(ci.begin_recovery(this, buf, len, seq_no, last_seq_no)) {
        return len;
      }
    } else if(last_seq_no <= ci.seq_no) {
      // duplicate
      return len;
    }

    parse2(header.hdr_unit, buf, len);
    ci.seq_no = last_seq_no;
    ci.last_update = Time::currentish_time();

    if(!ci.queue.empty()) {
      ci.process_recovery(this);
    }

    return len;

  }
};




void
print_stats(Logger& logger, uint64_t from_f1, uint64_t from_f2)
{
  double pct =  100.0 * (double) from_f1 / (double) (from_f1 + from_f2);

  logger.log_printf(Log::INFO, "%zd (%0.2f%%) packets copied from file1,  %zd from file2", from_f1, pct, from_f2);
}


bool
check_record_header(FILE* file_stream, rec& rec_buf, uint64_t max_context)
{
  if (strncmp(rec_buf.rh.title, record_header_title, 4) != 0 || rec_buf.rh.context > max_context) {
    fprintf(stderr, "Mangled record header detected...\n");
    size_t r;
    int byte_counter = 0;
    while (1) {
      int i = 0;
      for (; i < 4; i++) {
        r = fread(&rec_buf.rh.title[i], 1, 1, file_stream);
        ++byte_counter;
        if (!r || rec_buf.rh.title[i] != record_header_title[i]) {
          break;
        }
      }

      if (!r) {
        fprintf(stderr, "Error while looking for good record header title, possibly EOF\n");
        break;
      } else if (i == 4) {
        fprintf(stderr, "Found good record header title, reading rest of record header\n");
        r = fread(((char*)&rec_buf.rh)+4, sizeof(record_header)-4, 1, file_stream);
        byte_counter += (sizeof(record_header)-4);
        if (rec_buf.rh.context > max_context) {
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
  const char* usage = "Usage: %s [--mold|--nyse] file1 file2 output\n";

  if(argc < 5) {
#ifdef CW_DEBUG
    printf(usage, argv[0]);
#endif
    return 1;
  }

  Time::set_simulation_mode(true);

  FILE* file1 = 0;
  FILE* file2 = 0;
  FILE* output = 0;

  file1 = fopen(argv[2], "r");
  file2 = fopen(argv[3], "r");

  if (!file1 and !file2) {
#ifdef CW_DEBUG
    printf("Neither input file exists!\n");
#endif
    return 10;
  } else if (!file1) {
#ifdef CW_DEBUG
    printf("Could not open input file: %s\n", argv[2]);
#endif
#ifdef CW_DEBUG
    printf("Exiting successfully without merging...\n");
#endif
    fclose(file2);
    return 0;
  } else if (!file2) {
#ifdef CW_DEBUG
    printf("Could not open input file: %s\n", argv[3]);
#endif
#ifdef CW_DEBUG
    printf("Exiting successfully without merging...\n");
#endif
    fclose(file1);
    return 0;
  }

  fseek(file1, 0L, SEEK_END);
  size_t sz1 = ftell(file1);
  fclose(file1);

  fseek(file2, 0L, SEEK_END);
  size_t sz2 = ftell(file2);
  fclose(file2);

  if (sz1 < 1000 and sz2 < 1000) {
#ifdef CW_DEBUG
    printf("Both input files are too small (< 1000 bytes)!\n");
#endif
    return 10;
  } else if (sz1 < 1000) {
#ifdef CW_DEBUG
    printf("Input file1 too small: %s\n", argv[2]);
#endif
#ifdef CW_DEBUG
    printf("Copying file2 as merged file...\n");
#endif

    char file_cmd[128];
    sprintf(file_cmd, "cp %s %s", argv[3], argv[4]);
    if (system(file_cmd)) {
#ifdef CW_DEBUG
      printf("Error running command: %s\n", file_cmd);
#endif
      return 10;
    }
    return 0;
  } else if (sz2 < 1000) {
#ifdef CW_DEBUG
    printf("Input file2 too small: %s\n", argv[3]);
#endif
#ifdef CW_DEBUG
    printf("Copying file1 as merged file...\n");
#endif

    char file_cmd[128];
    sprintf(file_cmd, "cp %s %s", argv[2], argv[4]);
    if (system(file_cmd)) {
#ifdef CW_DEBUG
      printf("Error running command: %s\n", file_cmd);
#endif
      return 10;
    }
    return 0;
  }


  rec buf1;
  rec buf2;

  if(has_gzip_extension(argv[2])) {
    string file_cmd("/bin/zcat ");
    file_cmd += argv[2];
    file1 = popen(file_cmd.c_str(), "r");
  } else {
    file1 = fopen(argv[2], "r");
  }

  if(has_gzip_extension(argv[3])) {
    string file_cmd("/bin/zcat ");
    file_cmd += argv[3];
    file2 = popen(file_cmd.c_str(), "r");
  } else {
    file2 = fopen(argv[3], "r");
  }

  setvbuf(file1, 0, _IOFBF, 65536);
  setvbuf(file2, 0, _IOFBF, 65536);

  if(has_gzip_extension(argv[4])) {
    string file_cmd("gzip > ");
    file_cmd += argv[4];
    output = popen(file_cmd.c_str(), "w");
  } else {
    output = fopen(argv[4], "w");
  }
  setvbuf(output, 0, _IOFBF, 65536);

  Application app;
  app.start();

  Merge_Handler *m = 0;

  if(0 == strcmp(argv[1], "--mold")) {
    m = new Mold_Merger(&app, output);
  } else if(0 == strcmp(argv[1], "--nyse")) {
    m = new NYSE_Merger(&app, output);
  } else if(0 == strcmp(argv[1], "--pitch")) {
    m = new PITCH_Merger(&app, output);
  } else {
#ifdef CW_DEBUG
    printf("Unknown flag '%s'.  Usage: %s", argv[0], usage);
#endif
    return 1;
  }

  uint64_t max_context = 128;
  if (argc == 6) {
    max_context = (uint64_t)strtol(argv[5], 0, 10);
#ifdef CW_DEBUG
    printf("Using %lu as max context\n", max_context);
#endif
  }

  m->m_channel_info_map.resize(64);
  for(int i = 0; i < 64; ++i) {
    char buf[64];
    snprintf(buf, 64, "#%d", i);
    m->m_channel_info_map[i].name = buf;
    m->m_channel_info_map[i].context = i;
  }

  size_t r = fread(&buf1.rh, sizeof(record_header), 1, file1);
  if(r)  r = fread(buf1.buf, buf1.rh.len, 1, file1);
  if(!r) {
    fclose(file1);
    file1 = 0;
  }

  r = fread(&buf2.rh, sizeof(record_header), 1, file2);
  if(r) r = fread(buf2.buf, buf2.rh.len, 1, file2);
  if(!r) {
    fclose(file2);
    file2 = 0;
  }

  //vector<uint64_t> seq_nos(32, 0);

  uint64_t from_f1 = 0;
  uint64_t from_f2 = 0;

  uint64_t next_print = 0;

  //char rh_title_buf[4];

  while(file1 || file2) {
    if(file1 && strncmp(buf1.rh.title, record_header_title, 4) != 0) {
      if (!check_record_header(file1, buf1, max_context)) {
        fprintf(stderr, "file1 - Problem in record header title\n");
        return 10;
      }
    }
    if(file2 && strncmp(buf2.rh.title, record_header_title, 4) != 0) {
      if (!check_record_header(file2, buf2, max_context)) {
        fprintf(stderr, "file2 - Problem in record header title\n");
        return 10;
      }
    }
    
    if(file1 && buf1.rh.ticks < buf2.rh.ticks) {
      if(m->parse(buf1.rh.context, (const char*) &buf1,  sizeof(record_header) + buf1.rh.len, 1)) {
        ++from_f1;
      }
      
      size_t r = fread(&buf1.rh, sizeof(record_header), 1, file1);
      if(r && check_record_header(file1, buf1, max_context)) {
        r = fread(buf1.buf, buf1.rh.len, 1, file1);
      }
      if (!r) {
        fclose(file1);
        file1 = 0;
      }
    } else if(file2) {
      if(m->parse(buf2.rh.context, (const char*) &buf2,  sizeof(record_header) + buf2.rh.len, 2)) {
        ++from_f2;
      }

      size_t r = fread(&buf2.rh, sizeof(record_header), 1, file2);
      if(r && check_record_header(file2, buf2, max_context)) {
        r = fread(buf2.buf, buf2.rh.len, 1, file2);
      }
      if(!r) {
        fclose(file2);
        file2 = 0;
        buf2.rh.ticks = std::numeric_limits<uint64_t>::max();
      }
    }
    
    if(file1 && buf1.rh.ticks > next_print) {
      print_stats(m->logger(), from_f1, from_f2);
      next_print = buf1.rh.ticks + minutes(30).ticks();
    }
    if(file2 && buf2.rh.ticks > next_print) {
      print_stats(m->logger(), from_f1, from_f2);
      next_print = buf2.rh.ticks + minutes(30).ticks();
    }
  }

  print_stats(m->logger(), from_f1, from_f2);

  delete m;
  m = 0;

  return 0;
}

