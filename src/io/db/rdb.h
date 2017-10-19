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
#include "../../SimpleIni.h"

/*
storage class for rocks db
*/

class dbEngineRDB : public dbEngine
{
	private:
		rocksdb::DB										*m_pDB;
		rocksdb::WriteBatch								*m_pBatch;

	public:
														dbEngineRDB();
														~dbEngineRDB();
		virtual bool									initialize(std::unordered_map<std::string, std::string>* umapSettings);

		// functions for batch writing
		virtual void									batchStart();
		virtual void									batchAddStatement(std::string strKey, std::string strValue, std::string strEnv = "");		// strEnv is used for dbEngines that are not key/value based. e.g. mysql where tables can be defined and used
		virtual void									batchFinalize();
};

#endif