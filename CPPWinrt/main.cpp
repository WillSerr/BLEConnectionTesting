#include "pch.h"

#include "IDLTesting.LiteWatcher.h"
#include <iostream>
#include <bluetoothleapis.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Enumeration;

int main()
{
    init_apartment();
    Uri uri(L"http://aka.ms/cppwinrt");
    printf("Hello, %ls!\n", uri.AbsoluteUri().c_str());

    auto watcher = make<IDLTesting::implementation::LiteWatcher>();
    watcher.EnumerateButton_Click();
    auto devList = watcher.KnownDevices();

    std::string inp = "123";
    while (inp != "stop")
    {
        uint32_t size = devList.Size();
        for (uint32_t index = 0; index < size; index++)
        {
            auto bleDeviceDisplay = devList.GetAt(index).as<IDLTesting::BluetoothLEDeviceDisplay>();
            printf("Device Found:\n\tName: %ls!", bleDeviceDisplay.DeviceInformation().Name().c_str());

            switch (bleDeviceDisplay.DeviceInformation().Kind())
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

            printf("\n");
        }

        printf("Size of list is, %u !\n", devList.Size());

        std::cin >> inp;
    }
}
