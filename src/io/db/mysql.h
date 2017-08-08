#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __DB_ENGINE_MYSQL_H__
#define __DB_ENGINE_MYSQL_H__ 1

#include "dbEngine.h"
#include "../../external/SimpleIni.h"

/*
storage class for mysql
*/

class dbEngineMYSQL : public dbEngine
{
public:
													dbEngineMYSQL();
													~dbEngineMYSQL();
	virtual bool									initialize(CSimpleIniA* iniFile, bool *bNew);
};

#endif