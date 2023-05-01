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

#pragma region NetworkMessageStructs
	enum MessageType : char{
		None = 'N',
		Error = 'E',
		Power = 'P',
		AvailableBikes = 'A'
	};

	enum Errors : uint32_t {
		Unknown = 0,
		FailedToCreateFromUUID = 1
	};

	struct MessageHeader {
		MessageType type = MessageType::None;
		uint32_t messageLengthData = 0;
	};

	struct ErrorMessage {
		uint32_t errorCode = 0;
	};

	struct PowerMessage {
		uint32_t power = 0;
	};


#pragma endregion

	void sendErrorMessage(Errors errorType);

public:

	struct BikeDeviceInfo {
		char ID[12] = { 0 };
		std::string name;
	};

	void sendPowerMessage(uint32_t power);

	void sendAvailableBikesMessage(int bikeCount, std::vector<std::string>* IDs, std::vector<std::string>* names);

	void sendNetworkTestingMessages();
};

