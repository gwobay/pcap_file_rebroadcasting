#include <QBT/DBL_Dispatcher.h>

#include <app/app.h>
#include <app/App_Run_Loop.h>

#include <Util/except.h>
#include <Util/Network_Utils.h>

#include <algorithm>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>

namespace hf_md {
  using namespace std;
  using namespace hf_tools;

  void
  DBL_Dispatcher::init(const string& name, MDManager* md, const Parameter& params)
  {
    MDDispatcher::init(name, md, params);
    
    if(params.has("recv_mode")) {
      const string& recv_mode = params["recv_mode"].getString();
      if(recv_mode == "blocking") {
        m_recvmode = DBL_RECV_BLOCK;
      } else if(recv_mode == "non-blocking") {
        m_recvmode = DBL_RECV_NONBLOCK;
      } else {
        throw runtime_error(m_name + " unknown recv_mode '" + recv_mode + "'.  Choices are 'blocking' and 'non-blocking'");
      }
    }
    
    m_cpu_bind = params.getDefaultInt("cpu_bind",m_cpu_bind);
  }

  void
  DBL_Dispatcher::run()
  {
    m_started = true;
    
    if(!m_app_run_loop) {
      m_thread = boost::thread(&DBL_Dispatcher::run_loop, this);
    } else {
      m_app_run_loop->add_service(this);
      m_logger->log_printf(Log::INFO, "DBL_Dispatcher::run %s Added reader to run loop %s", m_name.c_str(), m_app_run_loop->name().c_str());
    }
  }

  void
  DBL_Dispatcher::handle_request(const request_t& req)
  {
    const char* where = "DBL_Dispatcher::handle_request";
    
    switch(req.command.index()) {
    case request_t::Command::start:
      {
        int bind_flags = DBL_BIND_REUSEADDR;
        //if(m_dbl_dup_to_kernel) {
        //  bind_flags |= DBL_BIND_DUP_TO_KERNEL;
        //}
        for(vector<Context>::iterator c_i = m_contexts.begin(), c_end = m_contexts.end(); c_i != c_end; ++c_i) {
          if(c_i->handler == req.handler && c_i->name == req.name && !c_i->bound) {
            Context& c = *c_i;
            m_logger->log_printf(Log::INFO, "%s: %s creating %s socket for handler %s port %d", where, m_name.c_str(),
                                 "mcast",
                                 c.handler->name().c_str(), c.port);
            
            size_t context_idx = c_i - m_contexts.begin();
            int ret = dbl_bind(c.dev, bind_flags, c.port, (void*) context_idx, &c.chan);
            if(ret) {
              char err_buf[128];
              char* err_msg = strerror_r(errno, err_buf, 512);
              send_alert("%s: %s %s dbl_bind() failed with %d - %s", where, m_name.c_str(), req.handler->name().c_str(), ret, err_msg);
            }
            c.bound = true;
            
            if(c.subscribe_on_start) {
              ret = dbl_mcast_join(c.chan, &c.mcast_addr, 0);
              c.subscribed = true;
              if(ret) {
                char err_buf[128];
                char* err_msg = strerror_r(errno, err_buf, 512);
                send_alert("%s: %s %s dbl_mcast_join() failed with %d - %s", where, m_name.c_str(), req.handler->name().c_str(), ret, err_msg);
              }
            }
          }
        }
      }
      break;
    case request_t::Command::stop:
      for(vector<Context>::iterator c_i = m_contexts.begin(), c_end = m_contexts.end(); c_i != c_end; ++c_i) {
        if(c_i->handler == req.handler && c_i->name == req.name && c_i->bound) {
          dbl_unbind(c_i->chan);
          c_i->bound = false;
          c_i->subscribed = false;
        }
      }
      break;
    case request_t::Command::join:
      for(vector<Context>::iterator c_i = m_contexts.begin(), c_end = m_contexts.end(); c_i != c_end; ++c_i) {
        if(c_i->handler == req.handler && c_i->name == req.name) {
          Context& c = *c_i;
          m_logger->log_printf(Log::INFO, "%s: %s join group for handler %s port %d", where, m_name.c_str(),
                               c.handler->name().c_str(), c.port);
          int ret = dbl_mcast_join(c.chan, &c.mcast_addr, 0);
          c.subscribed = true;
          if(ret) {
            char err_buf[128];
            char* err_msg = strerror_r(errno, err_buf, 512);
            send_alert("%s: %s %s dbl_mcast_join() failed with %d - %s", where, m_name.c_str(), req.handler->name().c_str(), ret, err_msg);
          }
        }
      }
      break;
    case request_t::Command::leave:
      for(vector<Context>::iterator c_i = m_contexts.begin(), c_end = m_contexts.end(); c_i != c_end; ++c_i) {
        if(c_i->handler == req.handler && c_i->name == req.name) {
          Context& c = *c_i;
          m_logger->log_printf(Log::INFO, "%s: %s leave group for handler %s port %d", where, m_name.c_str(),
                               c.handler->name().c_str(), c.port);
          int ret = dbl_mcast_leave(c.chan, &c.mcast_addr);
          c.subscribed = true;
          if(ret) {
            char err_buf[128];
            char* err_msg = strerror_r(errno, err_buf, 512);
            send_alert("%s: %s %s dbl_mcast_leave() failed with %d - %s", where, m_name.c_str(), req.handler->name().c_str(), ret, err_msg);
          }
        }
      }
      break;
    }
  }

  void
  DBL_Dispatcher::run_loop()
  {
    m_logger->log_printf(Log::INFO, "DBL_Dispatcher::run_loop %s Starting reader thread", m_name.c_str());
    
    if(m_md) {
      m_md->app().set_thread_name(syscall(SYS_gettid), m_name.c_str());
    }
    
    if (m_cpu_bind>0) {
      cpu_set_t mask; 
      CPU_ZERO(&mask); 
      CPU_SET(m_cpu_bind, &mask); 
      int ret = sched_setaffinity(0, sizeof mask, &mask); 
      if (ret==0) {
	m_logger->log_printf(Log::INFO, "DBL_Dispatcher::run_loop %s, CPU bind success %d", m_name.c_str(), m_cpu_bind);
      } else {
	m_logger->log_printf(Log::ERROR, "DBL_Dispatcher::run_loop %s, CPU bind failed %d, error [%s]",m_name.c_str(),m_cpu_bind,::strerror(errno));
      }
    }
    
    if(m_recvmode == DBL_RECV_BLOCK && m_dbl_devices.size() > 1) {
      throw runtime_error("DBL_RECV_BLOCK mode only works for 1 device.  " + m_name + " is configured with multiple devices (vlans)");
    }
    
    while(m_started) {
      // Pretty sure we're not going to get overridden here
      DBL_Dispatcher::poll_one();
    }
    
    m_logger->log_printf(Log::INFO, "DBL_Dispatcher::run_loop %s Finished reader thread", m_name.c_str());
  }
  
  size_t
  DBL_Dispatcher::poll_one()
  {
    if(!m_started) return 0;
    if(!m_requests.empty()) {
      request_t req;
      while(m_requests.try_pop(req)) {
        handle_request(req);
      }
    }
    
    dbl_recv_info recv_info;
    
    size_t processed = 0;
    
    for(vector<dbl_device_t>::iterator d = m_dbl_devices.begin(), d_end = m_dbl_devices.end(); d != d_end; ++d) {
      for(int count = 64; count > 0; --count) {  // More efficient to grab from a particular device as a streak
        int ret = dbl_recvfrom(*d, m_recvmode, m_buffer, m_buffer_len, &recv_info);
        if(ret == EAGAIN) {
          break;
        } else if(ret == 0) {
          size_t msgrecv_len = recv_info.msg_len;
          size_t c = (size_t) recv_info.chan_context;
          Context& context = m_contexts[c];
          if(!context.source_ip || context.source_ip == recv_info.sin_from.sin_addr.s_addr) {
            context.handler->parse(context.handler_context, m_buffer, msgrecv_len);
            context.recv_count += 1;
            context.recv_bytes += msgrecv_len;
          } else {
            char ip_addr[32];
            inet_ntop(AF_INET, &recv_info.sin_from.sin_addr, ip_addr, 32);
            send_alert("DBL_Dispatcher::poll_one %s received message from unknown source IP '%s'", m_name.c_str(), ip_addr);
          }
          ++processed;
        } else if(m_first_error) {
          m_logger->log_printf(Log::ERROR, "DBL_Dispatcher::poll_one %s dbl_recvfrom returned %d, only logging once", m_name.c_str(), ret);
          m_first_error = false;
        }
      }
    }
    
    return processed;
  }
  
  
  void
  DBL_Dispatcher::mcast_join(Handler* handler, const string& name, size_t handler_context, const string& group, int port, const string& iface, const string& source, bool bind_only)
  {
    const char* where = "DBL_Dispatcher::mcast_join: ";

    m_logger->log_printf(Log::INFO, "%s: %s handler %s %s %s:%d %s %s", where, m_name.c_str(), handler->name().c_str(), name.c_str(), group.c_str(),
                         port, iface.c_str(), source.c_str());

    dbl_device_t dbl_dev;
    
    string map_name = handler->name() + "_" + iface;
    
    hash_map<string, dbl_device_t>::iterator dbl_i = m_iface_to_dbl_device.find(iface);
    if(dbl_i == m_iface_to_dbl_device.end()) {
      int ret = dbl_open_if(iface.c_str(), 0, &dbl_dev);
      if(ret) {
        throw errno_exception("Dispatcher dbl_open_if() failed", ret);
      }
      m_iface_to_dbl_device.insert(make_pair(iface, dbl_dev));
      m_dbl_devices.push_back(dbl_dev);
    } else {
      dbl_dev = dbl_i->second;
    }

    in_addr mcast_addr;
    mcast_addr.s_addr = inet_addr(group.c_str());
    
    Context ctx(handler, name, handler_context, true, 0, dbl_dev, mcast_addr, port);
    
    if(bind_only) {
      ctx.subscribe_on_start = false;
    }
    
    m_contexts.push_back(ctx);
  }

  void
  DBL_Dispatcher::udp_bind(Handler* handler, const string& name, size_t handler_context, const string& host, int port)
  {
    m_logger->log_printf(Log::FATAL, "DBL_Dispatchert::udp_bind %s Handler %s called add_fd which is not supported", m_name.c_str(), handler->name().c_str());
    throw runtime_error("DBL_Dispatcher::udp_bind not supported");
  }

  void
  DBL_Dispatcher::add_udp_fd(Handler* handler, const string& name, size_t context, int fd)
  {
    m_logger->log_printf(Log::FATAL, "DBL_Dispatchert::add_fd %s Handler %s called add_udp_fd which is not supported", m_name.c_str(), handler->name().c_str());
    throw runtime_error("DBL_Dispatcher::add_fd not supported");
  }
  
  void
  DBL_Dispatcher::add_tcp_fd(Handler* handler, const string& name, size_t context, int fd, size_t)
  {
    m_logger->log_printf(Log::FATAL, "DBL_Dispatchert::add_fd %s Handler %s called add_tcp_fd which is not supported", m_name.c_str(), handler->name().c_str());
    throw runtime_error("DBL_Dispatcher::add_fd not supported");
  }
  
  void
  DBL_Dispatcher::start(Handler* h, const string& name)
  {
    m_requests.push(request_t(h, name, request_t::Command::start));
  }

  void
  DBL_Dispatcher::stop(Handler* h, const string& name)
  {
    m_requests.push(request_t(h, name, request_t::Command::stop));
  }
  
  void
  DBL_Dispatcher::start_sync(Handler* h, const string& name)
  {
    handle_request(request_t(h, name, request_t::Command::start));
  }

  void
  DBL_Dispatcher::stop_sync(Handler* h, const string& name)
  {
    handle_request(request_t(h, name, request_t::Command::stop));
  }
  
  void
  DBL_Dispatcher::join_sync(Handler* h, const string& name)
  {
    handle_request(request_t(h, name, request_t::Command::join));
  }

  void
  DBL_Dispatcher::leave_sync(Handler* h, const string& name)
  {
    handle_request(request_t(h, name, request_t::Command::leave));
  }
  
  void
  DBL_Dispatcher::status(ILogger& logger) const
  {
    logger.printf("Dispatcher: %s\n", m_name.c_str());
    
    for(vector<Context>::const_iterator i = m_contexts.begin(), i_end = m_contexts.end(); i != i_end; ++i) {
      logger.printf("  handler: %20s %5s %3zd subscribed:%d   recv_count: %20zd   recv_bytes: %20zd\n",
                    i->handler->name().c_str(), i->name.c_str(), i->handler_context, i->subscribed, i->recv_count, i->recv_bytes);
    }
  }
  
  void
  DBL_Dispatcher::full_stop()
  {
    m_started = false;
    if(m_thread.joinable()) {
      m_thread.join();
    }
    
    if(m_app_run_loop) {
      m_app_run_loop->remove_service(this);
      m_logger->log_printf(Log::INFO, "DBL_Dispatcher::full_stop %s Removed reader from run loop %s", m_name.c_str(), m_app_run_loop->name().c_str());
    }
    
    // At this point we should clean up and destroy all channels & devs
    for(vector<dbl_device_t>::iterator i = m_dbl_devices.begin(); i != m_dbl_devices.end(); ++i) {
      dbl_close(*i);
    }
    
    m_dbl_devices.clear();
    m_contexts.clear();
    m_iface_to_dbl_device.clear();
  }
  
  DBL_Dispatcher::DBL_Dispatcher() :
    m_dbl_dup_to_kernel(false),
    m_first_error(true),
    m_started(false),
    m_recvmode(DBL_RECV_NONBLOCK),
    m_cpu_bind(-1)
  {
    int ret = dbl_init(DBL_VERSION_API);
    if(ret) {
      throw errno_exception("DBL_Dispatcher dbl_init() failed", ret);
    }
  }
  
  DBL_Dispatcher::~DBL_Dispatcher()
  {
    full_stop();
  }


}
