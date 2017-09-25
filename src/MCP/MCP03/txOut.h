#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_TXOUT_H__
#define __MCP03_TXOUT_H__ 1

#include <string>
#include <cstdint>
#include <limits>

namespace MCP03
{
	class txOut
	{
		private:
			uint64_t							m_uint64tValue;
			std::string							m_strPubKey;

		public:
												txOut();
												~txOut();		

			// simple getter & setter
			bool								isEmpty() { return (m_uint64tValue == std::numeric_limits<uint64_t>::max()); };
			uint64_t							getValue() { return m_uint64tValue; };
			std::string							toString();

			// operators
			bool								operator==(const txOut& ref);
			bool								operator!=(const txOut& ref);
	};
}
#endif