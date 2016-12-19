#include <Data/LevelDB_Loader.h>


namespace hf_core {
  using namespace std;

  LevelDB_Loader::LevelDB_Loader(Application* app)
  : m_leveldb(0), p_batch(0), m_batch_size(10000000)
  {
    m_app = app;
    m_logger = m_app->lm()->get_logger("leveldb");

    //defaults
    m_options.block_cache = leveldb::NewLRUCache(1*1024*1024*1024);
    m_options.create_if_missing = true;
  }

  LevelDB_Loader::~LevelDB_Loader()
  {
    if (m_leveldb) {
      delete m_leveldb;
    }
  }

  bool LevelDB_Loader::open_db(string db_file_name)
  {
    const char* where = "LevelDB_Loader::open_db";

    m_status = leveldb::DB::Open(m_options, db_file_name, &m_leveldb);
    if (!m_status.ok()) {
      m_logger->log_printf(Log::ERROR, "%s: %s", where, m_status.ToString().c_str());
      m_logger->sync_flush();
      return false;
    }
    return true;
  }


  bool LevelDB_Loader::insert_batch(string key, string value)
  {
    if (!m_leveldb) {
      m_logger->log_printf(Log::ERROR, "LevelDB_Loader::insert_batch: DB not open!");
      m_logger->sync_flush();
      return false;
    }

    if (!p_batch) {
      p_batch = new leveldb::WriteBatch();
      m_count = 0;
      m_batch_count = 0;
    }

    m_count++;
    m_batch_count++;
    p_batch->Put(key, value);
    if (m_batch_count % m_batch_size) {
      m_status = m_leveldb->Write(m_write_options, p_batch);
      if (!m_status.ok()) {
        m_logger->log_printf(Log::ERROR, "LevelDB_Loader::insert_batch: %s",
                             m_status.ToString().c_str());
        m_logger->sync_flush();
        return false;
      }

      delete p_batch;
      p_batch = new leveldb::WriteBatch();
      m_logger->log_printf(Log::DEBUG, "LevelDB_Loader::insert_batch: Wrote %d records", m_count);
      m_batch_count = 0;
    }
    return true;
  }

  bool LevelDB_Loader::insert(string key, string value)
  {
    if (!m_leveldb) {
      m_logger->log_printf(Log::ERROR, "LevelDB_Loader::insert: DB not open!");
      m_logger->sync_flush();
      return false;
    }

    // non-batch insert
    return true;
  }

  bool LevelDB_Loader::finalize_batch()
  {
    const char* where = "LevelDB_Loader::finalize_batch";

    if (!m_leveldb) {
      m_logger->log_printf(Log::ERROR, "%s: DB not open!", where);
      m_logger->sync_flush();
      return false;
    }

    if (p_batch && m_batch_count > 0) {
      m_status = m_leveldb->Write(m_write_options, p_batch);
      if (!m_status.ok()) {
        m_logger->log_printf(Log::ERROR, "%s: %s", where, m_status.ToString().c_str());
        m_logger->sync_flush();
        return false;
      }

      delete p_batch;
      m_logger->log_printf(Log::DEBUG, "%s: Wrote %d records", where, m_count);
      m_batch_count = 0;
    }

    m_logger->log_printf(Log::DEBUG, "%s: Compacting ...", where);
    m_leveldb->CompactRange(NULL, NULL);
    return true;
  }


}
