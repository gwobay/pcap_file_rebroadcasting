#ifndef hf_data_Snappy_Util_H
#define hf_data_Snappy_Util_H

#include <cstring>
#include <iostream>
#include <stdlib.h>

#include <snappy.h>

#include <app/app.h>
#include <Data/Compression_Util.h>
#include <Logger/Logger.h>
#include <Logger/Logger_Manager.h>

namespace hf_core {
  using namespace std;

  class Snappy_Util : public Compression_Util {
  public:
    Snappy_Util(Application* app);
    Snappy_Util(Application* app, LoggerRP logger);

    virtual ~Snappy_Util();

    bool compress_file(const string& input_file, const string& output_file,
                       size_t block_size, int level, bool remove_source_file);
    bool decompress_file_start(const string& input_file);
    bool decompress_file_get_next_block_size();  // This sets up the size
    bool decompress_file_next_block(char** buffer_start, char** buffer_end);
    bool decompress_file_close();

  protected:
    void realloc_buffer(size_t new_size, char* buffer_start, size_t offset);

    string m_file_name;
    FILE* m_file;
    char* m_buffer;
    size_t m_buf_len;

    size_t m_max_buffer_size;
  };

  typedef boost::shared_ptr<Snappy_Util> Snappy_Util_RP;
}

#endif // hf_data_Snappy_Util_H
