#ifndef hf_core_Data_QBDF_Util_h
#define hf_core_Data_QBDF_Util_h

#include <Data/QBDF_Builder.h>

namespace hf_core {

  vector<uint32_t> get_qbdf_message_sizes(string version);

  bool gen_qbdf_quote(QBDF_Builder* p_builder, QuoteUpdate* p_quote,
                      uint32_t ms_since_midnight, char exch,
                      char source, char corr);

  bool gen_qbdf_quote_rec_time(QBDF_Builder* p_builder, QuoteUpdate* p_quote,
                               uint32_t rec_time, char exch, char source, char corr);

  bool gen_qbdf_quote_micros(QBDF_Builder* p_builder, uint16_t id,
                               uint64_t micros_since_midnight_exch, int32_t record_time_delta,
                               char exch, char cond, MDPrice bid_price, MDPrice ask_price,
			     uint32_t bid_size, uint32_t ask_size, bool suppress_64=true);

  bool gen_qbdf_quote_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
                                       uint64_t qbdf_time, int32_t record_time_delta,
                                       char exch, char cond, MDPrice bid_price, MDPrice ask_price,
                                       uint32_t bid_size, uint32_t ask_size, bool suppress_64=true);

  bool gen_qbdf_trade(QBDF_Builder* p_builder, TradeUpdate* p_trade,
                      uint32_t ms_since_midnight, char exch,
                      char source, char corr, char stop, char side);

  bool gen_qbdf_trade_rec_time(QBDF_Builder* p_builder, TradeUpdate* p_trade,
                      uint32_t rec_time, char exch, char source, char corr, char stop, char side);

  bool gen_qbdf_trade_seq_rec_time(QBDF_Builder* p_builder, qbdf_trade_seq* p_trade_seq,
                                   uint32_t rec_time, char exch, char source, char corr, char stop, char side);

  bool gen_qbdf_trade_micros(QBDF_Builder* p_builder, uint16_t id,
                             uint64_t micros_since_midnight_exch, int32_t record_time_delta,
                             char exch, char side, char corr, string cond,
                             uint32_t size, MDPrice price, bool suppress_64=true);

  bool gen_qbdf_trade_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
                                       uint64_t qbdf_time, int32_t record_time_delta,
                                       char exch, char side, char corr, string cond,
                                       uint32_t size, MDPrice price, bool suppress_64=true);

  bool gen_qbdf_trade_augmented(QBDF_Builder* p_builder, uint16_t id,
				uint64_t micros_since_midnight_exch, int32_t record_time_delta,
				char exch, char side, char exec_flag, char corr, const char* cond, const char* misc,
				char trade_status, char market_status,
                                uint32_t size, MDPrice price, uint64_t order_age, bool clean);

  bool gen_qbdf_trade_augmented_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					  uint64_t qbdf_time, int32_t record_time_delta,
					  char exch, char side, char exec_flag, char corr, const char* cond, const char* misc,
                                          char trade_status, char market_status,
					  uint32_t size, MDPrice price, uint64_t order_age, bool clean);

  bool gen_qbdf_cross_trade(QBDF_Builder* p_builder, TradeUpdate* p_trade,
                            uint32_t ms_since_midnight, char exch, char source,
                            char corr, char stop, char side, char cross_types);

  bool gen_qbdf_cross_trade_micros(QBDF_Builder* p_builder, uint16_t id,
                                   uint64_t micros_since_midnight, int32_t record_time_delta,
                                   char exch, char side, char corr, char cross_type,
                                   uint32_t size, MDPrice price);

  bool gen_qbdf_imbalance(QBDF_Builder* p_builder, AuctionUpdate* p_auction,
                          uint32_t ms_since_midnight, char exch);

  bool gen_qbdf_imbalance_micros(QBDF_Builder* p_builder, uint16_t id,
                                 uint64_t micros_since_midnight, int32_t record_time_delta,
                                 char exch, MDPrice ref_price, MDPrice near_price,
                                 MDPrice far_price, uint32_t paired_size,
                                 uint32_t imbalance_size, char imbalance_side,
                                 char indicative_flag, bool suppress_64=true);

  bool gen_qbdf_imbalance_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
                                           uint64_t qbdf_time, int32_t record_time_delta,
                                           char exch, MDPrice ref_price, MDPrice near_price,
                                           MDPrice far_price, uint32_t paired_size,
                                           uint32_t imbalance_size, char imbalance_side,
                                           char indicative_flag, bool suppress_64=true);

  bool gen_qbdf_order_add(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                          uint32_t ms_since_midnight, char exch,
                          char side, uint32_t size, float price);

  bool gen_qbdf_order_add_rec_time(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                          uint32_t rec_time, char exch, char side, uint32_t size, float price);

  bool gen_qbdf_order_add_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                 uint64_t micros_since_midnight, int32_t record_time_delta,
                                 char exch, char side, uint32_t size, MDPrice price);

  bool gen_qbdf_order_add_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                           uint64_t qbdf_time, int32_t record_time_delta,
                                           char exch, char side, uint32_t size, MDPrice price,
                                           bool delta_in_millis=false);
  
  bool gen_qbdf_order_exec(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                           uint32_t ms_since_midnight, char exch, uint32_t size, float price);
  
  bool gen_qbdf_order_exec_rec_time(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                                    uint32_t rec_time, char exch, uint32_t size, float price);
  
  bool gen_qbdf_order_exec_cond_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                  uint64_t micros_since_midnight, int32_t record_time_delta,
                                  char exch, char side, const char* cond, uint32_t size, MDPrice price);
  
  bool gen_qbdf_order_exec_cond_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                            uint64_t qbdf_time, int32_t record_time_delta,
                                            char exch, char side, const char* cond,
                                            uint32_t size, MDPrice price, bool delta_in_millis=false);
  
  bool gen_qbdf_order_exec_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                  uint64_t micros_since_midnight, int32_t record_time_delta,
                                  char exch, char side, char cond, uint32_t size, MDPrice price);
  
  bool gen_qbdf_order_exec_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                            uint64_t qbdf_time, int32_t record_time_delta,
                                            char exch, char side, char cond,
                                            uint32_t size, MDPrice price, bool delta_in_millis=false);
  
  bool gen_qbdf_order_modify(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                             uint32_t ms_since_midnight, char exch,
                             char side, uint32_t size, float price);
  
  bool gen_qbdf_order_modify_rec_time(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                                      uint32_t rec_time, char exch, char side,
                                      uint32_t size, float price);
  
  bool gen_qbdf_order_modify_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                    uint64_t micros_since_midnight, int32_t record_time_delta,
                                    char exch, char side, uint32_t size, MDPrice price);
  
  bool gen_qbdf_order_modify_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                              uint64_t qbdf_time, int32_t record_time_delta,
                                              char exch, char side, uint32_t size, MDPrice price,
                                              bool delta_in_millis=false);
  
  bool gen_qbdf_order_replace(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                              uint32_t ms_since_midnight, char exch,
                              uint64_t new_order_id, uint32_t size, float price);
  
  bool gen_qbdf_order_replace_rec_time(QBDF_Builder* p_builder, ProductID id, uint64_t order_id,
                                       uint32_t rec_time, char exch, uint64_t new_order_id,
                                       uint32_t size, float price);
  
  bool gen_qbdf_order_replace_micros(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                     uint64_t micros_since_midnight, int32_t record_time_delta,
                                     char exch, char side, uint64_t new_order_id,
                                     uint32_t size, MDPrice price);
  
  bool gen_qbdf_order_replace_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id, uint64_t order_id,
                                               uint64_t qbdf_time, int32_t record_time_delta,
                                               char exch, char side, uint64_t new_order_id,
                                               uint32_t size, MDPrice price, bool delta_in_millis=false);
  
  bool gen_qbdf_gap_marker(QBDF_Builder* p_builder, uint32_t ms_since_midnight,
                           ProductID id, size_t context);
  
  bool gen_qbdf_gap_marker_micros(QBDF_Builder* p_builder, uint64_t micros_since_midnight,
                                  uint16_t id, char exch, size_t context);
  
  bool gen_qbdf_gap_marker_micros_qbdf_time(QBDF_Builder* p_builder, uint64_t qbdf_time,
					    uint16_t id, char exch, size_t context);
  
  bool gen_qbdf_gap_summary(QBDF_Builder* p_builder, uint32_t ms_since_midnight, size_t context,
                            uint64_t expected_seq_no, uint64_t received_seq_no);
  
  bool gen_qbdf_level(QBDF_Builder* p_builder, ProductID id, uint32_t ms_since_midnight,
                      char exch, char side, uint32_t size, float price, uint16_t num_orders,
                      char trade_status, char quote_cond, char reason_code,
                      uint32_t link_id_1, uint32_t link_id_2, uint32_t link_id_3, uint8_t last_in_group);
  
  bool gen_qbdf_level_rec_time(QBDF_Builder* p_builder, ProductID id, uint32_t rec_time,
                               char exch, char side, uint32_t size, float price, uint16_t num_orders,
                               char trade_status, char quote_cond, char reason_code, uint32_t link_id_1,
                               uint32_t link_id_2, uint32_t link_id_3, uint8_t last_in_group);
  
  bool gen_qbdf_level_iprice64(QBDF_Builder* p_builder, uint16_t id,
                               uint64_t micros_since_midnight, int32_t record_time_delta,
                               char exch, char side, char trade_cond, char quote_cond,
                               char reason_code, uint16_t num_orders, uint32_t size, MDPrice md_price,
                               bool last_in_group, bool executed);

  bool gen_qbdf_level_iprice64_qbdf_time(QBDF_Builder* p_builder, ProductID id,
                                         uint64_t qbdf_time, int32_t record_time_delta,
                                         char exch, char side, char trade_cond, char quote_cond,
                                         char reason_code, uint16_t num_orders,
                                         uint32_t size, MDPrice md_price, bool last_in_group,
                                         bool executed, bool delta_in_millis=false);

  bool gen_qbdf_link(QBDF_Builder* p_builder, ProductID id, string vendor_id,
                     string ticker, uint32_t link_id, char side);

  bool gen_qbdf_cancel_correct(QBDF_Builder* p_builder, ProductID id, uint32_t ms_since_midnight,
                               uint32_t orig_ms_since_midnight, string vendor_id, string type,
                               uint64_t orig_seq_no, double orig_price, int orig_size,
                               string orig_cond, double new_price, int new_size, string new_cond);

  bool gen_qbdf_cancel_correct_rec_time(QBDF_Builder* p_builder, ProductID id, uint32_t rec_time,
                                        uint32_t orig_rec_time, string vendor_id, string type,
                                        uint64_t orig_seq_no, double orig_price, int orig_size,
                                        string orig_cond, double new_price, int new_size,
                                        string new_cond);

  bool gen_qbdf_status(QBDF_Builder* p_builder, ProductID id, uint32_t ms_since_midnight,
                       char exch, uint16_t status_type, const char* status_detail);

  bool gen_qbdf_status_rec_time(QBDF_Builder* p_builder, ProductID id, uint32_t rec_time,
                                char exch, uint16_t status_type, const char* status_detail);

  bool gen_qbdf_status_micros(QBDF_Builder* p_builder, uint16_t id,
                              uint64_t micros_since_midnight, int32_t record_time_delta,
                              char exch, uint16_t status_type, string status_detail);

  bool gen_qbdf_status_micros_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
                                        uint64_t qbdf_time, int32_t record_time_delta,
                                        char exch, uint16_t status_type, string status_detail);

  bool gen_qbdf_quote_micros_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					     uint64_t qbdf_time, int32_t exch_time_delta,
					     char exch, char cond, MDPrice bid_price, MDPrice ask_price,
					     uint32_t bid_size, uint32_t ask_size, uint32_t seq_no);

  bool gen_qbdf_quote_nanos_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					     uint64_t qbdf_time, int32_t exch_time_delta,
					     char exch, char cond, MDPrice bid_price, MDPrice ask_price,
					     uint32_t bid_size, uint32_t ask_size, uint32_t seq_no);

  bool gen_qbdf_trade_micros_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					   uint64_t qbdf_time, int32_t exch_time_delta,
					   char exch, char side, char corr, string cond,
					   uint32_t size, MDPrice price, uint32_t seq_no);

  
  bool gen_qbdf_trade_nanos_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					  uint64_t qbdf_time, int32_t exch_time_delta,
					  char exch, char side, char corr, string cond,
					  uint32_t size, MDPrice price, uint32_t seq_no);

  bool gen_qbdf_imbalance_micros_seq_qbdf_time(QBDF_Builder* p_builder, uint16_t id,
					       uint64_t qbdf_time, int32_t exch_time_delta,
					       char exch, MDPrice ref_price, MDPrice near_price,
					       MDPrice far_price, uint32_t paired_size,
					       uint32_t imbalance_size, char imbalance_side,
					       char indicative_flag, uint32_t seq_no);
}

#endif /* ifndef hf_core_Data_QBDF_Util_h */
