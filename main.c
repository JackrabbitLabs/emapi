/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file 		emapi.c
 *
 * @brief 		Code file for CXL Emulator API commands
 *
 * @copyright 	Copyright (C) 2024 Jackrabbit Founders LLC. All rights reserved.
 *
 * @date 		FEb 2024
 * @author 		Barrett Edwards <code@jrlabs.io>
 *
 */
/* INCLUDES ==================================================================*/

/* printf()
 */
#include <stdio.h>

/* Return error codes from functions
 */
#include <errno.h>

/* memcpy()
 */
#include <string.h>

#include <arrayutils.h>

#include "emapi.h"

/* MACROS ====================================================================*/

/* ENUMERATIONS ==============================================================*/

/* STRUCTS ===================================================================*/

/* GLOBAL VARIABLES ==========================================================*/

/**
 * String representations of EM API Command Message types (MT)
 */
const char *STR_EMMT[] = {
	"Request",		// EMMT_REQ		= 0
	"Response",		// EMMT_RSP		= 1
	"Event"			// EMMT_EVENT	= 2
};

/**
 * String repressentations of EM API Objects (OB)
 */ 
const char *STR_EMOB[] = {
	"Null", 		// EMOB_NULL			=  0,
	"emob_hdr", 	// EMOB_HDR				=  1, //!< struct emapi_hdr
	"emob_dev", 	// EMOB_LIST_DEV		=  2, //!< struct emapi_list_dev
};

/**
 * String repressentations of EM API Opcodes (OP)
 */ 
const char *STR_EMOP[] = {
	"Event Notification", 		// EMOP_EVENT 			= 0x00
	"List Devices", 			// EMOP_LIST_DEV		= 0x01
	"Connect Device", 			// EMOP_CONN_DEV		= 0x02
	"Disconnect Device",  		// EMOP_DISCON_DEV 		= 0x03
};

/**
 * String representations of CXL Emulator API Return Codes (RC)
 */
const char *STR_EMRC[] = {
	"Success",									// EMRC_SUCCESS 						= 0x00,
	"Background operation started", 			// EMRC_BACKGROUND_OP_STARTED			= 0x01,
	"Invalid input",							// EMRC_INVALID_INPUT					= 0x02, 
	"Unsupported",								// EMRC_UNSUPPORTED 					= 0x03,
	"Internal error", 							// EMRC_INTERNAL_ERROR					= 0x04,
	"Busy",										// EMRC_BUSY							= 0x06,
};

/* PROTOTYPES ================================================================*/

void emapi_prnt_hdr(void *ptr);
void emapi_prnt_list_dev(void *ptr);

/* FUNCTIONS =================================================================*/

/**
 * @brief Convert from a Little Endian byte array to a struct
 * 
 * @param[out] dst void Pointer to destination struct
 * @param[in] src void Pointer to unsigned char array
 * @param[in] type unsigned enum _EMOB representing type of object to deserialize
 * @param[in] param void * to data needed to deserialize the byte stream 
 * (e.g. count of objects to expect in the stream)
 * @return number of bytes consumed. -1 upon error otherwise. 
 */
int emapi_deserialize(void *dst, __u8 *src, unsigned type, void *param)
{
	int rv;

	// Initialize variables 
	rv = -1;

	// Validate Inputs 
	if ( (dst == NULL) || (type >= EMOB_MAX) )
		goto end;

	// Handle each object type 
	switch(type)
	{
		case EMOB_NULL:
			rv = 0;
			break;

		case EMOB_HDR: //!< struct emapi_hdr
		{
			struct emapi_hdr *o = (struct emapi_hdr*) dst;
			o->ver 			= (src[ 0] >> 4) & 0x0F;	
			o->type 		= (src[ 0]     ) & 0x0F;	
			o->tag 			=  src[ 1]; 
			o->rc 			=  src[ 2];
			o->opcode 		=  src[ 3];
			o->a 			=  src[ 4];
			o->len 			= (src[ 7] <<  8) |  src[ 6];
			o->b 			= (src[11] << 24) | (src[10] << 16) | (src[ 9] << 8) | src[ 8];
			rv = EMLN_HDR;
		}
			break;

		case EMOB_LIST_DEV: //!< struct emapi_dev
		{
			unsigned i, k, num;
			struct emapi_dev *o;

			// Initialize variables 
			k = 0;
			o = (struct emapi_dev*) dst;
			if (param == NULL) 
				num = 1;
			else 
				num = *((unsigned *) param);

			for ( i = 0 ; i < num ; i++ )
			{
				o->id 	= src[k++];	
				o->len 	= src[k++];	
				if (o->len == 0)
					o->name[0] = 0;
				else 
					memcpy(o->name, &src[k], o->len);
				k += o->len;
				o++;	
			}
			rv = k; 
		}
			break;

		default:
			goto end;
	}

end:

	return rv;
}

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
 */
int emapi_fill_hdr
(
	struct emapi_hdr *h, 
	__u8 type,				
	__u8 tag,				
	__u8 rc,				
	__u8 opcode,    		
	__u16 len, 				
	__u8 a,
	__u32 b					
)
{
	if (h == NULL)
		return 0;
	h->ver = 0;
	h->type = type;
	h->tag = tag;
	h->rc = rc;
	h->opcode = opcode;
	h->len = len;
	h->a = a;
	h->b = b;
	return EMLN_HDR + len;
}

/** 
 * Prepare an EM API Message - Connect 
 *
 * @param m		emapi_msg* to fill
 * @return 		0 upon success, non zero otherwise
 */
int emapi_fill_conn(struct emapi_msg *m, int ppid, int dev)
{
	int rv;

	// Initialize variables
	rv = 1;

	// Validate Inputs 
	if (m == NULL)
		goto end;

	// Clear Header
	memset(&m->hdr, 0, sizeof(struct emapi_hdr));

	// Set header 
	m->hdr.opcode = EMOP_CONN_DEV;	
	m->hdr.a = ppid;
	m->hdr.b = dev;

	// Set object
		
	rv = 0;

end:

	return rv;
}

/** 
 * Prepare an EM API Message - Disconnect 
 *
 * @param m		emapi_msg* to fill
 * @return 		0 upon success, non zero otherwise
 */
int emapi_fill_disconn(struct emapi_msg *m, int ppid, int all)
{
	int rv;

	// Initialize variables
	rv = 1;

	// Validate Inputs 
	if (m == NULL)
		goto end;

	// Clear Header
	memset(&m->hdr, 0, sizeof(struct emapi_hdr));

	// Set header 
	m->hdr.opcode = EMOP_DISCON_DEV;	
	m->hdr.a = ppid;
	m->hdr.b = all;

	// Set object
		
	rv = 0;

end:

	return rv;
}

/** 
 * Prepare an EM API Message - List Devices
 *
 * @param m		emapi_msg* to fill
 * @return 		0 upon success, non zero otherwise
 */
int emapi_fill_listdev(struct emapi_msg *m, int num, int start)
{
	int rv;

	// Initialize variables
	rv = 1;

	// Validate Inputs 
	if (m == NULL)
		goto end;

	// Clear Header
	memset(&m->hdr, 0, sizeof(struct emapi_hdr));

	// Set header 
	m->hdr.opcode = EMOP_LIST_DEV;	
	m->hdr.a = num;
	m->hdr.b = start;

	// Set object
		
	rv = 0;

end:

	return rv;
}

/**
 * @brief Convert an object into Little Endian byte array format
 * 
 * @param[out] dst void Pointer to destination unsigned char array
 * @param[in] src void Pointer to object to serialize
 * @param[in] type unsigned enum _EMOB representing type of object to serialize
 * @return number of serialized bytes, 0 if error 
 */
int emapi_serialize(__u8 *dst, void *src, unsigned type, void *param)
{
	int rv;

	// Initialize variables 
	rv = 0;

	// Validate Inputs 
	if ( (type == EMOB_NULL) || (type >= EMOB_MAX) )
		goto end;

	switch(type)
	{
		case EMOB_HDR: //!< struct emapi_hdr
		{
			struct emapi_hdr *o = (struct emapi_hdr*) src;
			dst[0]  = ((o->ver  << 4) & 0xF0) | (o->type & 0x0F);
			dst[1]  = o->tag;
			dst[2]  = o->rc;
			dst[3]  = o->opcode;
			dst[4]  = o->a;
			dst[6]  = (o->len      ) & 0x00FF;
			dst[7]  = (o->len >> 8 ) & 0x00FF;
			dst[ 8] = (o->b        ) & 0x00FF;
			dst[ 9] = (o->b   >>  8) & 0x00FF;
			dst[10] = (o->b   >> 16) & 0x00FF;
			dst[11] = (o->b   >> 24) & 0x00FF;
			rv = EMLN_HDR;
		}
			break;

		case EMOB_LIST_DEV: //!< struct emapi_dev
		{
			struct emapi_dev *o = (struct emapi_dev*) src;
			dst[0] = o->id;
			dst[1] = o->len;
			memcpy(&dst[2], o->name, o->len);
			rv = o->len + 2;;
		}
			break;

		default:
			goto end;
	}

end:
	
	return rv;
};

/**
 * Determine the Request Object Identifier [EMOB] for an EM API Message Opcode [EMOP]
 *
 * @param	opcode 	This is an EM API Opcode [EMOP]
 * @return	int		Returns the EM API Object Identifier [EMOB] used in a Request
 */
int emapi_emob_req(unsigned int opcode)
{
	switch (opcode)
	{
		case EMOP_EVENT:				return EMOB_NULL;	
		case EMOP_LIST_DEV:				return EMOB_LIST_DEV;
		case EMOP_CONN_DEV:				return EMOB_NULL;	
		case EMOP_DISCON_DEV: 			return EMOB_NULL;
		default: 						return EMOB_NULL;
	}
}

/**
 * Determine the Response Object Identifier [EMOB] for an EM API Message Opcode [EMOP]
 *
 * @param	opcode 	This is an EM API Opcode [EMOP]
 * @return	int		Returns the EM API Object Identifier [EMOB] used in a Response 
 */
int emapi_emob_rsp(unsigned int opcode)
{
	switch (opcode)
	{
		case EMOP_EVENT:				return EMOB_NULL;
		case EMOP_LIST_DEV:				return EMOB_LIST_DEV;
		case EMOP_CONN_DEV:				return EMOB_NULL;	
		case EMOP_DISCON_DEV: 			return EMOB_NULL;
		default: 						return EMOB_NULL;
	}
}

/* Functions to return a string representation of an object*/

const char *emmt(unsigned int u)
{
	if (u >= EMMT_MAX) 	return NULL;
	return STR_EMMT[u];	
}

const char *emob(unsigned int u)
{
	if (u >= EMOB_MAX) 	return NULL;
	return STR_EMOB[u];	
}

const char *emop(unsigned int u) 
{
	if (u >= EMOP_MAX) 	return NULL;
	return STR_EMOP[u];	
}

const char *emrc(unsigned int u)
{
	if (u >= EMRC_MAX) 	return NULL;
	return STR_EMRC[u];	
}



/**
 * Print an object to the screen
 *
 * @param ptr A pointer to the object to print 
 * @param type The type of object to be printed from enum _FMOB
 */
void emapi_prnt(void *ptr, unsigned type)
{
	switch (type)
	{
		case EMOB_HDR:         emapi_prnt_hdr(ptr);						break;
		case EMOB_LIST_DEV:    emapi_prnt_list_dev(ptr);				break;
		default: break;
	}
}

void emapi_prnt_hdr(void *ptr)
{
	struct emapi_hdr *o = (struct emapi_hdr*) ptr;
	printf("emapi_hdr:\n");
	printf("Version:           0x%02x\n", o->ver);
	printf("Type:              0x%02x\n", o->type);
	printf("Tag:               0x%02x\n", o->tag);
	printf("Return Code:       0x%02x\n", o->rc);
	printf("Opcode:            0x%02x\n", o->opcode);
	printf("Immediate: A       0x%02x\n", o->a);
	printf("Len:               0x%04x\n", o->len);
	printf("Immediate: B       0x%08x\n", o->b);
}

void emapi_prnt_list_dev(void *ptr)
{
	struct emapi_dev *o = (struct emapi_dev*) ptr;
	printf("%02d - %s\n", o->id, o->name);
}

