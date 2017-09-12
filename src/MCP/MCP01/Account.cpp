
/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "Account.h"
#include "../../MetaChain.h"
#include "../../io/StorageManager.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include "base58.h"
#include "../../../dependencies/secp256k1/include/secp256k1.h"
#include "../../crypto/sha3.h"

namespace MCP01
{
	Account::Account()
		:m_ecdsaPubKey(ECDSA::not_calculated)
	{
	}

	Account::Account(uint8_t *keyPriv)
		:m_ecdsaPubKey(ECDSA::not_calculated)
	{
		// sanitize
		memset(m_keyPriv, 0x00, 64 * sizeof(uint8_t));

		// cpy
		memcpy(m_keyPriv, keyPriv, 64 * sizeof(uint8_t));
	}

	Account::Account(MCP39::long_hash keyPriv)
		: Account(keyPriv.data())
	{
	}

	Account::Account(uint8_t *keyPriv, uint8_t *keyPub, ECDSA ecdsaPubKey)
		: m_ecdsaPubKey(ecdsaPubKey)
	{
		// sanitize
		memset(m_keyPriv, 0x00, 64 * sizeof(uint8_t));
		memset(m_keyPub, 0x00, 64 * sizeof(uint8_t));

		// cpy
		memcpy(m_keyPriv, keyPriv, 64 * sizeof(uint8_t));
		memcpy(m_keyPub, keyPub, 64 * sizeof(uint8_t));
	}

	Account::~Account()
	{
		// sanitize the memory of priv and pub key
		memset(m_keyPriv, 0x00, 64 * sizeof(uint8_t));
		memset(m_keyPub, 0x00, 64 * sizeof(uint8_t));
	}

	bool Account::calcPubKey(ECDSA type)
	{
		// sanitize pub key
		memset(m_keyPub, 0x00, 64 * sizeof(uint8_t));

		// check whether priv key is set
		if (memcmp(m_keyPriv, m_keyPub, 64 * sizeof(uint8_t)) == 0)
			return false;

		if (type == ECDSA::SECP256k1)
		{
			secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
			if (secp256k1_ec_seckey_verify(ctx, (unsigned char*)getPrivKeyStr().c_str()) != 1)
				return false;

			if (secp256k1_ec_pubkey_create(ctx, (secp256k1_pubkey*)m_keyPub, (unsigned char*)getPrivKeyStr().c_str()) == 1)
			{
				m_ecdsaPubKey = ECDSA::SECP256k1;
				return true;
			}
			else
				return false;
		}
		else if( type == ECDSA::SECP256r1 )
		{
			// SECP256r1 not implemented yet
			m_ecdsaPubKey = ECDSA::not_calculated;
			return false;
		}
		else
		{
			m_ecdsaPubKey = ECDSA::not_calculated;
			return false;
		}
	}

	// This function calculates the wallet address based on testnet flag, ECDSA signing algorithm and chain identifier
	// for prettier (std::string) output use getWalletAddress
	bool Account::calcWalletAddress(uint8_t* walletAddress, uint16_t uiChainIdentifier, bool bTestNet)
	{
		/*
		v1:
		Format: [4byte checksum][1byte flags][64byte hashed pubkey(/w chain identifier)][1byte address version]
		Raw Size: 70 byte

		Function flow:
		X = SHA3-512([pubkey][2byte chain identifier])
		C = SHA3-KECCAK-128(SHA3-KECCAK-128([flag byte]X[version byte]))
		WA = [C,first 4 bytes][flag byte][X][version byte]
		WA_nice = MC-<Chain Identifier String, up to 4 char>-<formatted WA>
		*/

		// check whether we have a pub key stored
		static const uint8_t zero[64] = { 0 };
		if (memcmp(m_keyPub, zero, 64 * sizeof(uint8_t)) == 0)
			return false;

		// right now we only support SECP256k1 - change this security check to also include r1
		if (m_ecdsaPubKey != ECDSA::SECP256k1)
			return false;

		// all security checks gone through, start calculating the address
		SHA3 crypto;

		// pub key with padded chain identifier, then calculate SHA3-512
		uint8_t buffer[66] = { 0 };
		memcpy(buffer, m_keyPub, 64);
		memcpy(buffer + 64, &uiChainIdentifier, 2);
		uint8_t* X = crypto.hash(SHA3::HashType::DEFAULT, SHA3::HashSize::SHA3_512, buffer, 65);

		// assemble byte flag
		uint8_t byteFlag = bTestNet ? BITFIELD_TESTNET : 0x00; // fifth bit indicates testnet
		byteFlag |= m_ecdsaPubKey == ECDSA::SECP256k1 ? BITFIELD_ECDSA_k1 : BITFIELD_ECDSA_r1; // first bit indicates ECDSA algorithm. 1=k1, 0=r1. may be changed in future versions to use more bit

		// calc hashsum
		buffer[0] = byteFlag;
		memcpy(buffer + 1, X, 64);
		buffer[65] = WALLET_ADDRESS_VERSION;
		uint8_t* C = crypto.hash(SHA3::HashType::KECCAK, SHA3::HashSize::SHA3_128, crypto.hash(SHA3::HashType::KECCAK, SHA3::HashSize::SHA3_128, buffer, 66), 32);

		// assemble the result
		memcpy(walletAddress, C, 4);
		memcpy(walletAddress + 4, buffer, 66);

		// free some buffers
		delete C, X, buffer;

		return true;
	}

	std::string Account::getWalletAddress( std::string strChainIdentifier, bool bTestNet )
	{
		// get the ChainIdentifier 2byte from string
#ifdef _UTILS
		// hardcoded in metachain-utils/keygenerator
		uint16_t uiChainIdentifier = 0;
		if (strChainIdentifier == "MC")
			uiChainIdentifier = 0;
		else if (strChainIdentifier == "TCT")
			uiChainIdentifier = 1;
		else if (strChainIdentifier == "MINE")
			uiChainIdentifier = 2;
		else
			return "";
#else
		uint16_t uiChainIdentifier = MetaChain::getInstance().getStorageManager()->getChainIdentifier(strChainIdentifier);
#endif

		// calculate the wallet address
		uint8_t uiAddress[70];
		if (!calcWalletAddress(uiAddress, uiChainIdentifier, bTestNet))
			return "";

		// base58 encode for better viewing
		std::string strWallet = base58::encode(uiAddress, 70);
		
		// return nicely formatted
		return WALLET_ADDRESS_STD_PREFIX + (std::string)"-" + strChainIdentifier + "-" + tokenize(strWallet);
	}

	std::string Account::tokenize(std::string strWalletAddress)
	{
		std::string strRet = strWalletAddress;

		static const int sciSpace = 16;
		for (auto it = strRet.begin(); std::distance(it, strRet.end()) >= sciSpace+1; ++it)
		{
			std::advance(it, sciSpace);
			it = strRet.insert(it, '-');
		}
		
		return strRet;
	}

	std::string Account::untokenize(std::string strWalletAddress)
	{
		return boost::replace_all_copy(strWalletAddress, "-", "");
	}

	bool Account::verifyWalletAddress(std::string strWalletAddress)
	{
		/*
		Steps to check for the validity:
		1) remove MC- from the beginning
		2) extract the Chain Identifier from the beginning and get their chainidentifier ID
		3) get the version of this address (last byte). steps below may change depending on version number
		4) extract the checksum
		5) extract the flag byte
		6) calculate checksum and compare it to the checksum provided
		*/

		std::string strTmp = strWalletAddress;

		// 70 chars is the adress alone without tokens or prefixes
		if (strWalletAddress.length() < 70)
			return false;

		// 1
		if (!boost::istarts_with(strWalletAddress, WALLET_ADDRESS_STD_PREFIX+(std::string)"-"))
			return false;
		strTmp = strWalletAddress.substr(3);

		// 2
		// calculating the chain identifier uid
		std::string strChainIdentifier = strTmp.substr(0, strTmp.find("-"));
#ifdef _UTILS
		// hardcoded in metachain-utils/keygenerator		
		if (strChainIdentifier == "MC")
			m_uint16ChainIdentifier = 0;
		else if (strChainIdentifier == "TCT")
			m_uint16ChainIdentifier = 1;
		else if (strChainIdentifier == "MINE")
			m_uint16ChainIdentifier = 2;
		else
			return false;
#else
		m_uint16ChainIdentifier = MetaChain::getInstance().getStorageManager()->getChainIdentifier(strChainIdentifier);
#endif

		strTmp = strTmp.substr(strChainIdentifier.length() + 1);

		// 3
		uint8_t uiAddress[70];
		if (!base58::decode(untokenize(strTmp), uiAddress))
			return false;
		m_uint8VersionNumber = uiAddress[69];

		// 4
		uint8_t uiChecksum[4];
		memcpy(uiChecksum, uiAddress, 4);

		// 5
		m_uiFlags = uiAddress[4];

		// 6
		SHA3 crypto;
		uint8_t* C = crypto.hash(SHA3::HashType::KECCAK, SHA3::HashSize::SHA3_128, crypto.hash(SHA3::HashType::KECCAK, SHA3::HashSize::SHA3_128, (uiAddress+4), 66), 32);
		if (memcmp(uiChecksum, C, 4) != 0)
		{
			delete C;
			return false;
		}
		delete C;

		// if we reached this point, everything went smooth and it's valid
		return true;
	}
}