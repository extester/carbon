#
#   Carbon Framework thparty custom makefile
#
#   Copyright (c) 2015 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 10.06.2015 11:18:02
#	Initial revision.
#
#   make [CROSS_COMPILE=<gcc-prefix->] [MTUNE=<arch>] [RELEASE=1]
#
#

LIBRARY = libpugixml.a
OBJ = src/pugixml.o
HEADER = src/pugixml.hpp src/pugiconfig.hpp

DEPS = $(HEADER)
INCLUDES = -Isrc

all: $(LIBRARY)

install:
	mkdir -p ${PREFIX}/include/pugixml
	cp src/*.hpp ${PREFIX}/include/pugixml
	mkdir -p ${PREFIX}/lib
	cp $(LIBRARY) ${PREFIX}/lib/

distclean: clean

include ../../tool/pkgrules.mak
