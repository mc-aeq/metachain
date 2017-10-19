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
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include "SubChainStruct.h"
#include "../MCP04/ChainInterface.h"
#include "../MCP04/MC/mcBlock.h"

// forward decl
namespace MCP04 { class ChainInterface; };

namespace MCP02
{
#define CI_DEFAULT_MC_STRING	"MC"
#define MC_DEFAULT_POP "PoMC"

	class SubChainManager
	{
		private:
			// required to have serialization overrides
			friend class ::boost::serialization::access;

			// vector of subchains, map of proof of process creators
			std::map< unsigned short, SubChainStruct >						m_mapSubChains;
			std::map< std::string, MCP04::ChainInterface*(*)(void) >		m_mapProofFactories;
			
			template<class Archive>
			void															save(Archive &ar, const unsigned int version) const
			{
				// note: version is always stored last
				if (version == 1)
					ar & m_mapSubChains;
			}

			template<class Archive>
			void															load(Archive &ar, const unsigned int version)
			{
				if (version == 1)
				{
					ar & m_mapSubChains;

					// after loading the SC the instances are nullptr. create instances for the SCs
					for( auto &it : m_mapSubChains)
						it.second.ptr = m_mapProofFactories.at(it.second.caPoP)();
				}
			}
			BOOST_SERIALIZATION_SPLIT_MEMBER()

			// subchain creation process
			void															initPoP();
			bool															isSubChainAllowed(std::string strChainName);
			uint16_t														addSubChain(MCP04::MetaChain::mcBlock *block);

		public:
																			SubChainManager();
																			~SubChainManager();
			bool															init();
			bool															registerFactory(std::string strName, MCP04::ChainInterface*(*ptr)(void) );
			bool															popExists(std::string strName);

			// simple getter and setter
			uint16_t														getChainIdentifier(std::string strChainName);
			std::string														getChainIdentifier(uint16_t uint16tChainIdentifier);
			dbEngine*														getDBEngine(unsigned short usChainIdentifier);
	};	
}

BOOST_CLASS_VERSION(MCP02::SubChainManager, 1)
#endif