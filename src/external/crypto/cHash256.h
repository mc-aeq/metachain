#pragma once

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __CHASH256_H__
#define __CHASH256_H__ 1

#include "sha256.h"

/** A hasher class for Bitcoin's 256-bit hash (double SHA-256). */
class cHash256 {
private:
	CSHA256 sha;
public:
	static const size_t OUTPUT_SIZE = CSHA256::OUTPUT_SIZE;

	void Finalize(unsigned char hash[OUTPUT_SIZE]) {
		unsigned char buf[CSHA256::OUTPUT_SIZE];
		sha.Finalize(buf);
		sha.Reset().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(hash);
	}

	cHash256& Write(const unsigned char *data, size_t len) {
		sha.Write(data, len);
		return *this;
	}

	cHash256& Reset() {
		sha.Reset();
		return *this;
	}
};

#endif