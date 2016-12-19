#include <Data/Snappy_Util.h>
#include <cstdio>
#include <netinet/in.h>

namespace hf_core {
  using namespace std;

  static const size_t max_offset = 1024;

  bool Snappy_Util::compress_file(const string& source_file, const string& output_file,
                                  size_t block_size, int level, bool remove_source_file)
  {
#ifdef CW_DEBUG
    cout << "check1" <<endl;
#endif
    const char* where = "Snappy_Util::compress_file";
    m_logger->log_printf(Log::INFO, "TEST");

    FILE* in_file = fopen(source_file.c_str(), "rb");
    if (in_file == NULL) {
#ifdef CW_DEBUG
      cout << "checkA" << endl;
#endif
      m_logger->log_printf(Log::ERROR, "%s: Could not open file: [%s]",
                           where, source_file.c_str());
      return false;
    }

#ifdef CW_DEBUG
    cout << "check2" <<endl;
#endif

    FILE* out_file = fopen(output_file.c_str(), "wb");
    if (out_file == 0) {
      m_logger->log_printf(Log::ERROR, "%s: Could not open file: [%s]",
                           where, output_file.c_str());
      return false;
    }

    char* buffer = new char[block_size];

    string output;
    while (true) {
      size_t bytes_read = fread(buffer, sizeof(char), block_size, in_file);

      output.clear();
      size_t compressed_length = snappy::Compress(buffer, bytes_read, &output);

      // Write uncompressed byte size, then compressed byte size, then compressed data
      fwrite(&bytes_read, sizeof(size_t), 1, out_file);
      fwrite(&compressed_length, sizeof(size_t), 1, out_file);
      fwrite(output.c_str(), 1, compressed_length, out_file);

      if (bytes_read < block_size)
        break;
    }

    fclose(in_file);
    fclose(out_file);

    delete[] buffer;

    if (remove_source_file)
      remove(source_file.c_str());


    return true;
  }


  bool Snappy_Util::decompress_file_start(const string& source_file)
  {
    const char* where = "Snappy_Util::decompress_file_start";
    m_file_name = source_file;
    m_file = fopen(m_file_name.c_str(), "r");

    if (!m_file) {
      m_logger->log_printf(Log::ERROR, "%s: Could not open file: [%s]",
                           where, m_file_name.c_str());
      return false;
    }

    return true;
  }


  bool Snappy_Util::decompress_file_close()
  {
    if (m_file) {
      fclose(m_file);
      m_file = 0;
    }
    return true;
  }

  void Snappy_Util::realloc_buffer(size_t new_size, char* buffer_start, size_t offset)
  {
    const char* where = "Snappy_Util::realloc_buffer";

    if(m_buf_len < (new_size + max_offset)) {
      m_logger->log_printf(Log::DEBUG, "%s: Increasing buffer_size from %zu to %zu", where, m_buf_len, (new_size + max_offset));
      char* new_buf = new char[new_size + max_offset];
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

  bool Snappy_Util::decompress_file_get_next_block_size()
  {
    size_t bytes_read = fread(&m_uncompressed_length, sizeof(size_t), 1, m_file);
    if (!bytes_read) {
      return false;
    }

    bytes_read = fread(&m_compressed_length, sizeof(size_t), 1, m_file);
    if (!bytes_read) {
      return false;
    }

    return true;
  }


  bool Snappy_Util::decompress_file_next_block(char** buffer_start, char** buffer_end)
  {
    const char* where = "Snappy_Util::decompress_file_next_block";
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

    realloc_buffer(m_uncompressed_length, *buffer_start, offset);

    char* compressed_buffer = new char[m_compressed_length];
    size_t bytes_read = fread(compressed_buffer, sizeof(char), m_compressed_length, m_file);
    if (bytes_read != m_compressed_length) {
      m_logger->log_printf(Log::ERROR, "%s: bytes read (%zu) less than expected (%zu)",
                           where, bytes_read, m_compressed_length);
    }

    if (!snappy::RawUncompress(compressed_buffer, m_compressed_length, m_buffer))
    {
      m_logger->log_printf(Log::ERROR, "%s: Could not uncompress buffer", where);
      return false;
    }
    delete[] compressed_buffer;

    *buffer_start = (char*)m_buffer;
    *buffer_end = *buffer_start + m_uncompressed_length + offset;

    return true;
  }


  Snappy_Util::Snappy_Util(Application* app)
    : Compression_Util(app), m_file(0), m_buffer(0), m_buf_len(0)
  {
    m_logger = m_app->lm()->get_logger("");
  }

  Snappy_Util::Snappy_Util(Application* app, LoggerRP logger)
    : Compression_Util(app, logger), m_file(0), m_buffer(0), m_buf_len(0)
  {
  }

  Snappy_Util::~Snappy_Util()
  {
    delete [] m_buffer;
  }

}
