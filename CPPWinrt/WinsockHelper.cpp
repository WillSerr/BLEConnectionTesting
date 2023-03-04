#include "pch.h"
#include "WinsockHelper.h"

WinsockHelper::WinsockHelper() {
    InitWinSock();
    StartServer();
}

int WinsockHelper::getClientCount()
{
    return clientCount;
}

void WinsockHelper::die(const char* message) {
    fprintf(stderr, "Error: %s (WSAGetLastError() = %d)\n", message, WSAGetLastError());

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

                //TODO: It'd be great if we could wait for a Read or Write event too...
                WSAEventSelect(AcceptSocket, AcceptEvent, FD_CLOSE);
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
    if (clientCount > 0) {
        returnVal = WSAWaitForMultipleEvents(1, &AcceptEvent, false, 500, false);
        if ((returnVal != WSA_WAIT_TIMEOUT) && (returnVal != WSA_WAIT_FAILED)) {
            eventIndex = returnVal - WSA_WAIT_EVENT_0; //In practice, eventIndex will equal returnVal, but this is here for compatability

            if (WSAEnumNetworkEvents(AcceptSocket, AcceptEvent, &NetworkEvents) == SOCKET_ERROR) {
                die("Retrieving event information failed");
            }
            if (NetworkEvents.lNetworkEvents & FD_CLOSE)
            {
                //We ignore the error if the client just force quit
                if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0 && NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 10053)
                {
                    printf("FD_CLOSE failed with error %d\n", NetworkEvents.iErrorCode[FD_CLOSE_BIT]);
                    return false;
                }
                //Gracefully Close the socket
                if (closesocket(AcceptSocket) != SOCKET_ERROR) {
                    printf("Successfully closed socket %d\n", AcceptSocket);
                }
                if (WSACloseEvent(AcceptEvent) == false) {
                    die("WSACloseEvent() failed");
                }
                clientCount = 0;
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