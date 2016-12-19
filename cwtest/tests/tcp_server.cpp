#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/eventfd.h>

#include <netinet/tcp.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <MD/MDManager.h>
#include <ITCH/functions.h>
#include <ITCH/Snapshot_Handler.h>
#include <ITCH/ASX_Test_Handler.h>
#include <ITCH/Simple_Dispatcher.h>
#include <Data/ASX_ITCH_1_1_Handler.h>
#include <Util/Parameter.h>
#include <Util/Network_Utils.h>
#include <Util/File_Utils.h>

#include <initializer_list>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/trim_all.hpp>

#include <stdarg.h>
#include <stdint.h>
#include <chrono>
#include <thread>
#include <vector>


	using namespace std;
	using namespace hf_core;
	using namespace hf_tools;
	using namespace hf_cwtest;

size_t total_channels;
int* tcp_listen_fds;
string data_file_name;

typedef vector<int> fd_sets;
fd_sets TCP_fds;

int handle_exchange_accept(int fd){

      size_t channel_id=0;
      for (; channel_id < total_channels; channel_id++){
	      if (fd!=tcp_listen_fds[channel_id]) continue;
	      break;
      }
      if (channel_id == total_channels) return -1;
      sockaddr addr;
      socklen_t addr_len = sizeof(addr);

      int new_fd = ::accept(fd, &addr, &addr_len);
      int flag=1;
      //update the channel info for this socket
      if(0 != setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY, (void*) &flag, sizeof(int))) {
            throw errno_exception("socket_fd::set_nodelay setsockopt(2) error: ", errno);
      }
      setnonblocking(new_fd);
      tcp_listen_fds[channel_id]=new_fd;
      cout << "Channel " << channel_id << " Accept with " << new_fd << endl;
      //
      return EPOLLIN;
}

int ePoll_FD=-1;

void local_poll_accept()//epoll_event* events)
{
boost::scoped_array<epoll_event> events(new epoll_event[256]);
      // Lets set the maximum timeout to 1 second
      int timeout = 3*60*1000;
      /*
      if(!m_scheduler.empty()) {
        Time next_timer = m_scheduler.top_time();
        int next_timeout = (next_timer - Time::system_time_no_update()).total_msec() + 1;
        if(next_timeout < timeout) {
          timeout = std::max(0, next_timeout);
        }
      }
      */
      size_t poll_found=0;
      
      while (poll_found < total_channels){
      int nfds = epoll_wait(ePoll_FD, events.get(), 256, timeout);
      if(unlikely(nfds == -1)) {
	      cout << __func__ << " No events!!" << endl;
        return;
      }
      
      if(nfds) {
        for(int n = 0; n < nfds; ++n) {
          //epoll_event* ev = events + n;
          int fd = events[n].data.fd;
            
            int fd_event_flags = 0;
              fd_event_flags = handle_exchange_accept(fd);
	      if (fd_event_flags < 0){
	      cout << "Poll listen got unknow socket:" << fd << endl;
	      continue;
	      }
	      epoll_ctl(ePoll_FD, EPOLL_CTL_DEL, fd, 0);
	      poll_found++;
          }
      }
   }
}
      
bool insert_poll(int (*handle_accept)(int fd), int listen_fd, bool read, bool write, bool edge_triggered){
	if (ePoll_FD==-1){
		ePoll_FD = epoll_create(300);
		if(ePoll_FD < 0) {
	printf("Error calling epoll_create %s\n", strerror(errno));
			                  exit(EXIT_FAILURE);
		}

	}
      struct epoll_event ev;
      ev.events = 0;
      if(read) {
        ev.events |= EPOLLIN;
      }
      if(write) {
        ev.events |= EPOLLOUT;
      }
      if(edge_triggered) {
        ev.events |= EPOLLET;
      }
     /* 
      if(m_start_threads > 1) {
        ev.events |= EPOLLONESHOT;
      }
      */
      ev.data.fd = listen_fd;
     /* 
      if(m_fd_to_object.size() <= (size_t)fd) {
        int new_size = m_fd_to_object.size() * 2;
        while(new_size <= fd) {
          new_size *= 2;
        }
        m_fd_to_object.resize(new_size, 0);
        m_fd_to_epoll_events.resize(new_size, 0);
      }
      
      if(m_fd_to_object[fd]) {
        lock.release();
        m_fd_to_object[fd] = 0;
        lock.acquire(m_mutex);
      }
      */
      
      if(epoll_ctl(ePoll_FD, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
   printf("epoll_ctl returned %d -- %s\n", errno, strerror(errno));
        return false;
      }
     /* 
      m_fd_to_object[fd] = obj;
      m_fd_to_epoll_events[fd] = ev.events;
      */
      
      return true;
}

void connectTcpPorts(Parameter aParam, size_t ttChannel){
	int port_base=stoi(aParam["TCP_port"].getString());
      for (size_t i=0; i<ttChannel; i++) {
	      sockaddr_in snapshot_addr;
	      fill_sockaddr_in(snapshot_addr, "127.0.0.1", port_base+i);
	      socket_fd snapshot_socket;
	      int ret=snapshot_socket.connect_tcp(snapshot_addr);
	      if (ret == 0) {
	      cout << i << " - channel connection failed" << endl;
	         continue;
	      }
	      snapshot_socket.set_nodelay();
	      if(fcntl(snapshot_socket.fd, F_SETFL, O_NONBLOCK) < 0)
	      {
		      cout << i << " - channel set O_NONBLOCK failed\n";
	      }
	      TCP_fds.push_back(snapshot_socket.fd);
	      cout << "Channel " << i << " connected at " << snapshot_socket.fd << endl;
	      snapshot_socket.fd=-1;
	}
}

void openTcpPortListen(Parameter aParam, size_t ttChannel)
{
	int port_base=stoi(aParam["TCP_port"].getString());
      for (size_t i=0; i<ttChannel; i++) {
        int listen_skt = socket(PF_INET, SOCK_STREAM, 0);
        
        int optval = 1;
  setsockopt(listen_skt, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_base+i);
        addr.sin_addr.s_addr = INADDR_ANY;
        
        if(bind(listen_skt, (sockaddr*) &addr, sizeof(addr))) {
 printf(": %u Unable to bind() to port %u\n", i, port_base+i);
          throw errno_exception(__func__, errno);
        }
        
        if(listen(listen_skt, 5)) {
    printf(": %u Unable to listen() to port %u", i, port_base+i);
          throw errno_exception(__func__, errno);
        }
        
        printf("Listening to port %d\n", port_base+i);
        
        setnonblocking(listen_skt);
        
        tcp_listen_fds[i]=listen_skt;
	insert_poll(handle_exchange_accept, listen_skt, true, true, true);
      }

      local_poll_accept();
      for (size_t i=0; i< total_channels; i++){
	      TCP_fds.push_back(tcp_listen_fds[i]);
      }
}


void start_exchange(Parameter& aParam)
{
	size_t ttChannel=5;
#ifdef USE_PIPE
	//to verify that those pipes are open
	//
	char aByte[3]="xx";
	size_t ttChannel=mcast_fds.size();
	for (size_t c=0; c<ttChannel; c++){
		aByte[0]='M'; aByte[1]=49+c;
		::write(mcast_fds[c], &aByte, 2);
	printf("exchange mcast sent %-2s on %d\n", aByte, mcast_fds[c]);
		aByte[0]='U'; 
		::write(udpSend_fds[c], &aByte, 2);
	printf("exchange udp sent %-2s on %d\n", aByte, udpSend_fds[c]);
		aByte[0]='T';
	}
	for (size_t c=0; c<ttChannel; c++){
		::read(udpRead_fds[c], &aByte, 2);
	printf("exchange udp read %-2s on %d\n", aByte, udpRead_fds[c]);
	}
#endif
	Application_Config eConfig;
	Application app(eConfig);
	//ASX_Test_Handler handler(&app);

	//MDManager md(&app);
	//ExchangeID exch(ExchangeID::XASX);//, 10058);//XASX 10058
	Simple_Dispatcher dispatcher;
	boost::shared_ptr<ASX_Test_Handler> ptHdr(new ASX_Test_Handler(&app));
	ASX_Test_Handler* hdr=ptHdr.get();//new ASX_Test_Handler(&app);
	hdr->init("ASX_Test_Handler", aParam);
	//vector<hf_cwtest::snapshot_session::channel_info>& channels=hdr->m_channels;
	string bName="test";
	openTcpPortListen(aParam, ttChannel);
	for (size_t c=0; c<ttChannel; c++){
		const string name=bName+std::to_string(c);
		size_t ch_id=c;
		boost::shared_ptr<hf_cwtest::snapshot_session::channel_info> oneC(new hf_cwtest::snapshot_session::channel_info());
		hdr->m_channels.push_back(oneC);
		hf_cwtest::snapshot_session::channel_info& newC=*(oneC.get());
		newC.username=name;
		newC.channel_id=ch_id;
		//newC.mcast_fd=mcast_fds[c];
		//newC.udp_send_fd=udpSend_fds[c];
		//newC.udp_read_fd=udpRead_fds[c];
		//newC.snapshot_skt.fd=TCP_fds[c];
		//newC.snapshot_skt.set_nodelay();
		//newC.snapshot_skt.set_nonblocking();
		//newC.tcp_send_fd=newC.tcp_read_fd=TCP_fds[c];
		newC.final_message_completed=false;
		//dispatcher.add_udp_fd(hdr, name, ch_id, udpRead_fds[c]);
		//dispatcher.add_tcp_fd(hdr, name, ch_id, TCP_fds[c], 56000);
		newC.snapshot_skt.fd=TCP_fds[c];
		//tcpC.snapshot_skt.set_nodelay();
		//tcpC.snapshot_skt.set_nonblocking();
		newC.tcp_send_fd=newC.tcp_read_fd=TCP_fds[c];
		dispatcher.add_tcp_fd(hdr, name, ch_id, TCP_fds[c], 56000);
	}
	//int tcp_fds[ttChannel];
	cout << "exchange has "<< hdr->m_channels.size()<< " channels" << endl;
	for (size_t c=0; c<ttChannel; c++){
		string name=bName+std::to_string(c);
		dispatcher.start_sync(hdr, name);
	}
	for (size_t c=0; c<ttChannel; c++){
	hdr->enable_test_mode();//set_udp_writer(mcast_fds[c], ::write);
	//hdr->enable_test_mode();//set_udp_writer(udpRead_fds[c], ::write);
	dispatcher.enable_test_mode();//set_udp_reader(udpRead_fds[c], ::read);
	}
cout << "finish loading data to outbound channels" << endl;

	string shm_file_name=aParam["test_shared_mm_name"].getString();
	size_t share_size=aParam.getDefaultInt("test_shared_mm_M", 100);
	share_size *= 1000000;
	//inside the shared memory the struct is
	//byte1(channel_id),byte2(message_len).....
	//use this to load the data for hr->load_test_data(char* mem_addr)
	
	size_t mm_len=0;
	char *pData=0;//=data_page.get_address();
	while (!pData){
using namespace boost::interprocess;
		try {
	boost::interprocess::shared_memory_object test_data (
		boost::interprocess::open_only 
	       ,shm_file_name.c_str()
	       ,boost::interprocess::read_only 
	);
	test_data.truncate(share_size);
	boost::interprocess::mapped_region data_page ( 
		test_data                      //Memory-mappable object 
		, boost::interprocess::read_only
		, 0                //Offset from the beginning of shm 
		, share_size        //Length of the region
	);
	pData=data_page.get_address();
	mm_len=data_page.get_size();
		} catch(...){
			
		}
	}
	hdr->load_test_data(pData, mm_len);
	
	dispatcher.set_external_start();
	
	while (!hdr->is_done()){
		dispatcher.set_external_start();
		dispatcher.poll_one();
		hdr->check_write_event();
	}
	boost::interprocess::shared_memory_object::remove(shm_file_name.c_str());
	for (size_t c=0; c<ttChannel; c++){
		string name=bName+std::to_string(c);
		dispatcher.stop_sync(hdr,name ); 
	}

	delete hdr;

}

int
main(int argc, char *argv[])
{
	Parameter aParam;
	char cfgbuf[160];
	if (argc != 2) {
		fprintf(stderr, "Usage: %s config_file_name\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	string config=argv[1];
	FILE* cfg=0;
	cfg=fopen(config.c_str(), "r");
	if (!cfg){
		fprintf(stderr, "%s config_file_name unknown\n", argv[0]);
		                  exit(EXIT_FAILURE);
	}
	const boost::char_separator<char> dm=boost::char_separator<char>(":");
	hash_map<string, Parameter> h_pm;
	while(fgets(cfgbuf, 120, cfg)){
		char *p_=strchr(cfgbuf, '#');
		if (p_) *p_=0;
		if (strlen(cfgbuf) < 2) continue;
	  //string line(cfgbuf);
	  p_=strchr(cfgbuf, ':');
	  if (!p_) continue;
	  *(p_++)=0;

	  Parameter saveData;
	  
	  saveData.setString(trim(p_));
	  
	  h_pm[trim(cfgbuf)]=saveData;
	}
	if (h_pm.find("session_name")==h_pm.end()){
		Parameter saveData;
		saveData.setString(string("CW-TEST-01"));
		h_pm[string("session_name")]=saveData;
	}
	aParam.setDict(h_pm);
	cout << aParam << endl;	
	if (
			!aParam.has("TCP_port") ||
			!aParam.has("test_shared_mm_name") ||
			!aParam.has("total_channels") ||
			!aParam.has("username") ||
			!aParam.has("password")){
		fprintf(stderr, "%s missing keyword test_shared_mm_name or user_name or password \n", argv[1]);
		                  exit(EXIT_FAILURE);
	}
	total_channels=stoi(aParam["total_channels"].getString());
	int listen_fds[total_channels];
	tcp_listen_fds=listen_fds;

		start_exchange(aParam);
}

#ifdef USE_PIPE
#endif

