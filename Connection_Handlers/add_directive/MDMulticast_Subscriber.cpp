#include <QBT/MDMulticast_Subscriber.h>

#include <Event_Hub/Event_Hub.h>
#include <Product/Product_Manager.h>

#include <MD/MDOrderBook.h>
#include <MD/MDManager.h>

#include <Logger/Logger_Manager.h>
#include <Util/Network_Utils.h>

#include <stdarg.h>
#include <netinet/tcp.h>

namespace hf_md {

  bool
  MDMulticast_Subscriber::init_mcast(const string& line_name, const Parameter& params)
  {
    //const char* where = "MDMulticast_Subscriber::init_mcast";
    
    if(!params.has(line_name)) {
      return false;
    }

    Handler::channel_info_map_t& cim = m_handler->m_channel_info_map;

    const vector<Parameter>& lines = params[line_name].getVector();
    for(vector<Parameter>::const_iterator l = lines.begin(); l != lines.end(); ++l) {
      const string& name = (*l)["name"].getString();
      //const string& p_source = (*l)["primary_source"].getString();
      
      size_t context = cim.size();
      cim.push_back(channel_info(name, context));
      channel_info& ci = cim.back();
      ci.init(params);
      
      if(l->has("A_group")) {
        const string& A_group = (*l)["A_group"].getString();
        int A_port = (*l)["A_port"].getInt();
        const string& A_iface = (*l)["A_iface"].getString();
        m_disp->mcast_join(m_handler.get(), "A", context, A_group, A_port, A_iface, "");
      }
      
      if(l->has("B_group")) {
        const string& B_group = (*l)["B_group"].getString();
        int B_port = (*l)["B_port"].getInt();
        const string& B_iface = (*l)["B_iface"].getString();
        m_disp->mcast_join(m_handler.get(), "B", context, B_group, B_port, B_iface, "");
      }
    }
    
    m_logger->log_printf(Log::INFO, "%s %s: Lines initialized", "MDMulticast_Subscriber", m_name.c_str());

    return true;
  }

  void
  MDMulticast_Subscriber::init(const string& name, const Parameter& params)
  {
    MDProvider::init(name, params);
    if(m_supported_exchanges.size() < 1) {
      throw runtime_error("MDMulticast_Subscriber " + name + " must have at least 1 exchange defined");
    }
    
    m_disp = m_md.get_dispatcher(params.getDefaultString("dispatcher", name));
    if(params.has("recovery_dispatcher")) {
      m_recov_disp = m_md.get_dispatcher(params["recovery_dispatcher"].getString());
    }
    
    m_handler.reset(ObjectFactory1<std::string, Handler, Application*>::allocate(params["handler"].getString(), &m_md.app()));
    
    vector<ExchangeID> exchanges;
    for(vector<Supported_Exchange>::const_iterator se = m_supported_exchanges.begin(); se != m_supported_exchanges.end(); ++se) {
      exchanges.push_back(se->exch);
    }
    
    m_handler->set_subscribers(m_md.subscribers_rp());
    m_handler->set_provider(this);
    m_handler->init(name, exchanges, params);
    m_order_books = m_handler->order_books();
   
    string mcast_lines_name;
    if(params.has("mcast_lines_name")) {
      mcast_lines_name = params["mcast_lines_name"].getString();
    }
    else {
      mcast_lines_name = "mcast_lines";
    }
 
    init_mcast(mcast_lines_name, params);
  }
  
  
  void
  MDMulticast_Subscriber::start(void)
  {
    if(!is_started) {
      MDProvider::start();
      m_handler->start();
      if(m_disp) {
        m_disp->start(m_handler.get(), "A");
        m_disp->start(m_handler.get(), "B");
      }
    }
  }

  void
  MDMulticast_Subscriber::stop(void)
  {
    if(is_started) {
      if(m_disp) {
        m_disp->stop(m_handler.get(), "B");
        m_disp->stop(m_handler.get(), "A");
      }
    }
    m_handler->stop();
    MDProvider::stop();
  }
  
  void
  MDMulticast_Subscriber::subscribe_product(const ProductID& id, const ExchangeID& exch,
                                            const string& mdSymbol, const string& mdExch)
  {
    m_handler->subscribe_product(id, exch, mdSymbol, mdExch);
  }
  
  bool
  MDMulticast_Subscriber::drop_packets(int num_packets)
  {
    if(m_handler) {
      m_handler->drop_packets(num_packets);
      return true;
    }
    return false;
  }
  
  MDMulticast_Subscriber::MDMulticast_Subscriber(MDManager& md) :
    MDProvider(md),
    m_disp(0),
    m_recov_disp(0)
  {

  }

  MDMulticast_Subscriber::~MDMulticast_Subscriber()
  {
    stop();
  }



}
