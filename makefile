# ****************************************************************************
# makefile - Makefile for libvxi11 library for VXI-11 instrument communication
#
# Written by Eddie Lew, Lew Engineering
# Copyright (C) 2020 Eddie Lew
#
# Edit history:
#
# 01-21-24 - Added support for Linux in addition to MacOS.
#            Added install target to install library to /usr/local.
#            MacOS now creates a libvxi11.dylib, Linux creates libvxi11.so.
#            Test program is now called test_vxi11.
# 05-18-20 - Started file.
# ****************************************************************************

# NOTE: If compiling with MacOS XCode 15 and later you may need to set
#       DYLD_LIBRARY_PATH=/usr/local/lib

# Shared object library version
SOVERSION=1

# OS name
UNAME := $(shell uname -s)

# OS independent flags
CCFLAGS=
LIBFLAGS=
SOFLAGS=

# MacOS (OSX) specific flags
ifeq ($(UNAME),Darwin)
  SOFLAGS+=-dynamiclib -undefined dynamic_lookup
  SOLIB=libvxi11.$(SOVERSION).dylib
  SOLIBBASE=libvxi11.dylib
endif

# Linux specific flags
ifeq ($(UNAME),Linux)
  SOFLAGS+=-shared
  CCFLAGS+=-I/usr/include/tirpc
  LIBFLAGS+=-ltirpc
  SOLIB=libvxi11.so.$(SOVERSION)
  SOLIBBASE=libvxi11.so
endif

# Default target
all: $(SOLIB) test_vxi11

# Clean
clean:
	rm -f *.o $(SOLIB) $(SOLIBBASE) vxi11_rpc.h vxi11_rpc_clnt.c vxi11_rpc_svc.c vxi11_rpc_xdr.c test_vxi11

# Library
${SOLIB}: vxi11.o vxi11_rpc_clnt.o vxi11_rpc_xdr.o
	g++ $(SOFLAGS) -o $@ $^ $(LIBFLAGS)
	ln -s -f $(SOLIB) $(SOLIBBASE)

# User interface to VXI-11 library
vxi11.o: vxi11.cpp libvxi11.h vxi11_rpc.h
	g++ -fPIC $(CCFLAGS) -c $< -o $@

# RPC generation of VXI-11 protocol
vxi11_rpc.h vxi11_rpc_clnt.c vxi11_rpc_xdr.c : vxi11_rpc.x
	rpcgen -C vxi11_rpc.x

vxi11_rpc_clnt.o : vxi11_rpc_clnt.c
	gcc -fPIC -Wno-incompatible-pointer-types $(CCFLAGS) -c $< -o $@

vxi11_rpc_xdr.o : vxi11_rpc_xdr.c
	gcc -fPIC -Wno-incompatible-pointer-types $(CCFLAGS) -c $< -o $@

# Test executable
test_vxi11: test_vxi11.cpp libvxi11.h $(SOLIB)
	g++ $(CCFLAGS) test_vxi11.cpp -L./ -lvxi11 -o test_vxi11

# Install libraries
install:
	cp libvxi11.h /usr/local/include
	cp $(SOLIB) /usr/local/lib
	ln -s -f $(SOLIB) /usr/local/lib/$(SOLIBBASE)
