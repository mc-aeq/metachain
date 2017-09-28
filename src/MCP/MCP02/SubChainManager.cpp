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
		initStandardPoP();
	}

	SubChainManager::~SubChainManager()
	{
	}

	bool SubChainManager::init()
	{
		// MC genesis block
		LOG("Generating MetaChain genesis block", "SCM");

		std::shared_ptr< MCP04::MetaChain::mcTransaction > txGenesis = std::make_shared<MCP04::MetaChain::mcTransaction>();
		txGenesis->uint16tVersion = 1;
		txGenesis->txIn.init( MCP04::MetaChain::mcTxIn::ACTION::CREATE_SUBCHAIN );
		memcpy(((MCP04::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->caChainName, CI_DEFAULT_MC_STRING, sizeof(CI_DEFAULT_MC_STRING));
		memcpy(((MCP04::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->caPoP, MC_DEFAULT_POP, sizeof(MC_DEFAULT_POP));
		((MCP04::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->uint64tMaxCoins = 0;

		MCP04::MetaChain::mcBlock genesis;
		genesis.nTime = 1506343480;
		genesis.uint16tVersion = 1;
		genesis.pTransaction = std::move(txGenesis);
		genesis.hashPrevBlock.SetNull();
		genesis.calcAll();

		LOG("MetaChain genesis block hash is: " + genesis.hash.ToString(), "SCM");

#ifdef _DEBUG
		LOG_DEBUG("MetaChain Genesis block contents: " + genesis.toString(), "SCM");
#endif

		// checking consistency of our MC genesis block
		uint8_t genesisRawBuffer[32] = { 0xdc, 0x03, 0x4e, 0x58, 0xf0, 0xaa, 0xcf, 0x47, 0x0d, 0x96, 0x73, 0x0d, 0x90, 0xb2, 0x69, 0x1f, 0x41, 0x87, 0x86, 0x5b, 0x0f, 0x75, 0x93, 0x6a, 0x49, 0xc6, 0xbf, 0x53, 0xb5, 0x94, 0xfe, 0x70 };
		const uint256 validMCGenesisHash(&genesisRawBuffer[0], 32);
		if (genesis.hash != validMCGenesisHash)
		{
#ifdef _DEBUG
			LOG_DEBUG("WARNING!!! Genesis Block Hash doesn't match. Not Exiting", "SCM");
#else
			LOG_ERROR("The MetaChain genesis block hash doesn't match the precomputed safe hash!", "SCM");
			return false;
#endif
		}

		// adding the MC as subchain
		LOG("Loading MetaChain genesis block", "SCM");
		if (addSubChain(&genesis) != 0)
		{
			LOG_ERROR("Adding of the MetaChain identifier didn't work as expected.", "SCM");
			return false;
		}

		// TCT genesis block
		LOG("Generating TCT genesis block", "SCM");

		// everything smooth
		return true;
	}

	void SubChainManager::initStandardPoP()
	{
		// load our standard proof of process initiators
		MCP04::PoMC::registerFactory(this);
		MCP04::PoS::registerFactory(this);
		MCP04::PoT::registerFactory(this);
	}

	// returns the uint16_t from the newly added SubChain
	uint16_t SubChainManager::addSubChain(MCP04::MetaChain::mcBlock *block)
	{
		// multithreading locking
		LOCK(MetaChain::getInstance().getStorageManager()->csSubChainManager);

		// checking if this is really a genesis request block
		if( block->pTransaction->txIn.eAction != MCP04::MetaChain::mcTxIn::ACTION::CREATE_SUBCHAIN )
		{
			LOG_ERROR("A non-genesis creation block was passed into the genesis creation function. Not executing!", "SCM");
			return (std::numeric_limits<uint16_t>::max)();
		}

		// check whether this chain name already exists. If so, we don't add it
		if (getChainIdentifier(((MCP04::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caChainName) != (std::numeric_limits<uint16_t>::max)())
		{
			LOG_ERROR("SC with the same name '" + std::string(((MCP04::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caChainName) + "' already exists. Not adding!", "SCM");
			return (std::numeric_limits<uint16_t>::max)();
		}

		// now we make sure that we have the requested PoP present.
		if( !popExists(((MCP04::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caPoP) )
		{
			LOG_ERROR("The requested PoP '" + std::string(((MCP04::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caPoP) + "' for this SC isn't loaded. This means that this node can't participate in the new SC.", "SCM");
			LOG_ERROR("If you wish to proceed participating in this subchain, please load the requested module and resync the MC", "SCM");
			return (std::numeric_limits<uint16_t>::max)();
		}

		// adding subchain into our vector
		SubChainStruct tmp;
		MetaChain::getInstance().getStorageManager()->incMetaValue(MC_NEXT_CI, (uint16_t)0, &tmp.uint16ChainIdentifier);
		strncpy(tmp.caChainName, ((MCP04::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caChainName, MAX_CHAINNAME_LENGTH);
		strncpy(tmp.caPoP, ((MCP04::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caPoP, MAX_POP_NAME);
		tmp.ptr = m_mapProofFactories.at(((MCP04::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caPoP)();
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

	bool SubChainManager::popExists(std::string strName)
	{
		try
		{
			m_mapProofFactories.at(strName);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
}