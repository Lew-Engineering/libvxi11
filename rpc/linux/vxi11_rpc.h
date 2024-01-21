/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _VXI11_RPC_H_RPCGEN
#define _VXI11_RPC_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef long Device_Link;

enum Device_AddrFamily {
	DEVICE_TCP = 0,
	DEVICE_UDP = 1,
};
typedef enum Device_AddrFamily Device_AddrFamily;

typedef long Device_Flags;

typedef long Device_ErrorCode;

struct Device_Error {
	Device_ErrorCode error;
};
typedef struct Device_Error Device_Error;

struct Create_LinkParms {
	long clientId;
	bool_t lockDevice;
	u_long lock_timeout;
	char *device;
};
typedef struct Create_LinkParms Create_LinkParms;

struct Create_LinkResp {
	Device_ErrorCode error;
	Device_Link lid;
	u_short abortPort;
	u_long maxRecvSize;
};
typedef struct Create_LinkResp Create_LinkResp;

struct Device_WriteParms {
	Device_Link lid;
	u_long io_timeout;
	u_long lock_timeout;
	Device_Flags flags;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct Device_WriteParms Device_WriteParms;

struct Device_WriteResp {
	Device_ErrorCode error;
	u_long size;
};
typedef struct Device_WriteResp Device_WriteResp;

struct Device_ReadParms {
	Device_Link lid;
	u_long requestSize;
	u_long io_timeout;
	u_long lock_timeout;
	Device_Flags flags;
	char termChar;
};
typedef struct Device_ReadParms Device_ReadParms;

struct Device_ReadResp {
	Device_ErrorCode error;
	long reason;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct Device_ReadResp Device_ReadResp;

struct Device_ReadStbResp {
	Device_ErrorCode error;
	u_char stb;
};
typedef struct Device_ReadStbResp Device_ReadStbResp;

struct Device_GenericParms {
	Device_Link lid;
	Device_Flags flags;
	u_long lock_timeout;
	u_long io_timeout;
};
typedef struct Device_GenericParms Device_GenericParms;

struct Device_RemoteFunc {
	u_long hostAddr;
	u_short hostPort;
	u_long progNum;
	u_long progVers;
	Device_AddrFamily progFamily;
};
typedef struct Device_RemoteFunc Device_RemoteFunc;

struct Device_EnableSrqParms {
	Device_Link lid;
	bool_t enable;
	struct {
		u_int handle_len;
		char *handle_val;
	} handle;
};
typedef struct Device_EnableSrqParms Device_EnableSrqParms;

struct Device_LockParms {
	Device_Link lid;
	Device_Flags flags;
	u_long lock_timeout;
};
typedef struct Device_LockParms Device_LockParms;

struct Device_DocmdParms {
	Device_Link lid;
	Device_Flags flags;
	u_long io_timeout;
	u_long lock_timeout;
	long cmd;
	bool_t network_order;
	long datasize;
	struct {
		u_int data_in_len;
		char *data_in_val;
	} data_in;
};
typedef struct Device_DocmdParms Device_DocmdParms;

struct Device_DocmdResp {
	Device_ErrorCode error;
	struct {
		u_int data_out_len;
		char *data_out_val;
	} data_out;
};
typedef struct Device_DocmdResp Device_DocmdResp;

struct Device_SrqParms {
	struct {
		u_int handle_len;
		char *handle_val;
	} handle;
};
typedef struct Device_SrqParms Device_SrqParms;

#define DEVICE_ASYNC 0x0607B0
#define DEVICE_ASYNC_VERSION 1

#if defined(__STDC__) || defined(__cplusplus)
#define device_abort 1
extern  Device_Error * device_abort_1(Device_Link *, CLIENT *);
extern  Device_Error * device_abort_1_svc(Device_Link *, struct svc_req *);
extern int device_async_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define device_abort 1
extern  Device_Error * device_abort_1();
extern  Device_Error * device_abort_1_svc();
extern int device_async_1_freeresult ();
#endif /* K&R C */

#define DEVICE_CORE 0x0607AF
#define DEVICE_CORE_VERSION 1

#if defined(__STDC__) || defined(__cplusplus)
#define create_link 10
extern  Create_LinkResp * create_link_1(Create_LinkParms *, CLIENT *);
extern  Create_LinkResp * create_link_1_svc(Create_LinkParms *, struct svc_req *);
#define device_write 11
extern  Device_WriteResp * device_write_1(Device_WriteParms *, CLIENT *);
extern  Device_WriteResp * device_write_1_svc(Device_WriteParms *, struct svc_req *);
#define device_read 12
extern  Device_ReadResp * device_read_1(Device_ReadParms *, CLIENT *);
extern  Device_ReadResp * device_read_1_svc(Device_ReadParms *, struct svc_req *);
#define device_readstb 13
extern  Device_ReadStbResp * device_readstb_1(Device_GenericParms *, CLIENT *);
extern  Device_ReadStbResp * device_readstb_1_svc(Device_GenericParms *, struct svc_req *);
#define device_trigger 14
extern  Device_Error * device_trigger_1(Device_GenericParms *, CLIENT *);
extern  Device_Error * device_trigger_1_svc(Device_GenericParms *, struct svc_req *);
#define device_clear 15
extern  Device_Error * device_clear_1(Device_GenericParms *, CLIENT *);
extern  Device_Error * device_clear_1_svc(Device_GenericParms *, struct svc_req *);
#define device_remote 16
extern  Device_Error * device_remote_1(Device_GenericParms *, CLIENT *);
extern  Device_Error * device_remote_1_svc(Device_GenericParms *, struct svc_req *);
#define device_local 17
extern  Device_Error * device_local_1(Device_GenericParms *, CLIENT *);
extern  Device_Error * device_local_1_svc(Device_GenericParms *, struct svc_req *);
#define device_lock 18
extern  Device_Error * device_lock_1(Device_LockParms *, CLIENT *);
extern  Device_Error * device_lock_1_svc(Device_LockParms *, struct svc_req *);
#define device_unlock 19
extern  Device_Error * device_unlock_1(Device_Link *, CLIENT *);
extern  Device_Error * device_unlock_1_svc(Device_Link *, struct svc_req *);
#define device_enable_srq 20
extern  Device_Error * device_enable_srq_1(Device_EnableSrqParms *, CLIENT *);
extern  Device_Error * device_enable_srq_1_svc(Device_EnableSrqParms *, struct svc_req *);
#define device_docmd 22
extern  Device_DocmdResp * device_docmd_1(Device_DocmdParms *, CLIENT *);
extern  Device_DocmdResp * device_docmd_1_svc(Device_DocmdParms *, struct svc_req *);
#define destroy_link 23
extern  Device_Error * destroy_link_1(Device_Link *, CLIENT *);
extern  Device_Error * destroy_link_1_svc(Device_Link *, struct svc_req *);
#define create_intr_chan 25
extern  Device_Error * create_intr_chan_1(Device_RemoteFunc *, CLIENT *);
extern  Device_Error * create_intr_chan_1_svc(Device_RemoteFunc *, struct svc_req *);
#define destroy_intr_chan 26
extern  Device_Error * destroy_intr_chan_1(void *, CLIENT *);
extern  Device_Error * destroy_intr_chan_1_svc(void *, struct svc_req *);
extern int device_core_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define create_link 10
extern  Create_LinkResp * create_link_1();
extern  Create_LinkResp * create_link_1_svc();
#define device_write 11
extern  Device_WriteResp * device_write_1();
extern  Device_WriteResp * device_write_1_svc();
#define device_read 12
extern  Device_ReadResp * device_read_1();
extern  Device_ReadResp * device_read_1_svc();
#define device_readstb 13
extern  Device_ReadStbResp * device_readstb_1();
extern  Device_ReadStbResp * device_readstb_1_svc();
#define device_trigger 14
extern  Device_Error * device_trigger_1();
extern  Device_Error * device_trigger_1_svc();
#define device_clear 15
extern  Device_Error * device_clear_1();
extern  Device_Error * device_clear_1_svc();
#define device_remote 16
extern  Device_Error * device_remote_1();
extern  Device_Error * device_remote_1_svc();
#define device_local 17
extern  Device_Error * device_local_1();
extern  Device_Error * device_local_1_svc();
#define device_lock 18
extern  Device_Error * device_lock_1();
extern  Device_Error * device_lock_1_svc();
#define device_unlock 19
extern  Device_Error * device_unlock_1();
extern  Device_Error * device_unlock_1_svc();
#define device_enable_srq 20
extern  Device_Error * device_enable_srq_1();
extern  Device_Error * device_enable_srq_1_svc();
#define device_docmd 22
extern  Device_DocmdResp * device_docmd_1();
extern  Device_DocmdResp * device_docmd_1_svc();
#define destroy_link 23
extern  Device_Error * destroy_link_1();
extern  Device_Error * destroy_link_1_svc();
#define create_intr_chan 25
extern  Device_Error * create_intr_chan_1();
extern  Device_Error * create_intr_chan_1_svc();
#define destroy_intr_chan 26
extern  Device_Error * destroy_intr_chan_1();
extern  Device_Error * destroy_intr_chan_1_svc();
extern int device_core_1_freeresult ();
#endif /* K&R C */

#define DEVICE_INTR 0x0607B1
#define DEVICE_INTR_VERSION 1

#if defined(__STDC__) || defined(__cplusplus)
#define device_intr_srq 30
extern  void * device_intr_srq_1(Device_SrqParms *, CLIENT *);
extern  void * device_intr_srq_1_svc(Device_SrqParms *, struct svc_req *);
extern int device_intr_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define device_intr_srq 30
extern  void * device_intr_srq_1();
extern  void * device_intr_srq_1_svc();
extern int device_intr_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_Device_Link (XDR *, Device_Link*);
extern  bool_t xdr_Device_AddrFamily (XDR *, Device_AddrFamily*);
extern  bool_t xdr_Device_Flags (XDR *, Device_Flags*);
extern  bool_t xdr_Device_ErrorCode (XDR *, Device_ErrorCode*);
extern  bool_t xdr_Device_Error (XDR *, Device_Error*);
extern  bool_t xdr_Create_LinkParms (XDR *, Create_LinkParms*);
extern  bool_t xdr_Create_LinkResp (XDR *, Create_LinkResp*);
extern  bool_t xdr_Device_WriteParms (XDR *, Device_WriteParms*);
extern  bool_t xdr_Device_WriteResp (XDR *, Device_WriteResp*);
extern  bool_t xdr_Device_ReadParms (XDR *, Device_ReadParms*);
extern  bool_t xdr_Device_ReadResp (XDR *, Device_ReadResp*);
extern  bool_t xdr_Device_ReadStbResp (XDR *, Device_ReadStbResp*);
extern  bool_t xdr_Device_GenericParms (XDR *, Device_GenericParms*);
extern  bool_t xdr_Device_RemoteFunc (XDR *, Device_RemoteFunc*);
extern  bool_t xdr_Device_EnableSrqParms (XDR *, Device_EnableSrqParms*);
extern  bool_t xdr_Device_LockParms (XDR *, Device_LockParms*);
extern  bool_t xdr_Device_DocmdParms (XDR *, Device_DocmdParms*);
extern  bool_t xdr_Device_DocmdResp (XDR *, Device_DocmdResp*);
extern  bool_t xdr_Device_SrqParms (XDR *, Device_SrqParms*);

#else /* K&R C */
extern bool_t xdr_Device_Link ();
extern bool_t xdr_Device_AddrFamily ();
extern bool_t xdr_Device_Flags ();
extern bool_t xdr_Device_ErrorCode ();
extern bool_t xdr_Device_Error ();
extern bool_t xdr_Create_LinkParms ();
extern bool_t xdr_Create_LinkResp ();
extern bool_t xdr_Device_WriteParms ();
extern bool_t xdr_Device_WriteResp ();
extern bool_t xdr_Device_ReadParms ();
extern bool_t xdr_Device_ReadResp ();
extern bool_t xdr_Device_ReadStbResp ();
extern bool_t xdr_Device_GenericParms ();
extern bool_t xdr_Device_RemoteFunc ();
extern bool_t xdr_Device_EnableSrqParms ();
extern bool_t xdr_Device_LockParms ();
extern bool_t xdr_Device_DocmdParms ();
extern bool_t xdr_Device_DocmdResp ();
extern bool_t xdr_Device_SrqParms ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_VXI11_RPC_H_RPCGEN */
