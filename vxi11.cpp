// ***************************************************************************
// vxi11.cpp - Implentation of the Vxi11 class
//
//             For commuicating with devices via the VXI-11 protocol
//             over TCP/IP.
//
// Written by Eddie Lew, Lew Engineering
//
// Edit history:
//
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

// Macro to conveniently access _p_client and _p_link members of the class.
// They are defined as void * in the class so that the .h file does not need
// to include the RPC interface.
#define _p_client ((CLIENT *)__p_client)
#define _p_link ((Create_LinkResp *)__p_link)

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

  timeout (10.0);                       // Default timeout
}

// ***************************************************************************
// Vxi11 constructor - Open connection to device 
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

  timeout (10.0);                       // Default timeout

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
//        Use this function if the default construtor was used.
// ***************************************************************************
  int Vxi11::
open (const char *s_address, const char *s_device)
{
  // Cannot open a new connection if one is already open in this instance
  if (_b_valid) {
    fprintf (stderr, "Vxi11 open error: connection already open\n");
    return (1);
    }

  // Check if host name or IP address is not null
  if (!s_address) {
    fprintf (stderr, "Vxi11 open error: null address\n");
    return (1);
    }
  
  // Use default device name if it is not specified
  if (!s_device)
    s_device = "inst0";                 // According to VXI-11.3 Rule B.1.2

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
    const char *s_err = "Vxi11 open error: link creation";
    clnt_perror (_p_client, (char *)s_err); // Print error message
    clnt_destroy (_p_client);
    return (1);
    }

  // Copy it to the object, since RPC will lose it on next call
  __p_link = (Create_LinkResp *)malloc (sizeof (Create_LinkResp));

  if (!_p_link) {                       // Exit early if error
    fprintf (stderr, "Vxi11 open error: could not allocate memory\n");
    destroy_link_1 (&(p_link->lid), _p_client);
    clnt_destroy (_p_client);
    return (1);
    }
  
  *_p_link = *p_link;

  // Change underlying RPC timeout from 25s that was set in vxi11_rpc_clnt.c
  // to 120s to deal with slow devices
  struct timeval timeval_timeout = {120, 0};
  clnt_control (_p_client, CLSET_TIMEOUT, (char *)(&timeval_timeout));

  _b_valid = 1;                         // Now have valid connection
  return (0);
}

// ***************************************************************************
// Vxi11::close - Close connection to the device
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

  _b_valid = 0;                         // No connection to device
  int err = 0;
  
  // Close link to device
  Device_Error *p_error = destroy_link_1 (&(_p_link->lid), _p_client);  
  if (!p_error) { 
    fprintf (stderr, "Vxi11 close error: no RPC response\n");
    err = 1;
    }
  else if (p_error->error) {
    fprintf (stderr, "Vxi11 close error: destroy_link error %d\n",
             int(p_error->error));
    err = 1;
    }
  
  free (_p_link);
  __p_link = 0;
  
  // Close RPC client
  clnt_destroy (_p_client);
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
// ***************************************************************************
  int Vxi11::
write (const char *ac_data, int cnt_data)
{
  // Check input parameters
  if ((!ac_data) || (cnt_data < 0)) {
    fprintf (stderr, "Vxi11 write error: invalid parameters\n");
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
      fprintf (stderr, "Vxi11 write error: no RPC response\n");
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
    if (p_writeResp->error) {
      fprintf (stderr, "Vxi11 write error: %d\n", (int)p_writeResp->error);
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
// ***************************************************************************
  int Vxi11::
printf (const char *s_format, ...)
{
  if (!s_format) {                      // Check input parameters
    fprintf (stderr, "Vxi11 printf error: invalid parameters\n");
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
    fprintf (stderr, "Vxi11 printf error: vsnprintf error, count = %d\n", cnt);
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
// ***************************************************************************
  int Vxi11::
read (char *ac_data, int cnt_data_max, int *pcnt_read)
{
  int cnt_read_default;                 // Use local variable if user does not
  if (!pcnt_read)                       // specify pcnt_read parameter
    pcnt_read = &cnt_read_default;
  
  *pcnt_read = 0;                       // No bytes read yet

  if (!ac_data || (cnt_data_max < 1)) { // Check input parameters
    fprintf (stderr, "Vxi11 read error: invalid parameters\n");
    return (1);
    }

  ac_data[0] = 0;                       // Null string for early return
  
  Device_ReadParms readParms;           // To send to device_read RPC
  readParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  readParms.requestSize = cnt_data_max; // Maximum # of bytes to read
  readParms.io_timeout = _timeout_ms;	// Timeout for I/O in ms
  readParms.lock_timeout = _timeout_ms;	// Timeout for lock in ms
  readParms.flags = 0;                  // No special flags
  readParms.termChar = 0;               // Termination character not used,
                                        // since flags = 0
  
  // Iterate reads, since internal buffer in device_read RPC call may be less
  // than the maximum number of bytes requested
  do {
    // Number of bytes to read
    readParms.requestSize = cnt_data_max - *pcnt_read;

    // Read from the device
    Device_ReadResp *p_readResp = device_read_1 (&readParms, _p_client);

    if (p_readResp == 0) {              // Check for error
      fprintf (stderr, "Vxi11 read error: no RPC response\n");
      return (1);
      }

    // Copy read data to user buffer
    const int cnt_read = p_readResp->data.data_len; // # of bytes actually read
    if (cnt_read > 0) {
      // Make sure we don't read more than the user buffer can hold.
      // This error should never occur; it would only happen if the device
      // sends more bytes than requested.
      if ((*pcnt_read + cnt_read) > cnt_data_max) {
        fprintf (stderr, "Vxi11 read error: Read more bytes than expected\n");
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
    if (p_readResp->error) {
      fprintf (stderr, "Vxi11 read error: %d\n", (int)p_readResp->error);
      return (1);
      }

    // If "END" indicator is read, then done reading from device
    if (p_readResp->reason & 4)
      break;

    // If user buffer is full, return with error
    else if (*pcnt_read == cnt_data_max) {
      fprintf (stderr, "Vxi11 read error: read buffer full with %d bytes "
               "before reaching END indicator\n", cnt_data_max);
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
// Vxi11::readstb - Read status byte from the device
//                  VXI-11 RPC is "device_readstb"
//
// Parameters: None
//
// Returns: 8-bit status byte if no error
//          -1 if error
// ***************************************************************************
  int Vxi11::
readstb (void)
{
  Device_GenericParms genericParms;        // To send to device_readstb RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  // Read status byte
  Device_ReadStbResp *p_readStbResp=device_readstb_1 (&genericParms,_p_client);

  if (!p_readStbResp) {
    fprintf (stderr, "Vxi11 readstb error: no RPC response\n");
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
  if (p_readStbResp->error) {           // Check error
    printf("Vxi11 readstb error: %d\n", (int)p_readStbResp->error);
    return (-1);
    }

  // Return status byte
  return (p_readStbResp->stb);
}

// ***************************************************************************
// Vxi11::trigger - Send group execute trigger (GET)
//                  VXI-11 RPC is "device_trigger"
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
// ***************************************************************************
  int Vxi11::
trigger (void)
{
  Device_GenericParms genericParms;        // To send to device_trigger RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  // Send trigger command
  Device_Error *p_error = device_trigger_1 (&genericParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11 trigger error: no RPC response\n");
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
  if (p_error->error != 0) {            // Check error
    fprintf (stderr, "Vxi11 trigger error: %d\n", (int)p_error->error);
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
// ***************************************************************************
  int Vxi11::
clear (void)
{
  Device_GenericParms genericParms;        // To send to device_clear RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  // Send clear command
  Device_Error *p_error = device_clear_1 (&genericParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11 clear error: no RPC response\n");
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
  if (p_error->error != 0) {            // Check error
    fprintf (stderr, "Vxi11 clear error: %d\n", (int)p_error->error);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::remote - Place device in remote state
//                 VXI-11 RPC is "device_remote"
//
// Parameters: None
//
// Returns: 0 = no error
//          1 = error
// ***************************************************************************
  int Vxi11::
remote (void)
{
  Device_GenericParms genericParms;        // To send to device_remote RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  // Send remote command
  Device_Error *p_error = device_remote_1 (&genericParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11 remote error: no RPC response\n");
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
  if (p_error->error != 0) {            // Check error
    fprintf (stderr, "Vxi11 remote error: %d\n", (int)p_error->error);
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
// ***************************************************************************
  int Vxi11::
local (void)
{
  Device_GenericParms genericParms;        // To send to device_local RPC
  genericParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  genericParms.io_timeout = _timeout_ms;   // Timeout for I/O in ms
  genericParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  genericParms.flags = 0;                  // Not used

  // Send local command
  Device_Error *p_error = device_local_1 (&genericParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11 local error: no RPC response\n");
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
  if (p_error->error != 0) {            // Check error
    fprintf (stderr, "Vxi11 local error: %d\n", (int)p_error->error);
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
  Device_LockParms lockParms;           // To send to device_lock RPC
  lockParms.lid = _p_link->lid;         // Link ID from create_link RPC call
  lockParms.lock_timeout = _timeout_ms; // Timeout for lock in ms
  lockParms.flags = 1;                  // Wait for lock

  // Send lock command
  Device_Error *p_error = device_lock_1 (&lockParms, _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11 lock error: no RPC response\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  // 11 = device locked by another link
  // 23 = abort
  if (p_error->error != 0) {            // Check error
    fprintf (stderr, "Vxi11 lock error: %d\n", (int)p_error->error);
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
  // Send unlock command
  Device_Error *p_error = device_unlock_1 (&(_p_link->lid), _p_client);

  if (!p_error) {
    fprintf (stderr, "Vxi11 unlock error: no RPC response\n");
    return (1);
    }
  
  // Possible errors
  //  0 = no error
  //  4 = invalid link identifier
  // 12 = no lock held by this link
  if (p_error->error != 0) {            // Check error
    fprintf (stderr, "Vxi11 unlock error: %d\n", (int)p_error->error);
    return (1);
    }

  return (0);
}

// ***************************************************************************
// Vxi11::docmd - Send raw low-level GPIB commands
//                VXI-11 RPC is "device_docmd"
//
// Parameters:
// 1. cmd    - Command code
// 2. s_data - Command data
//
// Returns: 0 = no error
//          1 = error
// ***************************************************************************
  int Vxi11::
docmd (long cmd, const char *s_data)
{
  Device_DocmdParms docmdParms;         // To send to device_docmd RPC
  docmdParms.lid = _p_link->lid;        // Link ID from create_link RPC call
  docmdParms.flags = 0;                 // No flags
  docmdParms.io_timeout = _timeout_ms;  // Timeout for I/O in ms
  docmdParms.lock_timeout= _timeout_ms; // Timeout for lock in ms
  docmdParms.cmd = cmd;                 // Command code
  docmdParms.network_order = 0;         // Little-endian
  docmdParms.datasize = 1;              // 1 byte size
  docmdParms.data_in.data_in_len = strlen (s_data); // Command data size
  docmdParms.data_in.data_in_val = (char *)s_data;  // Command data

  // Send raw low-level GPIB command
  Device_DocmdResp *p_docmdResp = device_docmd_1 (&docmdParms, _p_client);
  
  if (p_docmdResp == 0) {
    fprintf (stderr, "Vxi11 docmd error: no RPC response\n");
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
  if (p_docmdResp->error != 0) {        // Check error
    printf("Vxi11 docmd error: %d\n", (int)p_docmdResp->error);
    return (1);
    }

  return (0);
}
