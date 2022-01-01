/*
 *	Carbon Framework Examples
 *	DNS Client example
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 25.08.2017 11:46:32
 *	    Initial revision.
 */

#include "contact/dns_client_service.h"
#include "dns_client.h"

/*
 * Maximum DNS resolution time per host
 */
#define RESOLVE_TIMEOUT             HR_2SEC

/*
 * Hosts to resolve
 */
const char* CDnsResolveApp::m_arHost[] = {
    "google.com",
    "google.ru",
    "yandex.com",
    "yandex.ru",
	"no-exists.ru",
	"citywalls.ru",
	"max-4.ru"
};

#if GOOGLE_DNS
static CNetHost 	g_dns0 = CNetHost("8.8.8.8");
static CNetHost 	g_dns1 = CNetHost("8.8.4.4");
#else /* GOOGLE_DNS */
static CNetHost 	g_dns0;
static CNetHost 	g_dns1;
#endif /* GOOGLE_DNS */

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CDnsResolveApp::CDnsResolveApp(int argc, char* argv[]) :
    CApplication("DNS Resolve Application", MAKE_VERSION(1,0,0), 1, argc, argv),
    m_client(this),
    m_nIndex(0)
{
}

/*
 * Application class destructor
 */
CDnsResolveApp::~CDnsResolveApp()
{
}

/*
 * Event processor
 *
 *      pEvent      event object to process
 *
 * Return:
 *      TRUE        event processed
 *      FALSE       event is not processed
 */
boolean_t CDnsResolveApp::processEvent(CEvent* pEvent)
{
    boolean_t   bProcessed;

    switch ( pEvent->getType() )  {
        case EV_START:
            m_nIndex = 0;
            log_info(L_GEN, "[app] resolving %lu dns names\n", ARRAY_SIZE(m_arHost));

            if ( m_nIndex < ARRAY_SIZE(m_arHost) )  {
                m_client.resolve(m_arHost[m_nIndex], RESOLVE_TIMEOUT,
                                    g_dns0, g_dns1);
            }
            else {
                stopApplication(ESUCCESS);
            }

            bProcessed = TRUE;
            break;

        default:
            bProcessed = CApplication::processEvent(pEvent);
            break;
    }

    return bProcessed;
}

void CDnsResolveApp::dnsRequestResult(const CNetHost& ip, result_t nresult)
{
    if ( nresult == ESUCCESS )  {
        log_info(L_GEN, "Resolved host %s => %s\n",
                 m_arHost[m_nIndex], ip.cs());
    }
    else {
        log_error(L_GEN, "Failed to resolve host %s: %s(%d)\n",
                  m_arHost[m_nIndex], strerror(nresult), nresult);
    }

    m_nIndex++;
    if ( m_nIndex < ARRAY_SIZE(m_arHost) )  {
        m_client.resolve(m_arHost[m_nIndex], RESOLVE_TIMEOUT,
                         g_dns0, g_dns1);
    }
    else {
        stopApplication(ESUCCESS);
    }
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CDnsResolveApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* User application inialisation */

    nresult = initDnsClientService();
    if ( nresult != ESUCCESS )  {
        log_error(L_GEN, "[app] dns service initialisation failed, result %d\n", nresult);
        CApplication::terminate();
        return nresult;
    }

    m_nIndex = 0;
    return nresult;
}

/*
 * Application termination
 */
void CDnsResolveApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
    terminateDnsClientService();
    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    return CDnsResolveApp(argc, argv).run();
}
