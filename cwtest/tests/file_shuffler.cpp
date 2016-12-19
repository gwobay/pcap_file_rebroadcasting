#include <map>
#include <set>
#include <list>
#include <cmath>
#include <ctime>
#include <deque>
#include <queue>
#include <stack>
#include <string>
#include <bitset>
#include <cstdio>
#include <limits>
#include <vector>
#include <climits>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <sys/time.h>
#include <tbb/compat/thread>
#include <tbb/tbb_allocator.h> // zero_allocator defined here
#include <tbb/atomic.h>
#include <tbb/concurrent_vector.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <endian.h>

#include <utility>
#include <pcap.h>


using namespace std;

static unordered_map<string, char> order_id_map;

struct pcap_file_header {
	uint32_t magic;
	uint16_t version_major;
	uint16_t version_minor;
	int32_t  thiszone;     /* gmt to local correction */
	uint32_t sigfigs;    /* accuracy of timestamps */
	uint32_t snaplen;    /* max length saved portion of each pkt */
	uint32_t linktype;   /* data link type (LINKTYPE_*) */
};

struct shuffle_record
{
	tbb::atomic<bool> not_processed;
	struct timeval ts;
	const u_char *packet;
	unsigned int capture_len;
};

static uint64_t ntohll(uint64_t v) { return __builtin_bswap64(v);  }

/* UDP header struct 
* you might want to look at netinet/tcp.h for hints
* on how to deal with single bits or fields that are smaller than a byte
* in length.
* */

class pcap_parser{
	
public:

struct UDP_hdr {
	u_short uh_sport; // source port *UDP_hdr/
	u_short uh_dport;
	u_short uh_ulen;
	u_short uh_sum;
};

struct UTP_header_UTP_1_1 {
    char category;
    char type;
	char session_identifier;
	char retression_requester[2];
	char seq_no[8];
	char market_center_originator_id;
} __attribute__ ((packed));

struct UTP_header_MoldUdp64 {
	char      session[10];
	uint64_t  seq_no;
	uint16_t  message_count;
} __attribute__ ((packed));

struct message_protocol_name_tags {
	string name;
	uint16_t tag;
};

#define EXT 003
#define US 037
#define UTP_1_1 1
#define MOLDUDP64 2
static struct message_protocol_name_tags MESSAGE_PROTOCOLs[];
uint64_t m_seqno;
uint16_t m_working_protocol;

std::pair<uint64_t, uint16_t>  get_seq_no(const u_char* buf)
{
	std::pair<uint64_t, uint16_t> retPair;
	uint64_t seq_no=0;
	uint16_t msg_count=0;
	switch (m_working_protocol){
		case UTP_1_1:
			{
			struct UTP_header_UTP_1_1 aUTP_1_1;
			bool found_ext;
			const char *pLoc=buf;
			if (strlen(pLoc) < sizeof(aUTP_1_1)) return retPair;
			memcpy(&aUTP_1_1, ++pLoc, sizeof(aUTP_1_1));
			char seqno[9];
			memcpy(seqno, aUTP_1_1.seq_no, 8);
			seqno[8]=0;
			seq_no=atoll(seqno);
			//+1 for char SOH_delimiter;
			while (*pLoc != EXT){
				//memcpy(&aUTP_1_1, pLoc, sizeof(aUTP_1_1));
				if (strlen(pLoc) < sizeof(aUTP_1_1)) break;
				pLoc += sizeof(aUTP_1_1);
				msg_count++;
				while (*pLoc != 037 && *pLoc != EXT) ++pLoc;
			}
			if (aUTP_1_1.category=='C'){
				msg_count=0;
				if (aUTP_1_1.type=='I') seq_no=0; 
			}
			retPair.first=seq_no;
			retPair.second=msg_count;
			}
			break;
		case MOLDUDP64:
			{
			struct UTP_header_MoldUdp64 aMoldUdp64;
			struct UTP_header_MoldUdp64* header = reinterpret_cast<const struct UTP_header_MoldUdp64*>(buf);
			aMoldUdp64=*header;
			//memcpy(&aMoldUdp64, buf, sizeof(aMoldUdp64));
			retPair=std::make_pair<uint64_t, uint16_t>(ntohll(aMoldUdp64.seq_no), (uint16_t)(ntohs(aMoldUdp64.message_count)));
			}
			break;
		default:
			break;
	}
	return retPair;
}
/* Some helper functions, which we define at the end of this file. */

/* Returns a string representation of a timestamp. */
const char *timestamp_string(struct timeval ts);

/* Report a problem with dumping the packet with the given timestamp. */
void problem_pkt(struct timeval ts, const char *reason);

/* Report the specific problem of a packet being too short. */
void too_short(struct timeval ts, const char *truncated_hdr);

/* dump_UDP_packet()
*
* This routine parses a packet, expecting Ethernet, IP, and UDP headers.
* It extracts the UDP source and destination port numbers along with the UDP
* packet length by casting structs over a pointer that we move through
* the packet.  We can do this sort of casting safely because libpcap
* guarantees that the pointer will be aligned.
*
* The "ts" argument is the timestamp associated with the packet.
*
* Note that "capture_len" is the length of the packet *as captured by the
* tracing program*, and thus might be less than the full length of the
* packet.  However, the packet pointer only holds that much data, so
* we have to be careful not to read beyond it.
*/

vector<uint64_t> missing_seqno;
size_t m_missing_begin_index;
vector<uint64_t> found_filling_seqno;

	      
void dump_UDP_packet(const u_char *in_packet, struct timeval ts,
	      unsigned int capture_len)
{
struct ip *ip;
struct UDP_hdr *udp;
unsigned int IP_header_length;

const u_char *packet=in_packet;
	if (capture_len < sizeof(struct ether_header))
	{
	  too_short(ts, "Ethernet header");
	  return;
	}

	/* Skip over the Ethernet header. */
	packet += sizeof(struct ether_header);
	capture_len -= sizeof(struct ether_header);

	if (capture_len < sizeof(struct ip))
	{ /* Didn't capture a full IP header */
		too_short(ts, "IP header");
		return;
	}

	ip = (struct ip*) packet;
	IP_header_length = ip->ip_hl * 4;/* ip_hl is in 4-byte words */

	if (capture_len < IP_header_length)
	{ /* didn't capture the full IP header including options */
			too_short(ts, "IP header with options");
			return;
	}

	if (ip->ip_p != IPPROTO_UDP)
	{
			problem_pkt(ts, "non-UDP packet");
			return;
	}

	/* Skip over the IP header to get to the UDP header. */
	packet += IP_header_length;
	capture_len -= IP_header_length;

	if (capture_len < sizeof(struct UDP_hdr))
	{
		too_short(ts, "UDP header");
		//recorded
			return ;
	}

	udp = (struct UDP_hdr*) packet;
	capture_len -= sizeof(struct UDP_hdr);
	printf("%s UDP s_port=%d d_port=%d length=%d dataLen=%d \n",
			timestamp_string(ts),
			ntohs(udp->uh_sport),
			ntohs(udp->uh_dport),
			ntohs(udp->uh_ulen),
			capture_len);

	packet += sizeof(struct UDP_hdr);

	std::pair<uint64_t, uint16_t> seqno_pair=get_seq_no(packet);

	printf("\t\t==>seq_no:%lu, expeced:%lu msg_count:%u\n", seqno_pair.first, m_seqno, seqno_pair.second);
	if (seqno_pair.first > m_seqno){
		for (uint64_t k=m_seqno; k<seqno_pair.first; k++)
		missing_seqno.push(k);//m_seqno);
	}
	m_seqno=seqno_pair.first+seqno_pair.second;
	m_record.ts=ts;
	m_record.packet=in_packet;
	m_record.capture_len=capture_len;
	m_record.not_processed=true;
	while (m_record.not_processed){
		//u_sleep(?);
		//wakeup by main thread??
	}
}

string m_base_directory_name;
string m_first_directory_name;
string m_last_directory_name;
string m_current_directory_name;
string m_channel_file_name;
string m_B_channel_file_name;
uint16_t m_time_diff_minutes;
uint8_t m_channel_id;
struct shuffle_record& m_record;

string get_next_directory_name(string current_directory_name){
	string directory=dirname(current_directory_name);
	directory += '/';
	string time_name=basename(current_directory_name.c_str());
	uint16_t base_time=stol(time_name);
	uint16_t new_min=(base_time/100*60+base_time%100+m_time_diff_minutes);
	char buf[5];
	std::printf(buf, "%02u%02u", dirname(current_directory_name, )new_min/60, new_min % 60);
	return directory+buf;
}

pcap_parser(const string& protocol_name
		, const string& base_directory_name
		, const string& first_directory_name
		, const string& last_directory_name
		, uint16_t time_diff_minutes
		, const string& channel_file_name
		, const string& B_channel_file_name
		, uint8_t channel_id
		, struct shuffle_record& record
		)
{
	unordered_map<string, uint16_t> protocol_map;
	for (struct message_protocol_name_tags& a:MESSAGE_PROTOCOLs){
		protocol_map[a.name]=a.tag;
	}
	for (unordered_map<string, uint16_t>::iterator it=protocol_map.begin(); it != protocol_map.end(); it++){
		std::cout << (*it).first << " " << (*it).second << std::endl;
	}
	m_working_protocol=protocol_map[protocol_name.c_str()];
	if (m_working_protocol < 1){
		fprintf(stderr, "wrong protocol %s \n", protocol_name.c_str());
                exit(1);
	}
	m_base_directory_name( base_directory_name);
	m_first_directory_name= first_directory_name;
	m_current_directory_name=first_directory_name;
	m_last_directory_name= last_directory_name;
	m_time_diff_minutes=time_diff_minutes;
	m_channel_file_name=channel_file_name;
	m_B_channel_file_name=B_channel_file_name;
	m_channel_id = channel_id;
	m_record = record;
}

void start()
{
pcap_t *pcap;
const u_char *packet;
char errbuf[PCAP_ERRBUF_SIZE];
struct pcap_pkthdr header;

vector<sting> dirs;
	do{
		dirs.push(m_base_directory_name+"/"+m_current_directory_name+"/"+m_channel_file_name);
		m_current_directory_name=get_next_directory_name(m_current_directory_name);
		
	} while (stol(m_current_directory_name)<=stol(m_last_directory_name));

	for (uint8_t i=0; i<dirs.size(); i++){
		m_missing_begin_index=missing_seqno.size();
		
		pcap = pcap_open_offline(dirs[i].c_str(), errbuf);
		if (pcap == NULL)
		{
			fprintf(stderr, "error reading pcap file: %s\n", errbuf);
			exit(1);
		}

/* Now just loop through extracting packets as long as we have
* some to read.
*/
		int k=0;
		while ((packet = pcap_next(pcap, &header)) != NULL)
		{
			dump_UDP_packet(packet, header.ts, header.caplen);
		}
		for (size_t iGap=m_missing_begin_index; iGap < missing_seqno.size(); iGap++){
			//
		//  check the gap in B file
		//  and insert the found_filling_seqno
		}
	}
	
// terminate
	return 0;
}


/* Note, this routine returns a pointer into a 		static buffer, and
* so each call overwrites the value returned by the previous call.
*/
const char *timestamp_string(struct timeval ts)
{
static char timestamp_string_buf[256];

sprintf(timestamp_string_buf, "%d.%06d", (int) ts.tv_sec, (int) ts.tv_usec);

return timestamp_string_buf;

}

void problem_pkt(struct timeval ts, const char *reason)
{
fprintf(stderr, "%s: %s\fprintfn", timestamp_string(ts), reason);
}

void too_short(struct timeval ts, const char *truncated_hdr)
{
fprintf(stderr, "packet with timestamp_string_buf %s is truncated and lacks a full %s\n", timestamp_string(ts), truncated_hdr);
}
};

pcap_parser::MESSAGE_PROTOCOLs[]={
	{"NONE", 0}
	,{"UTP_1_1", UTP_1_1}
	,{"MoldUdp64", MOLDUDP64}
};

static vector<struct shuffle_record> provider_list;

int main(int argc, char** argv) 
{
	Parameter aParam;
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
	char cfgbuf[640];
	while(fgets(cfgbuf, 640, cfg)){
	    char *pB=strchr(cfgbuf, '#'); 
	    if (pB) *pB=0;
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
	create_mdcache_param( mdmap, cfg);
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
		  saveData.setBool(val[2]=='u'?true:false);
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
		fprintf(stderr, "%s missing keyword data_file_name or user_name or password \n", argv[1]);
		                  exit(EXIT_FAILURE);
	}
	total_channels=stoi(aParam["total_channels"].getString());
	int listen_fds[total_channels];
	tcp_listen_fds=listen_fds;

	for (size_t i=0; i<total_channels; i++){
	

		//munmap(turn_on, sizeof *turn_on); 
		_exit(EXIT_SUCCESS);
		exit(EXIT_SUCCESS);
	}
}


