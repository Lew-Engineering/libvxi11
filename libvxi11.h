#ifndef LIBVXI11_H
#define LIBVXI11_H

// ***************************************************************************
// libvxi11.h - Header file for libvxi11.so library
//              VXI-11 protocol for instrument communication via TCP/IP
//
// Written by Eddie Lew, Lew Engineering
// Copyright (C) 2020 Eddie Lew
//
// Edit history:
//
// 09-17-22 - Added support for SRQ (service request) interrupt callback with
//              enable_srq() and srq_callback().
//            Added device_addr() to get back string used to open the device.
// 08-22-22 - Added read_terminator() functions to set and get the terminator
//              used when reading data from the device.  Default is END (EOI
//              for GPIB).
// 08-21-22 - Added error description to error messages.
//            Added docmd_*() functions for low level control of
//              GPIB/LAN gateways.
// 05-18-20 - Started file.
// ***************************************************************************

// ***************************************************************************
// Example of basic use of Vxi11 class
// 
//   Vxi11 vxi11 ("dmm6500");      // Open connection to device
//   vxi11.printf ("*idn?");       // Send ID query
//   char s_msg[1000];
//   vxi11.read (s_msg, 1000);     // Read query response
//   printf ("ID = %s\n", s_msg);  // Print response to screen
//
// See the function header comments in vxi11.cpp for complete documentation
// of the use of each function.
// ***************************************************************************

class Vxi11 {
  
  // *************************************************************************
  // Private members
  // *************************************************************************
 private:

  int _b_valid;                         // 1 = has connection to device

  double _d_timeout;                    // Timeout time, in seconds
  int _timeout_ms;                      // Timeout time, in milliseconds

  char _c_read_terminator;              // Read termination character
                                        // -1 = END (use EOI for GPIB))
  
  void *__p_client;                     // RPC client, type CLIENT*
                                        // Use macro _p_client for access
  void *__p_link;                       // VXI-11 link, type Create_LinkResp*
                                        // Use macro _p_link for access

  char _s_device_addr[256];             // Device address & name used in the
                                        // constructor or open()

  bool _b_srq_ena;                      // True if SRQ interrupt is enabled
  bool _b_srq_udp;                      // True if SRQ uses UDP, else TCP
  char _a_srq_handle[40];               // Unique handle for SRQ interrupt
                                        // thread access to the devices
  static void *_p_pthread_svc_run;      // Thread pointer for _fn_svc_run()
  static void *_fn_svc_run(void *p_arg);// Func to run svc_run() for SRQ intr
  static void *_p_svcXprt_srq_tcp;      // TCP RPC service transport for SRQ
  static void *_p_svcXprt_srq_udp;      // UDP RPC service transport for SRQ
  static void _fn_srq_callback (void *, void *); // Internal callback for SRQ
  static void (*_pfn_srq_callback)(Vxi11 *); // User callback function for SRQ
  
  enum {CNT_ERR_DESC_MAX=32};           // Description of RPC call error codes
  static const char *_as_err_desc[CNT_ERR_DESC_MAX];
  
  // *************************************************************************
  // Public members
  //
  // NOTE: See function header comments in vxi11.cpp for full documentation
  // *************************************************************************
 public:
  
  // Default constructor
  Vxi11 (void);

  // Constructor to open connection to device
  // VXI-11 RPC is "create_link"
  Vxi11 (const char *s_address, const char *s_device = 0, int *p_err = 0);

  // Destructor
  // VXI-11 RPC is "destroy_link"
  ~Vxi11 ();

  // Open connection to device (if default constructor used);
  // VXI-11 RPC is "create_link"
  int open (const char *s_address, const char *s_device);

  // Close connection to device (if destructor is not used)
  // VXI-11 RPC is "destroy_link"
  int close (void);

  // Set/get timeout time in seconds
  // Default timeout is 10 seconds
  void timeout (double d_timeout);
  double timeout (void);

  // Set/get the read termination method
  // -1 = END (EOI line for GPIB); this is the default
  // Some devices use 10 (linefeed) or 0 (null)
  void read_terminator (char c_term) { _c_read_terminator = c_term; }
  char read_terminator (void) { return (_c_read_terminator); }

  // Get device address and name used in contructor or open()
  // This can be used in the SRQ callback function to identify which Vxi11
  // object is calling the callback.
  const char *device_addr (void) {
    return (_s_device_addr);
    }

  // Write data to device
  // VXI-11 RPC is "device_write"
  int write (const char *ac_data, int cnt_data);

  // Write data to device, printf style
  // VXI-11 RPC is "device_write"
  int printf (const char *s_format, ...);

  // Read data from device
  // VXI-11 RPC is "device_read"
  int read (char *ac_data, int cnt_data_max, int *pcnt_data = 0);

  // Query for a value (double, int, or string)
  // Convenience functions combines write and read
  int query (const char *s_query, double *pd_val);
  int query (const char *s_query, int *pi_val);
  int query (const char *s_query, char *s_val, int len_val_max);
  
  // Read status byte (serial poll)
  // VXI-11 RPC is "device_readstb"
  int readstb (void);

  // Send group execute trigger (GET)
  // VXI-11 RPC is "device_trigger"
  int trigger (void);

  // Send device clear
  // VXI-11 RPC is "device_clear"
  int clear (void);

  // Place device in remote state
  // VXI-11 RPC is "device_remote"
  int remote (void);

  // Place device in local state
  // VXI-11 RPC is "device_local"
  int local (void);

  // Lock device for exclusive access
  // VXI-11 RPC is "device_lock"
  int lock (void);

  // Unlock device to release exclusive access
  // VXI-11 RPC is "device_unlock"
  int unlock (void);

  // Set the callback function for SRQ (service request) interrupt
  static int srq_callback (void (*pfn_srq_callback)(Vxi11 *));
  
  // Enable/disable SRQ (service request) interrupt
  // VXI-11 RPCs are "device_enable_srq"
  //                 "create_intr_chan"
  //                 "destroy_intr_chan"
  int enable_srq (bool b_ena, bool b_udp = false);
  
  // Send raw GPIB command codes via GPIB/LAN gateway
  // VXI-11 RPC is "device_docmd" with command 0x20000 "Send command"
  int docmd_send_command (const char *s_data);

  // Get GPIB bus status via GPIB/LAN gateway
  // VXI-11 RPC is "device_docmd" with command 0x20001 "Bus status"
  int docmd_bus_status (int type);

  // Set GPIB ATN line state via GPIB/LAN gateway
  // VXI-11 RPC is "device_docmd" with command 0x20002 "ATN control"
  int docmd_atn_control (bool b_state);

  // Set GPIB REN line state via GPIB/LAN gateway
  // VXI-11 RPC is "device_docmd" with command 0x20003 "REN control"
  int docmd_ren_control (bool b_state);

  // Pass control to another GPIB controller via GPIB/LAN gateway
  // VXI-11 RPC is "device_docmd" with command 0x20004 "Pass control"
  int docmd_pass_control (int addr);

  // Set GPIB address of the GPIB/LAN gateway
  // VXI-11 RPC is "device_docmd" with command 0x2000A "Bus address"
  int docmd_bus_address (int addr);

  // Toggle GPIB IFC line state via GPIB/LAN gateway
  // VXI-11 RPC is "device_docmd" with command 0x20010 "IFC control"
  int docmd_ifc_control (void);
};

#endif
