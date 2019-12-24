#
#   NetMedia H264 Video MultiCamera test makefile
#
#   Copyright (c) 2016 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 02.11.2016 17:59:10
#	Initial revision.
#
#   make [all|clean|rebuild|r]
#
#

PROGRAM = multicam

OBJ = multicam.o storage.o
HEADER = multicam.h storage.h

DEPS = $(HEADER)
LIBS += -lmp4v2

all: carbon_dep $(PROGRAM)

include ../../tool/pkgrules.mak
