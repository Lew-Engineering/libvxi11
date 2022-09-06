# libvxi11

MacOS C++ library for communicating with devices via the VXI-11 TCP/IP protocol

Eddie Lew, Lew Engineering, 6/23/2022

INTRODUCTION
------------

This C++ library for Apple MacOS is used by applications to communicate with
devices supporting the VXI-11 TCP/IP protocol.

This includes various test and measurement equipment that have an Ethernet
port and labeled as LXI compliant.  This library can also talk to LAN / GPIB
gateways such as the Agilent E5810A or Keysight E5810B, allowing instruments
with a GPIB (also known as HP-IB and IEEE-488) interface to be accessible on a
local Ethernet network.

In fact, using an E5810A/B with this library is one of the easiest ways of
talking to GPIB instruments on a Mac.  No drivers are needed; simply link this
library to your program.  Also, since the E5810A/B is on your network, multiple
Macs can connect to multiple GPIB devices at the same time.  And unlike USB
GPIB interfaces, you are not tethered by a USB cable.  You can control your
GPIB instruments from any Mac anywhere in your lab.

The RPC protocol for VXI-11 is taken from the following document authored
by the VXIbus Consortium, Inc.:

>   VMEBus Extensions for Instrumentation TCP/IP Instrument Protocol
   Specification VXI-11, Revision 1.0, July 17, 1995

The specification is included here in the VXI-11_Specification directory.


COMPILING
---------

The library can be compiled from the command line on any Apple Macintosh
computer with the Apple Xcode development tools installed.  Simply place all
files of this project in a single directory and execute the following command:

> `make all`

This will create libvxi11.so.1, which should be linked to your application.
The libvxi11.h file is the library's C++ header file.


USAGE
-----

  Refer to libvxi11.h for a summary of the member functions available in the
  Vxi11 class.  Refer to the function header comments in vxi11.cpp for a
  detailed description of how to use each member function.

 
  Typical usage is as follows:

  1. Instantiate the Vxi11 class.
     The constructor parameter indicates the hostname or IP address of the
     device to talk to.  Create a separate object for each instrument you are
     talking to.

  2. Reset the instrument with the clear() member function.
  
  3. Send commands to the device via the write() or printf() member functions.

  4. Read from the device via the read() member function.

  5. For convenience, a query() member function does a write and read in one
     call.

  6. When exiting your application, optionally call the local() member
     function to return the instrument to local front panel control and
     optionally call the close() member function to close the connection.
  

EXAMPLE
-------
```
#include "libvxi11.h"
#include <stdio.h>

int main ()
{
  char s_msg[1000];

  // Example for an instrument with a built-in Ethernet port
  // "dmm6500" is the instrument hostname
  Vxi11 vxi11_dmm ("dmm6500");
  vxi11_dmm.clear ();                          // Reset the instrument
  vxi11_dmm.query ("*idn?", s_msg, 1000);      // Send ID query
  printf ("ID string = %s\n", s_msg);
  vxi11_dmm.printf (":sens:func \"volt:dc\""); // Select DC volts  
  double d_volts;
  vxi11_dmm.query (":read?", &d_volts);        // Do voltage measurement
  printf ("Volts = %f\n", d_volts);
  vxi11_dmm.local ();                          // Return to front-panel control

  // Example for a GPIB instrument connected to an LAN / GPIB gateway
  // "e5810a" below is the hostname of the gateway.  An IP address can also be
  // used.
  // "gpib0,25" indicates to talk to GPIB address 25
  Vxi11 vxi11_ps ("e5810a", "gpib0,25");
  vxi11_ps.clear ();                           // Reset the instrument
  vxi11_ps.query ("*idn?", s_msg, 1000);       // Send the ID query
  printf ("ID string = %s\n", s_msg);
  vxi11_ps.printf ("iset %f; vset %f", 0.25, 5.5); // Set current & voltage
  vxi11_ps.local ();                           // Return to front-panel control

  return (0);
}
```
