#ifndef hf_md_Data_ARCA_Handler_h
#define hf_md_Data_ARCA_Handler_h

#include <Data/NYSE_Handler.h>
#include <Util/Network_Utils.h>
#include <Data/ArcaL2Msg.h>

namespace hf_core {
  class MDDispatcher;
}

namespace hf_core {
  using namespace std;
  using namespace hf_tools;

  /**
   * Provides support for dealing with sequnce numbers for ARCA protocols that use the
   * common header format
   */

  namespace ARCA_structs {
    using NYSE_structs::message_header;

    using NYSE_structs::sequence_reset_message;

    using NYSE_structs::heartbeat_response_message;

    using NYSE_structs::retransmission_request_message;

    using NYSE_structs::message_unavailable;

    using NYSE_structs::retransmission_response_message;

    struct trade_message {
      uint32_t source_time;
      uint64_t buy_side_link_id;
      uint64_t sell_side_link_id;
      uint32_t price_numerator;
      uint32_t volume;
      uint64_t source_seq_num;
      char     source_session_id;
      char     price_scale_code;
      char     exchange_id;
      char     security_type;
      char     trade_condition[4];
      char     symbol[16];
      uint64_t quote_link_id;
    }  __attribute__ ((packed));

    struct trade_cancel_message {
      uint32_t source_time;
      uint32_t source_seq_num;
      uint32_t original_ref_num;
      char     source_session_id;
      char     exchange_id;
      char     security_type;
      char     filler;
      char     symbol[16];
    } __attribute__ ((packed));

    struct trade_correction_message {
      uint32_t source_time;
      uint32_t filler1;
      uint32_t filler2;
      uint32_t price_numerator;
      uint32_t volume;
      uint32_t source_seq_num;
      uint32_t original_ref_num;
      char     source_session_id;
      char     price_scale_code;
      char     exchange_id;
      char     security_type;
      char     corrected_trade_condition[4];
      char     symbol[16];
    } __attribute__ ((packed));


    struct book_message {
      uint16_t symbol_index;
      uint16_t msg_type;
    } __attribute__ ((packed));

    struct imbalance_message {
      uint16_t symbol_index;
      uint16_t msg_type;
      uint32_t source_seq_num;
      uint32_t source_time;
      uint32_t volume;
      uint32_t total_imbalance;
      uint32_t market_imbalance;
      uint32_t price_numerator;
      char     price_scale_code;
      char     auction_type;
      char     exchange_id;
      char     security_type;
      char     session_id;
      char     filler;
      uint16_t auction_time;
    } __attribute__ ((packed));

    struct symbol_index_mapping_request {
      message_header header;
      uint16_t symbol_index;
      char     session_id;
      char     retransmit_method;
      char     source_id[20];
    } __attribute__ ((packed));

    struct symbol_index_mapping {
      uint16_t symbol_index;
      char     session_id;
      char     filler;
      char     symbol[16];
    } __attribute__ ((packed));

  };

  class ARCA_Handler : public NYSE_Handler {
  public:
    virtual size_t parse2(size_t context, const char* buf, size_t len);

    virtual void init(const string&, ExchangeID exch, const Parameter&);

    virtual void add_context_product_id(size_t context, ProductID id);
    virtual void subscribe_product(ProductID id_,ExchangeID exch_,
				   const string& mdSymbol_,const string& mdExch_);

    void send_quote_update();
    void send_trade_update();
    
    static size_t dump(ILogger& log, Dump_Filters& filter, size_t context, const char* buf, size_t len);
    
    ARCA_Handler(Application* app);
    virtual ~ARCA_Handler();

  private:

    inline void handle_symbol_clear(size_t context,const ArcaL2SymbolClear_t& clr);
    inline void handle_symbol_update(size_t context,const ArcaL2SymbolUpdate_t& update);
    inline void handle_add_order(const ArcaL2Add_t& add);
    inline void handle_modify_order(const ArcaL2Modify_t& modify);
    inline void handle_delete_order(const ArcaL2Delete_t& del);
    inline void handle_imbalance(const ArcaL2Imbalance_t& imb,const Time_Duration&);
    inline void handle_book_refresh(const ArcaL2FreshHeader_t& fresh_header,const  ArcaL2BookOrder_t& add);
    inline void handle_trade(const ARCA_structs::trade_message&,const Time_Duration&);
    
    bool  m_compacted;
    ExchangeID m_exch;
  };
    

}

#endif /* ifndef hf_md_Data_ARCA_Handler_h */
