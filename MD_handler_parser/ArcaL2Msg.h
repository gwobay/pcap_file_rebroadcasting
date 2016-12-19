/********************************************************************************************

End user license agreement

NOTICE TO USER: BY USING ALL OR ANY PORTION OF THE SOFTWARE ("SOFTWARE") YOU ACCEPT ALL
THE TERMS AND CONDITIONS OF THIS AGREEMENT.  YOU AGREE THAT THIS AGREEMENT IS
ENFORCEABLE LIKE ANY WRITTEN NEGOTIATED AGREEMENT SIGNED BY YOU. THIS AGREEMENT IS
ENFORCEABLE AGAINST YOU AND ANY LEGAL ENTITY THAT OBTAINED THE SOFTWARE AND ON WHOSE
BEHALF IT IS USED. IF YOU DO NOT AGREE, DO NOT USE THIS SOFTWARE.

1. Software License. As long as you comply with the terms of this Software License Agreement (this
"Agreement"), NYSE Euronext, Inc. grants to you a non-exclusive license to Use the
Software for your business purposes.

2. Intellectual Property Ownership, Copyright Protection.  The Software and any authorized copies
that you make are the intellectual property of and are owned by NYSE Euronext.  The Software is protected by
law, including without limitation the copyright laws of the United States and other countries, and by
international treaty provisions.

3. NO WARRANTY. The Software is being delivered to you "AS IS" and NYSE Euronext makes no warranty as
to its use or performance. NYSE EURONEXT DOES NOT AND CANNOT WARRANT THE PERFORMANCE OR
RESULTS YOU MAY OBTAIN BY USING THE SOFTWARE. EXCEPT FOR ANY WARRANTY, CONDITION,
REPRESENTATION OR TERM TO THE EXTENT TO WHICH THE SAME CANNOT OR MAY NOT BE EXCLUDED OR
LIMITED BY LAW APPLICABLE TO YOU IN YOUR JURISDICTION, NYSE EURONEXT MAKES NO WARRANTIES
CONDITIONS, REPRESENTATIONS, OR TERMS (EXPRESS OR IMPLIED WHETHER BY STATUTE, COMMON
LAW, CUSTOM, USAGE OR OTHERWISE) AS TO ANY MATTER INCLUDING WITHOUT LIMITATION TITLE,
NONINFRINGEMENT OF THIRD PARTY RIGHTS, MERCHANTABILITY, INTEGRATION, SATISFACTORY
QUALITY, OR FITNESS FOR ANY PARTICULAR PURPOSE.

4.  Indemnity.  You agree to hold NYSE Euronext harmless from any and all liabilities, losses, actions,
damages, or claims (including all reasonable expenses, costs, and attorneys fees) arising out of or relating
to any use of, or reliance on, the Software.

5. LIMITATION OF LIABILITY. IN NO EVENT WILL NYSE EURONEXT BE LIABLE TO YOU FOR ANY DAMAGES,
CLAIMS OR COSTS WHATSOEVER OR ANY DIRECT, CONSEQUENTIAL, INDIRECT, INCIDENTAL DAMAGES,
OR ANY LOST PROFITS OR LOST SAVINGS, EVEN IF NYSE EURONEXT HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS OR FOR ANY CLAIM BY ANY THIRD PARTY.

6. Export Rules. You agree that the Software will not be shipped, transferred or exported into any country
or used in any manner prohibited by the United States Export Administration Act or any other export laws,
restrictions or regulations (collectively the "Export Laws"). In addition, if the Software is identified as export
controlled items under the Export Laws, you represent and warrant that you are not a citizen, or otherwise
located within, an embargoed nation (including without limitation Iran, Iraq, Syria, Sudan, Libya, Cuba,
North Korea, and Serbia) and that you are not otherwise prohibited under the Export Laws from receiving
the Software. All rights to Use the Software are granted on condition that such rights are forfeited if you fail
to comply with the terms of this Agreement.

7. Governing Law. This Agreement will be governed by and construed in accordance with the substantive
 laws in force in the State of Illinois.

***************************************************************************************************************/


/*
*	Program : ArcaL2Msg.h
*	By      : Fred Jones/David Le/Anil Nagasamudra 
*	Date    : 12/1/06
*	Purpose : ArcaBook Binary Multicast Messages
*
*	Note: All prices are in readable form. All fields are 
*	left justify and NULL padded. The first char of these 
*	structures will always be the message type.
*
*	Edit history:
*
*	Date		By		Reason
*	========	====	================================================
*	12-01-06	FLJ		Define New Messges
*
*
*/

#ifndef __ARCABOOK_L2_MSG_H__
#define __ARCABOOK_L2_MSG_H__

#include <sys/types.h>
#include <ctype.h>

#ifdef OS_LINUX
#include <stdint.h>
#endif

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef OS_LINUX
    #define SYS_BIG_ENDIAN __BIG_ENDIAN
    #define SYS_LITTLE_ENDIAN  __LITTLE_ENDIAN
    #define SYS_BYTE_ORDER __BYTE_ORDER
#else
#ifdef _WIN32
	#define SYS_BIG_ENDIAN 4321
    #define SYS_LITTLE_ENDIAN  1234
    #define SYS_BYTE_ORDER SYS_LITTLE_ENDIAN
#else
    #define SYS_BIG_ENDIAN 4321
    #define SYS_LITTLE_ENDIAN 1234
    #ifdef _BIG_ENDIAN
        #define SYS_BYTE_ORDER SYS_BIG_ENDIAN
    #else
        #define SYS_BYTE_ORDER SYS_LITTLE_ENDIAN
    #endif
#endif
#endif

#ifndef BSWAP_8
#define BSWAP_8(x) ((x) & 0xff)
#endif

#ifndef BSWAP_16
#define BSWAP_16(x)((BSWAP_8(x) << 8) | BSWAP_8((x) >> 8))
#endif

#ifndef BSWAP_32
#define BSWAP_32(x)((BSWAP_16(x) << 16) | BSWAP_16((x) >> 16))
#endif

#ifndef BSWAP_64
#define BSWAP_64(x) ((BSWAP_32(x) << 32) | BSWAP_32((x) >> 32))
#endif

#ifndef htonll
#if (SYS_BYTE_ORDER == SYS_BIG_ENDIAN)
#define htonll(x) (x)
#else
#define htonll(x) (BSWAP_64(x))
#endif
#endif

#ifndef ntohll
#if (SYS_BYTE_ORDER == SYS_BIG_ENDIAN)
#define ntohll(x) (x)
#else
#define ntohll(x) (BSWAP_64(x))
#endif
#endif

/****************** EXTERNAL MESSAGES ************************/

#ifdef _BIG_ENDIAN
	struct OrderID32 
	{
		uint32_t	iID_2;		// Other stuff 
		uint32_t	iID_1;		// OrderID or ExecID (0-4.2 billion)
	};

	struct OrderIDStruct
	{
		char		cBits;		// Bits defined above
		uint8_t		iSystemID;	// Matching engine number (0-256)
		uint16_t	iMarketID;	// Market ID (0-65,536)
		uint32_t	iID;		// OrderID or ExecID (0-4.2 billion)
	}   __attribute__ ((packed));

	union order_exec32_id
	{
		uint64_t lDLong;
		OrderIDStruct in;
		OrderID32 in2;
	};

	union order_exec64_id
	{
		uint64_t lDLong;
		OrderIDStruct in;
		OrderID32 in2;
	};
#else
	struct OrderID32 
	{
		uint32_t	iID_1;		// OrderID or ExecID (0-4.2 billion)
		uint32_t	iID_2;		// Other stuff 
	};

	struct OrderIDStruct
	{
		char		cBits;		// Bits defined above
		uint8_t		iSystemID;	// Matching engine number (0-256)
		uint16_t	iMarketID;	// Market ID (0-65,536)
		uint32_t	iID;		// OrderID or ExecID (0-4.2 billion)
	}   __attribute__ ((packed));

/*
	struct OrderIDStruct
	{
		uint32_t	iID;		// OrderID or ExecID (0-4.2 billion)
		uint16_t	iMarketID;	// Market ID (0-65,536)
		uint8_t		iSystemID;	// Matching engine number (0-256)
		char		cBits;		// Bits defined above
	}   __attribute__ ((packed));
*/

	union order_exec32_id
	{
                uint64_t lDLong;
		OrderIDStruct in;
		OrderID32 in2;
	};

	union order_exec64_id
	{
		uint64_t lDLong;
		OrderIDStruct in;
		OrderID32 in2;
	};
#endif 


#define ARCA_L2_SYMBOL_LEN							16
#define ARCA_L2_SOURCEID_LEN						20
#define ARCA_L2_FIRM_ID_LEN							5

#define ARCA_L2_BOOK_MSG_TYPE						99
#define ARCA_L2_ADD_MSG_TYPE						100
#define ARCA_L2_MODIFY_MSG_TYPE						101
#define ARCA_L2_DELETE_MSG_TYPE						102
#define ARCA_L2_IMBALANCE_MSG_TYPE					103

#define ARCA_L2_SEQ_RESET_MSG_TYPE					1
#define ARCA_L2_HB_MSG_TYPE							2
#define ARCA_L2_UNAVAILABLE_MSG_TYPE				5
#define ARCA_L2_RETRANS_RESPONSE_MSG_TYPE			10
#define ARCA_L2_RETRANS_REQUEST_MSG_TYPE			20
#define ARCA_L2_HB_REFRESH_RESPONSE_MSG_TYPE		24
#define ARCA_L2_BOOK_REFRESH_REQUEST_MSG_TYPE		30
#define ARCA_L2_IMBALANCE_REFRESH_REQUEST_MSG_TYPE	31
#define ARCA_L2_BOOK_REFRESH_MSG_TYPE				32
#define ARCA_L2_IMBALANCE_REFRESH_MSG_TYPE			33
#define ARCA_L2_SYMBOL_IDX_MAP_REQUEST_MSG_TYPE		34
#define ARCA_L2_SYMBOL_IDX_MAP_MSG_TYPE				35
#define ARCA_L2_SYMBOL_CLEAR_MSG_TYPE				36
#define ARCA_L2_FIRM_IDX_MAP_MSG_TYPE				37
#define ARCA_L2_FIRM_IDX_MAP_REQUEST_MSG_TYPE		38


/****************** EXTERNAL MESSAGES ************************/


/******************************************
 * External Message Header
 *
 * Size = 16 
 ******************************************/
struct ArcaL2Header_t
{
	uint16_t	iMsgSize;				// Refers to overall Packet Length minus 2 bytes
	uint16_t	iMsgType;				// 1   - Sequence Number Reset
								// 2   - HB Message
								// 5   - Message Unavailable
								// 10  - Retrans Request Message
								// 20  - Retrans Response Message
								// 23  - Refresh Request Message
								// 24  - HB Refresh Response Message
								// 30  - Book Refresh Request Message
								// 31  - Imbalance Refresh Request Message
								// 33  - Imbalance Refresh Message
								// 34  - Symbol Index Mapping Request Message
								// 35  - Symbol Index Mapping Message
								// 36  - Symbol Clear
								// 37  - firm Index Mapping Messasge
								// 38  - firm Index Mapping Messasge Request
								// 99  - Book Message (Add, Modify, Delete)

	uint32_t	iMsgSeqNum;				// Line Sequence Number
	uint32_t	iSendTime;				// Milliseconds since Midnight
	uint8_t		iProductID;				// 115 - Book Feed
	uint8_t		iRetransFlag;				// 1    - Original Message
								// 2    - Retrans Message
								// 3    - Message Replay
								// 4    - Retrans of replayed Message
								// 5    - Refresh Retrans
	uint8_t		iNumBodyEntries;
	uint8_t		iChannelID;				// Filler for Now. May specifiy id in future.
}  __attribute__ ((packed));
typedef struct ArcaL2Header_t ArcaL2Header_t;


/******************************************
 * External Refresh Message Header
 *
 * Message Type = 32
 *
 * Size = 48
 ******************************************/
struct ArcaL2FreshHeader_t
{
	ArcaL2Header_t Header;					// Packet Header

	uint8_t		iFiller;
	uint8_t		iSessionID;				// Source Session ID
	uint16_t	iSec;					// Stock Index
	uint16_t	iCurrentPktNum;				// Current Refresh Packet
	uint16_t	iTotalPktNum;				// Total Packets for this Refresh

	uint32_t	iLastSymbolSeqNum;			// Last seq number for symbol
	uint32_t	iLastPktSeqNum;				// Last Pkt seq number for symbol	

	char 		sSymbol[ARCA_L2_SYMBOL_LEN];		// Symbol
}  __attribute__ ((packed));
typedef struct ArcaL2FreshHeader_t ArcaL2FreshHeader_t;


/******************************************
 * External Book Order Message
 *
 * Message Type = 32
 *
 ******************************************/
struct ArcaL2BookOrder_t
{
	uint32_t		iSymbolSequence;		// Symbol Sequence
	uint32_t		iSourceTime;			// Milliseconds since midnight
	order_exec32_id	iOrderID;				// Order Reference #
	uint32_t		iVolume;				// Volume		
	uint32_t		iPrice;					// Price		

	uint8_t			iPriceScale;			// Price Scale Code

	char			cSide;					// B - Buy
											// S - Sell

	char			cExchangeID;			// N - NYSE
											// P - NYSE ARCA

	char			cSecurityType;			// E - Equity
											// B - Bond

	uint16_t		iFirm;					// Firm Index

	char			sFiller[2];				// Filler
}  __attribute__ ((packed));
typedef struct ArcaL2BookOrder_t ArcaL2BookOrder_t;


/******************************************
 * External Add Message
 *
 * Message Type  = 99/100
 *
 * Size = 36
 ******************************************/
struct ArcaL2Add_t
{
	uint16_t		iSec;					// Security/Stock Index
	uint16_t		iMsgType;				// 100 - Add Message
	
	uint32_t		iSymbolSequence;		// Symbol Sequence
	uint32_t		iSourceTime;			// Milliseconds since midnight
	order_exec32_id	iOrderID;				// Order Reference #
	uint32_t		iVolume;				// Volume		
	uint32_t		iPrice;					// Price

	uint8_t			iPriceScale;			// Price Scale Code

	char			cSide;					// B - Buy
											// S - Sell

	char			cExchangeID;			// N - NYSE
											// P - NYSE ARCA

	char			cSecurityType;			// E - Equity
											// B - Bond

	uint16_t		iFirm;					// Firm Index
	uint8_t			iSessionID;				// Source Session ID
	char			cFiller;				// Filler
}  __attribute__ ((packed));
typedef struct ArcaL2Add_t ArcaL2Add_t;


/******************************************
 * External Modify Message
 *
 * Message Type  = 99/101
 *
 * Size = 36
 ******************************************/
struct ArcaL2Modify_t
{
	uint16_t		iSec;					// Stock Index
	uint16_t		iMsgType;				// 101 - Modify Message
	
	uint32_t		iSymbolSequence;		// Symbol Sequence
	uint32_t		iSourceTime;			// Milliseconds since midnight
	order_exec32_id	iOrderID;				// Order Reference #
	uint32_t		iVolume;				// Volume		
	uint32_t		iPrice;					// Price

	uint8_t			iPriceScale;			// Price Scale Code

	char			cSide;					// B - Buy
											// S - Sell

	char			cExchangeID;			// N - NYSE
											// P - NYSE ARCA

	char			cSecurityType;			// E - Equity
											// B - Bond

	uint16_t		iFirm;					// Firm Index
	uint8_t			iSessionID;				// Source Session ID
	char			iReasonCode;				// ReasonCode
}  __attribute__ ((packed));
typedef struct ArcaL2Modify_t ArcaL2Modify_t;


/******************************************
 * External Delete Message
 *
 * Message Type = 99/102
 *
 * Size = 28
 ******************************************/
struct ArcaL2Delete_t
{
	uint16_t		iSec;					// Stock Index
	uint16_t		iMsgType;				// 102 - Delete Message
	
	uint32_t		iSymbolSequence;		// Symbol Sequence
	uint32_t		iSourceTime;			// Milliseconds since midnight
	order_exec32_id	iOrderID;				// Order Reference #

	char			cSide;					// B - Buy
											// S - Sell

	char			cExchangeID;			// N - NYSE
											// P - NYSE ARCA

	char			cSecurityType;			// E - Equity
											// B - Bond

	uint8_t			iSessionID;				// Source Session ID
	uint16_t		iFirm;					// Firm Index
	char			iReasonCode;				// Filler
	char			cFiller;				// Filler

}  __attribute__ ((packed));
typedef struct ArcaL2Delete_t ArcaL2Delete_t;


/******************************************
 * External Imbalance Message
 *
 * Message Type = 99/103
 *
 * Size = 36+
 ******************************************/
struct ArcaL2Imbalance_t
{
	uint16_t	iSec;						// Stock Index
	uint16_t	iMsgType;					// 103 - Imbalance Message
	
	uint32_t	iSymbolSequence;			// Symbol Sequence
	uint32_t	iSourceTime;				// Milliseconds since midnight
	uint32_t	iVolume;					// Indicative Match Volume		
	int32_t		iTotalImbalance;			// Total Imbalance Volume		
	int32_t		iMarketImbalance;			// Market Imbalance Volume		
	uint32_t	iPrice;						// Indicative Match Price
	uint8_t		iPriceScale;				// Price Scale Code

	char		cAuctionType;				// O - Open
											// M - Market
											// H - Halt
											// C - Closing

	char		cExchangeID;				// N - NYSE
											// P - NYSE ARCA

	char		cSecurityType;				// E - Equity
											// B - Bond

	uint8_t		iSessionID;					// Source Session ID
	char		cFiller;					// Filler
	uint16_t	iAuctionTime;				// Projected Auction Time (hhmm)
}  __attribute__ ((packed));
typedef struct ArcaL2Imbalance_t ArcaL2Imbalance_t;

/**************************
 * Unavailable Message
 *
 * Total Size = 24
 **************************/
struct ArcaL2Unavailable_t
{
	ArcaL2Header_t		Header;

	uint32_t	iBeginSeqNum;				// Begin Sequence Number
	uint32_t	iEndSeqNum;					// End Sequence Number
}  __attribute__ ((packed));
typedef struct ArcaL2Unavailable_t ArcaL2Unavailable_t;


/***********************************************
 * External Symbol Update 
 *
 * Total Size = 20+
 ************************************************/
struct ArcaL2SymbolUpdate_t
{
	uint16_t	iSec;						// Stock Index

	uint8_t		iSessionID;					// Source Session ID
	char		cSysCode;					// System Code - filler to customer 
											// L - Listed
											// O - OTC
											// E - ETF
											// B - BB

	char sSymbol[ARCA_L2_SYMBOL_LEN];		// Symbol
}  __attribute__ ((packed));
typedef struct ArcaL2SymbolUpdate_t ArcaL2SymbolUpdate_t;


/***********************************************
 * External Firm Update 
 *
 * Total Size = 12+
 ************************************************/
struct ArcaL2FirmUpdate_t
{
	uint16_t	iFirm;						// Firm Index
	char		sFiller[5];					// Filler

	char		sFirmID[ARCA_L2_FIRM_ID_LEN];	// Firm ID
}  __attribute__ ((packed));
typedef struct ArcaL2FirmUpdate_t ArcaL2FirmUpdate_t;


/************************************************
 * External Message-Packet Sequence Number Reset
 *
 * Total Size = 4
 ************************************************/
struct ArcaL2SequenceReset_t
{
	uint32_t	iNextSeqNum;				// Next Sequence Number
}  __attribute__ ((packed));
typedef struct ArcaL2SequenceReset_t ArcaL2SequenceReset_t;


/***************************************
 * External Symbol Clear Message
 *
 * Total Size = 8+
 **************************************/
struct ArcaL2SymbolClear_t
{
	uint32_t	iNextSeqNum;            // Next Sequence Number
	uint16_t	iSec;					// Stock Index

	uint8_t		iSessionID;				// Source Session ID
	char		cFiller;				// Filler
}  __attribute__ ((packed));
typedef struct ArcaL2SymbolClear_t ArcaL2SymbolClear_t;

/*************************
 * External Message Union
 *************************/
union ArcaL2MsgUnion
{
	ArcaL2Header_t        		Header;
	ArcaL2Unavailable_t			Unavailable;

	ArcaL2Add_t					Add;
	ArcaL2Modify_t				Modify;
	ArcaL2Delete_t				Delete;
	ArcaL2Imbalance_t			Imbalance;

	ArcaL2SequenceReset_t		SequenceReset;			// Packet sequence reset
	ArcaL2SymbolClear_t			SymbolClear;

	ArcaL2SymbolUpdate_t		SymbolUpdate;			// Symbol Update
	ArcaL2FirmUpdate_t			FirmUpdate;				// Firm Update
	ArcaL2BookOrder_t			BookRefreshOrder;		// Book Refresh
};


#endif
