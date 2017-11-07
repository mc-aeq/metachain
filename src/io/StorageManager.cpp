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
#include "../tinyformat.h"

StorageManager::StorageManager(MetaChain *mc)
	: m_pMC(mc),
	m_pSubChainManager(NULL)
{
}

StorageManager::~StorageManager()
{
	// serialize our loaded subchains
	MetaSerialize("SCM", m_pSubChainManager, &csSubChainManager);
	MetaSerialize("smSC", &m_umapSC, &m_csUmapSC);

	// remove the lock file in the data directory
	remove((m_pathDataDirectory /= LOCK_FILE).string().c_str());

	// delete the subchain manager
	if(m_pSubChainManager)
		delete m_pSubChainManager;

	// close the meta info db
	delete m_pMetaDB;
}

bool StorageManager::initialize(CSimpleIniA* iniFile)
{
	// store the reference to the ini file for later usage
	m_pINI = iniFile;

	// get the data and raw directory
	m_pathDataDirectory = iniFile->GetValue("data", "data_dir", "data");
	if( m_pathDataDirectory.is_relative() )
		m_pathDataDirectory = boost::filesystem::current_path() / iniFile->GetValue("data", "data_dir", "data");

	m_pathRawDirectory = iniFile->GetValue("data", "raw_dir", "raw");
	if (m_pathRawDirectory.is_relative())
		m_pathRawDirectory = m_pathDataDirectory / m_pathRawDirectory;

	// security check if certain directory exist, if not, create them
	if (!boost::filesystem::exists(m_pathDataDirectory) || !boost::filesystem::is_directory(m_pathDataDirectory))
	{
		LOG_ERROR("Data directory doesn't exist: " + m_pathDataDirectory.string(), "SM");
		LOG_ERROR("Creating data directory and resuming operation", "SM");
		if (!boost::filesystem::create_directories(m_pathDataDirectory))
		{
			LOG_ERROR("Couldn't create Data directory, exiting.", "SM");
			return false;
		}
	}

	if (!boost::filesystem::exists(m_pathRawDirectory) || !boost::filesystem::is_directory(m_pathRawDirectory))
	{
		LOG_ERROR("Raw directory doesn't exist: " + m_pathRawDirectory.string(), "SM");
		LOG_ERROR("Creating raw directory and resuming operation", "SM");
		if (!boost::filesystem::create_directories(m_pathRawDirectory))
		{
			LOG_ERROR("Couldn't create raw directory, exiting.", "SM");
			return false;
		}
	}

	boost::filesystem::path pathMCMetaInfo = m_pathDataDirectory / META_DB_FOLDER / MC_META_DB_FOLDER;
	if (!boost::filesystem::exists(pathMCMetaInfo) || !boost::filesystem::is_directory(pathMCMetaInfo))
	{
		LOG_ERROR("MC MetaInformation directory doesn't exist: " + pathMCMetaInfo.string(), "SM");
		LOG_ERROR("Creating MC MetaInformation directory and resuming operation", "SM");
		if (!boost::filesystem::create_directories(pathMCMetaInfo))
		{
			LOG_ERROR("Couldn't create MC MetaInformation directory, exiting.", "SM");
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

	rocksdb::Status dbStatus = rocksdb::DB::Open(dbOptions, pathMCMetaInfo.string(), &m_pMetaDB);
	if (!dbStatus.ok())
	{
		LOG_ERROR("Can't open the meta information database: " + dbStatus.ToString(), "SM");
		return false;
	}
	
	// if it's a new meta info db, we set the initialized flag and add some basic stuff
	bool bInitialized = getMetaValue("initialized", false);
	if (!bInitialized)
	{
		rocksdb::WriteBatch batch;
		batch.Put("initialized", "1");
		batch.Put(MC_NEXT_CI, "0");
		batch.Put("TestNet", m_pMC->isTestNet() ? "1" : "0");
		m_pMetaDB->Write(rocksdb::WriteOptions(), &batch);		
	}	

	// initialize the subchain manager & serializing it into our metaDB
	LOG("Initializing SubChainManager", "SM");
	if (bInitialized)
	{
		// get our subchains
		MetaDeserialize("SCM", &m_pSubChainManager, &csSubChainManager);
		MetaDeserializeObj("smSC", &m_umapSC, &m_csUmapSC );
	}
	else
	{
		m_pSubChainManager = new MCP02::SubChainManager();
		if (!m_pSubChainManager->init())
		{
			LOG_ERROR("SCM didn't initialize correctly. Terminating for security reasons", "SM");
			return false;
		}
		MetaSerialize("SCM", m_pSubChainManager, &csSubChainManager);
	}

	// check whether the meta db matches our testnet value or not (we don't accept meta DBs without testnet flag for testnet use and vice versa)
	if (getMetaValue("TestNet", false) != m_pMC->isTestNet())
	{
		LOG_ERROR("MetaDB and node.ini configurations about TestNet don't match. Terminating for security reasons", "SM");
		return false;
	}

	// print some output about the loaded states of the SCs and PoPs
	m_pSubChainManager->printSCInfo();
	m_pSubChainManager->printPoPInfo();

	// print some output about the smSC
	LOG("Number of loaded smSC: " + std::to_string(m_umapSC.size()), "SM");
	for (auto &it : m_umapSC)
	{
		openRawFile(it.first);
		LOG(strprintf("#%u - %s", it.first+1, it.second.filePath.string()), "SM");
		LOG(strprintf("    Raw File Counter: %u", it.second.uiRawFileCounter), "SM");
		LOG(strprintf("    Current Raw File Size: %u", it.second.uimRawFileSize), "SM");
	}

	// everything smooth
	return true;
}

bool StorageManager::openRawFile(unsigned short usChainIdentifier)
{
	// get an iterator for faster access
	auto it = m_umapSC.find(usChainIdentifier);
	if (it == m_umapSC.end())
	{
		LOG_ERROR("Can't find entry to open raw file for SC: " + std::to_string(usChainIdentifier), "SM");
		return false;
	}

	// close the stream if opened
	LOCK(it->second.csAccess);
	if (it->second.streamRaw.is_open())
		it->second.streamRaw.close();
		
	// open our fstream for raw output
	std::ostringstream ssFileName;
	ssFileName << std::setw(8) << std::setfill('0') << it->second.uiRawFileCounter;
	it->second.filePath = m_pathRawDirectory / std::to_string(usChainIdentifier) / (ssFileName.str() + RAW_FILEENDING);

	// if the file exists we get the size to calculate our splitting. If it doesn't we start with 0
	if (boost::filesystem::exists(it->second.filePath))
		it->second.uimRawFileSize = boost::filesystem::file_size(it->second.filePath);
	else
		it->second.uimRawFileSize = 0;

	it->second.streamRaw.open(it->second.filePath.string(), std::ios_base::app);
	if (!it->second.streamRaw.is_open())
	{
		LOG_ERROR("Unable to open raw file: " + it->second.filePath.string(), "SM");
		return false;
	}

	return true;
}

bool StorageManager::writeRaw(unsigned short usChainIdentifier, unsigned int uiLength, void *raw)
{
	// get the SC dbEngine
	dbEngine *db = m_pSubChainManager->getDBEngine(usChainIdentifier);
	if (db == nullptr)
	{
		LOG_ERROR("Can't open db for SC to store information: " + std::to_string(usChainIdentifier), "SM");
		return false;
	}

	// get the last block number and increment it to our current block
	unsigned int uiBlockNumber = db->get("last_block", (unsigned int)0) + 1;

	if (m_pMC->isFN())
	{
		// get an iterator for faster access
		auto it = m_umapSC.find(usChainIdentifier);
		if (it == m_umapSC.end())
		{
			LOG_ERROR("Can't find entry to store raw block for SC: " + std::to_string(usChainIdentifier), "SM");
			return false;
		}

		// check whether we have an open stream or not
		if (!it->second.streamRaw.is_open())
		{
			if (!openRawFile(usChainIdentifier))
			{
				LOG_ERROR("Can't open raw stream for output: " + std::to_string(usChainIdentifier), "SM");
				return false;
			}
		}

		// write the output in the file
		{
			LOCK(it->second.csAccess);
			it->second.streamRaw.write((char *)raw, uiLength);
			it->second.streamRaw.flush();
			it->second.uimRawFileSize += uiLength;
		}

		// write some meta data
		db->batchStart();
		db->batchAddStatement( "mbl." + std::to_string(uiBlockNumber) + ".file", it->second.filePath.string());
		db->batchAddStatement( "mbl." + std::to_string(uiBlockNumber) + ".offset", std::to_string(it->second.uimRawFileSize));
		db->batchAddStatement( "mbl." + std::to_string(uiBlockNumber) + ".length", std::to_string(uiLength));
		db->batchFinalize();

		// if the new file size is bigger than our maximum value, we close this file and open a new one for output
		if (it->second.uimRawFileSize >= m_uimRawFileSizeMax)
		{
			LOCK(it->second.csAccess);
			it->second.uiRawFileCounter++;

			if (!openRawFile(usChainIdentifier))
				return false;
		}
	}

	// write the current block height into the db
	db->write("last_block", std::to_string(uiBlockNumber));

	return true;
}

uint16_t StorageManager::getChainIdentifier(std::string strChainIdentifier)
{
	return m_pSubChainManager->getChainIdentifier(strChainIdentifier);
};


std::string	StorageManager::getChainIdentifier(uint16_t uint16ChainIdentifier)
{
	return m_pSubChainManager->getChainIdentifier(uint16ChainIdentifier);
};

bool StorageManager::registerSC(unsigned short usChainIdentifier)
{
	LOCK(m_csUmapSC);

	// check if the SC already is registered
	if (m_umapSC.count(usChainIdentifier) != 0)
	{
		LOG_ERROR("Can't register SC since it already exists: " + usChainIdentifier, "SM");
		return false;
	}

	// push back the new SC into our map (using [] on not created element will create a new element)
	m_umapSC[usChainIdentifier].uimRawFileSize = 0;
	m_umapSC[usChainIdentifier].uiRawFileCounter = 0;

	// check for the raw directory (this is only needed when we're in FN mode)
	if (m_pMC->isFN())
	{
		boost::filesystem::path pathRaw = m_pathRawDirectory / std::to_string(usChainIdentifier);
		if (!boost::filesystem::exists(pathRaw) || !boost::filesystem::is_directory(pathRaw))
		{
			LOG("Creating Raw Directory: " + pathRaw.string(), "SM");
			if (!boost::filesystem::create_directory(pathRaw))
			{
				LOG_ERROR("Couldn't create raw directory: " + pathRaw.string(), "SM");
				return false;
			}
		}

		// open the raw file and initialize the streams for writing
		openRawFile(usChainIdentifier);
	}

	return true;
}

dbEngine *StorageManager::createDBEngine(unsigned short usChainIdentifier)
{
	std::string strEngine = m_pINI->GetValue("data", "storage_engine", "rdb");

	if (strEngine == "rdb")
	{
		dbEngineRDB *db = new dbEngineRDB();
		std::unordered_map<std::string, std::string> settings;
		settings["path"] = (m_pathDataDirectory / META_DB_FOLDER / std::to_string(usChainIdentifier)).string();
		db->initialize(&settings);
		return db;
	}
#ifdef DB_ENGINE_MYSQL
	else if(strEngine == "mysql")
		return new dbEngineMYSQL();
#endif
	else
	{
		LOG_ERROR("Unknown DB Engine: " + strEngine, "SM");
		return nullptr;
	}
}

