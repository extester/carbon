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
 *	Revision 1.0, 30.09.2016 14:43:13
 *		Initial revision.
 */

#include "carbon/net_server/net_client.h"

#include "camera.h"

#define CAMERA_FPS					25
#define CAMERA_RTP_PORT				54454

#define CAMERA_STORE_FILENAME		"camera.mp4"

//#define RTSP_URL					"rtsp://192.168.1.10:554/user=admin_password=tlJwpbo6_channel=1_stream=0.sdp?real_stream"
//#define RTSP_URL					"rtsp://192.168.1.10:554/user=admin_password=_channel=1_stream=0"
//#define RTSP_URL					"rtsp://192.168.1.10:554/11"

/* stream=0.sdp, 0 - main stream, 1 - sub-stream. */

//#define RTSP_ADDR					"192.168.1.11"
//#define RTSP_URL					"rtsp://192.168.1.11:554/user=admin_password=_channel=1_stream=0"

//#define RTSP_ADDR					"172.20.20.192"
//#define RTSP_URL					"rtsp://172.20.20.192:554/user=admin_password=_channel=1_stream=0"
//#define RTSP_URL					"rtsp://172.20.20.194:554/11"

//#define RTSP_ADDR					"192.168.1.10"
//#define RTSP_URL					"rtsp://192.168.1.10:554/11"

#define RTSP_URL					"rtsp://192.168.104.15:554/user=admin_password=_channel=1_stream=0"
#define RTSP_ADDR					"192.168.104.15"

//#define RTSP_URL					"rtsp://172.20.20.184:554/user=admin_password=_channel=1_stream=0"
//#define RTSP_ADDR					"172.20.20.184"


enum {
	CAMERA_FSM_NONE				= 0,
	CAMERA_FSM_CONNECTING		= 1,
	CAMERA_FSM_CONFIGURING		= 2,
	CAMERA_FSM_PLAY				= 3,
	CAMERA_FSM_PAUSE			= 4,
	CAMERA_FSM_DISCONNECTING	= 5
};

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CCameraApp::CCameraApp(int argc, char* argv[]) :
    CApplication("IP Camera Test", MAKE_VERSION(1,0,0), 1, argc, argv),
	CStateMachine(CAMERA_FSM_NONE),
	IMediaClient(),
	m_pMediaClient(0),
	m_pVideoChannel(0),
	m_pRecorder(0)
{
}

/*
 * Application class destructor
 */
CCameraApp::~CCameraApp()
{
	SAFE_DELETE(m_pMediaClient);
	SAFE_DELETE(m_pVideoChannel);
	SAFE_DELETE(m_pRecorder);
}

#if 0
void test264()
{
	CH264		h264;
	result_t	nresult;

	nresult = h264.load("camera.h264");
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "can' load file %d\n", nresult);
		return;
	}

	h264.dumph264(500, H264_DUMP_ALL);
}
#endif

boolean_t CCameraApp::processEvent(CEvent* pEvent)
{
	boolean_t	bProcessed;

	switch( pEvent->getType() )  {
		case EV_QUIT:
			onTerm();
			bProcessed = TRUE;
			break;

		case EV_USR1:
			if ( m_pRecorder ) {
				m_pRecorder->dump();
			}
			bProcessed = TRUE;
			break;

		default:
			bProcessed = CApplication::processEvent(pEvent);
			break;
	}

	return bProcessed;
}

void CCameraApp::disconnect()
{
	log_info(L_GEN, "[camera] Disconnecting...\n");
	setFsmState(CAMERA_FSM_DISCONNECTING);
	m_pMediaClient->disconnect();
}

void CCameraApp::onMediaClientConnect(CMediaClient* pClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CAMERA_FSM_CONNECTING ) {
		log_debug(L_GEN, "[camera] connected, result %d\n", nresult);

		if ( nresult == ESUCCESS ) {
			setFsmState(CAMERA_FSM_CONFIGURING);
			m_pMediaClient->configure();
		}
		else {
			disconnect();
		}
	}
	else {
		log_debug(L_GEN, "[camera] fsm missing (%d/%d)\n", fsm, nresult);
	}
}

void CCameraApp::onMediaClientConfigure(CMediaClient* pClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CAMERA_FSM_CONFIGURING ) {
		log_debug(L_GEN, "[camera] configure result %d\n", nresult);

		if ( nresult == ESUCCESS ) {
			setFsmState(CAMERA_FSM_PLAY);
			m_pMediaClient->play();
		}
		else {
			disconnect();
		}
	}
	else {
		log_debug(L_GEN, "[camera] fsm missing (%d/%d)\n", fsm, nresult);
	}
}

void CCameraApp::onMediaClientPlay(CMediaClient* pClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CAMERA_FSM_PLAY ) {
		log_debug(L_GEN, "[camera] play result %d\n", nresult);

		if ( nresult == ESUCCESS )  {
			log_debug(L_GEN, "[camera] ==== writing ====\n");
		}
		else {
			disconnect();
		}
	}
	else {
		log_debug(L_GEN, "[camera] fsm missing (%d/%d)\n", fsm, nresult);
	}
}

void CCameraApp::onMediaClientPause(CMediaClient* pClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CAMERA_FSM_PAUSE ) {
		log_debug(L_GEN, "[camera] pause result %d\n", nresult);
		disconnect();
	}
	else {
		log_debug(L_GEN, "[camera] fsm missing (%d/%d)\n", fsm, nresult);
	}
}

void CCameraApp::onMediaClientDisconnect(CMediaClient* pClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CAMERA_FSM_DISCONNECTING ) {
		log_debug(L_GEN, "[camera] disconnected, result %d\n", nresult);

		if ( logger_is_enabled(LT_TRACE|L_NET_MEDIA) ) {
			pClient->dump();
		}

		setFsmState(CAMERA_FSM_NONE);
		CApplication::stopApplication(0);
	}
	else {
		log_debug(L_GEN, "[camera] fsm missing (%d/%d)\n", fsm, nresult);
	}
}

void CCameraApp::onTerm()
{
	fsm_t	fsm = getFsmState();

	log_debug(L_GEN, "[camera] ==== terminating ====\n");

	m_pMediaClient->dump();
	if ( fsm == CAMERA_FSM_NONE ) {
		CApplication::stopApplication(0);
	} else {
		if ( getFsmState() != CAMERA_FSM_DISCONNECTING )  {
			setFsmState(CAMERA_FSM_DISCONNECTING);
			m_pMediaClient->disconnect();
		}
	}
}

void CCameraApp::initLogger()
{
	CApplication::initLogger();

	////logger_disable(LT_DEBUG|L_RTSP_FL);
	////logger_disable(LT_DEBUG|L_RTCP_FL);
	////logger_disable(LT_DEBUG|L_NET_MEDIA_FL);
	//logger_enable(LT_DEBUG|L_NETCONN);
	//logger_enable(LT_DEBUG|L_NETCONN_IO);
	//logger_enable(LT_DEBUG|L_NETCONN_FL);

	logger_enable(LT_TRACE|L_MP4FILE);
	logger_disable(LT_DEBUG|L_MP4FILE_DYN);

	//logger_enable(LT_DEBUG|L_NETCLI_FL);
	//logger_enable(LT_DEBUG|L_NETCLI_IO);

	logger_addTcpServerAppender(10001);
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CCameraApp::init()
{
	CNetAddr		selfAddr("eth1", INADDR_ANY);
	CNetAddr		rtspAddr(RTSP_ADDR, RTSP_PORT);
    result_t    	nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

	if ( !selfAddr.isValid() )  {
		log_error(L_GEN, "[camera] invalid self (bind) address specified\n");
		CApplication::terminate();
		return EINVAL;
	}

	shell_assert(m_pMediaClient == 0);
	shell_assert(m_pVideoChannel == 0);
	shell_assert(m_pRecorder == 0);

#if AV_RECORDER
	m_pRecorder = new CMp4Recorder(CAMERA_STORE_FILENAME, MP4_RECORDER_HAS_VIDEO);
	nresult = m_pRecorder->init();
	if ( nresult != ESUCCESS )  {
		SAFE_DELETE(m_pRecorder);
		CApplication::terminate();
		return EINVAL;
	}
	m_pVideoChannel = new CRtspChannelH264(1, CAMERA_RTP_PORT, CAMERA_FPS,
									2, m_pRecorder->getVideoSink(), "camera");
#else /* AV_RECORDER */
	m_pRecorder = new CMp4Writer(CAMERA_STORE_FILENAME);
	m_pVideoChannel = new CRtspChannelH264(1, CAMERA_RTP_PORT, CAMERA_FPS, 2,
									m_pRecorder, "camera");
#endif /* AV_RECORDER */

	m_pMediaClient = new CMediaClient(selfAddr, this, this, "camera");
	nresult = m_pMediaClient->init();
	if ( nresult != ESUCCESS )  {
		SAFE_DELETE(m_pMediaClient);
		SAFE_DELETE(m_pVideoChannel);
		SAFE_DELETE(m_pRecorder);
		CApplication::terminate();
		return nresult;
	}

	m_pMediaClient->insertChannel(m_pVideoChannel);

	setFsmState(CAMERA_FSM_CONNECTING);
	m_pMediaClient->connect(rtspAddr, RTSP_URL);

    return ESUCCESS;
}

/*
 * Application termination
 */
void CCameraApp::terminate()
{
	if ( m_pMediaClient )  {
		m_pMediaClient->terminate();
		m_pMediaClient->removeChannel(m_pVideoChannel);
	}

#if AV_RECORDER
	if ( m_pRecorder )  {
		m_pRecorder->dump();
		m_pRecorder->terminate();
	}
#endif /* AV_RECORDER */

	SAFE_DELETE(m_pMediaClient);
	SAFE_DELETE(m_pVideoChannel);
	SAFE_DELETE(m_pRecorder);
    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    return CCameraApp(argc, argv).run();
}
