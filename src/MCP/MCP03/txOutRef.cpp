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
		: m_uint16tPos(std::numeric_limits<uint16_t>::max())
	{

	}

	txOutRef::~txOutRef()
	{

	}

	std::string txOutRef::toString()
	{
		return strprintf("txOutRef(%s, %u)", m_Hash.ToString(), m_uint16tPos);
	}

	bool txOutRef::operator<(const txOutRef& ref)
	{
		int cmp = m_Hash.Compare(ref.m_Hash);
		return cmp < 0 || (cmp == 0 && m_uint16tPos < ref.m_uint16tPos);
	}

	bool txOutRef::operator==(const txOutRef& ref)
	{
		return (m_Hash == ref.m_Hash && m_uint16tPos == ref.m_uint16tPos);
	}

	bool txOutRef::operator!=(const txOutRef& ref)
	{
		return !(*this == ref);
	}
}