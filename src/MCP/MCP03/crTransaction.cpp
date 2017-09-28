/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "crTransaction.h"
#include <boost/archive/binary_oarchive.hpp>
#include "../../logger.h"
#include "../../tinyformat.h"
#include "../../hash.h"
#include "../../prevector.h"
#include "../../crypto/sha3.h"

namespace MCP03
{
	crTransaction::crTransaction(uint16_t Version)
		: Transaction(Version),
		uint32tLockTime(0)
	{
	}

	crTransaction::~crTransaction()
	{
		vecIn.clear();
		vecOut.clear();
	}


	std::string crTransaction::toString()
	{
		std::stringstream s;
		s << strprintf("Transaction(Hash=%s, Version=%d, vecIn.size=%u, vecOut.size=%u, LockTime=%u)\n",
			getHash().ToString().substr(0, 10),
			uint16tVersion,
			vecIn.size(),
			vecOut.size(),
			uint32tLockTime);

		for (auto &it : vecIn)
			s << "    " + it.toString() + "\n";
		for (auto &it : vecOut)
			s << "    " + it.toString() + "\n";

		return s.str();
	}

	uint256 crTransaction::getHash()
	{
		// serialize this transaction
		std::stringstream stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
		{
			boost::archive::binary_oarchive oa(stream, boost::archive::no_header | boost::archive::no_tracking);
			oa << *this;
		}

		// return the calculated hash
		SHA3 crypto;
		return crypto.hash256(SHA3::HashType::DEFAULT, (uint8_t *)stream.str().data(), stream.str().size());
	}

	uint64_t crTransaction::getValueOut()
	{
		uint64_t nValueOut = 0;
		for (auto &it : vecOut)
		{
			nValueOut += it.getValue();
			if (!validCoinRange(it.getValue()) || !validCoinRange(nValueOut))
			{
				LOG_ERROR("Coin Amount out of range: " + it.toString(), "TX");
				return (std::numeric_limits<uint64_t>::max)();
			}
		}
		return nValueOut;
	}

	uint32_t crTransaction::getTotalSize()
	{
		// serialize this transaction
		std::stringstream stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
		{
			boost::archive::binary_oarchive oa(stream, boost::archive::no_header | boost::archive::no_tracking);
			oa << *this;
		}

		return stream.str().size();
	}
}