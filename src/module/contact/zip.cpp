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
 *  Revision 1.0, 08.11.2017 12:16:42
 *      Initial revision.
 */

#include "contact/zip.h"

/*******************************************************************************
 * CZip class
 */

/*
 * Convert zlib error code to carbon error code
 *
 * 		zerror			zlib error code
 *
 * Return: carnon error code, default error code is ENOSYS
 */
result_t CZip::zerror2nresult(int zerror)
{
	result_t	nresult;

	switch ( zerror )  {
		case Z_OK:
			nresult = ESUCCESS;
			break;

		case Z_MEM_ERROR:
			nresult = ENOMEM;
			break;

		case Z_BUF_ERROR:
			nresult = ENOSPC;
			break;

		case Z_STREAM_ERROR:
			nresult = EINVAL;
			break;

		case Z_DATA_ERROR:
			nresult = EFAULT;
			break;

		default:
			nresult = ENOSYS;
			break;
	}

	return nresult;
}

/*
 * Compress data
 *
 * 		pDstBuf			output buffer
 * 		pDstLen			IN: total length of the distination buffer
 * 						OUT: actual size of the compressed data
 *
 *		pSrcBuf			source data
 *		nSrcLen			source data length
 *		nLevel			complession level: Z_NO_COMPRESSION, Z_BEST_SPEED,
 *											Z_BEST_COMPRESSION, Z_DEFAULT_COMPRESSION, ...
 *
 *	 Return: ESUCCESS, ...
 *
 *	 Note:
 *	 	The output buffer pDstLen must be at least: nSrcLen + 0.1%(nSrcLen) + 12
 */
result_t CZip::compress(void* pDstBuf, size_t* pDstLen, const void* pSrcBuf, size_t nSrcLen, int nLevel)
{
	result_t 	nresult;
	int 		zerror;
	uLongf		dLen = *pDstLen;
	uLong		sLen = nSrcLen;

	zerror = ::compress2((Bytef*)pDstBuf, &dLen, (const Bytef*)pSrcBuf, sLen, nLevel);
	if ( zerror == Z_OK )  {
		nresult = ESUCCESS;
		*pDstLen = dLen;
	}
	else {
		nresult = zerror2nresult(zerror);
	}

	return nresult;
}

/*
 * Decompress data
 *
 * 		pDstBuf			output buffer
 * 		pDstLen			IN: total length of the distination buffer
 * 						OUT: actual size of the decompressed data
 *
 *		pSrcBuf			source compressed data
 *		nSrcLen			source data length
 *
 *	 Return: ESUCCESS, ...
 */
result_t CZip::decompress(void* pDstBuf, size_t* pDstLen, const void* pSrcBuf, size_t nSrcLen)
{
	result_t	nresult;
	int 		zerror;
	uLongf		dLen = *pDstLen;
	uLong		sLen = nSrcLen;

	zerror = ::uncompress((Bytef*)pDstBuf, &dLen, (const Bytef*)pSrcBuf, sLen);
	if ( zerror == Z_OK )  {
		nresult = ESUCCESS;
		*pDstLen = dLen;
	}
	else {
		nresult = zerror2nresult(zerror);
	}

	return nresult;
}
