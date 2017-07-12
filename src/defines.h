#pragma once

// version define, only changes when a new release is put out
#define VERSION 0.1

// logging macros for faster development, interchangeability and readability
#define LOG(logline, module) Logger::getInstance().log(##logline, Logger::facility::info, ##module)
#define LOG_WARNING(logline, module) Logger::getInstance().log(##logline, Logger::facility::warning, ##module)
#define LOG_DEBUG(logline, module) Logger::getInstance().log(##logline, Logger::facility::debug, ##module)
#define LOG_ERROR(logline, module) Logger::getInstance().log(##logline, Logger::facility::error, ##module)

// network define standard values
#define NET_STANDARD_PORT_LISTENING 5634
#define NET_STANDARD_CATCHALL_LISTENING "*"
#define DUMP_INTERVAL 600	// all 10 minutes we dump the peers and ban list into our files and store it
#define NET_DEFAULT_CONNECT_TIMEOUT 5000
#define NET_DEFAULT_CONNECT_TRIES	5
#define NET_DEFAULT_MAX_OUTGOING_CONNECTIONS 100
#define NET_DEFAULT_MAX_INCOMING_CONNECTIONS 200
#define NET_DEFAULT_TIME_BETWEEN_UNSUCCESSFUL 300