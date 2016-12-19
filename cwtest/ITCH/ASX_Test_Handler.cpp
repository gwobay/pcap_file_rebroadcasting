#include <Util/File_Utils.h>
#include <ITCH/Snapshot_Handler.h>
#include <ITCH/ASX_Test_Handler.h>
#include <ITCH/Simple_Dispatcher.h>
#include <ITCH/functions.h>
#include <stdarg.h>
#include <stdint.h>
#include <thread>
#include <boost/shared_array.hpp>
#include <boost/thread/thread.hpp>
#include <vector>

namespace hf_cwtest {

	using namespace hf_tools;
	using namespace hf_core;

void ASX_Test_Handler::init(const string& name, const Parameter& aParam)
{
	my_name=name;
	m_verify_message=false;
	if (aParam.has("verify_message"))
	{
		m_verify_message=aParam["verify_message"].getBool();
	}
	m_broadcast_packet_max=10;
	if (aParam.has("broadcast_packet_max"))
	m_broadcast_packet_max=stoi(aParam["broadcast_packet_max"].getString());
	m_drop_packet_frequency=1000;
	if (aParam.has("drop_packet_frequency"))
	m_drop_packet_frequency=stoi(aParam["drop_packet_frequency"].getString());
	m_drop_packet_frequency=1000;
	m_drop_packet_frequency=stoi(aParam.getDefaultString("drop_packet_frequency", "1200"));

	if (!aParam.has("username")||
		!aParam.has("password"))
	return;
	m_test_session=aParam["session_name"].getString();
add_user(aParam["username"].getString(), aParam["password"].getString());

	m_first_seqno=200;
	m_first_seqno=stoi(aParam.getDefaultString("snapshot_missing_count", "200"));
	m_mcast_interval=50;
	m_mcast_interval=stoi(aParam.getDefaultString("mcast_send_interval", "50"));
	m_packet_size=30;
	m_packet_size=stoi(aParam.getDefaultString("packet_size", "30"));
	
	m_tcp_interval=10;
	m_tcp_interval=stoi(aParam.getDefaultString("tcp_send_interval", "5"));

	m_recovery_interval=5;
	m_recovery_interval=stoi(aParam.getDefaultString("recovery_send_interval", "5"));
return;
}
void ASX_Test_Handler::set_udp_writer(size_t ch_id, UDP_READER aWriter){
	snapshot_session::channel_info* wk_channel=get_channel_by_id(ch_id);
	wk_channel->udp_writer=aWriter;
}

#include <Util/File_Utils.h>
void ASX_Test_Handler::load_test_data(const char* pData, size_t len)
{
	const char* pBegin=pData;
	const char* pStop=pData+len;
	while (pData < pStop){
    
		size_t channel_id=(size_t)(*pBegin);
      uint16_t message_length = ntohs(reinterpret_cast<const itch_session_packet::message_block_t*>(++pBegin)->message_length);
	    
      boost::shared_array<char> bb(new char[message_length+2]);
      memcpy(bb.get(), pBegin, message_length+2);
snapshot_session::channel_info* wk_channel=get_channel_by_id(channel_id);

	wk_channel->m_outbound_msg_list.push_back(bb);
      pBegin += 2 + message_length;
    }
}

void ASX_Test_Handler::load_test_data(const string& input){

	FILE* file1=0;
  
  if(has_gzip_extension(input)) {
    string file_cmd("/bin/zcat ");
    file_cmd += input;
    file1 = popen(file_cmd.c_str(), "r");
  } else if(has_7zip_extension(input)) {
    string file_cmd("/tools/p7zip/redhat/x_64/9.20.1/bin/7z x -so -bd -y ");
    file_cmd += input;
    file1 = popen(file_cmd.c_str(), "r");
  } else if(has_xz_extension(input)) {
    string file_cmd("/usr/bin/xzcat ");
    file_cmd += input;
    file1 = popen(file_cmd.c_str(), "r");
  } else if (input == "-") {
    file1 = stdin;
  } else {
    file1 = fopen(input.c_str(), "r");
  }
  
	if(!file1) {
#ifdef CW_DEBUG
cout << "data file" << input << " not found" << std::endl;
#endif
			        exit(2);
	}
	size_t mxChannelId=0;
	size_t header_len = sizeof(record_header);
	char buffer[1 * 1024 * 1024];
	while(true) {
		size_t header_len = sizeof(record_header);
		size_t r = fread(buffer, header_len, 1, file1);
		if(!r) {
			      break;
		}
  reread:
    if(strncmp(buffer, record_header_title, 4) != 0) {
	    /*
        if(!check_record_header(file1, buffer, 128)) {
          fprintf(stderr, "file1 - Problem in record header title\n");
          return 10;
        }
	*/
        goto reread;
      }
    
    size_t msg_len = 0;
      record_header *p = reinterpret_cast<record_header*>(buffer);
      msg_len = p->len;
    
    r = fread(buffer + header_len, msg_len, 1, file1);
    if(!r) {
      fprintf(stderr, "Short last record in file\n");
      return 12;
    }

    const record_header* rh = reinterpret_cast<const record_header*>(buffer);
    char* buf=buffer;
    buf += sizeof(record_header);
    size_t len=header_len + msg_len;
    len -= sizeof(record_header);
    
    /*if(m_filters.start.is_set()) {
      if(Time(rh->ticks) < m_filters.start) {
        return len;
      }
    }*/
    
    Time::set_current_time(Time(rh->ticks));
    
    char time_buf[32];
    Time_Utils::print_time(time_buf, Time(rh->ticks), Timestamp::MICRO);
    
    /*
    m_logger->printf("qrec_record_time=%s,context=%lu,qrec_len=%u", time_buf, rh->context, rh->len);
    */
    
    if (rh->context>mxChannelId) mxChannelId=rh->context;
    fill_message_list(rh->context, buf, rh->len);
  }

   for (int i=0; i<= mxChannelId; i++){
	snapshot_session::channel_info* wk_channel=get_channel_by_id(i);
	if (!wk_channel) continue;
	size_t mxSeqNo=wk_channel->m_outbound_msg_list.size();
	cout << "Ch. " << wk_channel->channel_id << " has " << mxSeqNo << " msgs " << endl;
		for (size_t gp=1; gp < mxSeqNo; gp += m_packet_size)
		wk_channel->pack_seqno_list.push_back(gp);
   }

  
  fclose(file1);
  file1 = 0;
  
  return ;
}

	void ASX_Test_Handler::init(const string& filename){
		//load messages into channel from production bin file
		load_test_data(filename);

	}
	void ASX_Test_Handler::init(){

	}
	  
void ASX_Test_Handler::start()
{
	    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
	      i->clear();
	    }

	    clear_order_books(false);
}

bool ASX_Test_Handler::is_done(){
	for (size_t i=0; i< m_channels.size(); i++){
		if (m_channels[i].get()->final_message_completed ) continue;
		return false;
	}
	return true;
}
	  void
	  ASX_Test_Handler::stop()
	  {
	    for(channel_info_map_t::iterator i = m_channel_info_map.begin(), i_end = m_channel_info_map.end(); i != i_end; ++i) {
	      i->clear();
	    }

	    clear_order_books(false);
	  }

void ASX_Test_Handler::sync_udp_channel(string name, size_t context, int  fd){

}

void ASX_Test_Handler::sync_tcp_channel(string name, size_t context, int  fd){
	
}

void ASX_Test_Handler::update_channel_seqno_time(snapshot_session::channel_info& wk_channel, size_t last_seqno){
	wk_channel.last_out_seqno=last_seqno;
	wk_channel.next_packet_seqno=last_seqno+1;
	wk_channel.last_sent_time=Time::current_time();
}

void 
ASX_Test_Handler::verify_outbound_packet(const char* buf)
{
	if (!m_verify_message) return;
    const itch_session_packet::downstream_header_t* header = 
    reinterpret_cast<const itch_session_packet::downstream_header_t*>(buf);
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
//for test
   //vector<std::shared_ptr<char> > retV; 
	   printf("org seqno %lu -> new:%lu\n", header->seq_num, seq_num);
   const char* block=buf+sizeof(*header);
   printf("org count %u -> new:%u\n", header->message_count, message_count);
    for(uint16_t i = 0; i < message_count; ++i) {
      uint16_t message_length = ntohs(reinterpret_cast<const itch_session_packet::message_block_t*>(block)->message_length);
		//printf ("message_length:%d ", message_length);
	//dump_message(block+2);
      block += 2 + message_length;
    }
    
    //return retV;
    return ;
}

size_t ASX_Test_Handler::create_packet(snapshot_session::channel_info& wk_channel, size_t seqno_begin, size_t& count)
{
	vector<boost::shared_array<char> >::iterator iBegin=wk_channel.m_outbound_msg_list.begin()+(seqno_begin-1);
	//vector<boost::shared_array<char> >::iterator iEnd=wk_channel.m_outbound_msg_list.begin()+(seqno_begin-1+count-1);
	char* buf=wk_channel.one_pack_buffer;
	itch_session_packet::downstream_header_t* aHeader=
      reinterpret_cast<itch_session_packet::downstream_header_t*>(buf);
	memset(aHeader, ' ', sizeof(*aHeader));
memcpy(aHeader->session, m_test_session.c_str(), sizeof(aHeader->session));
	uint64_t seqNo=seqno_begin;
	aHeader->seq_num=htonll(seqNo);
	uint16_t msgCount=count;
#ifdef CW_DEBUG
cout << "use session " << m_test_session << endl;
//printf("sending seqno %lu count %u\n", seqNo, msgCount);
#endif
	size_t loc=sizeof(*aHeader);
	while (count > 0){
		char *data=(*iBegin++).get();
		uint16_t len = ntohs(reinterpret_cast<const itch_session_packet::message_block_t*>(data)->message_length);
		len += 2;
		if (loc+len > sizeof(wk_channel.one_pack_buffer)){
	printf("*****Packet too big to fit 56k -----");
				break;
		}
		memcpy(&buf[loc], data, len);//sizeof((*itr).get()));
		loc += len;
		count--;
	}
	count=msgCount - count;
	aHeader->message_count=htons(count);
	return loc;
}

void ASX_Test_Handler::send_next_packet(snapshot_session::channel_info& wk_channel)
{
	//snapshot_session::channel_info& wk_channel=*(m_channels[i].get());
	size_t first_seqno=wk_channel.next_packet_seqno;
	size_t last_seqno=first_seqno + m_packet_size-1;;
	
	if (last_seqno > wk_channel.m_next_drop_seqno){
		wk_channel.next_packet_seqno=last_seqno+m_packet_size*drand48()+10;
		update_channel_seqno_time(wk_channel, last_seqno);
		wk_channel.m_next_drop_seqno += m_drop_packet_frequency;
#ifdef CW_DEBUG
cout << __func__ << " drop " << first_seqno << " to " << wk_channel.next_packet_seqno << endl;
#endif
		return;
	}
	vector<boost::shared_array<char> >::iterator iBegin=wk_channel.m_outbound_msg_list.begin()+(first_seqno-1);
	vector<boost::shared_array<char> >::iterator iLast=wk_channel.m_outbound_msg_list.begin()+(last_seqno-1);
	char buf[60*1000];
	itch_session_packet::downstream_header_t* aHeader=
      reinterpret_cast<itch_session_packet::downstream_header_t*>(buf);
	memset(aHeader, ' ', sizeof(*aHeader));
memcpy(aHeader->session, m_test_session.c_str(), sizeof(aHeader->session));
	uint64_t seqNo=first_seqno;
	aHeader->seq_num=htonll(seqNo);
	uint16_t msgCount=last_seqno-first_seqno+1;
	aHeader->message_count=htons(msgCount);
#ifdef CW_DEBUG
cout << "use session " << m_test_session << endl;
#endif
	//if (first_seqno < 2){
//printf("seqno from %d to %lu to %lu\n", first_seqno, seqNo, aHeader->seq_num);
//printf("count from %d to to %u to %u\n", last_seqno-first_seqno+1, msgCount, aHeader->message_count); 
	//}
printf("ch-%d sending seqno %lu count %u\n", wk_channel.channel_id, seqNo, msgCount);
	size_t loc=sizeof(*aHeader);
	for (vector<boost::shared_array<char> >::iterator itr=iBegin; itr<=iLast;itr++){
		char *data=(*itr).get();
		if (m_verify_message && first_seqno < 2){
			//dump_message(data+2);
		}
		uint16_t len = ntohs(reinterpret_cast<const itch_session_packet::message_block_t*>(data)->message_length);
		len += 2;
		memcpy(&buf[loc], data, len);//sizeof((*itr).get()));
		loc += len;
	}
	if (first_seqno < 2){
		//memcpy(buf, packet1, packet1_len);
		//loc=packet1_len;
		//verify_outbound_packet(buf);
	}
	size_t ret=loc;
	struct sockaddr to_addr;
	if (wk_channel.udp_send_fd > 0)
	ret=send_to_udp_fd(wk_channel.udp_send_fd, buf, loc, 0, &to_addr, sizeof(to_addr));
	if (ret <=0 )
	{
printf("%s failed fd=%d with %s\n", __func__, wk_channel.udp_send_fd, strerror(errno));
		return;
	}

	 printf("%s channel %d has %d sent out %d bytes on fd=%d\n", __func__, wk_channel.channel_id, loc, ret, wk_channel.mcast_fd);
	
	 update_channel_seqno_time(wk_channel, last_seqno);
	wk_channel.next_mcast_time=Time::current_time()+msec(m_mcast_interval);
	//(int64_t(drand48()*5000)));
	 wk_channel.next_mcast_packet=last_seqno+1;
	 /*
	wk_channel.last_out_seqno=last_seqno;
	wk_channel.next_packet_seqno=last_seqno+1;
	wk_channel.last_sent_time=Time::current_time();
	*/
	
}

size_t ASX_Test_Handler::send_to_udp_fd(int fd, const char* buffer, size_t len, int fg, struct sockaddr* addr, size_t addr_size)
{
	 int ret ;
#ifndef CW_DEBUG
		 struct sockaddr_in to_addr;
		 //in reallistic case, host, port need to be specified
	  ret = ::sendto(fd, buffer, len, 0, addr, addr_size);
#else
		 char mb[len+4];
		 memcpy(mb, buffer, len);
		 //memset(mb+len, UDP_BOUNDARY, 3);
	 	ret = ::write(fd, buffer, len);
	 	//ret = ::write(fd, buffer, len+3);
	 	//::write(fd, mb, 3); 
#endif

printf("\n%s has %d sent out %d bytes on fd=%d\n", __func__, len, ret, fd);
	 if(ret < 0) {
#ifdef CW_DEBUG
cout << __func__ << " failed " << strerror(errno) <<endl;
#endif
	 }
       return ret;
}

void ASX_Test_Handler::start_multicast()
{
#ifdef CW_DEBUG
cout << __func__ << " started !!" << endl;
#endif
	//use 2 seconds as max
	for (size_t i=0; i<m_channels.size(); i++){
		snapshot_session::channel_info& mcast_ch=*(m_channels[i].get());
		mcast_ch.m_next_drop_seqno=0;
		mcast_ch.next_packet_seqno=m_first_seqno+5*i;
		while (mcast_ch.m_next_drop_seqno < mcast_ch.next_packet_seqno)
		mcast_ch.m_next_drop_seqno += m_drop_packet_frequency;
		
	}
	size_t send_count=0;
    while (!is_done()){
	for (size_t i=0; i<m_channels.size(); i++){
		snapshot_session::channel_info& mcast_ch=*(m_channels[i].get());
		if (Time::current_time()< mcast_ch.next_mcast_time) 
			continue;
		send_next_packet(mcast_ch);
	} 
	//if (++send_count > m_broadcast_packet_max) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
#ifdef CW_DEBUG
cout << __func__ << " end !!" << endl;
#endif
}
//vector <std::shared_ptr<char> >
  
void
ASX_Test_Handler::dump_message(const char* buf)
{
    switch(int(buf[0])) {
    case int('T'):
      {
        const itch_session_packet::timestamp_message& msg = reinterpret_cast<const itch_session_packet::timestamp_message&>(*buf);
	
        uint32_t sec = ntohl(msg.seconds);
        Time_Duration ts = seconds(sec);
	
        char time_buf[32];
        Time_Utils::print_time(time_buf, ts, Timestamp::MILI);
	
        printf("  timestamp %s\n", time_buf);
      }
      break;
    case int('R'):
      {
        const itch_session_packet::order_book_directory& msg = reinterpret_cast<const itch_session_packet::order_book_directory&>(*buf);
        uint32_t ns = ntohl(msg.nanoseconds);
	printf("  order_book_directory ns=%u symbol=%-.32s isin=%-.12s currency=%-.3s financial_product=%hhd num_decimals_in_price=%hu\n",
                   ns, msg.symbol, msg.isin, msg.currency, msg.financial_product, ntohs(msg.num_decimals_in_price));
      }
      break;
    case int('M'):
      {
	printf("  combination_order_book_directory");
      }
      break;
    case int('S'):
      {
        const itch_session_packet::system_event_message& msg = reinterpret_cast<const itch_session_packet::system_event_message&>(*buf);
	
	printf("  system_event_message code=%c\n", msg.event_code);
      }
      break;
    case int('A'):
      {
        const itch_session_packet::add_order_no_mpid& msg = reinterpret_cast<const itch_session_packet::add_order_no_mpid&>(*buf);
	
        uint32_t ns = ntohl(msg.nanoseconds);
        uint64_t order_id = ntohll(msg.order_id);
        uint64_t qty = ntohll(msg.quantity);
        uint32_t shares = (uint32_t)qty;
	MDPrice price = MDPrice::from_fprice(ntohl(msg.price));
        
        printf("  add_order ns:%u order_id:%lu side:%c shares:%u stock:%u price:%0.4f\n",
                   ns, order_id, msg.side, shares, ntohl(msg.order_book_id), price.fprice());
      }
      break;
    default:
      printf("  message_type:%c not supported\n", buf[0]);
      break;
    }
    return ;
}
void 
ASX_Test_Handler::fill_message_list(size_t ch_id, const char* buf, size_t len)
{
	/** for test only
	for (size_t ss=0; ss<len-2; ss++){
		if (buf[ss]!=UDP_BOUNDARY//) //continue;
			|| buf[ss+1]!=UDP_BOUNDARY //) continue;
			|| buf[ss+2]!=UDP_BOUNDARY) continue;
#ifdef CW_DEBUG
cout << "*************found " << UDP_BOUNDARY << endl;
#endif
exit(EXIT_FAILURE);
		break;
	}
	*/
    snapshot_session::channel_info* wk_channel=get_channel_by_id(ch_id);
    const itch_session_packet::downstream_header_t* header = 
	    reinterpret_cast<const itch_session_packet::downstream_header_t*>(buf);
    uint64_t seq_num = ntohll(header->seq_num);
    //adjust the seq. because no send for L msg
	seq_num=wk_channel->m_outbound_msg_list.size();
	seq_num++;

    if (wk_channel->next_out_seqno==0) wk_channel->next_out_seqno=seq_num;
    //if (wk_channel->pack_seqno_list.size()==0 ||
		    //seq_num > wk_channel->pack_seqno_list.back())
    //wk_channel->pack_seqno_list.push_back(seq_num);
    uint16_t message_count = ntohs(header->message_count);
//for test
    const char* block = buf + sizeof(itch_session_packet::downstream_header_t);
   //vector<std::shared_ptr<char> > retV; 
   bool print1Record=true;
   if (seq_num < 2){
	   /*
#ifdef CW_DEBUG
cout << "save packet1 with len=" << len << endl;
#endif
	   memcpy(packet1, buf, len);
	   packet1_len=len;
	   printf("org seqno %lu new:%lu\n", header->seq_num, seq_num);
   printf("org count %u new:%u\n", header->message_count, message_count);
	   printf("context:%lu  seq_num:%lu  message_count:%u  \n", ch_id, seq_num, message_count);
	   */
   }
    for(uint16_t i = 0; i < message_count; ++i) {
      uint16_t message_length = ntohs(reinterpret_cast<const itch_session_packet::message_block_t*>(block)->message_length);
	    if (block[2] != 'L'){
      boost::shared_array<char> bb(new char[message_length+2]);
      memcpy(bb.get(), block, message_length+2);
	wk_channel->m_outbound_msg_list.push_back(bb);
	if (seq_num < 2) {
		//printf ("message_length:%d ", message_length);
	//dump_message(block+2);
	}
	    }
      block += 2 + message_length;
    }
    /*
   cout << wk_channel->m_outbound_msg_list.size() << ", ";
   if (wk_channel->m_outbound_msg_list.size() % 20 == 0) {
	   cout << endl;
   }
   */
    //return retV;
    return ;
}

/*
string trim(const string inString){
	char *sCh=inString.c_str();
	char *p0=sCh;
	char *p9=sCh+inString.length()-1;
	while (p0<p9 && (*p0 <= ' ' || *p0 > '~')) p0++;
	while (p9>p0 && (*p9 <= ' ' || *p9 > '~')) p9--;
	if (*p9 <= ' ') return string("");
	return string(p0, p9-p0+1);
}
*/

//run this in a separate thread and run broadcase mcast_fds in
//another thread broadcasting with random time interval
//you don't need poll_event in broadcasting thread
//you need one for this snapshot thread
//
void ASX_Test_Handler::start_snapshot_session(snapshot_session::channel_info& wk_channel)
{

	size_t snapshotCount=wk_channel.snapshot_last_seqno;
#ifdef CW_DEBUG
cout << "Starting " << __func__ << " to send " << snapshotCount << " msgs" << endl;
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(1*1000));
	for (size_t i=0; i< wk_channel.next_mcast_packet; i++){//}wk_channel.m_outbound_msg_list.size(); i++){
		char *pBuf=wk_channel.m_outbound_msg_list[i].get();
      uint16_t msgLen = ntohs(reinterpret_cast<const itch_session_packet::message_block_t*>(pBuf)->message_length);
	int ret = ::send(wk_channel.snapshot_skt, pBuf, msgLen+2, MSG_DONTWAIT);
	if (ret < 0){
#ifdef CW_DEBUG
cout << "Send data failed!!!\n";
#endif
	}
	else
#ifdef CW_DEBUG
cout << __func__ << wk_channel.channel_id << " channel " << ret << " bytes"<<endl;
#endif
	//check if something to read from client side
	//make sure it is for tis channel only
	//
	//add random timer here to send next one
    std::this_thread::sleep_for(std::chrono::milliseconds(5*1000));
	}
	        send_end_snapshot(wk_channel);
}

void ASX_Test_Handler::to_write(int fd){
	snapshot_session::channel_info *wk_channel=get_channel_info(fd);
	if (!wk_channel)return;
	if (wk_channel->snapshot_skt.fd ==fd &&
			wk_channel->snapshot_last_seqno >0)
	send_snapshot_message(*wk_channel);
	else if (wk_channel->recovery_end_seqno > 0)
	send_recovery_packet(*wk_channel);
}

void ASX_Test_Handler::check_write_event(){
	for (int i=0; i<m_channels.size(); i++){
	snapshot_session::channel_info* wk_channel=m_channels[i].get();
		if (wk_channel->snapshot_last_seqno >0)
			send_snapshot_message(*wk_channel);
		if (wk_channel->recovery_end_seqno > 0)
			send_recovery_packet(*wk_channel);
	}

}
void ASX_Test_Handler::send_snapshot_message(snapshot_session::channel_info& wk_channel)
{

	if (wk_channel.snapshot_last_seqno == -1){
		if (wk_channel.snapshot_next_send_time < Time::current_time())
			wk_channel.snapshot_skt.close();
		return;
	}
	char *pBuf=wk_channel.m_outbound_msg_list[wk_channel.next_out_seqno-1].get();
	if (pBuf){
	string filter_s="TRMSA";
      uint16_t msgLen = ntohs(reinterpret_cast<const itch_session_packet::message_block_t*>(pBuf)->message_length);
      	itch_session_packet::SoupBinTcpData d;
      	d.packet_length=ntohs(msgLen+1);
      	char tcpData[msgLen+2+1];
	memcpy(tcpData, &d, 2);
	tcpData[2]='S';
      	memcpy(&tcpData[3], &pBuf[2], msgLen);
#ifdef CW_DEBUG
cout <<  __func__ << " to send " << (msgLen+3) << " bytes msgNo:" << wk_channel.next_out_seqno << " to " << wk_channel.next_mcast_packet << endl;
#endif
	try{
	int ret = ::send(wk_channel.snapshot_skt.fd, tcpData, msgLen+3, MSG_DONTWAIT);
	if (ret < 0){
#ifdef CW_DEBUG
cout << "Send data failed!!!\n";
#endif
		return;
	}

	}
	catch(std::runtime_error e){
		wk_channel.snapshot_skt.close();
		return;
	}
	
	wk_channel.next_out_seqno++;

	wk_channel.snapshot_next_send_time += msec(m_tcp_interval);
	//check if something to read from client side
	//make sure it is for tis channel only
	//
	//add random timer here to send next one

	}
	if (wk_channel.next_out_seqno >= wk_channel.next_mcast_packet)
			//(wk_channel.next_mcast_time - Time::current_time()) > msec(10)  )
	{
		wk_channel.next_mcast_time += msec(100);
	wk_channel.snapshot_last_seqno=wk_channel.next_mcast_packet-1;
        send_end_snapshot(wk_channel);
		wk_channel.snapshot_last_seqno=-1;
	}
}

void ASX_Test_Handler::create_recovery_list(
		snapshot_session::channel_info& wk_channel)
{
	size_t from=wk_channel.next_recovery_packet;
	size_t to=wk_channel.recovery_end_seqno;
	for (size_t j=0; j<wk_channel.pack_seqno_list.size(); j++){ 
		if (wk_channel.pack_seqno_list[j]<from )
			continue;
		wk_channel.recovery_packet_seqno_list.push(wk_channel.pack_seqno_list[j]);
		if (wk_channel.pack_seqno_list[j]>to)
			break;
	}
	if (wk_channel.recovery_packet_seqno_list.size()<1)return;
	wk_channel.recovery_packet_seqno_list.front()=from;
	if (wk_channel.recovery_packet_seqno_list.size() > 1)
	wk_channel.recovery_packet_seqno_list.back()=to;
	cout << "Recovery list for ch." << wk_channel.channel_id;
	cout << " from "<< from << " to " << to << " list sz:" << wk_channel.recovery_packet_seqno_list.size() << endl;
}

void ASX_Test_Handler::send_recovery_packet(snapshot_session::channel_info& wk_channel)
{
	if (wk_channel.recovery_packet_seqno_list.size() < 1){
		wk_channel.recovery_end_seqno=0;
		return;
	}
	if (Time::current_time() < wk_channel.recovery_next_send_time)
		return;
#ifdef CW_DEBUG
cout <<  __func__ << " channel " << wk_channel.channel_id << " send ";
#endif
	cout << "Recovery list for ch." << wk_channel.channel_id;
	size_t from=wk_channel.recovery_packet_seqno_list.front();
	wk_channel.recovery_packet_seqno_list.pop();
	size_t to=wk_channel.recovery_end_seqno+1;
	if (wk_channel.recovery_packet_seqno_list.size()>0)
	to=wk_channel.recovery_packet_seqno_list.front();
	cout << " from "<< from << " to " << to << " list sz:" << wk_channel.recovery_packet_seqno_list.size() << endl;
	size_t msgCount=to-from;
	size_t ret=create_packet(wk_channel, from, msgCount); //** msgCount changed after create_packet call
	char *pBuf=wk_channel.one_pack_buffer;
	if (!ret) return;
	m_test_mode=true;
size_t ret1=send_to_udp_fd(wk_channel.udp_send_fd, pBuf, ret, 0, 0, 0);
	
#ifdef CW_DEBUG
cout << from << " to " << (from+msgCount) << " " << ret1 << " bytes"<<endl;
#endif
	wk_channel.next_recovery_packet=from+msgCount;
	//wk_channel.next_recovery_packet -= msgCount;
	if (wk_channel.recovery_packet_seqno_list.size() > 0)
	wk_channel.recovery_packet_seqno_list.front()=wk_channel.next_recovery_packet;
	else wk_channel.recovery_end_seqno=0;
	wk_channel.recovery_next_send_time += msec(m_recovery_interval);
}

size_t 
ASX_Test_Handler::parse(size_t context_id, const char* buffer, size_t len)
{
snapshot_session::channel_info* wk_channel=get_channel_by_id(context_id);
if (!wk_channel) return len;

	//this message might come from a Q !!
    const itch_session_packet::downstream_header_t* header = reinterpret_cast<const itch_session_packet::downstream_header_t*>(buffer);
    
    uint64_t seq_num = ntohll(header->seq_num);
    uint16_t message_count = ntohs(header->message_count);
    if (message_count < 1) return len;
    //uint64_t next_seq_num = seq_num + message_count;
    
    if (m_test_mode){
    printf("channel %d get retransmit request seqno %lu count %u\n", context_id, seq_num, message_count);//header->seq_num, header->message_count);
    //printf("count from %u new %u\n", header->message_count, message_count);
    }
    seq_num=seq_num>0?seq_num:1;
    wk_channel->next_recovery_packet=seq_num;
    wk_channel->recovery_end_seqno=seq_num+message_count-1;
    create_recovery_list(*wk_channel);
    wk_channel->recovery_next_send_time=Time::current_time();
    send_recovery_packet(*wk_channel);
    return sizeof(*header);
}

void ASX_Test_Handler::confirm_login(snapshot_session::channel_info& wk_channel, const char* buf)
{
	//TO-DO
	const itch_session_packet::SoupBinTcpLogin* pLogin=reinterpret_cast<const itch_session_packet::SoupBinTcpLogin*>(buf);
	string username(pLogin->username, sizeof(pLogin->username));
	string password(pLogin->password, sizeof(pLogin->password));
	trim(username);
	trim(password);
	string legal1=m_user_info[trim(username)];
	if (legal1==trim(password)){
		if (wk_channel.snapshot_seqno_1 < 1)
			wk_channel.snapshot_seqno_1=1;
		wk_channel.snapshot_seqno_1=stoi(pLogin->seqno);
		send_accept(wk_channel);
		wk_channel.snapshot_next_send_time=Time::current_time()+msec(10);
	wk_channel.snapshot_last_seqno=wk_channel.snapshot_seqno_1+500;
	if (wk_channel.snapshot_seqno_1 >= wk_channel.next_mcast_packet)
	{
		//not available for message no > mcast msgno so
		send_end_snapshot(wk_channel);
		
	}
	//wk_channel.snapshot_last_seqno=wk_channel.m_outbound_msg_list.size()/50;
	}

	//boost::thread snapshot_thread(&ASX_Test_Handler::start_snapshot_session, this, wk_channel);
	//verify the username and password for login:w
	//
}

void ASX_Test_Handler::send_data(snapshot_session::channel_info& wk_channel, size_t last_snapshot_seqno)
{
}

void ASX_Test_Handler::send_accept(snapshot_session::channel_info& wk_channel){
	itch_session_packet::SoupBinTcpAccept a;
	memset(&a, ' ', sizeof(a));
	a.packet_type='A';
	a.packet_length=htons(31);
	//snapshot_session::channel_info wk_channel=m_channels[context];
	//memcpy(&(a.session), m_session->name().c_str(), sizeof(a.session));
	//memcpy(&(a.session), m_session->name().c_str(), sizeof(a.session));
	//
	size_t sessionLen=m_test_session.length();
	if (sessionLen > sizeof(a.session)) sessionLen=sizeof(a.session);
	memcpy(a.session, m_test_session.c_str(), sessionLen);
	memset(a.seqno, ' ', sizeof(a.seqno));
	sprintf(a.seqno, "%20d", wk_channel.snapshot_seqno_1);
int ret = ::send(wk_channel.snapshot_skt, &a, sizeof(a), MSG_DONTWAIT);
	if (ret < 0)
	{
#ifdef CW_DEBUG
cout << __func__ << " failed " << strerror(errno) <<endl;
#endif
	}
	else {
cout << __func__ << "ch." << wk_channel.channel_id << " OK " << strerror(errno) <<endl;
		wk_channel.update_channel_state(snapshot_session::channel_state_types::accepted);
	}
}

void ASX_Test_Handler::send_reject(snapshot_session::channel_info& wk_channel)
{
	itch_session_packet::SoupBinTcpReject a;
	a.packet_type='J';
	a.reject_code='A';
	a.packet_length=htons(2);
	int ret = ::send(wk_channel.snapshot_skt.fd, &a, sizeof(a), MSG_DONTWAIT);
	if (ret < 0)
	{
#ifdef CW_DEBUG
cout << __func__ << " failed " << strerror(errno) <<endl;
#endif
	}

	
}

void ASX_Test_Handler::send_debug(snapshot_session::channel_info& wk_channel)
{
	itch_session_packet::SoupBinTcpDebug a;
	a.packet_type='+';
	char time_out="TIME OUT";
	char out_buf[sizeof(a)+sizeof(time_out)];
	a.packet_length=htons(1+sizeof(time_out));
	int ret = ::send(wk_channel.snapshot_skt.fd, &a, sizeof(a), MSG_DONTWAIT);
	if (ret < 0)
	{
#ifdef CW_DEBUG
cout << __func__ << " failed " << strerror(errno) <<endl;
#endif
	}
	
}

void ASX_Test_Handler::send_heartbeat(snapshot_session::channel_info& wk_channel)
{
	itch_session_packet::SoupBinTcpBeat a;
	a.packet_type='H';
	a.packet_length=htons(1);
	int ret = ::send(wk_channel.snapshot_skt.fd, &a, sizeof(a), MSG_DONTWAIT);
	if (ret < 0)
	{
#ifdef CW_DEBUG
cout << __func__ << " failed " << strerror(errno) <<endl;
#endif
	}
	wk_channel.last_sent_time=Time::current_time();
	
}

void ASX_Test_Handler::send_end_snapshot(snapshot_session::channel_info& wk_channel)
{
#ifdef CW_DEBUG
cout << __func__ << " seqno: " << wk_channel.next_mcast_packet <<endl;
#endif
	itch_session_packet::SoupBinTcpEndSnapshot a;
	a.packet_type='S';
	a.message_type='G';
	a.packet_length=htons(sizeof(itch_session_packet::SoupBinTcpEndSnapshot)-2);
	memset(a.itch_seqno, ' ', sizeof(a.itch_seqno));
	//sprintf(&a.itch_seqno, "%-d", wk_channel.next_out_seqno);
	sprintf(a.itch_seqno, "%20d", wk_channel.next_mcast_packet);
	int ret = ::send(wk_channel.snapshot_skt.fd, &a, sizeof(a), MSG_DONTWAIT);
	if (ret < 0)
	{
#ifdef CW_DEBUG
cout << __func__ << " failed " << strerror(errno) <<endl;
#endif
	wk_channel.update_channel_state(snapshot_session::channel_state_types::end_snapshot);
	return;
	}
	wk_channel.snapshot_last_seqno=-1;
	wk_channel.snapshot_next_send_time += seconds(10);
}

snapshot_session::channel_info* ASX_Test_Handler::get_channel_by_id(int id){
	for (size_t i=0; i< m_channels.size(); i++){ 
		if (id==m_channels[i].get()->channel_id)
		return m_channels[i].get();
	}
	return 0;
}

snapshot_session::channel_info* ASX_Test_Handler::get_channel_info(int fd){
	for (size_t i=0; i< m_channels.size(); i++){ 
		snapshot_session::channel_info* tmpCh=m_channels[i].get();
		if (fd==tmpCh->mcast_fd || 
				fd==tmpCh->udp_send_fd || 
				fd==tmpCh->udp_read_fd || 
				fd==tmpCh->snapshot_skt.fd ||
				fd==tmpCh->tcp_send_fd || 
				fd==tmpCh->tcp_read_fd)
		{
			tmpCh->fd=fd;
			return tmpCh; 
		}
	}
	return 0;
}

size_t ASX_Test_Handler:: process_client_data(int fd, const char* data, size_t l){
#ifdef CW_DEBUG
cout << __func__ << " for " << fd << " size " << l << endl;
#endif
	snapshot_session::channel_info* wk_channel=get_channel_info(fd);
	if (fd==0) wk_channel=m_channels[0].get();
	else
	wk_channel=get_channel_info(fd);
	printf("%s from %d on channel %u msg type=%c\n", __func__, fd, wk_channel->channel_id, data[2]);
	if (!wk_channel || wk_channel->fd != fd)return 0;
	wk_channel->last_recv_time= Time::current_time();
	const itch_session_packet::SoupBinTcpBeat* cd=reinterpret_cast<const itch_session_packet::SoupBinTcpBeat*>(data);
	uint16_t hLen=ntohs(cd->packet_length);
	if (l < hLen+2)return 0;
	size_t retLen=hLen+2;
	size_t typeInt=data[2];
	wk_channel->update_channel_state(snapshot_session::channel_state_types::connected);
	switch(typeInt){
		case size_t('L'): confirm_login(*wk_channel, data);
				send_snapshot_message(*wk_channel);
			  break;
		case size_t('O'):
			  //send_logout(*wk_channel);
			  //disconnect(wk_channel->fd);
printf("Client log-out closing tcp fd=%d\n", wk_channel->snapshot_skt.fd);
			  wk_channel->snapshot_skt.close();
			  wk_channel->snapshot_skt.fd=-1;
	wk_channel->update_channel_state(snapshot_session::channel_state_types::disconnected);
	//have to return -1 to indicate this socket is closed by me
			  retLen=UINT_MAX - EBADF ;
			  break;
		case size_t('R'): break;
		default:
			  break;
	}
	return retLen;
}

size_t ASX_Test_Handler::read(size_t channel_id, const char* data, size_t len)
{
	cout << Time::current_time().to_string() << __func__ << " len=" << len << endl;
	//if (len<2)return 0;
	int fd=0;
	size_t which1=-1;
	for (size_t c=0; c<m_channels.size(); c++)
	{
		if (m_channels[c].get()->channel_id != channel_id)
			continue;
		fd=m_channels[c].get()->tcp_read_fd;
		which1=c;
		break;
	}
	if (fd<=0) return 0;
	if (len==0) {
		to_write(fd);
		return 0;
	}
	if (len > UINT_MAX - 150){
		m_channels[which1].get()->tcp_read_fd=-1;
		return 0;
	}
	printf("%s got channel %d with fd=%d\n ", __func__, channel_id, fd);
	return process_client_data(fd, data, len);
}
void ASX_Test_Handler::check_time_out(){
	//channel_info* wk_channel=get_channel_info(fd);
	for (size_t i=0; i<m_channels.size(); i++){
		snapshot_session::channel_info* wk_channel=m_channels[i].get();
		if (wk_channel->last_recv_time < Time::current_time() + wk_channel->heartbeat_interval)
			continue;
		if (wk_channel->last_recv_time < Time(Time::current_time().ticks() + wk_channel->heartbeat_interval.ticks()*3))
				{
				send_debug(*wk_channel);
		wk_channel->last_sent_time=Time::current_time(); 
		}
		disconnect(*wk_channel);
	}
return ;	
}

void ASX_Test_Handler::disconnect(snapshot_session::channel_info& wk_channel){
	//TO-DO
	//need to update the Session so that it will handle mutiple connections
	//m_session->handle_disconnect();
}
bool ASX_Test_Handler::disconnect(int fd){
	//snapshot_session::channel_info* wk_channel=get_channel_info(fd);
	::close(fd);
	return true;
	//if (!wk_channel || wk_channel->fd != fd) return false;
	//disconnect(*wk_channel);
return true;	
}

void ASX_Test_Handler::load_user_info(const string& filename){

}
void  ASX_Test_Handler::add_user(string username, string password){
	m_user_info[username]= password;
}

  ASX_Test_Handler::ASX_Test_Handler(Application* app) :
    Snapshot_Handler(app, "ASX_Test_Handler"), my_name("ASX_Test_Handler")
  {
	memset(&m_soup_Login, ' ', sizeof(itch_session_packet::SoupBinTcpLogin));
	memcpy(&m_soup_Login.seqno, "1", 1);
	m_soup_Login.packet_type='L';
	m_soup_LogOut.packet_type='O';
	m_soup_Heartbeat.packet_type='R';
	m_soup_Debug.packet_type='+';
  }

  ASX_Test_Handler::ASX_Test_Handler(Application* app, const string& handler_type) :
    Snapshot_Handler(app, "ASX_Test_Handler"), my_name("ASX_Test_Handler")
  {
memset(&m_soup_Login, ' ', sizeof(itch_session_packet::SoupBinTcpLogin));
	memcpy(&m_soup_Login.seqno, "1", 1);
	m_soup_Login.packet_type='L';
	m_soup_LogOut.packet_type='O';
	m_soup_Heartbeat.packet_type='R';
	m_soup_Debug.packet_type='+';
  }

  ASX_Test_Handler::~ASX_Test_Handler()
  {
  }

}
