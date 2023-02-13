#include "pch.h"
#include "IDLTesting.BluetoothLEDeviceDisplay.h"
#include "IDLTesting.BluetoothLEDeviceDisplay.g.cpp"


namespace winrt::IDLTesting::implementation
{
    BluetoothLEDeviceDisplay::BluetoothLEDeviceDisplay(Windows::Devices::Enumeration::DeviceInformation const& deviceInfoIn)
        : m_deviceInformation(deviceInfoIn)
    {
    }

    void BluetoothLEDeviceDisplay::Update(winrt::Windows::Devices::Enumeration::DeviceInformationUpdate const& deviceInfoUpdate)
    {
        m_deviceInformation.Update(deviceInfoUpdate);

        //OnPropertyChanged(L"Id");
        //OnPropertyChanged(L"Name");
        //OnPropertyChanged(L"DeviceInformation");
        //OnPropertyChanged(L"IsPaired");
        //OnPropertyChanged(L"IsConnected");
        //OnPropertyChanged(L"Properties");
        //OnPropertyChanged(L"IsConnectable");

        //UpdateGlyphBitmapImage();
    }
    //bool BluetoothLEDeviceDisplay::LookupBooleanProperty(param::hstring const& property)
    //{
    //    auto value = m_deviceInformation.Properties().TryLookup(property);
    //    return value && unbox_value<bool>(value);
    //}

}
