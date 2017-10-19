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
#include <unordered_map>
#include <rocksdb/db.h>
#include <boost/filesystem/path.hpp>
#include "../MetaChain.h"
#include "../SimpleIni.h"
#include "db/dbEngine.h"
#include "../MCP/MCP02/SubChainManager.h"
#include "smSC.h"

// forward decl
class MetaChain;
namespace MCP02 { class SubChainManager; };

/*
this class handles the storage of the blocks and their metadata.
the functionality of this class differs in mode FN and CL, it also instances the db backend configured in the .ini File
*/

class StorageManager
{
	private:
		MetaChain										*m_pMC;
		CSimpleIniA										*m_pINI;

		// subchains in the metachains with their functions
		MCP02::SubChainManager							*m_pSubChainManager;

		// meta db and convenience functions
		rocksdb::DB										*m_pMetaDB;

		template<class obj>
		void MetaSerialize(std::string strKey, obj *ptr, cCriticalSection *cs)
		{
			LOCK(cs);
			std::stringstream stream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
			boost::archive::binary_oarchive oa(stream, boost::archive::no_header | boost::archive::no_tracking);
			oa << ptr;
			if (!m_pMetaDB->Put(rocksdb::WriteOptions(), strKey, rocksdb::Slice(stream.str().data(), stream.tellp())).ok())
				LOG_ERROR("Unable to serialize " + strKey, "SM");
#ifdef _DEBUG
			else
				LOG_DEBUG("Serialized " + strKey, "SM");
#endif
		};

		template<class obj>
		void MetaDeserialize(std::string strKey, obj *ptr, cCriticalSection *cs)
		{
			LOCK(cs);
			// we delete the object so that the memory is freed
			if (*ptr)
				delete *ptr;

			std::string strTmp;
			if (m_pMetaDB->Get(rocksdb::ReadOptions(), strKey, &strTmp).ok())
			{
				std::stringstream stream(strTmp);
				boost::archive::binary_iarchive ia(stream, boost::archive::no_header | boost::archive::no_tracking);
				// the >> operator creates a new object and the double pointer updates the reference
				ia >> *ptr;
#ifdef _DEBUG
				LOG_DEBUG("Deserialized " + strKey, "SM");
#endif
			}
			else
				LOG_ERROR("Unable to deserialize " + strKey, "SM");
		};

		boost::filesystem::path							m_pathDataDirectory;
		boost::filesystem::path							m_pathRawDirectory;

		// functions and variables for raw file output
		std::unordered_map<unsigned short, smSC>		m_umapSC;
		uintmax_t										m_uimRawFileSizeMax;
		bool											openRawFile(unsigned short usChainIdentifier);

	public:
														StorageManager(MetaChain *mc);
														~StorageManager();
		bool											initialize(CSimpleIniA* iniFile);
		bool											writeRaw(unsigned short usChainIdentifier, unsigned int uiBlockNumber, unsigned int uiLength, void *raw);
		bool											registerSC(unsigned short usChainIdentifier);

		// simple getter and setter
		uint16_t										getChainIdentifier(std::string strChainIdentifier);
		std::string										getChainIdentifier(uint16_t uint16ChainIdentifier);
		MCP02::SubChainManager*							getSubChainManager() { return m_pSubChainManager; };
		dbEngine*										createDBEngine(unsigned short usChainIdentifier);

		// critical section for the subchain manager
		cCriticalSection								csSubChainManager;

		// meta db getter template
		template<typename T>
		T getMetaValue(std::string strKey, T tDefault)
		{
			rocksdb::Status dbStatus;
			std::string strTmp;
			dbStatus = m_pMetaDB->Get(rocksdb::ReadOptions(), strKey, &strTmp);

			if (dbStatus.ok())
				return boost::lexical_cast<T>(strTmp);
			else
				return tDefault;
		}

		// meta db inc template for arithmetic types only!
		template<typename T,
				 typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type >
		bool incMetaValue(std::string strKey, T tDefault, T* tpOldValue = NULL, T tInc = 1)
		{
			T val = getMetaValue(strKey, tDefault);

			if (tpOldValue)
				*tpOldValue = val;

			val += tInc;

			return setMetaValue(strKey, &val);
		}

		// meta db setter template
		template<typename T>
		bool setMetaValue(std::string strKey, T* value)
		{
			rocksdb::Status dbStatus;
			dbStatus = m_pMetaDB->Put(rocksdb::WriteOptions(), strKey, boost::lexical_cast<std::string>(*value));

			if (dbStatus.ok())
				return true;
			else
				return false;
		}
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
