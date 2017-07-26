#pragma once

#include "external/SimpleIni.h"

/*
	This is the logging class, it'll use file logging if wanted and also print out to the cmdline.
	This is a singleton!
*/
class Logger
{
	private:
		// variables for faster time processing
		time_t						m_timeCur;
		tm							m_timeStruct;
		char						m_caTimeBuf[80];

		// variables for faster facility processing
		string						m_strFacility;

		// configuration variables
		bool						m_bLogToStdout;
		bool						m_bLogToFile;
		string						m_strFileName;
		ofstream					m_streamLogFile;

		// mutex to secure thread safe output
		mutex						m_mutexOutput;

		// constructor and operator
									Logger();
									Logger(Logger const& copy);					// not implemented
		Logger&						operator=(Logger const& copy);	// not implemented

	public:
		enum facility {
			debug,
			info,
			warning,
			error
		};

		static						Logger& getInstance();
									~Logger();
		void						log(string strLogLine, facility logFacility = facility::info, string strModule = "");
		void						initialize(CSimpleIniA* iniFile);
};

// logging macros for faster development, interchangeability and readability
#define LOG(logline, module) Logger::getInstance().log(logline, Logger::facility::info, module)
#define LOG_WARNING(logline, module) Logger::getInstance().log(logline, Logger::facility::warning, module)
#define LOG_DEBUG(logline, module) Logger::getInstance().log(logline, Logger::facility::debug, module)
#define LOG_ERROR(logline, module) Logger::getInstance().log(logline, Logger::facility::error, module)
