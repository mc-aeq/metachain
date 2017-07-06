#pragma once

// windows special includes
#ifdef _WINDOWS
	#include "targetver.h"
	#include <stdio.h>
	#include <tchar.h>
	#include <WinSock2.h>
	#include <WS2tcpip.h>
#endif

// special include if it's a microsoft visual studio dev IDE
#ifdef _MSC_VER
	#include <basetsd.h>
	typedef SSIZE_T ssize_t;
#endif

// linux special includes
#ifdef _LINUX
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif


// standard includes from standard libraries
#include <iostream>
#include <string>
#include <time.h>
#include <fstream>
#include <vector>
#include <map>
#include <stdint.h>
#include <limits>
#include <errno.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

// openssl includes
#include <openssl/crypto.h>

// defines and general functions
#include "src/defines.h"
#include "src/functions.h"

// includes from external sources
#include "src/external/SimpleIni.h"
#include "src/external/tinyformat.h"
#include "src/external/byteswap.h"
#include "src/external/endian.h"
#include "src/external/prevector.h"
#include "src/external/crypto/common.h"
#include "src/external/utilstrencodings.h"
#include "src/external/uint256.h"
#include "src/external/crypto/aes.h"
#include "src/external/crypto/chacha20.h"
#include "src/external/crypto/hmac_sha256.h"
#include "src/external/crypto/hmac_sha512.h"
#include "src/external/crypto/ripemd160.h"
#include "src/external/crypto/sha1.h"
#include "src/external/crypto/sha256.h"
#include "src/external/crypto/sha512.h"
#include "src/external/random.h"
#include "src/external/reverselock.h"
#include "src/external/hash.h"
#include "src/external/cThreadInterrupt.h"
#include "src/external/scheduler.h"

// includes of our own sources
#include "src/logger.h"
#include "src/network/CNetAddr.h"
#include "src/network/CSubNet.h"
#include "src/network/CService.h"
#include "src/MetaChain.h"
#include "src/network/NetworkManager.h"