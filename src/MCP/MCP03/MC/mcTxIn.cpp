/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "mcTxIn.h"
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "../../../logger.h"
#include "../../../tinyformat.h"

// register this class for polymorphic exporting
BOOST_CLASS_EXPORT_GUID(MCP03::MetaChain::mcTxIn, "MCP03::MetaChain::mcTxIn")

namespace MCP03
{
	namespace MetaChain
	{
		mcTxIn::mcTxIn()
			: pPayload(nullptr)
		{
		}

		mcTxIn::mcTxIn(ACTION eAction)
			: pPayload(nullptr)
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
			s << "mcTxIn(" << strprintf("Action: %u\n", eAction) << "Content ( ";

			switch (eAction)
			{
				case ACTION::CREATE_SUBCHAIN:
					s << strprintf( "caChainName '%.5s', caPoP '%.5s'\n", ((createSubchain*)pPayload)->caChainName, ((createSubchain*)pPayload)->caPoP );			
					s << "Params ( ";
					for (auto it : ((createSubchain*)pPayload)->mapParams)
						s << it.first << ": " << it.second << "\n";
				break;
			}

			s << ")\n)";

			return s.str();
		}

		uint32_t mcTxIn::getSize()
		{
			uint32_t size = sizeof(strSignature) + txPrev->getSize();

			switch (eAction)
			{
				case ACTION::CREATE_SUBCHAIN:
					size += sizeof(createSubchain);	 // todo: calc size with params
				break;
			}

			return size;
		}
	}
}