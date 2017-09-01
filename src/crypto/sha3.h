#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __SHA3_H__
#define __SHA3_H__ 1

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>

/*
sha-3 implementation based on https://github.com/DuSTman31/SHA-3
with heavy modifications to use it as a class and more implementations regarding cShake and KMAC
*/

class SHA3
{
private:
	// State structure
	struct keccakState
	{
		uint64_t *A;
		unsigned int blockLen;
		uint8_t *buffer;
		unsigned int bufferLen;
		int length;
		unsigned int d;
	};

	struct keccakfState
	{
		uint64_t B[25];
		uint64_t C[5];
		uint64_t D[5];
	};

	keccakState								m_keccakState;
	unsigned char							*m_pEncBuf;

	void									keccakCreate(int length);
	void									shakeCreate(int length, unsigned int d_);
	void									keccakUpdate(uint8_t input);
	void									keccakUpdate(const uint8_t *input, int off, unsigned int len);
	void									keccakProcessBuffer();
	void									keccakReset();

	unsigned char *							sha3Digest();
	unsigned char *							keccakDigest();
	unsigned char *							shakeDigest();

	void									keccakf();
	void									sha3AddPadding();
	void									keccakAddPadding();
	void									shakeAddPadding();

	// inline optimization functions
	inline int								index(int x);
	inline int								index(int x, int y);

	unsigned int							left_encode(unsigned char * encbuf, size_t value);
	unsigned int							right_encode(unsigned char * encbuf, size_t value);
	unsigned char *							encode_string(unsigned char *pInput, unsigned int uiLength, unsigned int *uiBytesWritten);

	// Constants of the Keccak algorithm.
	static constexpr uint64_t RC[] = {
		0x0000000000000001L, 0x0000000000008082L, 0x800000000000808aL,
		0x8000000080008000L, 0x000000000000808bL, 0x0000000080000001L,
		0x8000000080008081L, 0x8000000000008009L, 0x000000000000008aL,
		0x0000000000000088L, 0x0000000080008009L, 0x000000008000000aL,
		0x000000008000808bL, 0x800000000000008bL, 0x8000000000008089L,
		0x8000000000008003L, 0x8000000000008002L, 0x8000000000000080L,
		0x000000000000800aL, 0x800000008000000aL, 0x8000000080008081L,
		0x8000000000008080L, 0x0000000080000001L, 0x8000000080008008L
	};

	static constexpr int R[] = {
		0, 1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43,
		25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14
	};

public:
	enum HashType
	{
		DEFAULT,
		KECCAK,
		SHAKE
	};

	enum HashSize
	{
		SHA3_128 = 128,
		SHA3_224 = 224,
		SHA3_256 = 256,
		SHA3_384 = 384,
		SHA3_512 = 512
	};

	SHA3();
	~SHA3();

	std::string								to_string(uint8_t *input, unsigned int uiSize);

	uint8_t*								hashFile(std::string strFileName, HashType type, HashSize size, unsigned int uiDigestLength = 0);
	uint8_t*								hash(HashType type, HashSize size, const uint8_t *pBuffer, unsigned int uiLength, unsigned int uiDigestLength = 0);
	uint8_t*								cShake(HashSize size, const uint8_t *pBuffer, unsigned int uiLength, unsigned int uiDigestLength, std::string strFunctionName, std::string strCustomization);
	uint8_t*								kmac(HashSize size, const uint8_t *pBuffer, unsigned int uiLength, unsigned int uiDigestLength, std::string strKey, std::string strCustomization);
};

#endif