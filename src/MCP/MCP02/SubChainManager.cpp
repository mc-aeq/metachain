/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "SubChainManager.h"
#include "../../MetaChain.h"
#include "../../logger.h"

namespace MCP02
{
	SubChainManager::SubChainManager()
	{
	}

	SubChainManager::~SubChainManager()
	{
	}

	void SubChainManager::init()
	{
		// per default MC is loaded - these identifiers are always the same!
		LOG("Loading default MetaChain identifier", "SCM");
		addSubChain(CI_DEFAULT_MC_STRING, CI_DEFAULT_MC_UINT16);
	}

	void SubChainManager::addSubChain(std::string strChainName, uint16_t uint16ChainIdentifier)
	{
		// multithreading locking
		LOCK(MetaChain::getInstance().getStorageManager()->csSubChainManager);

		SubChainStruct tmp;
		tmp.uint16ChainIdentifier = uint16ChainIdentifier;
		strncpy(tmp.caChainName, strChainName.c_str(), MAX_CHAINNAME_LENGTH);

		m_vecSubChains.push_back(tmp);
	}

	uint16_t SubChainManager::getChainIdentifier(std::string strChainName)
	{
		// multithreading locking
		LOCK(MetaChain::getInstance().getStorageManager()->csSubChainManager);

		char cBuffer[MAX_CHAINNAME_LENGTH];
		strncpy(cBuffer, strChainName.c_str(), MAX_CHAINNAME_LENGTH);

		for (std::vector< SubChainStruct>::iterator it = m_vecSubChains.begin(); it != m_vecSubChains.end(); it++)
		{
			if (memcmp(cBuffer, it->caChainName, MAX_CHAINNAME_LENGTH) == 0)
				return it->uint16ChainIdentifier;
		}
		return (std::numeric_limits<uint16_t>::max)();
	}


	std::string	SubChainManager::getChainIdentifier(uint16_t uint16tChainIdentifier)
	{
		// multithreading locking
		LOCK(MetaChain::getInstance().getStorageManager()->csSubChainManager);

		for (std::vector< SubChainStruct>::iterator it = m_vecSubChains.begin(); it != m_vecSubChains.end(); it++)
		{
			if (it->uint16ChainIdentifier == uint16tChainIdentifier)
				return it->caChainName;
		}
		return "";
	}
}