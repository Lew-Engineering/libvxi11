# libvxi11

MacOS library for communicating with devices via VXI-11 protocol over TCP/IP

Eddie Lew
Lew Engineering
6/23/2022

INTRODUCTION
------------

This C++ library for Apple MacOS is used by applications to communicate with
devices supporting the VXI-11 protocol over TCP/IP.

This includes various test and measurement equipment that have an Ethernet
port and labeled as LXI compliant.  This library can also talk to LAN / GPIB
gateways such as the Agilent or Keysight E5810, allowing instruments with a
GPIB (also known as HP-IB and IEEE-488) interface to be accessible on a
local Ethernet network.

The RPC protocol for VXI-11 is taken from the following document authored
by the VXIbus Consortium, Inc.:

   VMEBus Extensions for Instrumentation TCP/IP Instrument Protocol
   Specification VXI-11, Revision 1.0, July 17, 1995


COMPILING
---------

The library can be compiled from the command line on any Apple MocOS computer
with the Apple Xcode development tools installed.  Simply place all files
of this project in a single directory and execute the following command:

  make all

This will create libvxi11.so.1, which should be linked to your application.
The libvxi11.h file is the library's C++ header file.


USAGE
-----

  Refer to libvxi11.h for a summary of the member functions available in the
  Vxi11 class.  Refer to the function header comments in vxi11.cpp for a
  detailed description of how to use each member function.

 
  Typical usage is as follows:

  1. Instantiate the Vxi11 class.
     The constructor parameter indicates the hostname of the device to talk to.
     Create a separate object for each instrument you are talking to.

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

  // Example for an instrument with built in Ethernet port
  Vxi11 vxi11_dmm ("dmm6500");      // "dmm6500" is the instrument hostname
  vxi11_dmm.clear ();               // Reset the instrument
  vxi11_dmm.printf ("*idn?");       // Send ID query
  vxi11_dmm.read (s_msg, 1000);     // Read the ID string
  printf ("ID string = %s\n", s_msg);
  vxi11_dmm.local ();               // Return instrument to front-panel control

  // Example for a GPIB instrument connected to an LAN / GPIB gateway
  // "e5810a" below is the hostname of the gateway.  An IP address can also be
  // used.
  // "gpib0,25" indicates to talk to GPIB address 25
  Vxi11 vxi11_ps ("e5810a", "gpib0,25");
  vxi11_ps.clear ();                // Reset the instrument
  vxi11_ps.printf ("*idn?");        // Send the ID query
  vxi11_ps.read (s_msg, 1000);
  printf ("ID string = %s\n", s_msg);
  vxi11_ps.local ();                // Return instrument to front-panel control
}
```
