#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP02_SUBCHAINSTRUCT_H__
#define __MCP02_SUBCHAINSTRUCT_H__ 1

#include <string>
#include "../../defines.h"
#include "../MCP04/ChainInterface.h"
#include "../../io/db/dbEngine.h"

// forward decl
namespace MCP04 { class ChainInterface; };

namespace MCP02
{
	// this is the struct that we store in our vector with information about the subchains
	struct SubChainStruct
	{
		uint16_t						uint16ChainIdentifier;
		char							caChainName[MAX_CHAINNAME_LENGTH];
		char							caPoP[MAX_POP_NAME];
		MCP04::ChainInterface			*ptr;
		dbEngine						*db; // todo: move this from the subchain struct into the class that is used for the subchain

		// serialization
		template <typename Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			if (version == 1)
			{
				ar & uint16ChainIdentifier;
				ar & caChainName;
				ar & caPoP;
			}
		}
	};
}

BOOST_CLASS_VERSION(MCP02::SubChainStruct, 1)
#endif