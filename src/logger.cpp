#include "../stdafx.h"

Logger::Logger() :
	m_bLogToStdout(true),
	m_bLogToFile(false)
{
	this->log("testing logging facility");
}

Logger& Logger::getInstance()
{
	static Logger instance;
	return instance;
}

void Logger::log( string strLogLine, facility logFacility, string strModule)
{
	/*
		Time processing
	*/
		// get the current time
		this->m_timeCur = time(0);

		// clear the buffer
		memset(this->m_caTimeBuf, '\0', sizeof(this->m_caTimeBuf));

		// get the structure
		localtime_s(&this->m_timeStruct, &this->m_timeCur);

		// format the time
		strftime(this->m_caTimeBuf, sizeof(this->m_caTimeBuf), "%Y-%m-%d %X:", &this->m_timeStruct);

	/*
		Facility processing
	*/
		m_strFacility = " [";
	switch (logFacility)
	{
		case facility::debug:		m_strFacility += "DEBUG";		break;
		case facility::info:		m_strFacility += "INFO";		break;
		case facility::warning:		m_strFacility += "WARNING";	break;
		case facility::error:		m_strFacility += "ERROR";		break;
		default:					m_strFacility += "UNKNOWN";
	}
	m_strFacility += "] ";

	// make the strModule more fancy if set
	if (strModule != "")
		strModule = " [" + strModule + "] ";
	
	// output to stdout if configured
	if(this->m_bLogToStdout)
		cout << this->m_caTimeBuf << m_strFacility << strModule << strLogLine << endl;

	// output to file if configured
	if(this->m_bLogToFile)
		m_streamLogFile << this->m_caTimeBuf <<  m_strFacility << strModule << strLogLine << endl;
}

void Logger::initialize(CSimpleIniA* iniFile)
{
	// check if logging to the cmd line is enabled / disabled
	this->m_bLogToStdout = iniFile->GetBoolValue("logging", "log_to_stdout", true);
	#ifdef _DEBUG
		LOG_DEBUG("Log to stdout: " + (string)(this->m_bLogToStdout ? "true" : "false"), "LOGGER");
	#endif

	// check if logging to file is enabled / disabled - if enabled, open the file and prepare the file pointer
	this->m_bLogToFile = iniFile->GetBoolValue("logging", "log_to_file", true);
	#ifdef _DEBUG
		LOG_DEBUG("Log to file: " + (string)(this->m_bLogToFile ? "true" : "false"), "LOGGER");
	#endif
	if (this->m_bLogToFile)
	{
		m_strFileName = iniFile->GetValue("logging", "log_file", "output.log");
		m_streamLogFile.open(m_strFileName, ios_base::out | ios_base::app | ios_base::ate);
		#ifdef _DEBUG
				LOG_DEBUG("Logfile name: " + (string)(m_strFileName), "LOGGER");
		#endif
	}
}