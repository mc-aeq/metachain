#include "stdafx.h"
/*
	start point for the metachain
*/
#ifdef _WINDOWS
	
#endif

#ifdef _LINUX
	
#endif

using namespace std;

// main entry point
int main( int argc, char* argv[] )
{
	// initialize the logging instance
	LOG("-------------------------------", "");

	// load our ini file and create our iniFile object
	LOG("loading file", "INI");
	CSimpleIniA *iniFile = new CSimpleIniA(true, false, false);
	SI_Error eErr = iniFile->LoadFile("metachain.ini");

	if( eErr == SI_OK )	// everything is OK with the ini file, we go on with our initialization
	{
		LOG("file successfully loaded", "INI");
		
		LOG("initializing", "LOGGER");
		Logger::getInstance().initialize(iniFile);
		LOG("successfully initialized", "LOGGER");

		LOG("initializing MetaChain", "MC");

		cin.get();
		// end metachain 
		return 0;
	}
	// error handling of ini files. after that there is nothing really more happening accept a keystroke until we quit the app
	else if(eErr == SI_FAIL)
		LOG_ERROR("Generic failure", "INI");
	else if(eErr == SI_NOMEM)
		LOG_ERROR("Memory error", "INI");
	else if(eErr == SI_FILE)
		LOG_ERROR("File error", "INI");

	cin.get();
	return 1;
}

