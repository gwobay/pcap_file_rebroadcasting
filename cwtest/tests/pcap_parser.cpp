#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <iostream>
#include <unordered_map>

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <endian.h>

#include <utility>
#include <pcap.h>


/* UDP header struct 
* you might want to look at netinet/tcp.h for hints
* on how to deal with single bits or fields that are smaller than a byte
* in length.
* */

using namespace std;
struct UDP_hdr {
	u_short uh_sport; // source port *UDP_hdr/
	u_short uh_dport;
	u_short uh_ulen;
	u_short uh_sum;
};

static uint64_t expecting_no=0;

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
static struct message_protocol_name_tags MESSAGE_PROTOCOLs[]={
	{"NONE", 0}
	,{"UTP_1_1", UTP_1_1}
	,{"MoldUdp64", MOLDUDP64}
};

static uint64_t ntohll(uint64_t v) { return __builtin_bswap64(v);  }
static uint16_t working_protocol=0;
static std::pair<uint64_t, uint16_t>  get_seq_no(const u_char* buf)
{
	std::pair<uint64_t, uint16_t> retPair;
	uint64_t seq_no=0;
	uint16_t msg_count=0;
	switch (working_protocol){
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
	      
void dump_UDP_packet(const u_char *packet, struct timeval ts,
	      unsigned int capture_len)
{
struct ip *ip;
struct UDP_hdr *udp;
unsigned int IP_header_length;

/* For simplicity, we assume Ethernet encapsulation. */
	
	if (capture_len < sizeof(struct ether_header))
	{
	/* failed even capture a full Ethernet header, so we
	 * can't analyze this any further.
	  */
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

	printf("\t\t==>seq_no:%lu, expeced:%lu msg_count:%u\n", seqno_pair.first, expecting_no, seqno_pair.second);
	expecting_no=seqno_pair.first+seqno_pair.second;
}

int main(int argc, char *argv[])
{
pcap_t *pcap;
const u_char *packet;
char errbuf[PCAP_ERRBUF_SIZE];
struct pcap_pkthdr header;

/* Skip over the program name. */
	++argv; --argc;

/* We expect exactly one argument, the name of the file to dump. */
	if ( argc != 2  )
	{
		fprintf(stderr, "program requires two argument, the trace file to dump and protocol to parse\n");
		exit(1);
	}

	unordered_map<string, uint16_t> protocol_map;
	for (struct message_protocol_name_tags& a:MESSAGE_PROTOCOLs){
		protocol_map[a.name]=a.tag;
	}
	for (unordered_map<string, uint16_t>::iterator it=protocol_map.begin(); it != protocol_map.end(); it++){
		std::cout << (*it).first << " " << (*it).second << std::endl;
	}
	working_protocol=protocol_map[argv[0]];
	if (working_protocol < 1){
		fprintf(stderr, "wrong protocol %s \n", argv[1]);
                exit(1);
	}
	pcap = pcap_open_offline(argv[1], errbuf);
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
		k++;	
		if (k > 10) break;
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

