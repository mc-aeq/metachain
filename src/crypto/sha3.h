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

/*
sha-3 implementation based on https://github.com/DuSTman31/SHA-3
*/

class SHA3
{
private:
	keccakState								m_keccakState;

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
		SHA3_224 = 224,
		SHA3_256 = 256,
		SHA3_384 = 384,
		SHA3_512 = 512
	};

	std::string								hash(HashType type, HashSize size, char *pcBuffer, unsigned int uiLength, unsigned int uiDigestLength = 0);
};

#endif