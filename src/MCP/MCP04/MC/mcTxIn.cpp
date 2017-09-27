/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "mcTxIn.h"
#include "../../../logger.h"
#include "../../../tinyformat.h"

namespace MCP04
{
	namespace MetaChain
	{
		mcTxIn::mcTxIn(uint16_t Version)
			: uint16tVersion(Version),
			pPayload(nullptr)
		{
		}

		mcTxIn::mcTxIn(uint16_t Version, ACTION eAction)
			: uint16tVersion(Version),
			pPayload(nullptr)
		{
			this->eAction = eAction;

			switch (eAction)
			{
				case ACTION::CREATE_SUBCHAIN:
					pPayload = new createSubchain();
				break;

				default:
					LOG_ERROR("Unknown Action for mcTxIn", "MCTXIN");
			}
		}

		mcTxIn::~mcTxIn()
		{
			if (pPayload)
				delete pPayload;
		}

		std::string mcTxIn::toString()
		{
			std::string str;
			str = "mcTxIn(";
			str += strprintf("Action %u\n", eAction);
			str += "Params(";

			switch (eAction)
			{
				case ACTION::CREATE_SUBCHAIN:		str += strprintf( "caChainName %s, caPoP %s, uint64tMaxCoins %u", ((createSubchain*)pPayload)->caChainName, ((createSubchain*)pPayload)->caPoP, ((createSubchain*)pPayload)->uint64tMaxCoins );			break;
			}

			str += ")\n)";
			return str;
		}

		uint32_t mcTxIn::getSize()
		{
			uint32_t size = sizeof(strSignature) + txPrev->getSize();

			switch (eAction)
			{
				case ACTION::CREATE_SUBCHAIN:		size += sizeof(createSubchain);			break;
			}

			return size;
		}
	}
}