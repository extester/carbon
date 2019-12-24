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

OBJ_udp_server = udp_server_app.o
OBJ_udp_client = udp_client_app.o
INCLUDE = udp_server_app.h udp_client_app.h shared.h

OBJ = $(OBJ_udp_server) $(OBJ_udp_client)

all: carbon_dep udp_server udp_client

clean: clean_program

udp_server: Makefile $(OBJ_udp_server)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_udp_server) $(_LIBS)

udp_client: Makefile $(OBJ_udp_client)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_udp_client) $(_LIBS)

clean_program:
	rm -f $(OBJ) udp_server udp_client

include ../../tool/pkgrules.mak
