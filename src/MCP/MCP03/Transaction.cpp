/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Transaction.h"
#include "../../logger.h"
#include "../../tinyformat.h"
#include "../../hash.h"

namespace MCP03
{
	Transaction::Transaction()
		: m_uint16tVersion(CURRENT_TRANSACTION_VERSION),
		m_uint32tLockTime(0)
	{

	}

	Transaction::~Transaction()
	{
		m_vecIn.clear();
		m_vecOut.clear();
	}


	std::string Transaction::toString()
	{
		std::string str;
		str += strprintf("Transaction(Hash=%s, Version=%d, vecIn.size=%u, vecOut.size=%u, LockTime=%u)\n",
			getHash().ToString().substr(0, 10),
			m_uint16tVersion,
			m_vecIn.size(),
			m_vecOut.size(),
			m_uint32tLockTime);

		for( auto &it : m_vecIn )
			str += "    " + it.toString() + "\n";
		for( auto &it : m_vecOut )
			str += "    " + it.toString() + "\n";

		return str;
	}

	uint256 Transaction::getHash()
	{
		// todo: calculate sha3 hash

		return uint256();
	}

	uint64_t Transaction::getValueOut()
	{
		uint64_t nValueOut = 0;
		for( auto &it : m_vecOut)
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

	uint32_t Transaction::getTotalSize()
	{
		// todo: calculate after serialization
		return 1;
	}
}