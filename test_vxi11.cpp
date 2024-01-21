// ***************************************************************************
// test_vxi11.cpp - Test the Vxi11 class
//
// Written by Eddie Lew, Lew Engineering
// Copyright (C) 2020 Eddie Lew
//
// Edit history:
//
// 01-21-24 - Renamed from test.cpp to test_vxi11.cpp.
// 05-19-20 - Started file.
// ***************************************************************************

#include "libvxi11.h"
#include <stdio.h>

int main ()
{
  char s_msg[1000];

  // Read ID from DMM6500
  Vxi11 vxi11_dmm ("dmm6500");
  vxi11_dmm.clear();
  vxi11_dmm.printf ("*idn?");
  vxi11_dmm.read (s_msg, 1000);
  printf ("DMM ID = %s\n", s_msg);
  vxi11_dmm.local();

  // Read ID from power supply
  Vxi11 vxi11_ps ("e5810a", "gpib0,25");
  vxi11_ps.clear();
  vxi11_ps.printf ("id?");
  vxi11_ps.read (s_msg, 1000);
  printf ("PS ID = %s\n", s_msg);
  vxi11_ps.local();
}
