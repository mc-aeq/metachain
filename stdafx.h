#pragma once

#ifdef _WINDOWS
	#include "targetver.h"
	#include <stdio.h>
	#include <tchar.h>
	#include <WinSock2.h>
	#include <WS2tcpip.h>
#endif

#ifdef _LINUX
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

#include <iostream>
#include <string>
#include <time.h>
#include <fstream>
#include <vector>
#include <map>
#include <stdint.h>
#include <limits>
#include <errno.h>

#include "src/defines.h"

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
#include "src/external/hash.h"

#include "src/logger.h"
#include "src/network/CNetAddr.h"
#include "src/network/CSubNet.h"
#include "src/network/CService.h"
#include "src/MetaChain.h"
#include "src/network/NetworkManager.h"