#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP02_SUBCHAINSTRUCT_H__
#define __MCP02_SUBCHAINSTRUCT_H__ 1

#include <string>

namespace MCP02
{
#define MAX_CHAINNAME_LENGTH 4

	// this is the struct that we store in our vector with information about the subchains
	struct SubChainStruct
	{
		uint16_t		uint16ChainIdentifier;
		char			caChainName[MAX_CHAINNAME_LENGTH];

		template <typename Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			if (version == 1)
			{
				ar & uint16ChainIdentifier;
				ar & caChainName;
			}
		}
	};
}

BOOST_CLASS_VERSION(MCP02::SubChainStruct, 1)
#endif