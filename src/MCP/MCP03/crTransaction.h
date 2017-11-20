#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_CRTRANSACTION_H__
#define __MCP03_CRTRANSACTION_H__ 1

#include <cstdint>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/vector.hpp>
#include "../../uint256.h"
#include "txIn.h"
#include "txOut.h"
#include "Transaction.h"

namespace MCP03
{
#define CURRENT_CRTRANSACTION_VERSION 1

	class crTransaction : public Transaction
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
															crTransaction(uint16_t Version = CURRENT_CRTRANSACTION_VERSION);
															~crTransaction();

		// advanced settings for currency transactions
		uint32_t											uint32tLockTime;

		// in and outgoing transactions
		std::vector<txIn>									vecIn;
		std::vector<txOut>									vecOut;

		// simple getter and setter
		uint256												getHash();
		bool												isEmpty() { return vecIn.empty() && vecOut.empty(); };
		uint64_t											getValueOut();
		uint32_t											getTotalSize();
		bool												isCoinBase() { return (vecIn.size() == 1 && vecIn[0].txPrev->isEmpty()); };
		std::string											toString();
	};
}

BOOST_CLASS_VERSION(MCP03::crTransaction, CURRENT_CRTRANSACTION_VERSION)
#endif