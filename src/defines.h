#pragma once

// version define, only changes when a new release is put out
const uint32_t g_cuint32tVersion = 1;

// network define standard values
#define NET_STANDARD_PORT_LISTENING 5634
#define NET_STANDARD_CATCHALL_LISTENING "*"
#define DUMP_INTERVAL 600	// all 10 minutes we dump the peers and ban list into our files and store it
#define NET_DEFAULT_CONNECT_TIMEOUT 5000
#define NET_DEFAULT_CONNECT_TRIES	5
#define NET_DEFAULT_MAX_OUTGOING_CONNECTIONS 100
#define NET_DEFAULT_MAX_INCOMING_CONNECTIONS 200
#define NET_DEFAULT_TIME_BETWEEN_UNSUCCESSFUL 300
#define MAX_PAYLOAD_SIZE 33554432 // 32M in bytes
#define	CHECKSUM_SIZE 4 // 4 bytes per network message as checksum
#define MAX_MSG_QUEUE 10

// HAVE_MSG_NOSIGNAL may not be available on all platforms
#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

// MSG_DONTWAIT is not available on some platforms, if it doesn't exist define it as 0
#if !defined(MSG_DONTWAIT)
#define MSG_DONTWAIT 0
#endif
