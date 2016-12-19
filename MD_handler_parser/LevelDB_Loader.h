#ifndef hf_core_Data_LevelDB_Loader_h
#define hf_core_Data_LevelDB_Loader_h

#include <string>

#include <app/app.h>
#include <Logger/Logger.h>
#include <Logger/Logger_Manager.h>

#include "leveldb/cache.h"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace hf_core {
  using namespace std;

  class LevelDB_Loader
  {
  public:

    LevelDB_Loader(Application* app);
    virtual ~LevelDB_Loader(void);

    void set_create_if_missing(bool c) { m_options.create_if_missing = c; }
    void set_synchronous(bool s) { m_write_options.sync = s; }
    void set_batch_size(uint32_t b) { m_batch_size = b; }

    bool open_db(string db_file_name);
    bool insert(string key, string value);
    bool insert_batch(string key, string value);
    bool finalize_batch();
  private:

    Application* m_app;
    LoggerRP m_logger;

    leveldb::DB* m_leveldb;
    leveldb::WriteBatch* p_batch;
    leveldb::Status m_status;
    leveldb::Options m_options;
    leveldb::WriteOptions m_write_options;

    uint32_t m_count;
    uint32_t m_batch_count;
    uint32_t m_batch_size;
  };

}

#endif /* ifndef hf_core_Data_LevelDB_Loader_h */
