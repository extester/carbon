#
#   Carbon framework example makefile
#
#   Copyright (c) 2015 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 10.06.2015 10:41:44
#	Initial revision.
#
#   make [CROSS_COMPILE=<gcc-prefix>] [RELEASE=1]
#

PROGRAM = minimal
OBJ = minimal_app.o
INCLUDE = minimal_app.h

all: $(PROGRAM)

$(PROGRAM): carbon_dep Makefile

include ../../tool/pkgrules.mak
