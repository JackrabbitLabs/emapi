/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file 		emapi.h 
 *
 * @brief 		Header file for CXL Emulator API commands
 *
 * @copyright 	Copyright (C) 2024 Jackrabbit Founders LLC. All rights reserved.
 *
 * @date 		Feb 2024
 * @author 		Barrett Edwards <code@jrlabs.io>
 *
 * Macro / Enumeration Prefixes 
 * EMMT - EM API Command Message Category Types (MT)
 * EMOB - Types of EM API Objects (OB)
 * EMOP - EM API Command Opcodes (OP)
 * RMRC - EM API Command Return Codes (RC)
 * 
 */
#ifndef _EMAPI_H
#define _EMAPI_H

/* INCLUDES ==================================================================*/

/**
 * For __u8, __u16, __u32, __u64 types
 */
#include <linux/types.h>

/* MACROS ====================================================================*/

// Length of struct emapi_hdr 
#define EMLN_HDR 					12
#define EMLN_MSG 					8192 					//!< Maximum length of a EM API Message Body (HDR + payload)
#define EMLN_PAYLOAD 				(EMLN_MSG - EMLN_HDR)  	//!< Maximum length of the EM API Message Payload 

// Maximum Length of a device name 
#define EMLN_DEV_NAME 				125

// Maximum numberof devices returned 
#define EMLN_DEV_NUM 				64

/* ENUMERATIONS ==============================================================*/

/**
 * Types of EM API Objects (OB)
 *
 * The primary purpose of this enum is for serialization/deserialization.
 */
enum _EMOB 
{
	EMOB_NULL				=  0,
	EMOB_HDR				=  1, //!< struct emapi_hdr
	EMOB_LIST_DEV			=  2, //!< struct emapi_list_dev
	EMOB_MAX
};

/**
 * EM API Command Message Category Types (MT)
 */
enum _EMMT 
{
	EMMT_REQ 		= 0,
	EMMT_RSP 		= 1,
	EMMT_EVENT 		= 2,
	EMMT_MAX
};

/**
 * EM API Command Opcodes (OP)
 */
enum _EMOP 
{
	EMOP_EVENT 							= 0x00,
	EMOP_LIST_DEV						= 0x01,
	EMOP_CONN_DEV						= 0x02,
	EMOP_DISCON_DEV 					= 0x03,
	EMOP_MAX
};

/**
 * EM API Command Return Codes (RC)
 */
enum _EMRC 
{
	EMRC_SUCCESS 						= 0x0,
	EMRC_BACKGROUND_OP_STARTED			= 0x1,
	EMRC_INVALID_INPUT					= 0x2, 
	EMRC_UNSUPPORTED 					= 0x3,
	EMRC_INTERNAL_ERROR					= 0x4,
	EMRC_BUSY							= 0x5,
	EMRC_MAX
};


/* STRUCTS ===================================================================*/

/** 
 * EM API Protocol Header
 */
struct emapi_hdr 
{
	__u8 type			: 4;	//!< Type of EM API message [EMMT]
	__u8 ver 			: 4;	//!< Header Version 
	__u8 tag;					//!< Tag used to track response messages 
	__u8 rc;					//!< Return Code [EMRC]
	__u8 opcode;    			//!< OpCode [EMOP]

	__u8 a;						//!< Immediate A 
	__u8 rsvd;				
	__u16 len;					//!< Payload length in bytes

	__u32 b;					//!< Immediate B
};

/**
 * List devices - Request (Opcode 01h) 
 * 
 * Immediate A: Num requested (0 = all)
 * Immediate B: Device num to start at 
 * Payload: None
 */

/**
 * List devices - Response Entry (Opcode 01h) 
 * 
 * Immediate A: Num devices returned 
 * Immediate B: Total devices 
 */

/**
 * List devices - Response Entry (Opcode 01h) 
 *
 */
struct emapi_dev 
{
	__u8 id;					//!< Device ID
	__u8 len; 					//!< Length of device name
	char name[EMLN_DEV_NAME];	//!< Device name 
}; 

/**
 * Connect - Request (Opcode 02h)
 * 
 * Immediate A: PPID 
 * Immediate B: Device ID
 */

/**
 * Disconnect - Request (Opcode 02h)
 * 
 * Immediate A: PPID 
 * Immediate B: All   1=Disconnect all, 0=Disconnect PPID in Immediate A
 */

/**
 * This struct is to store the serialized EM API header and object 
 */
struct __attribute__((__packed__)) emapi_buf
{
	__u8 hdr[EMLN_HDR];				//!< Buffer to store serialized EM API Header 
	__u8 payload[EMLN_PAYLOAD];		//!< Buffer to store serialized EM API Message Payload 
};

/**
 * EM API Message 
 */
struct emapi_msg
{
	struct emapi_hdr hdr;			//!< EM API Header 

	//!< This union is to store the deserialized object 
	union 
	{
		struct emapi_dev dev[EMLN_DEV_NUM];
	} obj;	
};

/* GLOBAL VARIABLES ==========================================================*/

/* PROTOTYPES ================================================================*/

/**
 * @brief Convert from a Little Endian byte array to a struct
 * 
 * @param[out] dst void Pointer to destination struct
 * @param[in] src void Pointer to unsigned char array
 * @param[in] type unsigned enum _EMOB representing type of object to deserialize
 * @param[in] param void * to data needed to deserialize the byte stream 
 * (e.g. count of objects to expect in the stream)
 * @return number of bytes consumed. -1 upon error
 */
int emapi_deserialize(void *dst, __u8 *src, unsigned type, void *param);

/**
 * Convenience function to populate a emapi_hdr object 
 *
 * @param[out] 	fh 			struct emapi_hdr* to fill in
 * @param[in] 	type    	__u8 Request / Response [EMMT]
 * @param[in] 	tag			__u8 message tag identifier 		
 * @param[in] 	rc 			__u8 retrun code
 * @param[in] 	opcode    	__u8 opcode [EMOP]	
 * @param[in] 	len  		__u16 Length of payload in bytes 		
 * @param[in] 	a         	__u8 Immediate value A
 * @param[in] 	b         	__u32 Immediate value B
 * @return 					length of EM API Header + payload 
 */
int emapi_fill_hdr
(
	struct emapi_hdr *eh, 
	__u8 type,				
	__u8 tag,				
	__u8 rc,				
	__u8 opcode,    		
	__u16 len, 				
	__u8 a,
	__u32 b					
);

/**
 * Determine the Request Object Identifier [EMOB] for an EM API Message Opcode [EMOP]
 *
 * @param	opcode 	This is an EM API Opcode [EMOP]
 * @return	int		Returns the EM API Object Identifier [EMOB] used in a Request
 */
int emapi_emob_req(unsigned int opcode);

/**
 * Determine the Response Object Identifier [EMOB] for an EM API Message Opcode [EMOP]
 *
 * @param	opcode 	This is an EM API Opcode [EMOP]
 * @return	int		Returns the EM API Object Identifier [EMOB] used in a Response 
 */
int emapi_emob_rsp(unsigned int opcode);

int emapi_fill_conn(struct emapi_msg *m, int ppid, int dev);
int emapi_fill_disconn(struct emapi_msg *m, int ppid, int all);
int emapi_fill_listdev(struct emapi_msg *m, int num, int start);

/**
 * @brief Convert an object into Little Endian byte array format
 * 
 * @param[out] dst void Pointer to destination unsigned char array
 * @param[in] src void Pointer to object to serialize
 * @param[in] type unsigned enum _EMOB representing type of object to serialize
 * @param[in] param void * to data needed to serialize the byte stream 
 * @return number of serialized bytes, -1 if error 
 */
int emapi_serialize(__u8 *dst, void *src, unsigned type, void *param);

/**
 * Print an object to the screen
 *
 * @param ptr A pointer to the object to print 
 * @param type The type of object to be printed from enum _EMOB
 */
void emapi_prnt(void *ptr, unsigned type);        

/* Functions to return a string representation of an object*/
const char *emmt(unsigned u);
const char *emob(unsigned u);
const char *emop(unsigned u);
const char *emrc(unsigned u);


#endif //ifndef _EMAPI_H
