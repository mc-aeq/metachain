/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "txOut.h"
#include "Transaction.h"
#include "../../tinyformat.h"

namespace MCP03
{
	txOut::txOut()
		: m_uint64tValue( std::numeric_limits<uint64_t>::max() )
	{

	}

	txOut::~txOut()
	{

	}

	std::string txOut::toString()
	{
		return "";
		//return strprintf("txOut(Value=%d.%08d, pubKey=%s)", m_uint64tValue / AMOUNT_COIN, m_uint64tValue % AMOUNT_COIN, m_strPubKey);
	}

	bool txOut::operator==(const txOut& ref)
	{
		return (m_uint64tValue == ref.m_uint64tValue &&
			m_strPubKey == ref.m_strPubKey);
	}

	bool txOut::operator!=(const txOut& ref)
	{
		return !(*this == ref);
	}
}