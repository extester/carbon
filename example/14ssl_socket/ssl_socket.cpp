/*
 *  Carbon Framework
 *  TCL/SSL http client example
 *
 *  Copyright (c) 2021 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 23.12.2021 17:10:35
 *      Initial revision.
 */

#include "shell/logger.h"
#include "shell/ssl_socket.h"

#include "carbon/carbon.h"
#include "carbon/http_container.h"

#define SERVER_IP				"109.236.91.4"
#define SERVER_PORT				443
#define SERVER_HTTP_HOST		"www.citywalls.ru"

int main(int argc, char* argv[])
{
	dec_ptr<CHttpContainer>		pContainer = new CHttpContainer;
	CSslSocket					sock(true);
	CNetAddr					destAddr(SERVER_IP, SERVER_PORT);
	result_t					nresult;

	shell_unused(argc);
	shell_unused(argv);

	carbon_init();

	shell_assert(destAddr.isValid());

	log_debug(L_GEN, "starting ssl connection to %s:%u\n", SERVER_IP, SERVER_PORT);

	/*
	 * Connect to remove
	 */
	nresult = sock.connect(destAddr, HR_8SEC);
	if ( nresult == ESUCCESS )  {
		/*
		 * Send HTTP request
		 */
		char	strTmp[64];

		log_debug(L_GEN, "remote server has been connected\n");

		pContainer->setStartLine("GET /about.html HTTP/1.1");
		_tsnprintf(strTmp, sizeof(strTmp), "Host: %s", SERVER_HTTP_HOST);
		pContainer->appendHeader(strTmp);
		pContainer->appendHeader("Accept: text/html");
		pContainer->appendHeader("Accept-Encoding: gzip,deflate");

		log_debug(L_GEN, "sending http request\n");
		nresult = pContainer->send(sock, HR_8SEC);
		if ( nresult == ESUCCESS )  {
			/*
			 * Receive a http response
			 */
			log_debug(L_GEN, "remote server has been connected\n");

			pContainer->clear();
			log_debug(L_GEN, "awaiting for server response...\n");
			nresult = pContainer->receive(sock, HR_8SEC);
			if ( nresult == ESUCCESS )  {
				/*
				 * Process response
				 */
				log_debug(L_GEN, "response has been received, body of %d bytes\n", pContainer->getBodySize());
				pContainer->dump("Response ");
			}
			else {
				log_error(L_GEN, "failed to receive response, result %d (%s)\n", nresult, strerror(nresult));
			}
		}
		else {
			log_error(L_GEN, "sending http request failure, result %d (%s)\n", nresult, strerror(nresult));
		}

		sock.close();
	}
	else {
		log_error(L_GEN, "connect failed, result %d (%s)\n", nresult, strerror(nresult));
	}

	carbon_terminate();

	return nresult == ESUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
