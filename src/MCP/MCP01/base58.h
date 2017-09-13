#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

// MCP01 Account functions: Wallet Address Generation, Checking

#ifndef __MCP01_BASE58_H__
#define __MCP01_BASE58_H__

#include <string>
#include <vector>

namespace MCP01
{
	/** All alphanumeric characters except for "0", "I", "O", and "l" */
	static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

	class base58
	{
		private:
												base58() {}; // don't allow copies or instances of this class

		public:
			static std::string					encode(const uint8_t *buffer, unsigned int uiLength);
			static bool							decode(const std::string input, uint8_t *output);
	};
}

#endif