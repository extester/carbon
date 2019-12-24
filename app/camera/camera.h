/*
 *	IP Camera Test Application
 *	Main module
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 30.09.2016 14:42:16
 *		Initial revision.
 */

#ifndef __CAMERA_H_INCLUDED__
#define __CAMERA_H_INCLUDED__

#define AV_RECORDER				1

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/timer.h"

#include "net_media/media_client.h"
#include "net_media/rtsp_channel_h264.h"
#if AV_RECORDER
#include "net_media/store/mp4_recorder.h"
#else /* AV_RECORDER */
#include "mp4_writer.h"
#endif /* AV_RECORDER */

class CCameraApp : public CApplication, public CStateMachine, public IMediaClient
{
	private:
		CMediaClient*			m_pMediaClient;
		CRtspChannelH264*		m_pVideoChannel;
#if AV_RECORDER
		CMp4Recorder*			m_pRecorder;
#else /* AV_RECORDER */
		CMp4Writer*				m_pRecorder;
#endif /* AV_RECORDER */

    public:
		CCameraApp(int argc, char* argv[]);
        virtual ~CCameraApp();

	public:
        virtual result_t init();
        virtual void terminate();

	protected:
		virtual void initLogger();
		virtual boolean_t processEvent(CEvent* pEvent);

		virtual void onMediaClientConnect(CMediaClient* pClient, result_t nresult);
		virtual void onMediaClientConfigure(CMediaClient* pClient, result_t nresult);
		virtual void onMediaClientPlay(CMediaClient* pClient, result_t nresult);
		virtual void onMediaClientPause(CMediaClient* pClient, result_t nresult);
		virtual void onMediaClientDisconnect(CMediaClient* pClient, result_t nresult);

	private:
		void disconnect();
		void onTerm();
};

#endif /* __CAMERA_H_INCLUDED__ */
