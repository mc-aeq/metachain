#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_POT_H__
#define __MCP04_POT_H__ 1

#include "ChainInterface.h"
#include "../MCP03/crBlock.h"
#include "../MCP03/crTransaction.h"
#include "../MCP02/SubChainManager.h"

namespace MCP04
{
	class PoT : public ChainInterface
	{
		protected:
			static const std::string			m_strName;

		public:
			static bool							registerFactory() { return ::MetaChain::getInstance().getStorageManager()->getSubChainManager()->registerFactory(m_strName, &createInstance); };
			static bool							registerFactory(MCP02::SubChainManager *ptr) { return ptr->registerFactory(m_strName, &createInstance); };
			static ChainInterface				*createInstance() { return new PoT(); };

			MCP03::Transaction*					createTXElement() { return new MCP03::crTransaction(); };
			MCP03::Block*						createBlockElement() { return new MCP03::crBlock(); };
	};
}

#endif