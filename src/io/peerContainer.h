#pragma once

/*
this class holds all information about known peers and their details
it'll be updated during runtime and saved frequently
*/

class peerContainer : public ipContainer
{
	public:
							peerContainer(string strFileName);
};