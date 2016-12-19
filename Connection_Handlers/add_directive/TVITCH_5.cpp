#include <QBT/TVITCH_5.h>
#include <Util/Network_Utils.h>

#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>
#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>
#include <Logger/Logger_Manager.h>

#include <stdarg.h>

namespace hf_md {

  void
  TVITCH_5::start()
  {
    if(!is_started) {
      MDProvider::start();
      m_itch_parser->start();
      for(vector<string>::const_iterator i = m_lines.begin(), i_end = m_lines.end(); i != i_end; ++i) {
        m_dispatcher->start(m_itch_parser.get(), *i);
      }
      if(m_recovery_dispatcher) {
        m_recovery_dispatcher->start(m_itch_parser.get(), "");
      }
    }
  }
  
  void
  TVITCH_5::stop()
  {
    if(m_recovery_dispatcher) {
      m_recovery_dispatcher->stop(m_itch_parser.get(), "");
    }
    for(vector<string>::const_iterator i = m_lines.begin(), i_end = m_lines.end(); i != i_end; ++i) {
      m_dispatcher->stop(m_itch_parser.get(), *i);
    }
    m_itch_parser->stop();
    MDProvider::stop();
  }

  void
  TVITCH_5::init(const string& name,const Parameter& params)
  {
    const char* where = "TVITCH_5::init";

    MDProvider::init(name, params);

    if(m_supported_exchanges.size() != 1) {
      throw runtime_error("TVITCH Parser supports exactly 1 exchange per Provider");
    }
    ExchangeID exch = m_supported_exchanges[0].exch;
    
    m_itch_parser.reset(new TVITCH_5_Handler(&m_md.app()));
    m_itch_parser->set_subscribers(m_md.subscribers_rp());
    m_itch_parser->set_provider(this);
    m_itch_parser->init(name, exch, params);
    
    m_order_books = m_itch_parser->order_books();
    
    if(params.has("multicast_group") || params.has("itch")) {
      m_dispatcher = m_md.get_dispatcher(params.getDefaultString("dispatcher", name));
      if(params.has("recovery_dispatcher")) {
        m_recovery_dispatcher = m_md.get_dispatcher(params.getDefaultString("recovery_dispatcher", name));
      }
      if(!m_dispatcher) {
        throw runtime_error("TVITCH_5::init:" + m_name + " dispatcher not found");
      }
      
      if(params.has("itch")) {
        const vector<Parameter>& lines = params["itch"].getVector();
        for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
          string line = (*l)["line"].getString();
          string source = (*l)["source_address"].getString();
          string group = (*l)["multicast_group"].getString();
          string iface = (*l)["multicast_iface"].getString();
          int port = (*l)["multicast_port"].getInt();
          m_dispatcher->mcast_join(m_itch_parser.get(), line, 1, group, port, iface, source);
          m_lines.push_back(line);
        }
      } else {
        string line = "A";
        string source = params["source_address"].getString();
        string group = params["multicast_group"].getString();
        string iface = params["multicast_iface"].getString();
        int port = params["multicast_port"].getInt();
        m_dispatcher->mcast_join(m_itch_parser.get(), line, 1, group, port, iface, source);
        m_lines.push_back(line);
      }
      
      if(m_recovery_dispatcher && params.has("recovery_host")) {
        string recovery_host = params["recovery_host"].getString();
        int recovery_port = params["recovery_port"].getInt();

        fill_sockaddr_in(m_recovery_addr, recovery_host, recovery_port);

        if(0 != m_itch_parser->m_channel_info.recovery_socket.connect_udp(m_recovery_addr)) {
          m_itch_parser->m_channel_info.has_recovery = true;
          m_recovery_dispatcher->add_udp_fd(m_itch_parser.get(), "", 1, m_itch_parser->m_channel_info.recovery_socket);
        }
        
        m_itch_parser->m_channel_info.init(params);
      }
    }
  }

  void
  TVITCH_5::subscribe_product(const ProductID& id_,const ExchangeID& exch_,
                                const string& mdSymbol_,const string& mdExch_)
  {
    if(m_itch_parser) {
      m_itch_parser->subscribe_product(id_, exch_, mdSymbol_, mdExch_);
    }
  }
  
  bool
  TVITCH_5::drop_packets(int num_packets)
  {
    if(m_itch_parser) {
      m_itch_parser->drop_packets(num_packets);
      return true;
    }
    return false;
  }
  
  TVITCH_5::TVITCH_5(MDManager& md) :
    MDProvider(md),
    m_dispatcher(0),
    m_recovery_dispatcher(0)
  {
  }

  TVITCH_5::~TVITCH_5()
  {
    stop();
  }


}
