#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __DB_ENGINE_BDB_H__
#define __DB_ENGINE_BDB_H__ 1

#include "dbEngine.h"
#include "../../external/SimpleIni.h"

/*
storage class for bdb
*/

class dbEngineBDB : public dbEngine
{
public:
													dbEngineBDB();
													~dbEngineBDB();
	virtual bool									initialize(CSimpleIniA* iniFile, bool *bNew);
};

#endif