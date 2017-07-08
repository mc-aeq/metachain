#pragma once

/*
this is the class to handle IP IOs
it's developed as template for better reusability. the template classes used need to have certain functions implemented for correct reading and writing of the contents
*/

template <class T>
class ipContainer
{
	private:
		string									m_strFileName;

		// this vector holds the read information
		vector< T >								m_vecIP;

	public:
												ipContainer();
												ipContainer(string strFileName);
												~ipContainer();

		void									setFileName(string strFileName) { m_strFileName = strFileName; };
		void									readContents();		
		void									writeContents();
};

/*
Template class implementation
*/
#include <boost/algorithm/string.hpp>

template <class T>
ipContainer<T>::ipContainer()
{
}

template <class T>
ipContainer<T>::ipContainer(string strFileName) :
	m_strFileName(strFileName)
{
}

template <class T>
ipContainer<T>::~ipContainer()
{

}

template <class T>
void ipContainer<T>::readContents()
{
	ifstream streamFile(m_strFileName);
#ifdef _DEBUG
	LOG_DEBUG("reading contents of file: " + m_strFileName, "IPC");
#endif

	for (string strLine; getline(streamFile, strLine); )
	{
		// renmove unwanted spaces from beginning and end
		boost::trim(strLine);

		// don't process empty lines
		if (strLine.length() == 0 || strLine.empty())
			continue;

		// if the first character is a # it's a comment and we skip this line
		if (strLine[0] == '#')
			continue;

#ifdef _DEBUG
		LOG_DEBUG("line content: " + strLine, "IPC");
#endif
		T tmp;
		if( tmp.init(strLine) )
			m_vecIP.push_back(tmp);
		else
		{
			LOG_ERROR("unable to add the following IP into our system: " + strLine, "IPC");
			continue;
		}
	}

#ifdef _DEBUG
	LOG_DEBUG("done reading contents of file: " + m_strFileName, "IPC");
	LOG_DEBUG("elements in the vector: " + to_string(m_vecIP.size()), "IPC");
#endif
}

template <class T>
void ipContainer<T>::writeContents()
{
#ifdef _DEBUG
	LOG_DEBUG("writing contents to file: " + m_strFileName, "IPC");
#endif

	ofstream streamOut;
	streamOut.open(m_strFileName, ios_base::out | ios_base::trunc);

	// write standard header
	streamOut << "# generation of the file: " << endl;
	streamOut << "# it will be automatically updated through the TCT blockchain" << endl;
	streamOut << "# any manual changes will be overridden!" << endl << endl;

	for (vector<T>::iterator it = m_vecIP.begin(); it != m_vecIP.end(); it++)
		streamOut << (*it).ToStringIP() << endl;

	streamOut.close();

#ifdef _DEBUG
	LOG_DEBUG("done writing contents to file: " + m_strFileName, "IPC");
#endif
}