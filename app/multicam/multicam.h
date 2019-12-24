/*
 *	IP MultiCamera Test Application
 *	Main module
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 01.11.2016 14:46:46
 *		Initial revision.
 */

#ifndef __MULTICAM_H_INCLUDED__
#define __MULTICAM_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"

#include "net_media/rtp_video_h264.h"

#include "storage.h"

typedef struct {
	uint32_t			id;				/* Camera Id */

	const char 			strUrl[128];	/* RTSP URL */
	const char			strIp[128];		/* RTSP Server IP */
	ip_port_t			nRtpPort;		/* RTP port */

	CRtpFileWriter*		pWriter;		/* Video Writer or 0 */
	CRtpVideoH264*		pCamera;		/* Camera object */
} camera_t;

class CMultiCamApp : public CApplication, public IRtpVideoH264Result
{
	private:
		static camera_t		m_arCamera[];
		uint32_t			m_bmpCamera;
		uint32_t			m_bmpStop;
		boolean_t			m_bStopping;

    public:
		CMultiCamApp(int argc, char* argv[]);
        virtual ~CMultiCamApp();

        virtual result_t init();
        virtual void terminate();

	protected:
		virtual void initLogger();
		virtual boolean_t processEvent(CEvent* pEvent);

		virtual void onRtpVideoH264Stop(CRtpVideoH264* pCamera);

	private:
		int getCameraIndex(CRtpVideoH264* pClient) const;
		void startCameras();
		void stopCameras();

		void onTerm();
		void onUsr1();
};

#endif /* __MULTICAM_H_INCLUDED__ */
