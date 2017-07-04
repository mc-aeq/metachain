#pragma once

using namespace std;

/*
	This is the logging class, it'll use file logging if wanted and also print out to the cmdline.
	This is a singleton!
*/
class Logger
{
	private:
		// variables for faster time processing
		time_t				curTime;
		tm					timeStruct;
		char				cTimeBuf[80];

		// variables for faster facility processing
		string				strFacility;

		// functions
		Logger();
		Logger(Logger const& copy);					// not implemented
		Logger&		operator=(Logger const& copy);	// not implemented

	public:
		enum facility {
			debug,
			info,
			warning,
			error
		};

		static		Logger& getInstance();
		void		log(string strLogLine, facility logFacility = facility::info);
};