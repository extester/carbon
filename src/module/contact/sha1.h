/*
 *  Carbon/Contact module
 *  SHA-1 Hash calculation
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.04.2018 18:02:51
 *      Initial revision.
 */

#ifndef __CONTACT_SHA1_H_INCLUDED__
#define __CONTACT_SHA1_H_INCLUDED__

#include "shell/shell.h"

typedef struct
{
	uint32_t state[5];
	uint32_t count[2];
	unsigned char buffer[64];
} SHA1_CTX;

class CSha1
{
	public:
		CSha1() {}
		~CSha1() {}

	public:
		void sha1(const void* pData, size_t length, void* pSha1);
		void sha1(const char* strData, void* pSha1) {
			return sha1(strData, _tstrlen(strData), pSha1);
		}

	private:
		void SHA1Init(SHA1_CTX * context);
		void SHA1Transform(uint32_t state[5], const unsigned char buffer[64]);
		void SHA1Update(SHA1_CTX * context, const unsigned char *data, uint32_t len);
		void SHA1Final(unsigned char digest[20], SHA1_CTX * context);


};

#endif /* __CONTACT_SHA1_H_INCLUDED__ */
