/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __ZIP_ZIP_H__
#define __ZIP_ZIP_H__ 1

#include <string>
#include <vector>
#include <iostream>
#include <time.h>

#include "ioapi.h"
#include "zlib_zip.h"

namespace ZIP
{
	class Zip
	{
	public:
		Zip();
		~Zip(void);

		bool open( const char* filename, bool append = false );
		void close();
		bool isOpen();

		bool addEntry( const char* filename );
		void closeEntry();
		bool isOpenEntry();

		Zip& operator<<( std::istream& is );

	private:
		void getTime(tm_zip& tmZip);

	private:
		zipFile			zipFile_;
		bool			entryOpen_;
	};

};
#endif
