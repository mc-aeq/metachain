/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "SubChainManager.h"
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
		SubChainStruct tmp;
		tmp.uint16ChainIdentifier = uint16ChainIdentifier;
		strncpy(tmp.caChainName, strChainName.c_str(), MAX_CHAINNAME_LENGTH);

		m_vecSubChains.push_back(tmp);
	}
}