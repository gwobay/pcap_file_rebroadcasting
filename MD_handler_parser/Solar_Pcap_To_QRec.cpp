#include "Solar_Pcap_To_QRec.h"
#include <Data/QRec.h>

#include <fstream>

namespace hf_core {
  using namespace std;

  #pragma pack(push,1)
  struct PcapFileHeader
  {
    unsigned int magic_number;
    unsigned short version_major;
    unsigned short version_minor;
    int thiszone;
    unsigned int sigfigs;
    unsigned int snaplen;
    unsigned int network;
  };
  
  struct PcapHeader
  {
    unsigned int epoch_seconds;
    unsigned int epoch_nanoseconds;
    unsigned int caplen;
    unsigned int len;
  };
  
  struct IpAddress
  {
    char byte1;
    char byte2;
    char byte3;
    char byte4;
  };
  
  struct EthernetHeader
  {
    char opaque[18];
  };
  
  struct IpHeader
  {
    unsigned char ver_ihl;
    unsigned char tos;
    unsigned short tlen;
    unsigned short identification;
    unsigned short flags_fo;
    unsigned char ttl;
    unsigned char proto;
    unsigned short crc;
    IpAddress source_address;
    IpAddress dest_address;
    //unsigned int op_pad;
  };
  
  struct UdpHeader
  {
    unsigned short source_port;
    unsigned short dest_port;
    unsigned short udp_length;
    unsigned short udp_checksum;
  };
  #pragma pack(pop)

  Solar_Pcap_To_QRec::Solar_Pcap_To_QRec()
  {
  }

  Solar_Pcap_To_QRec::~Solar_Pcap_To_QRec()
  {
    for(hash_map<string, ofstream*>::iterator it = m_file_name_to_handle.begin(); it != m_file_name_to_handle.end(); ++it)
    {
      ofstream* fileOut = it->second;
      fileOut->close();
    }
  }

  void Solar_Pcap_To_QRec::init(const string& input_file, const string& output_dir, const string& date_str)
  {
#ifdef CW_DEBUG
    cout << "input_file=" << input_file << endl;
#endif
    m_input_file = input_file;
    m_output_dir = output_dir;
    m_date_str = date_str;

#ifdef CW_DEBUG
    cout << "m_date_str=" << m_date_str << endl;
#endif
    Time startOfDay(m_date_str + " 00:00:00", "%Y%m%d %H:%M:%S");
    m_sod_ticks = static_cast<uint64_t>(startOfDay.ticks());
#ifdef CW_DEBUG
    cout << "m_sod_ticks=" << m_sod_ticks << endl;
#endif
  }

#define LOGV(x) if(false) { cout << x << endl; }

  uint64_t mc_group_str_to_key(const string& mc_group, uint16_t mc_port)
  {
    uint64_t byte1, byte2, byte3, byte4;
    char dot;
    istringstream s(mc_group);
    s >> byte1 >> dot >> byte2 >> dot >> byte3 >> dot >> byte4;
#ifdef CW_DEBUG
    cout << "byte1=" << byte1 << " byte2=" << byte2 << " byte3=" << byte3 << " byte4=" << byte4 << endl;
#endif
    uint64_t key = byte1 << 56 |
                   byte2 << 48 |
                   byte3 << 40 |
                   byte4 << 32 |
                   static_cast<uint64_t>(mc_port);
#ifdef CW_DEBUG
    cout << "key=" << key << endl;
#endif
    return key;
  }

  uint64_t mc_group_to_key(const uint32_t mc_group, uint16_t mc_port)
  {
    LOGV("mc_group=" << mc_group << " mc_port=" << mc_port)
    uint64_t key = static_cast<uint64_t>(mc_group) << 32 |
                   static_cast<uint64_t>(mc_port);
    LOGV("key=" << key)
    return key;
  }

  void Solar_Pcap_To_QRec::map_channel(const string& mc_group, uint16_t mc_port, const string& channel_file_name)
  {
    uint64_t mcKey = mc_group_str_to_key(mc_group, mc_port);
    hash_map<uint64_t, Channel>::iterator it = m_port_to_channel.find(mcKey);
    if (it != m_port_to_channel.end())
    {
#ifdef CW_DEBUG
      cout << "Tried to configure the same port (" << mc_port << ") twice!" << endl;
#endif
      throw new exception();
    }

    hash_map<string, ofstream*>::iterator fileIt = m_file_name_to_handle.find(channel_file_name);
    if(fileIt == m_file_name_to_handle.end())
    {
      ofstream* fileOut = new ofstream();
      // ToDo: combine m_output_dir with channel_file_name properly
      string channel_file_path = m_output_dir + "/" + m_date_str + "." + channel_file_name + ".bin";
#ifdef CW_DEBUG
      cout << "channel_file_path=" << channel_file_path << endl;
#endif
      fileOut->open(channel_file_path.c_str(), ios::out | ios::binary);
      m_file_name_to_handle[channel_file_name] = fileOut;
      fileIt = m_file_name_to_handle.find(channel_file_name);
      m_file_name_to_context[channel_file_name] = 0ULL;
    }
    ofstream* fOut = fileIt->second;

    uint64_t context = m_file_name_to_context[channel_file_name];
    ++m_file_name_to_context[channel_file_name];

    m_port_to_channel.insert(make_pair(mcKey, Channel(fOut, context)));
#ifdef CW_DEBUG
    cout << "Mapped group " << mc_group << " port " << mc_port << " -> " << channel_file_name << endl;
#endif
  }

  void Solar_Pcap_To_QRec::run()
  {
    LOGV("run")
    //return;

    char bufferMem[1024*1024];
    char *buffer = &(bufferMem[0]);
    ifstream inFile(m_input_file.c_str(), ios::in | ios::binary);

    if(inFile)
    {
      inFile.read(buffer, sizeof(PcapFileHeader));
    }
    LOGV("sizeof(PcapFileHeader) = " << sizeof(PcapFileHeader))
    const PcapFileHeader& pcapFileHeader = *(reinterpret_cast<PcapFileHeader*>(buffer));
    LOGV("magic_number = " << pcapFileHeader.magic_number)
    LOGV("version_major = " << pcapFileHeader.version_minor)
    LOGV("version_minor = " << pcapFileHeader.version_major)

    unsigned int numPackets(0);
    while(inFile)
    {
      unsigned bytesReadThisPacket(0);
      ++numPackets;
/*
      if(numPackets >= 10000)
      {
         break; // just for testing
      }
*/
      LOGV("*************** packet " << numPackets << " ***************")

      if(!inFile.read(buffer, sizeof(PcapHeader)))
      {
         LOGV("Break A: didn't read enough for PcapHeader")
         break;
      }
      bytesReadThisPacket += sizeof(PcapHeader);

      LOGV(" ")
      LOGV("sizeof(PcapHeader) = " << sizeof(PcapHeader))
      const PcapHeader& pcapHeader = *(reinterpret_cast<PcapHeader*>(buffer));
      LOGV("pcapHeader.epoch_seconds=" << pcapHeader.epoch_seconds)
      LOGV("pcapHeader.epoch_nanoseconds=" << pcapHeader.epoch_nanoseconds)
      LOGV("pcapHeader.caplen=" << pcapHeader.caplen)
      unsigned pcapHeaderLen = pcapHeader.len;
      LOGV("pcapHeader.len=" << pcapHeaderLen)
      uint64_t epochTimestampNs = static_cast<uint64_t>(pcapHeader.epoch_seconds)*1000000000ull + static_cast<uint64_t>(pcapHeader.epoch_nanoseconds);
      uint64_t timestampNs = epochTimestampNs;// - m_sod_ticks; 
      LOGV("timstampNs=" << timestampNs);

      if(!inFile.read(buffer, sizeof(EthernetHeader)))
      {
         break;
      }
      bytesReadThisPacket += sizeof(EthernetHeader);

      LOGV(" ")
      LOGV("sizeof(EthernetHeader) = " << sizeof(EthernetHeader))

      if(!inFile.read(buffer, sizeof(IpHeader)))
      {
         LOGV("Break B: didn't read enough for IpHeader")
         break;
      }
      bytesReadThisPacket += sizeof(IpHeader);

      LOGV(" ")
      LOGV("sizeof(IpHeader) = " << sizeof(IpHeader))
      const IpHeader& ipHeader = *(reinterpret_cast<IpHeader*>(buffer));
      LOGV("ipHeader.tlen = " << ntohs(ipHeader.tlen))
      LOGV("ipHeader.proto = " << ipHeader.proto)
      LOGV("ip_dest = " << (static_cast<unsigned>(ipHeader.dest_address.byte1) & 0x000000ff)
                    << "." << (static_cast<unsigned>(ipHeader.dest_address.byte2) & 0x000000ff)
                    << "." << (static_cast<unsigned>(ipHeader.dest_address.byte3) & 0x000000ff)
                    << "." << (static_cast<unsigned>(ipHeader.dest_address.byte4) & 0x000000ff))
      uint32_t ipDestAddress = ntohl(*reinterpret_cast<const uint32_t*>(&(ipHeader.dest_address)));
      LOGV("static_cast<unsigned>(ipHeader.proto) = " << static_cast<unsigned>(ipHeader.proto))
      unsigned int ipLen = ((ipHeader.ver_ihl & 0xf) * 4);
      LOGV("ipLen = " << ipLen)
      unsigned int ver = ((ipHeader.ver_ihl >> 4) & 0xf);
      LOGV("ver = " << ver)

      if (ipHeader.proto != 17)
      {
          int remainingLength = static_cast<int>(pcapHeaderLen) + static_cast<int>(sizeof(PcapHeader)) - static_cast<int>(bytesReadThisPacket);
          if(!inFile.read(buffer, remainingLength))
          {
             LOGV("Break C: didn't read enough for remainingLength")
             break;
          }
          continue;
      }

      unsigned int ipOptionsSize = (ipLen - sizeof(IpHeader));
      if(ipOptionsSize > 0)
      {
        LOGV("IP options size=" << ipOptionsSize)
        if(!inFile.read(buffer, ipOptionsSize))
        {
          LOGV("Break C.5: didn't read enough for optional IP padding")
          break;
        }
        bytesReadThisPacket += ipOptionsSize;
      }


      if(!inFile.read(buffer, sizeof(UdpHeader)))
      {
         LOGV("Break D: didn't read enough for UdpHeader")
         break;
      }
      bytesReadThisPacket += sizeof(UdpHeader);

      LOGV(" ")
      const UdpHeader& udpHeader = *(reinterpret_cast<UdpHeader*>(buffer));
      uint16_t destPort = ntohs(udpHeader.dest_port);
      LOGV("dest_port = " << destPort)
      unsigned udpPayloadLength = ntohs(udpHeader.udp_length) - sizeof(UdpHeader);
      LOGV("udp_length = " << udpPayloadLength)

      if(bytesReadThisPacket + udpPayloadLength > pcapHeaderLen + sizeof(PcapHeader))
      {
#ifdef CW_DEBUG
        cout << "Truncated packet " << endl;
#endif
        unsigned bytesToRead = pcapHeaderLen + sizeof(PcapHeader) - bytesReadThisPacket;
#ifdef CW_DEBUG
        cout << "Reading " << bytesToRead << " bytes to reach end of packet" << endl;
#endif
        if(!inFile.read(buffer, bytesToRead))
        {
          LOGV("Break D.5");
          break;
        }
        continue;
      }
      if(!inFile.read(buffer, udpPayloadLength))
      {
         LOGV("Break E: didn't read enough for UdpPayload")
         break;
      }
      bytesReadThisPacket += udpPayloadLength;
      LOGV("bytesReadThisPacket = " << bytesReadThisPacket)
      // Here, can get seq nums

      const uint64_t mcKey = mc_group_to_key(ipDestAddress, destPort);
      //const uint64_t mcKey = mc_group_to_key(ipHeader.dest_address, destPort);
      hash_map<uint64_t, Channel>::iterator it = m_port_to_channel.find(mcKey);
      if(it != m_port_to_channel.end())
      {
        LOGV("Channel found for port " << destPort)
        Channel& channel = it->second;
        uint64_t context(channel.context); 
        record_header qrecHeader(Time(timestampNs), context, udpPayloadLength);
        channel.file_out->write(reinterpret_cast<char*>(&qrecHeader), sizeof(qrecHeader));
        channel.file_out->write(buffer, udpPayloadLength);
      }
      else
      {
#ifdef CW_DEBUG
        cout << "Port " << destPort << " not found in map" << endl;
#endif
      }
  

      // some footer
      int footerLength = static_cast<int>(pcapHeaderLen) + static_cast<int>(sizeof(PcapHeader)) - static_cast<int>(bytesReadThisPacket);
      LOGV("footerLength = " << footerLength)
      if(footerLength >0)
      {
        if(!inFile.read(buffer, footerLength))
        {
           LOGV("Break F: didn't read enough for Footer")
           break;
        }
      }
    }
  }

}
