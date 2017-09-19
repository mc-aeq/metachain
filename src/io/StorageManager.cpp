/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "StorageManager.h"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "rocksdb/db.h"
#include "db/rdb.h"
#include "db/mysql.h"

StorageManager::StorageManager(MetaChain *mc)
	: m_pMC(mc),
	m_pSubChainManager(NULL)
{
}

StorageManager::~StorageManager()
{
	// remove the lock file in the data directory
	remove((m_pathDataDirectory /= LOCK_FILE).string().c_str());

	// close the raw output file
	if(m_pMC->isFN() )
		m_streamRaw.close();

	// delete the subchain manager
	if(m_pSubChainManager)
		delete m_pSubChainManager;

	// close the meta info db
	delete m_pMetaDB;
}

bool StorageManager::initialize(CSimpleIniA* iniFile)
{
	// get the data directory
	m_pathDataDirectory = iniFile->GetValue("data", "data_dir", "data");
	if( m_pathDataDirectory.is_relative() )
		m_pathDataDirectory = boost::filesystem::current_path() / iniFile->GetValue("data", "data_dir", "data");

	// security check if data directory exists, if it doesnt, create one
	if (!boost::filesystem::exists(m_pathDataDirectory) || !boost::filesystem::is_directory(m_pathDataDirectory))
	{
		LOG_ERROR("Data directory doesn't exist: " + m_pathDataDirectory.string(), "SM");
		LOG_ERROR("Creating data directory and resuming operation", "SM");
		if (!boost::filesystem::create_directory(m_pathDataDirectory))
		{
			LOG_ERROR("Couldn't create Data directory, exiting.", "SM");
			return false;
		}
	}

	// security check if there is a lock file (crashed instance or double starting in the same data directory)
	if (boost::filesystem::exists(m_pathDataDirectory / LOCK_FILE))
	{
		LOG_ERROR((m_pathDataDirectory / LOCK_FILE).string() + " file exists within the data directory. Instance crashed or started twice?", "SM");
		return false;
	}
	else
	{
		// create the lock file with the current timestamp info
#ifdef _DEBUG
		LOG_DEBUG("writing lock file: " + (m_pathDataDirectory / LOCK_FILE).string(), "SM");
#endif

		std::ofstream streamOut;
		streamOut.open((m_pathDataDirectory / LOCK_FILE).string(), std::ios_base::out | std::ios_base::trunc);

		// create a date/time string
		std::basic_stringstream<char> wss;
		wss.imbue(std::locale(std::wcout.getloc(), new boost::posix_time::wtime_facet(L"%Y.%m.%d %H:%M:%S")));
		wss << boost::posix_time::second_clock::universal_time();

		// write the timestamp
		streamOut << "# generation of the file: " << wss.str() << std::endl;
		streamOut.close();

#ifdef _DEBUG
		LOG_DEBUG("done writing lock file: " + (m_pathDataDirectory / LOCK_FILE).string(), "SM");
#endif
	}

	// fireing up the meta db
	rocksdb::Options dbOptions;
	dbOptions.create_if_missing = true;
#ifndef _DEBUG
	dbOptions.info_log = std::make_shared<RocksDBNullLogger>();
	dbOptions.keep_log_file_num = 1;
#endif

	rocksdb::Status dbStatus = rocksdb::DB::Open(dbOptions, (m_pathDataDirectory / META_DB_FOLDER).string(), &m_pMetaDB);
	if (!dbStatus.ok())
	{
		LOG_ERROR("Can't open the meta information database: " + dbStatus.ToString(), "SM");
		return false;
	}
	
	// if it's a new meta info db, we set the initialized flag and add some basic stuff
	bool bInitialized = getMetaValueBool("initialized", false);
	if (!bInitialized)
	{
		rocksdb::WriteBatch batch;
		batch.Put("initialized", "1");
		batch.Put(MC_HEIGHT, "0");
		batch.Put(LAST_RAW_FILE, "0");
		batch.Put("TestNet", m_pMC->isTestNet() ? "1" : "0");
		m_pMetaDB->Write(rocksdb::WriteOptions(), &batch);		
	}	

	// initialize the subchain manager & serializing it into our metaDB
	LOG("Initializing SubChainManager", "SM");
	if( bInitialized )
		// get our subchains
		MetaDeserialize("SCM", &m_pSubChainManager, &csSubChainManager);
	else
	{
		m_pSubChainManager = new MCP02::SubChainManager();
		m_pSubChainManager->init();
		MetaSerialize("SCM", m_pSubChainManager, &csSubChainManager);
	}

	// check whether the meta db matches our testnet value or not (we don't accept meta DBs without testnet flag for testnet use and vice versa)
	if (getMetaValueBool("TestNet", false) != m_pMC->isTestNet())
	{
		LOG_ERROR("MetaDB and node.ini configurations about TestNet don't match. Terminating for security reasons", "SM");
		return false;
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
	m_pDB->initialize(iniFile, &bInitialized);

	// check for the raw directory (this is only needed when we're in FN mode)
	if (m_pMC->isFN())
	{
		m_pathRawDirectory = m_pathDataDirectory / iniFile->GetValue("data", "raw_dir", "raw");
		if(!boost::filesystem::exists(m_pathRawDirectory) || !boost::filesystem::is_directory(m_pathRawDirectory))
		{
			LOG_ERROR("Raw directory doesn't exist: " + m_pathRawDirectory.string(), "SM");
			
			// if it's a new node, we simply create the raw directory.
			// if the node has already worked and has data stored in the db engine, then we have to quit since we're missing the raw files
			// in this case either the user has to restore the raw files or delete the db files in order to sync new
			if (!bInitialized)
			{
				LOG_ERROR("Creating new raw directory since it's a new node", "SM");
				if (!boost::filesystem::create_directory(m_pathRawDirectory))
				{
					LOG_ERROR("Couldn't create raw directory: " + m_pathRawDirectory.string(), "SM");
					return false;
				}
			}
			else
			{
				LOG_ERROR("The node seems to already have worked with this database, but the raw files are missing.", "SM");
				LOG_ERROR("Either remove the database and sync new or restore the raw files in order to continue.", "SM");
				return false;
			}
		}

		// get our max file size from the ini (the value there is in M, so we need to multiply it to get bytes
		m_uimRawFileSizeMax = iniFile->GetLongValue("data", "raw_filesplit", 100) * 1048576;

		// finally open the last raw file for writing
		openRawFile();
	}

	// everything smooth
	return true;
}

bool StorageManager::openRawFile()
{
	// close the stream if opened
	{
		LOCK(m_csRaw);
		if (m_streamRaw.is_open())
			m_streamRaw.close();
	}

	// get the raw file counter from the meta info db
	std::string strFileCounter;
	m_pMetaDB->Get(rocksdb::ReadOptions(), LAST_RAW_FILE, &strFileCounter);
	unsigned int uiRawFileCounter = boost::lexical_cast<unsigned int>(strFileCounter);

	// open our fstream for raw output
	{
		LOCK(m_csRaw);
		std::ostringstream ssFileName;
		ssFileName << std::setw(8) << std::setfill('0') << uiRawFileCounter;
		m_fileRaw = m_pathRawDirectory / ssFileName.str();

		// if the file exists we get the size to calculate our splitting. If it doesn't we start with 0
		if (boost::filesystem::exists(m_fileRaw))
			m_uimRawFileSize = boost::filesystem::file_size(m_fileRaw);
		else
			m_uimRawFileSize = 0;

		m_streamRaw.open(m_fileRaw.string() + RAW_FILEENDING, std::ios_base::app);
		if (!m_streamRaw.is_open())
		{
			LOG_ERROR("Unable to open raw file: " + m_fileRaw.string(), "SM");
			return false;
		}
	}
	return true;
}

void StorageManager::writeRaw(unsigned int uiBlockNumber, unsigned int uiLength, void *raw)
{
	if (m_pMC->isFN())
	{
		// write the output in the file
		{
			LOCK(m_csRaw);
			m_streamRaw.write((char *)raw, uiLength);
		}

		// write some meta data
		rocksdb::WriteBatch batch;
		batch.Put("mbl." + boost::lexical_cast<std::string>(uiBlockNumber) + ".file", m_fileRaw.string());
		batch.Put("mbl." + boost::lexical_cast<std::string>(uiBlockNumber) + ".offset", boost::lexical_cast<std::string>(m_uimRawFileSize) );
		batch.Put("mbl." + boost::lexical_cast<std::string>(uiBlockNumber) + ".length", boost::lexical_cast<std::string>(uiLength));
		m_pMetaDB->Write(rocksdb::WriteOptions(), &batch);

		m_uimRawFileSize += uiLength;


		// if the new file size is bigger than our maximum value, we close this file and open a new one for output
		if (m_uimRawFileSize >= m_uimRawFileSizeMax)
		{
			LOCK(m_csRaw);

			// increment raw file counter
			std::string strFileCounter;
			m_pMetaDB->Get(rocksdb::ReadOptions(), LAST_RAW_FILE, &strFileCounter);
			unsigned int uiRawFileCounter = boost::lexical_cast<unsigned int>(strFileCounter) + 1;
			m_pMetaDB->Put(rocksdb::WriteOptions(), LAST_RAW_FILE, boost::lexical_cast<std::string>(uiRawFileCounter));

			if (openRawFile())
				m_uimRawFileSize = 0;
		}
	}
}

bool StorageManager::getMetaValueBool(std::string strKey, bool bDefault )
{
	rocksdb::Status dbStatus;
	std::string strTmp;
	dbStatus = m_pMetaDB->Get(rocksdb::ReadOptions(), strKey, &strTmp);

	if (dbStatus.ok())
		return boost::lexical_cast<bool>(strTmp);
	else
		return bDefault;
}