#include "pch.h"
#include "WinsockHelper.h"


WinsockHelper::WinsockHelper() {
    InitWinSock();
    StartServer();
}

WinsockHelper::~WinsockHelper()
{
    if (closesocket(ListenSocket) != SOCKET_ERROR) {
        printf("Successfully closed Listen socket\n");
    }
    if (WSACloseEvent(ListenEvent) == false) {
        PrintError("WSACloseEvent() failed");
    }
}

int WinsockHelper::getClientCount()
{
    return clientCount;
}

void WinsockHelper::die(const char* message) {
    fprintf(stderr, "\n\n\nError: %s (WSAGetLastError() = %d)\n\n\n\n", message, WSAGetLastError());

#ifdef _DEBUG
    // Debug build -- drop the program into the debugger.
    abort();
#else
    exit(1);
#endif
}

void WinsockHelper::InitWinSock() {
    // We want version 2.2.
    WSADATA w;
    int error = WSAStartup(0x0202, &w);
    if (error != 0)
    {
        die("WSAStartup failed");
    }
    if (w.wVersion != 0x0202)
    {
        die("Wrong WinSock version");
    }
    printf("Winsock initialised\n");
}





void WinsockHelper::StartServer() {
    printf("Server starting\n");

    //Build socket address structure for binding the socket
    sockaddr_in InetAddr;
    InetAddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &(InetAddr.sin_addr));
    InetAddr.sin_port = htons(SERVERPORT);

    //Create our TCP server/listen socket
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        die("socket failed");
    }

    // Bind the server socket to its address.
    if (bind(ListenSocket, (SOCKADDR*)&InetAddr, sizeof(InetAddr)) != 0) {
        die("bind failed");
    }

    // Create a new event for checking listen socket activity
    ListenEvent = WSACreateEvent();
    if (ListenEvent == WSA_INVALID_EVENT) {
        die("server event creation failed");
    }
    //Assosciate this event with the socket types we're interested in
    //In this case, on the server, we're interested in Accepts and Closes
    WSAEventSelect(ListenSocket, ListenEvent, FD_ACCEPT | FD_CLOSE);

    //Start listening for connection requests on the socket
    if (listen(ListenSocket, 1) == SOCKET_ERROR) {
        die("listen failed");
    }

    eventCount++;

    printf("Listening on socket...\n");
}

void WinsockHelper::CleanupSocket()
{
    if (closesocket(AcceptSocket) != SOCKET_ERROR) {
        printf("Successfully closed Connected socket\n");
    }
    if (WSACloseEvent(AcceptEvent) == false) {
        PrintError("WSACloseEvent() failed");
    }
    clientCount = 0;
}

void WinsockHelper::PrintError(const char* eMessage)
{
    printf("Error: %s (WSAGetLastError() = %i)\n", eMessage, WSAGetLastError());
}

void WinsockHelper::sendErrorMessage(Errors errorType)
{
    WinsockHelper::MessageHeader messageHeader;
    messageHeader.type = MessageType::Error;
    //Send the message to the server.
    if (send(AcceptSocket, (char*)&messageHeader, sizeof(WinsockHelper::MessageHeader), 0) != sizeof(WinsockHelper::MessageHeader))
    {
        printf("Error message Header failed to send\n");
    }

    WinsockHelper::ErrorMessage messageBuffer;
    messageBuffer.errorCode = errorType;
    //Send the message to the server.
    if (send(AcceptSocket, (char*)&messageBuffer, sizeof(WinsockHelper::ErrorMessage), 0) != sizeof(WinsockHelper::ErrorMessage))
    {
        printf("Error message body failed to send\n");
    }
    printf("Error message sent\n");
}

void WinsockHelper::decodeNetworkMessage(SOCKET& socket)
{
    ClientMessage clientMessage;

    // Read a response back from the server.
    int count = recv(socket, (char*)&clientMessage, sizeof(ClientMessage), 0);
    if (count <= 0)
    {
        PrintError("recv returns");
    }

    std::printf("Message from Client:\n\t%c\n\t%i\n", clientMessage.type,clientMessage.data);

    switch (clientMessage.type)
    {
    case WinsockHelper::inValid:
        break;
    case WinsockHelper::reqTestMessage:
        sendNetworkTestingMessages();
        break;
    case WinsockHelper::reqAvailableBikes:
        updateBikeList();
        sendAvailableBikesMessage(IDs.size(),&IDs,&names);
        break;
    case WinsockHelper::reqConnect:
        bikeIDToConnect = IDs.at(clientMessage.data);
        std::printf("BikeID at index'%i' = %s\n", clientMessage.data, bikeIDToConnect.c_str());
        break;
    case WinsockHelper::reqExit:
        break;
    default:
        break;
    }

}


using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Foundation::Collections;

void WinsockHelper::updateBikeList()
{
    if (devList == NULL) {
        printf("Fail to update bike list: WinsockHandler.devList is NULL\n\n");
        return;
    }
    IVectorView<GattCharacteristic> characteristics;
    IVectorView<GattDeviceService> services;
    IDs.clear();
    names.clear();
    uint32_t size = devList->Size();
    for (uint32_t index = 0; index < size; index++) //-----Go through every device
    {
        if (index >= devList->Size()) {
            break;
        }
        //-----If the device is valid
        auto inspectDevice = devList->GetAt(index);
        if (inspectDevice != NULL) {
            auto bleDeviceDisplay = inspectDevice.as<winrt::IDLTesting::BluetoothLEDeviceDisplay>();
            if (bleDeviceDisplay != NULL) {
                BluetoothLEDevice bluetoothLeDevice = BluetoothLEDevice::FromIdAsync(bleDeviceDisplay.DeviceInformation().Id()).get(); //.get() makes this no longer Async
                if (bluetoothLeDevice != NULL) { //If device successfully created
                    IDs.push_back(to_string(bleDeviceDisplay.DeviceInformation().Id()));
                    names.push_back(to_string(bleDeviceDisplay.DeviceInformation().Name()));
                }
                else {
                    printf("main.cpp: Failed to create device from UUID: %ls\nCheck bluetooth is turned on\n", bleDeviceDisplay.DeviceInformation().Id().c_str());
                }
            }
        }
    }
}

void WinsockHelper::sendPowerMessage(uint32_t power)
{
    WinsockHelper::MessageHeader messageHeader;
    messageHeader.type = MessageType::Power;
    //Send the message to the server.
    if (send(AcceptSocket, (char*)&messageHeader, sizeof(WinsockHelper::MessageHeader), 0) != sizeof(WinsockHelper::MessageHeader))
    {
        printf("Power message Header failed to send");
    }

    WinsockHelper::PowerMessage messageBuffer;
    messageBuffer.power = power;
    //Send the message to the server.
    if (send(AcceptSocket, (char*)&messageBuffer, sizeof(WinsockHelper::PowerMessage), 0) != sizeof(WinsockHelper::PowerMessage))
    {
        printf("Power message body failed to send");
    }
}

void WinsockHelper::sendAvailableBikesMessage(int bikeCount, std::vector<std::string>* IDs, std::vector<std::string>* names)
{

        std::string messageBody;
        for (int i = 0; i < bikeCount; i++) {
            std::string ID = IDs->at(i).substr(41, 17);

            std::string bikeData = ID + names->at(i) + '¬';
            messageBody.append(bikeData);
        }

        WinsockHelper::MessageHeader messageHeader;
        messageHeader.type = MessageType::AvailableBikes;
        messageHeader.messageLengthData = messageBody.size();
        //Send the message header to the Game.
        if (send(AcceptSocket, (char*)&messageHeader, sizeof(WinsockHelper::MessageHeader), 0) != sizeof(WinsockHelper::MessageHeader))
        {
            printf("Available Bike message Header failed to send\n");
        }
        printf("Sent Bike string Header, %c:%i\t", messageHeader.type, messageHeader.messageLengthData);

        if (bikeCount > 0) {
        //Send the message body to the Game.
        if (send(AcceptSocket, (char*)messageBody.c_str(), messageBody.size(), 0) != messageBody.size())
        {
            printf("Available Bike message body failed to send\n");
        }
        printf("Sent Bike string: %s\n", messageBody.c_str());

        }
}

void WinsockHelper::sendNetworkTestingMessages()
{
    sendErrorMessage(Errors::Unknown);
    sendPowerMessage(1234);

    //std::vector<std::string>IDs;
    //std::vector<std::string>Names;
    IDs.clear();
    names.clear();

    for (int i = 0; i < 4; i++) {
        IDs.push_back("BluetoothLE#BluetoothLE70:66:55:73:ad:8a-fe:4a:3f:d8:8a:0" + std::to_string(i));
        names.push_back("BikeMessageNo:" + std::to_string(i));
    }

    sendAvailableBikesMessage(IDs.size(), &IDs, &names);
}

bool WinsockHelper::PollForConnection() {
    DWORD returnVal;

    if (clientCount < 1) {
        returnVal = WSAWaitForMultipleEvents(1, &ListenEvent, false, 50, false);

        if ((returnVal != WSA_WAIT_TIMEOUT) && (returnVal != WSA_WAIT_FAILED)) {
            eventIndex = returnVal - WSA_WAIT_EVENT_0; //In practice, eventIndex will equal returnVal, but this is here for compatability

            if (WSAEnumNetworkEvents(ListenSocket, ListenEvent, &NetworkEvents) == SOCKET_ERROR) {
                die("Retrieving event information failed");
            }
            if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
            {
                if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
                    printf("FD_ACCEPT failed with error %d\n", NetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
                    return false;
                }
                // Accept a new connection, and add it to the socket and event lists
                AcceptSocket = accept(ListenSocket, NULL, NULL);
                AcceptEvent = WSACreateEvent();

                WSAEventSelect(AcceptSocket, AcceptEvent, FD_CLOSE|FD_READ); //Socket will accept Close and Read events
                clientCount++;

                printf("Socket %d connected\n", AcceptSocket);
            }
        }
        else if (returnVal == WSA_WAIT_TIMEOUT) {
            //All good, we just have no activity
        }
        else if (returnVal == WSA_WAIT_FAILED) {
            die("WSAWaitForMultipleEvents failed!");
        }
    }
    return true;
}

bool WinsockHelper::HandleIncomingEvents()
{
    DWORD returnVal;

    if (clientCount > 0) { //If connected to the game client

        returnVal = WSAWaitForMultipleEvents(1, &AcceptEvent, false, 0, false); //Check if any network events allow winsock functions to run without blocking
        if ((returnVal != WSA_WAIT_TIMEOUT) && (returnVal != WSA_WAIT_FAILED)) {

            if (WSAEnumNetworkEvents(AcceptSocket, AcceptEvent, &NetworkEvents) == SOCKET_ERROR) {	//Get the network events as binary flags
                PrintError("Retrieving event information failed");
            }
            if (NetworkEvents.lNetworkEvents & FD_CLOSE)	//Server has closed the connection
            {
                printf("Server closed the connection\n");


                //We ignore the error if the client just force quit
                if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0 && NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 10053)
                {
                    printf("FD_CLOSE failed with error %d\n", NetworkEvents.iErrorCode[FD_CLOSE_BIT]);

                    return false;
                }

                CleanupSocket();
                return false;
            }
            else if (NetworkEvents.lNetworkEvents & FD_READ)	//Server has sent data to read
            {
                if (NetworkEvents.iErrorCode[FD_READ_BIT] != 0) {
                    printf("FD_ACCEPT failed with error %d\n", NetworkEvents.iErrorCode[FD_READ_BIT]);

                    return false;
                }

                decodeNetworkMessage(AcceptSocket);

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
