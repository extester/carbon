#
#   Carbon Center Test makefile
#
#   Copyright (c) 2016 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#	Revision 1.0, 05.06.2015 11:41:43
#	Initial revision.
#
#  make [CROSS_COMPILE=<gcc-prefix->] [MTUNE=<arch>] [RELEASE=1]
#
#

PROGRAM = hb

OBJ = hb.o 

DEPS = carbonlib_dep
INCLUDES = -I$(CARBONLIB_PATH) -I../
LIBS = -lcarbon -lshell -lstdc++ -lpthread -lrt
LDFLAGS = -L$(CARBONLIB_PATH)/carbon -L$(CARBONLIB_PATH)/shell

all: $(PROGRAM)

include ../../tool/rules.mak