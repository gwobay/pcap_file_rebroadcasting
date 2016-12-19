#ifndef hf_core_Data_QBT_QBDF_Builders_h
#define hf_core_Data_QBT_QBDF_Builders_h

#include <Data/QBDF_Builder.h>

#include <Data/LSE_Millennium_Handler.h>
#include <Data/TSE_FLEX_Handler.h>
#include <Data/CHIX_MMD_Handler.h>
#include <Data/TMX_QL2_Handler.h>
#include <Data/TMX_TL1_Handler.h>
#include <Data/TVITCH_5_Handler.h>
#include <Data/NYSE_Handler.h>

namespace hf_core {
  using namespace std;

  class QBT_LSE_MILLENNIUM_QBDF_Builder : public QBDF_Builder {
  public:
    QBT_LSE_MILLENNIUM_QBDF_Builder(Application* app, const string& date) :
      QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    LSE_Millennium_Handler* m_message_handler;
  };

  class QBT_TSE_FLEX_QBDF_Builder : public QBDF_Builder {
  public:
    QBT_TSE_FLEX_QBDF_Builder(Application* app, const string& date) :
      QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    TSE_FLEX_Handler* m_message_handler;
  };

  class QBT_CHIX_MMD_QBDF_Builder : public QBDF_Builder {
  public:
    QBT_CHIX_MMD_QBDF_Builder(Application* app, const string& date) :
      QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    CHIX_MMD_Handler* m_message_handler;
  };

  class QBT_TMX_QL2_QBDF_Builder : public QBDF_Builder {
  public:
    QBT_TMX_QL2_QBDF_Builder(Application* app, const string& date) :
      QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    TMX_QL2_Handler* m_message_handler;
  };

  class QBT_TMX_TL1_QBDF_Builder : public QBDF_Builder {
  public:
    QBT_TMX_TL1_QBDF_Builder(Application* app, const string& date) :
      QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    TMX_TL1_Handler* m_message_handler;
  };

  class Hist_ITCH5_QBDF_Builder : public QBDF_Builder {
  public:
    Hist_ITCH5_QBDF_Builder(Application* app, const string& date) :
      QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    TVITCH_5_Handler* m_message_handler;
  };

  class QBT_ITCH5_QBDF_Builder : public QBDF_Builder {
  public:
    QBT_ITCH5_QBDF_Builder(Application* app, const string& date) :
      QBDF_Builder(app, date), m_message_handler(0) {}

  protected:
    int process_raw_file(const string& file_name);

    TVITCH_5_Handler* m_message_handler;
  };

}

#endif /* ifndef hf_core_Data_QBT_QBDF_Builders_h */
