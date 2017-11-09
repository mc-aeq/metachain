#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_MC_ACTIONS_H__
#define __MCP04_MC_ACTIONS_H__ 1

#include "../../../defines.h"
#include <boost/serialization/map.hpp>

namespace MCP03
{
	namespace MetaChain
	{
		struct createSubchain
		{
			char										caChainName[MAX_CHAINNAME_LENGTH];
			char										caSubChainClassName[MAX_SUBCHAIN_CLASSNAME_LENGTH];
			char										caPoP[MAX_POP_NAME];
			std::map< std::string, std::string >		mapParams;

			// serialization
			template <typename Archive>
			void serialize(Archive& ar, const unsigned int version)
			{
				if (version == 1)
					ar & caChainName & caSubChainClassName & caPoP & mapParams;
			}
		};
	}
}

BOOST_CLASS_VERSION(MCP03::MetaChain::createSubchain, 1)

#endif