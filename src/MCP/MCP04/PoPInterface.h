#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_POPINTERFACE_H__
#define __MCP04_POPINTERFACE_H__ 1

#include "../../MetaChain.h"
#include "../MCP02/SubChainManager.h"
#include "../MCP03/Transaction.h"
#include "../MCP03/Block.h"

namespace MCP04
{
	class PoPInterface
	{
		public:
			virtual MCP03::Transaction*						createTXElement() = 0;
			virtual MCP03::Block*							createBlockElement() = 0;
	};
}
#endif