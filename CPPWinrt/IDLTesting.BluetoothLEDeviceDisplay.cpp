#include "pch.h"
#include <bluetoothleapis.h>
#include "IDLTesting.BluetoothLEDeviceDisplay.h"
#include "IDLTesting.BluetoothLEDeviceDisplay.g.cpp"



namespace winrt::IDLTesting::implementation
{
    BluetoothLEDeviceDisplay::BluetoothLEDeviceDisplay(Windows::Devices::Enumeration::DeviceInformation const& deviceInfoIn)
        : m_deviceInformation(deviceInfoIn)
    {
    }

    BluetoothLEDeviceDisplay::~BluetoothLEDeviceDisplay()
    {
    }


    void BluetoothLEDeviceDisplay::Update(winrt::Windows::Devices::Enumeration::DeviceInformationUpdate const& deviceInfoUpdate)
    {
        m_deviceInformation.Update(deviceInfoUpdate);
    }

}
