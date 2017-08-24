/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "sha3.h"
#include <iomanip>
#include <boost/lexical_cast.hpp>
#include "endian.h"

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

SHA3::SHA3()
	:m_pEncBuf(NULL)
{
}

SHA3::~SHA3()
{
	if (m_pEncBuf)
		delete m_pEncBuf;
}

std::string	SHA3::to_string(uint8_t *input, unsigned int uiSize)
{	
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (unsigned int i = 0; i < (uiSize / 8); i++)
		ss << std::setw(2) << static_cast<unsigned>(input[i]);

	return ss.str();
}

uint8_t* SHA3::hash(HashType type, HashSize size, const uint8_t * pBuffer, unsigned int uiLength, unsigned int uiDigestLength )
{
	if (type == SHA3::HashType::SHAKE)
		shakeCreate(size, uiDigestLength);
	else
		keccakCreate(size);
	
	keccakUpdate(pBuffer, 0, uiLength);
		
	uint8_t *op;
	switch (type)
	{
	case SHA3::HashType::DEFAULT:
		op = sha3Digest();
		break;
	case SHA3::HashType::KECCAK:
		op = keccakDigest();
		break;
	case SHA3::HashType::SHAKE:
		op = shakeDigest();
		break;
	}

	return op;
}

uint8_t* SHA3::cShake(HashSize size, const uint8_t *pBuffer, unsigned int uiLength, unsigned int uiDigestLength, std::string strFunctionName, std::string strCustomization)
{
	if (strFunctionName == "" && strCustomization == "")
		return hash(SHA3::HashType::SHAKE, size, pBuffer, uiLength, uiDigestLength);

	unsigned int uiBlockSize = 200 - 2 * (size / 8);
	unsigned int uiBufferSize = (strFunctionName.length() + strCustomization.length() == 0 ? 1 : ceil((float)(strFunctionName.length() + strCustomization.length()) / uiBlockSize))*uiBlockSize + uiLength + 1;
	m_pEncBuf = new unsigned char[uiBufferSize];
	memset(m_pEncBuf, 0x00, uiBufferSize);

	// the block size
	unsigned int uiBytesWritten = 0;
	unsigned int uiOffset = left_encode(m_pEncBuf, uiBlockSize);

	// the function name
	unsigned char *pPtr = encode_string((unsigned char*)strFunctionName.c_str(), strFunctionName.length(), &uiBytesWritten);
	memcpy( (m_pEncBuf + uiOffset), pPtr, uiBytesWritten);
	uiOffset += uiBytesWritten;
	delete pPtr;

	// the customization string
	pPtr = encode_string((unsigned char*)strCustomization.c_str(), strCustomization.length(), &uiBytesWritten);
	memcpy( (m_pEncBuf + uiOffset), pPtr, uiBytesWritten);
	uiOffset += uiBytesWritten;
	delete pPtr;

	// the content itself
	uiOffset = (strFunctionName.length() + strCustomization.length() == 0 ? 1 : ceil((float)(strFunctionName.length() + strCustomization.length()) / uiBlockSize))*uiBlockSize; //bytepad
	memcpy((m_pEncBuf + uiOffset), pBuffer, uiLength);

	uiOffset += uiLength;

	// call the has function with the prepared cShake buffer
	return hash(SHA3::HashType::SHAKE, size, m_pEncBuf, uiOffset, uiDigestLength);
}

uint8_t* SHA3::kmac(HashSize size, const uint8_t *pBuffer, unsigned int uiLength, unsigned int uiDigestLength, std::string strKey, std::string strCustomization)
{
	unsigned int uiBlockSize = 200 - 2 * (size / 8);
	unsigned int uiBufferSize = (strKey.length() == 0 ? 1 : ceil((float)strKey.length() / uiBlockSize))*uiBlockSize + uiLength + sizeof(size_t) + 1;
	m_pEncBuf = new unsigned char[uiBufferSize];
	memset(m_pEncBuf, 0x00, uiBufferSize);
	unsigned int uiBytesWritten = 0;

	// the key
	unsigned char *pPtr = encode_string((unsigned char*)strKey.c_str(), strKey.length(), &uiBytesWritten);
	memcpy(m_pEncBuf, pPtr, uiBytesWritten);
	delete pPtr;

	// the content
	unsigned int uiOffset = (strKey.length() == 0 ? 1 : ceil(strKey.length() / uiBlockSize))*uiBlockSize; //bytepad
	memcpy((m_pEncBuf + uiOffset), pBuffer, uiLength);
	uiOffset += uiLength;
	
	// the digest length
	uiBytesWritten = right_encode( (m_pEncBuf +uiOffset), uiDigestLength);
	uiOffset += uiBytesWritten;

	//cshake
	return cShake(size, m_pEncBuf, uiOffset, uiDigestLength, "KMAC", strCustomization);
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

unsigned int SHA3::left_encode(unsigned char * encbuf, size_t value)
{
	unsigned int n, i;
	size_t v;

	for (v = value, n = 0; v && (n < sizeof(size_t)); ++n, v >>= 8)
		; /* empty */
	if (n == 0)
		n = 1;
	for (i = 1; i <= n; ++i)
	{
		encbuf[i] = (unsigned char)(value >> (8 * (n - i)));
	}
	encbuf[0] = (unsigned char)n;
	return n + 1;
}

unsigned int SHA3::right_encode(unsigned char * encbuf, size_t value)
{
	unsigned int n, i;
	size_t v;

	for (v = value, n = 0; v && (n < sizeof(size_t)); ++n, v >>= 8)
		; /* empty */
	if (n == 0)
		n = 1;
	for (i = 1; i <= n; ++i)
	{
		encbuf[i - 1] = (unsigned char)(value >> (8 * (n - i)));
	}
	encbuf[n] = (unsigned char)n;
	return n + 1;
}

unsigned char* SHA3::encode_string(unsigned char *pInput, unsigned int uiLength, unsigned int *uiBytesWritten)
{
	unsigned char encbuf[sizeof(size_t) + 1];
	*uiBytesWritten = left_encode(encbuf, uiLength);
	unsigned char *pBuffer = new unsigned char[*uiBytesWritten + uiLength];

	memcpy(pBuffer, encbuf, *uiBytesWritten);
	memcpy((pBuffer + *uiBytesWritten), pInput, uiLength);

	*uiBytesWritten += uiLength;

	return pBuffer;
}