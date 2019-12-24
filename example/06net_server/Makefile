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
#   Module dependencies (add to <carbon_path>/depend.mak MODULE_DEP):
#	vep
#

OBJ_net_server = net_server_app.o
OBJ_net_client = net_client_app.o
INCLUDE = net_server_app.h net_client_app.h shared.h

OBJ = $(OBJ_net_server) $(OBJ_net_client)

all: carbon_dep net_server net_client

clean: clean_program

net_server: Makefile $(OBJ_net_server)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_net_server) $(_LIBS)

net_client: Makefile $(OBJ_net_client)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_net_client) $(_LIBS)

clean_program:
	rm -f net_server net_client

include ../../tool/pkgrules.mak
