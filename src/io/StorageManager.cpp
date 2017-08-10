/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "StorageManager.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include "rocksdb/db.h"
#include "db/rdb.h"
#include "db/mysql.h"

StorageManager::StorageManager(MetaChain *mc)
	: m_pMC(mc)
{
}

StorageManager::~StorageManager()
{
	// remove the lock file in the data directory
	remove(std::string(m_pathDataDirectory.string() + LOCK_FILE).c_str());

	// close the raw output file
	if( m_bModeFN)
		m_streamRaw.close();

	// close the meta info db
	delete m_pMetaDB;
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
	else
	{
		// create the lock file with the current timestamp info
#ifdef _DEBUG
		LOG_DEBUG("writing lock file: " + (std::string)LOCK_FILE, "SM");
#endif

		std::ofstream streamOut;
		streamOut.open(m_pathDataDirectory.string() + LOCK_FILE, std::ios_base::out | std::ios_base::trunc);

		// create a date/time string
		std::basic_stringstream<char> wss;
		wss.imbue(std::locale(std::wcout.getloc(), new boost::posix_time::wtime_facet(L"%Y.%m.%d %H:%M:%S")));
		wss << boost::posix_time::second_clock::universal_time();

		// write the timestamp
		streamOut << "# generation of the file: " << wss.str() << std::endl;
		streamOut.close();

#ifdef _DEBUG
		LOG_DEBUG("done writing lock file: " + (std::string)LOCK_FILE, "SM");
#endif
	}

	// fireing up the meta db
	rocksdb::Options dbOptions;
	dbOptions.create_if_missing = true;
	rocksdb::Status dbStatus = rocksdb::DB::Open(dbOptions, m_pathDataDirectory.string() + "meta", &m_pMetaDB);
	if (!dbStatus.ok())
	{
		LOG_ERROR("Can't open the meta information database: " + dbStatus.ToString(), "SM");
		return false;
	}

	// checking the status of the meta info db, is it new?
	std::string strInitialized = "0";
	dbStatus = m_pMetaDB->Get(rocksdb::ReadOptions(), "initialized", &strInitialized);
	bool bNewNode = !boost::lexical_cast<bool>(strInitialized);

	// if it's a new meta info db, we set the initialized flag and add some basic stuff
	if (bNewNode)
	{
		rocksdb::WriteBatch batch;
		batch.Put("initialized", "1");
		batch.Put("mc.height", "0");
		batch.Put("raw.file", "0");
		m_pMetaDB->Write(rocksdb::WriteOptions(), &batch);
	}

	// creating the dbEngine instance
	std::string strDBEngine = iniFile->GetValue("data", "storage_engine", "rdb");
	if (strDBEngine == "rdb")
		m_pDB = new dbEngineRDB();
#ifdef DB_ENGINE_MYSQL
	else if (strDBEngine == "mysql")
		m_pDB = new dbEngineMYSQL();
#endif
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

		// get the raw file counter from the meta info db
		std::string strFileCounter;
		m_pMetaDB->Get(rocksdb::ReadOptions(), "raw.file", &strFileCounter);
		m_uiRawFileCounter = boost::lexical_cast<unsigned int>(strFileCounter);

		// open our fstream for raw output
		{
			LOCK(m_csRaw);
			std::ostringstream ssFileName;
			ssFileName << std::setw(6) << std::setfill('0') << m_uiRawFileCounter;

			m_fileRaw = m_pathRawDirectory;
			m_fileRaw /= ssFileName.str();
			m_uimRawFileSize = boost::filesystem::file_size(m_fileRaw);

			m_streamRaw.open(ssFileName.str() + RAW_FILEENDING, std::ios_base::app);
		}

		// get our max file size from the ini (the value there is in M, so we need to multiply it to get bytes
		m_uimRawFileSizeMax = iniFile->GetLongValue("data", "raw_filesplit", 100) * 1048576;
	}

	// everything smooth
	return true;
}

void StorageManager::writeRaw(unsigned int uiLength, void *raw)
{
	if (m_bModeFN)
	{
		// write the output in the file
		{
			LOCK(m_csRaw);
			m_streamRaw.write((char *)raw, uiLength);
		}

		m_uimRawFileSize += uiLength;

		// if the new file size is bigger than our maximum value, we close this file and open a new one for output
		if (m_uimRawFileSize >= m_uimRawFileSizeMax)
		{
			LOCK(m_csRaw);
			m_uiRawFileCounter++;
			std::ostringstream ssFileName;
			ssFileName << std::setw(6) << std::setfill('0') << m_uiRawFileCounter;

			m_fileRaw = m_pathRawDirectory;
			m_fileRaw /= ssFileName.str();
			m_uimRawFileSize = 0;

			m_streamRaw.close();
			m_streamRaw.open(ssFileName.str() + RAW_FILEENDING, std::ios_base::trunc | std::ios_base::app);

			m_pMetaDB->Put(rocksdb::WriteOptions(), "raw.file", boost::lexical_cast<std::string>(m_uiRawFileCounter));
		}
	}
}