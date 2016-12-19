
/*********************************************************************************************************

End user license agreement

NOTICE TO USER: BY USING ALL OR ANY PORTION OF THE SOFTWARE ("SOFTWARE") YOU ACCEPT ALL
THE TERMS AND CONDITIONS OF THIS AGREEMENT.  YOU AGREE THAT THIS AGREEMENT IS
ENFORCEABLE LIKE ANY WRITTEN NEGOTIATED AGREEMENT SIGNED BY YOU. THIS AGREEMENT IS
ENFORCEABLE AGAINST YOU AND ANY LEGAL ENTITY THAT OBTAINED THE SOFTWARE AND ON WHOSE
BEHALF IT IS USED. IF YOU DO NOT AGREE, DO NOT USE THIS SOFTWARE.

1. Software License. As long as you comply with the terms of this Software License Agreement (this
"Agreement"), NYSE EURONEXT, Inc. grants to you a non-exclusive license to Use the
Software for your business purposes.

2. Intellectual Property Ownership, Copyright Protection.  The Software and any authorized copies
that you make are the intellectual property of and are owned by NYSE Euronext, Inc.  The Software is protected by
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
*******************************************************************************************************************/

/*================================================================================*/
/*
 *    	ArcaBook FAST Implementation header file for multicast feed
 *
 *		Program : AB_Fast.h
 *		By      : Fred Jones/David Le
 *		Date    : 08/25/09
 *		Purpose : ArcaBook FAST field descriptions and identifiers for multicast
 *
 */
/*================================================================================*/
#ifndef ARCABOOKFAST_INCLUDE
#define ARCABOOKFAST_INCLUDE

/* Define a macro to do some compile time checking of type sizes */
#define COMPILE_TIME_ASSERT(expr) typedef int __compile_time_assert_fail[1 - 2 * !(expr)];

/* We need to first make sure we have proper primitive types available. We prefer 
   to use the C99 standard types found in <stdint.h> and <stddef.h>, but not all compilers have it.
 */
#if !defined(_MSC_VER)    /* Easy case, we have a stddef.h , so use it */

#	include <stddef.h>

#else                     /* Harder, we have no stdint.h, so define the types we need */

#	include <limits.h>
#	if (UCHAR_MAX == 0xff)
		typedef unsigned char  uint8_t;
#	else
#		error: You must define a valid unsigned 8 bit type as uint8_t
#	endif

#	if (CHAR_MAX == 127)
		typedef signed char  int8_t;
#	else
#		error: You must define a valid signed 8 bit type as int8_t
#	endif

#	if (SHRT_MAX == 32767)
		typedef short int16_t;
#	else
#		error: You must define a valid signed 16 bit type as int16_t
#	endif

#	if (INT_MAX == 2147483647)
		typedef int int32_t;
#	elif (LONG_MAX == 2147483647)
		typedef long int32_t;
#	else
#		error: You must define a valid signed 32 bit type as int32_t
#   endif

#	if (UINT_MAX == 0xffffffff)
		typedef unsigned int uint32_t;
#	elif (ULONG_MAX == 0xffffffff)
		typedef unsigned long uint32_t;
#	else
#		error: You must define a valid unsigned 32 bit type as uint32_t
#   endif

#endif

/* needed for ntohs() etc. for Big-Endian to Little-Endian conversions */
#if defined(_WIN32) || defined(WIN32)
#	include <Winsock2.h>
#else
#	include <netinet/in.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* Make sure all our structures are 1 byte aligned */
#pragma pack(1)

/* include the ArcaBook Binary message header files for multicast feed */
#include "ArcaL2Msg.h"
/*===========================================================================*/
/* We need to keep some state information about every possible field we will
   be encoding/decoding.  This will allow us to do Copy/Increment encoding
   across messages 
*/
enum AB_FIELDS
{
	AB_MSG_TYPE		= 0,	/* 100-Add, 101-Modify, 102-Delete, 103-Imbalance		*/
	AB_STOCK_IDX	= 1,	/* Stock Index											*/
	AB_SEQUENCE		= 2,	/* Sequence Number										*/
	AB_TIME			= 3,	/* Source Time  Scaled Seconds + Milliseconds			*/
	AB_ORDER_ID		= 4,	/* Order Reference ID									*/
	AB_VOLUME		= 5,	/* Volume												*/
	AB_PRICE		= 6,	/* Price												*/
	AB_PRICE_SCALE	= 7,	/* Price Scale											*/
	AB_BUY_SELL		= 8,	/* Buy/Sell (B-Buy, S-Sell)								*/
	AB_EXCH_ID		= 9,	/* Exchange Code (N-NYSE, P-NYSE Arca)					*/
	AB_SECURITY_TYPE= 10,	/* Security Type (E-Equity)								*/
	AB_FIRM_ID		= 11,	/* Atrributed Quote/MMID								*/
	AB_SESSION_ID	= 12,	/* Session ID/Engine ID									*/
	AB_BITMAP		= 13,	/* Generic binary data									*/
	AB_SYSTEMID_ID	= 14,   /* unique Market ID for 64 bit order ID					*/
	AB_MARKET_ID	= 15,   /* unique Market ID for 64 bit order ID					*/
	AB_BIT			= 16,   /* unique cBit for 64 bit order ID						*/
	AB_MAX_FIELD	= 17	/* Maximum fields, placeholder							*/
};

/* We can use these duplicate ID's since they do not appear together in any message type	*/
#define AB_IMBALANCE AB_ORDER_ID
#define AB_MKT_IMBALANCE AB_BITMAP
#define AB_AUCTION_TYPE AB_BUY_SELL
#define AB_AUCTION_TIME AB_FIRM_ID
#define AB_SYMBOL_STRING AB_BITMAP
#define AB_FIRM_STRING AB_BITMAP


/*===========================================================================*/
/* FAST Field Encoding Operators */
#define OP_NONE        0
#define OP_COPY        1
#define OP_INCR        2

#define AB_MAX_STRLEN   (64)   /* the max length of single ASCII string in a field  */
#define AB_MAX_FAST_MSG (128)  /* the max length of a ArcaBook Options FAST encoded message */
#define AB_MIN_FAST_MSG (2)    /* the min length, we always have at least a pmap and the type field */
#define AB_MAX_PMAP ((AB_MAX_FIELD-1)/7 + 1)  /* the max length of a pmap, based on current field definitions     */

/* we need state information for each possible field */
typedef struct tagFAST_STATE
{
	uint32_t field;		/* AB_* field id, always matches the index into array	*/   
	int32_t valid;		/* Is this a valid entry ?				*/
	int32_t encodeType;	/* COPY, INCR, etc.					*/
	int32_t size;		/* size in bytes of value below				*/
	union
	{
		uint32_t	uint32Val;			/* This is a normal 32bit unsigned integer		*/
		uint16_t	uint16Val;			/* This is a normal 16bit unsigned integer		*/
		uint8_t		uint8Val[AB_MAX_STRLEN];	/* This is a normal unsigned string			*/
		int8_t		int8Val[AB_MAX_STRLEN];		/* This is a non null terminated string, use size above	*/
	}value;		/* the actual value, either a 4 byte unsigned integer, or up to a 64 byte string*/

}FAST_STATE;

/* Define a constant state array with encoding methods and storage for encoded values */
static const FAST_STATE fastStateInit[AB_MAX_FIELD] =
	{
		{ AB_MSG_TYPE,0,OP_NONE,0,{0}},
		{ AB_STOCK_IDX,0,OP_COPY,0,{0}},
		{ AB_SEQUENCE,0,OP_INCR,0,{0}},
		{ AB_TIME,0,OP_COPY,0,{0}},
		{ AB_ORDER_ID,0,OP_COPY,0,{0}},
		{ AB_VOLUME,0,OP_COPY,0,{0}},
		{ AB_PRICE,0,OP_COPY,0,{0}},
		{ AB_PRICE_SCALE,0,OP_COPY,0,{0}},
		{ AB_BUY_SELL,0,OP_COPY,0,{0}},
		{ AB_EXCH_ID,0,OP_COPY,0,{0}},
		{ AB_SECURITY_TYPE,0,OP_COPY,0,{0}},
		{ AB_FIRM_ID,0,OP_COPY,0,{0}},
		{ AB_SESSION_ID,0,OP_COPY,0,{0}},
		{ AB_BITMAP,0,OP_NONE,0,{0}},
		{ AB_SYSTEMID_ID,0,OP_COPY,0,{0}},		
		{ AB_MARKET_ID,0,OP_COPY,0,{0}},
		{ AB_BIT,0,OP_COPY,0,{0}}
	};

/*===========================================================================*/


/* Check some platform specifics to make sure our types and structures are the correct size
 * C-style compile-time assertion, will report zero size array if sizes are not what we expected
 */
//static void CompileTimeAssertions(void)
//{
//	{COMPILE_TIME_ASSERT(sizeof( uint8_t )	== 1);}
//	{COMPILE_TIME_ASSERT(sizeof( int8_t )   == 1);}
//
//	{COMPILE_TIME_ASSERT(sizeof( int16_t )  == 2);}
//	{COMPILE_TIME_ASSERT(sizeof( int32_t )  == 4);}
//
//	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Header_t )	== 16);}
//	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Add_t )	== 36);}
//	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Modify_t )	== 36);}
//	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Delete_t )	== 28);}
//	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Imbalance_t ) == 36);}
//	{COMPILE_TIME_ASSERT(sizeof( ArcaL2SequenceReset_t ) == 4);}
//	{COMPILE_TIME_ASSERT(sizeof( ArcaL2SymbolClear_t ) == 8);}
//	{COMPILE_TIME_ASSERT(sizeof( ArcaL2SymbolUpdate_t ) == 20);}
//	{COMPILE_TIME_ASSERT(sizeof( ArcaL2FirmUpdate_t ) == 12);}
//	/* A safety net to catch someone who adds a new, very large ArcaBook message */
//	{COMPILE_TIME_ASSERT( AB_MAX_FAST_MSG > (sizeof( ArcaL2MsgUnion ) / 4 * 5) );}
//}


/* Funtion Prototypes */

/*
 * Encode a ARCA Book for Options binary message to FAST 
 * 
 * dest			Pointer to a uint8_t buffer that will be filled with the FAST encoded message
 * destLen		Pointer to a int32_t that will be filled with the length of the encoded FAST message
 * srcMsg		Pointer to a ArcaQuoteMsgUnion structure containing the message to encode
 * msgType		Pointer to a message type for the srcMsg being passed in
 * state		Pointer to the FAST state information
 *
 * returns 0 if success, < 0 on error
 */
int32_t ABFastEncode(uint8_t *dest, int32_t *destLen, const union ArcaL2MsgUnion *srcMsg, const uint16_t iMsgType, FAST_STATE *state);

/*
 * Decode a FAST message to ARCA Book binary message
 * 
 * dstMsg	Pointer to a ArcaQuoteMsgUnion structure to be filled with the decoded message
 * src		Pointer to a uint8_t buffer containing the FAST encoded message
 * srcLen	uint32_t containing the length of the src buffer
 * msgType	Pointer to a message type of the ArcaL2MsgMsgUnion structure to be filled with the decoded message
 * state	Pointer to the FAST state information
 *
 * returns 0 if success, < 0 if error
 */
int32_t ABFastDecode(union ArcaL2MsgUnion *dstMsg, const uint8_t *src, int32_t *srcLen, uint16_t *iMsgType, FAST_STATE *state);

/*
 * Return codes, always < 0 on error
 */
#define AB_OK			(0)   /* No error, everything OK  */
#define AB_GENERAL_ERROR	(-1)  /* General error            */     
#define AB_INVALID_FIELD	(-2)  /* The field ID is out of range            */   
#define AB_INVALID_STATE	(-3)  /* The state structure is invalid          */     
#define AB_INCOMPLETE_ERROR	(-4)  /* There is not a complete stream available to decode */
#define AB_BUF_ERROR		(-5)  /* There is not enough room in the destination buffer */
#define AB_INVALID_HEADER	(-6)  /* FAST Header is invalid */
#define AB_INVALID_ASCII	(-7)  /* The ASCII data being encoded is not ASCII ( > 0x80) */
#define AB_INVALID_LENGTH	(-8)  /* The data is either too long or too short */

/*
 * 7-bit alignment and bit manipulation macros
 */

/* Set a bit in buf, using only 7-bits per byte, always skipping the high bit in each byte */
#define	SETBIT(buf,bitnum) (buf [(bitnum) / 7] |= (0x40 >> (bitnum) % 7))

/* Check if a bit is set, again using only 7-bits per byte */
#define ISBITSET(buf,bitnum) (buf[(bitnum) / 7] & (0x40 >> (bitnum) % 7))

/* Count how many bytes it will take to encode the 8-bit buf using 7-bits 
   We first convert the bytes to bits, then add 6 extra bits since the division by 7
   will truncate to an integer*/
#define ALIGN7(bytes) (((bytes)*8 + 6) / 7)

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#pragma pack()

#endif /*  ARCABOOKFAST_INCLUDE */
