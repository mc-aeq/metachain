// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "signatureChecker.h"
#include "scriptInterpreter.h"
#include "scriptError.h"
#include "../MCP03/txIn.h"

namespace MCP06
{
	bool signatureChecker::verifySignature(const std::vector<unsigned char>& vchSig, MCP01::Account& pubAccount, const uint256& sighash)
	{
		return pubAccount.verify(sighash, vchSig);
	}

	bool signatureChecker::checkSignature(const std::vector<unsigned char>& vchSigIn, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode)
	{
		MCP01::Account pubkey(vchPubKey);

		// todo: add check whether pubkey is valid or not
		/*
		if (!pubkey.IsValid())
			return false;
			*/

		// Hash type is one byte tacked on to the end of the signature
		std::vector<unsigned char> vchSig(vchSigIn);
		if (vchSig.empty())
			return false;
		int nHashType = vchSig.back();
		vchSig.pop_back();

		uint256 sighash = signatureHash(scriptCode, *txTo, nIn, nHashType);

		if (!verifySignature(vchSig, pubkey, sighash))
			return false;

		return true;
	}

	bool signatureChecker::checkLockTime(const CScriptNum& nLockTime)
	{
		// There are two kinds of nLockTime: lock-by-blockheight
		// and lock-by-blocktime, distinguished by whether
		// nLockTime < LOCKTIME_THRESHOLD.
		//
		// We want to compare apples to apples, so fail the script
		// unless the type of nLockTime being tested is the same as
		// the nLockTime in the transaction.
		if (!(
			(txTo->uint32tLockTime <  LOCKTIME_THRESHOLD && nLockTime <  LOCKTIME_THRESHOLD) ||
			(txTo->uint32tLockTime >= LOCKTIME_THRESHOLD && nLockTime >= LOCKTIME_THRESHOLD)
			))
			return false;

		// Now that we know we're comparing apples-to-apples, the
		// comparison is a simple numeric one.
		if (nLockTime >(int64_t)txTo->uint32tLockTime)
			return false;

		return true;
	}
}