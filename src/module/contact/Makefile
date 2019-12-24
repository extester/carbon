
OBJ += contact/base64.o contact/md5lib.o contact/md5.o contact/proc_utils.o \
	contact/punycode.o contact/icmp.o contact/ifconfig.o contact/net_route.o \
	contact/sha1.o contact/ntp_client.o contact/ntp_client_service.o \
	\
	contact/gpio.o contact/gpio_linux.o \
	\
	contact/modem_base.o contact/modem_sim900r.o contact/modem_sim800.o

DEPS += contact/base64.h contact/md5lib.h contact/md5.h contact/proc_utils.h \
	contact/punycode.h contact/icmp.h contact/ifconfig.h contact/net_route.h \
	contact/sha1.h contact/ntp_client.h contact/ntp_client_service.h \
	\
	contact/gpio.h contact/gpio_linux.h \
	\
	contact/modem_base.h contact/modem_sim900r.h contact/modem_sim800.h


ifeq ($(CARBON_ZLIB),1)
OBJ += contact/zip.o
DEPS += contact/zip.h
endif

ifeq ($(CARBON_UDNS),1)
OBJ += contact/dns.o contact/dns_client.o contact/dns_client_service.o
DEPS += contact/dns.h contact/dns_client.h contact/dns_client_service.h
endif
