#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 5555

class WinsockHelper
{
public:
	WinsockHelper();

	bool PollForConnection();

	SOCKET ListenSocket, AcceptSocket;

	struct MessageContents {
		int powerValue = -1;
	};

	int getClientCount();

private:
	void die(const char* message);
	void InitWinSock();
	void StartServer();

	WSAEVENT ListenEvent, AcceptEvent;
	int eventCount = 0;
	int clientCount = 0;

	//Structure to hold the result from WSAEnumNetworkEvents
	WSANETWORKEVENTS NetworkEvents;

	int eventIndex = 0;
};

