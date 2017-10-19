#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __SMSC_H__
#define __SMSC_H__ 1

#include <fstream>
#include <boost/filesystem/path.hpp>
#include "../cCriticalSection.h"

struct smSC
{
	cCriticalSection				csAccess;
	std::ofstream					streamRaw;
	boost::filesystem::path			filePath;
	uintmax_t						uimRawFileSize;

	// serialization
	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		if (version == 1)
		{
			/*ar & uint16ChainIdentifier;
			ar & caChainName;
			ar & caPoP;*/
		}
	}
};

BOOST_CLASS_VERSION(smSC, 1)
#endif