/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "SubChainManager.h"
#include <boost/archive/binary_oarchive.hpp>
#include "../../MetaChain.h"
#include "../../logger.h"
#include "../MCP01/base58.h"
#include "../MCP03/Transaction.h"
#include "../MCP03/Block.h"
#include "../MCP03/MC/mcActions.h"
#include "../MCP03/MC/mcTransaction.h"
#include "../MCP03/MC/mcBlock.h"
#include "../MCP04/PoMC.h"
#include "../MCP04/PoS.h"
#include "../MCP04/PoT.h"
#include "../../tinyformat.h"

namespace MCP02
{
	SubChainManager::SubChainManager()
	{
		initPoP();
	}

	SubChainManager::~SubChainManager()
	{
	}

	bool SubChainManager::init()
	{
		// since we can't control the metachain without the corresponding authorative TCT chain, we need to manually create the genesis block of the MC and the TCT.
		/*
		* MC genesis block
		*/
		{
			LOG("Generating MetaChain genesis block", "SCM");

			std::shared_ptr< MCP03::MetaChain::mcTransaction > txGenesis = std::make_shared<MCP03::MetaChain::mcTransaction>();
			txGenesis->uint16tVersion = 1;
			txGenesis->txIn.init(MCP03::MetaChain::mcTxIn::ACTION::CREATE_SUBCHAIN);
			memcpy(((MCP03::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->caChainName, CI_DEFAULT_MC_STRING, sizeof(CI_DEFAULT_MC_STRING));
			memcpy(((MCP03::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->caPoP, CI_DEFAULT_MC_POP, sizeof(CI_DEFAULT_MC_POP));
			((MCP03::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->uint64tMaxCoins = 0;

			MCP03::MetaChain::mcBlock genesis;
			genesis.nTime = 1506343480;
			genesis.uint16tVersion = 1;
			MCP01::base58::decode(CI_DEFAULT_INITIATOR, genesis.initiatorPubKey);
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
				LOG_DEBUG("WARNING!!! MetaChain Genesis Block Hash doesn't match. Not Exiting", "SCM");
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
		}

		/*
		* TCT genesis block
		*/
		{
			LOG("Generating TCT genesis block", "SCM");

			std::shared_ptr< MCP03::MetaChain::mcTransaction > txGenesis = std::make_shared<MCP03::MetaChain::mcTransaction>();
			txGenesis->uint16tVersion = 1;
			txGenesis->txIn.init(MCP03::MetaChain::mcTxIn::ACTION::CREATE_SUBCHAIN);
			memcpy(((MCP03::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->caChainName, CI_DEFAULT_TCT_STRING, sizeof(CI_DEFAULT_TCT_STRING));
			memcpy(((MCP03::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->caPoP, CI_DEFAULT_TCT_POP, sizeof(CI_DEFAULT_TCT_POP));
			((MCP03::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->uint64tMaxCoins = 8978156324;

			MCP03::MetaChain::mcBlock genesis;
			genesis.nTime = 1506343480;
			genesis.uint16tVersion = 1;
			MCP01::base58::decode(CI_DEFAULT_INITIATOR, genesis.initiatorPubKey);
			genesis.pTransaction = std::move(txGenesis);
			genesis.hashPrevBlock.SetNull();
			genesis.calcAll();

			LOG("TCT genesis block hash is: " + genesis.hash.ToString(), "SCM");

#ifdef _DEBUG
			LOG_DEBUG("TCT Genesis block contents: " + genesis.toString(), "SCM");
#endif

			// checking consistency of our MC genesis block
			uint8_t genesisRawBuffer[32] = { 0xdc, 0x03, 0x4e, 0x58, 0xf0, 0xaa, 0xcf, 0x47, 0x0d, 0x96, 0x73, 0x0d, 0x90, 0xb2, 0x69, 0x1f, 0x41, 0x87, 0x86, 0x5b, 0x0f, 0x75, 0x93, 0x6a, 0x49, 0xc6, 0xbf, 0x53, 0xb5, 0x94, 0xfe, 0x70 };
			const uint256 validMCGenesisHash(&genesisRawBuffer[0], 32);
			if (genesis.hash != validMCGenesisHash)
			{
#ifdef _DEBUG
				LOG_DEBUG("WARNING!!! TCT Genesis Block Hash doesn't match. Not Exiting", "SCM");
#else
				LOG_ERROR("The TCT genesis block hash doesn't match the precomputed safe hash!", "SCM");
				return false;
#endif
			}

			// adding the MC as subchain
			LOG("Loading TCT genesis block", "SCM");
			if (addSubChain(&genesis) != 1)
			{
				LOG_ERROR("Adding of the TCT identifier didn't work as expected.", "SCM");
				return false;
			}
		}


		// since MINE will be deployed earlier than the TCT, we also have to manually create the genesis block of the MINE. Future SubChains will use regular TCT control sequences to generate MetaChain genesis blocks
		/*
		* MINE genesis block
		*/
		{
			LOG("Generating MINE genesis block", "SCM");

			std::shared_ptr< MCP03::MetaChain::mcTransaction > txGenesis = std::make_shared<MCP03::MetaChain::mcTransaction>();
			txGenesis->uint16tVersion = 1;
			txGenesis->txIn.init(MCP03::MetaChain::mcTxIn::ACTION::CREATE_SUBCHAIN);
			memcpy(((MCP03::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->caChainName, CI_DEFAULT_MINE_STRING, sizeof(CI_DEFAULT_MINE_STRING));
			memcpy(((MCP03::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->caPoP, CI_DEFAULT_MINE_POP, sizeof(CI_DEFAULT_MINE_POP));
			((MCP03::MetaChain::createSubchain*)txGenesis->txIn.pPayload)->uint64tMaxCoins = 111000000;

			MCP03::MetaChain::mcBlock genesis;
			genesis.nTime = 1506343480;
			genesis.uint16tVersion = 1;
			MCP01::base58::decode(CI_DEFAULT_INITIATOR, genesis.initiatorPubKey);
			genesis.pTransaction = std::move(txGenesis);
			genesis.hashPrevBlock.SetNull();
			genesis.calcAll();

			LOG("MINE genesis block hash is: " + genesis.hash.ToString(), "SCM");

#ifdef _DEBUG
			LOG_DEBUG("MINE Genesis block contents: " + genesis.toString(), "SCM");
#endif

			// checking consistency of our MC genesis block
			uint8_t genesisRawBuffer[32] = { 0xdc, 0x03, 0x4e, 0x58, 0xf0, 0xaa, 0xcf, 0x47, 0x0d, 0x96, 0x73, 0x0d, 0x90, 0xb2, 0x69, 0x1f, 0x41, 0x87, 0x86, 0x5b, 0x0f, 0x75, 0x93, 0x6a, 0x49, 0xc6, 0xbf, 0x53, 0xb5, 0x94, 0xfe, 0x70 };
			const uint256 validMCGenesisHash(&genesisRawBuffer[0], 32);
			if (genesis.hash != validMCGenesisHash)
			{
#ifdef _DEBUG
				LOG_DEBUG("WARNING!!! MINE Genesis Block Hash doesn't match. Not Exiting", "SCM");
#else
				LOG_ERROR("The MINE genesis block hash doesn't match the precomputed safe hash!", "SCM");
				return false;
#endif
			}

			// adding the MC as subchain
			LOG("Loading MINE genesis block", "SCM");
			if (addSubChain(&genesis) != 2)
			{
				LOG_ERROR("Adding of the MINE identifier didn't work as expected.", "SCM");
				return false;
			}
		}
		
		// everything smooth
		return true;
	}

	void SubChainManager::initPoP()
	{		
		// load the POPs that are defined in the ini
		// todo: change this to module (.dll) loading when POPs are encapsulated in modules.
		// functionality would be: <modulename> load -> instance function

		for (auto &it : MetaChain::getInstance().vecPOPs)
		{
#ifdef _DEBUG
			LOG_DEBUG("Loading PoP: " + it, "SCM");
#endif
			if (boost::trim_copy(it) == "*")
			{
				MCP04::PoMC::registerFactory(this);
				MCP04::PoS::registerFactory(this);
				MCP04::PoT::registerFactory(this);
			}
			else if(boost::trim_copy(it) == "PoMC")
				MCP04::PoMC::registerFactory(this);
			else if (boost::trim_copy(it) == "PoS")
				MCP04::PoS::registerFactory(this);
			else if (boost::trim_copy(it) == "PoT")
				MCP04::PoT::registerFactory(this);
		}

	}

	bool SubChainManager::isSubChainAllowed(std::string strChainName)
	{
		// first check blacklist
		for (auto &it : MetaChain::getInstance().vecSC_Blacklist)
		{
#ifdef _DEBUG
			LOG_DEBUG("Checking Subchain Blacklist Entry: " + boost::trim_copy(it), "SCM");
#endif
			// wildcard blacklist means we don't allow anything
			if (boost::trim_copy(it) == "*")
				return false;

			// the chain name was specified in the blacklist
			if (boost::trim_copy(it) == strChainName)
				return false;
		}

		// then check whitelist
		for (auto &it : MetaChain::getInstance().vecSC_Whitelist)
		{
#ifdef _DEBUG
			LOG_DEBUG("Checking Subchain Whitelist Entry: " + it, "SCM");
#endif
			// wildcard whitelist means we allow everything
			if (boost::trim_copy(it) == "*")
				return true;

			// the chain name was specified in the whitelist
			if (boost::trim_copy(it) == strChainName)
				return true;
		}

		// if we reached this point it means we didnt match any white or blacklist entries, therefore we're rejecting this subchain per default
		return false;
	}

	// returns the uint16_t from the newly added SubChain
	uint16_t SubChainManager::addSubChain(MCP03::MetaChain::mcBlock *block)
	{
		// checking if this is really a genesis request block
		if( block->pTransaction->txIn.eAction != MCP03::MetaChain::mcTxIn::ACTION::CREATE_SUBCHAIN )
		{
			LOG_ERROR("A non-genesis creation block was passed into the genesis creation function. Not executing!", "SCM");
			return (std::numeric_limits<uint16_t>::max)();
		}

		// check if this subchain is allowed in the config
		if (!isSubChainAllowed(((MCP03::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caChainName))
		{
			LOG_ERROR("SC was not added since the configuration (subchain_whitelist, subchain_blacklist) prohibits this SC.", "SCM");
			return (std::numeric_limits<uint16_t>::max)();
		}

		// check whether this chain name already exists. If so, we don't add it
		if (getChainIdentifier(((MCP03::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caChainName) != (std::numeric_limits<uint16_t>::max)())
		{
			LOG_ERROR("SC with the same name '" + std::string(((MCP03::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caChainName) + "' already exists. Not adding!", "SCM");
			return (std::numeric_limits<uint16_t>::max)();
		}

		// now we make sure that we have the requested PoP present.
		if( !popExists(((MCP03::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caPoP) )
		{
			LOG_ERROR("The requested PoP '" + std::string(((MCP03::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caPoP) + "' for this SC isn't loaded. This means that this node can't participate in the new SC.", "SCM");
			LOG_ERROR("If you wish to proceed participating in this subchain, please load the requested module and resync the MC", "SCM");
			return (std::numeric_limits<uint16_t>::max)();
		}

		// multithreading locking
		LOCK(MetaChain::getInstance().getStorageManager()->csSubChainManager);

		// adding subchain into our vector
		SubChainStruct tmp;
		MetaChain::getInstance().getStorageManager()->incMetaValue(MC_NEXT_CI, (uint16_t)0, &tmp.uint16ChainIdentifier);
		strncpy(tmp.caChainName, ((MCP03::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caChainName, MAX_CHAINNAME_LENGTH);
		strncpy(tmp.caPoP, ((MCP03::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caPoP, MAX_POP_NAME);
		tmp.ptr = m_mapProofFactories.at(((MCP03::MetaChain::createSubchain*)block->pTransaction->txIn.pPayload)->caPoP)();
		tmp.db = MetaChain::getInstance().getStorageManager()->createDBEngine(tmp.uint16ChainIdentifier);
		m_mapSubChains[ tmp.uint16ChainIdentifier ] = tmp;

		// register this instance at the storage manager
		if( !MetaChain::getInstance().getStorageManager()->registerSC(tmp.uint16ChainIdentifier) )
		{
			LOG_ERROR("Couldn't register this SC at the SM.", "SCM");
			return (std::numeric_limits<uint16_t>::max)();
		}

		// store this block into our MC
		MetaChain::getInstance().getStorageManager()->writeRaw(MC_CHAIN_IDENTIFIER, sizeof(MCP03::MetaChain::mcBlock), block);

		return tmp.uint16ChainIdentifier;
	}

	uint16_t SubChainManager::getChainIdentifier(std::string strChainName)
	{
		char cBuffer[MAX_CHAINNAME_LENGTH];
		strncpy(cBuffer, strChainName.c_str(), MAX_CHAINNAME_LENGTH);

		// multithreading locking
		LOCK(MetaChain::getInstance().getStorageManager()->csSubChainManager);

		for (auto &it = m_mapSubChains.begin(); it != m_mapSubChains.end(); it++)
		{
			if (memcmp(cBuffer, it->second.caChainName, MAX_CHAINNAME_LENGTH) == 0)
				return it->second.uint16ChainIdentifier;
		}
		return (std::numeric_limits<uint16_t>::max)();
	}


	std::string	SubChainManager::getChainIdentifier(uint16_t uint16tChainIdentifier)
	{
		// multithreading locking
		LOCK(MetaChain::getInstance().getStorageManager()->csSubChainManager);

		if (m_mapSubChains.count(uint16tChainIdentifier) == 1)
			return m_mapSubChains[uint16tChainIdentifier].caChainName;
		else
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

	dbEngine* SubChainManager::getDBEngine(unsigned short usChainIdentifier)
	{
		if (m_mapSubChains.count(usChainIdentifier) == 1)
			return m_mapSubChains[usChainIdentifier].db;
		else
			return nullptr;
	}

	void SubChainManager::printSCInfo()
	{
		LOG("Number of loaded SubChains: " + std::to_string(m_mapSubChains.size()), "SCM");
		for (auto &it : m_mapSubChains)
		{
			LOG(strprintf("SC #%u - %s", it.first+1, it.second.caChainName), "SCM");
			LOG(strprintf("   PoP: %s", it.second.caPoP), "SCM");
			LOG(strprintf("   Height: %u", it.second.db->get("last_block", (unsigned int)0)), "SCM");
#ifdef _DEBUG
			LOG_DEBUG(strprintf("   Instance Address: %p", it.second.ptr), "SCM");
#endif
		}
	}

	void SubChainManager::printPoPInfo()
	{
		LOG("Number of loaded PoPs: " + std::to_string(m_mapProofFactories.size()), "SCM");
		for (auto &it : m_mapProofFactories)
		{
			LOG(strprintf("PoP: %s", it.first), "SCM");
#ifdef _DEBUG
			LOG_DEBUG(strprintf("   Factory Address: %p", it.second), "SCM");
#endif
		}
	}
}