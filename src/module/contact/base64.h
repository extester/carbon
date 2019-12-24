/*
 *  Carbon/Contact module
 *  Base64 encoder/decoder
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.10.2016 17:52:00
 *      Initial revision.
 *
 *
 *  Based on Libb64 1.2.1.
 */

#ifndef __CONTACT_BASE64_H_INCLUDED__
#define __CONTACT_BASE64_H_INCLUDED__

#include "shell/types.h"
#include "shell/error.h"
#include "shell/tstring.h"

typedef enum
{
	step_A, step_B, step_C
} base64_encodestep;

typedef struct
{
	base64_encodestep step;
	signed char result;
	int stepcount;
} base64_encodestate;

typedef enum
{
	step_a, step_b, step_c, step_d
} base64_decodestep;

typedef struct
{
	base64_decodestep step;
	signed char plainchar;
} base64_decodestate;

class CBase64
{
	protected:
		base64_encodestate		m_stateEncode;
		base64_decodestate		m_stateDecode;

		signed char*			m_pBuffer;		/* Result buffer */
		size_t					m_size;			/* Result data size, bytes */

	public:
		CBase64() : m_pBuffer(NULL), m_size(0)
		{
		}

		virtual ~CBase64()
		{
			reset();
		}

	public:
		result_t encode(const void* pRaw, size_t length);
		result_t encode(const char* strRaw) {
			return strRaw ? encode(strRaw, _tstrlen(strRaw)) : EINVAL;
		}

		result_t decode(const void* pEncoded, size_t length);
		result_t decode(const char* strEncoded) {
			return strEncoded ? decode(strEncoded, _tstrlen(strEncoded)) : EINVAL;
		}

		const uint8_t* getData() const { return (const uint8_t*)m_pBuffer; }
		size_t getSize() const { return m_size; }
		void reset();

	private:
		void allocBuffer(size_t size);
};

#endif /* __CONTACT_BASE64_H_INCLUDED__ */
