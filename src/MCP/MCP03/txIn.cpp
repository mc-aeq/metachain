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

	txIn::txIn(std::shared_ptr<txOutRef> ref, std::string sig)
	{
		strSignature = sig;
		txPrev = ref;
	}

	txIn::~txIn()
	{

	}

	std::string txIn::toString()
	{
		std::string str;
		str += "txIn(";

		if (txPrev)
		{
			str += txPrev->toString();

			if (txPrev->isEmpty())
				str += strprintf(", coinbase %s", strSignature);
			else
				str += strprintf(", Signature=%s", strSignature);
		}

		str += ")";
		return str;
	}

	bool txIn::operator==(const txIn& ref)
	{
		return (txPrev == ref.txPrev &&
			strSignature == ref.strSignature);
	}

	bool txIn::operator!=(const txIn& ref)
	{
		return !(*this == ref);
	}
}