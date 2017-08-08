/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "StorageManager.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include "db/bdb.h"
#include "db/mysql.h"

StorageManager::StorageManager(MetaChain *mc)
	: m_pMC(mc)
{
}

StorageManager::~StorageManager()
{
	// remove the lock file in the data directory
	remove(std::string(m_pathDataDirectory.string() + LOCK_FILE).c_str());
}

bool StorageManager::initialize(CSimpleIniA* iniFile)
{
	// getting our mode, since we only have FN and CL, we use a flag
	m_bModeFN = (iniFile->GetValue("general", "mode", "fn") == "fn" ? true : false);
	LOG("initializing this node to run in mode: " + (std::string)(m_bModeFN ? "FN" : "CL"), "SM");

	// get the data directory and sanitize it
	m_strDataDirectory = iniFile->GetValue("data", "data_dir", "data");
	boost::trim_right_if(m_strDataDirectory, boost::is_any_of("/"));
	m_pathDataDirectory = boost::filesystem::current_path();
	m_pathRawDirectory = m_pathDataDirectory /= m_strDataDirectory;
	
	// security check if data directory exists, if it doesnt, create one
	if (!boost::filesystem::exists(m_pathDataDirectory) || !boost::filesystem::is_directory(m_pathDataDirectory))
	{
		LOG_ERROR("Data directory doesn't exist: " + m_pathDataDirectory.string(), "SM");
		LOG_ERROR("Creating data directory and resuming operation", "SM");
		boost::filesystem::create_directory(m_pathDataDirectory);
	}

	// security check if there is a lock file (crashed instance or double starting in the same data directory)
	if (boost::filesystem::exists(m_pathDataDirectory.string() + LOCK_FILE))
	{
		LOG_ERROR((std::string)LOCK_FILE + " file exists within the data directory. Instance crashed or started twice?", "SM");
		return false;
	}

	// creating the dbEngine instance
	bool bNewNode = true;
	std::string strDBEngine = iniFile->GetValue("data", "storage_engine", "dbd");
	if (strDBEngine == "dbd")
		m_pDB = new dbEngineBDB();
	else if (strDBEngine == "mysql")
		m_pDB = new dbEngineMYSQL();
	else
	{
		LOG_ERROR("Unknown DB Engine: " + strDBEngine, "SM");
		return false;
	}
	m_pDB->initialize(iniFile, &bNewNode);

	// check for the raw directory (this is only needed when we're in FN mode)
	if (m_bModeFN)
	{
		m_pathRawDirectory /= iniFile->GetValue("data", "raw_dir", "raw");
		if(!boost::filesystem::exists(m_pathRawDirectory) || !boost::filesystem::is_directory(m_pathRawDirectory))
		{
			LOG_ERROR("Raw directory doesn't exist: " + m_pathRawDirectory.string(), "SM");
			
			// if it's a new node, we simply create the raw directory.
			// if the node has already worked and has data stored in the db engine, then we have to quit since we're missing the raw files
			// in this case either the user has to restore the raw files or delete the db files in order to sync new
			if (bNewNode)
			{
				LOG_ERROR("Creating new raw directory since it's a new node", "SM");
				boost::filesystem::create_directory(m_pathRawDirectory);
			}
			else
			{
				LOG_ERROR("The node seems to already have worked with this database, but the raw files are missing.", "SM");
				LOG_ERROR("Either remove the database and sync new or restore the raw files in order to continue.", "SM");
				return false;
			}
		}

	}

	// everything smooth
	return true;
}