#include "XListener.h"

typedef struct _SOCKET_INFORMATION {
	CHAR Buffer[BUFFER_SIZE];
	WSABUF DataBuf;
	SOCKET Socket;
	OVERLAPPED Overlapped;
	DWORD BytesSEND;
	DWORD BytesRECV;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;


DWORD TotalSockets = 0;
LPSOCKET_INFORMATION SocketArray[FD_SETSIZE];

bool XListener::CreateSocketInformation(SOCKET s)
{
	LPSOCKET_INFORMATION SI;

	Console_Print("Accepted socket number  " + s);

	if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
	{
		Console_Print("GlobalAlloc() failed with error  " + GetLastError());
		return FALSE;
	}
	else
		Console_Print("GlobalAlloc() for SOCKET_INFORMATION is OK!");

	// Prepare SocketInfo structure for use
	SI->Socket = s;
	SI->BytesSEND = 0;
	SI->BytesRECV = 0;

	SocketArray[TotalSockets] = SI;
	TotalSockets++;
	return(TRUE);
}

void XListener::FreeSocketInformation(DWORD Index)
{
	LPSOCKET_INFORMATION SI = SocketArray[Index];
	DWORD i;

	closesocket(SI->Socket);
	Console_Print("Closing socket number  " + SI->Socket);
	GlobalFree(SI);

	// Squash the socket array
	for (i = Index; i < TotalSockets; i++)
	{
		SocketArray[i] = SocketArray[i + 1];
	}

	TotalSockets--;
}
XListener::XListener(int nPort) : m_Port(nPort)
{

	Console_Print("Listener is initialized");
}

XListener::~XListener()
{
}

bool XListener::initialize()
{
	SOCKADDR_IN InternetAddr;
	WSADATA wsaData;
	INT Ret;

	if ((Ret = WSAStartup(0x0202, &wsaData)) != 0)
	{
		Console_Print("WSAStartup() failed with error " + Ret);
		WSACleanup();
		return false;
	}
	else
		Console_Print("WSAStartup() is fine!");

	// Prepare a socket to listen for connections
	if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		Console_Print("WSASocket() failed with error " + WSAGetLastError());
		return false;
	}
	else
		Console_Print("WSASocket() is OK!");

	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	InternetAddr.sin_port = htons(this->m_Port);

	if (bind(ListenSocket, (PSOCKADDR)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
	{
		Console_Print("bind() failed with error " + WSAGetLastError());
		return false;
	}
	else
		Console_Print("bind() is OK!");
	return true;
}

void XListener::start()
{
	SOCKET AcceptSocket;
	FD_SET WriteSet;
	FD_SET ReadSet;
	DWORD i;
	DWORD Total;
	ULONG NonBlock;
	DWORD Flags;
	DWORD SendBytes;
	DWORD RecvBytes;

	if (listen(this->ListenSocket, 5))
	{
		Console_Print("listen() failed with error " + WSAGetLastError());
		return;
	}
	else
		Console_Print("listen() is OK!");

	// Change the socket mode on the listening socket from blocking to
	// non-block so the application will not block waiting for requests
	NonBlock = 1;
	if (ioctlsocket(ListenSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
	{
		Console_Print("ioctlsocket() failed with error  "+ WSAGetLastError());
		return;
	}
	else
		Console_Print("ioctlsocket() is OK!");

	while (TRUE)
	{
		// Prepare the Read and Write socket sets for network I/O notification
		FD_ZERO(&ReadSet);
		FD_ZERO(&WriteSet);

		// Always look for connection attempts
		FD_SET(ListenSocket, &ReadSet);

		// Set Read and Write notification for each socket based on the
		// current state the buffer.  If there is data remaining in the
		// buffer then set the Write set otherwise the Read set
		for (i = 0; i < TotalSockets; i++)
		if (SocketArray[i]->BytesRECV > SocketArray[i]->BytesSEND)
			FD_SET(SocketArray[i]->Socket, &WriteSet);
		else
			FD_SET(SocketArray[i]->Socket, &ReadSet);

		if ((Total = select(0, &ReadSet, &WriteSet, NULL, NULL)) == SOCKET_ERROR)
		{
			Console_Print("select() returned with error  "+ WSAGetLastError());
			return ;
		}
		else
			Console_Print("select() is OK!");

		// Check for arriving connections on the listening socket.
		if (FD_ISSET(ListenSocket, &ReadSet))
		{
			Total--;
			if ((AcceptSocket = accept(ListenSocket, NULL, NULL)) != INVALID_SOCKET)
			{
				// Set the accepted socket to non-blocking mode so the server will
				// not get caught in a blocked condition on WSASends
				NonBlock = 1;
				if (ioctlsocket(AcceptSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
				{
					Console_Print("ioctlsocket(FIONBIO) failed with error  "+ WSAGetLastError());
					return ;
				}
				else
					Console_Print("ioctlsocket(FIONBIO) is OK!");

				if (CreateSocketInformation(AcceptSocket) == FALSE)
				{
					Console_Print("CreateSocketInformation(AcceptSocket) failed!");
					return ;
				}
				else
					Console_Print("CreateSocketInformation() is OK!");

			}
			else
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					Console_Print("accept() failed with error  "+ WSAGetLastError());
					return ;
				}
				else
					Console_Print("accept() is fine!");
			}
		}

		// Check each socket for Read and Write notification until the number
		// of sockets in Total is satisfied
		for (i = 0; Total > 0 && i < TotalSockets; i++)
		{
			LPSOCKET_INFORMATION SocketInfo = SocketArray[i];

			// If the ReadSet is marked for this socket then this means data
			// is available to be read on the socket
			if (FD_ISSET(SocketInfo->Socket, &ReadSet))
			{
				Total--;

				SocketInfo->DataBuf.buf = SocketInfo->Buffer;
				SocketInfo->DataBuf.len = BUFFER_SIZE;

				Flags = 0;
				if (WSARecv(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &RecvBytes, &Flags, NULL, NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						Console_Print("WSARecv() failed with error  "+ WSAGetLastError());
						FreeSocketInformation(i);
					}
					else
						Console_Print("WSARecv() is OK!");
					continue;
				}
				else
				{
					SocketInfo->BytesRECV = RecvBytes;

					// If zero bytes are received, this indicates the peer closed the connection.
					if (RecvBytes == 0)
					{
						FreeSocketInformation(i);
						continue;
					}
				}
			}

			// If the WriteSet is marked on this socket then this means the internal
			// data buffers are available for more data
			if (FD_ISSET(SocketInfo->Socket, &WriteSet))
			{
				Total--;

				SocketInfo->DataBuf.buf = SocketInfo->Buffer + SocketInfo->BytesSEND;
				SocketInfo->DataBuf.len = SocketInfo->BytesRECV - SocketInfo->BytesSEND;

				if (WSASend(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &SendBytes, 0, NULL, NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						Console_Print("WSASend() failed with error "+ WSAGetLastError());
						FreeSocketInformation(i);
					}
					else
						Console_Print("WSASend() is OK!");

					continue;
				}
				else
				{
					SocketInfo->BytesSEND += SendBytes;

					if (SocketInfo->BytesSEND == SocketInfo->BytesRECV)
					{
						SocketInfo->BytesSEND = 0;
						SocketInfo->BytesRECV = 0;
					}
				}
			}
		}
	}
}
