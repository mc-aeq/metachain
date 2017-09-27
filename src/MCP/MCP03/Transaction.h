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
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/vector.hpp>
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
		private:
			// required to have serialization overrides
			friend class ::boost::serialization::access;

			// serialization
			template<class Archive>
			void												serialize(Archive &ar, const unsigned int version) const
			{
				// note: version is always stored last
				if (version == 1)
					ar << uint16tVersion << uint32tLockTime << vecIn << vecOut;
			}

		public:
												Transaction(uint16_t Version = CURRENT_TRANSACTION_VERSION);
												~Transaction();

			// general settings for this transaction
			uint16_t							uint16tVersion;
			uint32_t							uint32tLockTime;
			
			// in and outgoing transactions
			std::vector<txIn>					vecIn;
			std::vector<txOut>					vecOut;			

			// simple getter and setter
			virtual uint256						getHash();
			virtual bool						isEmpty() { return vecIn.empty() && vecOut.empty(); };
			virtual uint64_t					getValueOut();
			virtual uint32_t					getTotalSize();
			virtual bool						isCoinBase() { return (vecIn.size() == 1 && vecIn[0].txPrev->isEmpty()); };
			virtual std::string					toString();
	};

	inline bool validCoinRange(const uint64_t& nValue) { return (nValue >= 0 && nValue <= MAX_AMOUNT_COIN); }
}

BOOST_CLASS_VERSION(MCP03::Transaction, CURRENT_TRANSACTION_VERSION)
#endif