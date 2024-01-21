/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "vxi11_rpc.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

Device_Error *
device_abort_1(Device_Link *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_abort,
		(xdrproc_t) xdr_Device_Link, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Create_LinkResp *
create_link_1(Create_LinkParms *argp, CLIENT *clnt)
{
	static Create_LinkResp clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, create_link,
		(xdrproc_t) xdr_Create_LinkParms, (caddr_t) argp,
		(xdrproc_t) xdr_Create_LinkResp, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_WriteResp *
device_write_1(Device_WriteParms *argp, CLIENT *clnt)
{
	static Device_WriteResp clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_write,
		(xdrproc_t) xdr_Device_WriteParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_WriteResp, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_ReadResp *
device_read_1(Device_ReadParms *argp, CLIENT *clnt)
{
	static Device_ReadResp clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_read,
		(xdrproc_t) xdr_Device_ReadParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_ReadResp, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_ReadStbResp *
device_readstb_1(Device_GenericParms *argp, CLIENT *clnt)
{
	static Device_ReadStbResp clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_readstb,
		(xdrproc_t) xdr_Device_GenericParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_ReadStbResp, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
device_trigger_1(Device_GenericParms *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_trigger,
		(xdrproc_t) xdr_Device_GenericParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
device_clear_1(Device_GenericParms *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_clear,
		(xdrproc_t) xdr_Device_GenericParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
device_remote_1(Device_GenericParms *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_remote,
		(xdrproc_t) xdr_Device_GenericParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
device_local_1(Device_GenericParms *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_local,
		(xdrproc_t) xdr_Device_GenericParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
device_lock_1(Device_LockParms *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_lock,
		(xdrproc_t) xdr_Device_LockParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
device_unlock_1(Device_Link *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_unlock,
		(xdrproc_t) xdr_Device_Link, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
device_enable_srq_1(Device_EnableSrqParms *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_enable_srq,
		(xdrproc_t) xdr_Device_EnableSrqParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_DocmdResp *
device_docmd_1(Device_DocmdParms *argp, CLIENT *clnt)
{
	static Device_DocmdResp clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_docmd,
		(xdrproc_t) xdr_Device_DocmdParms, (caddr_t) argp,
		(xdrproc_t) xdr_Device_DocmdResp, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
destroy_link_1(Device_Link *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, destroy_link,
		(xdrproc_t) xdr_Device_Link, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
create_intr_chan_1(Device_RemoteFunc *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, create_intr_chan,
		(xdrproc_t) xdr_Device_RemoteFunc, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

Device_Error *
destroy_intr_chan_1(void *argp, CLIENT *clnt)
{
	static Device_Error clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, destroy_intr_chan,
		(xdrproc_t) xdr_void, (caddr_t) argp,
		(xdrproc_t) xdr_Device_Error, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

void *
device_intr_srq_1(Device_SrqParms *argp, CLIENT *clnt)
{
	static char clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, device_intr_srq,
		(xdrproc_t) xdr_Device_SrqParms, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)&clnt_res);
}
