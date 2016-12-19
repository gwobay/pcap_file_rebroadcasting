#pragma once
#include <boost/function.hpp>

#include <Logger/Logger.h>
#include <Data/QRec.h>

namespace hf_core {

  typedef boost::function<size_t (hf_tools::ILogger& log,  hf_core::Dump_Filters& filter, size_t context, const char* packet_buf, size_t packet_len)>  Decoder;

  struct decoder_options_t {
    const char* name;
    Decoder dec;
    const char* help_str;
  };

  class Dump_Logger : public ILogger {
  public:
    virtual void puts(const char* str) { ::puts(str); }
    virtual void printf(const char* str, ...)  __attribute__ ((format (printf, 2, 3))) {
      va_list va;
      va_start(va, str);
      vfprintf(m_out, str, va);
      va_end(va);
    }
    
    virtual void vprintf(const char* str, va_list ap) {
      ::vprintf(str, ap);
    }

    Dump_Logger() : m_out(stdout) { }
    Dump_Logger(FILE *out) : m_out(out) { }

    virtual ~Dump_Logger() { }
  private:
    FILE *m_out;
  };

  size_t binary_dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len)
  {
    log.printf(" %4lu bytes", len);
    for (size_t b = 0; b != len; b++) {
      log.printf(" %8lu", b);
    }
    log.printf("\n                                             ");
    for (size_t b = 0; b != len; b++) {
      for (size_t s = 8; s != 0; s--) {
        log.printf("%d", (buf[b] >> (s-1)) & 1);
      }
      log.printf(" ");
    }
    log.printf("\n");
    return len;
  }


}
