// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "scriptInterpreter.h"
#include "signatureChecker.h"
#include "../../logger.h"
#include "../../crypto/cHash256.h"
#include "../../crypto/ripemd160.h"
#include "../../crypto/sha1.h"
#include "../../crypto/sha256.h"
#include "../../crypto/sha3.h"
#include "../MCP03/txIn.h"

namespace MCP06
{
	/**
	* Script is a stack machine (like Forth) that evaluates a predicate
	* returning a bool indicating valid or not.  There are no loops.
	*/
	scriptInterpreter::scriptInterpreter(CScript script)
		:m_Script(script)
	{
	}

	scriptInterpreter::~scriptInterpreter()
	{
	}

	void scriptInterpreter::popStack()
	{
		if (m_vecStack.empty())
			throw std::runtime_error("popStack(): m_vecStack empty");
		m_vecStack.pop_back();
	}

	void scriptInterpreter::popAltStack()
	{
		if (m_vecAltStack.empty())
			throw std::runtime_error("popAltStack(): stack empty");
		m_vecAltStack.pop_back();
	}

	bool scriptInterpreter::evalScript(SCRIPT_VERIFY flags, scriptError* serror)
	{
		static const CScriptNum bnZero(0);
		static const CScriptNum bnOne(1);
		static const valtype vchFalse(0);
		static const valtype vchTrue(1, 1);

		CScript::const_iterator pc = m_Script.begin();
		CScript::const_iterator pend = m_Script.end();
		CScript::const_iterator pbegincodehash = m_Script.begin();

		opcodetype opcode;
		valtype vchPushValue;
		std::vector<bool> vfExec;

		if (m_Script.size() > MAX_SCRIPT_SIZE)
			return set_error(serror, scriptError::SCRIPT_SIZE);
		int nOpCount = 0;
		bool fRequireMinimal = (flags & SCRIPT_VERIFY::MINIMALDATA) != 0;

		try
		{
			while (pc < pend)
			{
				bool fExec = !count(vfExec.begin(), vfExec.end(), false);

				//
				// Read instruction
				//
				if (!m_Script.GetOp(pc, opcode, vchPushValue))
					return set_error(serror, scriptError::BAD_OPCODE);
				if (vchPushValue.size() > MAX_SCRIPT_ELEMENT_SIZE)
					return set_error(serror, scriptError::PUSH_SIZE);

				// Note how OP_RESERVED does not count towards the opcode limit.
				if (opcode > OP_16 && ++nOpCount > MAX_OPS_PER_SCRIPT)
					return set_error(serror, scriptError::OP_COUNT);

				if (opcode == OP_CAT ||
					opcode == OP_SUBSTR ||
					opcode == OP_LEFT ||
					opcode == OP_RIGHT ||
					opcode == OP_INVERT ||
					opcode == OP_AND ||
					opcode == OP_OR ||
					opcode == OP_XOR ||
					opcode == OP_2MUL ||
					opcode == OP_2DIV ||
					opcode == OP_MUL ||
					opcode == OP_DIV ||
					opcode == OP_MOD ||
					opcode == OP_LSHIFT ||
					opcode == OP_RSHIFT)
					return set_error(serror, scriptError::DISABLED_OPCODE); // Disabled opcodes.

				if (fExec && 0 <= opcode && opcode <= OP_PUSHDATA4) {
					if (fRequireMinimal && !CheckMinimalPush(vchPushValue, opcode)) {
						return set_error(serror, scriptError::ERR_MINIMALDATA);
					}
					m_vecStack.push_back(vchPushValue);
				}
				else if (fExec || (OP_IF <= opcode && opcode <= OP_ENDIF))
					switch (opcode)
					{
						//
						// Push value
						//
					case OP_1NEGATE:
					case OP_1:
					case OP_2:
					case OP_3:
					case OP_4:
					case OP_5:
					case OP_6:
					case OP_7:
					case OP_8:
					case OP_9:
					case OP_10:
					case OP_11:
					case OP_12:
					case OP_13:
					case OP_14:
					case OP_15:
					case OP_16:
					{
						// ( -- value)
						CScriptNum bn((int)opcode - (int)(OP_1 - 1));
						m_vecStack.push_back(bn.getvch());
						// The result of these opcodes should always be the minimal way to push the data
						// they push, so no need for a CheckMinimalPush here.
					}
					break;


					//
					// Control
					//
					case OP_NOP:
						break;

					case OP_NOP1: case OP_NOP4: case OP_NOP5:
					case OP_NOP6: case OP_NOP7: case OP_NOP8: case OP_NOP9: case OP_NOP10:
					{
						if (flags & SCRIPT_VERIFY::DISCOURAGE_UPGRADABLE_NOPS)
							return set_error(serror, scriptError::ERR_DISCOURAGE_UPGRADABLE_NOPS);
					}
					break;

					case OP_IF:
					case OP_NOTIF:
					{
						// <expression> if [statements] [else [statements]] endif
						bool fValue = false;
						if (fExec)
						{
							if (m_vecStack.size() < 1)
								return set_error(serror, scriptError::UNBALANCED_CONDITIONAL);
							valtype& vch = stacktop(-1);
							
							fValue = CastToBool(vch);
							if (opcode == OP_NOTIF)
								fValue = !fValue;
							popStack();
						}
						vfExec.push_back(fValue);
					}
					break;

					case OP_ELSE:
					{
						if (vfExec.empty())
							return set_error(serror, scriptError::UNBALANCED_CONDITIONAL);
						vfExec.back() = !vfExec.back();
					}
					break;

					case OP_ENDIF:
					{
						if (vfExec.empty())
							return set_error(serror, scriptError::UNBALANCED_CONDITIONAL);
						vfExec.pop_back();
					}
					break;

					case OP_VERIFY:
					{
						// (true -- ) or
						// (false -- false) and return
						if (m_vecStack.size() < 1)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						bool fValue = CastToBool(stacktop(-1));
						if (fValue)
							popStack();
						else
							return set_error(serror, scriptError::VERIFY);
					}
					break;

					case OP_RETURN:
					{
						return set_error(serror, scriptError::RETURN);
					}
					break;


					//
					// Stack ops
					//
					case OP_TOALTSTACK:
					{
						if (m_vecStack.size() < 1)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						m_vecAltStack.push_back(stacktop(-1));
						popStack();
					}
					break;

					case OP_FROMALTSTACK:
					{
						if (m_vecAltStack.size() < 1)
							return set_error(serror, scriptError::INVALID_ALTSTACK_OPERATION);
						m_vecStack.push_back(altstacktop(-1));
						popAltStack();
					}
					break;

					case OP_2DROP:
					{
						// (x1 x2 -- )
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						popStack();
						popStack();
					}
					break;

					case OP_2DUP:
					{
						// (x1 x2 -- x1 x2 x1 x2)
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype vch1 = stacktop(-2);
						valtype vch2 = stacktop(-1);
						m_vecStack.push_back(vch1);
						m_vecStack.push_back(vch2);
					}
					break;

					case OP_3DUP:
					{
						// (x1 x2 x3 -- x1 x2 x3 x1 x2 x3)
						if (m_vecStack.size() < 3)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype vch1 = stacktop(-3);
						valtype vch2 = stacktop(-2);
						valtype vch3 = stacktop(-1);
						m_vecStack.push_back(vch1);
						m_vecStack.push_back(vch2);
						m_vecStack.push_back(vch3);
					}
					break;

					case OP_2OVER:
					{
						// (x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2)
						if (m_vecStack.size() < 4)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype vch1 = stacktop(-4);
						valtype vch2 = stacktop(-3);
						m_vecStack.push_back(vch1);
						m_vecStack.push_back(vch2);
					}
					break;

					case OP_2ROT:
					{
						// (x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2)
						if (m_vecStack.size() < 6)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype vch1 = stacktop(-6);
						valtype vch2 = stacktop(-5);
						m_vecStack.erase(m_vecStack.end() - 6, m_vecStack.end() - 4);
						m_vecStack.push_back(vch1);
						m_vecStack.push_back(vch2);
					}
					break;

					case OP_2SWAP:
					{
						// (x1 x2 x3 x4 -- x3 x4 x1 x2)
						if (m_vecStack.size() < 4)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						swap(stacktop(-4), stacktop(-2));
						swap(stacktop(-3), stacktop(-1));
					}
					break;

					case OP_IFDUP:
					{
						// (x - 0 | x x)
						if (m_vecStack.size() < 1)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype vch = stacktop(-1);
						if (CastToBool(vch))
							m_vecStack.push_back(vch);
					}
					break;

					case OP_DEPTH:
					{
						// -- stacksize
						CScriptNum bn(m_vecStack.size());
						m_vecStack.push_back(bn.getvch());
					}
					break;

					case OP_DROP:
					{
						// (x -- )
						if (m_vecStack.size() < 1)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						popStack();
					}
					break;

					case OP_DUP:
					{
						// (x -- x x)
						if (m_vecStack.size() < 1)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype vch = stacktop(-1);
						m_vecStack.push_back(vch);
					}
					break;

					case OP_NIP:
					{
						// (x1 x2 -- x2)
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						m_vecStack.erase(m_vecStack.end() - 2);
					}
					break;

					case OP_OVER:
					{
						// (x1 x2 -- x1 x2 x1)
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype vch = stacktop(-2);
						m_vecStack.push_back(vch);
					}
					break;

					case OP_PICK:
					case OP_ROLL:
					{
						// (xn ... x2 x1 x0 n - xn ... x2 x1 x0 xn)
						// (xn ... x2 x1 x0 n - ... x2 x1 x0 xn)
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						int n = CScriptNum(stacktop(-1), fRequireMinimal).getint();
						popStack();
						if (n < 0 || n >= (int)m_vecStack.size())
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype vch = stacktop(-n - 1);
						if (opcode == OP_ROLL)
							m_vecStack.erase(m_vecStack.end() - n - 1);
						m_vecStack.push_back(vch);
					}
					break;

					case OP_ROT:
					{
						// (x1 x2 x3 -- x2 x3 x1)
						//  x2 x1 x3  after first swap
						//  x2 x3 x1  after second swap
						if (m_vecStack.size() < 3)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						swap(stacktop(-3), stacktop(-2));
						swap(stacktop(-2), stacktop(-1));
					}
					break;

					case OP_SWAP:
					{
						// (x1 x2 -- x2 x1)
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						swap(stacktop(-2), stacktop(-1));
					}
					break;

					case OP_TUCK:
					{
						// (x1 x2 -- x2 x1 x2)
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype vch = stacktop(-1);
						m_vecStack.insert(m_vecStack.end() - 2, vch);
					}
					break;


					case OP_SIZE:
					{
						// (in -- in size)
						if (m_vecStack.size() < 1)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						CScriptNum bn(stacktop(-1).size());
						m_vecStack.push_back(bn.getvch());
					}
					break;


					//
					// Bitwise logic
					//
					case OP_EQUAL:
					case OP_EQUALVERIFY:
						//case OP_NOTEQUAL: // use OP_NUMNOTEQUAL
					{
						// (x1 x2 - bool)
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype& vch1 = stacktop(-2);
						valtype& vch2 = stacktop(-1);
						bool fEqual = (vch1 == vch2);
						// OP_NOTEQUAL is disabled because it would be too easy to say
						// something like n != 1 and have some wiseguy pass in 1 with extra
						// zero bytes after it (numerically, 0x01 == 0x0001 == 0x000001)
						//if (opcode == OP_NOTEQUAL)
						//    fEqual = !fEqual;
						popStack();
						popStack();
						m_vecStack.push_back(fEqual ? vchTrue : vchFalse);
						if (opcode == OP_EQUALVERIFY)
						{
							if (fEqual)
								popStack();
							else
								return set_error(serror, scriptError::EQUALVERIFY);
						}
					}
					break;


					//
					// Numeric
					//
					case OP_1ADD:
					case OP_1SUB:
					case OP_NEGATE:
					case OP_ABS:
					case OP_NOT:
					case OP_0NOTEQUAL:
					{
						// (in -- out)
						if (m_vecStack.size() < 1)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						CScriptNum bn(stacktop(-1), fRequireMinimal);
						switch (opcode)
						{
						case OP_1ADD:       bn += bnOne; break;
						case OP_1SUB:       bn -= bnOne; break;
						case OP_NEGATE:     bn = -bn; break;
						case OP_ABS:        if (bn < bnZero) bn = -bn; break;
						case OP_NOT:        bn = (bn == bnZero); break;
						case OP_0NOTEQUAL:  bn = (bn != bnZero); break;
						default:            assert(!"invalid opcode"); break;
						}
						popStack();
						m_vecStack.push_back(bn.getvch());
					}
					break;

					case OP_ADD:
					case OP_SUB:
					case OP_BOOLAND:
					case OP_BOOLOR:
					case OP_NUMEQUAL:
					case OP_NUMEQUALVERIFY:
					case OP_NUMNOTEQUAL:
					case OP_LESSTHAN:
					case OP_GREATERTHAN:
					case OP_LESSTHANOREQUAL:
					case OP_GREATERTHANOREQUAL:
					case OP_MIN:
					case OP_MAX:
					{
						// (x1 x2 -- out)
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						CScriptNum bn1(stacktop(-2), fRequireMinimal);
						CScriptNum bn2(stacktop(-1), fRequireMinimal);
						CScriptNum bn(0);
						switch (opcode)
						{
						case OP_ADD:
							bn = bn1 + bn2;
							break;

						case OP_SUB:
							bn = bn1 - bn2;
							break;

						case OP_BOOLAND:             bn = (bn1 != bnZero && bn2 != bnZero); break;
						case OP_BOOLOR:              bn = (bn1 != bnZero || bn2 != bnZero); break;
						case OP_NUMEQUAL:            bn = (bn1 == bn2); break;
						case OP_NUMEQUALVERIFY:      bn = (bn1 == bn2); break;
						case OP_NUMNOTEQUAL:         bn = (bn1 != bn2); break;
						case OP_LESSTHAN:            bn = (bn1 < bn2); break;
						case OP_GREATERTHAN:         bn = (bn1 > bn2); break;
						case OP_LESSTHANOREQUAL:     bn = (bn1 <= bn2); break;
						case OP_GREATERTHANOREQUAL:  bn = (bn1 >= bn2); break;
						case OP_MIN:                 bn = (bn1 < bn2 ? bn1 : bn2); break;
						case OP_MAX:                 bn = (bn1 > bn2 ? bn1 : bn2); break;
						default:                     assert(!"invalid opcode"); break;
						}
						popStack();
						popStack();
						m_vecStack.push_back(bn.getvch());

						if (opcode == OP_NUMEQUALVERIFY)
						{
							if (CastToBool(stacktop(-1)))
								popStack();
							else
								return set_error(serror, scriptError::NUMEQUALVERIFY);
						}
					}
					break;

					case OP_WITHIN:
					{
						// (x min max -- out)
						if (m_vecStack.size() < 3)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						CScriptNum bn1(stacktop(-3), fRequireMinimal);
						CScriptNum bn2(stacktop(-2), fRequireMinimal);
						CScriptNum bn3(stacktop(-1), fRequireMinimal);
						bool fValue = (bn2 <= bn1 && bn1 < bn3);
						popStack();
						popStack();
						popStack();
						m_vecStack.push_back(fValue ? vchTrue : vchFalse);
					}
					break;


					//
					// Crypto
					//
					case OP_RIPEMD160:
					case OP_SHA1:
					case OP_SHA256:
					case OP_HASH256:
					case OP_SHA3_256:
					case OP_SHA3_512:
					case OP_SHA3_CSHAKE:
					{
						// (in -- hash)
						if (m_vecStack.size() < 1)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						valtype& vch = stacktop(-1);
						valtype vchHash((opcode == OP_RIPEMD160 || opcode == OP_SHA1 ) ? 20 : 32);
						if (opcode == OP_RIPEMD160)
							CRIPEMD160().Write(vch.data(), vch.size()).Finalize(vchHash.data());
						else if (opcode == OP_SHA1)
							CSHA1().Write(vch.data(), vch.size()).Finalize(vchHash.data());
						else if (opcode == OP_SHA256)
							CSHA256().Write(vch.data(), vch.size()).Finalize(vchHash.data());
						else if (opcode == OP_HASH256)
							cHash256().Write(vch.data(), vch.size()).Finalize(vchHash.data());

						// todo: insert sha3 methos

						popStack();
						m_vecStack.push_back(vchHash);
					}
					break;

					case OP_CODESEPARATOR:
					{
						// Hash starts after the code separator
						pbegincodehash = pc;
					}
					break;

					case OP_CHECKSIG:
					case OP_CHECKSIGVERIFY:
					{
						// (sig pubkey -- bool)
						if (m_vecStack.size() < 2)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);

						valtype& vchSig = stacktop(-2);
						valtype& vchPubKey = stacktop(-1);

						// Subset of script starting at the most recent codeseparator
						CScript scriptCode(pbegincodehash, pend);

						// Drop the signature 
						scriptCode.FindAndDelete(CScript(vchSig));

						if (!CheckSignatureEncoding(vchSig, flags, serror) || !CheckPubKeyEncoding(vchPubKey, flags, serror))
						{
							//serror is set
							return false;
						}
						bool fSuccess = m_signatureChecker.checkSignature(vchSig, vchPubKey, scriptCode);

						if (!fSuccess && (flags & SCRIPT_VERIFY::NULLFAIL) && vchSig.size())
							return set_error(serror, scriptError::SIG_NULLFAIL);

						popStack();
						popStack();
						m_vecStack.push_back(fSuccess ? vchTrue : vchFalse);
						if (opcode == OP_CHECKSIGVERIFY)
						{
							if (fSuccess)
								popStack();
							else
								return set_error(serror, scriptError::CHECKSIGVERIFY);
						}
					}
					break;

					case OP_CHECKMULTISIG:
					case OP_CHECKMULTISIGVERIFY:
					{
						// ([sig ...] num_of_signatures [pubkey ...] num_of_pubkeys -- bool)

						int i = 1;
						if ((int)m_vecStack.size() < i)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);

						int nKeysCount = CScriptNum(stacktop(-i), fRequireMinimal).getint();
						if (nKeysCount < 0 || nKeysCount > MAX_PUBKEYS_PER_MULTISIG)
							return set_error(serror, scriptError::PUBKEY_COUNT);
						nOpCount += nKeysCount;
						if (nOpCount > MAX_OPS_PER_SCRIPT)
							return set_error(serror, scriptError::OP_COUNT);
						int ikey = ++i;
						// ikey2 is the position of last non-signature item in the m_vecStack. Top stack item = 1.
						// With SCRIPT_VERIFY::NULLFAIL, this is used for cleanup if operation fails.
						int ikey2 = nKeysCount + 2;
						i += nKeysCount;
						if ((int)m_vecStack.size() < i)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);

						int nSigsCount = CScriptNum(stacktop(-i), fRequireMinimal).getint();
						if (nSigsCount < 0 || nSigsCount > nKeysCount)
							return set_error(serror, scriptError::SIG_COUNT);
						int isig = ++i;
						i += nSigsCount;
						if ((int)m_vecStack.size() < i)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);

						// Subset of script starting at the most recent codeseparator
						CScript scriptCode(pbegincodehash, pend);

						// Drop the signature in pre-segwit scripts but not segwit scripts
						for (int k = 0; k < nSigsCount; k++)
						{
							valtype& vchSig = stacktop(-isig - k);
							scriptCode.FindAndDelete(CScript(vchSig));
						}

						bool fSuccess = true;
						while (fSuccess && nSigsCount > 0)
						{
							valtype& vchSig = stacktop(-isig);
							valtype& vchPubKey = stacktop(-ikey);

							// Note how this makes the exact order of pubkey/signature evaluation
							// distinguishable by CHECKMULTISIG NOT if the STRICTENC flag is set.
							// See the script_(in)valid tests for details.
							if (!CheckSignatureEncoding(vchSig, flags, serror) || !CheckPubKeyEncoding(vchPubKey, flags, serror)) {
								// serror is set
								return false;
							}

							// Check signature
							bool fOk = m_signatureChecker.checkSignature(vchSig, vchPubKey, scriptCode);

							if (fOk) {
								isig++;
								nSigsCount--;
							}
							ikey++;
							nKeysCount--;

							// If there are more signatures left than keys left,
							// then too many signatures have failed. Exit early,
							// without checking any further signatures.
							if (nSigsCount > nKeysCount)
								fSuccess = false;
						}

						// Clean up stack of actual arguments
						while (i-- > 1) {
							// If the operation failed, we require that all signatures must be empty vector
							if (!fSuccess && (flags & SCRIPT_VERIFY::NULLFAIL) && !ikey2 && stacktop(-1).size())
								return set_error(serror, scriptError::SIG_NULLFAIL);
							if (ikey2 > 0)
								ikey2--;
							popStack();
						}

						// A bug causes CHECKMULTISIG to consume one extra argument
						// whose contents were not checked in any way.
						//
						// Unfortunately this is a potential source of mutability,
						// so optionally verify it is exactly equal to zero prior
						// to removing it from the m_vecStack.
						if (m_vecStack.size() < 1)
							return set_error(serror, scriptError::INVALID_STACK_OPERATION);
						if ((flags & SCRIPT_VERIFY::NULLDUMMY) && stacktop(-1).size())
							return set_error(serror, scriptError::SIG_NULLDUMMY);
						popStack();

						m_vecStack.push_back(fSuccess ? vchTrue : vchFalse);

						if (opcode == OP_CHECKMULTISIGVERIFY)
						{
							if (fSuccess)
								popStack();
							else
								return set_error(serror, scriptError::CHECKMULTISIGVERIFY);
						}
					}
					break;

					default:
						return set_error(serror, scriptError::BAD_OPCODE);
					}

				// Size limits
				if (m_vecStack.size() + m_vecAltStack.size() > MAX_STACK_SIZE)
					return set_error(serror, scriptError::STACK_SIZE);
			}
		}
		catch (...)
		{
			return set_error(serror, scriptError::UNKNOWN_ERROR);
		}

		if (!vfExec.empty())
			return set_error(serror, scriptError::UNBALANCED_CONDITIONAL);

		return set_success(serror);
	}


	bool scriptInterpreter::verifyScript( const CScript& scriptPubKey, SCRIPT_VERIFY flags, scriptError* serror)
	{	
		if ((flags & SCRIPT_VERIFY::SIGPUSHONLY) != 0 && !m_Script.IsPushOnly()) {
			return set_error(serror, scriptError::SIG_PUSHONLY);
		}

		std::vector<std::vector<unsigned char> > vecStackCopy;
		if (!evalScript( flags, serror))
			// serror is set
			return false;
		if (flags & SCRIPT_VERIFY::P2SH)
			vecStackCopy = m_vecStack;
		
		// eval the pubkey
		CScript scriptBackup = m_Script;
		m_Script = scriptPubKey;
		if (!evalScript( flags, serror))
			// serror is set
			return false;
		m_Script = scriptBackup;

		if (m_vecStack.empty())
			return set_error(serror, scriptError::EVAL_FALSE);
		if (CastToBool(m_vecStack.back()) == false)
			return set_error(serror, scriptError::EVAL_FALSE);

		// Additional validation for spend-to-script-hash transactions:
		if ((flags & SCRIPT_VERIFY::P2SH) && scriptPubKey.IsPayToScriptHash())
		{
			// scriptSig must be literals-only or validation fails
			if (!m_Script.IsPushOnly())
				return set_error(serror, scriptError::SIG_PUSHONLY);

			// Restore stack.
			swap(m_vecStack, vecStackCopy);

			// stack cannot be empty here, because if it was the
			// P2SH  HASH <> EQUAL  scriptPubKey would be evaluated with
			// an empty stack and the EvalScript above would return false.
			if (m_vecStack.empty())
			{
				LOG_ERROR("Stack empty", "scriptInterpreter");
				return set_error(serror, scriptError::INVALID_STACK_OPERATION);
			}

			const valtype& pubKeySerialized = m_vecStack.back();
			CScript pubKey2(pubKeySerialized.begin(), pubKeySerialized.end());
			popStack();

			scriptBackup = m_Script;
			m_Script = pubKey2;
			if (!evalScript( flags, serror))
				// serror is set
				return false;
			m_Script = scriptBackup;

			if (m_vecStack.empty())
				return set_error(serror, scriptError::EVAL_FALSE);
			if (!CastToBool(m_vecStack.back()))
				return set_error(serror, scriptError::EVAL_FALSE);
		}

		// The CLEANSTACK check is only performed after potential P2SH evaluation,
		// as the non-P2SH evaluation of a P2SH script will obviously not result in
		// a clean stack (the P2SH inputs remain). The same holds for witness evaluation.
		if ((flags & SCRIPT_VERIFY::CLEANSTACK) != 0)
		{
			// Disallow CLEANSTACK without P2SH, as otherwise a switch CLEANSTACK->P2SH+CLEANSTACK
			// would be possible, which is not a softfork (and P2SH should be one).
			if ((flags & SCRIPT_VERIFY::P2SH) != 0)
			{
				LOG_ERROR("SCRIPT_VERIFY::CLEANSTACK can't run without SCRIPT_VERIFY::P2SH", "scriptInterpreter");
				return set_error(serror, scriptError::ERR_CLEANSTACK);
			}
			if (m_vecStack.size() != 1)
				return set_error(serror, scriptError::ERR_CLEANSTACK);
		}

		return set_success(serror);
	}
}