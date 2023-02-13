#pragma once
#include "IDLTesting.BluetoothLEDeviceDisplay.g.h"


namespace winrt::IDLTesting::implementation
{
    struct BluetoothLEDeviceDisplay : BluetoothLEDeviceDisplayT<BluetoothLEDeviceDisplay>
    {
        BluetoothLEDeviceDisplay(Windows::Devices::Enumeration::DeviceInformation const& deviceInfoIn);

        Windows::Devices::Enumeration::DeviceInformation DeviceInformation()
        {
            return m_deviceInformation;
        }

        hstring Id()
        {
            return m_deviceInformation.Id();
        }

        hstring Name()
        {
            return m_deviceInformation.Name();
        }

        bool IsPaired()
        {
            return m_deviceInformation.Pairing().IsPaired();
        }

        bool IsConnected()
        {
            return false;// LookupBooleanProperty(L"System.Devices.Aep.IsConnected");
        }

        bool IsConnectable()
        {
            return false;// LookupBooleanProperty(L"System.Devices.Aep.Bluetooth.Le.IsConnectable");
        }

        Windows::Foundation::Collections::IMapView<hstring, Windows::Foundation::IInspectable> Properties()
        {
            return m_deviceInformation.Properties();
        }

        void Update(Windows::Devices::Enumeration::DeviceInformationUpdate const& deviceInfoUpdate);

        //void PropertyChanged(event_token const& token) noexcept;

    private:
        Windows::Devices::Enumeration::DeviceInformation m_deviceInformation{ nullptr };
        //event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;

        //void OnPropertyChanged(param::hstring const& property);
        //bool LookupBooleanProperty(param::hstring const& property);
    };
}
