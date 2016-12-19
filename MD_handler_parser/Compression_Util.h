#ifndef hf_data_Compression_Util_H
#define hf_data_Compression_Util_H

#include <cstring>
#include <iostream>
#include <stdlib.h>

#include <app/app.h>
#include <Logger/Logger.h>
#include <Logger/Logger_Manager.h>

namespace hf_core {
  using namespace std;

  class Compression_Util {
  public:
    Compression_Util(Application* app)
    : m_app(app), m_logger(0), m_max_block_size(512*1024*1024),
    m_compressed_length(0), m_uncompressed_length(0)
    {
      m_logger = m_app->lm()->get_logger("");
    }

    Compression_Util(Application* app, LoggerRP logger)
    : m_app(app), m_logger(logger), m_max_block_size(512*1024*1024),
    m_compressed_length(0), m_uncompressed_length(0)
    {
    }
      
    virtual ~Compression_Util() { };
      
    virtual bool compress_file(const string& input_file, const string& output_file,
                               size_t block_size, int level, bool remove_source_file) = 0;
    virtual bool decompress_file_start(const string& input_file) = 0;
    virtual bool decompress_file_get_next_block_size() = 0;  // This sets up the size
    virtual bool decompress_file_next_block(char** buffer_start, char** buffer_end) = 0;
    virtual bool decompress_file_close() = 0;

    size_t get_max_block_size() const { return m_max_block_size; }
    size_t in_buffer_len() const { return m_compressed_length; }
    size_t out_buffer_len() const { return m_uncompressed_length; }
    size_t buffer_len() const { return std::max(m_compressed_length, m_uncompressed_length); }

  protected:
    Application* m_app;
    LoggerRP m_logger;

    size_t m_max_block_size;
    size_t m_compressed_length;
    size_t m_uncompressed_length;

  };

  typedef boost::shared_ptr<Compression_Util> Compression_Util_RP;
}

#endif // hf_data_Compression_Util_H
