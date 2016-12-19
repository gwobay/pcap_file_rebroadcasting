#include <Data/LZO_Util.h>
#include <cstdio>
#include <netinet/in.h>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

namespace hf_core {
  using namespace std;

  class Bytep_Wrap
  {
  public:
    Bytep_Wrap(lzo_bytep bp)
      : m_bp(bp)
    {
    }
    ~Bytep_Wrap()
    {
      lzo_free(m_bp);
    }
    lzo_bytep& get()
    {
      return m_bp;
    }
  private:
    lzo_bytep m_bp;    
  };

  lzo_uint LZO_Util::xwrite(FILE *fp, const lzo_voidp buf, lzo_uint len)
  {
    if (fp != NULL && lzo_fwrite(fp, buf, len) != len) {
      m_logger->log_printf(Log::ERROR, "LZO_Util::xwrite: write error (disk full?)");
      return 0;
    }
    return len;
  }

  void LZO_Util::xwrite32(FILE *fp, lzo_xint v)
  {
    lzo_uint32 b = htonl(v);
    xwrite(fp, &b, 4);
  }


  void LZO_Util::xputc(FILE *fp, int c)
  {
    unsigned char cc = (unsigned char)(c & 0xff);
    xwrite(fp, (const lzo_voidp) &cc, 1);
  }

  lzo_uint LZO_Util::xread(FILE *fp, lzo_voidp buf, lzo_uint len, lzo_bool allow_eof)
  {
    lzo_uint l;

    l = (lzo_uint) lzo_fread(fp, buf, len);
    if (l > len) {
      m_logger->log_printf(Log::ERROR, "LZO_Util::xread: Something is wrong with your C library!");
      return 0;
    }
    if (l != len && !allow_eof) {
      m_logger->log_printf(Log::ERROR, "LZO_Util::xread: Premature EOF!");
      return 0;
    }
    return l;
  }


  lzo_uint32 LZO_Util::xread32(FILE *fp)
  {
    lzo_uint32 b;
    xread(fp, &b, 4, 0);
    lzo_uint32 v = ntohl(b);
    return v;
  }

  int LZO_Util::xgetc(FILE *fp)
  {
    unsigned char c;
    xread(fp, (lzo_voidp) &c, 1, 0);
    return c;
  }

  lzo_bytep LZO_Util::allocate_mem(size_t size)
  {
    const char* where = "LZO_Util::allocate_mem";
    const int NUM_MEM_ALLOC_TRIES_BEFORE_FAILURE = 5;
    const int RETRY_INTERVAL_SECONDS = 10;
    int num_tries = 0;
    while(num_tries < NUM_MEM_ALLOC_TRIES_BEFORE_FAILURE)
    {
      lzo_bytep ret = (lzo_bytep) lzo_malloc(size);
      if(ret != NULL)
      {
        return ret;
      }
      if(num_tries < NUM_MEM_ALLOC_TRIES_BEFORE_FAILURE - 1)
      {
        m_logger->log_printf(Log::WARN, "%s: could not allocate memory. Waiting %d seconds to retry...", where, RETRY_INTERVAL_SECONDS);
        sleep(RETRY_INTERVAL_SECONDS);
      }
    }
    return NULL;
  }

  bool LZO_Util::compress_file(const string& source_file, const string& output_file,
                               size_t block_size, int level, bool remove_source_file)
  {
    const char* where = "LZO_Util::compress_file";

    FILE* fi = fopen(source_file.c_str(), "rb");
    if (fi == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Could not open file: [%s]", where, source_file.c_str());
      return false;
    }

    FILE* fo = fopen(output_file.c_str(), "wb");
    if (fo == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Could not open file: [%s]", where, output_file.c_str());
      return false;
    }

    int r = 0;
    lzo_uint in_len;
    lzo_uint out_len;
    lzo_uint32 wrk_len = 0;
    lzo_uint32 flags = 1;       /* do compute a checksum */
    int method = 1;             /* compression method: LZO1X */
    lzo_uint32 checksum;

    xwrite(fo, lzo_magic, sizeof(lzo_magic));
    xwrite32(fo, flags);
    xputc(fo, method); /* compression method */
    xputc(fo, level); /* compression level */
    xwrite32(fo, block_size);
    checksum = lzo_adler32(0, NULL, 0);

    Bytep_Wrap in(allocate_mem(block_size));
    if(in.get() == NULL)
    {
      m_logger->log_printf(Log::ERROR, "%s: Out of memory!", where);
      return false;
    }
    Bytep_Wrap out( allocate_mem(block_size + block_size / 16 + 64 + 3));
    if(out.get() == NULL)
    {
      m_logger->log_printf(Log::ERROR, "%s: Out of memory!", where);
      return false;
    }
    if (level == 9)
      wrk_len = LZO1X_999_MEM_COMPRESS;
    else
      wrk_len = LZO1X_1_MEM_COMPRESS;
    Bytep_Wrap wrkmem(allocate_mem(wrk_len));
    if (wrkmem.get() == NULL) {
      m_logger->log_printf(Log::ERROR, "%s: Out of memory!", where);
      return false;
    }

    for (;;) {
      /* read block */
      in_len = xread(fi, in.get(), block_size, 1);
      if (in_len <= 0)
        break;

      /* update checksum */
      checksum = lzo_adler32(checksum, in.get(), in_len);

      /* compress block */
      if (level == 9)
        r = lzo1x_999_compress(in.get(), in_len, out.get(), &out_len, wrkmem.get());
      else
        r = lzo1x_1_compress(in.get(), in_len, out.get(), &out_len, wrkmem.get());

      if (r != LZO_E_OK || out_len > in_len + in_len / 16 + 64 + 3) {
        /* this should NEVER happen */
        m_logger->log_printf(Log::ERROR, "%s: Compression failed (code=%d)", where, r);
        return false;
      }
      m_logger->log_printf(Log::DEBUG, "%s: %s (in_bytes=%lu out_bytes=%lu)",
                           where, source_file.c_str(), in_len, out_len);

      /* write uncompressed block size */
      xwrite32(fo, in_len);

      if (out_len < in_len) {
        /* write compressed block */
        xwrite32(fo, out_len);
        xwrite(fo, out.get(), out_len);
      }
      else {
        /* not compressible - write uncompressed block */
        xwrite32(fo, in_len);
        xwrite(fo, in.get(), in_len);
      }
    }

    /* write EOF marker */
    xwrite32(fo, 0);

    /* write checksum */
    xwrite32(fo, checksum);

    fclose(fi);
    fclose(fo);
    if (remove_source_file)
      remove(source_file.c_str());

    return true;
  }


  bool LZO_Util::decompress_file_start(const string& source_file)
  {
    const char* where = "LZO_Util::decompress_file_start";
    m_file_name = source_file;
    m_file = fopen(m_file_name.c_str(), "r");

    if (!m_file) {
      m_logger->log_printf(Log::ERROR, "%s: Could not open file: [%s]", where, m_file_name.c_str());
      return false;
    }

    unsigned char m [ sizeof(lzo_magic) ];
    if (xread(m_file, m, sizeof(lzo_magic), 1) != sizeof(lzo_magic) ||
        memcmp(m, lzo_magic, sizeof(lzo_magic)) != 0) {
      m_logger->log_printf(Log::ERROR, "%s: Invalid header, file may not be compressed", where);
      return false;
    }

    lzo_uint32 flags = xread32(m_file);
    int method = xgetc(m_file);
    int level = xgetc(m_file);
    lzo_uint block_size = xread32(m_file);
    m_logger->log_printf(Log::DEBUG, "%s: %s header info (flags=%u method=%d level=%d block_size=%lu)",
                         where, m_file_name.c_str(), flags, method, level, block_size);
    return true;
  }


  bool LZO_Util::decompress_file_close()
  {
    if (m_file) {
      fclose(m_file);
      m_file = 0;
    }
    return true;
  }

  static const size_t max_offset = 1024;

  void LZO_Util::realloc_buffer(size_t new_size, char* buffer_start, size_t offset)
  {
    const char* where = "LZO_Util::realloc_buffer";

    if(m_buf_len < (new_size + max_offset)) {
      m_logger->log_printf(Log::DEBUG, "%s: Increasing buffer_size from %zu to %zu", where, m_buf_len, (new_size + max_offset));
      lzo_bytep new_buf = new lzo_byte[new_size + max_offset];
      if(offset > 0) {
        m_logger->log_printf(Log::DEBUG, "%s: Moving %zu bytes to new buffer start", where, offset);
        memmove((char*)new_buf, buffer_start, offset);
      }
      delete [] m_buffer;
      m_buffer = new_buf;
      m_buf_len = new_size + max_offset;
    } else {
      if(offset > 0) {
        m_logger->log_printf(Log::DEBUG, "%s: Moving %zu bytes to buffer start", where, offset);
        memmove((char*)m_buffer, buffer_start, offset);
      }
    }
  }

  bool LZO_Util::decompress_file_get_next_block_size()
  {
    m_uncompressed_length = xread32(m_file);
    m_compressed_length = xread32(m_file);
    return true;
  }


  bool LZO_Util::decompress_file_next_block(char** buffer_start, char** buffer_end)
  {
    const char* where = "LZO_Util::decompress_file_next_block";
    size_t offset = 0;
    if (*buffer_start && *buffer_end && *buffer_start < *buffer_end) {
      offset = *buffer_end - *buffer_start;
      m_logger->log_printf(Log::DEBUG, "%s: Found %zu remaining bytes", where, offset);
      if (offset > max_offset)
        return false;
    }

    if (m_uncompressed_length == 0) {
      m_logger->log_printf(Log::DEBUG, "%s: FILE=%s (EOF)", where, m_file_name.c_str());
      buffer_start = buffer_end;
      return true;
    }

    if(m_compressed_length < m_uncompressed_length) {
      realloc_buffer(m_uncompressed_length, *buffer_start, offset);
      lzo_bytep in_buffer = new lzo_byte[m_compressed_length];
      xread(m_file, in_buffer, m_compressed_length, 0);
      lzo_uint new_len = m_uncompressed_length;
      int r = lzo1x_decompress_safe(in_buffer, m_compressed_length, m_buffer+offset, &new_len, NULL);
      delete[] in_buffer;

      if (r != LZO_E_OK || new_len != m_uncompressed_length) {
        m_logger->log_printf(Log::ERROR, "%s: Unsuccessful decompression (code=%d)", where, r);
        m_logger->log_printf(Log::ERROR, "%s: FILE=%s LZO_IN_LEN=%lu LZO_OUT_LEN=%lu",
                             where, m_file_name.c_str(), m_compressed_length, m_uncompressed_length);
        return false;
      }
      m_logger->log_printf(Log::DEBUG, "%s: FILE=%s LZO_IN_LEN=%lu LZO_OUT_LEN=%lu", where, m_file_name.c_str(), m_compressed_length, m_uncompressed_length);
    } else {
      realloc_buffer(m_compressed_length, *buffer_start, offset);
      xread(m_file, m_buffer, m_compressed_length, 0);
      m_logger->log_printf(Log::DEBUG, "%s: FILE=%s (UNCOMPRESSED BLOCK) LZO_IN_LEN=%lu LZO_OUT_LEN=%lu",
                           where, m_file_name.c_str(), m_compressed_length, m_uncompressed_length);
    }
    *buffer_start = (char*)m_buffer;
    *buffer_end = *buffer_start + std::max(m_compressed_length, m_uncompressed_length) + offset;

    return true;
  }

  LZO_Util::LZO_Util(Application* app)
    : Compression_Util(app), m_file(0), m_buffer(0), m_buf_len(0)
  {
    m_logger = m_app->lm()->get_logger("");
  }

  LZO_Util::LZO_Util(Application* app, LoggerRP logger)
    : Compression_Util(app, logger), m_file(0), m_buffer(0), m_buf_len(0)
  {
  }


  LZO_Util::~LZO_Util()
  {
    delete [] m_buffer;
  }

}
