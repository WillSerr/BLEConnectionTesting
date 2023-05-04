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
bool ConnectDeviceByID(std::string ID, Collections::IObservableVector<winrt::Windows::Foundation::IInspectable>* devList);
bool ConnectDevice(IDLTesting::BluetoothLEDeviceDisplay& device);
void DisconnectDevice();
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
IDLTesting::BluetoothLEDeviceDisplay connectedBike = NULL;

int main()
{
    init_apartment();

    printf("Welcome to the intermediary program for Pedal To The Metal!\n\n");

    auto watcher = make<IDLTesting::implementation::LiteWatcher>();
    
    auto devList = watcher.KnownDevices();


    // UPDATETHIS
    //printf("This program only scans for new devices for 30 seconds\nEnter any text continue and try connect the indoor bike\nREMEMBER TO enter 'stop' to stop the program\n\n\n");
    printf("This program only scans for new devices for 30 seconds\n\n\n");

    WinsockHelper winsockHelper;
    winsockHelper.devList = &devList;
    winsockHelper.deviceWatcher = &watcher;

    watcher.EnumerateButton_Click();
    winsockHelper.enumerating = true;

    std::vector< std::string> IDs;
    std::vector< std::string> names;

    bool mustClose = false;
    uint32_t lastSize = -1;
    while (!mustClose) {
        
        if (winsockHelper.getClientCount() > 0) { //IF the game is connected
            winsockHelper.HandleIncomingEvents();
            if (winsockHelper.getClientCount() == 0) {
                break;
            }
            
            if (winsockHelper.bikeIDToConnect != "NULL") {
                //winsockHelper.stopWatcherEnumerating();
                if (!watcher.SubscribeToPowerData(to_hstring(winsockHelper.bikeIDToConnect))) {
                    winsockHelper.sendErrorMessage(WinsockHelper::FailedToConnectToDevice);
                    //winsockHelper.startWatcherEnumerating();
                }
                else {
                    winsockHelper.sendErrorMessage(WinsockHelper::NoError);
                }
                winsockHelper.bikeIDToConnect = "NULL";
            }

            if (!isSubscribed) { //If in device selection mode

                //if (devList.Size() != lastSize) {
                //    printf("\n\nSize of list is, %u .\n", devList.Size());
                //    lastSize = devList.Size();
                //}
            }
            else
            {         
                //if (winsockHelper.enumerating) { //Redundant check but more readable
                //    winsockHelper.stopWatcherEnumerating();
                //}
                if (connectedBike) {
                    if (connectedBike.Updated())
                    {
                        winsockHelper.sendPowerMessage(connectedBike.Power());
                    }
                }
            }
        }
        else {
            winsockHelper.PollForConnection(); //Try connecting to the Game client
        }
    }
    if (isSubscribed) {
        DisconnectDevice();
        connectedBike = NULL;
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

bool ConnectDeviceByID(std::string ID, Collections::IObservableVector<winrt::Windows::Foundation::IInspectable>* devList)
{
    printf("Attempting to connect to %s\n", ID.c_str());

    //Disconnect old device
    if (connectedBike != NULL) {
        DisconnectDevice();
        connectedBike = NULL;
    }

    hstring selectedID = to_hstring(ID);
    bool found = false;

    uint32_t size = devList->Size();
    for (uint32_t index = 0; index < size; index++)
    {
        //If device is valid
        auto inspectDevice = devList->GetAt(index);
        if (inspectDevice != NULL) {
            auto bleDeviceDisplay = inspectDevice.as<IDLTesting::BluetoothLEDeviceDisplay>();
            if (bleDeviceDisplay != NULL) {

                //If device is the selected device
                if (bleDeviceDisplay.DeviceInformation().Id() == selectedID) {
                    found = true;
                    printf("Device Found:\n\tName: %ls", bleDeviceDisplay.DeviceInformation().Name().c_str());
                    printf("\tID: %ls", bleDeviceDisplay.DeviceInformation().Id().c_str());
                    PrintDevInfoKind(bleDeviceDisplay.DeviceInformation().Kind());
                    printf("\n");

                    if (ConnectDevice(bleDeviceDisplay)) { //Sets all vars used in ReadBuffer to target bleDeviceDisplay
                        printf("ConnectDevice Ran Successfully and has subscirbed\n");

                        connectedBike = devList->GetAt(index).as<IDLTesting::BluetoothLEDeviceDisplay>();
                        return true;
                    }
                    else {
                        printf("ConnectDevice Failed\n\n");
                        return false;
                    }
                    printf("\n");
                }
            }
        }
    }
    if (!found) {
        printf("ConnectDevice Failed: failed to find device in device list.\n");
        return false;
    }
    return true;
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

                                        GattCommunicationStatus status = chara.WriteClientCharacteristicConfigurationDescriptorAsync(
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

void DisconnectDevice() {
    if (isSubscribed) {
        GattCommunicationStatus status = currentSelectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
            GattClientCharacteristicConfigurationDescriptorValue::None).get();
        if (status == GattCommunicationStatus::Success)
        {
            printf("Unsubscribed from BT char\n");
            isSubscribed = false;
            currentSelectedService = NULL;
            currentSelectedCharacteristic = NULL;
        }
    }
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