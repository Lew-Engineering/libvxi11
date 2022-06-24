# ****************************************************************************
# makefile - Makefile for libvxi11 library for VXI-11 instrument communication
#
# Written by Eddie Lew, Lew Engineering
#
# Edit history:
#
# 05-18-20 - Started file.
# ****************************************************************************

# Shared object library version
SOVERSION=1

# C flags
CFLAGS=

# Default target
all: libvxi11.so.${SOVERSION}

# Clean
clean:
	rm -f *.o libvxi11.so.${SOVERSION} vxi11_rpc.h vxi11_rpc_clnt.c vxi11_rpc_svc.c vxi11_rpc_xdr.c test

# Library
libvxi11.so.${SOVERSION} : vxi11.o vxi11_rpc_clnt.o vxi11_rpc_xdr.o
	g++ -shared -undefined dynamic_lookup -o $@ $^

# User interface to VXI-11 library
vxi11.o: vxi11.cpp libvxi11.h vxi11_rpc.h
	g++ -fPIC $(CFLAGS) -c $< -o $@

# RPC generation of VXI-11 protocol
vxi11_rpc.h vxi11_rpc_clnt.c vxi11_rpc_xdr.c : vxi11_rpc.x
	rpcgen -C vxi11_rpc.x

vxi11_rpc_clnt.o : vxi11_rpc_clnt.c
	gcc -fPIC -Wno-incompatible-pointer-types $(CFLAGS) -c $< -o $@

vxi11_rpc_xdr.o : vxi11_rpc_xdr.c
	gcc -fPIC -Wno-incompatible-pointer-types $(CFLAGS) -c $< -o $@

# Test executable
test: test.cpp libvxi11.h libvxi11.so.${SOVERSION}
	g++ test.cpp libvxi11.so.${SOVERSION} -o test
