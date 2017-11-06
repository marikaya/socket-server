#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include "Util.h"

#pragma comment(lib, "ws2_32.lib")
#define BUFFER_SIZE 1024



class XListener
{
private:
	int m_Port;
	SOCKET ListenSocket;
	bool CreateSocketInformation(SOCKET s);
	void FreeSocketInformation(DWORD Index);

public:
	XListener(int nPort);
	~XListener();
	bool initialize();
	void start();


};
