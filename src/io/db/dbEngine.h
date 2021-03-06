#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __DB_ENGINE_H__
#define __DB_ENGINE_H__ 1

#include <unordered_map>
#include "../../cCriticalSection.h"
#include "../../SimpleIni.h"

/*
this class is our abstract db engine class and must be overridden in order to add a new db engine
*/

class dbEngine
{
	public:

		// this function is for initializing
		virtual bool									initialize(std::unordered_map<std::string, std::string>* umapSettings) = 0;

		// functions for writing, getting single entries
		virtual bool									write( std::string strKey, std::string strValue, std::string strEnv = "") = 0;		// strEnv is used for dbEngines that are not key/value based. e.g. mysql where tables can be defined and used
		virtual std::string								get(std::string strKey, std::string strDefault = "", std::string strEnv = "") = 0;
		virtual unsigned int							get(std::string strKey, unsigned int uiDefault = 0, std::string strEnv = "") = 0;
		virtual bool									get(std::string strKey, bool bDefault = false, std::string strEnv = "") = 0;
		virtual bool									deleteEntry(std::string strKey, std::string strEnv = "") = 0;

		// functions for batch writing
		virtual void									batchStart() = 0;
		virtual void									batchAddStatement( std::string strKey, std::string strValue, std::string strEnv = "" ) = 0;		// strEnv is used for dbEngines that are not key/value based. e.g. mysql where tables can be defined and used
		virtual void									batchDeleteEntry(std::string strKey, std::string strEnv = "") = 0;
		virtual void									batchFinalize() = 0;
		cCriticalSection								batchCriticalSection;
};

#endif