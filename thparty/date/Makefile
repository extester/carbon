#
#   Carbon Framework thparty custom makefile
#
#   Copyright (c) 2015 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 05.10.2021 16:34:42
#	Initial revision.
#
#   make [CROSS_COMPILE=<gcc-prefix->] [MTUNE=<arch>] [RELEASE=1]
#
#

LIBRARY = libdate.a
OBJ = src/tz.o
HEADER = include/date/date.h include/date/tz.h include/date/tz_private.h

DEPS = $(HEADER)
INCLUDES = -Iinclude -DHAS_REMOTE_API=0 -DAUTO_DOWNLOAD=0 -DUSE_OS_TZDB=1 -DUSE_SHELL_API=0

all: $(LIBRARY)

install:
	mkdir -p ${PREFIX}/include/date
	cp include/date/*.h ${PREFIX}/include/date
	mkdir -p ${PREFIX}/lib
	cp $(LIBRARY) ${PREFIX}/lib/

distclean: clean

include ../../tool/pkgrules.mak
