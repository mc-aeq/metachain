#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_TRANSACTION_H__
#define __MCP03_TRANSACTION_H__ 1

#include <cstdint>
#include <vector>
#include "../../uint256.h"
#include "txIn.h"
#include "txOut.h"

namespace MCP03
{

#define CURRENT_TRANSACTION_VERSION 1
#define	AMOUNT_COIN 100000000
#define MAX_AMOUNT_COIN 21000000*AMOUNT_COIN

	class Transaction
	{
		public:
										Transaction();
										~Transaction();

			// general settings for this block
			uint16_t					m_uint16tVersion;
			uint32_t					m_uint32tLockTime;
			
			// in and outgoing transactions
			std::vector<txIn>			m_vecIn;
			std::vector<txOut>			m_vecOut;			

			// simple getter and setter
			uint256						getHash();
			bool						isEmpty() { return m_vecIn.empty() && m_vecOut.empty(); };
			uint64_t					getValueOut();
			uint32_t					getTotalSize();
			bool						isCoinBase() { return (m_vecIn.size() == 1 && m_vecIn[0].getTxOutRef()->isEmpty()); };
			std::string					toString();
	};

	inline bool validCoinRange(const uint64_t& nValue) { return (nValue >= 0 && nValue <= MAX_AMOUNT_COIN); }
}

#endif