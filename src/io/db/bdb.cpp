/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "bdb.h"
#include "../../logger.h"

dbEngineBDB::dbEngineBDB()
{

}

dbEngineBDB::~dbEngineBDB()
{

}

bool dbEngineBDB::initialize(CSimpleIniA* iniFile, bool *bNew)
{
	LOG("Initializing BDB Storage Engine", "SE-BDB");
	return true;
}