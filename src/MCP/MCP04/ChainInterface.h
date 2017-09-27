#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_CHAININTERFACE_H__
#define __MCP04_CHAININTERFACE_H__ 1

#include "../../MetaChain.h"
#include "../MCP02/SubChainManager.h"
#include "../MCP03/Transaction.h"
#include "../MCP03/Block.h"

namespace MCP04
{
	class ChainInterface
	{
		public:
			MCP03::Transaction*						createTXElement() { return new MCP03::Transaction; };
			MCP03::Block*							createBlockElement() { return new MCP03::Block; };
	};
}
#endif