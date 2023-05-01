#pragma once
#include "pch.h"

#include "IDLTesting.LiteWatcher.h"

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
	~WinsockHelper();

	bool PollForConnection();

	bool HandleIncomingEvents();

	SOCKET ListenSocket, AcceptSocket;

	struct MessageContents {
		int powerValue = -1;
	};


	int getClientCount();

	winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable>* devList;

	std::string bikeIDToConnect = "NULL";

private:
	void die(const char* message);
	void InitWinSock();
	void StartServer();
	void CleanupSocket();
	void PrintError(const char* eMessage);

	WSAEVENT ListenEvent, AcceptEvent;
	int eventCount = 0;
	int clientCount = 0;

	//Structure to hold the result from WSAEnumNetworkEvents
	WSANETWORKEVENTS NetworkEvents;

	int eventIndex = 0;

	void decodeNetworkMessage(SOCKET& socket);

	void updateBikeList();

#pragma region NetworkMessageStructs
	//Server Messages
	enum MessageType : char{
		None = 'N',
		Error = 'E',
		Power = 'P',
		AvailableBikes = 'A'
	};


	struct MessageHeader {
		MessageType type = MessageType::None;
		uint32_t messageLengthData = 0;
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

public:
	enum Errors : uint32_t {
		Unknown = 0,
		FailedToCreateFromUUID = 1,
		FailedToConnectToDevice = 2
	};

	struct ErrorMessage {
		uint32_t errorCode = 0;
	};
#pragma endregion


public:
	void sendErrorMessage(Errors errorType);

	struct BikeDeviceInfo {
		char ID[12] = { 0 };
		std::string name;
	};

	void sendPowerMessage(uint32_t power);


	std::vector< std::string> IDs;
	std::vector< std::string> names;
	void sendAvailableBikesMessage(int bikeCount, std::vector<std::string>* IDs, std::vector<std::string>* names);

	void sendNetworkTestingMessages();
};

