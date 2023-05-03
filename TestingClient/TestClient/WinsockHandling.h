#pragma once

//Before writing any code, you need to disable common warnings in WinRT headers

#include "ConnectedBike.h"

#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
/**
 *
 */
class WinsockHandling
{
public:
	WinsockHandling();
	~WinsockHandling();

	ConnectedBike connectedBike;

	void StartClient();

	void update();

	int getClientCount();

	void requestAvailableBikeMsg();

	void requestNetworkTestMsg();

	bool requestConnectionToBike(int index);

	struct BikeInfo {
		std::string ID = "NULL";
		std::string Name = "NULL";
	};

	std::vector<BikeInfo> allAvailableBikes;

	bool waitingForResponse = false;
private:

	SOCKET ListenSocket;
	WSAEVENT ListenEvent;
	int eventCount = 0;
	int clientCount = 0;

	//Buffoer for WSAEnumNetworkEvents
	WSANETWORKEVENTS NetworkEvents;

	BikeInfo connectedBikeDescription;
	BikeInfo bikeToConnect;

	//struct MessageContents {
	//	int powerValue;
	//};

	// Initialise the WinSock library.
	void StartWinSock();


	void CleanupSocket();

	bool HandleIncomingEvents(SOCKET& socket, HANDLE& eventHandler);

	void PrintError(const char* eMessage);

	void decodeNetworkMessage(SOCKET& socket);

#pragma region NetworkMessageStructs
	enum MessageType : char {
		None = 'N',
		Error = 'E',
		Power = 'P',
		AvailableBikes = 'A'
	};

	enum Errors : uint32_t {
		Unknown = 0,
		FailedToCreateFromUUID = 1,
		FailedToConnectToDevice = 2,
		NoError = 3
	};

	struct MessageHeader {
		MessageType type = None;
		uint32_t messageLengthData = 0;
	};

	struct ErrorMessage {
		uint32_t errorCode = 0;
	};

	struct PowerMessage {
		uint32_t power = 0;
	};

	//Client Messages
	enum ClientMessageType : char {
		inValid = 'N',
		reqTestMessage = 'T',
		reqAvailableBikes = 'A',
		reqConnect = 'C',
		reqExit = 'E'
	};

	struct ClientMessage {
		ClientMessageType type = ClientMessageType::inValid;
		uint32_t data = -1;
	};

	void handleErrorMessage(SOCKET& socket);

	void handlePowerMessage(SOCKET& socket);

	void handleAvailableBikeMessage(SOCKET& socket, int msgSize);

	MessageType expectedMessageType = None;
	int expectedMessageSize = 0;
#pragma endregion
};