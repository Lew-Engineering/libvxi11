/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "vxi11_rpc.h"

bool_t
xdr_Device_Link (XDR *xdrs, Device_Link *objp)
{
	register int32_t *buf;

	 if (!xdr_long (xdrs, objp))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_AddrFamily (XDR *xdrs, Device_AddrFamily *objp)
{
	register int32_t *buf;

	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_Flags (XDR *xdrs, Device_Flags *objp)
{
	register int32_t *buf;

	 if (!xdr_long (xdrs, objp))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_ErrorCode (XDR *xdrs, Device_ErrorCode *objp)
{
	register int32_t *buf;

	 if (!xdr_long (xdrs, objp))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_Error (XDR *xdrs, Device_Error *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_ErrorCode (xdrs, &objp->error))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Create_LinkParms (XDR *xdrs, Create_LinkParms *objp)
{
	register int32_t *buf;


	if (xdrs->x_op == XDR_ENCODE) {
		buf = XDR_INLINE (xdrs, 3 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_long (xdrs, &objp->clientId))
				 return FALSE;
			 if (!xdr_bool (xdrs, &objp->lockDevice))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->lock_timeout))
				 return FALSE;

		} else {
		IXDR_PUT_LONG(buf, objp->clientId);
		IXDR_PUT_BOOL(buf, objp->lockDevice);
		IXDR_PUT_U_LONG(buf, objp->lock_timeout);
		}
		 if (!xdr_string (xdrs, &objp->device, ~0))
			 return FALSE;
		return TRUE;
	} else if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE (xdrs, 3 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_long (xdrs, &objp->clientId))
				 return FALSE;
			 if (!xdr_bool (xdrs, &objp->lockDevice))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->lock_timeout))
				 return FALSE;

		} else {
		objp->clientId = IXDR_GET_LONG(buf);
		objp->lockDevice = IXDR_GET_BOOL(buf);
		objp->lock_timeout = IXDR_GET_U_LONG(buf);
		}
		 if (!xdr_string (xdrs, &objp->device, ~0))
			 return FALSE;
	 return TRUE;
	}

	 if (!xdr_long (xdrs, &objp->clientId))
		 return FALSE;
	 if (!xdr_bool (xdrs, &objp->lockDevice))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->lock_timeout))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->device, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Create_LinkResp (XDR *xdrs, Create_LinkResp *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_ErrorCode (xdrs, &objp->error))
		 return FALSE;
	 if (!xdr_Device_Link (xdrs, &objp->lid))
		 return FALSE;
	 if (!xdr_u_short (xdrs, &objp->abortPort))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->maxRecvSize))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_WriteParms (XDR *xdrs, Device_WriteParms *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_Link (xdrs, &objp->lid))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->io_timeout))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->lock_timeout))
		 return FALSE;
	 if (!xdr_Device_Flags (xdrs, &objp->flags))
		 return FALSE;
	 if (!xdr_bytes (xdrs, (char **)&objp->data.data_val, (u_int *) &objp->data.data_len, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_WriteResp (XDR *xdrs, Device_WriteResp *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_ErrorCode (xdrs, &objp->error))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->size))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_ReadParms (XDR *xdrs, Device_ReadParms *objp)
{
	register int32_t *buf;


	if (xdrs->x_op == XDR_ENCODE) {
		 if (!xdr_Device_Link (xdrs, &objp->lid))
			 return FALSE;
		buf = XDR_INLINE (xdrs, 3 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_u_long (xdrs, &objp->requestSize))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->io_timeout))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->lock_timeout))
				 return FALSE;

		} else {
		IXDR_PUT_U_LONG(buf, objp->requestSize);
		IXDR_PUT_U_LONG(buf, objp->io_timeout);
		IXDR_PUT_U_LONG(buf, objp->lock_timeout);
		}
		 if (!xdr_Device_Flags (xdrs, &objp->flags))
			 return FALSE;
		 if (!xdr_char (xdrs, &objp->termChar))
			 return FALSE;
		return TRUE;
	} else if (xdrs->x_op == XDR_DECODE) {
		 if (!xdr_Device_Link (xdrs, &objp->lid))
			 return FALSE;
		buf = XDR_INLINE (xdrs, 3 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_u_long (xdrs, &objp->requestSize))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->io_timeout))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->lock_timeout))
				 return FALSE;

		} else {
		objp->requestSize = IXDR_GET_U_LONG(buf);
		objp->io_timeout = IXDR_GET_U_LONG(buf);
		objp->lock_timeout = IXDR_GET_U_LONG(buf);
		}
		 if (!xdr_Device_Flags (xdrs, &objp->flags))
			 return FALSE;
		 if (!xdr_char (xdrs, &objp->termChar))
			 return FALSE;
	 return TRUE;
	}

	 if (!xdr_Device_Link (xdrs, &objp->lid))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->requestSize))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->io_timeout))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->lock_timeout))
		 return FALSE;
	 if (!xdr_Device_Flags (xdrs, &objp->flags))
		 return FALSE;
	 if (!xdr_char (xdrs, &objp->termChar))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_ReadResp (XDR *xdrs, Device_ReadResp *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_ErrorCode (xdrs, &objp->error))
		 return FALSE;
	 if (!xdr_long (xdrs, &objp->reason))
		 return FALSE;
	 if (!xdr_bytes (xdrs, (char **)&objp->data.data_val, (u_int *) &objp->data.data_len, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_ReadStbResp (XDR *xdrs, Device_ReadStbResp *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_ErrorCode (xdrs, &objp->error))
		 return FALSE;
	 if (!xdr_u_char (xdrs, &objp->stb))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_GenericParms (XDR *xdrs, Device_GenericParms *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_Link (xdrs, &objp->lid))
		 return FALSE;
	 if (!xdr_Device_Flags (xdrs, &objp->flags))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->lock_timeout))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->io_timeout))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_RemoteFunc (XDR *xdrs, Device_RemoteFunc *objp)
{
	register int32_t *buf;


	if (xdrs->x_op == XDR_ENCODE) {
		buf = XDR_INLINE (xdrs, 4 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_u_long (xdrs, &objp->hostAddr))
				 return FALSE;
			 if (!xdr_u_short (xdrs, &objp->hostPort))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->progNum))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->progVers))
				 return FALSE;

		} else {
		IXDR_PUT_U_LONG(buf, objp->hostAddr);
		IXDR_PUT_U_SHORT(buf, objp->hostPort);
		IXDR_PUT_U_LONG(buf, objp->progNum);
		IXDR_PUT_U_LONG(buf, objp->progVers);
		}
		 if (!xdr_Device_AddrFamily (xdrs, &objp->progFamily))
			 return FALSE;
		return TRUE;
	} else if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE (xdrs, 4 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_u_long (xdrs, &objp->hostAddr))
				 return FALSE;
			 if (!xdr_u_short (xdrs, &objp->hostPort))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->progNum))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->progVers))
				 return FALSE;

		} else {
		objp->hostAddr = IXDR_GET_U_LONG(buf);
		objp->hostPort = IXDR_GET_U_SHORT(buf);
		objp->progNum = IXDR_GET_U_LONG(buf);
		objp->progVers = IXDR_GET_U_LONG(buf);
		}
		 if (!xdr_Device_AddrFamily (xdrs, &objp->progFamily))
			 return FALSE;
	 return TRUE;
	}

	 if (!xdr_u_long (xdrs, &objp->hostAddr))
		 return FALSE;
	 if (!xdr_u_short (xdrs, &objp->hostPort))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->progNum))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->progVers))
		 return FALSE;
	 if (!xdr_Device_AddrFamily (xdrs, &objp->progFamily))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_EnableSrqParms (XDR *xdrs, Device_EnableSrqParms *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_Link (xdrs, &objp->lid))
		 return FALSE;
	 if (!xdr_bool (xdrs, &objp->enable))
		 return FALSE;
	 if (!xdr_bytes (xdrs, (char **)&objp->handle.handle_val, (u_int *) &objp->handle.handle_len, 40))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_LockParms (XDR *xdrs, Device_LockParms *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_Link (xdrs, &objp->lid))
		 return FALSE;
	 if (!xdr_Device_Flags (xdrs, &objp->flags))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->lock_timeout))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_DocmdParms (XDR *xdrs, Device_DocmdParms *objp)
{
	register int32_t *buf;


	if (xdrs->x_op == XDR_ENCODE) {
		 if (!xdr_Device_Link (xdrs, &objp->lid))
			 return FALSE;
		 if (!xdr_Device_Flags (xdrs, &objp->flags))
			 return FALSE;
		buf = XDR_INLINE (xdrs, 5 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_u_long (xdrs, &objp->io_timeout))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->lock_timeout))
				 return FALSE;
			 if (!xdr_long (xdrs, &objp->cmd))
				 return FALSE;
			 if (!xdr_bool (xdrs, &objp->network_order))
				 return FALSE;
			 if (!xdr_long (xdrs, &objp->datasize))
				 return FALSE;

		} else {
		IXDR_PUT_U_LONG(buf, objp->io_timeout);
		IXDR_PUT_U_LONG(buf, objp->lock_timeout);
		IXDR_PUT_LONG(buf, objp->cmd);
		IXDR_PUT_BOOL(buf, objp->network_order);
		IXDR_PUT_LONG(buf, objp->datasize);
		}
		 if (!xdr_bytes (xdrs, (char **)&objp->data_in.data_in_val, (u_int *) &objp->data_in.data_in_len, ~0))
			 return FALSE;
		return TRUE;
	} else if (xdrs->x_op == XDR_DECODE) {
		 if (!xdr_Device_Link (xdrs, &objp->lid))
			 return FALSE;
		 if (!xdr_Device_Flags (xdrs, &objp->flags))
			 return FALSE;
		buf = XDR_INLINE (xdrs, 5 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_u_long (xdrs, &objp->io_timeout))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->lock_timeout))
				 return FALSE;
			 if (!xdr_long (xdrs, &objp->cmd))
				 return FALSE;
			 if (!xdr_bool (xdrs, &objp->network_order))
				 return FALSE;
			 if (!xdr_long (xdrs, &objp->datasize))
				 return FALSE;

		} else {
		objp->io_timeout = IXDR_GET_U_LONG(buf);
		objp->lock_timeout = IXDR_GET_U_LONG(buf);
		objp->cmd = IXDR_GET_LONG(buf);
		objp->network_order = IXDR_GET_BOOL(buf);
		objp->datasize = IXDR_GET_LONG(buf);
		}
		 if (!xdr_bytes (xdrs, (char **)&objp->data_in.data_in_val, (u_int *) &objp->data_in.data_in_len, ~0))
			 return FALSE;
	 return TRUE;
	}

	 if (!xdr_Device_Link (xdrs, &objp->lid))
		 return FALSE;
	 if (!xdr_Device_Flags (xdrs, &objp->flags))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->io_timeout))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->lock_timeout))
		 return FALSE;
	 if (!xdr_long (xdrs, &objp->cmd))
		 return FALSE;
	 if (!xdr_bool (xdrs, &objp->network_order))
		 return FALSE;
	 if (!xdr_long (xdrs, &objp->datasize))
		 return FALSE;
	 if (!xdr_bytes (xdrs, (char **)&objp->data_in.data_in_val, (u_int *) &objp->data_in.data_in_len, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_DocmdResp (XDR *xdrs, Device_DocmdResp *objp)
{
	register int32_t *buf;

	 if (!xdr_Device_ErrorCode (xdrs, &objp->error))
		 return FALSE;
	 if (!xdr_bytes (xdrs, (char **)&objp->data_out.data_out_val, (u_int *) &objp->data_out.data_out_len, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_Device_SrqParms (XDR *xdrs, Device_SrqParms *objp)
{
	register int32_t *buf;

	 if (!xdr_bytes (xdrs, (char **)&objp->handle.handle_val, (u_int *) &objp->handle.handle_len, ~0))
		 return FALSE;
	return TRUE;
}
