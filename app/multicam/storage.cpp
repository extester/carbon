/*
 *	IP MultiCamera Test Application
 *	File Storage
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 24.10.2016 11:22:27
 *		Initial revision.
 */

#include "net_media/rtp_playout_buffer_h264.h"

#include "storage.h"


/*******************************************************************************
 * CRtpFileWriter
 */

CRtpFileWriter::CRtpFileWriter(const char* strFilename) :
	CVideoSinkV(),
	m_strFile(strFilename),
	m_nFrames(ZERO_COUNTER),
	m_nErrors(ZERO_COUNTER)
{
	shell_assert(strFilename && *strFilename != '\0');
}

CRtpFileWriter::~CRtpFileWriter()
{
	shell_assert(!m_file.isOpen());
}

void CRtpFileWriter::processNode(CRtpPlayoutNode* pNode)
{
	CRtpPlayoutNodeH264*	pNodeH264;
	uint8_t*				pBuffer;
	size_t					nSize;
	result_t				nresult;

	pNodeH264 = dynamic_cast<CRtpPlayoutNodeH264*>(pNode);
	shell_assert(pNodeH264);

	pBuffer = pNodeH264->getData();
	nSize = pNodeH264->getSize();

	shell_assert(pBuffer != 0);

	if ( nSize > 0 && m_file.isOpen() )  {
		nresult = m_file.write(pBuffer, &nSize);
		if ( nresult == ESUCCESS ) {
			counter_inc(m_nFrames);
		}
		else {
			log_error(L_GEN, "[storage] WRITE FAILUDE, result %d\n", nresult);
			counter_inc(m_nErrors);
		}
	}
}

result_t CRtpFileWriter::init(int nFps, int nRate)
{
	result_t	nresult;

	counter_init(m_nFrames);
	counter_init(m_nErrors);

	nresult = m_file.open(m_strFile, CFile::fileCreate|CFile::fileTruncate|CFile::fileWrite);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[storage] failed to create output file %s, result %d\n",
				  m_strFile.cs(), nresult);
		return nresult;
	}

	log_info(L_GEN, "[storage] storage file open: %s\n", m_strFile.cs());

	nresult = CVideoSinkV::init(nFps, nRate);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[storage] failed to init decoder, result %d\n", nresult);
		m_file.close();
		return nresult;
	}

	return nresult;
}

void CRtpFileWriter::terminate()
{
	m_file.close();
	CVideoSinkV::terminate();

	log_info(L_GEN, "[storage] file %s closed: frames: %d, errors: %d\n",
			 m_strFile.cs(), counter_get(m_nFrames), counter_get(m_nErrors));
}
