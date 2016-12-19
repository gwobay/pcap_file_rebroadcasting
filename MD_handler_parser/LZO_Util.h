#ifndef hf_data_LZO_Util_H
#define hf_data_LZO_Util_H

#include <cstring>
#include <iostream>
#include <stdlib.h>

#include <lzo/lzoconf.h>
#include <lzo/lzoutil.h>
#include <lzo/lzo1x.h>

#include <app/app.h>
#include <Data/Compression_Util.h>
#include <Logger/Logger.h>
#include <Logger/Logger_Manager.h>

#define WANT_LZO_MALLOC 1
#define WANT_LZO_FREAD 1
#define WANT_LZO_WILDARGV 1

namespace hf_core {
  using namespace std;

  static const long lzo_max_block_size = 512 * 1024 * 1024L;
  static const unsigned char lzo_magic[7] =
      { 0x00, 0xe9, 0x4c, 0x5a, 0x4f, 0xff, 0x1a };

  struct LZO_Header {
    char     magic[7];
    uint32_t flags;
    char     method;
    char     level;
    uint32_t block_size;
  } __attribute__((packed));

  struct LZO_Block_Header {
    uint32_t out_len;
    uint32_t in_len;
  } __attribute__((packed));

  class LZO_Util : public Compression_Util {
  public:
    LZO_Util(Application* app);
    LZO_Util(Application* app, LoggerRP logger);

    virtual ~LZO_Util();

    bool compress_file(const string& input_file, const string& output_file,
                       size_t block_size, int level, bool remove_source_file);
    bool decompress_file_start(const string& input_file);
    bool decompress_file_get_next_block_size();  // This sets up the size
    bool decompress_file_next_block(char** buffer_start, char** buffer_end);
    bool decompress_file_close();
    lzo_uint xwrite(FILE *fp, const lzo_voidp buf, lzo_uint len);
    void xwrite32(FILE *fp, lzo_xint v);
    void xputc(FILE *fp, int c);
    lzo_uint xread(FILE *fp, lzo_voidp buf, lzo_uint len, lzo_bool allow_eof);
    lzo_uint32 xread32(FILE *fp);
    int xgetc(FILE *fp);
    void realloc_buffer(size_t new_size, char* buffer_start, size_t offset);

    size_t size() const { return m_buf_len; }

  private:
    lzo_bytep allocate_mem(size_t size);

    string    m_file_name;
    FILE*     m_file;
    lzo_bytep m_buffer;
    size_t    m_buf_len;
  };

  typedef boost::shared_ptr<LZO_Util> LZO_Util_RP;
}

#endif // hf_data_LZO_Util_H
