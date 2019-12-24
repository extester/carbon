#
#   Carbon framework example makefile
#
#   Copyright (c) 2015 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 10.06.2015 11:46:22
#	Initial revision.
#
#   make [CROSS_COMPILE=<gcc-prefix>] [RELEASE=1]
#

PROGRAM = timer
OBJ = timer_app.o receiver.o
INCLUDE = timer_app.h receiver.h

all: carbon_dep $(PROGRAM) Makefile

include ../../tool/pkgrules.mak
