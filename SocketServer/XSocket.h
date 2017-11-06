#pragma once
#define BUFFER_SIZE 512


class XSocket
{
public:
	XSocket();
	~XSocket();
	bool GetConnected();
private:
	bool m_Connected;
	

};

