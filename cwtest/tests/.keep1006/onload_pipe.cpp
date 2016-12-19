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
#include <QBT/OpenOnload_Dispatcher.h>
//#include <ITCH/TestOnLoad_Dispatcher.h>
#include <Util/Parameter.h>
#include <Util/Network_Utils.h>
#include <Util/File_Utils.h>

#include <initializer_list>

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

#include <onload/extensions.h>
#include <onload/extensions_zc.h>

	using namespace std;
	using namespace hf_core;
	using namespace hf_tools;
	using namespace hf_cwtest;
	using namespace hf_md;

static char* turn_on;
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
 printf(": %d Unable to bind() to port %u\n", i, port_base+i);
          throw errno_exception(__func__, errno);
        }
        
        if(listen(listen_skt, 5)) {
    printf(": %d Unable to listen() to port %u", i, port_base+i);
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

bool checkIfClientLate(Parameter& aParam){
	return aParam.getDefaultBool("client_late", false);
}
void start_client(Parameter& aParam,  fd_sets mcast_fds, 
		  fd_sets udpRead_fds, fd_sets udpSend_fds)
		  //fd_sets tcpRead_fds, fd_sets tcpSend_fds)
{
	char aByte[3]="xx";
	size_t ttChannel=mcast_fds.size();
	for (size_t c=0; c<ttChannel; c++){
		aByte[0]='U'; aByte[1]=49+c;
		::write(udpSend_fds[c], &aByte, 2);
	printf("Client udp sent %-2s on %d\n", aByte, udpSend_fds[c]);
		aByte[0]='T';
		//::write(tcpSend_fds[c], &aByte, 2);
	//cout << "Client tcp sent " << aByte << " on "<<tcpSend_fds[c] <<endl;
	}
	for (size_t c=0; c<ttChannel; c++){
		::read(mcast_fds[c], &aByte, 2);
	printf("Client mcast read %-2s on %d\n", aByte, mcast_fds[c]);
		::read(udpRead_fds[c], &aByte, 2);
	printf("Client udp read %-2s on %d\n", aByte, udpRead_fds[c]);
		//::read(tcpRead_fds[c], &aByte, 2);
	//cout << "Client tcp read " << aByte << " on "<<tcpRead_fds[c] <<endl;
	}

	bool I_am_late=checkIfClientLate(aParam);
	if (I_am_late){
		//wait for the G signal
		while (*turn_on!='G')
		{
		cout << "child " << ::getpid() << " flag:" << *turn_on;
cout << " wait for sec to start" << endl;
std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
	Application_Config eConfig;
	Application app(eConfig);
	MDManager md(&app);
	md.init(aParam);
	ExchangeID exch(ExchangeID::XASX);//, 10058);//XASX 10058
	//LLIO::EPoll_Dispatcher aDummy("ASX_TEST", aParam);
	//Simple_Dispatcher dispatcher;
	
	//TestOnLoad_Dispatcher dispatcher;
	OpenOnload_Dispatcher dispatcher;
	dispatcher.init("OpenDisp", &md, aParam);
	ASX_ITCH_1_1_Handler* hdr=new ASX_ITCH_1_1_Handler(&app);
	//Logger_Manager lm(&dispatcher);
	char* user=getenv("USER");
	hdr->init("ASX_ITCH_1_1_Handler", exch, aParam);
	Handler::channel_info_map_t& channels=hdr->m_channel_info_map;
	string bName="test";
	//size_t ttChannel=mcast_fds.size();
	for (size_t c=0; c<ttChannel; c++){
		const string name=bName+std::to_string(c);
		size_t ch_id=c;
		channels.push_back(channel_info(name, ch_id));
		hf_core::channel_info& newC=channels.back();
		newC.init(aParam);
		//--- borrow the field last_msg_ms temporarily
		newC.last_timestamp_ms=udpSend_fds[c];
		newC.last_msg_ms=udpRead_fds[c];
		//newC.snapshot_socket.fd=TCP_fds[c];
		//newC.snapshot_socket.set_nodelay();
		//newC.snapshot_socket.set_nonblocking();
		newC.final_message_received=false;
		dispatcher.add_udp_fd(hdr, name, ch_id, mcast_fds[c]);
		dispatcher.add_udp_fd(hdr, name, ch_id, udpRead_fds[c]);
		dispatcher.add_udp_fd(hdr, name, ch_id, udpSend_fds[c]);
		//dispatcher.add_tcp_fd(hdr, name, ch_id, TCP_fds[c], 56000);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	connectTcpPorts(aParam, ttChannel);
	for (size_t c=0; c<ttChannel; c++){
		const string name=bName+std::to_string(c);
		size_t ch_id=c;
		hf_core::channel_info& newC=channels[c];
		newC.snapshot_socket.fd=TCP_fds[c];
		dispatcher.add_tcp_fd(hdr, name, ch_id, TCP_fds[c], 56000);
	}
	for (size_t c=0; c<ttChannel; c++){
		string name=bName+std::to_string(c);
		dispatcher.start_sync(hdr, name);
	}
	for (size_t c=0; c<ttChannel; c++){
		//dispatcher.enable_test_mode();
		//hdr->enable_test_mode();
		//dispatcher.set_udp_reader(mcast_fds[c], ::read);
		//dispatcher.set_udp_reader(udpRead_fds[c], ::read);
		//hdr->set_udp_writer(mcast_fds[c], ::write);
		//hdr->set_udp_writer(udpRead_fds[c], ::write);
	}
	//run here to simulate the job before the exchange start
	//hdr->update_channel_state();
	size_t child_delay=2;
	if (aParam.has("Child_Delay_Second"))
		child_delay=stoi(aParam["Child_Delay_Second"].getString());
	/*
	while (*turn_on=='N')
	{
		cout << "process " << ::getpid() << " flag:" << *turn_on;
cout << " wait for " << child_delay <<" secs to load the data" << endl;
std::this_thread::sleep_for(std::chrono::milliseconds(child_delay*1000));
	}
	*/
	cout << "process " << ::getpid() << " flag:" << *turn_on << endl;

	hdr->update_channel_state();
	//for debug
	for (size_t c=0; c<ttChannel; c++){
	//now send log on to Exchange --- parent fork
		//handler->send_snapshot_login(c);
	cout << hdr->name() << "Sent login channel:" << c <<endl;
		//hdr->send_snapshot_request(c);
	}
	if (!I_am_late){
std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//tell server to Go
	*turn_on='G';
	}
	
	// run here to simulate the program start after the exchange opened
	//need to add handler status
	//dispatcher.set_external_start();
	//so that the outer loop can check the status to stop
	//and remove stop the dispatcher (it will be run as thread with run_loop function)
	//need to set the udp reader
	//need to add_udp_fd
	// for readable, writable, .....
	//need to add_tcp_fd
	if (I_am_late){
	munmap(turn_on, sizeof *turn_on); 
	}
	struct onload_zc_recv_args aZC_arg;
	memset(&aZC_arg, 0, sizeof(aZC_arg));
    aZC_arg.cb = OpenOnload_Dispatcher::onload_callback;
	        //zc_args.user_ptr = 0;
    aZC_arg.flags = ONLOAD_MSG_DONTWAIT | ONLOAD_MSG_RECV_OS_INLINE;
	while (!hdr->is_done()){
		dispatcher.set_external_start();
		dispatcher.poll_one(&aZC_arg);
	}
	for (size_t c=0; c<ttChannel; c++){
		string name=bName+std::to_string(c);
		dispatcher.stop_sync(hdr,name ); 
	}
	delete hdr;
}

void start_exchange(Parameter& aParam, fd_sets mcast_fds, fd_sets udpSend_fds, fd_sets udpRead_fds)
{
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
	Application_Config eConfig;
	Application app(eConfig);
	//ASX_Test_Handler handler(&app);

	MDManager md(&app);
	ExchangeID exch(ExchangeID::XASX);//, 10058);//XASX 10058
	Simple_Dispatcher dispatcher;
	boost::shared_ptr<ASX_Test_Handler> ptHdr(new ASX_Test_Handler(&app));
	ASX_Test_Handler* hdr=ptHdr.get();//new ASX_Test_Handler(&app);
	hdr->init("ASX_Test_Handler", aParam);
	//vector<hf_cwtest::snapshot_session::channel_info>& channels=hdr->m_channels;
	string bName="test";
	for (size_t c=0; c<ttChannel; c++){
		const string name=bName+std::to_string(c);
		size_t ch_id=c;
		boost::shared_ptr<hf_cwtest::snapshot_session::channel_info> oneC(new hf_cwtest::snapshot_session::channel_info());
		hdr->m_channels.push_back(oneC);
		hf_cwtest::snapshot_session::channel_info& newC=*(oneC.get());
		newC.username=name;
		newC.channel_id=ch_id;
		newC.mcast_fd=mcast_fds[c];
		newC.udp_send_fd=udpSend_fds[c];
		newC.udp_read_fd=udpRead_fds[c];
		//newC.snapshot_skt.fd=TCP_fds[c];
		//newC.snapshot_skt.set_nodelay();
		//newC.snapshot_skt.set_nonblocking();
		//newC.tcp_send_fd=newC.tcp_read_fd=TCP_fds[c];
		newC.final_message_completed=false;
		dispatcher.add_udp_fd(hdr, name, ch_id, udpRead_fds[c]);
		//dispatcher.add_tcp_fd(hdr, name, ch_id, TCP_fds[c], 56000);
	}
//****** load data into channel
cout << "Exchange process " << ::getpid() << " flag:" << *turn_on << endl;
	if (aParam.has("data_file_name")){
		hdr->load_test_data(aParam["data_file_name"].getString());
	}
	//*turn_on = 'Y'; 
cout << "finish loading data to outbound channels" << endl;
cout << "Exchange process " << ::getpid() << " flag:" << *turn_on << endl;

	bool open_early=checkIfClientLate(aParam);
	if (!open_early){
		//then I have to wait
		while (*turn_on!='G')
		{
			size_t n_sec=1;
			cout << "process " << ::getpid() << " flag:" << *turn_on;
			cout << " wait for " << 1 <<" secs to wakeup child" << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(n_sec*1000));
		}
	}
	*turn_on='G';
	openTcpPortListen(aParam, ttChannel);
	for (size_t c=0; c<ttChannel; c++){
		const string name=bName+std::to_string(c);
		size_t ch_id=c;
		hf_cwtest::snapshot_session::channel_info& tcpC=*(hdr->m_channels[c].get());
		tcpC.snapshot_skt.fd=TCP_fds[c];
		//tcpC.snapshot_skt.set_nodelay();
		//tcpC.snapshot_skt.set_nonblocking();
		tcpC.tcp_send_fd=tcpC.tcp_read_fd=TCP_fds[c];
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

//
	//
	//need to add handler status
	//for debug
	dispatcher.set_external_start();
	//so that the outer loop can check the status to stop
	//and remove stop the dispatcher (it will be run as thread with run_loop function)
	//need to set the udp reader
	thread mcast_thread(&ASX_Test_Handler::start_multicast, hdr);
	//need to add_udp_fd
	//if I don't wait for child, 
	if (open_early){
	cout << "exchange process " << ::getpid() << " flag:" << *turn_on;
std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//I have to inform child to GO now
	*turn_on='G';
	cout << "exchange process " << ::getpid() << " flag:" << *turn_on;
	}
	else {
	munmap(turn_on, sizeof *turn_on); 
	}
	// for readable, writable, .....
	//need to add_tcp_fd
	while (!hdr->is_done()){
		dispatcher.set_external_start();
		dispatcher.poll_one();
		hdr->check_write_event();
	}
	for (size_t c=0; c<ttChannel; c++){
		string name=bName+std::to_string(c);
		dispatcher.stop_sync(hdr,name ); 
	}
	delete hdr;

}

int
main1(int argc, char *argv[])
{
	Parameter aParam;
	int mcastfd[5][2]; //mcast 1
	int UdpU[5][2]; //for retransmi
	int UdpD[5][2];
	//int TcpU[5][2]; //for TCP
	//int TcpD[5][2];
	pid_t cpid;
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
		char *pH=cfgbuf;
		char *p0=strchr(cfgbuf, '#');
		if (p0) *p0=0;
		if (strlen(cfgbuf) < 2) continue;
	  //string line(cfgbuf);
	  p0=strchr(pH, ':');
	  if (!p0) continue;
	  *p0++=0;
		while ((*p0 <= ' ' || *p0=='"' || *p0=='\'') && *p0) p0++;
	  char* key = strtok(cfgbuf, " :");
	  char* val = strtok(NULL, " :\n");
	  Parameter saveData;
	  
	  if (strstr("True true False false N Y ", p0)){
		  if (p0[3]=='u' || p0[0]=='Y') saveData.setBool(true);
		  else saveData.setBool(false);
	  }
	  else
	  saveData.setString(val);
	  
	  h_pm[trim(key)]=saveData;
	}
	if (h_pm.find("session_name")==h_pm.end()){
		Parameter saveData;
		saveData.setString(string("CW-TEST-01"));
		h_pm[string("session_name")]=saveData;
	}
	aParam.setDict(h_pm);
	cout << aParam << endl;	
	if (!aParam.has("data_file_name") ||
			!aParam.has("TCP_port") ||
			!aParam.has("total_channels") ||
			!aParam.has("username") ||
			!aParam.has("password")){
		fprintf(stderr, "%s missing keyword data_file_name or username or password \n", argv[1]);
		                  exit(EXIT_FAILURE);
	}
	total_channels=stoi(aParam["total_channels"].getString());
	int listen_fds[total_channels];
	tcp_listen_fds=listen_fds;

	for (size_t i=0; i<total_channels; i++){
	if (pipe2(mcastfd[i], O_DIRECT|O_NONBLOCK) == -1 ||
		pipe2(UdpU[i], O_DIRECT|O_NONBLOCK) == -1 ||
		pipe2(UdpD[i], O_DIRECT|O_NONBLOCK) == -1 //||
		//tcp will be opened after the fork
		//pipe(TcpU[i]) == -1 ||
		//pipe(TcpD[i]) == -1 
			) 
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	}

	turn_on = (char *)mmap(NULL, sizeof *turn_on, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0); 
	*turn_on = 'N'; 

	cpid = fork();
	
	if (cpid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
//in the following
//the parent == Exchange
//the child == CW Handler
//the turn_on key is first use to sync the preconnection data load
//after both get the key value "G" they can start however
//the param will specify what mode to start
//for client_late then 
//client have to wait until exchange set the turn_on to "G"
//otherwise Exchange have to wait for the "G"
	if (cpid == 0) {    /* Child reads from pipe */
		//close(pipefd[1]);          /* Close unused write end */
		//add to poll with subscribed=yes,
		//wait for poll to return available to read
		//child will only use mcastfd[0], UdpD[0], UdpU[1], TcpD[0], TcpU[1]
		vector<int> mcastfds, UdpDs,UdpUs;//, TcpDs,  TcpUs;
		for (int i=0; i<5; i++){
			close(mcastfd[i][1]);
			mcastfds.push_back(mcastfd[i][0]);
			close(UdpD[i][1]);
			UdpDs.push_back(UdpD[i][0]);
			//close(TcpD[i][1]);
			//TcpDs.push_back(TcpD[i][0]);
			close(UdpU[i][0]);
			UdpUs.push_back(UdpU[i][1]);
			//close(TcpU[i][0]);
			//TcpUs.push_back(TcpU[i][1]);
		}
		// start a test handler object that take
		// these open fd as the sockets
		// bind those recvfrom and sendto function to
		// read and write of these pipe fd 
	//start_client(mcastfd[0], UdpD[0], UdpU[1], TcpD[0], TcpU[1]);
		start_client(aParam, mcastfds, UdpDs, UdpUs);//, TcpDs, TcpUs);
		/*
		int i=0;
		p2[1]='+';
		while (read(mcastfd[0], &buf, 1) > 0)
		{
			if (i++ % 2 == 0) {
			    p2[0]=buf;
				write(UdpU[1], p2, 2);
			}
			write(STDOUT_FILENO, &buf, 1);
		}
			write(STDOUT_FILENO, "\n", 1);
		*/
		for (int i=0; i<5; i++){
			close(mcastfd[i][0]);
			close(UdpD[i][0]);
			//close(TcpD[i][0]);
			close(UdpU[i][1]);
			//close(TcpU[i][1]);
		}
		//munmap(turn_on, sizeof *turn_on); 
		_exit(EXIT_SUCCESS);
	} else {            /* Parent writes argv[1] to pipe */
		//close(pipefd[0]);          /* Close unused read end */
		vector<int> mcastfds, UdpDs, UdpUs;//, TcpDs, TcpUs;
		for (int i=0; i<5; i++){
			close(mcastfd[i][0]);
			mcastfds.push_back(mcastfd[i][1]);
			close(UdpD[i][0]);
			UdpDs.push_back(UdpD[i][1]);
		//close(TcpD[i][0]);
		//TcpDs.push_back(TcpD[i][1]);
			close(UdpU[i][1]);
			UdpUs.push_back(UdpU[i][0]);
		//close(TcpU[i][1]);
		//TcpUs.push_back(TcpU[i][0]);
		}
		//start to build data from binary file
		//use random number to decide how many pack to send in the
		//                   snapshot period
		//use random number to decide which packet to drop
		//and save it in pending resend vector
		//try the random number whenever the pending resend is empty
		//wait for random minutes to send packet

	//start_exchange(mcastfd[1], UdpD[1], UdpU[0], TcpD[1], TcpU[0]);
		start_exchange(aParam, mcastfds, UdpDs, UdpUs);//, TcpDs, TcpUs);
		/*
			write(mcastfd[1], argv[1], strlen(argv[1]));
			while (read(UdpU[0], &buf, 1) > 0)
			{
			 	write(STDOUT_FILENO, &buf, 1);
			}
		*/
		for (int i=0; i<5; i++){
			close(mcastfd[i][1]);
			close(UdpD[i][1]);
		//close(TcpD[i][1]);
			close(UdpU[i][0]);
		//close(TcpU[i][0]);
		}
			wait(NULL);                /* Wait for child */
			exit(EXIT_SUCCESS);
	}
}

int
main(int argc, char *argv[])
{
	Parameter aParam;
	int mcastfd[5][2]; //mcast 1
	int UdpU[5][2]; //for retransmi
	int UdpD[5][2];
	//int TcpU[5][2]; //for TCP
	//int TcpD[5][2];
	pid_t cpid;
	char cfgbuf[640];
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
	hash_map<string, Parameter> h_mp;
	while(fgets(cfgbuf, 640, cfg)){
            uint8_t len=strlen(cfgbuf);
            if (len < 1) continue;
            char *p=&cfgbuf[0];
            char *p9=&cfgbuf[len-1];
            while (*p <= ' ' && p < p9) p++;
            while (*p9 <= ' ' && p9>p) p9--;
            if (*p == '#') continue;
            if (p==p9 && *p9 <= ' ') continue;
            *(p9+1)=0;
	  //string line(cfgbuf);
            if (!strchr(p, ':')) continue;

            Parameter saveData;
            bool need_more=!strchr(p, ']');
            char* key = strtok(p, ":");
            char* val = strtok(NULL, "]\n");
            if (!key || !val) continue;
            while (*key && *key <= ' ') key++;
            while (*val && *val <= ' ') val++;
            if (!*key || !*val) continue;
            char *pV=strchr(val, '{');
            if (pV){
		if (!strstr(key, "mdcache"))
                continue;
	hash_map<string, Parameter> mdmap;
	//create_mdcache_param( mdmap, cfg);
		saveData.setDict(mdmap);
			h_mp["mdcache"]=saveData;
                continue;
            }
            pV=strchr(val, '[');
            if (pV){
		  pV++;
		  char *pE=strchr(pV, ']');
		  if (pE)*pE=0;
		  vector<string> v;
		  while (pV && *pV)
		  {
			  char *pComma=strchr(pV, ',');
			  if (pComma) *pComma=0;
			  v.push_back(trim(pV));
			  if (!pComma) break;
			  pV = ++pComma;
		  }
		  if (!need_more){
		  	saveData.setStringVector(v);
	          	h_mp[key]=saveData;
		  	continue;
		  }
		  string vec_text="";
		  while (need_more){
			fgets(cfgbuf, 160, cfg);
			char *pH=strchr(cfgbuf, '#');
			if (pH)*pH=0;
			pH=cfgbuf;
			while (*pH <= ' ' && *pH > 0)pH++;
			if (*pH==0) continue;
			  vec_text += pH;
		      need_more=!strchr(pH, ']');
		  }
		  strcpy(cfgbuf, vec_text.c_str());
		  pV=cfgbuf;
		  pE=strchr(pV, ']');
		  if (pE)*pE=0;
		  while (pV && *pV)
		  {
			  char *pComma=strchr(pV, ',');
			  if (pComma) *pComma=0;
			  v.push_back(trim(pV));
			  if (!pComma) break;
			  pV = ++pComma;
		  }
		  saveData.setStringVector(v);
	          h_mp[key]=saveData;
		  continue;
	  }
	  if (!val || strlen(val)<1) continue;
	  if (val[0]=='{') continue;
	  
	  if (strstr(" True true False false Y N y n ", val))
	  {
		  saveData.setBool((val[2]=='u' || val[0]=='Y' || val[0]=='y')?true:false);
	  }
	  else
	  saveData.setString(trim(val));
	  
	  h_mp[trim(key)]=saveData;
	}
	if (h_mp.find("session_name")==h_mp.end()){
		Parameter saveData;
		saveData.setString(string("CW-TEST-01"));
		h_mp[string("session_name")]=saveData;
	}
	aParam.setDict(h_mp);
	cout << aParam << endl;	
	if (!aParam.has("data_file_name") ||
			!aParam.has("TCP_port") ||
			!aParam.has("total_channels") ||
			!aParam.has("username") ||
			!aParam.has("password")){
		fprintf(stderr, "%s missing keyword data_file_name or username or password \n", argv[1]);
		                  exit(EXIT_FAILURE);
	}
	total_channels=stoi(aParam["total_channels"].getString());
	int listen_fds[total_channels];
	tcp_listen_fds=listen_fds;

	for (size_t i=0; i<total_channels; i++){
	if (pipe(mcastfd[i]) == -1 ||
		pipe(UdpU[i]) == -1 ||
		pipe(UdpD[i]) == -1 //||
		//tcp will be opened after the fork
		//pipe(TcpU[i]) == -1 ||
		//pipe(TcpD[i]) == -1 
			) 
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	}

	turn_on = (char *)mmap(NULL, sizeof *turn_on, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0); 
	*turn_on = 'N'; 

	cpid = fork();
	
	if (cpid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
//in the following
//the parent == Exchange
//the child == CW Handler
//the turn_on key is first use to sync the preconnection data load
//after both get the key value "G" they can start however
//the param will specify what mode to start
//for client_late then 
//client have to wait until exchange set the turn_on to "G"
//otherwise Exchange have to wait for the "G"
	if (cpid == 0) {    /* Child reads from pipe */
		//close(pipefd[1]);          /* Close unused write end */
		//add to poll with subscribed=yes,
		//wait for poll to return available to read
		//child will only use mcastfd[0], UdpD[0], UdpU[1], TcpD[0], TcpU[1]
		vector<int> mcastfds, UdpDs,UdpUs;//, TcpDs,  TcpUs;
		for (int i=0; i<5; i++){
			close(mcastfd[i][1]);
			mcastfds.push_back(mcastfd[i][0]);
			close(UdpD[i][1]);
			UdpDs.push_back(UdpD[i][0]);
			//close(TcpD[i][1]);
			//TcpDs.push_back(TcpD[i][0]);
			close(UdpU[i][0]);
			UdpUs.push_back(UdpU[i][1]);
			//close(TcpU[i][0]);
			//TcpUs.push_back(TcpU[i][1]);
		}
		// start a test handler object that take
		// these open fd as the sockets
		// bind those recvfrom and sendto function to
		// read and write of these pipe fd 
	//start_client(mcastfd[0], UdpD[0], UdpU[1], TcpD[0], TcpU[1]);
		start_client(aParam, mcastfds, UdpDs, UdpUs);//, TcpDs, TcpUs);
		/*
		int i=0;
		p2[1]='+';
		while (read(mcastfd[0], &buf, 1) > 0)
		{
			if (i++ % 2 == 0) {
			    p2[0]=buf;
				write(UdpU[1], p2, 2);
			}
			write(STDOUT_FILENO, &buf, 1);
		}
			write(STDOUT_FILENO, "\n", 1);
		*/
		for (int i=0; i<5; i++){
			close(mcastfd[i][0]);
			close(UdpD[i][0]);
			//close(TcpD[i][0]);
			close(UdpU[i][1]);
			//close(TcpU[i][1]);
		}
		//munmap(turn_on, sizeof *turn_on); 
		_exit(EXIT_SUCCESS);
	} else {            /* Parent writes argv[1] to pipe */
		//close(pipefd[0]);          /* Close unused read end */
		vector<int> mcastfds, UdpDs, UdpUs;//, TcpDs, TcpUs;
		for (int i=0; i<5; i++){
			close(mcastfd[i][0]);
			mcastfds.push_back(mcastfd[i][1]);
			close(UdpD[i][0]);
			UdpDs.push_back(UdpD[i][1]);
		//close(TcpD[i][0]);
		//TcpDs.push_back(TcpD[i][1]);
			close(UdpU[i][1]);
			UdpUs.push_back(UdpU[i][0]);
		//close(TcpU[i][1]);
		//TcpUs.push_back(TcpU[i][0]);
		}
		//start to build data from binary file
		//use random number to decide how many pack to send in the
		//                   snapshot period
		//use random number to decide which packet to drop
		//and save it in pending resend vector
		//try the random number whenever the pending resend is empty
		//wait for random minutes to send packet

	//start_exchange(mcastfd[1], UdpD[1], UdpU[0], TcpD[1], TcpU[0]);
		start_exchange(aParam, mcastfds, UdpDs, UdpUs);//, TcpDs, TcpUs);
		/*
			write(mcastfd[1], argv[1], strlen(argv[1]));
			while (read(UdpU[0], &buf, 1) > 0)
			{
			 	write(STDOUT_FILENO, &buf, 1);
			}
		*/
		for (int i=0; i<5; i++){
			close(mcastfd[i][1]);
			close(UdpD[i][1]);
		//close(TcpD[i][1]);
			close(UdpU[i][0]);
		//close(TcpU[i][0]);
		}
			wait(NULL);                /* Wait for child */
			exit(EXIT_SUCCESS);
	}
}

