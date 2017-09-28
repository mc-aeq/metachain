#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_BLOCK_H__
#define __MCP03_BLOCK_H__ 1

#include <cstdint>
#include "../../uint256.h"

namespace MCP03
{
	class Block
	{
		public:
																Block(uint16_t Version)
																{
																	uint16tVersion = Version;
																	hashPrevBlock.SetNull();
																	hashMerkleRoot.SetNull();
																	hash.SetNull();
																	nTime = 0;
																	uint32tByte = 0;
																};

			// general settings for all blocks
			uint16_t											uint16tVersion;
			uint256												hashPrevBlock;
			uint256												hashMerkleRoot;
			uint256												hash;
			uint32_t											nTime;
			uint32_t											uint32tByte;

			// calculation functions
			virtual void										calcMerkleRoot() = 0;
			virtual void										calcSize() = 0;
			virtual void										calcHash() = 0;
			virtual void										calcAll() { calcSize(); calcMerkleRoot(); calcHash(); };

			// simple getter and setter
			virtual bool										isEmpty() { return (uint32tByte == 0); };
			virtual std::string									toString() = 0;
	};
}

#endif