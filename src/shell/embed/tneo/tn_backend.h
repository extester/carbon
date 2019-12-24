/*
 *  Shell library
 *  TNeo backend specific definitions and functions
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 15.03.2017 11:27:07
 *      Initial revision.
 */

#ifndef __SHELL_TN_BACKEND_H_INCLUDED__
#define __SHELL_TN_BACKEND_H_INCLUDED__

#include "tn.h"

#include "shell/error.h"

extern result_t errorTn2Shell(enum TN_RCode rc);

#endif /* __SHELL_TN_BACKEND_H_INCLUDED__ */
