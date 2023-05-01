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
bool IsConnectableTrainer(IDLTesting::BluetoothLEDeviceDisplay& device);

//LAPTOP Kickr Snap ID
hstring bikeId = L"BluetoothLE#BluetoothLE70:66:55:73:ad:8a-fe:4a:3f:d8:8a:ee";
//Computer Kickr Snap ID
//hstring bikeId = L"BluetoothLE#BluetoothLE00:1a:7d:da:71:13-fe:4a:3f:d8:8a:ee";
//Kickr Core
//hstring bikeId = L"BluetoothLE#BluetoothLE00:1a:7d:da:71:13-d3:3f:a8:c6:98:e2";
array_view<uint8_t> readData;   //This variable is the main point of failure ?w
bool isSubscribed = false;

GattDeviceService currentSelectedService = NULL;
GattCharacteristic currentSelectedCharacteristic = NULL;


int main()
{
    init_apartment();
    /* DELETEME
    Uri uri(L"http://aka.ms/cppwinrt");
    printf("Hello, %ls!\n", uri.AbsoluteUri().c_str());*/

    auto watcher = make<IDLTesting::implementation::LiteWatcher>();
    watcher.EnumerateButton_Click();
    auto devList = watcher.KnownDevices();


    // UPDATETHIS
    //printf("This program only scans for new devices for 30 seconds\nEnter any text continue and try connect the indoor bike\nREMEMBER TO enter 'stop' to stop the program\n\n\n");
    printf("This program only scans for new devices for 30 seconds\n\n\n");

    WinsockHelper winsockHelper;

    IDLTesting::BluetoothLEDeviceDisplay connectedBike = NULL;

    std::vector< std::string> IDs;
    std::vector< std::string> names;

    bool mustClose = false;
    while (!mustClose) {
        
        if (winsockHelper.getClientCount() > 0) { //IF the game is connected
            

            if (!isSubscribed) { //If in device selection mode

                IDs.clear();
                uint32_t size = devList.Size();
                for (uint32_t index = 0; index < size; index++)
                {
                    auto inspectDevice = devList.GetAt(index);
                    if (inspectDevice != NULL) {
                        auto bleDeviceDisplay = inspectDevice.as<IDLTesting::BluetoothLEDeviceDisplay>();
                        if (bleDeviceDisplay != NULL) {
                            if (IsConnectableTrainer(bleDeviceDisplay))
                            {
                                printf("Device Found:\tName: %ls", bleDeviceDisplay.DeviceInformation().Name().c_str());
                                printf("\tID: %ls", bleDeviceDisplay.DeviceInformation().Id().c_str());
                                PrintDevInfoKind(bleDeviceDisplay.DeviceInformation().Kind());
                                printf("\n");

                                IDs.push_back(to_string(bleDeviceDisplay.DeviceInformation().Id()));
                                names.push_back(to_string(bleDeviceDisplay.DeviceInformation().Name()));
                                //if (ConnectDevice(bleDeviceDisplay)) { //Sets all vars used in ReadBuffer to target bleDeviceDisplay
                                //    printf("ConnectDevice Ran Successfully and has subscirbed\n");

                                //    printf("\n");

                                //    connectedBike = devList.GetAt(index).as<IDLTesting::BluetoothLEDeviceDisplay>();
                                //    break;
                                //}
                                //else {
                                //    printf("ConnectDevice Failed\n");
                                //    mustClose = true;
                                //}
                                //printf("\n");
                                //}
                            }
                        }
                    }
                }


                printf("\n\nSize of list is, %u .\n", devList.Size());

                //winsockHelper.sendAvailableBikesMessage(IDs.size(), &IDs, &names);

                std::string inputmessage;
                std::cin >> inputmessage;
                if (inputmessage == "stop") {
                    mustClose = true;
                    printf("\n");
                }
                else if (inputmessage == "test") {
                    winsockHelper.sendNetworkTestingMessages();
                }
                else if (inputmessage == "select") {
                    printf("Please enter the ID for the device you wish to inspect");
                    std::string inSelectedID;
                    std::cin >> inSelectedID;

                    hstring selectedID = to_hstring(inSelectedID);
                    bool found = false;

                    uint32_t size = devList.Size();
                    for (uint32_t index = 0; index < size; index++)
                    {
                        auto inspectDevice = devList.GetAt(index);
                        if (inspectDevice != NULL) {
                            auto bleDeviceDisplay = inspectDevice.as<IDLTesting::BluetoothLEDeviceDisplay>();

                            if (bleDeviceDisplay != NULL) {
                                if (bleDeviceDisplay.DeviceInformation().Id() == selectedID) {
                                    found = true;
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
                                        //mustClose = true;
                                    }
                                    printf("\n");
                                }
                            }
                        }
                    }
                }
            }
            else
            {         
                if (connectedBike) {
                    if (connectedBike.Updated())
                    {
                        winsockHelper.sendPowerMessage(connectedBike.Power());
                    }
                }
            }
        }
        else {
            winsockHelper.PollForConnection(); //Try connecting to the Game Listen Server
        }
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

                                 ////Probably a bette way of doing the code below
                                 /*if ((properties & GattCharacteristicProperties::Notify) == GattCharacteristicProperties::Notify) {

                                 }*/

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
                 printf("\n"); //for printing servs
                 
             }
             /*if (currentSelectedService == NULL) {
                 printf("Could not find Cycling power service on device\n");
             }*/
             return true;
         }
         else { 
             
             //printf("%ls", ServiceName.c_str());
             printf("main.cpp: Failed to retrieve service data\n"); 
             return false;
         }
     }
     else {
         printf("main.cpp: Failed to create device from UUID. Check bluetooth is turned on\n");
     }
    return false;
}

bool IsConnectableTrainer(IDLTesting::BluetoothLEDeviceDisplay& device)
{
    //get bluetooth device information
    BluetoothLEDevice bluetoothLeDevice = BluetoothLEDevice::FromIdAsync(device.DeviceInformation().Id()).get(); //.get() makes this no longer Async

    if (bluetoothLeDevice != NULL) { //If device successfully created

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

                if (ServiceName.size() >= 9) //Redundant error checking
                {
                    std::string nameString = to_string(ServiceName.c_str());

                    
                    if (std::char_traits<char>::compare(nameString.c_str(), "{00001818", 9) == 0) // If Service = Cycle power Service
                    {
                        return true;
                    }
                    else if (std::char_traits<char>::compare(nameString.c_str(), "{00001826", 9) == 0) //If Service = Fitness Machine service
                    {
                        return true;
                    }
                }
            }
        }
        else {
            printf("main.cpp: Failed to retrieve service data from %ls\n", device.DeviceInformation().Id().c_str());
        }
    }
    else {
        printf("main.cpp: Failed to create device from UUID: %ls\nCheck bluetooth is turned on\n", device.DeviceInformation().Id().c_str());
    }
    return false;
}

bool DisplayIfBike(IDLTesting::BluetoothLEDeviceDisplay& device) {
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

                printf("%ls", ServiceName.c_str());
                if (ServiceName.size() >= 9);
                {
                    std::string nameString = to_string(ServiceName.c_str());

                    //Cycle power Service
                    if (std::char_traits<char>::compare(nameString.c_str(), "{00001818", 9) == 0) {
                        printf("\t <-- Cycle power Service");
                    }
                    else if (std::char_traits<char>::compare(nameString.c_str(), "{00001826", 9) == 0) {
                        printf("\t <-- Fitness Machine service ");
                    }
                }
                printf("\n");
              

            }
            /*if (currentSelectedService == NULL) {
                printf("Could not find Cycling power service on device\n");
            }*/
            return true;
        }
        else {
            printf("main.cpp: Failed to retrieve service data\n");
            return false;
        }
    }
    else {
        printf("main.cpp: Failed to create device from UUID. Check bluetooth is turned on\n");
    }
    return false;
}