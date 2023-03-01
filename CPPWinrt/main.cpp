#include "pch.h"

#include "IDLTesting.LiteWatcher.h"
#include <iostream>
#include <bluetoothleapis.h>
#include <future>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#define CYCLE_POWER_FEATURE L"{00002a65-0000-1000-8000-00805f9b34fb}"
#define CYCLE_POWER_MEASURE L"{00002a63-0000-1000-8000-00805f9b34fb}"
#define CYCLING_POWER_SERVICE L"{00001818-0000-1000-8000-00805f9b34fb}"


using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Storage::Streams;

void PrintDevInfoKind(DeviceInformationKind kind);
bool ConnectDevice(IDLTesting::BluetoothLEDeviceDisplay& device);
void ReadBuffer();
void InitWinSock();
void startServer(); 
bool PollForConnection();
hstring bikeId = L"BluetoothLE#BluetoothLE70:66:55:73:ad:8a-fe:4a:3f:d8:8a:ee";
array_view<uint8_t> readData;   //This variable is the main point of failure ?w
bool isSubscribed = false;

GattDeviceService currentSelectedService = NULL;
GattCharacteristic currentSelectedCharacteristic = NULL;

#define SERVERIP "127.0.0.1"
#define SERVERPORT 5555
#define MESSAGESIZE 40

//TODO: Boy, having a socket variable for each client will get tiring fast.
// It would be nice to have a nicer system for this...
SOCKET ListenSocket, AcceptSocket;
WSAEVENT ListenEvent, AcceptEvent;
int eventCount = 0;
int clientCount = 0;

//Structure to hold the result from WSAEnumNetworkEvents
WSANETWORKEVENTS NetworkEvents;

int eventIndex = 0;


// The (fixed) size of message that we send between the two programs
#define MESSAGESIZE 40

int main()
{
    init_apartment();
    Uri uri(L"http://aka.ms/cppwinrt");
    printf("Hello, %ls!\n", uri.AbsoluteUri().c_str());

    auto watcher = make<IDLTesting::implementation::LiteWatcher>();
    watcher.EnumerateButton_Click();
    auto devList = watcher.KnownDevices();

    printf("This program only scans for new devices for 30 seconds\nEnter any text continue and try connect the indoor bike\nREMEMBER TO enter 'stop' to stop the program\n\n\n");

    InitWinSock();
    startServer();

    bool mustClose = false;
    while (!mustClose) {

        PollForConnection();
        
        if (clientCount == 1) {
            printf("the server works properly!\n");

            // We'll use this buffer to hold what we receive from the server.
            char buffer[MESSAGESIZE];

            // Read a line of text from the user.
            int power = 444;
            // Now "line" contains what the user typed (without the trailing \n).
            std::string boop = std::to_string(htons(power));

            // Copy the line into the buffer, filling the rest with dashes.
            // (We must be careful not to write past the end of the array.)
            memset(buffer, '.', MESSAGESIZE);
            memcpy(buffer, &boop, min(boop.size(), MESSAGESIZE));

            
            

            // Send the message to the server.
            if (send(AcceptSocket, buffer, MESSAGESIZE, 0) != MESSAGESIZE)
            {
                //die("send failed");
                printf("failed to send");
            }

            //std::string inp = "123";
            //while (inp != "stop")
            //{
            //    uint32_t size = devList.Size();
            //    for (uint32_t index = 0; index < size; index++)
            //    {
            //        auto inspectDevice = devList.GetAt(index);
            //        if (inspectDevice != NULL) {
            //            auto bleDeviceDisplay = inspectDevice.as<IDLTesting::BluetoothLEDeviceDisplay>();

            //            if (bleDeviceDisplay != NULL) {
            //                if (bleDeviceDisplay.DeviceInformation().Id() == bikeId) {
            //                    printf("Device Found:\n\tName: %ls", bleDeviceDisplay.DeviceInformation().Name().c_str());
            //                    printf("\tID: %ls", bleDeviceDisplay.DeviceInformation().Id().c_str());
            //                    PrintDevInfoKind(bleDeviceDisplay.DeviceInformation().Kind());
            //                    printf("\n");

            //                    if (ConnectDevice(bleDeviceDisplay)) { //Sets all vars used in ReadBuffer to target bleDeviceDisplay
            //                        printf("ConnectDevice Ran Successfully\n");

            //                        printf("\n");
            //                    }
            //                    else {
            //                        printf("ConnectDevice Failed\n");
            //                    }
            //                    printf("\n");
            //                }
            //            }
            //        }
            //    }

            //    printf("Size of list is, %u .\n", devList.Size());

            //    std::cin >> inp;
            //}
        }
    }
    if (isSubscribed) {
        GattCommunicationStatus status = currentSelectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
            GattClientCharacteristicConfigurationDescriptorValue::None).get();
        if (status == GattCommunicationStatus::Success)
        {
            printf("Unsubscribed from BT char\n");
            isSubscribed = false;
        }
    }
}

void die(const char* message) {
    fprintf(stderr, "Error: %s (WSAGetLastError() = %d)\n", message, WSAGetLastError());

#ifdef _DEBUG
    // Debug build -- drop the program into the debugger.
    abort();
#else
    exit(1);
#endif
}

void InitWinSock() {
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





void startServer() {
    printf("Server starting\n");

    //Build socket address structure for binding the socket
    sockaddr_in InetAddr;
    InetAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(InetAddr.sin_addr));
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

bool PollForConnection() {
    DWORD returnVal;

    if (clientCount < 1) {
        returnVal = WSAWaitForMultipleEvents(1, &ListenEvent, false, 0, false);

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
        returnVal = WSAWaitForMultipleEvents(1, &AcceptEvent, false, 0, false);
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

void PrintDevInfoKind(DeviceInformationKind kind) {
    switch (kind)
    {
    case(DeviceInformationKind::AssociationEndpoint):
        printf("\t Kind: AssociationEndpoint");
        break;
    case(DeviceInformationKind::AssociationEndpointContainer):
        printf("\t Kind: AssociationEndpointContainer");
        break;
    case(DeviceInformationKind::AssociationEndpointService):
        printf("\t Kind: AssociationEndpointService");
        break;
    case(DeviceInformationKind::Device):
        printf("\t Kind: Device");
        break;
    case(DeviceInformationKind::DeviceContainer):
        printf("\t Kind: DeviceContainer");
        break;
    case(DeviceInformationKind::DeviceInterface):
        printf("\t Kind: DeviceInterface");
        break;
    case(DeviceInformationKind::DeviceInterfaceClass):
        printf("\t Kind: DeviceInterfaceClass");
        break;
    case(DeviceInformationKind::DevicePanel):
        printf("\t Kind: DevicePanel");
        break;
    case(DeviceInformationKind::Unknown):
        printf("\t Kind: Unknown");
        break;
    default:
        break;
    }
}

//https://stackoverflow.com/questions/63336568/bluetooth-le-characteristic-values-only-read-once
//translated to english and C++ ?W

using namespace Windows::Foundation::Collections;
IVectorView<GattCharacteristic> characteristics;
IVectorView<GattDeviceService> services;



bool ConnectDevice(IDLTesting::BluetoothLEDeviceDisplay& device)
{
    //get bluetooth device information
     BluetoothLEDevice bluetoothLeDevice = BluetoothLEDevice::FromIdAsync(device.DeviceInformation().Id()).get(); //.get() makes this no longer Async
    //Respond(bluetoothLeDevice.ConnectionStatus.ToString());

     if (bluetoothLeDevice != NULL) { //Redundant error prevention

         //get device's services
         GattDeviceServicesResult result = bluetoothLeDevice.GetGattServicesAsync().get(); //.get() makes this no longer Async

         if (result.Status() == GattCommunicationStatus::Success)
         {
             //store all device services
             services = result.Services();

             //loop each services in list
             for (auto serv : services)
             {
                 //get serviceName from service UUID interface
                 hstring ServiceName = to_hstring(serv.Uuid()); //Using hstring instead of std::string for compatability with winrt


                 //Search for Cycling Power Service
                 if (ServiceName == CYCLING_POWER_SERVICE)
                 {
                     //store the current service
                     currentSelectedService = serv;

                     //get all characteristics from current service
                     GattCharacteristicsResult resultCharacteristics = serv.GetCharacteristicsAsync().get();//.get() makes this no longer Async

                     //verify if getting characteristics is success 
                     if (resultCharacteristics.Status() == GattCommunicationStatus::Success)
                     {
                         //store device services to list
                         characteristics = resultCharacteristics.Characteristics();

                         //loop through its characteristics
                         for (auto chara : characteristics)
                         {
                             hstring CharacteristicName = to_hstring(chara.Uuid()); //Using hstring instead of std::string for compatability with winrt

                             printf("Checking characs: %ls ", CharacteristicName.c_str());

                             //Search for Cycling Power Measurement characteristic
                             if (CharacteristicName == CYCLE_POWER_MEASURE)
                             {
                                 printf("BikeBLEDevice.CyclingPowerService.CyclingPowerMeasurement Has been found \n");

                                 //store the current characteristic
                                 currentSelectedCharacteristic = chara;
                                 

                                 GattCharacteristicProperties properties = chara.CharacteristicProperties();

                                 //if selected characteristics has notify property
                                 if (static_cast<uint32_t>(properties) & static_cast<uint32_t>(GattCharacteristicProperties::Notify)) //properties.HasFlag(GattCharacteristicProperties::Notify)
                                 {
                                    if (isSubscribed == false) {

                                        GattCommunicationStatus status = currentSelectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                                            GattClientCharacteristicConfigurationDescriptorValue::Notify).get(); //.get() makes this no longer Async

                                        if (status == GattCommunicationStatus::Success)
                                        {
                                            // Server has been informed of clients interest.
                                            printf("Subscribed to notification\n\n");
                                            isSubscribed = true;
                                            device.NotifyOnCharacteristicChange(currentSelectedCharacteristic);
                                        }
                                    }
                                 }
                                 


                                 return true;
                             }
                             //printf("\n"); //for printing characs
                         }
                         if (currentSelectedCharacteristic == NULL) {
                             printf("Could not find Cycling power measurement characteristic on device\n");
                         }
                     }
                     else{ printf("main.cpp line 179: Failed to retrieve characteristics data\n"); }
                     
                 }
                 //printf("\n"); //for printing servs
             }
             if (currentSelectedService == NULL) {
                 printf("Could not find Cycling power service on device\n");
             }
         }
         else { printf("main.cpp line 153: Failed to retrieve service data\n"); }
     }
    return false;
}