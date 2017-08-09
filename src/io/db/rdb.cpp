/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "rdb.h"
#include "../../logger.h"

dbEngineRDB::dbEngineRDB()
{

}

dbEngineRDB::~dbEngineRDB()
{

}

bool dbEngineRDB::initialize(CSimpleIniA* iniFile, bool *bNew)
{
	LOG("Initializing RDB Storage Engine", "SE-RDB");
	return true;
}