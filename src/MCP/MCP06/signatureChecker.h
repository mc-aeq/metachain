#pragma once

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __MCP06_SIGNATURE_CHECKER_H__
#define __MCP06_SIGNATURE_CHECKER_H__

#include <vector>
#include "scriptFunctions.h"
#include "script.h"
#include "scriptError.h"
#include "../../uint256.h"
#include "../MCP01/Account.h"
#include "../MCP03/crTransaction.h"

namespace MCP06
{
	/** Signature hash types/flags */
	enum SIGHASH
	{
		HASH_ALL = 1,
		HASH_NONE = 2,
		HASH_SINGLE = 3,
		HASH_ANYONECANPAY = 0x80
	};

	class signatureChecker
	{
		private:
			const						MCP03::crTransaction* txTo;
			unsigned int				nIn;

		protected:
			virtual bool				verifySignature(const std::vector<unsigned char>& vchSig, MCP01::Account& pubAccount, const uint256& sighash);

		public:
										signatureChecker() {};
										signatureChecker(const MCP03::crTransaction* txToIn, unsigned int nInIn) : txTo(txToIn), nIn(nInIn) {}
			bool						checkSignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode);
			bool						checkLockTime(const CScriptNum& nLockTime);
	};
}
#endif