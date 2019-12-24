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

OBJ_net_sync_server = net_sync_server_app.o
OBJ_net_sync_client = net_sync_client_app.o
INCLUDE = net_sync_server_app.h net_sync_client_app.h shared.h

OBJ = $(OBJ_net_sync_server) $(OBJ_net_sync_client)

all: carbon_dep net_sync_server net_sync_client

clean: clean_program

net_sync_server: Makefile $(OBJ_net_sync_server)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_net_sync_server) $(_LIBS)

net_sync_client: Makefile $(OBJ_net_sync_client)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_net_sync_client) $(_LIBS)

clean_program:
	rm -f $(OBJ) net_sync_server net_sync_client

include ../../tool/pkgrules.mak
