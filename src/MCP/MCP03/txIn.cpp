/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "txIn.h"
#include "../../tinyformat.h"

namespace MCP03
{
	txIn::txIn()
	{

	}

	txIn::~txIn()
	{

	}

	std::string txIn::toString()
	{
		std::string str;
		str += "txIn(";
		str += m_txPrev.toString();

		if (m_txPrev.isEmpty())
			str += strprintf(", coinbase %s", m_strSignature);
		else
			str += strprintf(", Signature=%s", m_strSignature);

		str += ")";
		return str;
	}

	bool txIn::operator==(const txIn& ref)
	{
		return (m_txPrev == ref.m_txPrev &&
			m_strSignature == ref.m_strSignature);
	}

	bool txIn::operator!=(const txIn& ref)
	{
		return !(*this == ref);
	}
}