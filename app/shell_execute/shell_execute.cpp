/*
 *  Carbon Framework
 *  Shell programs executor
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.08.2016 13:06:00
 *      Initial revision.
 */

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>

#include "shell/file.h"

#include "carbon/carbon.h"
#include "carbon/memory.h"
#include "carbon/tcp_server.h"
#include "carbon/event/remote_event_service.h"
#include "carbon/utils.h"

#define SHELL_EXECUTE_CMD_MAX			1024
#define SHELL_EXECUTE_OUTPUT_MAX		(16*1024)

static int xpopen(const char* program, const char *type, pid_t* pChildPid)
{
	int pdes[2], fd, pid;

    if ( (*type != 'r' && *type != 'w') || type[1] ) {
		errno = EINVAL;
		return -1;
	}

    if ( pipe(pdes) < 0 ) {
		return -1;
	}
    switch (pid = fork()) {
    case -1:            /* error */
		(void) close(pdes[0]);
        (void) close(pdes[1]);
        return -1;
        /* NOTREACHED */

    case 0:             /* child */
		if (*type == 'r') {
			(void) close(pdes[0]);

            if (pdes[1] != fileno(stdout)) {
                (void) dup2(pdes[1], fileno(stdout));
                (void) close(pdes[1]);
            }

        }
		else {
			(void) close(pdes[1]);

            if (pdes[0] != fileno(stdin)) {
                (void) dup2(pdes[0], fileno(stdin));
                (void) close(pdes[0]);
            }
        }

		str_vector_t	v;
		int				n, i, icmd, ienv;
		const char**	args;
		const char**	env;
		const char*		s;

		n = (int)strSplit(program, ' ', &v, " \t");
		if ( n < 1 )  {
			if ( *type == 'r' ) close(fileno(stdout)); else close(fileno(stdin));
			errno = EINVAL;
			return -1;
		}

		args = (const char**)memAlloca((n+1)*sizeof(char*));
		env = (const char**)memAlloca((n+1)*sizeof(char*));
		icmd = 0; ienv = 0;

		for(i=0; i<n; i++)  {
			if ( icmd == 0 && (s=_tstrchr(v[i], '=')) != 0 ) {
				/* This is environment variable, NAME=["']VALUE["'] */
				s++;
				if ( *s == '"' )  {
					CString		st(v[i], A(s)-A(v[i].cs()));
					CString		sv(s);
					sv.trim("\"");
					st += sv;
					v[i] = st;
				}
				else if ( *s == '\'' ) {
					CString		st(v[i], A(s)-A(v[i].cs()));
					CString		sv(s);
					sv.trim("'");
					st += sv;
					v[i] = st;
				}
				env[ienv] = v[i];
				//log_debug(L_GEN, "ENV %d: '%s'\n", ienv, env[ienv]);
				ienv++;
			}
			else {
				/* This is either program name or command line option */
				args[icmd] = v[i];
				//log_debug(L_GEN, "CMD %d: '%s'\n", icmd, args[icmd]);
				icmd++;
			}
		}

		args[icmd] = NULL;
		env[ienv] = NULL;

		execvpe(args[0], (char* const*)&args[0], (char* const*)env);
        _exit(127);
        /* NOTREACHED */
    }

    /* parent; assume fdopen can't fail...  */
	*pChildPid = pid;

    if (*type == 'r') {
        fd = pdes[0];
        (void) close(pdes[1]);
    } else {
        fd = pdes[1];
    	(void) close(pdes[0]);
	}

    return fd;
}

static int xpclose(int fd, pid_t pid)
{
    int pstat;

     /*
 	 * pclose returns -1 if stream is not associated with a
 	 * `popened' command, if already `pclosed', or waitpid
 	 * returns an error.
 	 */
     (void) close(fd);
     do {
         pid = waitpid(pid, &pstat, 0);
     } while (pid == -1 && errno == EINTR);
     return (pid == -1 ? -1 : pstat);
 }

class CShellExecuteServer : public CTcpServer
{
	private:
		CString			m_strSocket;

	public:
		CShellExecuteServer() : CTcpServer("shell_execute") {
			m_strSocket = REMOTE_EVENT_ROOT_PATH;
			m_strSocket.appendPath(CARBON_SHELL_EXECUTE_RID);
			setListenAddr(m_strSocket);
		}

		virtual ~CShellExecuteServer() {
			//CFile::removeFile(m_strSocket);
		}

	public:
		virtual result_t run();
		virtual result_t processClient(CSocketRef* pSocket);

	private:
		result_t prepareSocket();
		result_t sendReply(CRemoteEvent* pEvent, const char* strCmd);

		result_t executeEvent(CRemoteEvent* pEvent);
		boolean_t executeCmd(const char* strCmd, result_t* pnresult, int* pRetVal,
							 char** pstrOutBuffer);
		result_t doExecute(const char* strCmd, int* pRetVal, char** pstrOutBuffer);
};

result_t CShellExecuteServer::prepareSocket()
{
	int 		n = 0;
	result_t	nresult = ESUCCESS;

	CFile::makeDir(REMOTE_EVENT_ROOT_PATH);
	CFile::removeFile(m_strSocket);

	while ( CFile::fileExists(m_strSocket) && n < 10 )  {
		sleep_ms(100);
		n++;
	}

	if ( CFile::fileExists(m_strSocket) )  {
		log_error(L_SHELL_EXECUTE, "[shell_execute] failed to prepare socket %s\n",
				  m_strSocket.cs());
		nresult = EEXIST;
	}

	return nresult;
}

result_t CShellExecuteServer::sendReply(CRemoteEvent* pEvent, const char* strCmd)
{
	CString				strReplySocket;
	dec_ptr<CSocketRef>	pSocket = new CSocketRef;
	result_t			nresult;

	strReplySocket = REMOTE_EVENT_ROOT_PATH;
	strReplySocket.appendPath(pEvent->getReplyRid());

	nresult = pSocket->connect(strReplySocket, HR_8SEC);
	if ( nresult == ESUCCESS )  {
		nresult = pEvent->send(*pSocket, HR_8SEC);
		if ( nresult != ESUCCESS )  {
			log_error(L_SHELL_EXECUTE, "[shell_execute] failed to send reply to %s, cmd %s, result: %d\n",
					  strReplySocket.cs(), strCmd, nresult);
		}
	}
	else {
		log_error(L_SHELL_EXECUTE, "[shell_execute] failed to connect to %s, cmd %s, result: %d\n",
				  strReplySocket.cs(), strCmd, nresult);
	}

	pSocket->close();
	return nresult;
}


result_t CShellExecuteServer::run()
{
	result_t	nresult;

	nresult = prepareSocket();
	if ( nresult == ESUCCESS )  {
		log_debug(L_SHELL_EXECUTE_FL, "[shell_execute] listening on %s...\n", m_strSocket.cs());
		nresult = CTcpServer::run();
	}

	//CFile::removeFile(m_strSocket);
	return nresult;
}

result_t CShellExecuteServer::processClient(CSocketRef* pSocket)
{
	dec_ptr<CRemoteEvent>	pEvent = new CRemoteEvent;
	result_t 				nresult;

	nresult = pEvent->receive(*pSocket, HR_8SEC);
	if ( nresult == ESUCCESS )  {
		nresult = executeEvent(pEvent);
	}

	return nresult;
}

result_t CShellExecuteServer::executeEvent(CRemoteEvent* pEvent)
{
	CString			strCmd;
	const void*		pData;
	size_t			size;
	char*			strOutBuffer = NULL;
	boolean_t		bResult = TRUE;
	int 			retVal = 250;
	result_t		nresult, nrChild = ENOENT;

	log_debug(L_SHELL_EXECUTE_FL, "[shell_execute] received a request from %s\n",
			  pEvent->getReplyRid().cs());

	pData = pEvent->getData();
	size = pEvent->getDataSize();
	if ( pData != 0 && size > 0 )  {
		strCmd.append(pData, MIN(size, SHELL_EXECUTE_CMD_MAX));

		bResult = executeCmd(strCmd, &nrChild, &retVal, &strOutBuffer);
	}

	if ( bResult && pEvent->getSessId() != NO_SEQNUM )  {
		dec_ptr<CRemoteEvent>	pReplyEvent;
		size_t					outSize = strOutBuffer ? strlen(strOutBuffer) : 0;

		pReplyEvent = new CRemoteEvent(EV_R_SHELL_EXECUTE_REPLY,
									   pEvent->getReplyReceiver(), 0,
									   pEvent->getSessId(),
									   strOutBuffer, outSize,
									   (PPARAM)A(retVal), (NPARAM)nrChild);
		pReplyEvent->setReplyRid(pEvent->getReplyRid());
		nresult = sendReply(pReplyEvent, strCmd);
	}
	else {
		nresult = ESUCCESS;
	}

	SAFE_FREE(strOutBuffer);
	return nresult;
}

boolean_t CShellExecuteServer::executeCmd(const char* strCmd, result_t* pnresult,
										  int* pRetVal, char** pstrOutBuffer)
{
	pid_t	pidExecute;

	log_debug(L_SHELL_EXECUTE, "[shell_execute] processing cmd '%s'\n", strCmd);

	pidExecute = fork();
	if ( pidExecute == -1 )  {
		/*
		 * Fork failed
		 */
		*pRetVal = ENOMEM;
		*pnresult = ENOMEM;
		return TRUE;	/* send reply event */
	}

	if ( pidExecute != 0 )  {
		/*
		 * Parent process
		 */
		waitpid(-1, NULL, WNOHANG);	/* clean up previous children zombee */
		return FALSE;	/* reply will send by child process */
	}

	/* New child process */
	*pnresult = doExecute(strCmd, pRetVal, pstrOutBuffer);

	log_debug(L_SHELL_EXECUTE_FL, "[shell_execute] execute result %d, retCode %d, outbuf size %d bytes\n",
			  	*pnresult, *pRetVal, *pstrOutBuffer ? _tstrlen(*pstrOutBuffer) : 0);
	stop();
	return TRUE;
}

/*
 * Run shell command, executed in new executor process
 *
 * 		strCmd			full command line
 * 		pRetVal			program result code [out]
 * 		pstrOutBuffer	program stdout content
 *
 * Return: ESUCCESS, ...
 */
result_t CShellExecuteServer::doExecute(const char* strCmd, int* pRetVal,
										char** pstrOutBuffer)
{
	char*		strOutBuffer;
	size_t 		length;
	CFile		file;
	int			fd, retVal;
	pid_t		pid;
	result_t	nresult;

	strOutBuffer = (char*)memAlloc(SHELL_EXECUTE_OUTPUT_MAX);
	if ( strOutBuffer == NULL )  {
		log_error(L_SHELL_EXECUTE, "[shell_execute] failed to allocate output buffer %d bytes, cmd: %s\n",
				  SHELL_EXECUTE_OUTPUT_MAX, strCmd);
		*pRetVal = ENOMEM;
		return ENOMEM;
	}

	fd = xpopen(strCmd, "r", &pid);
	if ( fd < 0 )  {
		*pRetVal = errno == 0 ? ENOMEM : errno;
		log_error(L_SHELL_EXECUTE, "[shell_execute] popen() failed, cmd: %s, errno: %d\n",
				  strCmd, errno);
		memFree(strOutBuffer);
		return *pRetVal;
	}

	file.attachHandle(fd);
	file.setBlocking(FALSE);

	*strOutBuffer = '\0';
	length = 0;

	while ( TRUE ) {
		size_t 		len;
		result_t	nr;
		siginfo_t 	info;
		int			retVal;

		len = (SHELL_EXECUTE_OUTPUT_MAX-1)-length;
		nr = file.read(&strOutBuffer[length], &len, 0, HR_3SEC);
		//log_debug(L_GEN, "pid = %u, NR=%d, len=%u, length=%u\n", pid, nr, len, length);
		if ( nr != ESUCCESS && nr != ETIMEDOUT )  {
			break;
		}
		if ( nr == ESUCCESS && len == 0 )  {
			break;
		}

		length += len;
		strOutBuffer[length] = '\0';
		//log_debug(L_GEN, "S='%s'\n", strOutBuffer);

		_tbzero_object(info);
		retVal = waitid(P_PID, pid, &info,  WEXITED|WNOHANG|WNOWAIT);
		if ( retVal == 0 && info.si_pid == pid )  {
			log_debug(L_SHELL_EXECUTE_FL, "[shell_execute] client exitted\n");
			break;
		}
	}

	file.setBlocking(FALSE);
	file.detachHandle();

	retVal = xpclose(fd, pid);
	if ( retVal != -1 ) {
		if ( WIFEXITED(retVal) )  {
			/* Child terminated normally */
			*pRetVal = WEXITSTATUS(retVal);

			switch ( *pRetVal )  {
				case 0:		/* Success */
					nresult = ESUCCESS; break;

				case 1:		/* General error */
					nresult = EFAULT; break;

				case 2:		/* Misuse of shell buildins */
					nresult = ENOEXEC; break;

				case 126:	/* Command invoked cannot execute */
					nresult = EPERM; break;

				case 127:	/* Command not found */
					nresult = ENOENT; break;

				case 130:	/* Control-C */
					nresult = EPIPE; break;

				case 255:	/* Exit status out of range */
					nresult = ERANGE; break;

				default: 	/* FAtal error signal */
					nresult = EFAULT; break;
			}
		}
		else {
			/* Child failure */
			*pRetVal = EFAULT;
			nresult = EFAULT;
		}
	}
	else {
		nresult = errno == 0 ? EINVAL : errno;
		*pRetVal = nresult;
		log_error(L_SHELL_EXECUTE, "[shell_execute] pclose() failed, errno %d, retVal %d, cmd: %s\n",
				  	nresult, *pRetVal, strCmd);
	}

	*pstrOutBuffer = strOutBuffer;
	return nresult;
}

/*******************************************************************************/

CShellExecuteServer*	g_pShellExecuteServer = 0;

static void signalHandler(int sig)
{
	g_pShellExecuteServer->stop();
}

int main(int argc, char* argv[])
{
	result_t	nresult;

	carbon_init();

	logger_set_format(LT_DEBUG, "%T [%l] %P(%N): (%p) %s");
	logger_set_format(LT_ERROR, "%T [%l] %P(%N): (%p) %s");
	logger_set_format(LT_INFO, "%T [%l] %P(%N): (%p) %s");

	//logger_disable(LT_DEBUG|L_SHELL_EXECUTE);
	logger_disable(LT_ALL|L_SHELL_EXECUTE_FL);

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGABRT, signalHandler);
	signal(SIGPIPE, SIG_IGN);

	g_pShellExecuteServer = new CShellExecuteServer;

	log_info(L_SHELL_EXECUTE_FL, "[shell_execute] starting server instance\n");
	nresult = g_pShellExecuteServer->run();
	log_info(L_SHELL_EXECUTE_FL, "[shell_execute] stopping server instance\n");

	SAFE_DELETE(g_pShellExecuteServer);

	carbon_terminate();

	return nresult == ESUCCESS ? 0 : 1;
}
