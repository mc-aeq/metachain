/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "rdb.h"
#include <rocksdb/db.h>
#include "../../logger.h"

// define a RELEASE macro that deletes a pointer if it's set
#define RELEASE(x) if(x) {delete x; x = nullptr;}

dbEngineRDB::dbEngineRDB()
	: m_pDB(nullptr),
	m_pBatch(nullptr)
{
}

dbEngineRDB::~dbEngineRDB()
{

}

// initialize (open) the rdb engine
// required parameters in the unordered_map:
// "path": needs to contain the full path to the rdb folder where the db will be stored
bool dbEngineRDB::initialize(std::unordered_map<std::string, std::string>* umapSettings)
{
	try
	{
		LOG("Initializing RDB Storage Engine, Path: " + umapSettings->at("path"), "SE-RDB");

		// fireing up the meta db
		rocksdb::Options dbOptions;
		dbOptions.create_if_missing = true;
#ifndef _DEBUG
		dbOptions.info_log = std::make_shared<RocksDBNullLogger>();
		dbOptions.keep_log_file_num = 1;
#endif

		rocksdb::Status dbStatus = rocksdb::DB::Open(dbOptions, umapSettings->at("path"), &m_pDB);
		if (!dbStatus.ok())
		{
			LOG_ERROR("Can't open database: " + dbStatus.ToString(), "SE-RDB");
			return false;
		}

		// store the umap for later references
		m_umapSettings = *umapSettings;

		return true;
	}
	catch (std::out_of_range &e)
	{
		LOG_ERROR("Unable to initialize RDB Engine due to missing configuration params. Error: " + (std::string)e.what(), "RDB-E");
		return false;
	}
}

bool dbEngineRDB::write(std::string strKey, std::string strValue, std::string strEnv)
{
	m_dbStatus = m_pDB->Put(rocksdb::WriteOptions(), strKey, strValue);

	if (m_dbStatus.ok())
		return true;
	else
	{
		LOG_ERROR("Writing to RDB " + m_umapSettings.at("path") + " didn't work: " + m_dbStatus.ToString(), "RDB-E");
		return false;
	}
}

void dbEngineRDB::batchStart()
{
	RELEASE(m_pBatch);

	m_pBatch = new rocksdb::WriteBatch;
}

void dbEngineRDB::batchAddStatement(std::string strKey, std::string strValue, std::string strEnv)
{
	m_pBatch->Put( strKey, strValue );
}

void dbEngineRDB::batchFinalize()
{
	if (m_pBatch == nullptr)
		return;

	m_pDB->Write(rocksdb::WriteOptions(), m_pBatch);
	RELEASE(m_pBatch);
}