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
 *  Revision 1.0, 15.03.2017 11:29:34
 *      Initial revision.
 */

#include "shell/embed/tneo/tn_backend.h"

result_t errorTn2Shell(enum TN_RCode rc)
{
	result_t	nresult;

	switch ( rc )  {
		case TN_RC_OK:				nresult = ESUCCESS; break;
		case TN_RC_TIMEOUT:			nresult = ETIMEDOUT; break;
		case TN_RC_OVERFLOW:		nresult = EOVERFLOW; break;
		case TN_RC_WCONTEXT:		nresult = EFAULT; break;
		case TN_RC_WSTATE:			nresult = EBUSY; break;
		case TN_RC_WPARAM:			nresult = EINVAL; break;
		case TN_RC_ILLEGAL_USE:		nresult = EINVAL; break;
		case TN_RC_INVALID_OBJ:		nresult = EACCES; break;
		case TN_RC_DELETED:			nresult = ENOENT; break;
		case TN_RC_FORCED:			nresult = ERESTART; break;
		case TN_RC_INTERNAL:		nresult = ENXIO; break;

		default:
			nresult = ENOSYS;
			break;
	}

	return nresult;
}
