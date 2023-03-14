#include "pch.h"

#include "IDLTesting.LiteWatcher.h"
#include "WinsockHelper.h"
#include <iostream>
#include <bluetoothleapis.h>
#include <future>

#define TESTMESSAGECOUNT 0


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
hstring bikeId = L"BluetoothLE#BluetoothLE70:66:55:73:ad:8a-fe:4a:3f:d8:8a:ee";
array_view<uint8_t> readData;   //This variable is the main point of failure ?w
bool isSubscribed = false;

GattDeviceService currentSelectedService = NULL;
GattCharacteristic currentSelectedCharacteristic = NULL;


int main()
{
    init_apartment();
    Uri uri(L"http://aka.ms/cppwinrt");
    printf("Hello, %ls!\n", uri.AbsoluteUri().c_str());

    auto watcher = make<IDLTesting::implementation::LiteWatcher>();
    watcher.EnumerateButton_Click();
    auto devList = watcher.KnownDevices();

    printf("This program only scans for new devices for 30 seconds\nEnter any text continue and try connect the indoor bike\nREMEMBER TO enter 'stop' to stop the program\n\n\n");

    WinsockHelper winsockHelper;


    int TESTmessagesToSend = TESTMESSAGECOUNT;


    IDLTesting::BluetoothLEDeviceDisplay connectedBike = NULL;

    bool mustClose = false;
    while (!mustClose) {

        winsockHelper.PollForConnection(); //Try connecting to the Game Listen Server
        
        if (winsockHelper.getClientCount() > 0) { //IF the game is connected
            
#pragma region BikeLessTestMessages
            if (TESTmessagesToSend > 0) {
            //    TESTmessagesToSend -= 1;

            //    WinsockHelper::MessageContents messageBuffer;


            //    messageBuffer.powerValue = 1234;


            //    //Send the message to the server.
            //    if (send(winsockHelper.AcceptSocket, (char*)&messageBuffer, sizeof(WinsockHelper::MessageContents), 0) != sizeof(WinsockHelper::MessageContents))
            //    {
            //        //die("send failed");
            //        printf("failed to send");
            //    }
            //    printf("sent message\n");
            }

#pragma endregion

            if (!isSubscribed) { //Try find the bike in the device list and subscribe to Cycle Power Measurement notifications

                uint32_t size = devList.Size();
                for (uint32_t index = 0; index < size; index++)
                {
                    auto inspectDevice = devList.GetAt(index);
                    if (inspectDevice != NULL) {
                        auto bleDeviceDisplay = inspectDevice.as<IDLTesting::BluetoothLEDeviceDisplay>();

                        if (bleDeviceDisplay != NULL) {
                            if (bleDeviceDisplay.DeviceInformation().Id() == bikeId) {
                                printf("Device Found:\n\tName: %ls", bleDeviceDisplay.DeviceInformation().Name().c_str());
                                printf("\tID: %ls", bleDeviceDisplay.DeviceInformation().Id().c_str());
                                PrintDevInfoKind(bleDeviceDisplay.DeviceInformation().Kind());
                                printf("\n");

                                if (ConnectDevice(bleDeviceDisplay)) { //Sets all vars used in ReadBuffer to target bleDeviceDisplay
                                    printf("ConnectDevice Ran Successfully and has subscirbed\n");

                                    printf("\n");

                                    connectedBike = devList.GetAt(index).as<IDLTesting::BluetoothLEDeviceDisplay>();
                                    break;
                                }
                                else {
                                    printf("ConnectDevice Failed\n");
                                }
                                printf("\n");
                            }
                        }
                    }
                }

                printf("Size of list is, %u .\n", devList.Size());
            }
            else
            {         
                if (connectedBike) {
                    if (connectedBike.Updated())
                    {
                        WinsockHelper::MessageContents messageBuffer;


                        messageBuffer.powerValue = connectedBike.Power();


                        //Send the message to the server.
                        if (send(winsockHelper.AcceptSocket, (char*)&messageBuffer, sizeof(WinsockHelper::MessageContents), 0) != sizeof(WinsockHelper::MessageContents))
                        {
                            //die("send failed");
                            printf("failed to send");
                        }
                        printf("power value: %i", connectedBike.Power());
                    }
                }
            }
        }
        //else {
        //    TESTmessagesToSend = TESTMESSAGECOUNT;
        //}
    }
    if (isSubscribed) {
        GattCommunicationStatus status = currentSelectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
            GattClientCharacteristicConfigurationDescriptorValue::None).get();
        if (status == GattCommunicationStatus::Success)
        {
            printf("Unsubscribed from BT char\n");
            isSubscribed = false;
            connectedBike = NULL;
        }
    }
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

                                        /*currentSelectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                                            GattClientCharacteristicConfigurationDescriptorValue::Notify);*/

                                        if (status == GattCommunicationStatus::Success)
                                        {
                                            // Server has been informed of clients interest.
                                            printf("Subscribed to notification\n\n");
                                            isSubscribed = true;
                                            device.NotifyOnCharacteristicChange(currentSelectedCharacteristic);
                                            
                                            return true;
                                        }
                                        else
                                        {
                                            printf("Failed to subscribe to notification\n\n");

                                        }
                                    }
                                 }
                                 


                                 return false;
                             }
                             //printf("\n"); //for printing characs
                         }
                         if (currentSelectedCharacteristic == NULL) {
                             printf("Could not find Cycling power measurement characteristic on device\n");
                         }
                     }
                     else{ printf("main.cpp line 288: Failed to retrieve characteristics data\n"); }
                     
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