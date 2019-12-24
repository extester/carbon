/*
 *  Shell library
 *  Utility functions (UNIX)
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 20.05.2013 11:32:04
 *      Initial revision.
 */

#include <stdio.h>
#ifndef __UCLIBC__
#include <iconv.h>
#endif /* __UNLIBC__ */
#include <unistd.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/utils.h"


/*
 * Convert string encoding
 *
 *		incode		source encoding
 *		outcode		output encoding
 *		inbuf		string to convert
 *		outbuf		output buffer
 *		outsize		output buffer size
 *
 * Return: ESUCCESS, E2BIG - output buffer too short,
 *			EINVAL - conversion not supported, EILSEQ - illegal characters
 */
result_t convertEncoding(const char* incode, const char* outcode,
                     const char* inbuf, size_t insize, char* outbuf, size_t* poutsize)
{
#ifndef __UCLIBC__
    iconv_t		ihandle;
    size_t		size, outsize = *poutsize;
    char*		ss = const_cast<char*>(inbuf);
    result_t    nresult;

    ihandle = iconv_open(outcode, incode);
    if ( ihandle == (iconv_t)-1 )  {
        log_debug(L_GEN, "[utils] conversion from %s to %s not supported, result: %d\n",
                  incode, outcode, errno);
        return errno;
    }

    nresult = ESUCCESS;
#ifdef __FreeBSD__
	size = iconv(ihandle, (const char**)&inbuf, &insize, &outbuf, poutsize);
#else
    size = iconv(ihandle, 				&ss, &insize, &outbuf, poutsize);
#endif
    if ( (signed)size == -1 )  {
        switch(errno)  {
            case EILSEQ:
                log_debug(L_GEN, "[utils] an invalid multibyte sequence is encountered, "
                        "conversion from %s to %s\n", incode, outcode);
                nresult = errno;
                break;

            case EINVAL:
                log_debug(L_GEN, "[utils] an incomplete multibyte sequence is encountered, "
                        "conversion from %s to %s\n", incode, outcode);
                nresult = errno;
                break;

            case E2BIG:
                log_debug(L_GEN, "[utils] output buffer too short, conversion from %s to %s\n",
                       incode, outcode);
                nresult = errno;
                break;

            default:
                nresult = errno ? errno : ENOSYS;
                log_debug(L_GEN, "[utils] unexpected error, conversion from %s to %s, result=%d\n",
                       incode, outcode, nresult);
                break;
        }
    }
    else  {
        *poutsize = outsize - (*poutsize);
    }

    iconv_close(ihandle);
    return nresult;
#else /* __UCLIBC__ */
	UNUSED(incode);
	UNUSED(outcode);
	UNUSED(inbuf);
	UNUSED(insize);
	UNUSED(outbuf);
	UNUSED(poutsize);
	log_error(L_ALWAYS_ENABLED, "convertEncoding() is not implemented\n");
	return EINVAL;
#endif /* __UCLIBC__ */
}

/*
 * Delay execution of the current thread
 *
 *      nSecond     seconds to delay
 */
void sleep_s(unsigned int nSecond)
{
    sleep(nSecond);
}

/*
 * Delay execution of the current thread
 *
 *      nMillisecond        milliseconds to delay
 */
void sleep_ms(unsigned int nMillisecond)
{
    sleep_us(nMillisecond*1000);
}

/*
 * Delay execution of the current thread
 *
 *      nMicrosecond        microseconds to delay
 */
void sleep_us(unsigned int nMicrosecond)
{
    usleep(nMicrosecond);
}
