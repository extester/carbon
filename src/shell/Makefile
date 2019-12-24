#
#   Shell library makefile
#
#   Copyright (c) 2013-2017 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 28.06.2013

#   Revision 1.1, 04.02.2015

#   Revision 1.2, 27.08.2015 14:29:35
#
#   Revision 2.0, 30.03.2016 18:12:23
#	Moved to use generic rules.mak make rules
#
#   Revision 2.1, 16.02.2017 18:07:11
#	Separated to unix and stm32 objects and headers 
#
#   Input variables:
#	MACHINE="machine-backend"	default is "unix"
#	CROSS_COMPILE="gcc_prefix"	
#	RELEASE=1
#

include ../../tool/config.mak

LIBRARY = $(BUILD_PATH)/libshell.a
OBJ_PATH = $(BUILD_PATH)/shell

################################################################################
# Common sources
################################################################################

OBJ_COMMON = \
	debug.o utils.o static_allocator.o

HEADER_COMMON = \
	assert.h atomic.h counter.h debug.h dec_ptr.h defines.h error.h \
	hr_time.h lock.h lockedlist.h logger.h object.h \
	queue.h ref_object.h shell.h thread.h tstdio.h tstdlib.h tstring.h \
	types.h unaligned.h utils.h static_allocator.h

ifeq ($(CARBON_DEBUG_TRACK_OBJECT), 1)
HEADER_COMMON += track_object.h
endif

################################################################################
# UNIX sources
################################################################################

OBJ_unix = \
	breaker.o file.o socket.o netaddr.o \
	net/netutils.o \
 	\
	logger/logger_base.o logger/appender_stdout.o \
	logger/appender_syslog.o logger/appender_file.o \
	logger/appender_tcp_server.o logger/appender_pickup.o \
	\
 	unix/assert.o unix/debug.o unix/thread.o unix/utils.o	

HEADER_unix = \
	breaker.h file.h memory.h socket.h netaddr.h \
	net/netutils.h \
	\
	unix/hr_time.h unix/lock.h unix/logger.h \
	unix/thread.h \
	\
	logger/logger_base.h logger/appender_file.h

################################################################################
# EMBED sources
################################################################################

OBJ_embed = \
 	embed/assert.o embed/logger.o embed/console_output.o embed/functor_allocator.o

HEADER_embed = \
	embed/logger.h embed/console_output.h embed/functor_allocator.h

-include embed/$(CARBON_BACKEND)/Makefile

################################################################################

OBJ = $(OBJ_COMMON) $(OBJ_$(CARBON_MACHINE))
HEADER = $(HEADER_COMMON) $(HEADER_$(CARBON_MACHINE))

INCLUDES +=  -I../ -I../../thparty/__root__/include
DEPS = $(HEADER)


all: $(LIBRARY)

include ../../tool/rules.mak
