#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP02_SUBCHAIN_H__
#define __MCP02_SUBCHAIN_H__ 1

#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/split_member.hpp>
#include "../../defines.h"
#include "../MCP03/Block.h"
#include "../MCP04/PoPInterface.h"
#include "SubChainManager.h"
#include "../../io/db/dbEngine.h"
#include "../../io/StorageManager.h"

// forward decl
namespace MCP04 { class PoPInterface; };
class StorageManager;

namespace MCP02
{
	class SubChain
	{
		friend class ::boost::serialization::access;

		private:
			// CC function
			void											makeDeepCopy(SubChain & obj);

			// pointers for faster processing. don't need to be released, initialized in init function
			StorageManager									*m_pStorageManager;

			// serialization
			template<class Archive>
			void											save(Archive &ar, const unsigned int version) const
			{
				if (version == 1)
					ar & m_bGenesis & m_uint32GenesisTimestamp & m_uint16ChainIdentifier & m_caChainName & m_caSubChainClassName & m_caPoP & m_mapParams;
			}

			template<class Archive>
			void											load(Archive &ar, const unsigned int version)
			{
				if (version == 1)
				{
					ar & m_bGenesis & m_uint32GenesisTimestamp & m_uint16ChainIdentifier & m_caChainName & m_caSubChainClassName & m_caPoP & m_mapParams;

					m_pPoP = MetaChain::getInstance().getStorageManager()->getSubChainManager()->getPoPInstance(m_caPoP);
					m_pDB = MetaChain::getInstance().getStorageManager()->createDBEngine(m_uint16ChainIdentifier);

					postInit();
				}
			}

			BOOST_SERIALIZATION_SPLIT_MEMBER()

		protected:
			bool											m_bGenesis; // only set to false before a genesis block was created.
			uint16_t										m_uint16ChainIdentifier;
			uint32_t										m_uint32GenesisTimestamp;
			char											m_caChainName[MAX_CHAINNAME_LENGTH];
			char											m_caSubChainClassName[MAX_SUBCHAIN_CLASSNAME_LENGTH];
			char											m_caPoP[MAX_POP_NAME];
			std::map< std::string, std::string >			m_mapParams;
			MCP04::PoPInterface								*m_pPoP;
			dbEngine										*m_pDB;

			// this function gets called after the init Function gets called.
			// private variables or precalculations can be done in this function. the function gets called after first initialization and after each serialization loading
			virtual void									postInit() = 0;

			// block handling
			bool											checkBlock(MCP03::Block *Block);
			void											saveBlock(MCP03::Block *Block);

		public:
															SubChain();
															~SubChain();
															SubChain(SubChain& obj);
			SubChain&										operator=(SubChain& obj);

			// initialization functions
			uint16_t										init(char caChainName[MAX_CHAINNAME_LENGTH], char caSubChainClassName[MAX_SUBCHAIN_CLASSNAME_LENGTH], char m_caPoP[MAX_POP_NAME], std::map< std::string, std::string > mapParams );
			virtual bool									initGenesis( uint8_t initiatorPubKey[64], uint64_t uint64tGenesisCoins ) = 0;

			// block handling
			virtual bool									processBlock(MCP03::Block* Block) = 0;

			// simple setter & getter
			std::string										getChainName() { return m_caChainName; };
			uint16_t										getChainIdentifier() { return m_uint16ChainIdentifier; };
			dbEngine*										getDBEngine() { return m_pDB; };
			unsigned int									getHeight() { return m_pDB->get("last_block", (unsigned int)0); };
			virtual std::string								toString();
	};
}

BOOST_CLASS_VERSION(MCP02::SubChain, 1)
#endif