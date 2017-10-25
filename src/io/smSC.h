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
	unsigned int					uiRawFileCounter;

	// serialization
	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		if (version == 1)
		{
			ar & uimRawFileSize;
			ar & uiRawFileCounter;
		}
	}

	// copy constructor
	smSC(const smSC& a)
	{
		filePath = a.filePath;
		uimRawFileSize = a.uimRawFileSize;
		uiRawFileCounter = a.uiRawFileCounter;

		// handle the stream since we can't copy it. if the stream wasn't opened before, we don't open it now either
		if( a.streamRaw.is_open() )
			streamRaw.open( filePath.string() + RAW_FILEENDING, std::ios_base::app );
	}

	// default constructor
	smSC()
	{
		uimRawFileSize = 0;
		uiRawFileCounter = 0;
	}
};

BOOST_CLASS_VERSION(smSC, 1)
#endif