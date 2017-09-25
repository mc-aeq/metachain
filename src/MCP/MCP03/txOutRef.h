#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP03_TXOUTREF_H__
#define __MCP03_TXOUTREF_H__ 1

#include <string>
#include <cstdint>
#include <limits>
#include "../../uint256.h"

namespace MCP03
{
	class txOutRef
	{
		private:
			uint256							m_Hash;
			uint16_t						m_uint16tPos;
	
		public:
											txOutRef();
											~txOutRef();

			// simple getter and setter
			bool							isEmpty() { return (m_Hash.IsNull() && m_uint16tPos == std::numeric_limits<uint16_t>::max()); }
			std::string						toString();

			// operators
			bool operator<(const txOutRef& ref);
			bool operator==(const txOutRef& ref);
			bool operator!=(const txOutRef& ref);
	};
}

#endif