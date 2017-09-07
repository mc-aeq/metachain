#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __ZIP_UNZIP_H__
#define __ZIP_UNZIP_H__ 1

#include <minizip/ioapi.h>
#include <minizip/unzip.h>
#include <string>
#include <vector>
#include <iostream>

namespace ZIP
{
	class Unzip
	{
	public:
		Unzip();
		~Unzip(void);

		bool open( const char* filename );
		void close();
		bool isOpen();

		bool openEntry( const char* filename );
		void closeEntry();
		bool isOpenEntry();
		unsigned int getEntrySize();

		const std::vector<std::string>& getFilenames();
		const std::vector<std::string>& getFolders();

		Unzip& operator>>( std::ostream& os );

	private:
		void readEntries();

	private:
		unzFile			zipFile_;
		bool			entryOpen_;

		std::vector<std::string> files_;
		std::vector<std::string> folders_;
	};
};
#endif
