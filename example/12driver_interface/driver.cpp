/*
 *	Carbon Framework Examples
 *	Linux driver communication module
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 27.01.2017 18:20:54
 *	    Initial revision.
 */

#include "driver_app.h"
#include "driver.h"
#include "shared.h"

#define MODULE_NAME				"ExampleDriver"

#define EX_MESSAGE_INTERVAL		1 /* in seconds */

/*
 * Module class constructor
 *
 *      pParent     parent application pointer
 */
CDriverModule::CDriverModule(CDriverApp* pParent) :
    CModule(MODULE_NAME),
    m_pParent(pParent),
    m_worker(MODULE_NAME, HR_0, HR_2SEC)
{
}

/*
 * Module class destructor
 */
CDriverModule::~CDriverModule()
{
    shell_assert(!m_worker.isRunning());
	shell_assert(!m_driver.isOpen());
}

/*
 * Separate worker thread
 *
 * 		pThread		thread object pointer
 * 		pData		user-defined thread data
 *
 * Return:
 * 		thread exit code
 */
void* CDriverModule::workerThread(CThread* pThread, void* pData)
{
	boolean_t	bDone = FALSE;
	result_t	nresult;

	UNUSED(pThread);
	UNUSED(pData);

	shell_assert(m_pParent);
	shell_assert(m_driver.isOpen());

	while ( !bDone )  {
		ex_message_t	message;
		size_t			size;
		CEvent*			pEvent;

		size = sizeof(message);
		nresult = m_driver.read(&message, &size);

		if ( nresult == ESUCCESS )  {
			if ( size == sizeof(message) ) {
				if ( message.ident == EX_MESSAGE_ID ) {
					pEvent = new CEvent(EV_DRIVER_MESSAGE, m_pParent, NULL, message.data, "message");
					appSendEvent(pEvent);
				}
				else {
					log_error(L_GEN, "[driver] invalid notify id %Xh, expected %Xh\n",
							  message.ident, EX_MESSAGE_ID);
				}
			}
			else {
				log_error(L_GEN, "[driver] invalid notify size %u, expected %u\n",
						  		size, sizeof(message));
			}
		}
		else {
			bDone = TRUE;
		}
	}

	return NULL;
}

/*
 * Module initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CDriverModule::init()
{
	char		strDev[64];
    result_t    nresult;

	shell_assert(!m_driver.isOpen());

    nresult = CModule::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Driver interface module initialisation...\n\r");

    /* User module inialisation */
	_tsnprintf(strDev, sizeof(strDev), "/dev/%s", EX_DEVICE_NAME);
	nresult = m_driver.open(strDev, CFile::fileBlocking|CFile::fileReadWrite);
	if ( nresult == ESUCCESS ) {
		nresult = m_driver.ioctl(EX_IOC_INTERVAL, (void*)EX_MESSAGE_INTERVAL);
		if ( nresult != ESUCCESS ) {
			log_warning(L_GEN, "[driver] can't set message interval %d seconds, result: %d\n",
							EX_MESSAGE_INTERVAL, nresult);
		}
	}
	else {
		log_error(L_GEN, "[driver] failed to open device %s, result: %d\n",
				  strDev, nresult);
		CModule::terminate();
		return nresult;
	}

    nresult = m_worker.start(THREAD_CALLBACK(CDriverModule::workerThread, this));
    if ( nresult != ESUCCESS )  {
		m_driver.close();
        CModule::terminate();
    }

    return nresult;
}

/*
 * Module termination
 */
void CDriverModule::terminate()
{
    log_info(L_GEN, "Driver interface module termination...\n\r");

    /* User module termination */
	m_worker.stop();
	m_driver.close();

    CModule::terminate();
}

/*
 * Debugging support
 */
void CDriverModule::dump(const char* strPref) const
{
    UNUSED(strPref);
}
