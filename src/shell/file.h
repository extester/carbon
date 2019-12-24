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
 *  Revision 1.0, 09.08.2015 13:16:26
 *      Initial revision.
 *
 *  Revision 1.1, 21.09.2017 16:25:08
 *  	Added CFileAsync::getTcAttr(), CFileAsync::getTcAttr().
 *
 *  Revision 1.2, 26.09.2017 18:34:13
 *  	Added CFileAsync::getInputSpeed(), CFileAsync::setInputSpeed(),
 *  		CFileAsync::getOutputSpeed(), CFileAsync::setOutputSpeed().
 *
 *  Revision 1.3, 09.01.2018 10:32:39
 *  	Changed closed handle form 0 to -1.
 *
 *  Revision 1.4, 28.05.2018 11:04:18
 *  	Renamed setHandle() => attachHandle()/detachHandle(),
 *  	Added setBlocking().
 */

#ifndef __SHELL_FILE_H_INCLUDED__
#define __SHELL_FILE_H_INCLUDED__

#include <termios.h>
#include <unistd.h>

#include "shell/types.h"
#include "shell/hr_time.h"
#include "shell/breaker.h"

#define PATH_SEPARATOR					'/'

/*
 * Asynchronous file I/O operations
 */

class CFileAsync
{
	protected:
		char		m_strFile[256];			/* Full filename or "" */
		int			m_hFile;				/* File handle or -1 */

	public:
		CFileAsync() : m_hFile(-1) { m_strFile[0] = '\0'; }
		virtual ~CFileAsync() { close(); }

	public:
		const char* getName() const { return m_strFile; }
		boolean_t isOpen() const { return m_hFile >= 0; }
		int getHandle() const { return m_hFile; }

		void attachHandle(int hFile);
		void detachHandle() { m_hFile = -1; }

		virtual result_t open(const char* strFile, int options, mode_t mode = 0);
		virtual result_t create(const char* strFile, int options, mode_t mode = 0);
		virtual result_t close();

		virtual result_t setBlocking(boolean_t bBlocking);

		virtual result_t read(void* pBuffer, size_t* pSize);
		virtual result_t readLine(void* pBuffer, size_t nPreSize,
									 size_t* pSize, const char* strEol);
		virtual result_t write(const void* pBuffer, size_t* pSize);

		virtual result_t setPos(off_t offset, int options);
		virtual result_t getPos(off_t* pOffset) const;
		virtual result_t getSize(uint64_t* pSize) const;
		virtual result_t getMode(mode_t* pMode) const;

		virtual result_t ioctl(unsigned long req, void* arg = 0);

		virtual result_t tcFlush(int queue);
		virtual result_t tcDrain();
		virtual result_t getTcAttr(struct termios* pTermios) const;
		virtual result_t setTcAttr(const struct termios* pTermios, int actions = TCSANOW);

		virtual speed_t getInputSpeed(const struct termios* pTermios) const;
		virtual result_t setInputSpeed(struct termios* pTermios, speed_t speed);

		virtual speed_t getOutputSpeed(const struct termios* pTermios) const;
		virtual result_t setOutputSpeed(struct termios* pTermios, speed_t speed);

		virtual result_t createTempFile(char* strTempl = NULL, int options = 0);
};


/*
 * Synchronous file I/O operations
 */
class CFile : public CFileAsync
{
	protected:
		CFileBreaker	m_breaker;

	public:
		enum FileOption {
			fileRead			= 0x0001,		/* open/create for READ */
			fileWrite			= 0x0002,		/* open/create for WRITE */
			fileReadWrite		= 0x0003,		/* open/create for READ and WRITE */
			fileCreate 			= 0x0004,		/* create new file */
			fileAppend			= 0x0008,		/* append mode */
			fileTruncate		= 0x0010,		/* trancate file */
			fileBlocking		= 0x0020,		/* Blocking I/O */
			fileReadFull		= 0x0040,		/* read a specified data length */
			fileNoctty			= 0x0080,
			fileExcl			= 0x0100,

			filePosBegin		= 0x0200,		/* seek from the beginning of the file */
			filePosCur			= 0x0400,		/* seek from the current position */
			filePosEnd			= 0x0800,		/* seek from the end of the file */

			filePollRead		= 0x1000,		/* internal use only */
			filePollWrite		= 0x2000		/* internal use only */
		};

	public:
		CFile() : CFileAsync() {}
		virtual ~CFile() {}

	public:
		virtual result_t read(void* pBuffer, size_t* pSize, int options = 0, hr_time_t hrTimeout = HR_0);
		virtual result_t readLine(void* pBuffer, size_t* pSize, const char* strEol, hr_time_t hrTimeout = HR_0);
		virtual result_t write(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout = HR_0);
		virtual result_t write(const void* pBuffer, size_t size);
		virtual result_t writeLine(const char* strBuffer);

		virtual result_t select(hr_time_t hrTimeout, int options, short* prevents = 0);

		result_t breakerEnable() { return m_breaker.enable(); }
		result_t breakerDisable() { return m_breaker.disable(); }
		void breakerBreak() { m_breaker._break(); }
		void breakerReset() { m_breaker.reset(); }

		/*
		 * Generic static functions
		 */
		static result_t readFile(const char* strFilename, void* pBuffer,
								 size_t nSize, int offset = 0);
		static result_t writeFile(const char* strFilename, const void* pBuffer,
							  size_t nSize, int offset = 0);

		static result_t copyFile(const char* strDestination, const char* strSource);
		static result_t renameFile(const char* strDestination, const char* strSource);

		static result_t makeDir(const char* strPath, mode_t mode = 0);
		static result_t validatePath(const char* strPath, mode_t mode = 0);

		static boolean_t fileExists(const char* strFilename, int access = F_OK);
		static boolean_t pathExists(const char* strPath, int access = F_OK) {
			return fileExists(strPath, access);
		}

		static result_t removeFile(const char* strFilename);
		static result_t unlinkFile(const char* strFilename) {
			return removeFile(strFilename);
		}
		static result_t removeDir(const char* strPath, boolean_t bRemoveFull = FALSE);

		static result_t sizeFile(const char* strFilename, uint64_t* pSize);

	private:
		result_t checkREvents(short revents) const;
};

#endif /* __SHELL_FILE_H_INCLUDED__ */
