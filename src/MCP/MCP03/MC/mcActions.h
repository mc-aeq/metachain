#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_MC_ACTIONS_H__
#define __MCP04_MC_ACTIONS_H__ 1

#include "../../../defines.h"

namespace MCP03
{
	namespace MetaChain
	{
		struct createSubchain
		{
			char			caChainName[MAX_CHAINNAME_LENGTH];
			char			caPoP[MAX_POP_NAME];
			uint64_t		uint64tMaxCoins;

			// serialization
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				if (version == 1)
					ar & caChainName & caPoP & uint64tMaxCoins;
			}
		};
	}
}

BOOST_CLASS_VERSION(MCP03::MetaChain::createSubchain, 1)

#endif