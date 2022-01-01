#
#   Carbon framework example makefile
#
#   Copyright (c) 2032 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 23.12.2021 17:08:35
#	Initial revision.
#
#   make [CROSS_COMPILE=<gcc-prefix>] [RELEASE=1]
#

PROGRAM = ssl_socket
OBJ = ssl_socket.o
INCLUDE =
LIBS=-lcrypto -lssl

all: carbon_dep $(PROGRAM) Makefile

include ../../tool/pkgrules.mak
