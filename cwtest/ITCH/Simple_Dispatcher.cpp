#include <ITCH/functions.h>
#include <ITCH/Simple_Dispatcher.h>
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
#include <time.h>

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

namespace hf_cwtest {
  using namespace std;
  using namespace hf_tools;
  using namespace hf_core;
  
  static
  int setnonblocking(int fd)
  {
    int flags;
    
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
      flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }
  
  void
  Simple_Dispatcher::init(const string& name, MDManager* md, const Parameter& params)
  {
	  /*
    MDDispatcher::init(name, md, params);
    
    m_recv_buffer_size = params.getDefaultInt("recv_buffer_size", m_recv_buffer_size);
    m_cpu_bind = params.getDefaultInt("cpu_bind",m_cpu_bind);
    m_timestamp_on = params.getDefaultBool("timestamp", false);
    m_hw_timestamp_on = params.getDefaultBool("hw_timestamp", false);
    
    if(m_timestamp_on) {
      if(m_hw_timestamp_on) {
        printf("%s:  Hardware timestamping enabled\n", m_name.c_str());
      } else {
        printf("%s:  Software timestamping enabled\n", m_name.c_str());
      }
    }
    m_timeout = 20;
    string sTmOut=params.getDefaultString("poll_timeout", "20");
    m_timeout=stoi(trim(sTmOut));
    if(m_app_run_loop) {
      m_timeout = 0; // Switch into non-blocking mode if we're part of a run loop
    }
    */
    m_timeout = 2;
    m_timeout=stoi(params.getDefaultString("poll_timeout", "10").c_str());
  }
  
  void
  Simple_Dispatcher::add_fd(int fd, size_t context_idx, bool writeable, bool et)
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
      snprintf(buf, 256, "Simple_Dispatcher::add_fd: %d insertion poll_fd=%d error %s\n",  fd, m_ep_fd, strerror(errno));
      throw runtime_error(buf);
    }
    
      printf("%s inserted %d to poll_fd=%d \n",  __func__, fd, m_ep_fd);
    while(m_fd_to_context.size() <= (size_t)fd) {
      m_fd_to_context.resize(m_fd_to_context.size() * 2, -1);
      m_fd_to_tcp_buffer.resize(m_fd_to_context.size() * 2, 0);
    }
    m_fd_to_context[fd] = context_idx;
  }
  
  void
  Simple_Dispatcher::remove_fd(int fd)
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
  Simple_Dispatcher::clear()
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
  Simple_Dispatcher::run()
  {
    m_started = true;
    
    if(!m_app_run_loop) {
      printf("Simple_Dispatcher::run %s Starting reader thread\n", m_name.c_str());
      m_thread = boost::thread(&Simple_Dispatcher::run_loop, this);
    } else {
      m_app_run_loop->add_service(this);
      printf("Simple_Dispatcher::run %s Added reader to run loop %s\n", m_name.c_str(), m_app_run_loop->name().c_str());
    }
  }
  
  size_t
  Simple_Dispatcher::poll_one()
  {
    
    if(!m_started) return 0;
    
    struct sockaddr_in from_addr;
    socklen_t from_addr_len = sizeof(sockaddr_in);
    
    int nfds = epoll_wait(m_ep_fd, m_events.get(), max_events, m_timeout);
    
    if(nfds) {
      if(nfds == -1) {
        //if(m_first_error) {
          printf("%s epoll_wait returned %s\n", __func__, strerror(errno));
          //m_first_error = false;
        //}
        return -1;
      }
      
      //mutex_t::scoped_lock lock(m_mutex);
      for(int n = 0; n < nfds; ++n) {
        int fd = m_events[n].data.fd;
	cout << "Poll found " << fd << endl;
        //printf("Simple_Dispatcher::poll_one: %s received event on fd %d", m_name.c_str(), fd);
        size_t context_idx = -1;
        if(likely((size_t)fd < m_fd_to_context.size())) {
          context_idx = m_fd_to_context[fd];
        }
        if(likely(context_idx != (size_t)-1)) {
          Context& context = m_contexts[context_idx];
	  cout << context.handler->name();
          //printf("Simple_Dispatcher::poll_one: %s received event on context handler %s", m_name.c_str(), context.handler->name().c_str());
          if(context.udp) {
		  cout << " udp fd=" << fd << " ";
		  if (m_events[n].events & EPOLLOUT){
			  context.handler->to_write(fd);
			  continue;
		  }
	  size_t iTry0=0;
	  size_t udp_boundary_read=0;
            for(;;) {
              ssize_t msgrecv_len=0;
	      if (!m_test_mode) msgrecv_len = recvfrom(fd, m_buffer, m_buffer_len, MSG_DONTWAIT, (struct sockaddr*)& from_addr, &from_addr_len);
	      else {
		      //I have to simulate the udp message boundary here 
		      //so the parse function behaves similar to tcp
		      //will contiue read the data until empty read
		      //the message boundary will be a 0x03 
		      //
		
		      iTry0=0;
		      size_t loc=0;
		      bool foundMB=false;
		      size_t resv=20;
		      do {
	      		  msgrecv_len = ::read(fd, &m_buffer[loc], resv);
				if (msgrecv_len < 0){
					if(errno != EAGAIN) {
		cout << __func__ <<  " read : " << strerror(errno) << endl;
		break;
					}
					if (++iTry0 > 3) break;
					resv /= 2;
					if (resv==0)
					resv=1;
				}
			  	else if (msgrecv_len==0) iTry0++;
			  	else {
					iTry0=0; 
					while (msgrecv_len-- > 0){
					//here is the simulated 
					//message boundary 3x 0x01
					   if (loc > 1 &&
						m_buffer[loc]==UDP_BOUNDARY &&
						m_buffer[loc-1]==UDP_BOUNDARY &&
						m_buffer[loc-2]==UDP_BOUNDARY)
					   {
	printf("%s ch.%d get MB at %d\n", context.handler->name(),
			context.handler_context, loc-2);
	context.handler->parse(context.handler_context, m_buffer, loc-2);
				memcpy(m_buffer, &m_buffer[loc+1], msgrecv_len);
						loc=msgrecv_len;
						msgrecv_len=0;
						continue;
						//foundMB=true;
					//break;
					   }
					   loc++;
					}
				}
		      } while (iTry0 <2);
		      if (loc < 1) break;
		      //following few lines will be reached only error
		      //of the test pipe 
		      while (m_buffer[loc-1]==UDP_BOUNDARY) loc--;
		      msgrecv_len=loc;
		      udp_boundary_read++;
		      cout << __func__ << " udp cycle" <<udp_boundary_read << " read " << msgrecv_len << " bytes\n";
	      }
              if(msgrecv_len >= 0) {
		    if (msgrecv_len==0) iTry0++;
		    else { 
		        iTry0=0;
			if(context.source_ip == 0 || 
				from_addr.sin_addr.s_addr == context.source_ip) {
                  		context.handler->parse(context.handler_context, m_buffer, msgrecv_len);
                  		++context.recv_count;
                  		context.recv_bytes += msgrecv_len;
			} else {
				char got[INET_ADDRSTRLEN];
                  		char expected[INET_ADDRSTRLEN];
           			inet_ntop(AF_INET, &(from_addr.sin_addr), got, INET_ADDRSTRLEN);
           			inet_ntop(AF_INET, &(context.source_ip), expected, INET_ADDRSTRLEN);
                 		cout << __func__ << " recvfrom:(unknown source):"; 
         			printf(" %s expected '%s'\n", got, expected);
				break;
                	}
		    }
		    if (iTry0 > 3) break;
              } else if(errno == EAGAIN) {
		      cout << "recvfrom: got port busy ";
		      if (++iTry0 > 3) break;
              } else {
       printf("%s recv returned %s\n", __func__,  strerror(errno));
       		break;
              }
            }
          } else {
	     cout << " tcp fd \n";
		  if (m_events[n].events & EPOLLOUT){
			  context.handler->to_write(fd);
		  }
		  //if (!m_events[n].events & EPOLLIN)
       	     TCP_Buffer* buf = m_fd_to_tcp_buffer[fd];
            if(unlikely(!buf)) {
              printf("%s: %s Received message on fd %d which claims to be TCP but does not have allocated buffer\n",
                                   __func__, m_name.c_str(), fd);
              continue;
            }
            
	    bool skip=false;
	 	size_t tcpTry=0;
            for(;;) {
		    if (skip) break;
              ssize_t read_len = ::read(fd, buf->buffer + buf->buffer_end, buf->buffer_len - buf->buffer_end);
		tcpTry++;

		bool bad_fd=false;

              if(read_len <= 0) {
		      if (errno == EAGAIN){
			      struct timespec t_req, t_rem;
			      memset(&t_req, 0, sizeof(t_req));
			      t_req.tv_nsec=10;

	      			nanosleep(&t_req, 0);//struct timespec *rem);)
	      			if (tcpTry++ < 3) continue;
	      			break;
		      }
                	if(read_len == 0) {
      printf("%s: socket %d returned EOF\n", __func__,  fd);
	cout << context.handler->name() << " closing " << fd << endl;
			} else {
                  		string err = errno_to_string();
     printf("%s: error reading socket %d %d - %s\n", __func__,  fd, errno, err.c_str());
     			}
                  remove_fd(fd);
		  bad_fd=true;
                  break;
                
		//if (bad_fd){
	//context.handler->read(context.handler_context, 0, UINT_MAX - errno);
                //break;
		//}
		//else
                //context.handler->read(context.handler_context, 0, 0);
              }
	      tcpTry=0;
      cout << context.handler->name() << " Got data len=" << read_len << endl;
              ++context.recv_count;
              context.recv_bytes += read_len;
              buf->buffer_end += read_len;
              
              unsigned int remaining = buf->buffer_end;
              char* tcp_buffer_begin = buf->buffer;
              while(remaining >= 0) {
                unsigned int msg_size = context.handler->read(context.handler_context, tcp_buffer_begin, remaining);
		if (msg_size > UINT_MAX-150)
		{
	cout << context.handler->name() << " closing " << fd << endl;
			skip=true;
			break;
		}
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
    //I have to update here to check if I have something if I don't use the timer to trigger
    //the write
    return nfds;
  }
  
  void
  Simple_Dispatcher::run_loop()
  {
    
    if(m_md) {
      m_md->app().set_thread_name(syscall(SYS_gettid), name().c_str());
    }
    
    if (m_cpu_bind>0) {
      cpu_set_t mask; 
      CPU_ZERO(&mask); 
      CPU_SET(m_cpu_bind, &mask); 
      int ret = sched_setaffinity(0, sizeof mask, &mask); 
      if (ret==0) {
	printf("Simple_Dispatcher::run_loop %s, CPU bind success %d\n", m_name.c_str(), m_cpu_bind);
      } else {
	printf("Simple_Dispatcher::run_loop %s, CPU bind failed %d, error [%s]\n",m_name.c_str(),m_cpu_bind,::strerror(errno));
      }
    }
    
    Time time_since_last_flush = Time::current_time();
    while(m_started) {
      Simple_Dispatcher::poll_one();
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
    
    printf("%s: %s Finished reader thread\n", __func__, m_name.c_str());
  }

  void
  Simple_Dispatcher::mcast_join(Handler* handler, const string& name, size_t handler_context, const string& group, int port, const string& iface, const string& source, bool bind_only)
  {
    mutex_t::scoped_lock lock(m_mutex);

    printf("%s: %s handler %s %s:%d %s %s\n", __func__, m_name.c_str(), handler->name().c_str(), group.c_str(),
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
	      string where(__func__);
        throw errno_exception(where + " " + m_name + " ioctl(SIOCGIFADDR) failed ("+ iface + ")- likely invalid interface ", errno);
      }
      iface_addr = ((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr;
      close(fd);
    }

    m_contexts.push_back(Context(handler, name, handler_context, true, true, source_addr, group_addr, iface_addr, port));
  }

  void
  Simple_Dispatcher::udp_bind(Handler* handler, const string& name, size_t handler_context, const string& host, int port)
  {
    mutex_t::scoped_lock lock(m_mutex);

    printf("%s: %s handler %s %d %s\n", __func__, m_name.c_str(), handler->name().c_str(), port, host.c_str());

    uint32_t group_addr = 0;
    uint32_t source_addr = 0;
    if(!host.empty()) {
      source_addr = inet_addr(host.c_str());
    }
    uint32_t iface_addr = 0;

    m_contexts.push_back(Context(handler, name, handler_context, true, false, source_addr, group_addr, iface_addr, port));
  }
  
  void
  Simple_Dispatcher::add_udp_fd(Handler* handler, const string& name, size_t context, int fd)
  {
    //printf("Simple_Dispatcher::add_udp_fd %s Handler %s %s fd=%d", m_name.c_str(), handler->name().c_str(), name.c_str(), fd);
    printf("Simple_Dispatcher::add_udp_fd %s Handler %s %s fd=%d\n", m_name.c_str(), handler->name().c_str(), name.c_str(), fd);
    m_contexts.push_back(Context(handler, name, context, true, false, 0, 0, 0, 0));
    m_contexts.back().fd = fd;
    m_contexts.back().subscribed = false;
    m_contexts.back().externally_subscribed = true;
  }
  
  void
  Simple_Dispatcher::add_tcp_fd(Handler* handler, const string& name, size_t context, int fd, size_t buf_size)
  {
    printf("Simple_Dispatcher::add_tcp_fd %s Handler %s %s fd=%d\n", m_name.c_str(), handler->name().c_str(), name.c_str(), fd);
    m_contexts.push_back(Context(handler, name, context, false, false, 0, 0, 0, 0));
    m_contexts.back().fd = fd;
    m_contexts.back().subscribed = false;
    m_contexts.back().externally_subscribed = true;
    
    TCP_Buffer* buf = m_fd_to_tcp_buffer[fd] = new TCP_Buffer();
    buf->init(buf_size);
  }
  
  void Simple_Dispatcher::set_udp_reader(int fd, UDP_READER reader){
        size_t context_idx = -1;
        if(likely((size_t)fd < m_fd_to_context.size())) {
          context_idx = m_fd_to_context[fd];
        }
	if(likely(context_idx != (size_t)-1)) {
          Context& context = m_contexts[context_idx];
	  context.fd_reader=reader;
	  cout << __func__ << " changed\n";
	}
  }

  void
  Simple_Dispatcher::start(Handler* h, const string& name)
  {
    //mutex_t::scoped_lock lock(m_mutex);
    //start_sync(h, name);
  }
  
  void
  Simple_Dispatcher::start_sync(Handler* h, const string& name)
  {    
    cout << __func__ << " check " << name << endl;
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
		    string where=__func__;
              throw runtime_error(where + " " + m_name + " socket() failed");
            }
            
            printf("%s: %s creating %s socket %d for handler %s port %d\n", __func__, m_name.c_str(), context.mcast ? "mcast" : "udp", context.fd,
                                 context.handler->name().c_str(), context.port);
            
            int flag_on = 1;
            /* set reuse port to on to allow multiple binds per host */
            if ((setsockopt(context.fd, SOL_SOCKET, SO_REUSEADDR, &flag_on, sizeof(flag_on))) < 0) {
            printf("%s: %s setsockopt %s socket %d for handler %s port %d (%s)\n", __func__, m_name.c_str(), context.mcast ? "mcast" : "udp", context.fd,
                               context.handler->name().c_str(), context.port,
			       strerror(errno));
	    string where=__func__;
              throw errno_exception(where + " " + m_name + " socket() failed", errno);
            }
            
            /* Give us a nice big buffer */
            if ((setsockopt(context.fd, SOL_SOCKET, SO_RCVBUF, &m_recv_buffer_size, sizeof(m_recv_buffer_size))) < 0) {
	    string where=__func__;
              throw errno_exception(where + " " + m_name + " setsockopt(SO_RCVBUF) failed", errno);
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
	      snprintf(buf, 128, "%s %s bind() failed for port %d ", __func__, m_name.c_str(), context.port);
              throw errno_exception(buf, errno);
            }
            
            if(context.mcast) {
              struct ip_mreq mc_req;
              mc_req.imr_multiaddr.s_addr = context.group_addr;
              mc_req.imr_interface.s_addr = context.iface_addr;
              
              /* send an ADD MEMBERSHIP message via setsockopt */
              if ((setsockopt(context.fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mc_req, sizeof(mc_req))) < 0) {
		      string where=__func__;
                throw errno_exception(where + " " + m_name + " setsockopt(IP_ADD_MEMBERSHIP) failed for " + h->name() + " " + name + " ", errno);
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
  Simple_Dispatcher::stop(Handler* h, const string& name)
  {
    mutex_t::scoped_lock lock(m_mutex);
    stop_sync(h, name);
  }
  
  void
  Simple_Dispatcher::stop_sync(Handler* h, const string& name)
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
  Simple_Dispatcher::status(ILogger& logger) const
  {
    logger.printf("Dispatcher: %s\n", m_name.c_str());
    
    for(vector<Context>::const_iterator i = m_contexts.begin(), i_end = m_contexts.end(); i != i_end; ++i) {
      logger.printf("  handler: %20s %5s %3zd recv_count: %20zd   recv_bytes: %20zd\n",
                    i->handler->name().c_str(), i->name.c_str(), i->handler_context, i->recv_count, i->recv_bytes);
    }
  }
  
  void
  Simple_Dispatcher::full_stop()
  {
    m_started = false;
    if(m_thread.joinable()) {
      m_thread.join();
    }
    
    if(m_app_run_loop) {
      m_app_run_loop->remove_service(this);
      printf("Simple_Dispatcher::full_stop %s Removed reader from run loop %s\n", m_name.c_str(), m_app_run_loop->name().c_str());
    }
    
    clear();
    if(m_ep_fd > 0) {
      close(m_ep_fd);
      m_ep_fd = -1;
    }
  }
  
  Simple_Dispatcher::Simple_Dispatcher() :
    m_started(false),
    m_ep_fd(-1),
    m_timeout(10), // 0.1 second
    m_recv_buffer_size(64 * 1024 * 1024),
    //m_first_error(true),
    m_cpu_bind(-1)
  {
    m_ep_fd = epoll_create(300);
    if(m_ep_fd < 0) {
      throw hf_tools::errno_exception("Disaptcher::Simple_Dispatcher: Error calling epoll_create", errno);
    }
    
    m_fd_to_context.resize(2048, -1);
    m_fd_to_tcp_buffer.resize(2048, 0);
    m_events.reset(new epoll_event[max_events]);
  }
  
  Simple_Dispatcher::~Simple_Dispatcher()
  {
    full_stop();
  }


}
