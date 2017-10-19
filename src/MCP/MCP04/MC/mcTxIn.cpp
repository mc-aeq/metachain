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
			init(eAction);
		}

		mcTxIn::~mcTxIn()
		{
			if (pPayload)
				delete pPayload;
		}

		void mcTxIn::init(ACTION eAction)
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

		std::string mcTxIn::toString()
		{
			std::stringstream s;
			s << "mcTxIn(" << strprintf("Action %u\n", eAction) << "Params(";

			switch (eAction)
			{
				case ACTION::CREATE_SUBCHAIN:		s << strprintf( "caChainName %s, caPoP %s, uint64tMaxCoins %u", ((createSubchain*)pPayload)->caChainName, ((createSubchain*)pPayload)->caPoP, ((createSubchain*)pPayload)->uint64tMaxCoins );			break;
			}

			s << ")\n)";

			return s.str();
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