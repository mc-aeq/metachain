/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "txOutRef.h"
#include "../../tinyformat.h"

namespace MCP03
{
	txOutRef::txOutRef()
		: uint16tPos(std::numeric_limits<uint16_t>::max())
	{

	}

	txOutRef::~txOutRef()
	{

	}

	std::string txOutRef::toString()
	{
		return strprintf("txOutRef(%s, %u)", hash.ToString(), uint16tPos);
	}

	bool txOutRef::operator<(const txOutRef& ref)
	{
		int cmp = hash.Compare(ref.hash);
		return cmp < 0 || (cmp == 0 && uint16tPos < ref.uint16tPos);
	}

	bool txOutRef::operator==(const txOutRef& ref)
	{
		return (hash == ref.hash && uint16tPos == ref.uint16tPos);
	}

	bool txOutRef::operator!=(const txOutRef& ref)
	{
		return !(*this == ref);
	}
}