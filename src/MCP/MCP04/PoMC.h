#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP04_POMC_H__
#define __MCP04_POMC_H__ 1

#include "ChainInterface.h"
#include "MC/mcTransaction.h"
#include "MC/mcBlock.h"
#include "../MCP02/SubChainManager.h"

namespace MCP04
{
	class PoMC : public ChainInterface
	{
		protected:
			static const std::string			m_strName;

		public:
			static bool							registerFactory() { return ::MetaChain::getInstance().getStorageManager()->getSubChainManager()->registerFactory(m_strName, &createInstance); };
			static bool							registerFactory(MCP02::SubChainManager *ptr) { return ptr->registerFactory(m_strName, &createInstance); };
			static ChainInterface				*createInstance() { return new PoMC(); };

			MCP03::Transaction*					createTXElement() { return new MCP04::MetaChain::mcTransaction; };
			MCP03::Block*						createBlockElement() { return new MCP04::MetaChain::mcBlock; };
	};
}
#endif