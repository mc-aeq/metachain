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

#ifndef __MCP06_SCRIPT_ERROR_H__
#define __MCP06_SCRIPT_ERROR_H__

#include <string>

namespace MCP06
{
	enum scriptError
	{
		OK,
		UNKNOWN_ERROR,
		EVAL_FALSE,
		RETURN,

		/* Max sizes */
		SCRIPT_SIZE,
		PUSH_SIZE,
		OP_COUNT,
		STACK_SIZE,
		SIG_COUNT,
		PUBKEY_COUNT,

		/* Failed verify operations */
		VERIFY,
		EQUALVERIFY,
		CHECKMULTISIGVERIFY,
		CHECKSIGVERIFY,
		NUMEQUALVERIFY,

		/* Logical/Format/Canonical errors */
		BAD_OPCODE,
		DISABLED_OPCODE,
		INVALID_STACK_OPERATION,
		INVALID_ALTSTACK_OPERATION,
		UNBALANCED_CONDITIONAL,

		/* CHECKLOCKTIMEVERIFY and CHECKSEQUENCEVERIFY */
		NEGATIVE_LOCKTIME,
		UNSATISFIED_LOCKTIME,

		/* Malleability */
		SIG_HASHTYPE,
		SIG_DER,
		ERR_MINIMALDATA,
		SIG_PUSHONLY,
		SIG_HIGH_S,
		SIG_NULLDUMMY,
		PUBKEYTYPE,
		ERR_CLEANSTACK,
		MINIMALIF,
		SIG_NULLFAIL,

		/* softfork safeness */
		ERR_DISCOURAGE_UPGRADABLE_NOPS,

		ERROR_COUNT
	};

	std::string scriptErrorString( scriptError err );
}

#endif
