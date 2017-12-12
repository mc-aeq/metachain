// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "scriptError.h"
#include <string>

namespace MCP06
{
	std::string scriptErrorString(scriptError err)
	{
		switch (err)
		{
			case scriptError::OK:
				return "No error";
			case scriptError::EVAL_FALSE:
				return "Script evaluated without error but finished with a false/empty top stack element";
			case scriptError::VERIFY:
				return "Script failed an OP_VERIFY operation";
			case scriptError::EQUALVERIFY:
				return "Script failed an OP_EQUALVERIFY operation";
			case scriptError::CHECKMULTISIGVERIFY:
				return "Script failed an OP_CHECKMULTISIGVERIFY operation";
			case scriptError::CHECKSIGVERIFY:
				return "Script failed an OP_CHECKSIGVERIFY operation";
			case scriptError::NUMEQUALVERIFY:
				return "Script failed an OP_NUMEQUALVERIFY operation";
			case scriptError::SCRIPT_SIZE:
				return "Script is too big";
			case scriptError::PUSH_SIZE:
				return "Push value size limit exceeded";
			case scriptError::OP_COUNT:
				return "Operation limit exceeded";
			case scriptError::STACK_SIZE:
				return "Stack size limit exceeded";
			case scriptError::SIG_COUNT:
				return "Signature count negative or greater than pubkey count";
			case scriptError::PUBKEY_COUNT:
				return "Pubkey count negative or limit exceeded";
			case scriptError::BAD_OPCODE:
				return "Opcode missing or not understood";
			case scriptError::DISABLED_OPCODE:
				return "Attempted to use a disabled opcode";
			case scriptError::INVALID_STACK_OPERATION:
				return "Operation not valid with the current stack size";
			case scriptError::INVALID_ALTSTACK_OPERATION:
				return "Operation not valid with the current altstack size";
			case scriptError::RETURN:
				return "OP_RETURN was encountered";
			case scriptError::UNBALANCED_CONDITIONAL:
				return "Invalid OP_IF construction";
			case scriptError::NEGATIVE_LOCKTIME:
				return "Negative locktime";
			case scriptError::UNSATISFIED_LOCKTIME:
				return "Locktime requirement not satisfied";
			case scriptError::SIG_HASHTYPE:
				return "Signature hash type missing or not understood";
			case scriptError::SIG_DER:
				return "Non-canonical DER signature";
			case scriptError::ERR_MINIMALDATA:
				return "Data push larger than necessary";
			case scriptError::SIG_PUSHONLY:
				return "Only non-push operators allowed in signatures";
			case scriptError::SIG_HIGH_S:
				return "Non-canonical signature: S value is unnecessarily high";
			case scriptError::SIG_NULLDUMMY:
				return "Dummy CHECKMULTISIG argument must be zero";
			case scriptError::MINIMALIF:
				return "OP_IF/NOTIF argument must be minimal";
			case scriptError::SIG_NULLFAIL:
				return "Signature must be zero for failed CHECK(MULTI)SIG operation";
			case scriptError::ERR_DISCOURAGE_UPGRADABLE_NOPS:
				return "NOPx reserved for soft-fork upgrades";
			case scriptError::PUBKEYTYPE:
				return "Public key is neither compressed or uncompressed";
			case scriptError::UNKNOWN_ERROR:
			case scriptError::ERROR_COUNT:
			default: break;
		}
		return "unknown error";
	}
}