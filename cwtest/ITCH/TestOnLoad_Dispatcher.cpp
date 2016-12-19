#define BOOST_DISABLE_ASSERTS 1

#include <ITCH/Simple_Dispatcher.h>
#include <ITCH/TestOnLoad_Dispatcher.h>

#include <app/app.h>
#include <app/App_Run_Loop.h>

#include <Util/except.h>
#include <Util/Network_Utils.h>

#include <boost/scoped_array.hpp>
#include <algorithm>

#include <netinet/in.h>
#include <net/if.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syscall.h>
#include <sys/socket.h>

/* These are defined in socket.h, but older versions might not have all 3 */
#ifndef SO_TIMESTAMP
  #define SO_TIMESTAMP            29
#endif
#ifndef SO_TIMESTAMPNS
  #define SO_TIMESTAMPNS          35
#endif
#ifndef SO_TIMESTAMPING
  #define SO_TIMESTAMPING         37
#endif

enum {
  SOF_TIMESTAMPING_TX_HARDWARE = (1<<0),
  SOF_TIMESTAMPING_TX_SOFTWARE = (1<<1),
  SOF_TIMESTAMPING_RX_HARDWARE = (1<<2),
  SOF_TIMESTAMPING_RX_SOFTWARE = (1<<3),
  SOF_TIMESTAMPING_SOFTWARE = (1<<4),
  SOF_TIMESTAMPING_SYS_HARDWARE = (1<<5),
  SOF_TIMESTAMPING_RAW_HARDWARE = (1<<6),
        SOF_TIMESTAMPING_MASK =
  (SOF_TIMESTAMPING_RAW_HARDWARE - 1) |
        SOF_TIMESTAMPING_RAW_HARDWARE
};


#include <onload/extensions.h>
#include <onload/extensions_zc.h>

#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)

namespace hf_cwtest {
  using namespace std;
  using namespace hf_tools;
  
  void
  TestOnLoad_Dispatcher::init(const string& name, MDManager* md, const Parameter& params)
  {
    Simple_Dispatcher::init(name, md, params);
    pUdpBuffer_offset=0;
    m_onload_set_spin = params.getDefaultBool("onload_set_spin", m_onload_set_spin);
  }
  
  void
  TestOnLoad_Dispatcher::run()
  {
    m_started = true;
    
    if(!m_app_run_loop) {
      printf("TestOnLoad_Dispatcher::run %s Starting reader thread\n", m_name.c_str());
      m_thread = boost::thread(&TestOnLoad_Dispatcher::run_loop, this);
    } else {
      m_app_run_loop->add_service(this);
      printf("TestOnLoad_Dispatcher::run %s Added reader to run loop %s\n", m_name.c_str(), m_app_run_loop->name().c_str());
    }
  }
  
  inline static
  void
  handle_timestamping(Simple_Dispatcher::Context* context, msghdr* msg)
  {
    
    struct cmsghdr* cmsg;
    for( cmsg = CMSG_FIRSTHDR(msg); cmsg; cmsg = CMSG_NXTHDR(msg,cmsg) ) {
      if(cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING) {
        struct timespec* ts = (struct timespec*) CMSG_DATA(cmsg);
        Time hw_recv_time = Time(((uint64_t)ts[2].tv_sec * Time_Constants::ticks_per_second) + (uint64_t)ts[2].tv_nsec);
        context->handler->set_hw_recv_time(hw_recv_time);
        if(context->handler->provider()->collect_latency_stats()) {
          context->handler->provider()->stats_collector().current_timestamps().hardware_read = hw_recv_time;
        }
        return;
      }
    }
  }
  
  //static
  onload_zc_callback_rc
  TestOnLoad_Dispatcher::onload_callback(struct onload_zc_recv_args* args, int flags)
  {
    Simple_Dispatcher::Context* context = static_cast<Simple_Dispatcher::Context*>(args->user_ptr);
    if(context->handler->provider()->collect_latency_stats()) {
      context->handler->provider()->stats_collector().current_timestamps().dispatcher_read = Time::current_time();
      context->handler->provider()->stats_collector().current_timestamps().after_commit = Time();
    }
    struct msghdr& msghdr = args->msg.msghdr;
    
    handle_timestamping(context, &msghdr);
    
    for(unsigned int i = 0; i < msghdr.msg_iovlen; ++i) {
      const char* buf = static_cast<const char*>(args->msg.iov[i].iov_base);
      size_t msgrecv_len = args->msg.iov[i].iov_len;
      
      if(likely(msgrecv_len >= 0)) {
        if(context->handler->provider()->collect_latency_stats()) {
          context->handler->provider()->stats_collector().current_timestamps().before_handler_parse = Time::current_time();
        }
        context->handler->parse(context->handler_context, buf, msgrecv_len);
        ++(context->recv_count);
        context->recv_bytes += msgrecv_len;
      }
    }
    if(context->handler->provider()->collect_latency_stats() && context->handler->provider()->stats_collector().current_timestamps().after_commit != Time()) {
      context->handler->provider()->stats_collector().finished_parsing_packet();
    }
    
    return ONLOAD_ZC_CONTINUE;
    /*
    if(zc_rc > 0) {
      return ONLOAD_ZC_CONTINUE;
    } else {
      return ONLOAD_ZC_TERMINATE;
    }
    */
  }

  struct message_block_t {
        uint16_t message_length;
  } __attribute__ ((packed)) msgB;

  struct downstream_header_t {
        char      session[10];
      uint64_t  seq_num;
        uint16_t  message_count;
  } __attribute__ ((packed)) packetH;

  size_t
  TestOnLoad_Dispatcher::poll_one(onload_zc_recv_args* zc_args)
  {
    //static const char* where = "TestOnLoad_Dispatcher::poll_one";
    
    if(unlikely(!m_started)) return 0;
    
    int nfds = epoll_wait(m_ep_fd, m_events.get(), max_events, m_timeout);
    
    if(nfds) {
      if(unlikely(nfds == -1)) {
        if(m_first_error) {
          send_alert("TestOnLoad_Dispatcher::poll_one %s epoll_wait returned %d %s, only logging once", m_name.c_str(), errno, strerror(errno));
          m_first_error = false;
        }
        return -1;
      }
      
      //mutex_t::scoped_lock lock(m_mutex);
      for(int n = 0; n < nfds; ++n) {
        int fd = m_events[n].data.fd;
printf("TestOnLoad_Dispatcher::poll: event on fd %d ", fd);
        size_t context_idx = -1;
        if(likely((size_t)fd < m_fd_to_context.size())) {
          context_idx = m_fd_to_context[fd];
        }
        if(likely(context_idx != (size_t)-1)) {
          Context& context = m_contexts[context_idx];
          //printf("TestOnLoad_Dispatcher::poll_one: %s received event on context handler %s\n", m_name.c_str(), context.handler->name().c_str());
          if(context.udp) {
		  printf(" udp\n");
		  zc_args->user_ptr = &context;
            int rc = onload_zc_recv(fd, zc_args);
            if(rc == ENOTEMPTY) {
              struct msghdr msgs;
              onload_recvmsg_kernel(fd, &msgs, ONLOAD_MSG_DONTWAIT);
              for(unsigned int i = 0; i < msgs.msg_iovlen; ++i) {
                const char* buf = static_cast<const char*>(msgs.msg_iov[i].iov_base);
                size_t msgrecv_len = msgs.msg_iov[i].iov_len;
                if(msgrecv_len >= 0) {
                  context.handler->parse(context.handler_context, buf, msgrecv_len);
                  ++context.recv_count;
                  context.recv_bytes += msgrecv_len;
                }
              }
            } else if(rc == -ESOCKTNOSUPPORT) {
              for(;;) {
                struct sockaddr_in from_addr;
                socklen_t from_addr_len = sizeof(sockaddr_in);
                ssize_t msgrecv_len = recvfrom(fd, m_buffer, m_buffer_len, MSG_DONTWAIT, (struct sockaddr*)& from_addr, &from_addr_len);
                if(msgrecv_len >= 0) {
                  if(context.source_ip == 0 || from_addr.sin_addr.s_addr == context.source_ip) {
printf("TestOnLoad_Dispatcher::poll_one: %s handler\n",  context.handler->name().c_str());
                    context.handler->parse(context.handler_context, m_buffer, msgrecv_len);
                    ++context.recv_count;
                    context.recv_bytes += msgrecv_len;
                  } else {
                    char got[INET_ADDRSTRLEN];
                    char expected[INET_ADDRSTRLEN];
                    
                    inet_ntop(AF_INET, &(from_addr.sin_addr), got, INET_ADDRSTRLEN);
                    inet_ntop(AF_INET, &(context.source_ip), expected, INET_ADDRSTRLEN);
                    
                    printf("TestOnLoad_Dispatcher::poll_one: %s %s received message from unknown source IP '%s' expected '%s'\n",
                                         m_name.c_str(), context.handler->name().c_str(), got, expected);
                  }
                } else if(errno == EAGAIN) {
                  break;
                } else {
                  char err_buf[512];
                  char* err_msg = strerror_r(errno, err_buf, 512);
                  send_alert("TestOnLoad_Dispatcher::run %s recv returned %d - %s", name().c_str(), errno, err_msg);
                }
              }
            } else if(rc != 0) {
              //printf("TestOnLoad_Dispatcher::poll_one: %s received event on context handler %s rc= %d\n", m_name.c_str(), context.handler->name().c_str(), rc);
		    //use format to read
		    //1. get count
		    //2 then use msg length for that count
                  ssize_t msgrecv_len=0;
		  size_t loc=20;
		  //char *pBuf=m_udp_buffer;
      		  //msgrecv_len = ::read(fd, m_udp_buffer, 0x00FFFFFE);
      		  	::read(fd, &packetH, sizeof(packetH));
		  if (!errno && errno != EAGAIN) break;
	  memcpy(&m_udp_buffer[pUdpBuffer_offset], &packetH, sizeof(packetH));
		  pUdpBuffer_offset += sizeof(packetH);
	      
	          uint16_t message_count = ntohs(packetH.message_count);
		  uint32_t packet_len=sizeof(packetH);
		  size_t r=0;
		  do{
      		  	::read(fd, &msgB, sizeof(msgB));
			if (!errno && errno != EAGAIN) {
				pUdpBuffer_offset -= packet_len;
				break;
			}
	  	memcpy(&m_udp_buffer[pUdpBuffer_offset], &msgB, sizeof(msgB));
			packet_len += sizeof(msgB);
			pUdpBuffer_offset += sizeof(msgB);
			uint16_t ll=ntohs(msgB.message_length);
			::read(fd, &m_udp_buffer[pUdpBuffer_offset], ll);
			if (!errno && errno != EAGAIN) {
				pUdpBuffer_offset -= packet_len;
				break;
			}
			packet_len += ll;
			pUdpBuffer_offset += ll;
		  } while (r++ < message_count);
	printf("%s ch.%d get %lu bytes\n", context.handler->name().c_str(),
			context.handler_context, packet_len);
	context.handler->parse(context.handler_context, 
		&m_udp_buffer[pUdpBuffer_offset - packet_len], packet_len);
			pUdpBuffer_offset -= packet_len;
               }
          
	  } else {
		  printf(" tcp\n");
            TCP_Buffer* buf = m_fd_to_tcp_buffer[fd];
            if(unlikely(!buf)) {
              printf("TestOnLoad_Dispatcher::poll_one: %s Received message on fd %d which claims to be TCP but does not have allocated buffer\n",
                                   m_name.c_str(), fd);
              continue;
            }
            
            for(;;) {
              ssize_t read_len = ::read(fd, buf->buffer + buf->buffer_end, buf->buffer_len - buf->buffer_end);
              if(read_len <= 0) {
                if(errno != EAGAIN) {
                  string err = errno_to_string();
                  send_alert("TestOnLoad_Dispatcher::poll_one: %s error reading socket %d %d - %s", m_name.c_str(), fd, errno, err.c_str());
                  remove_fd(fd);
                  break;
                }
                // We must have been woke up for a write
                context.handler->read(context.handler_context, 0, 0);
                break;
              }
              ++context.recv_count;
              context.recv_bytes += read_len;
              buf->buffer_end += read_len;
              
              unsigned int remaining = buf->buffer_end;
              char* tcp_buffer_begin = buf->buffer;
              while(remaining >= 0) {
                unsigned int msg_size = context.handler->read(context.handler_context, tcp_buffer_begin, remaining);
                if(msg_size == 0) {
                  break;
                }
                tcp_buffer_begin += msg_size;
                remaining-= msg_size;
              }
              
              if(remaining == 0) {
                buf->buffer_end = 0;
              } else {
                memmove(buf->buffer, tcp_buffer_begin, remaining);
                buf->buffer_end = remaining;
              }
            }
          }
        }
      }
    }
    return nfds;
  }
  
  
  void
  TestOnLoad_Dispatcher::run_loop()
  {
    const char* where = "TestOnLoad_Dispatcher::run_loop";
    
    if(m_md) {
      m_md->app().set_thread_name(syscall(SYS_gettid), name().c_str());
    }
    
    if (m_cpu_bind>0) {
      cpu_set_t mask; 
      CPU_ZERO(&mask); 
      CPU_SET(m_cpu_bind, &mask);
      int ret = sched_setaffinity(0, sizeof mask, &mask); 
      if (ret==0) {
	printf("TestOnLoad_Dispatcher::run_loop %s, CPU bind success %d\n", m_name.c_str(), m_cpu_bind);
      } else {
	printf("TestOnLoad_Dispatcher::run_loop %s, CPU bind failed %d, error [%s]\n",m_name.c_str(),m_cpu_bind,::strerror(errno));
      }
    }
    
    if(0 != onload_set_stackname(ONLOAD_THIS_THREAD, ONLOAD_SCOPE_THREAD, m_name.c_str())) {
      printf("TestOnLoad_Dispatcher::run_loop %s, onload_set_stackname failed '%s', error [%s]\n",m_name.c_str(), "",::strerror(errno));
    }
    
    if(m_onload_set_spin) {
      onload_thread_set_spin(ONLOAD_SPIN_ALL, 1);
      printf("TestOnLoad_Dispatcher::run_loop %s, onload_set_spin=1\n",m_name.c_str());
    }
    
    if(m_ep_fd != -1) {
      ::close(m_ep_fd);
    }
    m_ep_fd = epoll_create(300);
    if(m_ep_fd < 0) {
      throw hf_tools::errno_exception("Disaptcher::TestOnLoad_Dispatcher::start_sync: Error calling epoll_create", errno);
    }
    
    char buf[1024];
    
    struct onload_zc_recv_args zc_args;
    memset(&zc_args, 0, sizeof(zc_args));
    zc_args.cb = onload_callback;
    //zc_args.user_ptr = 0;
    zc_args.flags = ONLOAD_MSG_DONTWAIT | ONLOAD_MSG_RECV_OS_INLINE;
    
    Time time_since_last_flush = Time::current_time();
    while(m_started) {
      zc_args.msg.msghdr.msg_control = buf;
      zc_args.msg.msghdr.msg_controllen = 1024;
      
      TestOnLoad_Dispatcher::poll_one(&zc_args);
      if(!m_flush_buffers.empty()) {
        Time t = Time::current_time();
        if(time_since_last_flush + msec(200) > t) {
          for(vector<LoggerRP>::iterator i = m_flush_buffers.begin(), i_end = m_flush_buffers.end(); i != i_end; ++i) {
            (*i)->flush_if_half_full();
          }
        } else {
          for(vector<LoggerRP>::iterator i = m_flush_buffers.begin(), i_end = m_flush_buffers.end(); i != i_end; ++i) {
            (*i)->flush();
          }
          Time t2 = Time::current_time();
          time_since_last_flush = t2;
        }
      }
      
      handle_requests();
    }
    
    printf("%s: %s Finished reader thread\n", where, m_name.c_str());
  }
  
  void
  TestOnLoad_Dispatcher::start(Handler* h, const string& name)
  {
    m_requests.push(boost::bind(&TestOnLoad_Dispatcher::start_sync, this, h, name));
  } 
  
  void
  TestOnLoad_Dispatcher::stop(Handler* h, const string& name)
  {
    m_requests.push(boost::bind(&TestOnLoad_Dispatcher::stop_sync, this, h, name));
  }
  
  void
  TestOnLoad_Dispatcher::start_sync(Handler* h, const string& name)
  {
    const char* where = "TestOnLoad_Dispatcher::start";
    
    for(vector<Context>::iterator c_i = m_contexts.begin(), c_end = m_contexts.end(); c_i != c_end; ++c_i) {
      Context& context = *c_i;
      size_t context_idx = c_i - m_contexts.begin();
      
      if(context.handler == h && context.name == name && !context.subscribed) {
        if(context.udp) {
          if(!context.externally_subscribed) {
            if ((context.fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
              throw runtime_error(where + m_name + " socket() failed");
            }
            
            printf("%s: %s creating %s socket %d for handler %s port %d\n", where, m_name.c_str(),
                                 context.mcast ? "mcast" : "udp", context.fd,
                                 context.handler->name().c_str(), context.port);
            
            int flag_on = 1;
            /* set reuse port to on to allow multiple binds per host */
            if ((setsockopt(context.fd, SOL_SOCKET, SO_REUSEADDR, &flag_on, sizeof(flag_on))) < 0) {
              throw errno_exception(where + m_name + " socket() failed", errno);
            }
            
            /* Give us a nice big buffer */
            if ((setsockopt(context.fd, SOL_SOCKET, SO_RCVBUF, &m_recv_buffer_size, sizeof(m_recv_buffer_size))) < 0) {
              throw errno_exception(where + m_name + " setsockopt(SO_RCVBUF) failed", errno);
            }
           
            struct sockaddr_in bind_addr;
            memset(&bind_addr, 0, sizeof(bind_addr));
            bind_addr.sin_family      = AF_INET;
            if(context.mcast) {
              bind_addr.sin_addr.s_addr = context.group_addr;
              //bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            } else if(context.iface_addr == 0) {
              bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            } else {
              bind_addr.sin_addr.s_addr = context.iface_addr;
            }
            bind_addr.sin_port        = htons(context.port);
            
            if ((bind(context.fd, (struct sockaddr *) &bind_addr, sizeof(bind_addr))) < 0) {
	      char buf[128];
	      printf("%s %s bind() failed for port %d \n", where, m_name.c_str(), context.port);
	      snprintf(buf, 128, "%s %s bind() failed for port %d \n", where, m_name.c_str(), context.port);
              throw errno_exception(buf, errno);
            }
            
            if(context.mcast) {
              struct ip_mreq mc_req;
              mc_req.imr_multiaddr.s_addr = context.group_addr;
              mc_req.imr_interface.s_addr = context.iface_addr;
              
              /* send an ADD MEMBERSHIP message via setsockopt */
              if ((setsockopt(context.fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mc_req, sizeof(mc_req))) < 0) {
                throw errno_exception(where + m_name + " setsockopt(IP_ADD_MEMBERSHIP) failed for " + h->name() + " " + name + " ", errno);
              }
            }
          }
         
          if(m_timestamp_on) {
            int enable = 1;
            if(m_hw_timestamp_on) {
              enable = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
            }
            if ((setsockopt(context.fd, SOL_SOCKET, SO_TIMESTAMPING, &enable, sizeof(int))) < 0) {
              printf("%s: Enable timestamping failed\n", m_name.c_str());
            } else {
              printf("%s: setsockopt timestamping enabled\n", m_name.c_str());
            }
          }
          
          add_fd(context.fd, context_idx, false, false);
          context.subscribed = true;
        } else {
          add_fd(context.fd, context_idx, true, true);
          context.subscribed = true;
        }
      }
    }
    
  }
  
  void
  TestOnLoad_Dispatcher::stop_sync(Handler* h, const string& name)
  {    
    for(vector<Context>::iterator c_i = m_contexts.begin(), c_end = m_contexts.end(); c_i != c_end; ++c_i) {
      Context& context = *c_i;
      if(context.handler == h && context.name == name && context.subscribed && context.udp) {
        remove_fd(context.fd);
        if(!context.externally_subscribed) {
          close(context.fd);
          context.fd = -1;
        }
        context.subscribed = false;
      }
    }
  }
  
  void
  TestOnLoad_Dispatcher::status(ILogger& logger) const
  {
    printf("Dispatcher: %s\n\n", m_name.c_str());
    
    for(vector<Context>::const_iterator i = m_contexts.begin(), i_end = m_contexts.end(); i != i_end; ++i) {
      printf("  handler: %20s %5s %3zd recv_count: %20zd   recv_bytes: %20zd\n\n",
                    i->handler->name().c_str(), i->name.c_str(), i->handler_context, i->recv_count, i->recv_bytes);
    }
  }
  
  
  TestOnLoad_Dispatcher::TestOnLoad_Dispatcher() :
    Simple_Dispatcher(),
    m_onload_set_spin(false)
  {
  }
  
  TestOnLoad_Dispatcher::~TestOnLoad_Dispatcher()
  {
  }


}
