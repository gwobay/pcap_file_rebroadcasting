#include <Data/QBDF_Dump.h>

#include <MD/MDPrice.h>
#include <Logger/Logger_Manager.h>
#include <Util/Time.h>

namespace hf_core {

  using namespace hf_tools;

  void
  QBDF_Dump::run()
  {
    bool summary = m_parser.m_params.getDefaultBool("use_summary", false);
    bool oc_summary = m_parser.m_params.getDefaultBool("use_oc_summary", false);

    if(m_csv) {
      if(summary) {
        m_logger->puts("date,time,epoch,sid,ticker,ex,bid,bid_size,ask,ask_size,price,size,trade_time,market_status,cum_total_volume,cum_clean_volume,cum_clean_value,total_trade_count,clean_trade_count,high,low\n");
      } else if(oc_summary) {

      } else {
        m_logger->puts("msg_type,date,time,rec_time_delta,sid,ticker,ex,cond/imb_type,side,trade_price/bid_price/imb_near_price,trade_size/bid_size/imb_paired_size,ask_price/imb_far_price,ask_size/imb_size,corr/imb_side,imb_ref_px,orig_order_id,new_order_id\n");
      }
    }

    if(oc_summary) {
      m_parser.read_oc_summary_file();
      do {
        dump_qbdf_event();
      } while(m_parser.advance_buffer_pos_one_step());
      return;
    }

    while(m_parser.advance_buffer_pos_one_step()) {
      if(m_parser.get_next_time().is_set()) {
        if(m_parser.m_time > m_end_time) {
          break;
        }
        if(m_parser.m_time >= m_start_time) {
          dump_qbdf_event();
        }
      }
    }
  }

  void
  QBDF_Dump::dump_qbdf_event()
  {
    //static const char* common_header = "format,time,exch_time_delta,exch,sid";
    //static const char* where = "QBDF_Dump::dump_qbdf_event";

    char timebuf[32];

    qbdf_format_header *pHdr = (qbdf_format_header*) m_parser.m_buffer_pos;
    if(!pHdr) { return; }

    switch (pHdr->format) {
    case QBDFMsgFmt::QuoteSmall:
      {
        qbdf_quote_small *p_quote_small = (qbdf_quote_small*) m_parser.m_buffer_pos;
        qbdf_quote_price *p_quote_price = &((qbdf_quote_small*) m_parser.m_buffer_pos)->qr;

        if(!m_ids_to_print.test(p_quote_price->symbolId)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_quote_price->symbolId);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_quote_price->exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        if(m_csv) {
          m_logger->printf("QS,%s,%s,,%ld,%s,%s,%0.4f,%0.4f,%d,%d,%c\n",
                           m_date, timebuf,
                           p_id_rec->sid, p_id_rec->symbol.c_str(), exchange->primary_id().str(),
                           p_quote_price->bidPrice, p_quote_price->askPrice,
                           p_quote_small->sizes.bidSize, p_quote_small->sizes.askSize,
                           p_quote_price->cond);
        } else {
          m_logger->printf("%s  %4s %-8s %5d   %10.4f  %10.4f   %5d\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           p_quote_small->sizes.bidSize, p_quote_price->bidPrice,
                           p_quote_price->askPrice, p_quote_small->sizes.askSize);
        }
      }
      break;
    case QBDFMsgFmt::QuoteLarge:
      {
        qbdf_quote_large *p_quote_large = (qbdf_quote_large*) m_parser.m_buffer_pos;
        qbdf_quote_price *p_quote_price = &((qbdf_quote_small*) m_parser.m_buffer_pos)->qr;

        if(!m_ids_to_print.test(p_quote_price->symbolId)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_quote_price->symbolId);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_quote_price->exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        if(m_csv) {
          m_logger->printf("QL,%s,%s,,%ld,%s,%s,%0.4f,%0.4f,%d,%d,%c\n",
                           m_date, timebuf,
                           p_id_rec->sid, p_id_rec->symbol.c_str(), exchange->primary_id().str(),
                           p_quote_price->bidPrice, p_quote_price->askPrice,
                           p_quote_large->sizes.bidSize, p_quote_large->sizes.askSize,
                           p_quote_price->cond);
        } else {
          m_logger->printf("%s  %4s %-8s %5d   %10.4f  %10.4f   %5d\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           p_quote_large->sizes.bidSize, p_quote_price->bidPrice,
                           p_quote_price->askPrice, p_quote_large->sizes.askSize);
        }

      }
      break;
    case QBDFMsgFmt::quote_small_iprice32:
      {
        qbdf_quote_small_iprice32 *p_quote = (qbdf_quote_small_iprice32*) m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_quote->common.id)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_quote->common.id);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_quote->common.exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        double bid = MDPrice::from_iprice32(p_quote->bid_price).fprice();
        double ask = MDPrice::from_iprice32(p_quote->ask_price).fprice();

        if(m_csv) {
          m_logger->printf("Q,%s,%d,%c,%ld,%s,%0.4f,%0.4f,%d,%d,%c\n", timebuf,
                           p_quote->common.exch_time_delta,
                           p_quote->common.exch, p_id_rec->sid, p_id_rec->symbol.c_str(),
                           bid, ask,
                           p_quote->bid_size, p_quote->ask_size,
                           p_quote->cond);
        } else {
          m_logger->printf("%s  %4s %-8s %5d   %10.4f  %10.4f   %5d\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           p_quote->bid_size, bid,
                           ask, p_quote->ask_size);
        }
      }
      break;
    case QBDFMsgFmt::quote_large_iprice32:
      {
        qbdf_quote_large_iprice32 *p_quote = (qbdf_quote_large_iprice32*) m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_quote->common.id)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_quote->common.id);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_quote->common.exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        double bid = MDPrice::from_iprice32(p_quote->bid_price).fprice();
        double ask = MDPrice::from_iprice32(p_quote->ask_price).fprice();

        if(m_csv) {
          m_logger->printf("Q,%s,%d,%c,%ld,%s,%0.4f,%0.4f,%d,%d,%c\n", timebuf,
                           p_quote->common.exch_time_delta,
                           p_quote->common.exch, p_id_rec->sid, p_id_rec->symbol.c_str(),
                           bid, ask,
                           p_quote->bid_size, p_quote->ask_size,
                           p_quote->cond);
        } else {
          m_logger->printf("%s  %4s %-8s %5d   %10.4f  %10.4f   %5d\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           p_quote->bid_size, bid,
                           ask, p_quote->ask_size);
        }
      }
      break;
    case QBDFMsgFmt::Trade:
      {
        qbdf_trade *p_trade = (qbdf_trade*) m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_trade->symbolId)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_trade->symbolId);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_trade->exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        if(m_csv) {
          m_logger->printf("T,%s,,%c,%ld,%s,%0.4f,%d,%c,%.4s\n", timebuf,
                           p_trade->exch, p_id_rec->sid, p_id_rec->symbol.c_str(),
                           p_trade->price, p_trade->size, p_trade->side, p_trade->cond);
        } else {
          m_logger->printf("%s  %4s %-8s               %10.4f               %5d  %.4s\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           p_trade->price, p_trade->size, p_trade->cond);
        }
      }
      break;
    case QBDFMsgFmt::trade_iprice32:
      {
        qbdf_trade_iprice32 *p_trade = (qbdf_trade_iprice32*) m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_trade->common.id)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_trade->common.id);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_trade->common.exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        double price = MDPrice::from_iprice32(p_trade->price).fprice();
        char side = get_side(*p_trade);
        char cond = p_trade->cond[0];
        if(cond == '\0') {
          cond = ' ';
        }
        cond = ' ';

        if(m_csv) {
          m_logger->printf("T,%s,%d,%c,%ld,%s,%0.4f,%d,%c,%.4s\n", timebuf,
                           p_trade->common.exch_time_delta,
                           p_trade->common.exch, p_id_rec->sid, p_id_rec->symbol.c_str(),
                           price, p_trade->size, side, p_trade->cond);
        } else {
          m_logger->printf("%s  %4s %-8s               %10.4f               %5d %c %.4s\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           price, p_trade->size, side, p_trade->cond);
        }
      }
      break;
    case QBDFMsgFmt::Imbalance:
      {
        //qbdf_imbalance *p_imbalance = (qbdf_imbalance*) m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::imbalance_iprice32:
      {
        //qbdf_imbalance_iprice32 *p_imbalance = (qbdf_imbalance_iprice32*) m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::Status:
      {
        qbdf_status* p_status = (qbdf_status*)m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_status->symbolId)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_status->symbolId);
        if (!p_id_rec) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        QBDFStatusType qstatus = *(QBDFStatusType::get_by_index(p_status->status_type));

        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_status->exch];
        if(!exchange) return;

        if(m_csv) {
          //m_logger->printf("%c,%s,,%c,%ld,%s,%0.4f,%d,%c,%.4s\n", 'ST', timebuf,
          //                p_status->exch, p_id_rec->sid, p_id_rec->symbol.c_str());
        } else {
          m_logger->printf("%s  %4s %-8s       Status: %-20s               %.8s\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           qstatus.str(), p_status->status_detail);
        }
      }
      break;
    case QBDFMsgFmt::status_micros:
      {
        qbdf_status_micros* p_status = (qbdf_status_micros*)m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_status->common.id)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_status->common.id);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_status->common.exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        QBDFStatusType qstatus = *(QBDFStatusType::get_by_index(p_status->status_type));

        if(m_csv) {
          //m_logger->printf("%c,%s,,%c,%ld,%s,%0.4f,%d,%c,%.4s\n", 'ST', timebuf,
          //                p_status->exch, p_id_rec->sid, p_id_rec->symbol.c_str());
        } else {
          m_logger->printf("%s  %4s %-8s       Status: %-20s           %.32s\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           qstatus.str(), p_status->status_detail);
        }
      }
      break;
    case QBDFMsgFmt::OrderAddSmall:
    case QBDFMsgFmt::OrderAddLarge:
      if (pHdr->format == QBDFMsgFmt::OrderAddSmall) {
        //qbdf_order_add_small *p_order_add_small = (qbdf_order_add_small*) m_parser.m_buffer_pos;
        //qbdf_order_add_large *p_order_add_large = (qbdf_order_add_large*) m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::order_add_iprice32:
      {
        qbdf_order_add_iprice32 *p_order_add = (qbdf_order_add_iprice32*)m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_order_add->common.id)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_order_add->common.id);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_order_add->common.exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        MDPrice iprice = MDPrice::from_iprice32(p_order_add->price);
        char side = get_side(*p_order_add);

        if(m_csv) {

        } else {
          if(side == 'B') {
            m_logger->printf("%s  %4s %-8s %-20lu B  %5d  %10.4f\n", timebuf,
                             exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                             p_order_add->order_id, p_order_add->size, iprice.fprice());
          } else {
            m_logger->printf("%s  %4s %-8s %-20lu S                       %10.4f  %5d\n", timebuf,
                             exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                             p_order_add->order_id, iprice.fprice(), p_order_add->size);
          }
        }
      }
      break;
    case QBDFMsgFmt::OrderExec:
      {
        //qbdf_order_exec *p_order_exec = (qbdf_order_exec*)m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::order_exec_iprice32:
      {
        qbdf_order_exec_iprice32 *p_order_exec = (qbdf_order_exec_iprice32*)m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_order_exec->common.id)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_order_exec->common.id);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_order_exec->common.exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        MDPrice iprice = MDPrice::from_iprice32(p_order_exec->price);
        char side = get_side(*p_order_exec);
        char cond = p_order_exec->cond;
        if(cond == '\0') {
          cond = ' ';
        }
        cond = ' ';

        if(m_csv) {

        } else {
          m_logger->printf("%s  %4s %-8s %-20lu %c          %10.4f                 %5d %c\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           p_order_exec->order_id, side, iprice.fprice(), p_order_exec->size, cond);
        }

      }
      break;
    case QBDFMsgFmt::OrderReplace:
      {
        //qbdf_order_replace *p_order_replace = (qbdf_order_replace*)m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::order_replace_iprice32:
      {
        //qbdf_order_replace_iprice32 *p_order_replace = (qbdf_order_replace_iprice32*)m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::OrderModify:
      {
        //qbdf_order_modify *p_order_modify = (qbdf_order_modify*)m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::order_modify_iprice32:
      {
        //qbdf_order_modify_iprice32 *p_order_modify = (qbdf_order_modify_iprice32*)m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::GapMarker:
      {
        qbdf_gap_marker *p_gap = (qbdf_gap_marker*)m_parser.m_buffer_pos;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        if(p_gap->symbol_id) {
          if(!m_ids_to_print.test(p_gap->symbol_id)) return;
          QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_gap->symbol_id);
          if (!p_id_rec) return;
          if(m_csv) {
            m_logger->printf("GAP,%s,%s,,%ld,%s,,,,,,\n",
                             m_date, timebuf,
                             p_id_rec->sid, p_id_rec->symbol.c_str());
          } else {
            m_logger->printf("%s       %-8s       GAP  context:%ld\n", timebuf, p_id_rec->symbol.c_str(), p_gap->context);
          }
        } else {
          if(m_csv) {
            m_logger->printf("GAP,%s,%s,,,,,,,,,\n", m_date, timebuf);
          } else {
            m_logger->printf("%s                        GAP  context:%ld\n", timebuf, p_gap->context);
          }
        }
      }
      break;
    case QBDFMsgFmt::gap_marker_micros:
      {
        qbdf_gap_marker_micros *p_gap = (qbdf_gap_marker_micros*)m_parser.m_buffer_pos;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);

        if(p_gap->common.id) {
          if(!m_ids_to_print.test(p_gap->common.id)) return;
          QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_gap->common.id);
          if (!p_id_rec) return;
          if(m_csv) {
            m_logger->printf("GAP,%s,%s,,%ld,%s,,,,,,\n",
                             m_date, timebuf,
                             p_id_rec->sid, p_id_rec->symbol.c_str());
          } else {
            m_logger->printf("%s       %-8s       GAP  context:%ld\n", timebuf, p_id_rec->symbol.c_str(), p_gap->context);
          }
        } else {
          if(m_csv) {
            m_logger->printf("GAP,%s,%s,,,,,,,,,\n", m_date, timebuf);
          } else {
            m_logger->printf("%s                        GAP  context:%ld\n", timebuf, p_gap->context);
          }
        }
      }
      break;
    case QBDFMsgFmt::Level:
      {
        //qbdf_level* p_level = (qbdf_level*)m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::level_iprice64:
      {
        //qbdf_level_iprice64* p_level = (qbdf_level_iprice64*)m_parser.m_buffer_pos;
      }
      break;
    case QBDFMsgFmt::Summary:
      {
        qbdf_summary *p_summary = (qbdf_summary*) m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_summary->symbolId)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_summary->symbolId);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_summary->exch];
        if(!exchange) return;

        Time_Utils::print_time(timebuf, m_parser.m_time, Timestamp::MICRO);
        Time tt = m_parser.m_midnight + msec(p_summary->trade_time);
        char trade_time[32];
        Time_Utils::print_time(trade_time, tt, Timestamp::MILI);

        uint64_t epoch = m_parser.m_time.total_sec();
        uint64_t micros = m_parser.m_time.usec();

        MarketStatus market_status = *(MarketStatus::get_by_index(p_summary->market_status));

        if(m_csv) {
          m_logger->printf("%s,%s,%lu,%ld,%s,%s,%0.4f,%d,%0.4f,%d,%0.4f,%d,%s,%s,%lu,%lu,%lu,%u,%u,%0.4f,%0.4f\n",
                           m_date, timebuf, epoch, p_id_rec->sid, p_id_rec->symbol.c_str(), exchange->primary_id().str(),
                           p_summary->bidPrice, p_summary->bidSize, p_summary->askPrice, p_summary->askSize,
                           p_summary->price, p_summary->size,
                           trade_time, market_status.str(),
                           p_summary->cum_total_volume, p_summary->cum_clean_volume, p_summary->cum_clean_value,
                           p_summary->total_trade_count, p_summary->clean_trade_count,
                           p_summary->high, p_summary->low);
        } else if(m_json) {
          m_logger->printf("{\"date\":%s,\"time\":\"%s\",\"epoch\":%lu,\"micros\":%lu,\"sid\":%ld,\"symbol\":\"%s\",\"exch\":\"%s\",\"bid\":%0.4f,\"bid_size\":%d,\"ask\":%0.4f,\"ask_size\":%d,\"price\":%0.4f,\"size\":%d,\"trade_time\":\"%s\",\"market_status\":\"%s\",\"ttvolume\":%lu,\"ccvolume\":%lu,\"ccvalue\":%lu,\"ttcount\":%u,\"cccount\":%u,\"high\":%0.4f,\"low\":%0.4f}\n",
                           m_date, timebuf, epoch, micros,p_id_rec->sid, p_id_rec->symbol.c_str(), exchange->primary_id().str(),
                           p_summary->bidPrice, p_summary->bidSize, p_summary->askPrice, p_summary->askSize,
                           p_summary->price, p_summary->size,
                           trade_time, market_status.str(),
                           p_summary->cum_total_volume, p_summary->cum_clean_volume, p_summary->cum_clean_value,
                           p_summary->total_trade_count, p_summary->clean_trade_count,
                           p_summary->high, p_summary->low);
        } else {
          m_logger->printf("%s  %4s %-8s %-5d   %10.4f  %10.4f   %5d\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           p_summary->bidSize, p_summary->bidPrice,
                           p_summary->askPrice, p_summary->askSize);
          m_logger->printf("%s  %4s %-8s               %10.4f               %8ld  %6d\n", timebuf,
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           p_summary->price, p_summary->cum_clean_volume, p_summary->clean_trade_count);
        }
      }
      break;
    case QBDFMsgFmt::OpenCloseSummary:
      {
        qbdf_oc_summary *p_summary = (qbdf_oc_summary*) m_parser.m_buffer_pos;

        if(!m_ids_to_print.test(p_summary->symbolId)) return;
        QBDFIdRec* p_id_rec = m_parser.getMappingByBinaryId(p_summary->symbolId);
        if (!p_id_rec) return;
        const Exchange* exchange = m_parser.m_qbdf_exch_to_hf_exch[(uint8_t)p_summary->exch];
        if(!exchange) return;

        char open_time[32], close_time[32];

        Time ot = m_parser.m_midnight + msec(p_summary->open_time);
        Time ct = m_parser.m_midnight + msec(p_summary->close_time);

        Time_Utils::print_time(open_time, ot, Timestamp::MILI);
        Time_Utils::print_time(close_time, ct, Timestamp::MILI);

        if(m_csv) {


        } else {
          m_logger->printf("%4s %-8s  open: %s $%0.4f %d   close: %s $%0.4f %d\n",
                           exchange->primary_id().str(), p_id_rec->symbol.c_str(),
                           open_time, p_summary->open_price, p_summary->open_size,
                           close_time, p_summary->close_price, p_summary->close_size);
        }
      }
      break;
    }

  }

  Time
  QBDF_Dump::parse_time(const string& t)
  {
    int HH, MM, SS;

    int r = sscanf(t.c_str(), "%2d:%2d:%2d", &HH, &MM, &SS);

    //m_logger->printf("sscanf returned %d %d %d %d\n", r, HH, MM, SS);

    if(r != 3) {
      return Time();
    }

    Time pt = m_parser.m_midnight + seconds(HH * 3600 + MM * 60 + SS);

    return pt;
  }

  QBDF_Dump::QBDF_Dump(Application* app, const Parameter& params) :
    m_app(app),
    m_parser(app),
    m_csv(false),
    m_json(false),
    m_logger(0)
  {
    m_parser.init("QBDF_Dump", params);

    m_default_logger = m_app->lm()->stdout();
    m_logger = m_default_logger.get();

    string today_date = m_parser.m_midnight.to_iso_date();

    strncpy(m_date, today_date.c_str(), 9);
    m_date[8] = '\0';

    m_ids_to_print.resize(m_parser.m_id_vector.size(), true);

    m_csv = params.getDefaultBool("csv", m_csv);
    m_json = params.getDefaultBool("json", m_json);

    m_start_time = parse_time(m_parser.m_start_time);
    if(!m_start_time.is_set()) {
      throw runtime_error("start_time " + m_parser.m_start_time + " not in HH:MM:SS format");
    }
    m_end_time = parse_time(m_parser.m_end_time);
    if(!m_end_time.is_set()) {
      throw runtime_error("end_time " + m_parser.m_end_time + " not in HH:MM:SS format");
    }

    if(params.has("sym")) {
      m_ids_to_print.reset();
      string sym = params["sym"].getString();
      for(vector<QBDFIdRec>::const_iterator i = m_parser.m_id_vector.begin(), i_end = m_parser.m_id_vector.end(); i != i_end; ++i) {
        if(i->symbol == sym) {
          m_ids_to_print.set(i->binaryId);
        }
      }
    }

    if(params.has("sid")) {
      m_ids_to_print.reset();
      Product::sid_t sid = params["sid"].getInt();
      for(vector<QBDFIdRec>::const_iterator i = m_parser.m_id_vector.begin(), i_end = m_parser.m_id_vector.end(); i != i_end; ++i) {
        if(i->sid == sid) {
          m_ids_to_print.set(i->binaryId);
        }
      }
    }

  }


}
