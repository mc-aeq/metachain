/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "SubChainManager.h"
#include <boost/archive/binary_oarchive.hpp>
#include "../../MetaChain.h"
#include "../../logger.h"
#include "../MCP03/Transaction.h"
#include "../MCP03/Block.h"
#include "../MCP04/PoMC.h"
#include "../MCP04/MC/mcActions.h"
#include "../MCP04/PoS.h"
#include "../MCP04/PoT.h"

namespace MCP02
{
	SubChainManager::SubChainManager()
	{
	}

	SubChainManager::~SubChainManager()
	{
	}

	bool SubChainManager::init()
	{

		// MC default ChainIdentifier
		LOG("Loading default MetaChain identifier", "SCM");
		if (addSubChain(CI_DEFAULT_MC_STRING) != 0)
		{
			LOG_ERROR("Adding of the MetaChain identifier didn't work as expected.", "SCM");
			return false;
		}

		// MC genesis block
		LOG("Generating MetaChain genesis block", "SCM");

		std::shared_ptr< MCP04::MetaChain::mcTransaction > txGenesis = std::make_shared<MCP04::MetaChain::mcTransaction>();
		txGenesis->uint16tVersion = 1;
		txGenesis->vecIn.emplace_back(1, MCP04::MetaChain::mcTxIn::ACTION::CREATE_SUBCHAIN );
		memcpy(((MCP04::MetaChain::createSubchain*)txGenesis->vecIn[0].pPayload)->caChainName, CI_DEFAULT_MC_STRING, sizeof(CI_DEFAULT_MC_STRING));
		memcpy(((MCP04::MetaChain::createSubchain*)txGenesis->vecIn[0].pPayload)->caPoP, MC_DEFAULT_POP, sizeof(MC_DEFAULT_POP));
		((MCP04::MetaChain::createSubchain*)txGenesis->vecIn[0].pPayload)->uint64tMaxCoins = 0;

		MCP04::MetaChain::mcBlock genesis;
		genesis.nTime = 1506343480;
		genesis.uint16tVersion = 1;
		genesis.vecTx.push_back( std::move(txGenesis) );
		genesis.hashPrevBlock.SetNull();
		genesis.calcAll();

		// TCT genesis block
		LOG("Generating TCT genesis block", "SCM");

		// everything smooth
		return true;
	}

	void SubChainManager::initStandardPoP()
	{
		// load our standard proof of process initiators
		MCP04::PoMC::registerFactory();
		MCP04::PoS::registerFactory();
		MCP04::PoT::registerFactory();
	}

	// returns the uint16_t from the newly added CI
	uint16_t SubChainManager::addSubChain(std::string strChainName)
	{
		// multithreading locking
		LOCK(MetaChain::getInstance().getStorageManager()->csSubChainManager);

		// check whether this chain name already exists. If so, we don't add it
		if (getChainIdentifier(strChainName) != (std::numeric_limits<uint16_t>::max)())
		{
			LOG_ERROR("SC with the same name already exists. Not adding!", "SCM");
			return (std::numeric_limits<uint16_t>::max)();
		}

		// adding subchain into our vector
		SubChainStruct tmp;
		MetaChain::getInstance().getStorageManager()->incMetaValue(MC_NEXT_CI, (uint16_t)0, &tmp.uint16ChainIdentifier);
		strncpy(tmp.caChainName, strChainName.c_str(), MAX_CHAINNAME_LENGTH);
		m_vecSubChains.push_back(tmp);

		return tmp.uint16ChainIdentifier;
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

	bool SubChainManager::registerFactory(std::string strName, MCP04::ChainInterface*(*ptr)(void))
	{
		try
		{
			m_mapProofFactories.at(strName);

			// if this doesn't throw an exception it means we already have this entry. So we have to report that and don't add it
			LOG_ERROR("Already having a module loaded that provides the following proof of process: " + strName + ", not loading new module!", "SCM");
			return false;
		}
		catch (...)
		{
			// the only thing we care about this exception is, that there is no entry! Now we can safely add this new creator function to our factory
			LOG("Registering new proof of process: " + strName, "SCM");
			m_mapProofFactories.emplace(strName, ptr);
			return true;
		}
	}
}