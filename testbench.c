/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file 		testbench.c
 *
 * @brief 		Code file for testing the EM API commands
 *
 * @details 	This program has a series of tests that can be run to 
 *              demonstrate the functionality of the EM API
 *
 * @copyright 	Copyright (C) 2024 Jackrabbit Founders LLC. All rights reserved.
 *
 * @date 		Feb 2024
 * @author 		Barrett Edwards <code@jrlabs.io>
 *
 */
/* INCLUDES ==================================================================*/

/* printf()
 */
#include <stdio.h>

#include <stdlib.h>

/* memset()
 */
#include <string.h>

/* au_prnt_buf()
 */
#include <arrayutils.h>

#include "main.h"

/* MACROS ====================================================================*/

/* ENUMERATIONS ==============================================================*/

/* STRUCTS ===================================================================*/

/* GLOBAL VARIABLES ==========================================================*/

/* PROTOTYPES ================================================================*/

void print_strings()
{
	int i; 
	i = 0;

	for ( i = 0 ; i < EMOP_MAX; i++ )
		printf("emop %d: %s\n", i, emop(i));	

	for ( i = 0 ; i < EMMT_MAX; i++ )
		printf("emmt %d: %s\n", i, emmt(i));	
	
	for ( i = 0 ; i < EMRC_MAX; i++ )
		printf("emrc %d: %s\n", i, emrc(i));	
}

int verify_object(void * obj, unsigned obj_len, unsigned type, unsigned buf_len)
{
	__u8 *data;

	/* STEPS 
	 * 1: Allocate Memory
	 * 3: Clear memory
	 * 4: Fill in object with test data
	 * 5: Print Object 
	 * 6: Serialize Object
	 * 7: Print the buffer
	 * 8: Clear the object 
	 * 9: Deserialize buffer into object
	 * 10: Free memory
	 */

	// STEP 1: Allocate Memory
	data = (__u8*) malloc(buf_len);

	// STEP 2: Clear memory
	memset(data, 0 , buf_len);

	// STEP 5: Print Object 
	emapi_prnt(obj, type);

	// STEP 6: Serialize Object
	emapi_serialize(data, obj, type, NULL);

	// STEP 6: Print the buffer
	autl_prnt_buf(data, buf_len, 4, 1);
	
	// STEP 7: Clear the object 
	memset(obj, 0 , obj_len);

	// STEP 8: Deserialize buffer into object
	emapi_deserialize(obj, data, type, NULL);

	// STEP 9: Print object
	emapi_prnt(obj, type);

	// STEP 10: Free memory
	free(data);

	return 0;
}

int verify_hdr()
{
	struct emapi_hdr obj; 

	/* STEPS 
	 * 1: Clear memory
	 * 2: Fill in object with test data
	 * 3: Verify object
	 */

	// STEP 1: Clear memory
	memset(&obj, 0 , sizeof(obj));

	// STEP 2: Fill in object with test data
	obj.ver = 0;
	obj.type = EMMT_RSP;
	obj.tag = 0x42;
	obj.rc = 0xCD;
	obj.opcode = 0xAB; 
	obj.len = 0x1FFF; 
	obj.a = 0x23;
	obj.b = 0x12345678;
	
	// STEP 3: Verify object
	return verify_object(&obj, sizeof(obj), EMOB_HDR, EMLN_HDR);
}

int verify_dev()
{
	struct emapi_dev obj;	
	char *name = "Device name";

	/* STEPS 
	 * 1: Clear memory
	 * 2: Fill in object with test data
	 * 3: Verify object
	 */

	// STEP 1: Clear memory
	memset(&obj, 0 , sizeof(obj));

	// STEP 2: Fill in object with test data
	obj.id = 0x21;
	obj.len = strlen(name) + 1;
	sprintf(obj.name, name, obj.len);

	// STEP 3: Verify object
	return verify_object(&obj, sizeof(obj), EMOB_LIST_DEV, obj.len+2);
}

int verify_sizes()
{
	printf("Sizeof:\n");
	printf("struct emapi_hdr:         %lu\n", sizeof(struct emapi_hdr));
	printf("struct emapi_dev:         %lu\n", sizeof(struct emapi_dev));
	return 0;
}

int main(int argc, char **argv)
{
	int i, max;
	const char *names[] = {
		"",
		"fmapi_hdr",					// 1
		"fmapi_dev",					// 2
		"sizeof()"						// 3
	};

	max = 3;

	if (argc > 1)
		i = atoi(argv[1]);
	else {
		for ( i = 0 ; i <= max ; i++ )
			printf("TEST %d: %s\n", i, names[i]);
		goto end;
	}
	if (i > max)
		goto end;

	printf("TEST %d: %s\n", i, names[i]);

	switch(i)
	{
		case EMOB_HDR					: verify_hdr(); 					break;	// 1,  //!< struct emapi_hdr
		case EMOB_LIST_DEV				: verify_dev();  		 			break;	// 2,  //!< struct emapi_dev
		case EMOB_MAX 					: verify_sizes();					break;  // 3,  
		default 						: print_strings();					break;
	}

end:
	return 0;
}
