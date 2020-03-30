#
#   Carbon Center Client (gtk version) makefile
#
#   Copyright (c) 2016 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#	Revision 1.0, 10.06.2015 11:18:02
#	Initial revision.
#
#  make [CROSS_COMPILE=<gcc-prefix->] [MTUNE=<arch>] [RELEASE=1]
#

PROGRAM = acc

OBJ = acc.o interface.o memory_stat.o netconn_stat.o log_control.o ../center/packet_center.o
HEADER = acc.h interface.h memory_stat.h netconn_stat.h log_control.h

DEPS = $(HEADER)
CPPFLAGS := -std=c++11 -Wno-error=deprecated-declarations
INCLUDES := `pkg-config --cflags gtk+-3.0`
LIBS := `pkg-config --libs gtk+-3.0`

all: carbon_dep $(PROGRAM)

include $(CARBON)/tool/pkgrules.mak
