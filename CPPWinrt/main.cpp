#include "pch.h"

#include "IDLTesting.LiteWatcher.h"
#include <iostream>
#include <bluetoothleapis.h>
#include <future>

#define CYCLE_POWER_MEASURE L"{00002a65-0000-1000-8000-00805f9b34fb}"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Storage::Streams;

void PrintDevInfoKind(DeviceInformationKind kind);
bool ConnectDevice(DeviceInformation deviceInfo);
void ReadBuffer();

hstring bikeId = L"BluetoothLE#BluetoothLE70:66:55:73:ad:8a-fe:4a:3f:d8:8a:ee";
array_view<uint8_t> readData;   //This variable is the main point of failure ?w


int main()
{
    init_apartment();
    Uri uri(L"http://aka.ms/cppwinrt");
    printf("Hello, %ls!\n", uri.AbsoluteUri().c_str());

    auto watcher = make<IDLTesting::implementation::LiteWatcher>();
    watcher.EnumerateButton_Click();
    auto devList = watcher.KnownDevices();

    printf("This program only scans for 30 seconds\nEnter any text to print device watcher's found BLE devices list\nenter 'stop' to stop the program\n\n\n");

    std::string inp = "123";
    while (inp != "stop")
    {
        uint32_t size = devList.Size();
        for (uint32_t index = 0; index < size; index++)
        {
            auto bleDeviceDisplay = devList.GetAt(index).as<IDLTesting::BluetoothLEDeviceDisplay>();
            

            if (bleDeviceDisplay.DeviceInformation().Id() == bikeId) {
                printf("Device Found:\n\tName: %ls", bleDeviceDisplay.DeviceInformation().Name().c_str());
                printf("\tID: %ls", bleDeviceDisplay.DeviceInformation().Id().c_str());
                PrintDevInfoKind(bleDeviceDisplay.DeviceInformation().Kind());
                printf("\n");

                if (ConnectDevice(bleDeviceDisplay.DeviceInformation())) { //Sets all vars used in ReadBuffer to target bleDeviceDisplay
                    printf("ConnectDevice Ran Successfully\n");
                    ReadBuffer();   //reads the data in (hopefully)
                    printf("Data read: %u", readData); //generates annoying compiler error, but should be fine
                    printf("\n");
                }
                printf("\n");
            }
        }

        printf("Size of list is, %u .\n", devList.Size());

        std::cin >> inp;
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


GattDeviceService currentSelectedService = NULL;
GattCharacteristic currentSelectedCharacteristic = NULL;

bool ConnectDevice(DeviceInformation deviceInfo)
{
    //get bluetooth device information
     BluetoothLEDevice bluetoothLeDevice = BluetoothLEDevice::FromIdAsync(deviceInfo.Id()).get(); //no longer async but who cares ?W
    //Respond(bluetoothLeDevice.ConnectionStatus.ToString());

    //get its services
    GattDeviceServicesResult result = bluetoothLeDevice.GetGattServicesAsync().get(); //no longer async but who cares ?W

    //verify if getting success 
    if (result.Status() == GattCommunicationStatus::Success)
    {
        //store device services to list
        services = result.Services();

        //loop each services in list
        for(auto serv : services)
        {
            //get serviceName by converting the service UUID
            hstring ServiceName = to_hstring(serv.Uuid()); //Using hstring instead of std::string coz it works ?W
            
            //printf("Checking Services: %ls ", ServiceName.c_str());

            //if current servicename matches the input service name / 65520 = 0xFFF0
            // 0x1826, 0x1818
            if (ServiceName == L"{00001818-0000-1000-8000-00805f9b34fb}") //ServiceTxtBox.Text)
            {
                //store the current service
                currentSelectedService = serv;

                //get the current service characteristics
                GattCharacteristicsResult resultCharacterics = serv.GetCharacteristicsAsync().get();//no longer async but who cares ?W

                //verify if getting characteristics is success 
                if (resultCharacterics.Status() == GattCommunicationStatus::Success)
                {
                    //store device services to list
                    characteristics = resultCharacterics.Characteristics();

                    //loop through its characteristics
                    for(auto chara : characteristics)
                    {
                        //get CharacteristicName by converting the current characteristic UUID
                        hstring CharacteristicName = to_hstring(chara.Uuid()); //Using hstring instead of std::string coz it works ?W

                        printf("Checking characs: %ls ", CharacteristicName.c_str());

                        //if current CharacteristicName matches the input characteristic name / 65524 = 0xFFF4
                        if (CharacteristicName == CYCLE_POWER_MEASURE)//CharacteristicsTxtBox.Text)
                        {
                            printf("WE FOUND THE POWER!!! \n");
                            //store the current characteristic
                            currentSelectedCharacteristic = chara;
                            //stop method execution  
                            //done = true;
                            return true;
                        }
                        printf("\n"); //for printing characs
                    }
                }
            }
            //printf("\n"); //for printing servs
        }
    }
    return false;
}


//Funktion ließt die Charakteristik und übergibt sie asynchron
//Function reads the characteristic and passes it asynchronously ?W
void ReadBuffer()
{
    //std::vector<Byte> ret;
    if (currentSelectedService != NULL && currentSelectedCharacteristic != NULL)
    {
        GattCharacteristicProperties properties = currentSelectedCharacteristic.CharacteristicProperties();
        
        //if selected characteristics has read property
        if (static_cast<uint32_t>(properties) & static_cast<uint32_t>(GattCharacteristicProperties::Read)) //properties.HasFlag(GattCharacteristicProperties::Read)?W
        {
            //read value asynchronously
            GattReadResult result = currentSelectedCharacteristic.ReadValueAsync(BluetoothCacheMode::Cached).get(); //Dunno why its Uncached, it just is ?W
            if (result.Status() == GattCommunicationStatus::Success)
            {
                //result.Value()
                //printf("Data read is: ");
                auto reader = DataReader::FromBuffer(result.Value());
                reader.ReadBytes(readData);
                return;// ret;
            }
            return;// null;
        }
        return;// null;
    }
    //return null;
}

