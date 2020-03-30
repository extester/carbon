#
#   Carbon Center makefile
#
#   Copyright (c) 2016 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#	Revision 1.0, 31.05.2015 16:37:20
#	Initial revision.
#
#  make [CROSS_COMPILE=<gcc-prefix->] [MTUNE=<arch>] [RELEASE=1]
#
#

PROGRAM = center

OBJ = center.o host_list.o heart_beat.o event.o packet_center.o host_db.o
HEADER = center.h host_list.h heart_beat.h event.h packet_center.h host_db.h

DEPS = $(HEADER) 
CPPFLAGS := -std=c++11

all: carbon_dep $(PROGRAM)

include $(CARBON)/tool/pkgrules.mak
