/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include <atomic>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/process.hpp>
#include <boost/process/spawn.hpp>

#include "src/defines.h"
#include "src/ArgsManager.h"
#include "src/functions.h"
#include "src/logger.h"
#include "src/MetaChain.h"

/*
	start point for the metachain
*/

std::atomic<bool> sabShutdown(false);
std::atomic<bool> sabReboot(false);

// argument manager
ArgsManager gArgs;

// our loop for the main thread
void WaitForShutdown(boost::thread_group* threadGroup)
{
	// Tell the main threads to shutdown.
	while (!sabShutdown)
	{
		MilliSleep(200);
	}
	if (threadGroup)
	{
		threadGroup->interrupt_all();
		threadGroup->join_all();
	}
}

// main entry point
int main( int argc, char* argv[] )
{
	// initialize the logging instance
	LOG("-------------------------------", "");

	// borrowed from the bitcoin core we initialize the argmanager and parse the arguments if provided
	ParseParameters(argc, argv);

	// Process help and version
	if (IsArgSet("-?") || IsArgSet("-h") || IsArgSet("--help") || IsArgSet("-v") || IsArgSet("--version"))
	{
		LOG("TCT node - version " + std::to_string(g_cuint32tVersion), "TCT");
		MetaChain::getInstance().LicenseInfo();

		if (IsArgSet("-?") || IsArgSet("-h") || IsArgSet("--help"))
		{
			LOG("Arguments:", "TCT");
			LOG("-v, --version: print version information", "TCT");
			LOG("-?, -h, --help: print this help and exit", "TCT");
			LOG("-c, --conf=<file>: use the following config file", "TCT");
		}
		return true;
	}

	// load our ini file and create our iniFile object
	std::string strIni;
	if (IsArgSet("-c"))
		strIni = GetArg("-c", "node.ini");
	else if (IsArgSet("--conf"))
		strIni = GetArg("--conf", "node.ini");
	else
		strIni = "node.ini";

	LOG("loading file: " + strIni, "INI");
	CSimpleIniA *iniFile = new CSimpleIniA(true, false, false);
	SI_Error eErr = iniFile->LoadFile(strIni.c_str());

	if( eErr == SI_OK )	// everything is OK with the ini file, we go on with our initialization
	{
		LOG("file successfully loaded", "INI");
		
		LOG("initializing", "LOGGER");
		Logger::getInstance().initialize(iniFile);
		LOG("successfully initialized", "LOGGER");

        // daemonize if wanted
        if( iniFile->GetBoolValue("general", "daemonize", false) )
        {
#if HAVE_DAEMON
            LOG("daemonizing process", "MC");

            // Daemonize
            if( daemon(1, 0) )
            {
                    std::string strError = strerror(errno);
                    LOG_ERROR("daemon() failed - " + strError, "MC");
					return 1;
            }
#else
            LOG("daemonizing is not supported on this operating system, continuing attached mode", "MC");
#endif
		}
		else
			LOG("not daemonizing process, running in foreground", "MC");

		LOG("creating MetaChain", "MC");
		MetaChain::getInstance();

		LOG("initializing MetaChain", "MC");
		if (!MetaChain::getInstance().initialize(iniFile, boost::filesystem::system_complete(argv[0])))
		{
			// if sabReboot is set, the autoupdate upon start was successfull. we don't need an error msg and continue to restart the node with the new versions
			if (!sabReboot)
			{
				LOG_ERROR("Something terrible happened, we're terminating for security reasons.", "MC");
				return 1;
			}
		}
		else
		{
			// go into our execution loop and wait for shutdown
			WaitForShutdown(MetaChain::getInstance().getThreadGroup());
		}

		// it can happen that a update was successfully made. if so, everything is prepared and this variable is true
		// we need to start up a new MC process and let this one die
		// there are no race conditions with databases since everything needs to be shut down correctly before the following line is called
		if (sabReboot)
		{
			// start a new process in async mode (we dont need anything from this new process)
			std::string strParams;
			for (int i = 1; i < argc; i++)
				strParams += argv[i];
			boost::process::spawn(argv[0], strParams);
		}

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

	return 1;
}
