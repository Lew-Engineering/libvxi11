// ***************************************************************************
// vxi11.cpp - Implentation of the Vxi11 class for libvxi11.so library
//             VXI-11 protocol for instrument communication via TCP/IP
//
// Written by Eddie Lew, Lew Engineering
// Copyright (C) 2020 Eddie Lew
//
// Edit history:
//
// 09-23-22 - Added support for the abort channel, added abort() function.
// 09-17-22 - Added support for SRQ (service request) interrupt callback with
//              enable_srq() and srq_callback().
//            Support multi-threaded operation by adding mutex around RPC use.
// 08-22-22 - read(): Use read termination method specified by
//              read_terminator().  Defualt is END (EOI for GPIB).
// 08-21-22 - Added error description to error messages.
//            Added docmd_*() functions for low level control of
//              GPIB/LAN gateways.
// 11-27-21 - read(): Add number of bytes read in error message when END
//              indicator is not found.
// 06-10-20 - First releaesd version.
// 05-18-20 - Started file.
// ***************************************************************************

#include "libvxi11.h"
#include "vxi11_rpc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <rpc/pmap_clnt.h>

// Macro to conveniently access __p_client and __p_link members of the class.
// They are defined as void * in the class so that the .h file does not need
// to include the RPC interface.
#define _p_client ((CLIENT *)__p_client)
#define _p_link ((Create_LinkResp *)__p_link)

#define _p_client_abort ((CLIENT *)__p_client_abort)

// Static members to support SRQ callback function
void *Vxi11::_p_pthread_svc_run  = 0;   // Thread pointer for _fn_svc_run()
void *Vxi11::_p_svcXprt_srq_tcp = 0;    // TCP RPC service transport for SRQ
void *Vxi11::_p_svcXprt_srq_udp = 0;    // UDP RPC service transport for SRQ
void (*Vxi11::_pfn_srq_callback)(Vxi11 *) = 0; // User callback for SRQ intr

// Error description for each error code from the VXI-11 RPC calls
const char *Vxi11::_as_err_desc[Vxi11::CNT_ERR_DESC_MAX] =
  {"",                                  // 0 (no error)
   "syntax error",                      // 1
   "",                                  // 2
   "device not accessible",             // 3
   "invalid link identifier",           // 4
   "parameter error",                   // 5
   "channel not established",           // 6
   "",                                  // 7
   "operation not supported",           // 8
   "out of resources",                  // 9
   "",                                  // 10
   "device locked by another link",     // 11
   "no lock held by this link",         // 12
   "",                                  // 13
   "",                                  // 14
   "I/O timeout",                       // 15
   "",                                  // 16
   "I/O error",                         // 17
   "",                                  // 18
   "",                                  // 19
   "",                                  // 20
   "invalid address",                   // 21
   "",                                  // 22
   "abort",                             // 23
   "",                                  // 24
   "",                                  // 25
   "",                                  // 26
   "",                                  // 27
   "",                                  // 28
   "channel already established",       // 29
   "",                                  // 30
   "",                                  // 31
  };

// ***************************************************************************
// Vxi11Mutex - Class to serialize access VXI-11 RPCs via pthread mutexes
//
// This is needed to properly handle SRQ interrupts and Vxi11 objects
// used in multiple threads.
//
// Create a local instance of this class in each function where a lock on the
// mutex is required.  The mutex will be unlocked when that instance goes out
// of scope, such as when the function returns.
// ***************************************************************************
class Vxi11Mutex
{
  private:
    static pthread_mutex_t mutex;       // Mutex to lock & unlock
  
  public:
  // Constructor to lock mutex
  Vxi11Mutex () {
    int err = pthread_mutex_lock (&mutex);
    if (err)
      fprintf (stderr, "Vxi11 error: could not lock mutex, error %d", err);
    }

  // Destructor to unlock mutex
  ~Vxi11Mutex () {
    int err = pthread_mutex_unlock (&mutex);
    if (err)
      fprintf (stderr, "Vxi11 error: could not unlock mutex, error %d", err);
    }
};

pthread_mutex_t Vxi11Mutex::mutex = PTHREAD_MUTEX_INITIALIZER;

// ***************************************************************************
// Vxi11 default constructor - Do not connect to device yet
//
// Parameters: None
// Returns:    N/A
//
// Notes: Use this constructor if you want to connect to the device at
//        a later time using the open() call.
// ***************************************************************************
  Vxi11::
Vxi11 (void)
{
  _b_valid = 0;                         // No connection to device
  __p_client = 0;                       // No RPC client yet
  __p_link = 0;                         // No link to device yet
  __p_client_abort = 0;                 // No RPC client for abort channel yet
  _s_device_addr[0] = 0;                // No device address/name yet
  _ui_device_ip_addr = 0;               // No device IP address yet
  _b_srq_ena = false;                   // SRQ interrupt not enabled
  _b_srq_udp = false;                   // SRQ interrupt will use TCP, not UDP

  timeout (10.0);                       // Default timeout
}

// ***************************************************************************
// Vxi11 constructor - Open connection to device 
//
// Parameters:
// 1. s_address   - Device network address, either a host name or IP
//                  address in dot notation
//
// 2. s_device    - Device name at s_address
//
//                  May be set to null pointer if device is directly
//                  connected to the network (an Ethernet or Wifi device).
//
//                  For GPIB devices connected to a GPIB/LAN gateways, this is
//                  usually "gpib0,n", where 'n' is the GPIB address number of
//                  the GPIB device.
//
//                  For GPIB interfaces (the GPIB/LAN gateway itself), this is
//                  usually "gpib0".
//
// 3. p_err       - Returns error code if given non-zero pointer
//                  Stores to pointer locaion 0 = no error
//                                            1 = error
//
// Notes: If there was an error in the RPC call, that error message will
//        be printed to stderr.
// ***************************************************************************
  Vxi11::
Vxi11 (const char *s_address, const char *s_device, int *p_err)
{
  _b_valid = 0;                         // No connection to device
  __p_client = 0;                       // No RPC client yet
  __p_link = 0;                         // No link to device yet
  __p_client_abort = 0;                 // No RPC client for abort channel yet
  _s_device_addr[0] = 0;                // No device address/name yet
  _ui_device_ip_addr = 0;               // No device IP address yet
  _b_srq_ena = false;                   // SRQ interrupt not enabled
  _b_srq_udp = false;                   // SRQ interrupt will use TCP, not UDP

  timeout (10.0);                       // Default timeout
  read_terminator (-1);                 // Terminate read with END (EOI line
                                        // for GPIB)

  int err = open (s_address, s_device); // Connect to device

  if (p_err)                            // Return error to caller
    *p_err = err;
}

// ***************************************************************************
// Vxi11 destructor - Close connection to device if it was open
// ***************************************************************************
  Vxi11::
~Vxi11 ()
{
  if (_b_valid)                         // Close connection to device if
    close ();                           // it currently open
}

// ***************************************************************************
// Vxi11::open - Open connection to device
//               VXI-11 RPC is "create_link"
//
// Parameters:
// 1. s_address   - Device network address, either a host name or IP
//                  address in dot notation
// 2. s_device    - Device name at s_address
//                  May be set to null pointer if device is directly
//                  connected to the network.
//                  For GPIB/LAN gateways, this is usually "gpib0,n",
//                  where 'n' is the GPIB address number.  If connecting
//                  to the gateway itself, this is usually "gpib0".
//
// Returns:  0 = no error
//           1 = error
//
// Notes: If there was an error in the RPC call, that error message will
//        be printed to stderr.
//
//        Use this function if the default construtor was used, or if
//        re-opening the device after closing it.
// ***************************************************************************
  int Vxi11::
open (const char *s_address, const char *s_device)
{
  // Cannot open a new connection if one is already open in this instance
  if (_b_valid) {
    fprintf (stderr, "Vxi11::open error: connection already open.\n");
    return (1);
    }

  // Check if host name or IP address is not null
  if (!s_address) {
    fprintf (stderr, "Vxi11::open error: null address.\n");
    return (1);
    }
  
  // Use default device name if it is not specified
  if (!s_device)
    s_device = "inst0";                 // According to VXI-11.3 Rule B.1.2

  // Store the address and device name so that the user can distinguish
  // the Vxi11 object used.
  strlcpy (_s_device_addr, s_address, 256);
  strlcat (_s_device_addr, ":", 256);
  strlcat (_s_device_addr, s_device, 256);

  // *************************************************************************
  // Set up core RPC channel
  // *************************************************************************

  // Create a client of the RPC functions for device at given address
  const char *s_tcp = "tcp";
  __p_client = clnt_create ((char *)s_address, DEVICE_CORE,
                            DEVICE_CORE_VERSION, (char *)s_tcp);
    
  if (!_p_client) {                     // Exit early if error
    const char *s_err = "Vxi11 open error: client creation";
    clnt_pcreateerror ((char *)s_err);  // Print error message
    return (1);
    }

  // Create a link to the device
  Create_LinkParms linkParms;
  linkParms.clientId = (long)_p_client; // RPC client ID
  linkParms.lockDevice = 0;             // Do not lock device
  linkParms.lock_timeout = _timeout_ms; // Timeout in ms
  linkParms.device = (char *)s_device;  // Device name
  
  Create_LinkResp *p_link = create_link_1 (&linkParms, _p_client);
  
  if (!p_link) {                        // Exit early if error
    const char *s_err = "Vxi11::open error: link creation";
    clnt_perror (_p_client, (char *)s_err); // Print error message
    clnt_destroy (_p_client);
    return (1);
    }

  // Copy it to the object, since RPC will lose it on next call
  __p_link = (Create_LinkResp *)malloc (sizeof (Create_LinkResp));

  if (!_p_link) {                       // Exit early if error
    fprintf (stderr, "Vxi11::open error: could not allocate memory.\n");
    destroy_link_1 (&(p_link->lid), _p_client);
    clnt_destroy (_p_client);
    return (1);
    }
  
  *_p_link = *p_link;

  // Change underlying RPC timeout from 25s that was set in vxi11_rpc_clnt.c
  // to 120s to deal with slow devices
  // FIXME - this should follow the timeout() function value
  struct timeval timeval_timeout = {120, 0};
  clnt_control (_p_client, CLSET_TIMEOUT, (char *)(&timeval_timeout));

  // Get IP address of the device
  // This is used later if the abort channel is used
  hostent *p_hostent = gethostbyname (s_address);
  if (!p_hostent) {
    fprintf (stderr, "Vxi11::open error: could not get device IP address.\n");
    destroy_link_1 (&(p_link->lid), _p_client);
    clnt_destroy (_p_client);
    return (1);
    }
  _ui_device_ip_addr = *(unsigned int *)(p_hostent->h_addr_list[0]);
  
  _b_valid = 1;                         // Now have valid connection
  return (0);
}

// ***************************************************************************
// Vxi11::close - Close connection to the device
//                VXI-11 RPC is "destroy_link"
//
// Parameters: None
//
// Returns: 0 = no error
// ***************************************************************************
  int Vxi11::
close (void)
{
  if (!_b_valid)                        // Early return if no connection to
    return (0);                         // the device

  // Close SRQ interrupt channel
  // Leave RPC service running for SRQ since it is global to all Vxi11 objects
  int err = enable_srq (false);

  _b_valid = 0;                         // No connection to device
  
  // Close link to device
  Device_Error *p_error = destroy_link_1 (&(_p_link->lid), _p_client);  
  if (!p_error) { 
    fprintf (stderr, "Vxi11::close error: no RPC response.\n");
    err = 1;
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  else {
    int err_code = int (p_error->error);
    if (err_code) {
      int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                         err_code : 0;
      fprintf (stderr, "Vxi11::close error: destroy_link error %d %s.\n",
               err_code, _as_err_desc[idx_err_desc]);
      err = 1;
      }
    }
  
  free (_p_link);
  __p_link = 0;
  
  if (_p_client_abort) {
    clnt_destroy (_p_client_abort);     // Abort channel
    __p_client_abort = 0;
    }

  // Close RPC client
  clnt_destroy (_p_client);             // Core (normal) channel
  __p_client = 0;

  return (err);
}

// ***************************************************************************
// Vxi11::timeout - Set timeout time
//
// Parameters:
// 1. d_timeout - Timeout time, in seconds, for subsequent VXI-11 operations
//
// Returns: None
//
// Notes: Default timeout if this function is not called is 10 seconds.
// ***************************************************************************
  void Vxi11::
timeout (double d_timeout)
{
  if (d_timeout < 0)
    d_timeout = 0;
  
  _d_timeout = d_timeout;                     // Store timeout in s
  _timeout_ms = int (d_timeout * 1000 + 0.5); // Store timeout in ms
}

// ***************************************************************************
// Vxi11::timeout - Get timeout time
//
// Parameters: None
//
// Returns: Timeout time, in seconds
// ***************************************************************************
  double Vxi11::
timeout (void)
{
  return (_d_timeout);
}

// ***************************************************************************
// Vxi11::write - Write data to the device
//                VXI-11 RPC is "device_write"
//
// Parameters:
// 1. ac_data  - Data to send to device
// 2. cnt_data - Number of bytes in ac_data to send
//
// Returns: 0 = no error
//          1 = error
//
// Notes: If Vxi11 object is associated with a GPIB device or GPIB interface
//        via a GPIB/LAN gateway, GPIB uses SEND command sequence, with EOI
//        on last byte.
// ***************************************************************************
  int Vxi11::
write (const char *ac_data, int cnt_data)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::write error: no connection to device.\n");
    return (1);
    }

  // Check input parameters
  if ((!ac_data) || (cnt_data < 0)) {
    fprintf (stderr, "Vxi11::write error: invalid parameters.\n");
    return (1);
    }
  if (cnt_data == 0)                    // No error for sending no data
    return (0);
    
  Device_WriteParms writeParms;         // To send to device_write RPC call
  writeParms.lid = _p_link->lid;        // Link ID from create_link RPC call
  writeParms.io_timeout = _timeout_ms;  // Timeout for I/O in ms
  writeParms.lock_timeout = _timeout_ms;// Timeout for lock in ms

  // Get max # of bytes to send at a time
  int cnt_max = _p_link->maxRecvSize;
  if (cnt_max <= 0)                     // Check for valid max value
    cnt_max = 1024;                     // Default to 1K if not valid
  
  int cnt_left = cnt_data;              // Number of bytes left to send

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns
  
  // Loop sending data to the device, limited to the max allowed at a time
  do {
    // If remaining bytes to send is less than max
    if (cnt_left <= cnt_max) {
      writeParms.flags = 8;             // Indicate this is the end of the data
      writeParms.data.data_len = cnt_left; // Send remaining # of bytes
      }
    
    // Remaining bytes more than max, so send max
    else {
      writeParms.flags = 0;             // Not last block of data yet
      writeParms.data.data_len=cnt_max; // Send max allowed # of bytes
      }

    // Send data to device
    writeParms.data.data_val = (char *)(&ac_data[cnt_data - cnt_left]);

    Device_WriteResp *p_writeResp = device_write_1 (&writeParms, _p_client);

    if (p_writeResp == 0) {             // Error if device does not respond
      fprintf (stderr, "Vxi11::write error: no RPC response.\n");
      return (1);
      }

    // Possible errors
    //  0 = no error
    //  4 = invalid link identifier
    //  5 = parameter error
    // 11 = device locked by another link
    // 15 = I/O timeout
    // 17 = I/O error
    // 23 = abort
    int err_code = int (p_writeResp->error);
    if (err_code) {
      int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                         err_code : 0;
      fprintf (stderr, "Vxi11::write error: %d %s.\n",
               err_code, _as_err_desc[idx_err_desc]);
      return (1);
      }
    
    cnt_left -= p_writeResp->size;      // Number of bytes left
    } while (cnt_left > 0);             // End when all sent

  return (0);
}

// ***************************************************************************
// Vxi11::printf - Write data to the device using printf format
//                 VXI-11 RPC is "device_write"
//
// Parameters:
// 1. s_format - printf style format data to send to device
// 2. ...      - Variable number of parameters, depending on s_format
//
// Returns: 0 = no error
//          1 = error
//
// Notes: If Vxi11 object is associated with a GPIB device or GPIB interface
//        via a GPIB/LAN gateway, GPIB uses SEND command sequence, with EOI
//        on last byte.
// ***************************************************************************
  int Vxi11::
printf (const char *s_format, ...)
{
  if (!s_format) {                      // Check input parameters
    fprintf (stderr, "Vxi11::printf error: invalid parameters.\n");
    return (1);
    }

  const int CNT_DATA_MAX = 65536;       // Max string to send
  static char s_data[CNT_DATA_MAX];

  va_list va;                           // Process input like printf() does
  va_start (va, s_format);
  int cnt = vsnprintf (s_data, CNT_DATA_MAX, s_format, va);
  va_end (va);
  s_data[CNT_DATA_MAX-1] = 0;           // Make sure it is terminated
  
  if ((cnt < 0) || (cnt>CNT_DATA_MAX)) {// Check for error
    fprintf (stderr, "Vxi11::printf error: vsnprintf error, count = %d.\n",
             cnt);
    return (1);
    }
  
  // Send string to the device
  int err = write (s_data, strlen (s_data));
  
  return (err);
}

// ***************************************************************************
// Vxi11::read - Read data from the device
//               VXI-11 RPC is "device_read"
//
// Parameters:
// 1. ac_data      - Store read data here
// 2. cnt_data_max - Max length allocated in ac_data
// 3. pcnt_read    - Returns actual number of bytes returned in ac_data
//                   This does not include the null terminator
//
// Returns: 0 = no error
//          1 = error
//
// Notes: If Vxi11 object is associated with a GPIB device or GPIB interface
//        via a GPIB/LAN gateway, GPIB uses RECEIVE command sequence
// ***************************************************************************
  int Vxi11::
read (char *ac_data, int cnt_data_max, int *pcnt_read)
{
  int cnt_read_default;                 // Use local variable if user does not
  if (!pcnt_read)                       // specify pcnt_read parameter
    pcnt_read = &cnt_read_default;
  
  *pcnt_read = 0;                       // No bytes read yet

  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::read error: no connection to device.\n");
    return (1);
    }

  if (!ac_data || (cnt_data_max < 1)) { // Check input parameters
    fprintf (stderr, "Vxi11::read error: invalid parameters.\n");
    return (1);
    }

  ac_data[0] = 0;                       // Null string for early return
  
  Device_ReadParms readParms;           // To send to device_read RPC
  readParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  readParms.requestSize = cnt_data_max; // Maximum # of bytes to read
  readParms.io_timeout = _timeout_ms;	// Timeout for I/O in ms
  readParms.lock_timeout = _timeout_ms;	// Timeout for lock in ms

  if (_c_read_terminator == -1) {       // No term character, use END (EOI)
    readParms.flags = 0;                // No special flags
    readParms.termChar = 0;             // Termination character not used,
                                        // since flags = 0
    }
  else {
    readParms.flags = 128;              // Use termination character
    readParms.termChar = _c_read_terminator;
    }
  
  Vxi11Mutex vxi11Mutex;                // Lock access until function returns
  
  // Iterate reads, since internal buffer in device_read RPC call may be less
  // than the maximum number of bytes requested
  do {
    // Number of bytes to read
    readParms.requestSize = cnt_data_max - *pcnt_read;

    // Read from the device
    Device_ReadResp *p_readResp = device_read_1 (&readParms, _p_client);

    if (p_readResp == 0) {              // Check for error
      fprintf (stderr, "Vxi11::read error: no RPC response.\n");
      return (1);
      }

    // Copy read data to user buffer
    const int cnt_read = p_readResp->data.data_len; // # of bytes actually read
    if (cnt_read > 0) {
      // Make sure we don't read more than the user buffer can hold.
      // This error should never occur; it would only happen if the device
      // sends more bytes than requested.
      if ((*pcnt_read + cnt_read) > cnt_data_max) {
        fprintf (stderr,"Vxi11::read error: Read more bytes than expected.\n");
        return (1);
        }

      // Copy to user buffer
      memcpy (ac_data + *pcnt_read, p_readResp->data.data_val, cnt_read);
      *pcnt_read += cnt_read;;          // Update total # of bytes read

      if (*pcnt_read < cnt_data_max)    // Null terminate string if there is
        ac_data[*pcnt_read] = 0;        // enough room in the buffer
      }
    
    // Possible errors
    //  0 = no error
    //  4 = invalid link identifier
    // 11 = device locked by another link
    // 15 = I/O timeout
    // 17 = I/O error
    // 23 = abort
    int err_code = int (p_readResp->error);
    if (err_code) {
      int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                         err_code : 0;
      fprintf (stderr, "Vxi11::read error: %d %s.\n",
               err_code, _as_err_desc[idx_err_desc]);
      return (1);
      }

    // If "END" indicator or termination characer is read, then done reading
    // from device
    if (p_readResp->reason & 6)         // bit 2 = END (EOI), bit 1 = term char
      break;

    // If user buffer is full, return with error
    else if (*pcnt_read == cnt_data_max) {
      fprintf (stderr, "Vxi11::read error: read buffer full with %d bytes "
               "before reaching END indicator.\n", cnt_data_max);
      return (1);
      }
    } while (1);

  return (0);
}

// ***************************************************************************
// Vxi11::query - Send query for a double from the device
//                Convenience function combines write() and read()
//
// Parameters:
// 1. s_query     - Query string to send to device
// 2. pd_val      - Stores double read from device here
//
// Returns: 0 = no error
//          1 = error
// ***************************************************************************
  int Vxi11::
query (const char *s_query, double *pd_val)
{
  int CNT_READ_MAX = 256;               // Assume 256 characters is enough
  char s_read[CNT_READ_MAX+1];          // to read back a value

  // Query to get string
  int err = query (s_query, s_read, CNT_READ_MAX);
  if (err) {                            // Return early if query failed
    *pd_val = 0.0;
    return (1);
    }

  // Convert string to double
  int cnt = sscanf (s_read, "%le", pd_val);
  if (cnt != 1) {
    *pd_val = 0.0;
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::query - Send query for an integer from the device
//                Convenience function combines write() and read()
//
// Parameters:
// 1. s_query     - Query string to send to device
// 2. pi_val      - Stores integer read from device here
//
// Returns: 0 = no error
//          1 = error
// ***************************************************************************
  int Vxi11::
query (const char *s_query, int *pi_val)
{
  int CNT_READ_MAX = 256;               // Assume 256 characters is enough
  char s_read[CNT_READ_MAX+1];          // to read back a value

  // Query to get string
  int err = query (s_query, s_read, CNT_READ_MAX);
  if (err) {                            // Return early if query failed
    *pi_val = 0;
    return (1);
    }

  // Convert string to double
  int cnt = sscanf (s_read, "%d", pi_val);
  if (cnt != 1) {
    *pi_val = 0;
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::query - Send query for a null-terminated string from the device
//                Convenience function combines write() and read()
//
// Parameters:
// 1. s_query     - Query string to send to device
// 2. s_val       - Stores null-terminated string read from device here
// 3. len_val_max - Max length allocated in s_val, including null termination
//
// Returns: 0 = no error
//          1 = error
// ***************************************************************************
  int Vxi11::
query (const char *s_query, char *s_val, int len_val_max)
{
  // Send query
  int err = write (s_query, strlen (s_query));
  if (err) {
    s_val[0] = 0;
    return (1);
    }

  // Read response
  err = read (s_val, len_val_max);

  return (err);
}

// ***************************************************************************
// Vxi11::readstb - Read status byte from the device (serial poll)
//                  VXI-11 RPC is "device_readstb"
//
// Parameters: None
//
// Returns: 8-bit status byte if no error
//          -1 if error
//
// Notes: If Vxi11 object is associated with a GPIB device (a GPIB instrument
//        connected to a GPIB/LAN gateway), the READ STATUS BYTE control
//        sequence is executed for the addressed device, with GPIB commands
//        SPE (serial poll enable, ATN code 24) and SPD (serial poll disable,
//        ATN code 25) (see notes)
//        
//        If Vxi11 object is associated with a GPIB interface (the GPIB/LAN
//        gateway itself), this function will return an error.
// ***************************************************************************
  int Vxi11::
readstb (void)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::readstb error: no connection to device.\n");
    return (1);
    }

  Device_GenericParms genericParms;        // To send to device_readstb RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns
  
  // Read status byte
  Device_ReadStbResp *p_readStbResp=device_readstb_1 (&genericParms,_p_client);

  if (!p_readStbResp) {
    fprintf (stderr, "Vxi11::readstb error: no RPC response.\n");
    return (-1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_readStbResp->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::readstb error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (-1);
    }

  // Return status byte
  int stb = p_readStbResp->stb;
  return (stb);
}

// ***************************************************************************
// Vxi11::trigger - Send group execute trigger
//                  VXI-11 RPC is "device_trigger"
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
//
// Notes: If Vxi11 object is associated with a GPIB device (a GPIB instrument
//        connected to a GPIB/LAN gateway), only that device will receive the
//        GPIB command GET (group execute trigger, ATN code 8).
//
//        If Vxi11 object is associated with a GPIB interface (the GPIB/LAN
//        gateway itself), then all addressed devices will receive the
//        GPIB command GET (group execute trigger, ATN code 8).
//        Use the doccmd_send_command() function to send the raw GPIB commands
//        to address the set of devices prior to calling trigger().
// ***************************************************************************
  int Vxi11::
trigger (void)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::trigger error: no connection to device.\n");
    return (1);
    }

  Device_GenericParms genericParms;        // To send to device_trigger RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Send trigger command
  Device_Error *p_error = device_trigger_1 (&genericParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11::trigger error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_error->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::trigger error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::clear - Clear (reset) a device
//                VXI-11 RPC is "device_clear"
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
//
// Notes: If Vxi11 object is associted with a GPIB device (a GPIB instrument
//        connected to a GPIB/LAN gateway) the GPIB command is SDC (selective
//        device clear, ATN code 4), and resets only that device
//
//        If Vxi11 object is associated with a GPIB interface (a GPIB/LAN
//        gateway itself), GPIB command is DCL (device clear, ATN code 20),
//        and resets all devices
// ***************************************************************************
  int Vxi11::
clear (void)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::clear error: no connection to device.\n");
    return (1);
    }

  Device_GenericParms genericParms;        // To send to device_clear RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Send clear command
  Device_Error *p_error = device_clear_1 (&genericParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11::clear error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_error->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::clear error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::remote - Place device in remote state, with local lockout
//                 VXI-11 RPC is "device_remote"
//                 
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
//
// Notes: If Vxi11 object is associated with a GPIB device (a GPIB instrument
//        connected to a GPIB/LAN gateway), the SET RWLS control sequence is
//        excuted for the addressed device with GPIB command LLO (local
//        lockout, ATN code 17)
//
//        If Vxi11 object is associated with a GPIB interface (the GPIB/LAN
//        gateway itself), this function will return an error.
// ***************************************************************************
  int Vxi11::
remote (void)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::remote error: no connection to device.\n");
    return (1);
    }

  Device_GenericParms genericParms;        // To send to device_remote RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Send remote command
  Device_Error *p_error = device_remote_1 (&genericParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11::remote error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_error->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::remote error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::local - Place device in local state
//                VXI-11 RPC is "device_local"
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
//
// Notes: If Vxi11 object is associated with a GPIB device (a GPIB instrument
//        connected to a GPIB/LAN gateway), the SET RWLS control sequence is
//        excuted for the addressed device, with GPIB command GTL (go to local,
//        ATN code 1).
//
//        If Vxi11 object is associated with a GPIB interface (the GPIB/LAN
//        gateway itself), this functon may return an error if the operation
//        is not supported, or it may deactivate the REN line, depending on
//        the GPIB/LAN interface capabillity.
// ***************************************************************************
  int Vxi11::
local (void)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::local error: no connection to device.\n");
    return (1);
    }

  Device_GenericParms genericParms;        // To send to device_local RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Send local command
  Device_Error *p_error = device_local_1 (&genericParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11::local error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_error->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::local error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::lock - Request lock on device for exclusive access
//               VXI-11 RPC is "device_lock"
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
// ***************************************************************************
  int Vxi11::
lock (void)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::lock error: no connection to device.\n");
    return (1);
    }

  Device_LockParms lockParms;           // To send to device_lock RPC
  lockParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  lockParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  lockParms.flags = 1;                  // Wait for lock

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Send lock command
  Device_Error *p_error = device_lock_1 (&lockParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11::lock error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  // 11 = device locked by another link
  // 23 = abort
  int err_code = int (p_error->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::lock error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::unlock - Release lock acquired by previous call to Vxi11::lock
//                 VXI-11 RPC is "device_unlock"
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
// ***************************************************************************
  int Vxi11::
unlock (void)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::unlock error: no connection to device.\n");
    return (1);
    }

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Send unlock command
  Device_Error *p_error = device_unlock_1 (&(_p_link->lid), _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11::unlock error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  // 12 = no lock held by this link
  int err_code = int (p_error->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::unlock error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::abort - Abort an in-progress VXI-11 RPC
//                VXI-11 RPC is "device_abort"
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
//
// Notes: This is normally called from a separate thread from which the RPC
//        that is being aborted is called.  The purpose of the abort is to
//        end an existing RPC before its timeout expires.
// ***************************************************************************
  int Vxi11::
abort (void)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::abort error: no connection to device.\n");
    return (1);
    }

  // Create abort channel if it was not already created
  if (!_p_client_abort) {
    sockaddr_in sockaddr = {0};
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons (_p_link->abortPort);
    sockaddr.sin_addr.s_addr = _ui_device_ip_addr;
    int sock = RPC_ANYSOCK;
  
    __p_client_abort = clnttcp_create (&sockaddr, DEVICE_ASYNC,
                                       DEVICE_ASYNC_VERSION, &sock, 0, 0);
  
    if (!_p_client_abort) {               // Exit early if error
      const char *s_err = "Vxi11 abort error: abort channel client creation";
      clnt_pcreateerror ((char *)s_err);  // Print error message
      return (1);
      }
    }

  // Send abort command
  Device_Error *p_error = device_abort_1 (&(_p_link->lid), _p_client_abort);

  if (!p_error) {
    fprintf (stderr, "Vxi11::abort error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  int err_code = int (p_error->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::abort error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::srq_callback - Specify the callback function for the SRQ (service
//                       request) interrupt.
//
// Parameters:
// 1. pfn_srq_callback - Function to call when an SRQ interrupt occurs.
//                       Set to NULL to disable callbacks.
//                       Function is of the type
//                         void srq_callback (Vxi11 *)
// Returns: 0 = no error
//          1 = error
//
// Notes: The function pfn_srq_callback will be called on a separate thread
//        than the thread calling this function.  It will be called only if
//        SRQ is enabled via enable_srq() and if the device issues a SRQ.
//
//        Call this function before calling enable_srq().
//
//        This is a static member function, so only one callback function
//        can be used for all instances of the Vxi11 class.
//
//        The Vxi11* parameter to the callback function can be used to talk
//        to the device that created the SRQ and to identify the source via
//        the device_addr() member.
// ***************************************************************************
  int Vxi11::
srq_callback (void (*pfn_srq_callback)(Vxi11 *))
{
  // Early return callback function is the same as before
  if (pfn_srq_callback == _pfn_srq_callback)
    return (0);
  
  // *************************************************************************
  // Remove RPC service and callback for SRQ if callback previously set
  // *************************************************************************
  if (_pfn_srq_callback) {
    int err = 0;
    
    // Stop thread running svc_run()
    if (_p_pthread_svc_run) {
      if (pthread_cancel (*(pthread_t *)_p_pthread_svc_run)) {
        fprintf (stderr,"Vxi11::srq_callback error: could not kill thread.\n");
        err = 1;
        }
      _p_pthread_svc_run = 0;
      }

    // Unregister the callback function
    svc_unregister (DEVICE_INTR, DEVICE_INTR_VERSION);
    
    // Destroy RPC service transport
    svc_destroy ((SVCXPRT *)_p_svcXprt_srq_tcp);
    svc_destroy ((SVCXPRT *)_p_svcXprt_srq_udp);

    // Mark callback as null so it won't be destroyed again on repeat call
    _pfn_srq_callback = NULL;

    if (err)
      return (1);
    }

  // Early return if no callback is specified
  if (pfn_srq_callback == NULL)
    return (0);

  // *************************************************************************
  // Create RPC service and callback for SRQ if callback is not null
  // *************************************************************************

  // Save the user's callback function
  _pfn_srq_callback = pfn_srq_callback;

  // Clear the port mapping for the SRQ interrupt channel
  pmap_unset (DEVICE_INTR, DEVICE_INTR_VERSION);

  // Create RPC service transport for the SRQ interrupt channel
  _p_svcXprt_srq_tcp = (void *)svctcp_create (RPC_ANYSOCK, 0, 0);
  if (!_p_svcXprt_srq_tcp) {
    fprintf (stderr, "Vxi11::srq_callback error: could not create RPC service "
             "transport for TCP.\n");
    _pfn_srq_callback = NULL;
    return (1);
    }

  _p_svcXprt_srq_udp = (void *)svcudp_create (RPC_ANYSOCK);
  if (!_p_svcXprt_srq_udp) {
    fprintf (stderr, "Vxi11::srq_callback error: could not create RPC service "
             "transport for UDP.\n");
    svc_destroy ((SVCXPRT *)_p_svcXprt_srq_tcp);
    _pfn_srq_callback = NULL;
    return (1);
    }
  
  // Register the SRQ interrupt callback function with this service
  if (!svc_register ((SVCXPRT*)_p_svcXprt_srq_tcp,
                     DEVICE_INTR, DEVICE_INTR_VERSION,
                     (void(*)(void))&_fn_srq_callback, IPPROTO_TCP)) {
    fprintf (stderr, "Vxi11::srq_callback error: could not register SRQ "
             "callback function for TCP.\n");
    svc_destroy ((SVCXPRT *)_p_svcXprt_srq_tcp);
    svc_destroy ((SVCXPRT *)_p_svcXprt_srq_udp);
    _pfn_srq_callback = NULL;    
    return (1);    
    }

  if (!svc_register ((SVCXPRT*)_p_svcXprt_srq_udp,
                     DEVICE_INTR, DEVICE_INTR_VERSION,
                     (void(*)(void))&_fn_srq_callback, IPPROTO_UDP)) {
    fprintf (stderr, "Vxi11::srq_callback error: could not register SRQ "
             "callback function for UDP.\n");
    svc_destroy ((SVCXPRT *)_p_svcXprt_srq_tcp);
    svc_destroy ((SVCXPRT *)_p_svcXprt_srq_udp);
    _pfn_srq_callback = NULL;    
    return (1);    
    }

  // Create a thread so that it can monitor SRQ interrupts via svc_run()
  static pthread_t pthread_svc_run;
  _p_pthread_svc_run =(void *)&pthread_svc_run;
  if (pthread_create ((pthread_t *)_p_pthread_svc_run, NULL, &_fn_svc_run,
                      NULL)) {
    fprintf (stderr, "Vxi11::srq_callback error: could not start SRQ service "
             "thread.\n");
    svc_unregister (DEVICE_INTR, DEVICE_INTR_VERSION);
    svc_destroy ((SVCXPRT *)_p_svcXprt_srq_tcp);
    svc_destroy ((SVCXPRT *)_p_svcXprt_srq_udp);
    _pfn_srq_callback = NULL;    
    return (1);
    }
  
  return (0);
}

// ***************************************************************************
// Vxi11::_fn_svc_run - Private static function to run the RPC service for
//                      the SRQ interrupt processing on its own thread
//
// Parameters:
// 1. p_arg - Not used
//
// Returns: None
//
// Notes: This function is called by srq_callback() and executes in a
//        separate thread.  This function never returns.
// ***************************************************************************
  void *Vxi11::
_fn_svc_run (void *p_arg)
{
  // Start the RPC server to listen for SRQ interrupts
  svc_run();                            // This function will never return

  fprintf (stderr,
           "Vxi11::_fn_svc_run error: svc_run() returned unexpectedly.\n");

  return (0);                           // This should never be called
}

// ***************************************************************************
// Vxi11::_fn_srq_callback - Private static callback function for the SRQ
//                           interrupt, which then calls the user speicified
//                           SRQ callback function.
//                           VXI-11 RPC is "device_intr_srq"
//
// Parameters:
// 1. rqstp  - Request type, used to get RPC function requested
//             Actual type is svc_req*, but declared void* to reduce include
//             dependencies in libvxi11.h.
// 2. transp - RPC service transport, used to get arguments to the RPC
//             Actual type is SVCXPRT*, but declared void* to reduce include
//             dependencies in libvxi11.h.
//
// Returns: None
//
// Notes; This function assumes it is only called due to receiving the
//        device_intr_srq RPC from the VXI-11 device.
//
//        This function is registered as an RPC service callback in
//        srq_callback(), and is called by the svc_run() call in fn_svc_run()
//        when the device_intr_srq RPC is received from the device.
//
//        See the notes for Vxi11::enable_srq() for an example of the
//        sequence needed to use SRQ interrupts.
// ***************************************************************************
  void Vxi11::
_fn_srq_callback (void /*svc_req*/ *rqstp, void /*SVCXPRT*/ *transp)
{
  // This is a simplified version of the code that is generated by rpcgen in
  // vxi11_rpc_svc.c

  // Check if getting the expected RPC call
  if (((svc_req*)rqstp)->rq_proc != device_intr_srq) {
    fprintf (stderr, "Vxi11::_fn_srq_callback error: unexpected RPC call.\n");
    svcerr_noproc ((SVCXPRT *)transp);
    return;
    }
  
  union {
    Device_SrqParms device_intr_srq_1_arg;
    } argument;

  // Translate type sizes between client (VXI-11 deivce) and server (this Mac)
  xdrproc_t xdr_argument = (xdrproc_t) xdr_Device_SrqParms;
  (void) memset((char *)&argument, 0, sizeof (argument));
  if (!svc_getargs ((SVCXPRT *)transp, xdr_argument, (caddr_t) &argument)) {
    fprintf (stderr,
             "Vxi11::_fn_srq_callback error: could not decode arguments.\n");
    svcerr_decode ((SVCXPRT *)transp);
    return;
    }

  // Extract the pointer to the Vxi11 object associated with the device that
  // generated the SRQ interrupt.
  // Vxi11 object pointer was stored into the Device_SrqParms object by
  // enable_srq().

  // Check that the pointer is the correct length
  int len_ptr = argument.device_intr_srq_1_arg.handle.handle_len;
  if (len_ptr == sizeof (Vxi11 *)) {
    Vxi11 *p_vxi11;
    memcpy (&p_vxi11, argument.device_intr_srq_1_arg.handle.handle_val,
            sizeof (Vxi11 *));

    // Call the user specified SRQ callback function with the Vxi11 object as
    // the parameter
    _pfn_srq_callback (p_vxi11);
    }
  else {
    fprintf (stderr, "Vxi11::_fn_srq_callback error:  pointer in SRQ callback "
             "has incorrect length %d, expected %lu.\n",
             len_ptr, sizeof (Vxi11 *));
    }

  svc_freeargs ((SVCXPRT *)transp, xdr_argument, (caddr_t) &argument);
}

// ***************************************************************************
// Vxi11::enable_srq - Enable/disable SRQ (service request) interrupt
//                     VXI-11 RPCs are "device_enable_srq"
//                                     "create_intr_chan"
//                                     "destroy_intr_chan"
//
// Parameters:
// 1. b_ena - false = disable SRQ interrupts
//            true  = enable SRQ interrupts
// 2. b_udp - false = Use TCP protocol (default)
//            true  = Use UDP protocol
//            This parameter is only used when b_ena = true.
//            All devices should support TCP, but not all will support UDP.
//            Some devices work better with UDP (faster).
//
// Returns: 0 = no error
//          1 = error
//
// Notes: To use SRQ interrupts, follow this process:
//        1. Specify the SRQ callback function wtih Vxi11::srq_callback()
//        2. Enable SRQ at the VXI-11 level with this function,
//           Vxi11::enable_srq()
//        3. Configure your device to create SRQ under the required conditions.
//           For example, sending the following commands (via Vxi11::write()
//           or Vxi11::printf()) work on many modern devices to issue
//           SRQ on "operation complete".
//              *CLS           // Clear all event status
//              *ESE 1         // Enable operation complete event
//              *SRE 32        // Enable SRQ on event
//              *OPC           // Issue operation complete, this will cause SRQ
//        4. In the callback function, have it clear the SRQ by reading the
//           status byte via Vxi11::readstb(), and clear the condition that
//           caused the SRQ by sending "*CLS".  In the callback function, use
//           the Vxi11 pointer passed to the function to access the device.
// ***************************************************************************
  int Vxi11::
enable_srq (bool b_ena, bool b_udp)
{
  // Early return if enable state and protocol are the same as before
  if ((b_ena && _b_srq_ena && (b_udp == _b_srq_udp)) ||// Re-Enable, same prot
      (!b_ena && !_b_srq_ena))                         // Re-disable
    return (0);

  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr, "Vxi11::enable_srq error: no connection to device.\n");
    return (1);
    }

  if ((_p_svcXprt_srq_tcp == NULL) || (_p_svcXprt_srq_udp == NULL)) {
    fprintf (stderr,
             "Vxi11::enable_srq error: must call srq_callback() first.\n");
    return (1);
    }
  
  int err = 0;                          // No error yet
  
  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // *************************************************************************
  // Disable SRQ interrupt
  // *************************************************************************

  // Disable first if user changing protocol or if user disabling SRQ
  if ((_b_srq_ena && (b_udp != _b_srq_udp)) || !b_ena) {
    _b_srq_ena = false;                 // SRQ interrupt is disabled
    
    // Configuration to diaable SRQ interupts
    Device_EnableSrqParms enableSrqParms;
    enableSrqParms.lid = _p_link->lid;  // Link ID from create_link RPC call
    enableSrqParms.enable = false;      // Disable interrupts

    // Set handle member to allow identification of the SRQ source
    Vxi11 *p_vxi11 = this;
    memcpy (_a_srq_handle, &p_vxi11, sizeof (Vxi11 *));
    enableSrqParms.handle.handle_len = sizeof (Vxi11 *);
    enableSrqParms.handle.handle_val = _a_srq_handle;
    
    // Send SRQ disable command
    Device_Error *p_error = device_enable_srq_1 (&enableSrqParms, _p_client);
    
    if (!p_error) {
      fprintf (stderr, "Vxi11::enable_srq error: no RPC response.\n");
      err = 1;
      }
    else {
      // Possible errors
      //  0 = no error
      //  4 = invalid link identifier
      int err_code = int (p_error->error);
      if (err_code) {
        int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
          err_code : 0;
        fprintf (stderr, "Vxi11::enable_srq error: %d %s.\n",
                 err_code, _as_err_desc[idx_err_desc]);
        err = 1;
        }
      }

    // Destroy the SRQ interrupt channel
    p_error = destroy_intr_chan_1 (0, _p_client);
    
    if (!p_error) {
      fprintf (stderr,
               "Vxi11::enable_srq error: could not destroy intr channel.\n");
      err = 1;
      }
    else {
      // Possible errors
      //  0 = no error
      //  6 = channel not established
      int err_code = int (p_error->error);
      if (err_code) {
        int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
          err_code : 0;
        fprintf (stderr, "Vxi11::enable_srq error:  destroy_intr_chan error "
                 "%d %s.\n", err_code, _as_err_desc[idx_err_desc]);
        err = 1;
        }
      }
    }
  
  // *************************************************************************
  // Enable SRQ interrupt
  // *************************************************************************
  if (b_ena) {
    _b_srq_ena = false;                 // Set false in case of early return
    _b_srq_udp = b_udp;                 // Save protocol used
    
    // Get hostname of this computer
    char s_hostname[256];
    if (gethostname (s_hostname, sizeof (s_hostname))) {
      fprintf (stderr, "Vxi11::enable_srq error: could not get hostname.\n");
      _pfn_srq_callback = NULL;    
      return (1);
      }
    s_hostname[255] = 0;                // Make sure it is null terminated
    
    // Get the IP address for this hostname
    hostent *p_hostent = gethostbyname (s_hostname);
    if (!p_hostent) {
      fprintf (stderr,
               "Vxi11::enable_srq error: could not get host IP address.\n");
      return (1);
      }
    
    // There might be multiple addresses, find the first one that is not
    // the loopback address
    int idx_ip_addr=0;
    unsigned int ip_addr = 0;
    while (p_hostent->h_addr_list[idx_ip_addr]) {
      ip_addr = ntohl (*(unsigned int *)(p_hostent->h_addr_list[idx_ip_addr]));
      if (ip_addr != 0x7f000001)        // Skip loopback address 127.0.0.1
        break;
      idx_ip_addr++;
      }

    // Check if IP address is valid (not 0.0.0.0 or 127.0.0.1)
    if ((ip_addr == 0) || (ip_addr == 0x7f000001)) {
      fprintf (stderr,
               "Vxi11::enable_srq error: could not determine IP address.\n");
      return (1);
      }
  
    // Configuration to create SRQ interrupt channel
    Device_RemoteFunc remoteFunc;
    remoteFunc.hostAddr = ip_addr;             // IP address of this host
                                               // Port # for interrupt channel
    remoteFunc.hostPort = (b_udp) ? ((SVCXPRT*)_p_svcXprt_srq_udp)->xp_port :
                                    ((SVCXPRT*)_p_svcXprt_srq_tcp)->xp_port;
    remoteFunc.progNum = DEVICE_INTR;          // Must be this value
    remoteFunc.progVers = DEVICE_INTR_VERSION; // Must be this value
    remoteFunc.progFamily = (b_udp) ? DEVICE_UDP :DEVICE_TCP; // Protocol
  
    // Create SRQ interrupt channel
    Device_Error *p_error = create_intr_chan_1 (&remoteFunc, _p_client);

    if (!p_error) {
      fprintf (stderr, "Vxi11::enable_srq error:  create_intr_chan no RPC "
               "response.\n");
      return (1);
      }
  
    // Possible errors
    //  0 = no error
    //  6 = channel not established
    //  8 = operation not supported
    // 29 = channel already established
    int err_code = int (p_error->error);
    if (err_code) {
      int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
        err_code : 0;
      fprintf (stderr, "Vxi11::enable_srq_error:  create_intr_chan error "
               "%d %s.\n", err_code, _as_err_desc[idx_err_desc]);
      return (1);
      }

    // Configuration to enable SRQ interupts
    Device_EnableSrqParms enableSrqParms;
    enableSrqParms.lid = _p_link->lid;  // Link ID from create_link RPC call
    enableSrqParms.enable = true;       // Enable interrupts

    // Set handle member to allow identification of the SRQ source
    Vxi11 *p_vxi11 = this;
    memcpy (_a_srq_handle, &p_vxi11, sizeof (Vxi11 *));
    enableSrqParms.handle.handle_len = sizeof (Vxi11 *);
    enableSrqParms.handle.handle_val = _a_srq_handle;
    
    // Send SRQ enable command
    p_error = device_enable_srq_1 (&enableSrqParms, _p_client);
    
    if (!p_error) {
      fprintf (stderr, "Vxi11::enable_srq error: no RPC response\n");
      destroy_intr_chan_1 (0, _p_client);
      return (1);
      }
  
    // Possible errors
    //  0 = no error
    //  4 = invalid link identifier
    err_code = int (p_error->error);
    if (err_code) {
      int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
        err_code : 0;
      fprintf (stderr, "Vxi11::enable_srq error: %d %s\n",
               err_code, _as_err_desc[idx_err_desc]);
      destroy_intr_chan_1 (0, _p_client);
      return (1);
      }

    _b_srq_ena = true;                  // No errors, so mark as enabled
    }

  return (err);
}

// ***************************************************************************
// The following docmd_* functions implement low level GPIB communication
// with a GPIB interface (GPIB/LAN gateway itself) defined in the following
// document:
//
//   VMEbus Extensions for Instrumentation
//   TCP/IP-IEEE 488.1 Interface Specification VXI-11.2
//   Draft 0.3  July 17, 1995
//   Section B.5. Interface Communication
//
// ***************************************************************************

// ***************************************************************************
// Vxi11::docmd_send_command - Send raw low-level GPIB commands
//                             VXI-11 RPC is "device_docmd" with cmd 0x20000
//
// Parameters:
// 1. s_data - GPIB control command data to send, null terminated
//             This will be sent with the ATN line enabled
//
// Returns: 0 = no error
//          1 = error
//
// Notes: This command should be used with a Vxi11 object assoicated with the
//        GPIB interface itself (such as a GPIB/LAN gateway), not a GPIB
//        instrument.
//
//        For example, the string "?U#$" will first unlisten all, then talk
//        to GPIB address 21 (the device address of the GPIB/LAN gateway
//        interface), then enable listening on addresses 3 and 4.
// ***************************************************************************
  int Vxi11::
docmd_send_command (const char *s_data)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr,
             "Vxi11::docmd_send_command error: no connection to device.\n");
    return (1);
    }

  Device_DocmdParms docmdParms;         // To send to device_docmd RPC
  docmdParms.lid = _p_link->lid;        // Link ID from create_link RPC call
  docmdParms.flags = 0;                 // No flags
  docmdParms.io_timeout = _timeout_ms;  // Timeout for I/O in ms
  docmdParms.lock_timeout= _timeout_ms; // Timeout for lock in ms
  docmdParms.cmd = 0x20000;             // Command code
  docmdParms.network_order = 0;         // Little-endian
  docmdParms.datasize = 1;              // 1 byte size
  docmdParms.data_in.data_in_len = strlen (s_data); // Command data size
  docmdParms.data_in.data_in_val = (char *)s_data;  // Command data

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Send raw low-level GPIB command
  Device_DocmdResp *p_docmdResp = device_docmd_1 (&docmdParms, _p_client);
  
  if (p_docmdResp == 0) {
    fprintf (stderr, "Vxi11::docmd_send_command error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  5 = parameter error
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_docmdResp->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::docmd_send_command error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::docmd_bus_status - Get GPIB bus status
//                           VXI-11 RPC is "device_docmd" with command 0x20001
//
// Parameters:
// 1. type - Selects the GPIB status to query
//           1 = REN line state, returns 1 if REN is true, 0 otherwise
//
//           2 = SRQ line state, returns 1 if SRQ is true, 0 otherwise
//
//           3 = NDAC line state, returns 1 if NDAC is true, 0 otherwise
//
//           4 = System controller state, returns 1 if active, 0 otherwise
//               In a multi-controller setup, only one controller can be the
//               system controller.  This will return 1 for a single-controller
//               setup.
//
//           5 = Controller-in-charge state, returns 1 if active, 0 otherwise
//               In a multi-controller setup, this indicates which controller
//               is the controller in charge.  This will return 1 for a
//               single-controller setup.
//
//           6 = Talker state, returns 1 if interface addressed to talk,
//               0 otherwise
//
//           7 = Listener state, returns 1 if interface addressed to listen,
//               0 otherwise
//
//           8 = Bus address, returns interface address (0 to 30)
//               For example, the default value for the Agilent E5810A is 21.
//
// Returns: -1 = error
//          otherwise, see above based on type
//
// Notes: This command should be used with a Vxi11 object assoicated with the
//        GPIB interface itself (such as a GPIB/LAN gateway), not a GPIB
//        instrument.
// ***************************************************************************
  int Vxi11::
docmd_bus_status (int type)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr,
             "Vxi11::docmd_bus_status error: no connection to device.\n");
    return (-1);
    }

  Device_DocmdParms docmdParms;         // To send to device_docmd RPC
  docmdParms.lid = _p_link->lid;        // Link ID from create_link RPC call
  docmdParms.flags = 0;                 // No flags
  docmdParms.io_timeout = _timeout_ms;  // Timeout for I/O in ms
  docmdParms.lock_timeout= _timeout_ms; // Timeout for lock in ms
  docmdParms.cmd = 0x20001;             // Command code
  docmdParms.network_order = 0;         // Little-endian
  docmdParms.datasize = 2;              // 2 byte size
  docmdParms.data_in.data_in_len = 2;   // Status type size
  docmdParms.data_in.data_in_val = (char *)(&type);  // Status type

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Send request for bus status
  Device_DocmdResp *p_docmdResp = device_docmd_1 (&docmdParms, _p_client);
  
  if (p_docmdResp == 0) {
    fprintf (stderr, "Vxi11::docmd_bus_status error: no RPC response.\n");
    return (-1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  5 = parameter error
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_docmdResp->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::docmd_bus_status error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (-1);
    }

  // Get status value to return
  int status = *(short *)p_docmdResp->data_out.data_out_val;
  
  return (status);
}

// ***************************************************************************
// Vxi11::docmd_atn_control - Set GPIB ATN (attention, command mode) line state
//                            VXI-11 RPC is "device_docmd" with cmd 0x20002
//
// Parameters:
// 1. b_state - Sets GPIB ATN line state
//              true  = set ATN active (line driven low)
//              false = set ATN inactive (line driven high)
//
// Returns: 0 = no error
//          1 = error
//
// Notes: This command should be used with a Vxi11 object assoicated with the
//        GPIB interface itself (such as a GPIB/LAN gateway), not a GPIB
//        instrument.
// ***************************************************************************
  int Vxi11::
docmd_atn_control (bool b_state)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr,
             "Vxi11::docmd_atn_control error: no connection to device.\n");
    return (1);
    }

  // Convert boolean state to 2-byte value needed by the RPC call
  unsigned short us_state = (b_state) ? 1 : 0;
  
  Device_DocmdParms docmdParms;         // To send to device_docmd RPC
  docmdParms.lid = _p_link->lid;        // Link ID from create_link RPC call
  docmdParms.flags = 0;                 // No flags
  docmdParms.io_timeout = _timeout_ms;  // Timeout for I/O in ms
  docmdParms.lock_timeout= _timeout_ms; // Timeout for lock in ms
  docmdParms.cmd = 0x20002;             // Command code
  docmdParms.network_order = 0;         // Little-endian
  docmdParms.datasize = 2;              // 2 byte size
  docmdParms.data_in.data_in_len = 2;   // Command data size
  docmdParms.data_in.data_in_val = (char *)&us_state;  // Command data

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Set ATN line state
  Device_DocmdResp *p_docmdResp = device_docmd_1 (&docmdParms, _p_client);
  
  if (p_docmdResp == 0) {
    fprintf (stderr, "Vxi11::docmd_atn_control error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  5 = parameter error
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_docmdResp->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::docmd_atn_control error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::docmd_ren_control - Set GPIB REN (remote enable) line state
//                            VXI-11 RPC is "device_docmd" with cmd 0x20003
//
// Parameters:
// 1. b_state - Sets GPIB REN line state
//              true  = set REN active (line driven low)
//              false = set REN inactive (line driven high)
//
// Returns: 0 = no error
//          1 = error
//
// Notes: This command should be used with a Vxi11 object assoicated with the
//        GPIB interface itself (such as a GPIB/LAN gateway), not a GPIB
//        instrument.
//
//        In a multi-controller setup, only the system controller can set
//        this state.
// ***************************************************************************
  int Vxi11::
docmd_ren_control (bool b_state)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr,
             "Vxi11::docmd_ren_control error: no connection to device.\n");
    return (1);
    }

  // Convert boolean state to 2-byte value needed by the RPC call
  unsigned short us_state = (b_state) ? 1 : 0;
  
  Device_DocmdParms docmdParms;         // To send to device_docmd RPC
  docmdParms.lid = _p_link->lid;        // Link ID from create_link RPC call
  docmdParms.flags = 0;                 // No flags
  docmdParms.io_timeout = _timeout_ms;  // Timeout for I/O in ms
  docmdParms.lock_timeout= _timeout_ms; // Timeout for lock in ms
  docmdParms.cmd = 0x20003;             // Command code
  docmdParms.network_order = 0;         // Little-endian
  docmdParms.datasize = 2;              // 2 byte size
  docmdParms.data_in.data_in_len = 2;   // Command data size
  docmdParms.data_in.data_in_val = (char *)&us_state;  // Command data

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Set REN line state
  Device_DocmdResp *p_docmdResp = device_docmd_1 (&docmdParms, _p_client);
  
  if (p_docmdResp == 0) {
    fprintf (stderr, "Vxi11::docmd_ren_control error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  5 = parameter error
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_docmdResp->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::docmd_ren_control error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::docmd_pass_control - Set another GPIB controller as the controller-
//                             in-charge
//                             VXI-11 RPC is "device_docmd" with cmd 0x20004
//
// Parameters:
// 1. addr - GPIB address of the controller to pass control to
//
// Returns: 0 = no error
//          1 = error
//
// Notes: This command should be used with a Vxi11 object assoicated with the
//        GPIB interface itself (such as a GPIB/LAN gateway), not a GPIB
//        instrument.
//
//        GPIB command is TCT (take control, ATN code 9).
//
//        This function is only applicable in a multi-controller setup.
//        This does not change which controller is the system controller.
// ***************************************************************************
  int Vxi11::
docmd_pass_control (int addr)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr,
             "Vxi11::docmd_pass_control error: no connection to device.\n");
    return (1);
    }

  Device_DocmdParms docmdParms;         // To send to device_docmd RPC
  docmdParms.lid = _p_link->lid;        // Link ID from create_link RPC call
  docmdParms.flags = 0;                 // No flags
  docmdParms.io_timeout = _timeout_ms;  // Timeout for I/O in ms
  docmdParms.lock_timeout= _timeout_ms; // Timeout for lock in ms
  docmdParms.cmd = 0x20004;             // Command code
  docmdParms.network_order = 0;         // Little-endian
  docmdParms.datasize = 4;              // 4 byte size
  docmdParms.data_in.data_in_len = 4;   // Command data size
  docmdParms.data_in.data_in_val = (char *)&addr;  // Command data

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Pass control to other GPIB controller
  Device_DocmdResp *p_docmdResp = device_docmd_1 (&docmdParms, _p_client);
  
  if (p_docmdResp == 0) {
    fprintf (stderr, "Vxi11::docmd_pass_control error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  5 = parameter error
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_docmdResp->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::docmd_pass_control error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::docmd_bus_address - Set the GPIB address of the GPIB/LAN gateway
//                            itself
//                            VXI-11 RPC is "device_docmd" with cmd 0x2000a
//
// Parameters:
// 1. addr - GPIB address to set this of the controller to pass control to
//           (0 to 30)
//
// Returns: 0 = no error
//          1 = error
//
// Notes: This command should be used with a Vxi11 object assoicated with the
//        GPIB interface itself (such as a GPIB/LAN gateway), not a GPIB
//        instrument.
// ***************************************************************************
  int Vxi11::
docmd_bus_address (int addr)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr,
             "Vxi11::docmd_bus_address error: no connection to device.\n");
    return (1);
    }

  Device_DocmdParms docmdParms;         // To send to device_docmd RPC
  docmdParms.lid = _p_link->lid;        // Link ID from create_link RPC call
  docmdParms.flags = 0;                 // No flags
  docmdParms.io_timeout = _timeout_ms;  // Timeout for I/O in ms
  docmdParms.lock_timeout= _timeout_ms; // Timeout for lock in ms
  docmdParms.cmd = 0x2000a;             // Command code
  docmdParms.network_order = 0;         // Little-endian
  docmdParms.datasize = 4;              // 4 byte size
  docmdParms.data_in.data_in_len = 4;   // Command data size
  docmdParms.data_in.data_in_val = (char *)&addr;  // Command data

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Set GPIB address of GPIB/LAN gateway
  Device_DocmdResp *p_docmdResp = device_docmd_1 (&docmdParms, _p_client);
  
  if (p_docmdResp == 0) {
    fprintf (stderr, "Vxi11::docmd_bus_address error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  5 = parameter error
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_docmdResp->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::docmd_bus_address error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::docmd_ifc_control - Toggle GPIB IFC (interface clear) line state
//                            VXI-11 RPC is "device_docmd" with cmd 0x20010
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
//
// Notes: This command should be used with a Vxi11 object assoicated with the
//        GPIB interface itself (such as a GPIB/LAN gateway), not a GPIB
//        instrument.
//
//        This will cause all GPIB devices to go to their inactive state.
//        In a multi-controller setup, only the system controller can do this
//        operation.
// ***************************************************************************
  int Vxi11::
docmd_ifc_control (void)
{
  // Early return if object did not make connection to instrument
  if (!_b_valid) {
    fprintf (stderr,
             "Vxi11::docmd_ifc_control error: no connection to device.\n");
    return (1);
    }

  Device_DocmdParms docmdParms;         // To send to device_docmd RPC
  docmdParms.lid = _p_link->lid;        // Link ID from create_link RPC call
  docmdParms.flags = 0;                 // No flags
  docmdParms.io_timeout = _timeout_ms;  // Timeout for I/O in ms
  docmdParms.lock_timeout= _timeout_ms; // Timeout for lock in ms
  docmdParms.cmd = 0x20010;             // Command code
  docmdParms.network_order = 0;         // Little-endian
  docmdParms.datasize = 0;              // 0 byte size (not used)
  docmdParms.data_in.data_in_len = 0;   // Command data size (not used)
  docmdParms.data_in.data_in_val = 0;;  // Command data (not used)

  Vxi11Mutex vxi11Mutex;                // Lock access until function returns

  // Toggle IFC line state
  Device_DocmdResp *p_docmdResp = device_docmd_1 (&docmdParms, _p_client);
  
  if (p_docmdResp == 0) {
    fprintf (stderr, "Vxi11::docmd_ifc_control error: no RPC response.\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  //  5 = parameter error
  //  8 = operation not supported
  // 11 = device locked by another link
  // 15 = I/O timeout
  // 17 = I/O error
  // 23 = abort
  int err_code = int (p_docmdResp->error);
  if (err_code) {
    int idx_err_desc = ((err_code >= 0) && (err_code < CNT_ERR_DESC_MAX)) ?
                       err_code : 0;
    fprintf (stderr, "Vxi11::docmd_ifc_control error: %d %s.\n",
             err_code, _as_err_desc[idx_err_desc]);
    return (1);
    }

  return (0);
}
