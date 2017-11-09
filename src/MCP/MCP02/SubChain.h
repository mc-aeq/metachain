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
#include "../MCP04/PoPInterface.h"
#include "SubChainManager.h"
#include "../../io/db/dbEngine.h"

// forward decl
namespace MCP04 { class PoPInterface; };

namespace MCP02
{
	class SubChain
	{
		// todo: add workflows to the base subchain class
		friend class ::boost::serialization::access;

		private:
			uint16_t										m_uint16ChainIdentifier;
			char											m_caChainName[MAX_CHAINNAME_LENGTH];
			char											m_caSubChainClassName[MAX_SUBCHAIN_CLASSNAME_LENGTH];
			char											m_caPoP[MAX_POP_NAME];
			std::map< std::string, std::string >			m_mapParams;
			MCP04::PoPInterface								*m_pPoP;
			dbEngine										*m_pDB;

			// CC function
			void											makeDeepCopy(SubChain & obj);

			// serialization
			template<class Archive>
			void											save(Archive &ar, const unsigned int version) const
			{
				if (version == 1)
					ar & m_uint16ChainIdentifier & m_caChainName & m_caSubChainClassName & m_caPoP & m_mapParams;
			}

			template<class Archive>
			void											load(Archive &ar, const unsigned int version)
			{
				if (version == 1)
				{
					ar & m_uint16ChainIdentifier & m_caChainName & m_caSubChainClassName & m_caPoP & m_mapParams;

					m_pPoP = MetaChain::getInstance().getStorageManager()->getSubChainManager()->getPoPInstance(m_caPoP);
					m_pDB = MetaChain::getInstance().getStorageManager()->createDBEngine(m_uint16ChainIdentifier);
				}
			}

			BOOST_SERIALIZATION_SPLIT_MEMBER()

		public:
															SubChain();
															~SubChain();
															SubChain(SubChain& obj);
			SubChain&										operator=(SubChain& obj);

			// initialization functions
			uint16_t										init(char caChainName[MAX_CHAINNAME_LENGTH], char caSubChainClassName[MAX_SUBCHAIN_CLASSNAME_LENGTH], char m_caPoP[MAX_POP_NAME], std::map< std::string, std::string > mapParams );
			//virtual bool									initGenesis(uint8_t initiatorPubKey[64], float) = 0;

			// simple setter & getter
			std::string										getChainName() { return m_caChainName; };
			uint16_t										getChainIdentifier() { return m_uint16ChainIdentifier; };
			dbEngine*										getDBEngine() { return m_pDB; };
			std::string										toString();
	};
}

BOOST_CLASS_VERSION(MCP02::SubChain, 1)
#endif