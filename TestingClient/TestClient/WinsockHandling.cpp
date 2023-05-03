#include "WinsockHandling.h"



#define SERVERIP "127.0.0.1"
#define SERVERPORT 5555

WinsockHandling::WinsockHandling()
{
	StartWinSock();
}

WinsockHandling::~WinsockHandling()
{
}

void WinsockHandling::update()
{
	HandleIncomingEvents(ListenSocket, ListenEvent);
}

int WinsockHandling::getClientCount()
{
	return clientCount;
}

void WinsockHandling::requestAvailableBikeMsg()
{
	WinsockHandling::ClientMessage clientMessage;
	clientMessage.type = ClientMessageType::reqAvailableBikes;
	//Send the message to the server.
	if (send(ListenSocket, (char*)&clientMessage, sizeof(WinsockHandling::ClientMessage), 0) != sizeof(WinsockHandling::ClientMessage))
	{
		printf("Available Bike request failed to send");
	}
	else {
		waitingForResponse = true;
	}
}

void WinsockHandling::requestNetworkTestMsg()
{
	WinsockHandling::ClientMessage clientMessage;
	clientMessage.type = ClientMessageType::reqTestMessage;
	//Send the message to the server.
	if (send(ListenSocket, (char*)&clientMessage, sizeof(WinsockHandling::ClientMessage), 0) != sizeof(WinsockHandling::ClientMessage))
	{
		printf("Network Test request failed to send");
	}
}

bool WinsockHandling::requestConnectionToBike(int index)
{
	if (index < 0 || index >= allAvailableBikes.size()) {
		printf("Error requesting bike connection: index '%i' out of range", index);
		return false;
	}
	if (bikeToConnect.ID == "NULL" && bikeToConnect.Name == "NULL") {
		WinsockHandling::ClientMessage clientMessage;
		clientMessage.type = ClientMessageType::reqConnect;
		clientMessage.data = index;
		//Send the message to the server.
		if (send(ListenSocket, (char*)&clientMessage, sizeof(WinsockHandling::ClientMessage), 0) != sizeof(WinsockHandling::ClientMessage))
		{
			printf("Network Test request failed to send");
		}
		else {
			bikeToConnect = allAvailableBikes.at(index);
		}
		waitingForResponse = true;
		return true;
	}
	else {
		printf("Error: Still waiting for last connection attempt");
	}
	return false;
}

void WinsockHandling::StartWinSock()
{
	// Verify winsock version is 2.2.
	WSADATA w;
	int error = WSAStartup(0x0202, &w);
	if (error != 0)
	{
		PrintError("WSAStartup failed"); //TODO gracefully fail instead of ignore
		return;
	}
	if (w.wVersion != 0x0202)
	{
		PrintError("Wrong WinSock version");//TODO gracefully fail instead of ignore
		return;
	}
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Winsock initialised")));
	printf("Winsock initialised\n");
}

void WinsockHandling::StartClient()
{
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Attempting Connection")));
	printf("Attempting Connection\n");

	sockaddr_in InetAddr;
	InetAddr.sin_family = AF_INET;
	
	inet_pton(AF_INET, SERVERIP, &(InetAddr.sin_addr));
	InetAddr.sin_port = htons(SERVERPORT);

	// Create a TCP socket that we'll connect to the server
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSocket == INVALID_SOCKET)
	{
		PrintError("socket failed to initialise");
		return;
	}

	// Connect the socket to the server.
	if (connect(ListenSocket, (const sockaddr*)&InetAddr, sizeof InetAddr) == SOCKET_ERROR)
	{
		PrintError("connect failed");
		return;
	}

	clientCount = 1;

	// Create a new event for checking listen socket activity
	ListenEvent = WSACreateEvent();
	if (ListenEvent == WSA_INVALID_EVENT) {
		PrintError("server event creation failed");

		if (closesocket(ListenSocket) != SOCKET_ERROR) {
			//if (GEngine)
			//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Successfully closed socket")));
			printf("Successfully closed socket\n");
		}
		return;
	}

	eventCount++;

	//Event to detect: Closes and Reads
	WSAEventSelect(ListenSocket, ListenEvent, FD_CLOSE | FD_READ);

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Connected to server")));
	printf("Connected to server\n");

	connectedBike.setAsConnected();
}

void WinsockHandling::CleanupSocket()
{
	connectedBike.setAsDisconnected();
	if (closesocket(ListenSocket) != SOCKET_ERROR) {
		//if (GEngine)
		//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Successfully closed socket")));
		printf("Successfully closed socket\n");
	}
	if (WSACloseEvent(ListenEvent) == false) {
		PrintError("WSACloseEvent() failed");
	}
	clientCount = 0;
}

bool WinsockHandling::HandleIncomingEvents(SOCKET& socket, HANDLE& eventHandler)
{
	DWORD returnVal;

	if (clientCount > 0) { //If connected to the bike interface server

		returnVal = WSAWaitForMultipleEvents(1, &eventHandler, false, 0, false); //Check if any network events allow winsock functions to run without blocking
		if ((returnVal != WSA_WAIT_TIMEOUT) && (returnVal != WSA_WAIT_FAILED)) {

			if (WSAEnumNetworkEvents(socket, eventHandler, &NetworkEvents) == SOCKET_ERROR) {	//Get the network events as binary flags
				PrintError("Retrieving event information failed");
			}
			if (NetworkEvents.lNetworkEvents & FD_CLOSE)	//Server has closed the connection
			{
				//if (GEngine)
				//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Server closed the connection")));
				printf("Server closed the connection\n");

				
				//We ignore the error if the client just force quit
				if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0 && NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 10053)
				{
					//if (GEngine)
					//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("FD_CLOSE failed with error %d"), NetworkEvents.iErrorCode[FD_CLOSE_BIT]));
					printf("FD_CLOSE failed with error %d\n", NetworkEvents.iErrorCode[FD_CLOSE_BIT]);

					return false;
				}

				CleanupSocket();
				return false;
			}
			else if (NetworkEvents.lNetworkEvents & FD_READ)	//Server has sent data to read
			{
				if (NetworkEvents.iErrorCode[FD_READ_BIT] != 0) {
					//if (GEngine)
					//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("FD_ACCEPT failed with error %d"), NetworkEvents.iErrorCode[FD_READ_BIT]));
					printf("FD_ACCEPT failed with error %d\n", NetworkEvents.iErrorCode[FD_READ_BIT]);

					return false;
				}

				decodeNetworkMessage(socket);

			}
		}
		else if (returnVal == WSA_WAIT_TIMEOUT) {
			//All good, we just have no activity
		}
		else if (returnVal == WSA_WAIT_FAILED) {
			PrintError("WSAWaitForMultipleEvents failed!");
		}
	}
	return true;
}

void WinsockHandling::PrintError(const char* eMessage)
{
//	if (GEngine)
//		//FString::Printf incorrectly handles c_string parameter, FString::Format solves this error
//		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Format(TEXT("Error: {0} (WSAGetLastError() = {1})"), { eMessage, WSAGetLastError() }));
	printf("Error: %s (WSAGetLastError() = %i)\n", eMessage, WSAGetLastError());
}

void WinsockHandling::decodeNetworkMessage(SOCKET& socket)
{
	if (expectedMessageType == None) {
		MessageHeader messageHeader;

		// Read a response back from the server.
		int count = recv(socket, (char*)&messageHeader, sizeof(MessageHeader), 0);
		if (count <= 0)
		{
			PrintError("recv returns");
		}
		printf("Header:\n\t%c\n\t%i\n", messageHeader.type, messageHeader.messageLengthData);

		expectedMessageType = messageHeader.type;
		expectedMessageSize = messageHeader.messageLengthData;
	}
	else {
		switch (expectedMessageType)
		{
		case WinsockHandling::None:
			break;

		case WinsockHandling::Error:
			handleErrorMessage(socket);
			break;

		case WinsockHandling::Power:
			handlePowerMessage(socket);
			break;

		case WinsockHandling::AvailableBikes:
			handleAvailableBikeMessage(socket, expectedMessageSize);
			break;

		default:
			break;
		}
		expectedMessageType = None;
	}
	
}

void WinsockHandling::handleErrorMessage(SOCKET& socket)
{
	ErrorMessage errorMessageBuffer;
	if (recv(socket, (char*)&errorMessageBuffer, sizeof(ErrorMessage), 0) != sizeof(ErrorMessage))
	{
		//if (GEngine)
		//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Error message body failed to recieve")));
		//printf("Error message body failed to recieve\n");
		PrintError("Error message body failed to recieve");
	}

	printf("Error Code: %i\t", errorMessageBuffer.errorCode);
	switch (errorMessageBuffer.errorCode)
	{
	case WinsockHandling::Unknown:
		printf("Uknown error");
		break;
	case WinsockHandling::FailedToCreateFromUUID:
		printf("FailedToCreateFromUUID");
		break;
	case WinsockHandling::FailedToConnectToDevice:
		printf("FailedToConnectToDevice");
		bikeToConnect.ID = "NULL";
		bikeToConnect.Name = "NULL";
		break;
	case WinsockHandling::NoError:
		printf("BikeConnectionSuccesfull");
		connectedBikeDescription = bikeToConnect;
		bikeToConnect.ID = "NULL";
		bikeToConnect.Name = "NULL";
		waitingForResponse = false;
	default:
		break;
	}
	printf("\n\n");
}

void WinsockHandling::handlePowerMessage(SOCKET& socket)
{
	PowerMessage powerMessageBuffer;
	if (recv(socket, (char*)&powerMessageBuffer, sizeof(PowerMessage), 0) != sizeof(PowerMessage))
	{
		//if (GEngine)
		//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Power message body failed to recieve")));
		PrintError("Power message body failed to recieve");
	}
	std::cout << "Power:" << powerMessageBuffer.power << std::endl;
	connectedBike.AddInstantPowerReading(powerMessageBuffer.power);
}

void WinsockHandling::handleAvailableBikeMessage(SOCKET& socket, int msgSize)
{
	std::vector<char> buffer(msgSize);
	std::string availableBikeMessageBuffer;

	availableBikeMessageBuffer.reserve(msgSize);
	
	int receivedBytes = recv(socket, &buffer[0], msgSize, 0);
	if (receivedBytes != msgSize)
	{
		//if (GEngine)
		//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Available Bike message body failed to recieve")));
		PrintError("Available Bike message body failed to recieve");
		printf("MsgSize: %i\nRecieved: %i\n", msgSize, receivedBytes);
	}
	else {
		availableBikeMessageBuffer.append(buffer.cbegin(), buffer.cend());

		printf("Available Bike message contents: %s\n", availableBikeMessageBuffer.c_str());

		
		//Process Bike string
		allAvailableBikes.clear();
		char delim = '¬';
		size_t start = 0;
		size_t end = 0;
		std::string temp;
		BikeInfo bikeTemp;
		while ((start = availableBikeMessageBuffer.find_first_not_of(delim, end)) != std::string::npos) {
			end = availableBikeMessageBuffer.find(delim, start);

			temp = availableBikeMessageBuffer.substr(start, end - start);
			
			bikeTemp.ID = temp.substr(0, 17);
			bikeTemp.Name = temp.substr(17,std::string::npos);	//npos causes the rest of the string to be read

			allAvailableBikes.push_back(bikeTemp);
		}

		printf("Available Bikes Formatted:\n");
		for (BikeInfo bike : allAvailableBikes) {
			printf("\tID:%s\t\tName:%s\n", bike.ID.c_str(), bike.Name.c_str());
		}

		availableBikeMessageBuffer.clear();

		availableBikeMessageBuffer.shrink_to_fit();
	}
	waitingForResponse = false;
}

