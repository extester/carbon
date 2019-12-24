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
 *	Revision 1.0, 01.11.2016 16:59:34
 *		Initial revision.
 */

#include "multicam.h"

#define STORAGE_FILENAME_FORMAT				"camera%d.h264"

/*{
0,
"rtsp://192.168.104.15:554/user=admin_password=_channel=1_stream=0",
"192.168.104.15",
4444,
0,
0
},*/

camera_t CMultiCamApp::m_arCamera[] = {
	/*{
		0,
		"rtsp://172.20.20.181:554/user=admin_password=_channel=1_stream=0",
		"172.20.20.181",
		4444,
		0,
		0
	},*/
	{
		1,
		"rtsp://192.168.104.15:554/user=admin_password=_channel=1_stream=0",
		"192.168.104.15",
		4446,
		0,
		0
	}/*,
	{
		2,
		"rtsp://192.168.104.16:554/user=admin_password=_channel=1_stream=0",
		"192.168.104.16",
		4448,
		0,
		0
	},
	{
		3,
		"rtsp://172.20.20.184:554/user=admin_password=_channel=1_stream=0",
		"172.20.20.184",
		4450,
		0,
		0
	}*//*,
	{
		4,
		"rtsp://172.20.20.185:554/user=admin_password=_channel=1_stream=0",
		"172.20.20.185",
		4452,
		0,
		0
	},
	{
		5,
		"rtsp://172.20.20.186:554/user=admin_password=_channel=1_stream=0",
		"172.20.20.186",
		4454,
		0,
		0
	}*/
};

#define NUM_CAMERAS		ARRAY_SIZE(CMultiCamApp::m_arCamera)

#define SET_BIT(value, bit)			(value)|= (1<<bit)
#define RESET_BIT(value, bit)		(value)&= (~(1<<bit))

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CMultiCamApp::CMultiCamApp(int argc, char* argv[]) :
    CApplication("IP MultiCamera Test", MAKE_VERSION(1,0), 1, argc, argv),
	IRtpVideoH264Result(),
	m_bmpCamera(0),
	m_bmpStop(0),
	m_bStopping(0)
{
}

/*
 * Application class destructor
 */
CMultiCamApp::~CMultiCamApp()
{
}

boolean_t CMultiCamApp::processEvent(CEvent* pEvent)
{
	boolean_t	bProcessed;

	switch( pEvent->getType() )  {
		case EV_QUIT:
			onTerm();
			bProcessed = TRUE;
			break;

		case EV_USR1:
			onUsr1();
			bProcessed = TRUE;
			break;

		default:
			bProcessed = CApplication::processEvent(pEvent);
			break;
	}

	return bProcessed;
}

/*
 * Find camera index based on media client address
 *
 * Return: camera index, 0..NUM_CAMERAS-1
 */
int CMultiCamApp::getCameraIndex(CRtpVideoH264* pCamera) const
{
	int 	index = -1;
	size_t	i, count = NUM_CAMERAS;

	for(i=0; i<count; i++)  {
		if ( m_arCamera[i].pCamera  == pCamera )  {
			index = (int)i;
			break;
		}
	}

	shell_assert(index >= 0);
	return index;
}

void CMultiCamApp::startCameras()
{
	size_t		i, count = NUM_CAMERAS;

	shell_assert(!m_bStopping);

	for(i=0; i<count; i++)  {
		log_info(L_GEN, "[multicam(%d)] starting camera id:%d\n", i, m_arCamera[i].id);
		m_arCamera[i].pCamera->start();
	}
}

void CMultiCamApp::stopCameras()
{
	size_t		i, count = NUM_CAMERAS;

	m_bmpStop = m_bmpCamera;
	m_bStopping = TRUE;

	for(i=0; i<count; i++)  {
		log_info(L_GEN, "[multicam(%d)] stopping camera id:%d\n", i, m_arCamera[i].id);
		m_arCamera[i].pCamera->stop();
	}
}

void CMultiCamApp::onRtpVideoH264Stop(CRtpVideoH264* pCamera)
{
	if ( m_bStopping )  {
		int	index = getCameraIndex(pCamera);

		log_info(L_GEN, "[multicam(%d)] camera id:%d stopped\n", index, m_arCamera[index].id);

		RESET_BIT(m_bmpStop, index);
		if ( m_bmpStop == 0 ) {
			stopApplication(0);
		}
	}
}

void CMultiCamApp::initLogger()
{
	CApplication::initLogger();

	logger_disable(LT_DEBUG|L_RTSP_FL);
	logger_disable(LT_DEBUG|L_RTCP_FL);
	//logger_disable(LT_DEBUG|L_NET_MEDIA_FL);
	//logger_disable(LT_DEBUG|L_RTP_VIDEO_H264_FL);
}

/*
 * Called when user pressed ^C
 */
void CMultiCamApp::onTerm()
{
	log_debug(L_GEN, "[multicam] ==== terminating ====\n");
	m_arCamera[0].pCamera->dump();

	if ( !m_bStopping )  {
		stopCameras();
	}
}

void CMultiCamApp::onUsr1()
{
	int 	nFrameLost, nNodeDropped;
	int 	i, count = NUM_CAMERAS;

	if ( count > 1 )  {
		log_dump("-----------------------------------------------------------------\n");
	}

	for(i=0; i<count; i++)  {
		if ( m_arCamera[i].pCamera )  {
			m_arCamera[i].pCamera->getFrameStat(&nFrameLost, &nNodeDropped);

			log_dump("[video%d] rtp frames lost: %u, video nodes dropped: %d\n",
				 i, nFrameLost, nNodeDropped);
		}
	}
}


/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CMultiCamApp::init()
{
	CNetAddr	selfAddr("eth1", INADDR_ANY);
	size_t		i, count = NUM_CAMERAS;
	char		strFile[128], strName[128];
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

	if ( !selfAddr.isValid() )  {
		log_error(L_GEN, "[multicam] invalid self (bind) address specified\n");
		CApplication::terminate();
		return EINVAL;
	}

	m_bmpCamera = 0;

	for(i=0; i<count; i++)  {
		camera_t*	pCam = &m_arCamera[i];

		_tsnprintf(strName, sizeof(strName), "cam%d", (int)i);

		_tsnprintf(strFile, sizeof(strFile), STORAGE_FILENAME_FORMAT, (int)i);
		pCam->pWriter = new CRtpFileWriter(strFile);

		pCam->pCamera = new CRtpVideoH264(pCam->id, selfAddr, CNetHost(pCam->strIp), pCam->strUrl,
								  pCam->nRtpPort, VIDEO_FPS, 4, pCam->pWriter, this, strName);
		SET_BIT(m_bmpCamera, i);
	}

	startCameras();

    return ESUCCESS;
}

/*
 * Application termination
 */
void CMultiCamApp::terminate()
{
	size_t		i, count = NUM_CAMERAS;

	for(i=0; i<count; i++)  {
		camera_t*	pCam = &m_arCamera[i];

		SAFE_DELETE(pCam->pCamera);
		SAFE_DELETE(pCam->pWriter);
	}

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
	return CMultiCamApp(argc, argv).run();
}
