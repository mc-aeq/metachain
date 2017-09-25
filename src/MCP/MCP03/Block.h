#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_BLOCK_H__
#define __MCP03_BLOCK_H__ 1

#include <cstdint>
#include <vector>
#include "../../uint256.h"
#include "Transaction.h"

namespace MCP03
{

#define CURRENT_BLOCK_VERSION 1

	class Block
	{
		private:
			// general settings for this block
			uint16_t								m_uint16tVersion;
			uint256									m_hashPrevBlock;
			uint256									m_hashMerkleRoot;
			uint32_t								m_nTime;
			uint32_t								m_uint32tByte;

			// the tx inside this block
			std::vector< Transaction >				m_vecTx;

		public:
													Block();
													~Block();

			// simple getter and setter
			bool									isEmpty() { return (m_uint32tByte == 0); };
			uint16_t								getVersion() { return m_uint16tVersion; };
			uint32_t								getTime() { return m_uint32tByte; };
			std::string								toString();
			uint256									getHash();
	};
}

#endif