/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "mysql.h"
#include "../../logger.h"

dbEngineMYSQL::dbEngineMYSQL()
{

}

dbEngineMYSQL::~dbEngineMYSQL()
{

}

bool dbEngineMYSQL::initialize(CSimpleIniA* iniFile, bool *bNew)
{
	LOG("Initializing MySQL Storage Engine", "SE-MYSQL");
	return true;
}