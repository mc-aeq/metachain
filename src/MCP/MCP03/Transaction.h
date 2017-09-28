#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_TRANSACTION_H__
#define __MCP03_TRANSACTION_H__ 1

#include <cstdint>
#include "../../uint256.h"

namespace MCP03
{
	class Transaction
	{
		public:
												Transaction(uint16_t Version) { uint16tVersion = Version; };

			// general settings for this transaction
			uint16_t							uint16tVersion;

			// simple getter and setter
			virtual uint256						getHash() = 0;
			virtual bool						isEmpty() = 0;
			virtual uint64_t					getValueOut() = 0;
			virtual uint32_t					getTotalSize() = 0;
			virtual std::string					toString() = 0;
	};
}

#endif