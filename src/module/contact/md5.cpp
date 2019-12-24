/*
 *  Carbon/Contact module
 *  MD5 Hash calculation
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 14.11.2017 15:08:59
 *      Initial revision.
 */

#include "contact/md5.h"

/*******************************************************************************
 * CMd5 class
 */

/*
 * Calculate MD5 hash
 *
 * 		pData		data to calculate
 * 		length		data length, bytes
 * 		strMd5		calculated ASCII hash, buffer 33 bytes long
 */
void CMd5::md5(const void* pData, size_t length, char* strMd5)
{
	md5_state_t	state;
	md5_byte_t	digest[16];
	int			i;

	md5_init(&state);
	md5_append(&state, (const md5_byte_t*)pData, (int)length);
	md5_finish(&state, digest);

	for(i=0; i<16; i++) {
		_tsnprintf(&strMd5[i*2], 3, "%02x", (digest[i])&0xff);
	}
}
