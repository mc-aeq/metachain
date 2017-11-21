/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "txOut.h"
#include "Transaction.h"
#include "../../tinyformat.h"
#include "../MCP01/base58.h"

namespace MCP03
{
	txOut::txOut()
		: uint64tValue( std::numeric_limits<uint64_t>::max() )
	{

	}

	txOut::~txOut()
	{

	}

	std::string txOut::toString()
	{
		return strprintf("txOut(Value=%d, pubKey=%x)", uint64tValue, MCP01::base58::encode(uint8tPubKey, 64) );
	}

	bool txOut::operator==(const txOut& ref)
	{
		return (uint64tValue == ref.uint64tValue &&
			(memcmp(uint8tPubKey, ref.uint8tPubKey, 64) == 0));
	}

	bool txOut::operator!=(const txOut& ref)
	{
		return !(*this == ref);
	}
}