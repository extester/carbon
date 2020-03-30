/*
 *	Multi I/O test
 *
 *	Revision history:
 *		Version 1.0, 03.05.2015 18:08:35
 */

#include <string.h>
#include <unistd.h>

#include "shell/shell.h"
#include "shell/net/netutils.h"
#include "shell/net/icmp.h"

#include "carbon/carbon.h"
#include "carbon/packet_io.h"
#include "packet_center.h"

#define LISTEN_IFACE		"eth0"
#define LISTEN_PORT			10001


static void printHeartBeat(CInfoPacket* pPacket, hr_time_t hrElapsed)
{
	union centerPacketArgs* 	args;
	heartbeat_t* 				pItem;
	int i, count = pPacket->getSize() / sizeof(heartbeat_t);

	args = (union centerPacketArgs*)(packet_header_t*)*pPacket;
	pItem = args->heartbeat_reply.list;

	log_dump("--- hb_result (%.3f ms) ---\n", ((double)HR_TIME_TO_MICROSECONDS(hrElapsed))/1000);

	for (i=0; i<count; i++) {
		char strTmp[32];

		if ( pItem->hrPing != ICMP_ITER_FAILED && pItem->hrPing != ICMP_ITER_NOTSET ) {
			if ( pItem->hrPing < HR_1SEC ) {
				snprintf(strTmp, sizeof(strTmp), "%.3f ms",
						 ((double)HR_TIME_TO_MICROSECONDS(pItem->hrPing))/1000 );
			}
			else {
				snprintf(strTmp, sizeof(strTmp), "%.3f sec",
						 ((double)HR_TIME_TO_MILLISECONDS(pItem->hrPing))/1000 );
			}
		}
		else {
			if ( pItem->hrPing == ICMP_ITER_FAILED )  {
				snprintf(strTmp, sizeof(strTmp), "FAILED");
			}
			else {
				strcpy(strTmp, "NOSET");
			}
		}

		log_dump("%-12s: %-14s\t %s\n",
				 pItem->strName, (const char*) CNetHost(pItem->ipAddr), strTmp);

		pItem++;
	}
}

int main(int argc, char* argv[])
{
	struct in_addr	inaddr;
	result_t		nresult;
	hr_time_t		hrStart, hrElapsed;

	CCarbon::init(argc, argv);

	nresult = getInterfaceIp(LISTEN_IFACE, &inaddr);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "failed to get iface %s IP, result: %d\n", LISTEN_IFACE, nresult);
		return 1;
	}

	CInfoContainer*	pContainer = new CInfoContainer;
	CNetAddr		addr(inaddr, htons(LISTEN_PORT));
	CPacketIo		exchanger(HR_4SEC);

	while ( TRUE )  {
		pContainer->clear();

		pContainer->create(INFO_CONTAINER_CENTER, CENTER_PACKET_HEARTBEAT);
		pContainer->finalise();
		hrStart = hr_time_now();
		nresult = exchanger.execute(pContainer, NULL, addr);
		hrElapsed = hr_time_get_elapsed(hrStart);
		if ( nresult == ESUCCESS )  {
			CInfoPacket*	pPacket = *pContainer;

			if ( pContainer->isValid() && pContainer->getType() == INFO_CONTAINER_CENTER &&
					pPacket->getType() == CENTER_PACKET_HEARTBEAT_REPLY )
			{
				printHeartBeat(pPacket, hrElapsed);
			}
			else {
				log_error(L_GEN, "received unexpected container\n");
				pContainer->dump();
			}
		}
		else {
			log_error(L_GEN, "I/O failed, result: %s (%d)\n", strerror(nresult), nresult);
			break;
		}

		sleep(3);
	}

	pContainer->release();
	CCarbon::exit();

	return 0;
}
