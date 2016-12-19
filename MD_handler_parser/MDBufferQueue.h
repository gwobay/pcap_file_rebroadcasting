#ifndef hf_md_QBT_MDBufferQueue_h
#define hf_md_QBT_MDBufferQueue_h

#include <Data/Handler.h>
#include <Logger/Logger.h>
#include <Product/Product.h>
#include <Util/Network_Utils.h>
#include <Util/Parameter.h>
#include <Util/Time.h>

#include <queue>

namespace hf_core {
  using namespace hf_tools;
  using namespace std;

  class Handler;

  class MDBufferQueue {
  public:
    struct queued_message {
      uint64_t      seq_num;
      uint64_t      next_seq_num;
      size_t        msg_len;
      char*         msg;

      queued_message(size_t seq_num, size_t next_seq_num, size_t msg_len, char* msg) :
	seq_num(seq_num), next_seq_num(next_seq_num), msg_len(msg_len), msg(msg) { }

      struct compare {
	bool operator()(const queued_message& a, const queued_message& b) const {
	  return a.seq_num > b.seq_num;
	}
      };
    };

    typedef priority_queue<queued_message, vector<queued_message>, queued_message::compare> queue_t;
    queue_t queued_messages;

    inline size_t size() const { return queued_messages.size(); }
    inline bool empty() const { return queued_messages.empty(); }
    inline const queued_message& top() const { return queued_messages.top(); }

    bool queue_push(uint64_t seq_num, uint64_t next_seq_num, const char* buffer, size_t len) {
      char* msg = new char[len];
      memcpy(msg, buffer, len);
      queued_messages.push(queued_message(seq_num, next_seq_num, len, msg));
      return true;
    }
    void queue_clear() {
      while(!queued_messages.empty()) {
        delete [] queued_messages.top().msg;
        queued_messages.pop();
      }
    }
    void queue_pop() {
      delete [] queued_messages.top().msg;
      queued_messages.pop();
    }

  };

  struct channel_info {
    string   name;
    size_t   context;
    uint64_t seq_no;
    //uint64_t processed_seq_no;
    uint64_t next_seq_no; 
    uint64_t gap_seq_no;
    Time     last_update;
    Time     began_recovery;
    Time_Duration timeout;
    Time_Duration send_retransmit_timeout;
    size_t        max_messages;
    bool     has_reordering;
    bool     has_recovery;
    bool     sent_resend_request;
    bool     snapshot_is_on;
    bool     final_message_received;
    socket_fd recovery_socket;
    socket_fd snapshot_socket;
    MDBufferQueue queue;
    sockaddr_in recovery_addr;
    sockaddr_in snapshot_addr;
    vector<ProductID> clear_products;
    Time     timestamp;
    uint64_t last_timestamp_ms;  // Only used by QBDF_Builder
    uint64_t last_msg_ms;        // Only used by QBDF_Builder
        
    channel_info(const string& name, size_t context) :
      name(name), context(context), seq_no(0),
      timeout(msec(100)), send_retransmit_timeout(msec(2)), max_messages(50000),
      has_reordering(true), has_recovery(false), sent_resend_request(false),
      snapshot_is_on(false), final_message_received(false),
	last_timestamp_ms(0), last_msg_ms(0)
    {
      timestamp = Time::currentish_time();
    }

    channel_info() :
      name(""), context(0), seq_no(0),
      timeout(msec(100)), send_retransmit_timeout(msec(2)), max_messages(50000),
      has_reordering(true), has_recovery(false), sent_resend_request(false),
      snapshot_is_on(false),final_message_received(false), 
	last_timestamp_ms(0), last_msg_ms(0)
    {
      timestamp = Time::currentish_time();
    }
    
    void init(const Parameter& params);

    bool begin_recovery(Handler* handler, const char* buf, size_t len, uint64_t seq_no, uint64_t next_seq_num);
    void process_recovery(Handler* handler);
    void recovery_complete(Handler* handler);
    void clear();
  };
}

#endif /* ifndef hf_md_QBT_MDBufferQueue_h */
