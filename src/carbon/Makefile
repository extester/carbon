#
#   Carbon Framework Makefile
#
#   Copyright (c) 2015 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 06.04.2015
#	Initial revision.
#
#
#  make [CROSS_COMPILE=<gcc-prefix->] [ARCH=<arch_options>] [RELEASE=1]
#

include ../../tool/config.mak

LIBRARY = $(BUILD_PATH)/libcarbon.a
OBJ_PATH = $(BUILD_PATH)/carbon

OBJ_COMMON = \
	carbon.o module.o event.o \
	\
	event/event.o event/timer.o event/eventloop.o

OBJ_unix = \
	memory.o text_container.o tcp_server.o thread_pool.o \
	object_tracer.o cstring.o utils.o packet_io.o multi_io.o \
	raw_container.o http_container.o shell_execute.o signal_server.o \
	\
	net_connector/tcp_connector.o net_connector/tcp_listen.o \
	net_connector/tcp_worker.o net_connector/udp_connector.o \
	\
	net_server/net_server.o net_server/net_server_connection.o \
	net_server/net_client.o \
	\
	event/rpc.o event/remote_event.o event/remote_event_service.o \
	\
	unix/allocator.o unix/carbon.o unix/application.o

OBJ_embed = \
	embed/allocator.o embed/carbon.o embed/application.o

HEADER_COMMON = \
	carbon.h module.h logger.h timer.h thread.h lock.h fsm.h \
	event.h application.h \
	\
	event/event.h event/timer.h event/eventloop.h

HEADER_unix = \
	memory.h text_container.h tcp_server.h thread_pool.h \
	object_tracer.h cstring.h utils.h packet_io.h raw_container.h \
	http_container.h shell_execute.h signal_server.h multi_io.h  \
	net_container.h sync.h \
	\
	net_connector/tcp_connector.h net_connector/tcp_listen.h \
	net_connector/tcp_worker.h net_connector/udp_connector.h \
	\
	net_server/net_server.h net_server/net_server_connection.h \
	net_server/net_client.h \
	\
	event/rpc.h event/remote_event.h event/remote_event_service.h \
	\
	unix/application.h

HEADER_embed = \
	embed/application.h


ifeq ($(CARBON_DEBUG_EVENT), 1)
OBJ_COMMON += event/event_debug.o
HEADER_COMMON += event/event_debug.h
endif

ifeq ($(CARBON_JANSSON), 1)
OBJ_unix += config_json.o json.o
HEADER_unix += config_json.h json.h
endif

ifeq ($(CARBON_XML), 1)
OBJ_unix += config_xml.o
HEADER_unix += config_xml.h
endif

OBJ = $(OBJ_COMMON) $(OBJ_$(CARBON_MACHINE))
HEADER = $(HEADER_COMMON) $(HEADER_$(CARBON_MACHINE))


#ifeq ($(ENABLE_MYSQL),1)
#OBJ += db_mysql.o
#HEADER += $(H)/db_mysql.h
#INCLUDES += `mysql_config --include`
#DEFINES += -DMYSQL_ENABLE=1
#endif

INCLUDES += -I../ -I../../thparty/__root__/include -I../module
DEPS += $(HEADER)
CPPFLAGS += -Wno-error=deprecated-declarations

all: $(LIBRARY)

include ../../tool/rules.mak
