#
#   NetMedia H264 Video Camera test makefile
#
#   Copyright (c) 2010-2019 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 02.11.2016 17:58:26
#	Initial revision.
#
#  make [all|clean|rebuild|r]
#
#

PROGRAM = camera

OBJ = camera.o mp4_writer.o
HEADER = camera.h mp4_writer.h
LIBS += -lmp4v2

DEPS = $(HEADER)

all: carbon_dep $(PROGRAM)

include ../../tool/pkgrules.mak
