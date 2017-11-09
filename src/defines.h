#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __DEFINES_H__
#define __DEFINES_H__ 1

// version define, only changes when a new release is put out
const uint32_t g_cuint32tVersion = 1;

// windows special includes
#ifdef _WIN32
	#include "../targetver.h"
	#include <tchar.h>
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <mswsock.h>
	#include <windows.h>

	// special include if it's a microsoft visual studio dev IDE
	#ifdef _MSC_VER
        	#include <basetsd.h>
	        typedef SSIZE_T ssize_t;
	#endif
#else
        #include <sys/socket.h>
        #include <netinet/in.h>
	#include <sys/fcntl.h>
	#include <sys/mman.h>
	#include <sys/select.h>
	#include <sys/types.h>
	#include <net/if.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <limits.h>
	#include <netdb.h>
	#include <unistd.h>

        typedef unsigned int SOCKET;

        #define WSAGetLastError()   errno
        #define WSAEINVAL           EINVAL
        #define WSAEALREADY         EALREADY
        #define WSAEWOULDBLOCK      EWOULDBLOCK
        #define WSAEMSGSIZE         EMSGSIZE
        #define WSAEINTR            EINTR
        #define WSAEINPROGRESS      EINPROGRESS
        #define WSAEADDRINUSE       EADDRINUSE
        #define WSAENOTSOCK         EBADF
        #define INVALID_SOCKET      (SOCKET)(~0)
        #define SOCKET_ERROR        -1
#endif

// network define standard values
#define NET_STANDARD_PORT_LISTENING 5634
#define NET_STANDARD_CATCHALL_LISTENING "*"
#define DUMP_INTERVAL 600	// all 10 minutes we dump the peers and ban list into our files and store it
#define NET_DEFAULT_CONNECT_TIMEOUT 5000
#define NET_DEFAULT_CONNECT_TRIES	5
#define NET_DEFAULT_MAX_OUTGOING_CONNECTIONS 100
#define NET_DEFAULT_MAX_INCOMING_CONNECTIONS 200
#define NET_DEFAULT_TIME_BETWEEN_UNSUCCESSFUL 300
#define MAX_PAYLOAD_SIZE 33554432 // 2M in bytes
#define MAX_BLOCK_SIZE 4124672 // 8M in bytes
#define	CHECKSUM_SIZE 4 // 4 bytes per network message as checksum
#define MAX_MSG_QUEUE 10

// meta database, rawfile and blockchain output defines
#define LOCK_FILE ".lock"
#define RAW_FILEENDING ".mbl"
#define META_DB_FOLDER "meta"
#define MC_META_DB_FOLDER "mc"
#define MC_CHAIN_IDENTIFIER 0
#define LAST_RAW_FILE "_last.raw.file"
#define MC_NEXT_CI "mc.next_ci"
#define MAX_CHAINNAME_LENGTH 5
#define MAX_POP_NAME 5

// autoupdate defines
#define AU_VERSION_FILE "latest.version"
#define AU_VERSION_FILE_TESTNET "latest.version.testnet"
#define AU_CHECKSUM_FILE "check.sum"
#define AU_NODE_FILE "node.zip"
#define AU_NODE_BACKUPFILE "node.bak"

// HAVE_MSG_NOSIGNAL may not be available on all platforms
#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

// MSG_DONTWAIT is not available on some platforms, if it doesn't exist define it as 0
#if !defined(MSG_DONTWAIT)
#define MSG_DONTWAIT 0
#endif

// define a RELEASE macro that deletes a pointer if it's set
#define RELEASE(x) if(x) {delete x; x = nullptr;}

#endif
