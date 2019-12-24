/*
 *  Carbon/Contact module
 *  ZIP compession/decompression utilities
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 08.11.2017 12:09:56
 *      Initial revision.
 */

#ifndef __CONTACT_ZIP_H_INCLUDED__
#define __CONTACT_ZIP_H_INCLUDED__

#include <zlib.h>

#include "shell/shell.h"

class CZip
{
	public:
		CZip()
		{
		}

		~CZip()
		{
		}

	public:
		result_t compress(void* pDstBuf, size_t* pDstLen, const void* pSrcBuf, size_t nSrcLen, int nLevel);
		result_t decompress(void* pDstBuf, size_t* pDstLen, const void* pSrcBuf, size_t nSrcLen);

	protected:
		result_t zerror2nresult(int zerror);
};

#endif /* __CONTACT_ZIP_H_INCLUDED__ */
