#include <Data/PCAP_Parser.h>

#include <Util/except.h>

#include <Util/File_Utils.h>

#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>


namespace hf_core
{
  using namespace hf_tools;

  struct pcap_file_header {
    uint32_t magic;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t  thiszone;     /* gmt to local correction */
    uint32_t sigfigs;    /* accuracy of timestamps */
    uint32_t snaplen;    /* max length saved portion of each pkt */
    uint32_t linktype;   /* data link type (LINKTYPE_*) */
  };

  struct pcap_timeval {
    int32_t tv_sec;
    int32_t tv_usec;
  };

  struct pcap_pkthdr {
    pcap_timeval ts;     /* time stamp */
    uint32_t caplen;     /* length of portion present */
    uint32_t len;        /* length this packet (off wire) */
  };
  
  struct vlan_tag {
    u_int16_t       vlan_tpid;              /* ETH_P_8021Q */
    u_int16_t       vlan_tci;               /* VLAN TCI */
  };
  
  #define VLAN_TAG_LEN    4

  void
  PCAP_Parser::read(const string& filename, Handler* handler)
  {
    if(has_gzip_extension(filename)) {
      string file_cmd("/bin/zcat " + filename);
      m_file = popen(file_cmd.c_str(), "r");
      m_used_popen = true;
    } else if(has_rar_extension(filename)) {
      string file_cmd = "";
      if (m_inside_file_name.empty()) {
        file_cmd = "/tools/unrar/redhat/x_64/5.00/bin/unrar p -inul " + filename;
      } else {
        file_cmd = "/tools/unrar/redhat/x_64/5.00/bin/unrar p -inul " + filename + " " + m_inside_file_name;
      }
#ifdef CW_DEBUG
      printf("unrar command = %s\n", file_cmd.c_str());
#endif
      m_file = popen(file_cmd.c_str(), "r");
      m_used_popen = true;
    } else if(has_xz_extension(filename)) {
      string file_cmd("/usr/bin/xzcat " + filename);
      m_file = popen(file_cmd.c_str(), "r");
      m_used_popen = true;
    } else {
      m_file = fopen(filename.c_str(), "r");
    }

    if(!m_file) {
      throw errno_exception("PCAP_Parser::read: error opening file '" + filename + "'", errno);
    }

    char* buffer = new char[m_buf_size];

    ::fread(buffer, sizeof(pcap_file_header), 1, m_file);

    const pcap_file_header& header = reinterpret_cast<const pcap_file_header&>(*buffer);

    const uint32_t TCPDUMP_MAGIC      =     0xa1b2c3d4;
    const uint32_t NSEC_TCPDUMP_MAGIC =     0xa1b23c4d;

#ifdef CW_DEBUG
    printf("version_major = %d\n", header.version_major);
#endif
#ifdef CW_DEBUG
    printf("version_minor = %d\n", header.version_minor);
#endif
#ifdef CW_DEBUG
    printf("thiszone      = %d\n", header.thiszone);
#endif
#ifdef CW_DEBUG
    printf("sigfigs       = %d\n", header.sigfigs);
#endif
#ifdef CW_DEBUG
    printf("snaplen       = %d\n", header.snaplen);
#endif
#ifdef CW_DEBUG
    printf("linktype      = %d\n", header.linktype);
#endif
    fflush(stdout);

    if(header.magic == TCPDUMP_MAGIC) {
      m_nanoseconds_mode = false;
    } else if(header.magic == NSEC_TCPDUMP_MAGIC) {
      m_nanoseconds_mode = true;
    } else {
      throw runtime_error("PCAP_Parser::read: unknown file format (invalid magic number)  '" + filename + "'");
    }


    size_t parsed_len = 0;
    size_t bytes_to_parse = 0;

    bool read_more = true;
    while(read_more) {
      size_t unparsed_len = bytes_to_parse - parsed_len;
      if (parsed_len > 0) {
        // Move leftover bytes to beginning of buffer, fill remaining buffer
        memmove(buffer, buffer+parsed_len, unparsed_len);
        bytes_to_parse = fread(buffer+unparsed_len, 1, m_buf_size - unparsed_len, m_file);
        if (bytes_to_parse < (m_buf_size-unparsed_len))
          read_more = false;
        parsed_len = 0;
        bytes_to_parse += unparsed_len;
      } else {
        bytes_to_parse = fread(buffer, 1, m_buf_size, m_file);
        if (bytes_to_parse < m_buf_size)
          read_more = false;
      }

      while((parsed_len + sizeof(pcap_pkthdr)) < bytes_to_parse) {
        const pcap_pkthdr& pkt_header = reinterpret_cast<const pcap_pkthdr&>(*(buffer + parsed_len));
        if((parsed_len + sizeof(pcap_pkthdr) + pkt_header.caplen) < bytes_to_parse) {
          const char* eth_header_buf = buffer + parsed_len + sizeof(pcap_pkthdr);
          const ether_header& eth_header = reinterpret_cast<const ether_header&>(*eth_header_buf);
          if(eth_header.ether_type == htons(ETHERTYPE_IP)) {
            const char* ip_header_buf = eth_header_buf + sizeof(ether_header);
            const ip& ip_header = reinterpret_cast<const ip&>(*ip_header_buf);

            int header_len = ip_header.ip_hl * 4;

            if(ip_header.ip_p == IPPROTO_UDP) {
              const char* udp_header_buf = ip_header_buf + header_len;
              const udphdr& udp_header = reinterpret_cast<const udphdr&>(*udp_header_buf);

              uint16_t pkt_len = ntohs(udp_header.len) - sizeof(udphdr);
              const char* data = udp_header_buf + sizeof(udphdr);

              size_t context = 0;

              uint64_t port_ip = (uint64_t)udp_header.source << 32;
              port_ip |= ip_header.ip_src.s_addr;

              hash_map<uint64_t, size_t>::const_iterator i = m_ip_to_context.find(port_ip);
              if(i == m_ip_to_context.end()) {
                context = m_ip_to_context.size();
                m_ip_to_context.insert(make_pair(port_ip, context));
              } else {
                context = i->second;
              }

              uint64_t ns_latest = pkt_header.ts.tv_sec * Time_Constants::ticks_per_second;
              if(m_nanoseconds_mode) {
                ns_latest += pkt_header.ts.tv_usec;
              } else {
                ns_latest += pkt_header.ts.tv_usec * 1000;
              }

              //handler->set_ms_latest(ms_latest);
              Time::set_current_time(Time(ns_latest));
              handler->parse(context, data, pkt_len);
            } else if(ip_header.ip_p == IPPROTO_TCP) {
              //handler->read(context, data, pkt_len);
            }
          }
          parsed_len += sizeof(pcap_pkthdr) + pkt_header.caplen;
        } else {
          break;
        }
      }
    }
  }

  void
  PCAP_Parser::set_inside_file_name(const string& filename) {
    m_inside_file_name = filename;

  }

  PCAP_Parser::PCAP_Parser() :
    m_buf_size(1 * 1024 * 1024),
    m_file(0),
    m_used_popen(false),
    m_nanoseconds_mode(false),
    m_inside_file_name("")
  {
  }

  PCAP_Parser::~PCAP_Parser()
  {
    if(m_file) {
      if(m_used_popen) {
        pclose(m_file);
      } else {
        fclose(m_file);
      }
      m_file = 0;
    }

  }


}
