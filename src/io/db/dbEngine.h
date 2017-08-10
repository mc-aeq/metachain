#pragma once

/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#ifndef __DB_ENGINE_H__
#define __DB_ENGINE_H__ 1

#include "../../SimpleIni.h"

/*
this class is our abstract db engine class and must be overridden in order to add a new db engine
*/

class dbEngine
{
public:
	virtual bool									initialize(CSimpleIniA* iniFile, bool *bNew) = 0;
};

#endif