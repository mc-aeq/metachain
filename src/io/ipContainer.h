#pragma once

/*
this is the class to handle IP IOs
it can also be overridden to endvance it with more functionality
*/

class ipContainer
{
	friend class peerContainer;

	private:
		string									m_strFileName;

		// this vector holds the read information
		vector< CNetAddr >						m_vecIP;

	public:
												ipContainer();
												ipContainer(string strFileName);
												~ipContainer();

		void									setFileName(string strFileName) { m_strFileName = strFileName; };
		void									readContents();		
		void									writeContents();
};