#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP02_SUBCHAINS_H__
#define __MCP02_SUBCHAINS_H__ 1

#include <vector>
#include <map>
#include <boost/serialization/access.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/version.hpp>
#include "SubChain.h"
#include "../MCP04/PoPInterface.h"
#include "../MCP03/MC/mcBlock.h"

// forward decl
namespace MCP04 { class PoPInterface; };
namespace MCP02 { class SubChain; };

namespace MCP02
{
#define CI_DEFAULT_INITIATOR	"4yGrKXjLu5utWhhWFb4ymCtxGQ7io11HUnXT5TPZbpGMrTrzXkZ7auvkJAHuBqbjhqHMHthSCE8QYC4P1mkNToip"
#define CI_DEFAULT_MC_STRING	"MC"
#define CI_DEFAULT_TCT_STRING	"TRUST"
#define CI_DEFAULT_MINE_STRING	"MINE"
#define CI_DEFAULT_MC_POP		"PoMC"
#define CI_DEFAULT_TCT_POP		"PoT"
#define CI_DEFAULT_MINE_POP		"PoS"

	class SubChainManager
	{
		private:
			// required to have serialization overrides
			friend class ::boost::serialization::access;

			// vector of subchains, map of proof of process creators
			std::map< unsigned short, SubChain >							m_mapSubChains;
			std::map< std::string, MCP04::PoPInterface*(*)(void) >			m_mapPoPFactories;
			std::map< std::string, MCP02::SubChain*(*)(void) >				m_mapSCFactories;
			
			template<class Archive>
			void															serialize(Archive &ar, const unsigned int version) 
			{
				// note: version is always stored last
				if (version == 1)
					ar & m_mapSubChains;
			}

			// subchain creation process
			void															initPoPFactories();
			void															initSCFactories();
			bool															isSubChainAllowed(std::string strChainName);
			uint16_t														addSubChain(MCP03::MetaChain::mcBlock *block);

		public:
																			SubChainManager();
																			~SubChainManager();
			bool															init();
			bool															registerPoPFactory(std::string strName, MCP04::PoPInterface*(*ptr)(void) );
			bool															registerSCFactory(std::string strName, MCP02::SubChain*(*ptr)(void));
			bool															popExists(std::string strName);

			void															printSCInfo();
			void															printPoPInfo();

			// simple getter and setter
			uint16_t														getChainIdentifier(std::string strChainName);
			std::string														getChainIdentifier(uint16_t uint16tChainIdentifier);
			dbEngine*														getDBEngine(unsigned short usChainIdentifier);
			MCP04::PoPInterface*											getPoPInstance(std::string strPoPFactory) { return m_mapPoPFactories[strPoPFactory](); };
	};	
}

BOOST_CLASS_VERSION(MCP02::SubChainManager, 1)
#endif