#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __STORAGEMANAGER_H__
#define __STORAGEMANAGER_H__ 1

#include <string>
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
		dbEngine							*m_pDB;

		bool								m_bModeFN;
		std::string							m_strDataDirectory;
		boost::filesystem::path				m_pathDataDirectory;
		boost::filesystem::path				m_pathRawDirectory;

public:
											StorageManager(MetaChain *mc);
											~StorageManager();
	bool									initialize(CSimpleIniA* iniFile);
};

#endif