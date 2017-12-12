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

#ifndef __MCP06_SCRIPT_FUNCTIONS_H__
#define __MCP06_SCRIPT_FUNCTIONS_H__

#include <vector>
#include "script.h"
#include "scriptError.h"
#include "../../uint256.h"
#include "../MCP03/crTransaction.h"
#include "../../logger.h"

namespace MCP06
{
	// typedef for readability
	typedef std::vector<unsigned char> valtype;

	// success function that updates a pointer to scriptError and returns true
	inline bool set_success(scriptError* ret)
	{
		if (ret)
			*ret = scriptError::OK;
		return true;
	};

	// error function that updates a pointer to scriptError, prints the error string when build in debug mode and returns false
	inline bool set_error(scriptError* ret, const scriptError serror)
	{
		if (ret)
		{
			*ret = serror;
#ifdef _DEBUG
			LOG_DEBUG("Error in Script: " + scriptErrorString(serror), "scriptFunctions");
#endif
		}
		return false;
	};

	// checks the passed vector if it's 0 or negative 0 - if any other bit is set, it returns true
	bool CastToBool(const valtype& vch);

	// check if the signature has the right encoding
	bool CheckSignatureEncoding(const std::vector<unsigned char> &vchSig, unsigned int flags, scriptError* serror);

	// check if the signature is in low DER format
	bool IsLowDERSignature(const valtype &vchSig, scriptError* serror);

	/**
	* A canonical signature exists of: <30> <total len> <02> <len R> <R> <02> <len S> <S> <hashtype>
	* Where R and S are not negative (their first byte has its highest bit not set), and not
	* excessively padded (do not start with a 0 byte, unless an otherwise negative number follows,
	* in which case a single 0 byte is necessary and even required).
	*
	* See https://bitcointalk.org/index.php?topic=8392.msg127623#msg127623
	*
	* This function is consensus-critical since BIP66.
	*/
	bool IsValidSignatureEncoding(const std::vector<unsigned char> &sig);

	// check if the pub key is compressed or uncompressed
	bool IsCompressedOrUncompressedPubKey(const valtype &vchPubKey);

	// check if the pub key is compressed (true when compressed)
	bool IsCompressedPubKey(const valtype &vchPubKey);

	// check if the hash type is defined in the signature
	bool IsDefinedHashtypeSignature(const valtype &vchSig);

	// check if the encoding of the pubkey is consistent with the corresponding flags
	bool CheckPubKeyEncoding(const valtype &vchPubKey, unsigned int flags, scriptError* serror);

	// check minimal push
	bool CheckMinimalPush(const valtype& data, opcodetype opcode);

	// calculate the signature hash
	uint256 signatureHash(const CScript& scriptCode, const MCP03::crTransaction& txTo, unsigned int nIn, int nHashType);
}

#endif