#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __DB_ENGINE_RDB_H__
#define __DB_ENGINE_RDB_H__ 1

#include "dbEngine.h"
#include <unordered_map>
#include <rocksdb/db.h>
#include <boost/lexical_cast.hpp>
#include "../../SimpleIni.h"

/*
storage class for rocks db
*/

class dbEngineRDB : public dbEngine
{
	private:
		std::unordered_map<std::string, std::string>	m_umapSettings;
		rocksdb::DB										*m_pDB;
		rocksdb::WriteBatch								*m_pBatch;
		rocksdb::Status									m_dbStatus;

		// template for getting values of different type
		template<typename T>
		T getT(std::string strKey, T tDefault)
		{
			std::string strTmp;
			m_dbStatus = m_pDB->Get(rocksdb::ReadOptions(), strKey, &strTmp);

			if (m_dbStatus.ok())
				return boost::lexical_cast<T>(strTmp);
			else
				return tDefault;
		}

	public:
														dbEngineRDB();
														~dbEngineRDB();
		virtual bool									initialize(std::unordered_map<std::string, std::string>* umapSettings);

		// functions for writing, getting single entries
		virtual bool									write(std::string strKey, std::string strValue, std::string strEnv = "");
		virtual std::string								get(std::string strKey, std::string strDefault = "", std::string strEnv = "") {	return getT(strKey, strDefault); };
		virtual unsigned int							get(std::string strKey, unsigned int uiDefault = 0, std::string strEnv = "") { return getT(strKey, uiDefault); };
		virtual bool									get(std::string strKey, bool bDefault = false, std::string strEnv = "") { return getT(strKey, bDefault); };

		// functions for batch writing
		virtual void									batchStart();
		virtual void									batchAddStatement(std::string strKey, std::string strValue, std::string strEnv = "");		// strEnv is used for dbEngines that are not key/value based. e.g. mysql where tables can be defined and used
		virtual void									batchFinalize();
};

#ifndef _DEBUG
// NullLogger class for rocksdb on release
class RocksDBNullLogger : public rocksdb::Logger {
public:
	using rocksdb::Logger::Logv;
	virtual void Logv(const char* format, va_list hap) override {}
	virtual size_t GetLogFileSize() const override { return 0; }
};
#endif

#endif