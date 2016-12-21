#include <QBT/EPoll_Dispatcher.h>
#include <Data/MDBufferQueue.h>

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

#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)

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

namespace kou_connect {
  using namespace std;
  using namespace kou_tools;
  
  static
  int setnonblocking(int fd)
  {
    int flags;
    
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
      flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }
  
  void
  EPoll_Dispatcher::init(const string& name, MDManager* md, const Parameter& params)
  {
    MDDispatcher::init(name, md, params);
    
    m_recv_buffer_size = params.getDefaultInt("recv_buffer_size", m_recv_buffer_size);
    m_cpu_bind = params.getDefaultInt("cpu_bind",m_cpu_bind);
    m_timestamp_on = params.getDefaultBool("timestamp", false);
    m_hw_timestamp_on = params.getDefaultBool("hw_timestamp", false);
    
    if(m_timestamp_on) {
      if(m_hw_timestamp_on) {
        m_logger->log_printf(Log::INFO, "%s:  Hardware timestamping enabled", m_name.c_str());
      } else {
        m_logger->log_printf(Log::INFO, "%s:  Software timestamping enabled", m_name.c_str());
      }
    }
    if(m_app_run_loop) {
      m_timeout = 0; // Switch into non-blocking mode if we're part of a run loop
    }
  }
  
  void
  EPoll_Dispatcher::add_fd(int fd, size_t context_idx, bool writeable, bool et)
  {
    struct epoll_event ev;
    
    setnonblocking(fd);
    
    ev.events = EPOLLIN;
    if(et) {
      ev.events |= EPOLLET;
    }
    if(writeable) {
      ev.events |= EPOLLOUT;
    }
    ev.data.fd = fd;
    
    if (epoll_ctl(m_ep_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
      char buf[256];
      snprintf(buf, 256, "EPoll_Dispatcher::add_fd: %s: epoll set insertion error fd=%d %s", m_name.c_str(), fd, strerror(errno));
      throw runtime_error(buf);
    }
    
    while(m_fd_to_context.size() <= (size_t)fd) {
      m_fd_to_context.resize(m_fd_to_context.size() * 2, -1);
      m_fd_to_tcp_buffer.resize(m_fd_to_context.size() * 2, 0);
    }
    m_fd_to_context[fd] = context_idx;
  }
  
  void
  EPoll_Dispatcher::remove_fd(int fd)
  {
    struct epoll_event ev; // ignored
    epoll_ctl(m_ep_fd, EPOLL_CTL_DEL, fd, &ev);
    m_fd_to_context[fd] = -1;
    if(m_fd_to_tcp_buffer[fd]) {
      delete m_fd_to_tcp_buffer[fd];
      m_fd_to_tcp_buffer[fd] = 0;
    }
  }

  void
  EPoll_Dispatcher::clear()
  {
    struct epoll_event ev; // ignored
    for(vector<Context>::iterator i = m_contexts.begin(); i != m_contexts.end(); ++i) {
      if(i->subscribed) {
        int fd = i->fd;
        epoll_ctl(m_ep_fd, EPOLL_CTL_DEL, fd, &ev);
        if(!i->externally_subscribed) {
          ::close(fd);
        }
        i->subscribed = false;
      }
    }
    
    for(size_t fd = 0; fd < m_fd_to_context.size(); ++fd) {
      m_fd_to_context[fd] = -1;
      if(m_fd_to_tcp_buffer[fd]) {
        delete m_fd_to_tcp_buffer[fd];
        m_fd_to_tcp_buffer[fd] = 0;
      }
    }
  }

  void
  EPoll_Dispatcher::run()
  {
    m_started = true;
    
    if(!m_app_run_loop) {
      m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::run %s Starting reader thread", m_name.c_str());
      m_thread = boost::thread(&EPoll_Dispatcher::run_loop, this);
    } else {
      m_app_run_loop->add_service(this);
      m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::run %s Added reader to run loop %s", m_name.c_str(), m_app_run_loop->name().c_str());
    }
  }
  
  size_t
  EPoll_Dispatcher::poll_one()
  {
    static const char* where = "EPoll_Dispatcher::poll_one";
    
    if(!m_started) return 0;
    
    struct sockaddr_in from_addr;
    socklen_t from_addr_len = sizeof(sockaddr_in);
    
    int nfds = epoll_wait(m_ep_fd, m_events.get(), max_events, m_timeout);
    
    if(nfds) {
      if(nfds == -1) {
        if(m_first_error) {
          send_alert("EPoll_Dispatcher::poll_one %s epoll_wait returned %d %s, only logging once", m_name.c_str(), errno, strerror(errno));
          m_first_error = false;
        }
        return -1;
      }
      
      mutex_t::scoped_lock lock(m_mutex);
      for(int n = 0; n < nfds; ++n) {
        int fd = m_events[n].data.fd;
        //m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::poll_one: %s received event on fd %d", m_name.c_str(), fd);
        size_t context_idx = -1;
        if(likely((size_t)fd < m_fd_to_context.size())) {
          context_idx = m_fd_to_context[fd];
        }
        if(likely(context_idx != (size_t)-1)) {
          Context& context = m_contexts[context_idx];
          //m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::poll_one: %s received event on context handler %s", m_name.c_str(), context.handler->name().c_str());
          if(context.udp) {
            for(;;) {
              ssize_t msgrecv_len = recvfrom(fd, m_buffer, m_buffer_len, MSG_DONTWAIT, (struct sockaddr*)& from_addr, &from_addr_len);
              if(msgrecv_len >= 0) {
                if(context.source_ip == 0 || from_addr.sin_addr.s_addr == context.source_ip) {
                  //m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::poll_one: %s passed event on context handler", m_name.c_str(), context.handler->name().c_str());
                  context.handler->parse(context.handler_context, m_buffer, msgrecv_len);
                  ++context.recv_count;
                  context.recv_bytes += msgrecv_len;
                } else {
                  char got[INET_ADDRSTRLEN];
                  char expected[INET_ADDRSTRLEN];
                  
                  inet_ntop(AF_INET, &(from_addr.sin_addr), got, INET_ADDRSTRLEN);
                  inet_ntop(AF_INET, &(context.source_ip), expected, INET_ADDRSTRLEN);
                  
                  m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::poll_one: %s %s received message from unknown source IP '%s' expected '%s'",
                                       m_name.c_str(), context.handler->name().c_str(), got, expected);
                }
              } else if(errno == EAGAIN) {
                break;
              } else {
                char err_buf[512];
                char* err_msg = strerror_r(errno, err_buf, 512);
                send_alert("EPoll_Dispatcher::run %s recv returned %d - %s", name().c_str(), errno, err_msg);
              }
            }
          } else {
            TCP_Buffer* buf = m_fd_to_tcp_buffer[fd];
            if(unlikely(!buf)) {
              m_logger->log_printf(Log::ERROR, "%s: %s Received message on fd %d which claims to be TCP but does not have allocated buffer",
                                   where, m_name.c_str(), fd);
              continue;
            }
            
            for(;;) {
              ssize_t read_len = ::read(fd, buf->buffer + buf->buffer_end, buf->buffer_len - buf->buffer_end);
              if(read_len <= 0) {
                if(read_len == 0) {
                  send_alert("%s: %s socket %d returned EOF", where, m_name.c_str(), fd);
                  remove_fd(fd);
                  break;
                } else if(errno != EAGAIN) {
                  string err = errno_to_string();
                  send_alert("%s: %s error reading socket %d %d - %s", where, m_name.c_str(), fd, errno, err.c_str());
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
  EPoll_Dispatcher::run_loop()
  {
    const char* where = "EPoll_Dispatcher::run_loop";
    
    if(m_md) {
      m_md->app().set_thread_name(syscall(SYS_gettid), name().c_str());
    }
    
    if (m_cpu_bind>0) {
      cpu_set_t mask; 
      CPU_ZERO(&mask); 
      CPU_SET(m_cpu_bind, &mask); 
      int ret = sched_setaffinity(0, sizeof mask, &mask); 
      if (ret==0) {
	m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::run_loop %s, CPU bind success %d", m_name.c_str(), m_cpu_bind);
      } else {
	m_logger->log_printf(Log::ERROR, "EPoll_Dispatcher::run_loop %s, CPU bind failed %d, error [%s]",m_name.c_str(),m_cpu_bind,::strerror(errno));
      }
    }
    
    Time time_since_last_flush = Time::current_time();
    while(m_started) {
      EPoll_Dispatcher::poll_one();
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
    }
    
    m_logger->log_printf(Log::INFO, "%s: %s Finished reader thread", where, m_name.c_str());
  }

  void
  EPoll_Dispatcher::mcast_join(Handler* handler, const string& name, size_t handler_context, const string& group, int port, const string& iface, const string& source, bool bind_only)
  {
    const char where[] = "EPoll_Dispatcher::mcast_join ";
    mutex_t::scoped_lock lock(m_mutex);

    m_logger->log_printf(Log::INFO, "%s: %s handler %s %s:%d %s %s", where, m_name.c_str(), handler->name().c_str(), group.c_str(),
                         port, iface.c_str(), source.c_str());

    uint32_t group_addr = inet_addr(group.c_str());
    uint32_t source_addr = 0;
    if(!source.empty()) {
      source_addr = inet_addr(source.c_str());
    }
    
    uint32_t iface_addr = htonl(INADDR_ANY);
    if(!iface.empty()) {
      // create a socket just for the request
      int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

      ifreq req;
      strncpy(req.ifr_ifrn.ifrn_name, iface.c_str(), IFNAMSIZ);
      req.ifr_addr.sa_family = AF_INET;

      if(ioctl(fd, SIOCGIFADDR, (char*)&req) == -1) {
        throw errno_exception(where + m_name + " ioctl(SIOCGIFADDR) failed ("+ iface + ")- likely invalid interface ", errno);
      }
      iface_addr = ((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr;
      close(fd);
    }

    m_contexts.push_back(Context(handler, name, handler_context, true, true, source_addr, group_addr, iface_addr, port));
  }

  void
  EPoll_Dispatcher::udp_bind(Handler* handler, const string& name, size_t handler_context, const string& host, int port)
  {
    const char where[] = "EPoll_Dispatcher::udp_bind ";
    mutex_t::scoped_lock lock(m_mutex);

    m_logger->log_printf(Log::INFO, "%s: %s handler %s %d %s", where, m_name.c_str(), handler->name().c_str(), port, host.c_str());

    uint32_t group_addr = 0;
    uint32_t source_addr = 0;
    if(!host.empty()) {
      source_addr = inet_addr(host.c_str());
    }
    uint32_t iface_addr = 0;

    m_contexts.push_back(Context(handler, name, handler_context, true, false, source_addr, group_addr, iface_addr, port));
  }
  
  void
  EPoll_Dispatcher::add_udp_fd(Handler* handler, const string& name, size_t context, int fd)
  {
    m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::add_udp_fd %s Handler %s %s fd=%d", m_name.c_str(), handler->name().c_str(), name.c_str(), fd);
    m_contexts.push_back(Context(handler, name, context, true, false, 0, 0, 0, 0));
    m_contexts.back().fd = fd;
    m_contexts.back().subscribed = false;
    m_contexts.back().externally_subscribed = true;
  }
  
  void
  EPoll_Dispatcher::add_tcp_fd(Handler* handler, const string& name, size_t context, int fd, size_t buf_size)
  {
    m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::add_tcp_fd %s Handler %s %s fd=%d", m_name.c_str(), handler->name().c_str(), name.c_str(), fd);
    m_contexts.push_back(Context(handler, name, context, false, false, 0, 0, 0, 0));
    m_contexts.back().fd = fd;
    m_contexts.back().subscribed = false;
    m_contexts.back().externally_subscribed = true;
    
    TCP_Buffer* buf = m_fd_to_tcp_buffer[fd] = new TCP_Buffer();
    buf->init(buf_size);
  }
  
  void
  EPoll_Dispatcher::start(Handler* h, const string& name)
  {
    mutex_t::scoped_lock lock(m_mutex);
    start_sync(h, name);
  }
  
  void
  EPoll_Dispatcher::start_sync(Handler* h, const string& name)
  {    
    static const char* where = "EPoll_Dispatcher::start";
    if(h->recorder()) {
      m_flush_buffers.push_back(h->recorder());
      h->recorder()->set_auto_flush(false);
    }
    
    for(vector<Context>::iterator c_i = m_contexts.begin(), c_end = m_contexts.end(); c_i != c_end; ++c_i) {
      Context& context = *c_i;
      size_t context_idx = c_i - m_contexts.begin();
      
      if(context.handler == h && context.name == name && !context.subscribed) {
        if(context.udp) {
          if(!context.externally_subscribed) {
            if ((context.fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
              throw runtime_error(where + m_name + " socket() failed");
            }
            
            m_logger->log_printf(Log::INFO, "%s: %s creating %s socket %d for handler %s port %d", where, m_name.c_str(),
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
	      snprintf(buf, 128, "%s %s bind() failed for port %d ", where, m_name.c_str(), context.port);
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
          add_fd(context.fd, context_idx, false, true);
          context.subscribed = true;
        } else {
          add_fd(context.fd, context_idx, true, true);
          context.subscribed = true;
        }
      }
      if(context.handler == h && context.name == name){
      	if (context.udp)
      	h->sync_udp_channel(context.name, context.handler_context, context.fd);
      	else
      	h->sync_tcp_channel(context.name, context.handler_context, context.fd);
      } 
    }
  }
  
  void
  EPoll_Dispatcher::stop(Handler* h, const string& name)
  {
    mutex_t::scoped_lock lock(m_mutex);
    stop_sync(h, name);
  }
  
  void
  EPoll_Dispatcher::stop_sync(Handler* h, const string& name)
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
  EPoll_Dispatcher::status(ILogger& logger) const
  {
    logger.printf("Dispatcher: %s\n", m_name.c_str());
    
    for(vector<Context>::const_iterator i = m_contexts.begin(), i_end = m_contexts.end(); i != i_end; ++i) {
      logger.printf("  handler: %20s %5s %3zd recv_count: %20zd   recv_bytes: %20zd\n",
                    i->handler->name().c_str(), i->name.c_str(), i->handler_context, i->recv_count, i->recv_bytes);
    }
  }
  
  void
  EPoll_Dispatcher::full_stop()
  {
    m_started = false;
    if(m_thread.joinable()) {
      m_thread.join();
    }
    
    if(m_app_run_loop) {
      m_app_run_loop->remove_service(this);
      m_logger->log_printf(Log::INFO, "EPoll_Dispatcher::full_stop %s Removed reader from run loop %s", m_name.c_str(), m_app_run_loop->name().c_str());
    }
    
    clear();
    if(m_ep_fd > 0) {
      close(m_ep_fd);
      m_ep_fd = -1;
    }
  }
  
  EPoll_Dispatcher::EPoll_Dispatcher() :
    m_started(false),
    m_ep_fd(-1),
    m_timeout(1 * 1000), // 1 second
    m_recv_buffer_size(64 * 1024 * 1024),
    m_first_error(true),
    m_cpu_bind(-1)
  {
    m_ep_fd = epoll_create(300);
    if(m_ep_fd < 0) {
      throw kou_tools::errno_exception("Disaptcher::EPoll_Dispatcher: Error calling epoll_create", errno);
    }
    
    m_fd_to_context.resize(2048, -1);
    m_fd_to_tcp_buffer.resize(2048, 0);
    m_events.reset(new epoll_event[max_events]);
  }
  
  EPoll_Dispatcher::~EPoll_Dispatcher()
  {
    full_stop();
  }


}
