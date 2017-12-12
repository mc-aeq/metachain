// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "scriptFunctions.h"
#include "scriptInterpreter.h"
#include "../MCP03/crTransaction.h"

namespace MCP06
{
	bool CastToBool(const valtype& vch)
	{
		for (unsigned int i = 0; i < vch.size(); i++)
		{
			if (vch[i] != 0)
			{
				// Can be negative zero
				if (i == vch.size() - 1 && vch[i] == 0x80)
					return false;
				return true;
			}
		}
		return false;
	}

	bool CheckSignatureEncoding(const std::vector<unsigned char> &vchSig, unsigned int flags, scriptError* serror)
	{
		// Empty signature. Not strictly DER encoded, but allowed to provide a
		// compact way to provide an invalid signature for use with CHECK(MULTI)SIG
		if (vchSig.size() == 0) {
			return true;
		}
		if ((flags & (SCRIPT_VERIFY::DERSIG | SCRIPT_VERIFY::LOW_S | SCRIPT_VERIFY::STRICTENC)) != 0 && !IsValidSignatureEncoding(vchSig)) {
			return set_error(serror, scriptError::SIG_DER);
		}
		else if ((flags & SCRIPT_VERIFY::LOW_S) != 0 && !IsLowDERSignature(vchSig, serror)) {
			// serror is set
			return false;
		}
		else if ((flags & SCRIPT_VERIFY::STRICTENC) != 0 && !IsDefinedHashtypeSignature(vchSig)) {
			return set_error(serror, scriptError::SIG_HASHTYPE);
		}
		return true;
	}

	bool IsLowDERSignature(const valtype &vchSig, scriptError* serror)
	{
		if (!IsValidSignatureEncoding(vchSig))
			return set_error(serror, scriptError::SIG_DER);

		// todo: check low DER signature
		/*
		std::vector<unsigned char> vchSigCopy(vchSig.begin(), vchSig.begin() + vchSig.size() - 1);
		if (!CPubKey::CheckLowS(vchSigCopy))
		return set_error(serror,scriptError::SIG_HIGH_S);
		*/
		return true;
	}

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
	bool IsValidSignatureEncoding(const std::vector<unsigned char> &sig)
	{
		// Format: 0x30 [total-length] 0x02 [R-length] [R] 0x02 [S-length] [S] [sighash]
		// * total-length: 1-byte length descriptor of everything that follows,
		//   excluding the sighash byte.
		// * R-length: 1-byte length descriptor of the R value that follows.
		// * R: arbitrary-length big-endian encoded R value. It must use the shortest
		//   possible encoding for a positive integers (which means no null bytes at
		//   the start, except a single one when the next byte has its highest bit set).
		// * S-length: 1-byte length descriptor of the S value that follows.
		// * S: arbitrary-length big-endian encoded S value. The same rules apply.
		// * sighash: 1-byte value indicating what data is hashed (not part of the DER
		//   signature)

		// Minimum and maximum size constraints.
		if (sig.size() < 9) return false;
		if (sig.size() > 73) return false;

		// A signature is of type 0x30 (compound).
		if (sig[0] != 0x30) return false;

		// Make sure the length covers the entire signature.
		if (sig[1] != sig.size() - 3) return false;

		// Extract the length of the R element.
		unsigned int lenR = sig[3];

		// Make sure the length of the S element is still inside the signature.
		if (5 + lenR >= sig.size()) return false;

		// Extract the length of the S element.
		unsigned int lenS = sig[5 + lenR];

		// Verify that the length of the signature matches the sum of the length
		// of the elements.
		if ((size_t)(lenR + lenS + 7) != sig.size()) return false;

		// Check whether the R element is an integer.
		if (sig[2] != 0x02) return false;

		// Zero-length integers are not allowed for R.
		if (lenR == 0) return false;

		// Negative numbers are not allowed for R.
		if (sig[4] & 0x80) return false;

		// Null bytes at the start of R are not allowed, unless R would
		// otherwise be interpreted as a negative number.
		if (lenR > 1 && (sig[4] == 0x00) && !(sig[5] & 0x80)) return false;

		// Check whether the S element is an integer.
		if (sig[lenR + 4] != 0x02) return false;

		// Zero-length integers are not allowed for S.
		if (lenS == 0) return false;

		// Negative numbers are not allowed for S.
		if (sig[lenR + 6] & 0x80) return false;

		// Null bytes at the start of S are not allowed, unless S would otherwise be
		// interpreted as a negative number.
		if (lenS > 1 && (sig[lenR + 6] == 0x00) && !(sig[lenR + 7] & 0x80)) return false;

		return true;
	}

	bool IsCompressedOrUncompressedPubKey(const valtype &vchPubKey)
	{
		if (vchPubKey.size() < 33) {
			//  Non-canonical public key: too short
			return false;
		}
		if (vchPubKey[0] == 0x04) {
			if (vchPubKey.size() != 65) {
				//  Non-canonical public key: invalid length for uncompressed key
				return false;
			}
		}
		else if (vchPubKey[0] == 0x02 || vchPubKey[0] == 0x03) {
			if (vchPubKey.size() != 33) {
				//  Non-canonical public key: invalid length for compressed key
				return false;
			}
		}
		else {
			//  Non-canonical public key: neither compressed nor uncompressed
			return false;
		}
		return true;
	}

	bool IsCompressedPubKey(const valtype &vchPubKey)
	{
		if (vchPubKey.size() != 33) {
			//  Non-canonical public key: invalid length for compressed key
			return false;
		}
		if (vchPubKey[0] != 0x02 && vchPubKey[0] != 0x03) {
			//  Non-canonical public key: invalid prefix for compressed key
			return false;
		}
		return true;
	}

	bool IsDefinedHashtypeSignature(const valtype &vchSig)
	{
		if (vchSig.size() == 0)
			return false;

		unsigned char nHashType = vchSig[vchSig.size() - 1] & (~(SIGHASH::HASH_ANYONECANPAY));
		if (nHashType < SIGHASH::HASH_ALL || nHashType > SIGHASH::HASH_SINGLE)
			return false;

		return true;
	}

	bool CheckPubKeyEncoding(const valtype &vchPubKey, unsigned int flags, scriptError* serror)
	{
		if ((flags & SCRIPT_VERIFY::STRICTENC) != 0 && !IsCompressedOrUncompressedPubKey(vchPubKey))
			return set_error(serror, scriptError::PUBKEYTYPE);

		return true;
	}

	bool CheckMinimalPush(const valtype& data, opcodetype opcode)
	{
		if (data.size() == 0) {
			// Could have used OP_0.
			return opcode == OP_0;
		}
		else if (data.size() == 1 && data[0] >= 1 && data[0] <= 16) {
			// Could have used OP_1 .. OP_16.
			return opcode == OP_1 + (data[0] - 1);
		}
		else if (data.size() == 1 && data[0] == 0x81) {
			// Could have used OP_1NEGATE.
			return opcode == OP_1NEGATE;
		}
		else if (data.size() <= 75) {
			// Could have used a direct push (opcode indicating number of bytes pushed + those bytes).
			return opcode == data.size();
		}
		else if (data.size() <= 255) {
			// Could have used OP_PUSHDATA.
			return opcode == OP_PUSHDATA1;
		}
		else if (data.size() <= 65535) {
			// Could have used OP_PUSHDATA2.
			return opcode == OP_PUSHDATA2;
		}
		return true;
	}

	uint256 signatureHash(const CScript& scriptCode, const MCP03::crTransaction& txTo, unsigned int nIn, int nHashType )
	{
		MCP03::crTransaction tmpTransaction = txTo;

		static const uint256 one(uint256S("0000000000000000000000000000000000000000000000000000000000000001"));
		if (nIn >= txTo.vecIn.size()) {
			//  nIn out of range
			return one;
		}

		// Check for invalid use of SIGHASH_SINGLE
		if ((nHashType & 0x1f) == SIGHASH::HASH_SINGLE)
		{
			if (nIn >= txTo.vecOut.size())
				//  nOut out of range
				return one;
		}

		tmpTransaction.calcHash();
		return tmpTransaction.hash;
	}
}