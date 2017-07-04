#include "../stdafx.h"

Logger::Logger()
{
	this->log("initializing logging facility");
}

Logger& Logger::getInstance()
{
	static Logger instance;
	return instance;
}

void Logger::log( string strLogLine, facility logFacility )
{
	/*
		Time processing
	*/
		// get the current time
		this->curTime = time(0);

		// clear the buffer
		memset(this->cTimeBuf, '\0', sizeof(this->cTimeBuf));

		// get the structure
		localtime_s(&this->timeStruct, &this->curTime);

		// format the time
		strftime(this->cTimeBuf, sizeof(this->cTimeBuf), "%Y-%m-%d %X:", &this->timeStruct);

	/*
		Facility processing
	*/
	strFacility = "[";
	switch (logFacility)
	{
		case facility::debug:		strFacility += "DEBUG";		break;
		case facility::info:		strFacility += "INFO";		break;
		case facility::warning:		strFacility += "WARNING";	break;
		case facility::error:		strFacility += "ERROR";		break;
		default:					strFacility += "UNKNOWN";
	}
	strFacility += "]";
	
	// output to cmdline
	cout << this->cTimeBuf << " " << strFacility << " " << strLogLine << endl;
}