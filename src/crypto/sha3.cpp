/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "sha3.h"
#include <boost/lexical_cast.hpp>
#include "../endian.h"

static inline uint64_t rotateLeft(uint64_t x, unsigned int n)
{
	const unsigned int mask = (8 * sizeof(x) - 1);  // assumes width is a power of 2.

													// assert ( (c<=mask) &&"rotate by type width or more");
	n &= mask;
	return (x << n) | (x >> ((-n)&mask));
}

static inline uint64_t rotateRight(uint64_t x, int n)
{
	const unsigned int mask = (8 * sizeof(x) - 1);  // assumes width is a power of 2.

													// assert ( (c<=mask) &&"rotate by type width or more");
	n &= mask;
	return (x >> n) | (x << ((-n)&mask));

}

std::string SHA3::hash(HashType type, HashSize size, char * pcBuffer, unsigned int uiLength, unsigned int uiDigestLength )
{
	if (type == SHA3::HashType::SHAKE)
		shakeCreate(size, uiDigestLength);
	else
		keccakCreate(size);
	
	keccakUpdate((uint8_t*)pcBuffer, 0, uiLength);
	return boost::lexical_cast<std::string>(sha3Digest());	

	switch (type)
	{
	case SHA3::HashType::DEFAULT:
		return boost::lexical_cast<std::string>(sha3Digest());
	case SHA3::HashType::KECCAK:
		return boost::lexical_cast<std::string>(keccakDigest());
	case SHA3::HashType::SHAKE:
		return boost::lexical_cast<std::string>(shakeDigest());
	}
}

// Function to create the state structure for keccak application, of size length
//   (where length is the number of bits in the hash divided by 8. 
//   (eg 64 for SHA-3-512)
void SHA3::keccakCreate(int length)
{
	memset(&m_keccakState, 0, sizeof(keccakState));

	m_keccakState.A = new uint64_t[25];
	memset(m_keccakState.A, 0, 25 * sizeof(uint64_t));
	m_keccakState.blockLen = 200 - 2 * (length / 8);
	m_keccakState.buffer = new uint8_t[m_keccakState.blockLen];
	memset(m_keccakState.buffer, 0, m_keccakState.blockLen * sizeof(uint8_t));
	m_keccakState.bufferLen = 0;
	m_keccakState.length = length;
	m_keccakState.d = length;
}

// Function to create the state structure for SHAKE application, of size length
//   (where length is the number of bits in the hash divided by 8. 
//   (eg 32 for SHAKE256)
void SHA3::shakeCreate(int length, unsigned int d_)
{
	memset(&m_keccakState, 0, sizeof(keccakState));

	m_keccakState.A = new uint64_t[25];
	memset(m_keccakState.A, 0, 25 * sizeof(uint64_t));
	m_keccakState.blockLen = 200 - 2 * (length / 8);
	m_keccakState.buffer = new uint8_t[m_keccakState.blockLen];
	memset(m_keccakState.buffer, 0, m_keccakState.blockLen * sizeof(uint8_t));
	m_keccakState.bufferLen = 0;
	m_keccakState.length = length;
	m_keccakState.d = d_;
}

// keccakUpdate - Functions to pack input data into a block
//  One byte input at a time - process buffer if it's empty
void SHA3::keccakUpdate(uint8_t input)
{
	m_keccakState.buffer[m_keccakState.bufferLen] = input;
	if (++(m_keccakState.bufferLen) == m_keccakState.blockLen)
	{
		keccakProcessBuffer();
	}
}

//  Process a larger buffer with varying amounts of data in it
void SHA3::keccakUpdate(const uint8_t *input, int off, unsigned int len)
{
	uint8_t *buffer = m_keccakState.buffer;
	while (len > 0)
	{
		int cpLen = 0;
		if ((m_keccakState.blockLen - m_keccakState.bufferLen) > len)
		{
			cpLen = len;
		}
		else
		{
			cpLen = m_keccakState.blockLen - m_keccakState.bufferLen;
		}

		for (unsigned int i = 0; i != cpLen; i++)
		{
			buffer[m_keccakState.bufferLen + i] = input[off + i];
		}
		m_keccakState.bufferLen += cpLen;
		off += cpLen;
		len -= cpLen;
		if (m_keccakState.bufferLen == m_keccakState.blockLen)
		{
			keccakProcessBuffer();
		}
	}
}

void SHA3::keccakProcessBuffer()
{
	uint64_t *A = m_keccakState.A;
	for (unsigned int i = 0; i < m_keccakState.blockLen / 8; i++)
	{
		A[i] ^= htole64(((uint64_t*)m_keccakState.buffer)[i]);
	}
	keccakf();
	m_keccakState.bufferLen = 0;
}

void SHA3::keccakReset()
{
	for (unsigned int i = 0; i<25; i++)
	{
		m_keccakState.A[i] = 0L;
	}
	m_keccakState.bufferLen = 0;
}

// sha3Digest - called once all data has been few to the keccakUpdate functions
//  Pads the structure (in case the input is not a multiple of the block length)
//  returns the hash result in a char array (not null terminated)
unsigned char *SHA3::sha3Digest()
{
	uint64_t *A = m_keccakState.A;
	sha3AddPadding();
	keccakProcessBuffer();
	uint64_t *tmp = new uint64_t[m_keccakState.length];
	for (int i = 0; i < m_keccakState.length; i += 8)
	{
		tmp[i >> 3] = htole64(A[i >> 3]);
	}
	keccakReset();
	return (unsigned char*)tmp;
}

// keccakDigest - called once all data has been few to the keccakUpdate functions
//  Pads the structure (in case the input is not a multiple of the block length)
//  returns the hash result in a char array (not null terminated)
unsigned char *SHA3::keccakDigest()
{
	uint64_t *A = m_keccakState.A;
	keccakAddPadding();
	keccakProcessBuffer();
	uint64_t *tmp = new uint64_t[m_keccakState.length];
	for (unsigned int i = 0; i < m_keccakState.length; i += 8)
	{
		tmp[i >> 3] = htole64(A[i >> 3]);
	}
	keccakReset();
	return (unsigned char*)tmp;
}

// shakeDigest - called once all data has been few to the keccakUpdate functions
//  Pads the structure (in case the input is not a multiple of the block length)
//  returns the hash result in a char array (not null terminated)
unsigned char *SHA3::shakeDigest()
{
	uint64_t *A = m_keccakState.A;
	shakeAddPadding();
	keccakProcessBuffer();
	uint64_t *tmp = new uint64_t[m_keccakState.d];
	for (unsigned int i = 0; i < m_keccakState.d; i += 8)
	{
		tmp[i >> 3] = htole64(A[i >> 3]);
	}
	keccakReset();
	return (unsigned char*)tmp;
}


void SHA3::sha3AddPadding()
{
	unsigned int bufferLen = m_keccakState.bufferLen;
	uint8_t *buffer = m_keccakState.buffer;
	if (bufferLen + 1 == m_keccakState.blockLen)
	{
		buffer[bufferLen] = (uint8_t)0x86;
	}
	else
	{
		buffer[bufferLen] = (uint8_t)0x06;
		for (unsigned int i = bufferLen + 1; i < m_keccakState.blockLen - 1; i++)
		{
			buffer[i] = 0;
		}
		buffer[m_keccakState.blockLen - 1] = (uint8_t)0x80;
	}
}

void SHA3::keccakAddPadding()
{
	unsigned int bufferLen = m_keccakState.bufferLen;
	uint8_t *buffer = m_keccakState.buffer;
	if (bufferLen + 1 == m_keccakState.blockLen)
	{
		buffer[bufferLen] = (uint8_t)0x81;
	}
	else
	{
		buffer[bufferLen] = (uint8_t)0x01;
		for (unsigned int i = bufferLen + 1; i < m_keccakState.blockLen - 1; i++)
		{
			buffer[i] = 0;
		}
		buffer[m_keccakState.blockLen - 1] = (uint8_t)0x80;
	}
}

void SHA3::shakeAddPadding()
{
	unsigned int bufferLen = m_keccakState.bufferLen;
	uint8_t *buffer = m_keccakState.buffer;
	if (bufferLen + 1 == m_keccakState.blockLen)
	{
		buffer[bufferLen] = (uint8_t)0x9F;
	}
	else
	{
		buffer[bufferLen] = (uint8_t)0x1F;
		for (unsigned int i = bufferLen + 1; i < m_keccakState.blockLen - 1; i++)
		{
			buffer[i] = 0;
		}
		buffer[m_keccakState.blockLen - 1] = (uint8_t)0x80;
	}
}

// Hash function proper. 
void SHA3::keccakf()
{
	uint64_t *A = m_keccakState.A;
	keccakfState kState;
	for (int n = 0; n < 24; n++)
	{
		for (int x = 0; x < 5; x++)
		{
			kState.C[x] = A[index(x, 0)] ^ A[index(x, 1)] ^ A[index(x, 2)] ^ A[index(x, 3)] ^ A[index(x, 4)];
		}
		int i;
		int x = 0;
		int y = 0;
		x = 0;
		kState.D[x] = kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
		y = 0;
		A[index(x, y)] ^= kState.D[x];
		y = 1;
		A[index(x, y)] ^= kState.D[x];
		y = 2;
		A[index(x, y)] ^= kState.D[x];
		y = 3;
		A[index(x, y)] ^= kState.D[x];
		y = 4;
		A[index(x, y)] ^= kState.D[x];
		x = 1;
		kState.D[x] = kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
		y = 0;
		A[index(x, y)] ^= kState.D[x];
		y = 1;
		A[index(x, y)] ^= kState.D[x];
		y = 2;
		A[index(x, y)] ^= kState.D[x];
		y = 3;
		A[index(x, y)] ^= kState.D[x];
		y = 4;
		A[index(x, y)] ^= kState.D[x];
		x = 2;
		kState.D[x] = kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
		y = 0;
		A[index(x, y)] ^= kState.D[x];
		y = 1;
		A[index(x, y)] ^= kState.D[x];
		y = 2;
		A[index(x, y)] ^= kState.D[x];
		y = 3;
		A[index(x, y)] ^= kState.D[x];
		y = 4;
		A[index(x, y)] ^= kState.D[x];
		x = 3;
		kState.D[x] = kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
		y = 0;
		A[index(x, y)] ^= kState.D[x];
		y = 1;
		A[index(x, y)] ^= kState.D[x];
		y = 2;
		A[index(x, y)] ^= kState.D[x];
		y = 3;
		A[index(x, y)] ^= kState.D[x];
		y = 4;
		A[index(x, y)] ^= kState.D[x];
		x = 4;
		kState.D[x] = kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
		y = 0;
		A[index(x, y)] ^= kState.D[x];
		y = 1;
		A[index(x, y)] ^= kState.D[x];
		y = 2;
		A[index(x, y)] ^= kState.D[x];
		y = 3;
		A[index(x, y)] ^= kState.D[x];
		y = 4;
		A[index(x, y)] ^= kState.D[x];


		x = 0;
		y = 0;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 1;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 2;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 3;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 4;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		x = 1;
		y = 0;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 1;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 2;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 3;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 4;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		x = 2;
		y = 0;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 1;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 2;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 3;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 4;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		x = 3;
		y = 0;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 1;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 2;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 3;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 4;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		x = 4;
		y = 0;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 1;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 2;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 3;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
		y = 4;
		i = index(x, y);
		kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);

		x = 0;
		y = 0;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 1;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 2;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 3;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 4;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		x = 1;
		y = 0;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 1;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 2;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 3;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 4;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		x = 2;
		y = 0;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 1;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 2;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 3;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 4;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		x = 3;
		y = 0;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 1;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 2;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 3;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 4;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		x = 4;
		y = 0;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 1;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 2;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 3;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
		y = 4;
		i = index(x, y);
		A[i] = kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);

		A[0] ^= RC[n];
	}
}

inline int SHA3::index(int x)
{
	return x < 0 ? index(x + 5) : x % 5;
}

inline int SHA3::index(int x, int y)
{
	return index(x) + 5 * index(y);
}