#include <Data/MDBufferQueue.h>

#include <app/app.h>
#include <Data/Handler.h>

#include <stdarg.h>

namespace hf_core {

  void
  channel_info::init(const Parameter& params)
  {
    timeout = params.getDefaultTimeDuration("recovery_timeout", timeout);
    send_retransmit_timeout = params.getDefaultTimeDuration("send_retransmit_timeout", send_retransmit_timeout);
    max_messages = params.getDefaultInt("recovery_max_messages", max_messages);
  }

  bool
  channel_info::begin_recovery(Handler* handler, const char* buf, size_t len, uint64_t seq_no, uint64_t next_seq_num)
  {
    const char* where = "channel_info::begin_recovery";
    
    if(!this->has_reordering) {
      char buf[256];
      snprintf(buf, 256, "%s: %s Unrecoverable gap -- replaying w/ gaps expecting=%zd  got=%zd", where, handler->name().c_str(), this->seq_no, seq_no);
      handler->reset(this->context, buf, this->seq_no, seq_no);
      return false;
    }
    
    if(this->last_update.is_set()) {
      this->queue.queue_push(seq_no, next_seq_num, buf, len);
      if(this->began_recovery.is_set()) {
        Time t = Time::current_time();
        Time_Duration diff = t - this->began_recovery;
        
        if(this->queue.size() > this->max_messages) {
          handler->send_alert("%s: %s line %s #%zd Sequence re-ordering exceeded max_messages %zd last=%zd next_in_queue=%zd",
                              where, handler->name().c_str(), this->name.c_str(), this->context,
                              this->queue.size(), this->seq_no, this->queue.top().seq_num);
          recovery_complete(handler);
        } else if(diff > this->timeout) {
          handler->send_alert("%s: %s line %s #%zd Sequence re-ordering timed out last=%zd next_in_queue=%zd",
                              where, handler->name().c_str(), this->name.c_str(), this->context,
                              this->seq_no, this->queue.top().seq_num);
          recovery_complete(handler);
        } else if(this->has_recovery && !this->sent_resend_request && diff > this->send_retransmit_timeout) {
          handler->send_retransmit_request(*this);
          this->sent_resend_request = true;
        }
        // otheriwse, just continue
      } else {
        uint64_t c = seq_no - this->seq_no;
        if(c < 32000) {
          handler->logger().log_printf(Log::INFO, "%s: %s line %s #%zd beginning recovery queuing",
                                       where, handler->name().c_str(), this->name.c_str(), this->context);
          this->began_recovery = Time::current_time();
        } else {
          handler->send_alert("%s: %s line %s #%zd Unrecoverable gap, missed message count %zd too high expecting seq_no = %zd,  received = %zd",
                              where, handler->name().c_str(), this->name.c_str(), this->context, c, this->seq_no, seq_no);
          recovery_complete(handler);
        }
      }
      return true;
    } else {
      return false; // first update -- allow to continue
    }
  }

  void
  channel_info::process_recovery(Handler* handler)
  {
    const char* where = "channel_info::process_recovery";
    
    bool progressed = false;
    size_t q_size = 0;
    while(!this->queue.empty() && this->queue.top().seq_num <= this->seq_no) {
      const MDBufferQueue::queued_message& msg = this->queue.top();
      if(msg.seq_num == this->seq_no ||           /* queue will contain duplicates due to A/B line */
         msg.next_seq_num > this->seq_no) {       /* This allows partially-new packets to pass */
        handler->parse2(this->context, msg.msg, msg.msg_len);
        if(msg.next_seq_num != 0) {
          this->seq_no = msg.next_seq_num;
        }
        ++q_size;
        progressed = true;
      }
      this->queue.queue_pop();
    }
    if(this->queue.empty()) {
      handler->logger().log_printf(Log::INFO, "%s: %s line %s #%zd Successfully re-ordered packets, num_packets=%zd",
                                   where, handler->name().c_str(), this->name.c_str(), this->context, q_size);
      recovery_complete(handler);
    } else if(progressed && this->sent_resend_request) {
      handler->send_retransmit_request(*this);
      this->began_recovery = Time::current_time();
    }
  }

  void
  channel_info::recovery_complete(Handler* handler)
  {
    char buf[512];
    this->sent_resend_request = false;
    this->began_recovery.clear();

    bool first = true;

    if(!this->queue.empty()) {
      while(!this->queue.empty()) {
        const MDBufferQueue::queued_message& msg = this->queue.top();
        if(msg.seq_num >= this->seq_no) {  // queue may contain duplicates due to A/B line
          if(msg.seq_num > this->seq_no) {
            int num_packets = msg.seq_num - this->seq_no;
            snprintf(buf, 512, "channel_info::recovery_complete: %s line %s #%zd Sequence Gap %s, num_packets=%d",
                     handler->name().c_str(), this->name.c_str(), this->context, first ? "" : "queued", num_packets);
            handler->reset(this->context, buf);
          }

          this->seq_no = msg.seq_num;
          handler->parse2(this->context, msg.msg, msg.msg_len);
          if(msg.next_seq_num != 0) {
            this->seq_no = msg.next_seq_num;
          }
        }
        this->queue.queue_pop();
        first = false;
      }
    }
  }

  void
  channel_info::clear()
  {
    seq_no = 0;
    last_update.clear();
    began_recovery.clear();
    sent_resend_request = false;
    queue.queue_clear();
  }


}
