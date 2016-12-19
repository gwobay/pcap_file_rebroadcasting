#!/tools/python/redhat/x_64/2.6.1/bin/python

import sys
import os
import re
from optparse import OptionParser

sys.path.append ( '/q/common/exec/lib')

import warnings
with warnings.catch_warnings():
    warnings.filterwarnings("ignore", category=DeprecationWarning)
    import scipy.weave

def getCodeTemplate():
    supportCode = """  

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
   unsigned int epoch_microseconds;
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


    """

    code = """

   using namespace std;

   char buffer[1024*1024];
   string filePath("$FILE_PATH");
   cout << "filePath = '" << filePath << "'" << endl;
   ifstream inFile(filePath.c_str(), ios::in | ios::binary);

   if(inFile)
   {
       inFile.read(buffer, sizeof(PcapFileHeader));
   }
   cout << "sizeof(PcapFileHeader) = " << sizeof(PcapFileHeader) << endl;
   const PcapFileHeader& pcapFileHeader = *(reinterpret_cast<PcapFileHeader*>(buffer));
   cout << "magic_number = " << pcapFileHeader.magic_number << endl; 
   cout << "version_major = " << pcapFileHeader.version_minor << endl; 
   cout << "version_minor = " << pcapFileHeader.version_major << endl; 

   unsigned int numPackets(0);
   while(inFile)
   {
      unsigned bytesReadThisPacket(0);
      ++numPackets;
      if(numPackets >= 50)
      {
         break; // just for testing
      }
      cout << "*************** packet " << numPackets << " ***************" << endl;

      if(!inFile.read(buffer, sizeof(PcapHeader)))
      {
         break;
      }
      bytesReadThisPacket += sizeof(PcapHeader);

      cout << endl;
      cout << "sizeof(PcapHeader) = " << sizeof(PcapHeader) << endl;
      const PcapHeader& pcapHeader = *(reinterpret_cast<PcapHeader*>(buffer));
      cout << "pcapHeader.epoch_seconds=" << pcapHeader.epoch_seconds << endl;
      cout << "pcapHeader.epoch_microseconds=" << pcapHeader.epoch_microseconds << endl;
      cout << "pcapHeader.caplen=" << pcapHeader.caplen << endl;
      unsigned pcapHeaderLen = pcapHeader.len;
      cout << "pcapHeader.len=" << pcapHeaderLen << endl;

      if(!inFile.read(buffer, sizeof(EthernetHeader)))
      {
         break;
      }
      bytesReadThisPacket += sizeof(EthernetHeader);

      cout << endl;
      cout << "sizeof(EthernetHeader) = " << sizeof(EthernetHeader) << endl;

      if(!inFile.read(buffer, sizeof(IpHeader)))
      {
         break;
      }
      bytesReadThisPacket += sizeof(IpHeader);

      cout << endl;
      cout << "sizeof(IpHeader) = " << sizeof(IpHeader) << endl;
      const IpHeader& ipHeader = *(reinterpret_cast<IpHeader*>(buffer));
      cout << "ipHeader.tlen = " << ntohs(ipHeader.tlen) << endl;
      cout << "ipHeader.proto = " << ipHeader.proto << endl;
      cout << "ip_dest = " << (static_cast<unsigned>(ipHeader.dest_address.byte1) & 0x000000ff)
                    << "." << (static_cast<unsigned>(ipHeader.dest_address.byte2) & 0x000000ff)
                    << "." << (static_cast<unsigned>(ipHeader.dest_address.byte3) & 0x000000ff)
                    << "." << (static_cast<unsigned>(ipHeader.dest_address.byte4) & 0x000000ff)
                    << endl;
      cout << "static_cast<unsigned>(ipHeader.proto) = " << static_cast<unsigned>(ipHeader.proto) << endl;
      unsigned int ipLen = ((ipHeader.ver_ihl & 0xf) * 4);
      cout << "ipLen = " << ipLen << endl;
      unsigned int ver = ((ipHeader.ver_ihl >> 4) & 0xf);
      cout << "ver = " << ver << endl;

      if (ipHeader.proto != 17)
      {
          int remainingLength = static_cast<int>(pcapHeaderLen) + static_cast<int>(sizeof(PcapHeader)) - static_cast<int>(bytesReadThisPacket);
          if(!inFile.read(buffer, remainingLength))
          {
             break;
          }
          continue;
      }

      if(!inFile.read(buffer, sizeof(UdpHeader)))
      {
         break;
      }
      bytesReadThisPacket += sizeof(UdpHeader);

      cout << endl;
      const UdpHeader& udpHeader = *(reinterpret_cast<UdpHeader*>(buffer));
      cout << "dest_port = " << ntohs(udpHeader.dest_port) << endl;
      unsigned udpPayloadLength = ntohs(udpHeader.udp_length);
      cout << "udp_length = " << udpPayloadLength << endl;     

      if(!inFile.read(buffer, udpPayloadLength))
      {
         break;
      }
      bytesReadThisPacket += udpPayloadLength;
      cout << "bytesReadThisPacket = " << bytesReadThisPacket << endl;
      // Here, can get seq nums

      // some footer
      int footerLength = static_cast<int>(pcapHeaderLen) + static_cast<int>(sizeof(PcapHeader)) - static_cast<int>(bytesReadThisPacket);
      cout << "footerLength = " << footerLength << endl;
      if(!inFile.read(buffer, footerLength))
      {
         break;
      }
   }
   

   return_val = 0;
    """
    return (code, supportCode)

def startUp():

    global options
    usage = "usage: --file_path=FILE_PATH"
    parser = OptionParser(usage)
    parser.add_option("--file_path", dest="filePath", help="")

    (options, args) = parser.parse_args()
    if not options.filePath:
        print usage
        sys.exit(0)

if __name__ == '__main__' :
   
    startUp()

    (runCodeStr, codeStr) = getCodeTemplate()
    runCodeStr = runCodeStr.replace("$FILE_PATH", options.filePath)

    #print codeStr

    res = scipy.weave.inline(runCodeStr, support_code=codeStr, force=1, headers = ["<iostream>", "<string>", "<list>", "<fstream>", "<sstream>", "<vector>", "<ctime>", "<netinet/in.h>"], compiler='gcc')

