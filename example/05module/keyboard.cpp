/*
 *	Carbon Framework Examples
 *	Keyboard module
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 26.01.2017 12:31:13
 *	    Initial revision.
 */

#include "module_app.h"
#include "receiver.h"
#include "keyboard.h"

#define MODULE_NAME			"Keyboard"

/*
 * Module class constructor
 *
 *      pParent     parent application pointer
 *      pReceiver	key processing module pointer
 */
CKeyboardModule::CKeyboardModule(CModuleApp* pParent, CReceiverModule* pReceiver) :
    CModule(MODULE_NAME),
    m_pParent(pParent),
	m_pReceiver(pReceiver),
    m_worker(MODULE_NAME, HR_0, HR_2SEC),
	m_pHwKeyboard(0)
{
}

/*
 * Module class destructor
 */
CKeyboardModule::~CKeyboardModule()
{
    shell_assert(!m_worker.isRunning());
	shell_assert(!m_pHwKeyboard);
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
void* CKeyboardModule::workerThread(CThread* pThread, void* pData)
{
	int 		keyCode;
	CEvent*		pEvent;
	boolean_t 	bDone = FALSE;

	UNUSED(pThread);
	UNUSED(pData);

	shell_assert(m_pHwKeyboard);
	shell_assert(m_pReceiver);
	shell_assert(m_pParent);

	while ( !bDone )  {
		keyCode = m_pHwKeyboard->getKey();
		if ( keyCode >= 0 )  {
			pEvent = new CEvent(EV_KEY, m_pReceiver, NULL, keyCode, "keycode");
			appSendEvent(pEvent);
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
result_t CKeyboardModule::init()
{
    result_t    nresult;

	shell_assert(m_pHwKeyboard == 0);

    nresult = CModule::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom Keyboard module initialisation...\n\r");

    /* User module inialisation */
	m_pHwKeyboard = CKeyboardDriverFactory::createDriver();
	if ( m_pHwKeyboard == 0 )  {
		log_error(L_GEN, "[keyboard] failed to create keyboard driver\n");
		CModule::terminate();
		return EIO;
	}

    nresult = m_worker.start(THREAD_CALLBACK(CKeyboardModule::workerThread, this));
    if ( nresult != ESUCCESS )  {
		CKeyboardDriverFactory::deleteDriver(m_pHwKeyboard);
		m_pHwKeyboard = 0;
        CModule::terminate();
    }

    return nresult;
}

/*
 * Module termination
 */
void CKeyboardModule::terminate()
{
    log_info(L_GEN, "Custom Keyboard module termination...\n\r");

    /* User module termination */
    m_worker.stop();
	CKeyboardDriverFactory::deleteDriver(m_pHwKeyboard);
	m_pHwKeyboard = 0;

    CModule::terminate();
}

/*
 * Debugging support
 */
void CKeyboardModule::dump(const char* strPref) const
{
    UNUSED(strPref);
}
