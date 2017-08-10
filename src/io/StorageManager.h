#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __STORAGEMANAGER_H__
#define __STORAGEMANAGER_H__ 1

#include <string>
#include <fstream>
#include "rocksdb/db.h"
#include <boost/filesystem/path.hpp>
#include "../MetaChain.h"
#include "../external/SimpleIni.h"
#include "db/dbEngine.h";

class MetaChain;

/*
this class handles the storage of the blocks and their metadata.
the functionality of this class differs in mode FN and CL, it also instances the db backend configured in the .ini File
*/

class StorageManager
{
	private:
		MetaChain							*m_pMC;
		// the dbEngine defines where block informations will be stored.
		// this won't store the full blocks, they are in raw format stored somewhere else.
		// the type of dbEngine can be chosen in the ini
		dbEngine							*m_pDB;
		// the rdb specified here stores meta information
		rocksdb::DB							*m_pMetaDB;

		bool								m_bModeFN;
		std::string							m_strDataDirectory;
		boost::filesystem::path				m_pathDataDirectory;
		boost::filesystem::path				m_pathRawDirectory;

		// variables for raw file output
		cCriticalSection					m_csRaw;
		unsigned int						m_uiRawFileCounter;
		std::ofstream						m_streamRaw;
		boost::filesystem::path				m_fileRaw;
		uintmax_t							m_uimRawFileSize;
		uintmax_t							m_uimRawFileSizeMax;
public:
											StorageManager(MetaChain *mc);
											~StorageManager();
	bool									initialize(CSimpleIniA* iniFile);
	void									writeRaw(unsigned int uiLength, void *raw);
};

#endif