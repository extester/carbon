/*
 *  Shell library
 *  File operations
 *
 *  Copyright (c) 2015-2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 09.08.2015 13:17:35
 *      Initial revision.
 *
 *  Revision 1.1, 02.02.2017 16:20:28
 *  	Added CAsyncFile::ioctl().
 *
 *  Revision 1.2, 09.01.2018 10:32:39
 *  	Changed closed handle form 0 to -1.
 *
 *  Revision 1.3, 27.02.2018 16:24:00
 *  	Added flag CFile::fileTruncate to the CFile::writeFile().
 */

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "shell/shell.h"
#include "shell/memory.h"
#include "shell/logger.h"
#include "shell/utils.h"
#include "shell/file.h"


#define FILE_PERM_DEFAULT			(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define DIR_PERM_DEFAULT			(S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)


/*******************************************************************************
 * CFileAsync class
 */

void CFileAsync::attachHandle(int hFile)
{
	shell_assert(hFile >= 0);

	if ( isOpen() )  {
		log_warning(L_FILE, "[file] attached handle to open file!\n");
	}

	m_hFile = hFile;
}

/*
 * Open/create file
 *
 * 		strFile			full filename
 * 		options			open option CFile::fileXXX
 * 		mode			mode for new created file S_IXXX (0-use default FILE_PERM_DEFAULT)
 *
 * Return: ESUCCESS, EACCESS, ...
 *
 * Note: the files are always opened in non-blocking mode
 */
result_t CFileAsync::open(const char* strFile, int options, mode_t mode)
{
	int			retVal, flags = 0;
	mode_t		rMode;
	result_t	nresult;

	switch (options&CFile::fileReadWrite)  {
		case CFile::fileRead:		flags |= O_RDONLY; break;
		case CFile::fileWrite:		flags |= O_WRONLY; break;
		case CFile::fileReadWrite: 	flags |= O_RDWR; break;
		default:
			log_debug(L_FILE, "[file] open (CFileAsync) file %s failed, no READ or WRITE "
					"access option specified\n", strFile);
			return EACCES;
	}

	if ( (options&CFile::fileBlocking) == 0 )	flags |= O_NONBLOCK;
	if ( options&CFile::fileCreate )			flags |= O_CREAT;
	if ( options&CFile::fileAppend )			flags |= O_APPEND;
	if ( options&CFile::fileTruncate )			flags |= O_TRUNC;
	if ( options&CFile::fileNoctty )			flags |= O_NOCTTY;
	if ( options&CFile::fileExcl )				flags |= O_EXCL;

	rMode = mode ? mode : FILE_PERM_DEFAULT;

	retVal = ::open(strFile, flags, rMode);
	if ( retVal >= 0 )  {
		close();
		m_hFile = retVal;
		copyString(m_strFile, strFile, sizeof(m_strFile));
		nresult = ESUCCESS;
	}
	else {
		nresult = errno;
	}

	return nresult;
}

/*
 * Create new file, return error if a file exists
 *
 * 		strFile			full filename
 * 		options			open option CFile::fileXXX
 * 		mode			mode for new created file S_IXXX (0-use default FILE_PERM_DEFAULT)
 *
 * Return: ESUCCESS, ...
 *
 * Note: files are always opened in non-blocking mode
 */
result_t CFileAsync::create(const char* strFile, int options, mode_t mode)
{
	return open(strFile, options|CFile::fileCreate, mode);
}

/*
 * Close file if it was open
 *
 * Return: ESUCCESS, ...
 */
result_t CFileAsync::close()
{
	result_t	nresult = ESUCCESS;

	if ( m_hFile >= 0 )  {
		int		retVal;

		retVal = ::close(m_hFile);
		if ( retVal != 0 )  {
			nresult = errno;
			log_debug(L_FILE, "[file] failed to close file %s, result: %d\n", m_strFile, nresult);
		}

		m_hFile = -1;
		m_strFile[0] = '\0';
	}

	return nresult;
}

result_t CFileAsync::setBlocking(boolean_t bBlocking)
{
	int 		flags, retVal;
	result_t	nresult = EBADF;

	if ( isOpen() )  {
		if ( bBlocking )  {
			flags = ::fcntl(m_hFile, F_GETFL);
			flags &= ~O_NONBLOCK;
			retVal = ::fcntl(m_hFile, F_SETFL, flags);
			nresult = retVal == 0 ? ESUCCESS : errno;
		}
		else {
			flags = ::fcntl(m_hFile, F_GETFL);
			flags |= O_NONBLOCK;
			retVal = ::fcntl(m_hFile, F_SETFL, flags);
			nresult = retVal == 0 ? ESUCCESS : errno;
		}
	}

	return nresult;
}


/*
 * Set current file position pointer
 *
 * 		offset			position
 * 		options			calculate position from: filePosBegin/Cur/End
 *
 * Return:
 * 		ESUCCESS		success
 * 		EBADF			file is not open
 * 		EINVAL			invalid parameter(s)
 * 		...
 */
result_t CFileAsync::setPos(off_t offset, int options)
{
	result_t	nresult = EBADF;

	if ( isOpen() )  {
		int		whence = SEEK_SET;
		off_t	retVal;

		nresult = ESUCCESS;
		switch ( options )  {
			case CFile::filePosBegin:
				whence = SEEK_SET;
				break;

			case CFile::filePosCur:
				whence = SEEK_CUR;
				break;

			case CFile::filePosEnd:
				whence = SEEK_END;
				break;

			default:
				log_debug(L_FILE, "[file] invalid SEEK option %u, file: %s\n", options, m_strFile);
				nresult = EINVAL;
				break;
		}

		if ( nresult == ESUCCESS )  {
			retVal = ::lseek(m_hFile, offset, whence);
			if ( retVal == -1 )  {
				nresult = errno;
				log_debug(L_FILE, "[file] lseek() failed, file %s, offset %u, where %d, result %d\n",
						  m_strFile, offset, whence, nresult);
			}
		}
	}

	return nresult;
}

/*
 * Get current position in the file
 *
 * 		pOffset			current position [out]
 *
 * Return: ESUCCESS, EBADF, ...
 */
result_t CFileAsync::getPos(off_t* pOffset) const
{
	off_t		offset;
	result_t	nresult = EBADF;

	if ( isOpen() )  {
		offset = ::lseek(m_hFile, 0, SEEK_CUR);
		if ( offset >= 0 )  {
			*pOffset = offset;
			nresult = ESUCCESS;
		}
		else {
			nresult = errno;
			log_debug(L_FILE, "[file] failed to get position, file: %s, result: %d\n",
					  m_strFile, nresult);
		}
	}

	return nresult;
}

/*
 * Get file size
 *
 * 		pSize		file size in bytes [out]
 *
 * Return: ESUCCESS, EBADF, ...
 */
result_t CFileAsync::getSize(uint64_t* pSize) const
{
	struct stat		fileStat;
	int				retVal;
	result_t		nresult = EBADF;

	if ( isOpen() ) {
		retVal = ::fstat(m_hFile, &fileStat);
		if ( retVal == 0 )  {
			*pSize = (uint64_t)fileStat.st_size;
			nresult = ESUCCESS;
		}
		else {
			nresult = errno;
			*pSize = 0;
			log_debug(L_FILE, "[file] failed to get size, file %s, result: %d\n",
					  m_strFile, nresult);
		}
	}

	return nresult;
}

/*
 * Get file permissions
 *
 * 		pMode		file permissions, [out]
 *
 * Return: ESUCCESS, EBADF, ...
 */
result_t CFileAsync::getMode(mode_t* pMode) const
{
	struct stat		fileStat;
	int				retVal;
	result_t		nresult = EBADF;

	if ( isOpen() ) {
		retVal = ::fstat(m_hFile, &fileStat);
		if ( retVal == 0 )  {
			*pMode = fileStat.st_mode&(~S_IFMT);
			nresult = ESUCCESS;
		}
		else {
			nresult = errno;
			*pMode = 0;
			log_debug(L_FILE, "[file] failed to get permission, file %s, result: %d\n",
					  m_strFile, nresult);
		}
	}

	return nresult;
}

/*
 * Read up to specified bytes asynchronous
 *
 * 		pBuffer			output bytes
 * 		pSize			IN: maximum bytes to read,
 * 						OUT: read bytes
 *
 * Return:
 * 		ESUCCESS		full data buffer has been read
 * 		EAGAIN			partial data has been read
 * 		EBADF			file is not open
 * 		ENODATA			end of file was reached
 * 		...
 */
result_t CFileAsync::read(void* pBuffer, size_t* pSize)
{
	size_t		length, size, l;
	ssize_t		len;
	char*		p = (char*)pBuffer;
	result_t	nresult;

	//log_debug(L_FILE_FL, "[file] read %d bytes\n", *pSize);

	if ( !isOpen() )  {
		*pSize = 0;
		return EBADF;
	}

	size = *pSize;
	length = 0;
	nresult = ESUCCESS;

	while( length < size )  {

		l = size-length;
		len = ::read(m_hFile, p+length, l);

		if ( len < 0 )  {
			nresult = errno;
			if ( nresult != EAGAIN )  {
				/* Read error */
				log_debug(L_FILE, "[file] read failed, file: %s, result=%d\n", m_strFile, nresult);
			}
			break;
		}

		if ( len == 0 )  {
			/* End of file */
			log_trace(L_FILE, "[file] file %s, end of file reached\n", m_strFile);
			nresult = ENODATA;
			break;
		}

		//log_debug(L_FILE_FL, "[file] read a byte '%d'\n", *(p+length));

		length += len;
	}

	*pSize = length;

	//log_debug(L_FILE_FL, "file read %d bytes\n", *pSize);
	//log_debug_bin(L_FILE_FL, pBuffer, *pSize, "data: ");
	return nresult;
}

/*
 * Read line
 *
 * 		pBuffer			output buffer
 * 		nPreSize		filled bytes of the output buffer
 * 		pSize			IN: maximum bytes to read,
 * 						OUT: read bytes
 * 		strEol			eol marker
 *
 * Return:
 * 		ESUCCESS		successfully read (maximum bytes or up to eol)
 * 		EBADF			file is not open
 * 		EAGAIN			partial data has been read
 * 		ENODATA			eof of file reached
 * 		...
 */
result_t CFileAsync::readLine(void* pBuffer, size_t nPreSize, size_t* pSize, const char* strEol)
{
	size_t		size, length, lenEol;
	ssize_t		len;
	char*		p = (char*)pBuffer + nPreSize;
	result_t	nresult;

	if ( !isOpen() )  {
		*pSize = 0;
		return EBADF;
	}

	lenEol = _tstrlen(strEol);
	size = *pSize;
	length = 0;
	nresult = ESUCCESS;

	//log_debug(L_FILE_FL, "[file] read, presize %d, size %d\n", nPreSize, *pSize);

	while( length < size )  {
		len = ::read(m_hFile, p+length, 1);

		if ( len < 0 )  {
			nresult = errno;
			if ( nresult != EAGAIN )  {
				/* Read error */
				log_debug(L_FILE, "[file] read failed, file: %s, result: %d\n",
						  m_strFile, nresult);
			}
			break;
		}

		if ( len == 0 )  {
			/* Eof detected */
			log_trace(L_FILE, "[file] file %s, end of file reached\n", m_strFile);
			nresult = length > 0 ? ESUCCESS : ENODATA;
			break;
		}

		/*log_debug(L_FILE_FL, "[file] read a byte '%c'\n", *(p+length));*/

		length += len;
		if ( (length+nPreSize) >= lenEol )  {
			/* Check for eol */
			if ( _tmemcmp((char*)pBuffer+nPreSize+length-lenEol, strEol, lenEol) == 0 )  {
				//log_debug(L_FILE_FL, "[file] EOL detected\n");
				break;	/* Done */
			}
		}
	}

	*pSize = length;
	return nresult;
}

/*
 * Write up to specified bytes asynchronous
 *
 * 		pBuffer			data buffer
 * 		pSize			IN: maximum bytes to write,
 * 						OUT: written bytes
 *
 * Return:
 * 		ESUCCESS		full data buffer has been written
 * 		EAGAIN			partial data has been written
 * 		EBADF			file is not open
 * 		ENOSPC			can't write data (possible out of space)
 * 		...
 */
result_t CFileAsync::write(const void* pBuffer, size_t* pSize)
{
	size_t		length, size, l;
	ssize_t		len;
	result_t	nresult;

	if ( !isOpen() )  {
		*pSize = 0;
		return EBADF;
	}

	size = *pSize;
	length = 0;
	nresult = ESUCCESS;

	while( length < size )  {
		l = size-length;
		len = ::write(m_hFile, (const char*)pBuffer+length, l);
		if ( len < 0 )  {
			nresult = errno;
			if ( errno != EAGAIN )  {
				log_debug(L_FILE, "[file] write %d bytes failed, file: %s, result: %d\n",
						  l, m_strFile, nresult);
			}
			break;
		}

		if ( len == 0 )  {
			log_debug(L_FILE, "[file] write %d bytes returns ZERO, file: %s, set ENOSPC error\n",
					  l, m_strFile);
			nresult = ENOSPC;
			break;
		}

		length += len;
	}

	*pSize = length;
	return nresult;
}

/*
 * Execute a control request on device
 *
 * 		req		request ID to perform
 * 		arg		optional argument
 *
 * Return: ESUCCESS, ...
 */
result_t CFileAsync::ioctl(unsigned long req, void* arg)
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	if ( !isOpen() )  {
		return EBADF;
	}

	retVal = ::ioctl(m_hFile, req, arg);
	if ( retVal == -1 )  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Flush non-transmitted output data, non-read input data or both
 *
 * 		queue		TCIFLUSH flush received but not read data
 * 					TCOFLUSH flush data written but not transmitted
 * 					TCIOFLUSH flush both
 *
 * Return: ESUCCESS, ...
 */
result_t CFileAsync::tcFlush(int queue)
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	if ( !isOpen() )  {
		return EBADF;
	}

	retVal = ::tcflush(m_hFile, queue);
	if ( retVal < 0 )  {
		nresult = errno;
	}

	return nresult;
}

result_t CFileAsync::tcDrain()
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	if ( !isOpen() )  {
		return EBADF;
	}

	retVal = ::tcdrain(m_hFile);
	if ( retVal < 0 )  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Get communication part attributes
 *
 * 		pTermios	attributes [out]
 *
 * Return: ESUCCESS, ...
 */
result_t CFileAsync::getTcAttr(struct termios* pTermios) const
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	shell_assert(pTermios);

	if ( !isOpen() )  {
		return EBADF;
	}

	retVal = ::tcgetattr(m_hFile, pTermios);
	if ( retVal < 0 )  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Set communication port attributes
 *
 * 		pTermios	attributes to set
 * 		actions		specifies when the changes take effect (TCSANOW, TCSADRAIN, TCSAFLUSH)
 *
 * Return: ESUCCESS, ...
 */
result_t CFileAsync::setTcAttr(const struct termios* pTermios, int actions)
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	shell_assert(pTermios);

	if ( !isOpen() )  {
		return EBADF;
	}

	retVal = ::tcsetattr(m_hFile, actions, pTermios);
	if ( retVal < 0 )  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Get input baud rate in termios structure
 *
 * 		pTermios		attributes
 *
 * Return: input baud rate, Bxxx
 */
speed_t CFileAsync::getInputSpeed(const struct termios* pTermios) const
{
	speed_t		speed;

	speed = ::cfgetispeed(pTermios);
	return speed;
}

/*
 * Set input baud rate to the termios structure
 *
 * 		pTermios		attributes
 * 		speed			baud rate, Bxxx
 *
 * Return: ESUCCESS, ...
 */
result_t CFileAsync::setInputSpeed(struct termios* pTermios, speed_t speed)
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	retVal = ::cfsetispeed(pTermios, speed);
	if ( retVal < 0 )  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Get output baud rate in termios structure
 *
 * 		pTermios		attributes
 *
 * Return: output baud rate, Bxxx
 */
speed_t CFileAsync::getOutputSpeed(const struct termios* pTermios) const
{
	speed_t		speed;

	speed = ::cfgetospeed(pTermios);
	return speed;
}

/*
 * Set input baud rate to the termios structure
 *
 * 		pTermios		attributes
 * 		speed			baud rate, Bxxx
 *
 * Return: ESUCCESS, ...
 */
result_t CFileAsync::setOutputSpeed(struct termios* pTermios, speed_t speed)
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	retVal = ::cfsetospeed(pTermios, speed);
	if ( retVal < 0 )  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Create temporary file
 *
 * 		strTempl		writable string, template, last 6 characters are 'X' (optional, may be 0)
 * 		options			open options: fileAppend (optional, default 0)
 *
 * Return: ESUCCESS, ...
 */
result_t CFileAsync::createTempFile(char* strTempl, int options)
{
	int		flags, retVal;
	char	templ[] = "/tmp/cXXXXXX", *pTempl;

	pTempl = strTempl ? strTempl : templ;

	flags = 0;
	if ( options&CFile::fileAppend )	flags |= O_APPEND;

	retVal = ::mkostemp(pTempl, flags);
	if ( retVal == -1 ) {
		return errno;
	}

	if ( !(options&CFile::fileBlocking) )  {
		int		fl;

		fl = ::fcntl(retVal, F_GETFL, 0);
		fcntl(retVal, F_SETFL, fl|O_NONBLOCK);
	}

	close();
	m_hFile = retVal;
	copyString(m_strFile, pTempl, sizeof(m_strFile));

	return ESUCCESS;
}

/*******************************************************************************
 * CFile class
 */

/*
 * Awaiting for the I/O activity on the file
 *
 * 		hrTimeout		awaiting timeout
 * 		options			awaiting event, filePollRead or filePollWrite
 *
 * Return:
 * 		ESUCCESS		i/o detected
 * 		EBADF			file is not open
 * 		ETIMEDOUT		no i/o detected
 * 		EINVAL			invalid parameter(s)
 *		ECANCEL			manual cancel
 *		EINTR			interrupted by signal
 */
result_t CFile::select(hr_time_t hrTimeout, int options, short* prevents)
{
	struct pollfd       pfd[2];
	hr_time_t           hrStart, hrElapsed;
	int                 msTimeout, n;
	nfds_t				nfds;
	short				chevents;
	result_t			nresult;

	if ( !isOpen() )  {
		return EBADF;
	}

	shell_assert((options&(CFile::filePollRead|CFile::filePollWrite)) != 0);
	if ( (options&(CFile::filePollRead|CFile::filePollWrite)) == 0 )  {
		log_debug(L_FILE, "[file] no polling mode specified, file: %s\n", m_strFile);
		return EINVAL;
	}

	nresult = ETIMEDOUT;
	hrStart = hr_time_now();

	while ( (hrElapsed=hr_time_get_elapsed(hrStart)) < hrTimeout ) {
		msTimeout = (int)HR_TIME_TO_MILLISECONDS(hrTimeout-hrElapsed);
		if ( msTimeout < 1 )  {
			break;
		}

		pfd[0].fd = m_hFile;
		pfd[0].events = 0;
		if ( options&CFile::filePollRead ) {
			pfd[0].events |= POLLIN|POLLPRI;
		}
		if ( options&CFile::filePollWrite )  {
			pfd[0].events |= POLLOUT;
		}
		pfd[0].revents = 0;

		nfds = 1;
		if ( m_breaker.isEnabled() )  {
			pfd[1].fd = m_breaker.getRHandle();
			pfd[1].events = POLLIN;
			pfd[1].revents = 0;
			nfds++;
		}

		n = ::poll(pfd, nfds, msTimeout);
		if ( n == 0 )  {
			/* Timeout and no files are ready */
			break;
		}

		if ( n < 0 )  {
			/* Polling failed */
			nresult = errno;
			if ( nresult != EINTR ) {
				log_debug(L_FILE, "[file] polling failed, file: %s, result: %d\n",
						  m_strFile, nresult);
			}
			break;
		}

		if ( m_breaker.isEnabled() && (pfd[1].revents&(POLLIN|POLLPRI)) )  {
			/* Canceled */
			//log_debug(L_FILE_FL, "[file] poll cancelled, file: %s\n", m_strFile);
			m_breaker.reset();
			nresult = ECANCELED;
			break;
		}

		/*
		 * Check events
		 */
		chevents = 0;
		if ( options&CFile::filePollRead ) {
			chevents |= POLLIN|POLLPRI;
		}
		if ( options&CFile::filePollWrite ) {
			chevents |= POLLOUT;
		}

		if ( pfd[0].revents&chevents )  {
			/* Success */
			if ( prevents )  {
				*prevents = pfd[0].revents;
			}
			nresult = ESUCCESS;
			break;
		}
	}

	return nresult;
}

/*
 * Check returned events from the CFile::select() and
 * return appropriate result code
 *
 * 		revents		returned events from the select()
 *
 * Return: ESUCCESS, EINVAL, EIO
 */
result_t CFile::checkREvents(short revents) const
{
	result_t	nresult = ESUCCESS;

	if ( (revents&POLLHUP) != 0 )  {
		nresult = EIO;
	} else
	if ( (revents&POLLNVAL) != 0 )  {
		nresult = EINVAL;
	} else
	if ( (revents&POLLERR) != 0 )  {
		nresult = EIO;
	}

	return nresult;
}

/*
 * Read file
 *
 * 		pBuffer			output buffer
 * 		pSize			IN: maximum bytes to read,
 * 						OUT: read bytes
 * 		options			read options (supported: fileReadFull)
 * 		hrTimeout		maximum timeout (HR_0: read forever)
 *
 * Return:
 * 		ESUCCESS		data buffer has been read (full or partial depends of options)
 * 		EBADF			file is not open
 * 		ETIMEDOUT		timeout error (*pSize contains read bytes)
 * 		ENODATA			end of file reached (zero or more bytes read)
 * 		EINTR			interrupted by signal
 * 		ECANCELED		manual cancel
 * 		...
 */
result_t CFile::read(void* pBuffer, size_t* pSize, int options, hr_time_t hrTimeout)
{
	uint8_t*		pBuf = (uint8_t*)pBuffer;
	size_t			size, length;
	result_t		nresult;
	hr_time_t		hrStart, hrIterTimeout;
	short			revents;

	if ( !isOpen() )  {
		return EBADF;
	}

	length = 0;
	hrStart = hr_time_now();
	nresult = (*pSize) ? EAGAIN : ESUCCESS;

	while ( *pSize > length && nresult == EAGAIN )  {
		hrIterTimeout = hrTimeout ? hr_timeout(hrStart, hrTimeout) : HR_FOREVER;

		nresult = select(hrIterTimeout, CFile::filePollRead, &revents);
		if ( nresult == ESUCCESS )  {
			size = (*pSize) - length;
			nresult = CFileAsync::read(pBuf+length, &size);
			length += size;

			if ( length > 0 && !(options&CFile::fileReadFull) )  {
				if ( nresult == EAGAIN || nresult == ENODATA )  {
					nresult = ESUCCESS;
				}
			}
		}
	}

	*pSize = length;
	return nresult;
}

/*
 * Read line
 *
 * 		pBuffer			output buffer
 * 		pSize			IN: maximum bytes to read,
 * 						OUT: read bytes
 * 		strEol			eol marker
 * 		hrTimeout		maximum timeout (HR_0: read forever)
 *
 * Return:
 * 		ESUCCESS		successfully read (maximum bytes or up to eol)
 * 		EBADF			file is not open
 * 		ENODATA			eof of file reached
 * 		EINTR			interrupted by signal
 * 		ECANCELED		manual cancel
 * 		...
 *
 * Note: The result string contains trailing '\0'
 */
result_t CFile::readLine(void* pBuffer, size_t* pSize, const char* strEol, hr_time_t hrTimeout)
{
	uint8_t*		pBuf = (uint8_t*)pBuffer;
	size_t			lenEol, length, szPre, size;
	result_t		nresult;
	hr_time_t		hrStart, hrIterTimeout;
	short			revents;

	if ( !isOpen() )  {
		return EBADF;
	}

	if ( (*pSize) < 2 )  {
		if ( (*pSize) > 0 )  {
			*((char*)pBuffer) = '\0';
		}
		*pSize = 0;
		return ESUCCESS;
	}

	length = 0;
	(*pSize)--;
	lenEol = _tstrlen(strEol);
	hrStart = hr_time_now();
	nresult = (*pSize) ? EAGAIN : ESUCCESS;

	while ( *pSize > length && nresult == EAGAIN )  {
		hrIterTimeout = hrTimeout ? hr_timeout(hrStart, hrTimeout) : HR_FOREVER;

		nresult = select(hrIterTimeout, CFile::filePollRead, &revents);
		if ( nresult == ESUCCESS )  {
			szPre = MIN(length, lenEol);
			size = (*pSize) - length;
			nresult = CFileAsync::readLine(pBuf+length-szPre, szPre, &size, strEol);
			length += size;
		}
	}

	((char*)pBuffer)[length] = '\0';
	*pSize = length;
	return nresult;
}

/*
 * Write file
 *
 * 		pBuffer			data buffer
 * 		pSize			IN: maximum bytes to write,
 * 						OUT: written bytes
 * 		hrTimeout		maximum timeout (HR_0: write forever)
 *
 * Return:
 * 		ESUCCESS		full data buffer has been successfully written
 * 		EINTR			interrupted by signal
 * 		ECANCELED		manual cancel
 * 		EBADF			file is not open
 * 		ENOSPC			can't write data (possible out of space)
 * 		...
 */
result_t CFile::write(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout)
{
	const uint8_t*	pBuf = (const uint8_t*)pBuffer;
	size_t			size, length;
	result_t		nresult;
	hr_time_t		hrStart, hrIterTimeout;
	short			revents;

	if ( !isOpen() )  {
		return EBADF;
	}

	length = 0;
	hrStart = hr_time_now();
	nresult = (*pSize) ? EAGAIN : ESUCCESS;

	while ( *pSize > length && nresult == EAGAIN )  {
		hrIterTimeout = hrTimeout ? hr_timeout(hrStart, hrTimeout) : HR_FOREVER;

		nresult = select(hrIterTimeout, CFile::filePollWrite, &revents);
		if ( nresult == ESUCCESS )  {
			size = (*pSize) - length;
			nresult = CFileAsync::write(pBuf+length, &size);
			length += size;
		}
	}

	*pSize = length;
	return nresult;
}

/*
 * Write file
 *
 * 		pBuffer		data to write
 * 		size		data size to write, bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CFile::write(const void* pBuffer, size_t size)
{
	size_t		size2;
	result_t	nresult;

	size2 = size;
	nresult = write(pBuffer, &size2, HR_0);

	return nresult;
}

result_t CFile::writeLine(const char* strBuffer)
{
	size_t		length;
	result_t	nresult;

	length = _tstrlen(strBuffer);
	nresult = write(strBuffer, length);

	return nresult;
}

/*******************************************************************************
 * Generic file I/O operations (static)
 */

/*
 * Read file
 *
 * 		strFilename			full filename
 * 		pBuffer				output buffer
 * 		nSize				bytes to read
 * 		offset				file offset from the beginning
 *
 * Return: ESUCCESS, ...
 */
result_t CFile::readFile(const char* strFilename, void* pBuffer, size_t nSize, int offset)
{
	CFile		file;
	size_t		size;
	result_t	nresult;

	shell_assert(strFilename && *strFilename != '\0');
	shell_assert(pBuffer);

	nresult = file.open(strFilename, CFile::fileRead);
	if ( nresult == ESUCCESS )  {
		if ( offset )  {
			nresult = file.setPos(offset, CFile::filePosBegin);
		}
		if ( nresult == ESUCCESS )  {
			size = nSize;
			nresult = file.read(pBuffer, &size, CFile::fileReadFull);
		}
		file.close();
	}

	return nresult;
}

/*
 * Write file
 *
 * 		strFilename			full filename
 * 		pBuffer				input buffer
 * 		nSize				bytes to write
 * 		offset				file offset from the beginning
 *
 * Return: ESUCCESS, ...
 */
result_t CFile::writeFile(const char* strFilename, const void* pBuffer, size_t nSize, int offset)
{
	CFile		file;
	size_t		size;
	result_t	nresult;

	shell_assert(strFilename && *strFilename != '\0');
	shell_assert(pBuffer);

	nresult = file.open(strFilename, CFile::fileWrite|CFile::fileCreate|CFile::fileTruncate);
	if ( nresult == ESUCCESS )  {
		if ( offset )  {
			nresult = file.setPos(offset, CFile::filePosBegin);
		}
		if ( nresult == ESUCCESS )  {
			size = nSize;
			nresult = file.write(pBuffer, &size);
		}
		file.close();
	}

	return nresult;
}

#define COPYFILE_CHUNK_SIZE		(256*1024)

/*
 * Copy file
 *
 * 		strDestination			destination file (will be overwrite if exists)
 * 		strSource				source file
 *
 * Return: ESUCCESS, ENOMEM, ...
 */
result_t CFile::copyFile(const char* strDestination, const char* strSource)
{
	CFile		src, dst;
	void*		pBuffer;
	uint64_t	szFile;
	size_t		szAlloc, size;
	mode_t		mode;
	result_t	nresult, nr;

	nresult = src.open(strSource, CFile::fileRead);
	if ( nresult != ESUCCESS )  {
		log_trace(L_FILE, "[file] failed to open source file %s, result: %d\n",
				  strSource, nresult);
		return nresult;
	}

	nresult = src.getSize(&szFile);
	if ( nresult != ESUCCESS )  {
		src.close();
		log_trace(L_FILE, "[file] failed to get source file size, result: %d\n", nresult);
		return nresult;
	}

	nresult = src.getMode(&mode);
	if ( nresult != ESUCCESS )  {
		mode = FILE_PERM_DEFAULT;
		log_warning(L_FILE, "[file] failed to get source file mode, file: %s, result: %d\n",
					strSource, nresult);
	}

	szAlloc = (size_t)MIN(COPYFILE_CHUNK_SIZE, szFile);
	pBuffer = memAlloc(szAlloc);
	if ( pBuffer == NULL )  {
		log_debug(L_GEN, "[file] failed to allocate buffer of %d bytes\n", szFile);
		src.close();
		return ENOMEM;
	}

	nresult = dst.create(strDestination, CFile::fileWrite|CFile::fileTruncate, mode);
	if ( nresult != ESUCCESS )  {
		src.close();
		memFree(pBuffer);
		log_trace(L_FILE, "[file] failed to create destination file %s, result: %d\n",
				  strDestination, nresult);
		return nresult;
	}

	while ( nresult == ESUCCESS ) {
		size = szAlloc;
		nresult = src.read(pBuffer, &size);
		if ( nresult == ESUCCESS || nresult == ENODATA/*eof*/ ) {
			nr = dst.write(pBuffer, &size);
			if ( nr != ESUCCESS ) {
				nresult = nr;
			}
		}
	}

	if ( nresult == ENODATA )  {
		nresult = ESUCCESS;
	}

	src.close();
	dst.close();
	memFree(pBuffer);

	return nresult;
}

/*
 * Rename file
 *
 * 		strDestination			destination file (will be overwrite if exists)
 * 		strSource				source file
 *
 * Return: ESUCCESS, ...
 */
result_t CFile::renameFile(const char* strDestination, const char* strSource)
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	retVal = ::rename(strSource, strDestination);
	if ( retVal != 0 )  {
		nresult = errno;
		log_debug(L_FILE, "[file] failed to rename file: %s => %s, result: %s(%d)\n",
				  strSource, strDestination, strerror(nresult), nresult);
	}

	return nresult;
}

/*
 * Create directory
 *
 * 		strPath			full path (intermediate paths must be exist)
 * 		mode			mode for the directory S_IXXX (0-use default DIR_PERM_DEFAULT)
 *
 * Return: ESUCCESS, ...
 */
result_t CFile::makeDir(const char* strPath, mode_t mode)
{
	mode_t		realMode = mode ? mode : DIR_PERM_DEFAULT;
	int 		retVal;
	result_t	nresult = ESUCCESS;

	retVal = ::mkdir(strPath, realMode);
	if ( retVal < 0 )  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Create directory and all intermediate directories
 *
 * 		strPath			full path (intermediate paths must be exist)
 * 		mode			mode for the new directories S_IXXX (0-use default DIR_PERM_DEFAULT)
 *
 * Return: ESUCCESS, ...
 */
result_t CFile::validatePath(const char* strPath, mode_t mode)
{
	result_t	nresult;

	nresult = makeDir(strPath, mode);
	if ( nresult != ESUCCESS && nresult != EEXIST )  {
		size_t			length;
		const char*		p;
		char*			strTmp;

		p = _tstrrchr(strPath, PATH_SEPARATOR);
		if ( p && p != strPath )  {
			length = A(p) - A(strPath);
			strTmp = (char*)memAlloca(length+1);
			UNALIGNED_MEMCPY(strTmp, strPath, length);
			strTmp[length] = '\0';

			nresult = validatePath(strTmp, mode);
			if ( nresult == ESUCCESS || nresult == EEXIST )  {
				nresult = makeDir(strPath, mode);
			}
		}
	}

	return nresult == EEXIST ? ESUCCESS : nresult;
}

/*
 * Check if a file exists
 *
 * 		strFilename			full filename
 * 		access				required access mode (OR allowed):
 * 							F_OK	file exists
 * 							R_OK	file exists and readable
 * 							W_OK	file exists and writable
 *							X_OK	file exists and can execute
 * Return:
 * 		TRUE		file exists
 * 		FALSE		file doesn't exist (or error occurred)
 */
boolean_t CFile::fileExists(const char* strFilename, int access)
{
	int			retVal;
	boolean_t	bresult;

	retVal = ::access(strFilename, access);
	bresult = retVal == 0;
	return bresult;
}

/*
 * Delete file
 *
 * 		strFilename			full filename
 *
 * Return: ESUCCESS, ...
 */
result_t CFile::removeFile(const char* strFilename)
{
	result_t	nresult = ESUCCESS;
	int			retVal;

	retVal = ::unlink(strFilename);
	if ( retVal < 0 )  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Delete directory
 *
 * 		strPath			full directory path
 * 		bRemoveFull		TRUE: remove all subdirectories and files recursive
 * 						FALSE: remove empty directory only
 *
 * Return: ESUCCESS, ...
 */
result_t CFile::removeDir(const char* strPath, boolean_t bRemoveFull)
{
	char		strTmp[PATH_MAX];
	result_t	nresult = ESUCCESS;

	if ( bRemoveFull )  {
		DIR*			dir;
		struct dirent*	el;

		dir = ::opendir(strPath);
		if ( dir )  {
			while ( (el=::readdir(dir)) != NULL && nresult == ESUCCESS )  {
				copyString(strTmp, strPath, sizeof(strTmp));
				appendPath(strTmp, el->d_name, sizeof(strTmp));

				if ( el->d_type == DT_DIR )  {
					if ( _tstrcmp(el->d_name, ".") != 0 && _tstrcmp(el->d_name, "..") != 0 ) {
						nresult = removeDir(strTmp, TRUE);
					}
				}
				else {
					nresult = removeFile(strTmp);
				}
			}

			::closedir(dir);
		}
		else {
			nresult = errno;
			log_trace(L_FILE, "[file] can't open directory %s, result %d\n",
					  strPath, nresult);
		}
	}

	if ( nresult == ESUCCESS )  {
		int 	retVal;

		retVal = ::rmdir(strPath);
		if ( retVal < 0 )  {
			nresult = errno;
			log_trace(L_FILE, "[file] can't remove directory %s, result %d\n",
					  	strPath, nresult);
		}
	}

	return nresult;
}

/*
 * Get file size
 *
 * 		strFilename		full filename
 * 		pSize			file size in bytes [out]
 *
 * Return: ESUCCESS, ...
 */
result_t CFile::sizeFile(const char* strFilename, uint64_t* pSize)
{
	struct stat		fileStat;
	int				retVal;
	result_t		nresult = ESUCCESS;

	retVal = ::stat(strFilename, &fileStat);
	if ( retVal == 0 )  {
		*pSize = (uint64_t)fileStat.st_size;
	}
	else {
		nresult = errno;
		log_trace(L_FILE, "[file] failed to get size, file %s, result: %d\n",
					  strFilename, nresult);
	}

	return nresult;
}
