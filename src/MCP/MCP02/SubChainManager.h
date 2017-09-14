#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP02_SUBCHAINS_H__
#define __MCP02_SUBCHAINS_H__ 1

#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include "SubChainStruct.h"
#include "../../cCriticalSection.h"

namespace MCP02
{
#define CI_DEFAULT_MC_STRING	"MC"
#define CI_DEFAULT_MC_UINT16	0

	class SubChainManager
	{
		private:
			// required to have serialization overrides
			friend class ::boost::serialization::access;

			// vector of subchains
			std::vector< SubChainStruct >			m_vecSubChains;
			
			template<class Archive>
			void save(Archive &ar, const unsigned int version) const
			{
				// note: version is always stored last
				if (version == 1)
					ar & m_vecSubChains;
			}

			template<class Archive>
			void load(Archive &ar, const unsigned int version)
			{
				if (version == 1)
					ar & m_vecSubChains;
			}
			BOOST_SERIALIZATION_SPLIT_MEMBER()

		public:
													SubChainManager();
													~SubChainManager();
			void									init();
			void									addSubChain(std::string strChainName, uint16_t uint16ChainIdentifier);
			uint16_t								getChainIdentifier(std::string strChainName);
			std::string								getChainIdentifier(uint16_t uint16tChainIdentifier);

			// critical locking
			cCriticalSection						csAccess;
	};	
}

BOOST_CLASS_VERSION(MCP02::SubChainManager, 1)
#endif