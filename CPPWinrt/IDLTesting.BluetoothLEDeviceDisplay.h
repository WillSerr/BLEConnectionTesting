#pragma once
#include "IDLTesting.BluetoothLEDeviceDisplay.g.h"


using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;

namespace winrt::IDLTesting::implementation
{
    struct BluetoothLEDeviceDisplay : BluetoothLEDeviceDisplayT<BluetoothLEDeviceDisplay>
    {
        BluetoothLEDeviceDisplay(Windows::Devices::Enumeration::DeviceInformation const& deviceInfoIn);
        ~BluetoothLEDeviceDisplay();

        Windows::Devices::Enumeration::DeviceInformation DeviceInformation()
        {
            return m_deviceInformation;
        }

        bool Updated();

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

        int16_t Power()
        {
            updated = false;
            return power;
        }

        Windows::Foundation::Collections::IMapView<hstring, Windows::Foundation::IInspectable> Properties()
        {
            return m_deviceInformation.Properties();
        }

        void Update(Windows::Devices::Enumeration::DeviceInformationUpdate const& deviceInfoUpdate);


        void NotifyOnCharacteristicChange(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const& sender);
        
        void StopNotifyOnCharacteristicChange();

    private:
        Windows::Devices::Enumeration::DeviceInformation m_deviceInformation{ nullptr };
        fire_and_forget characteristicNotification(GattCharacteristic sender,
            GattValueChangedEventArgs args);
        event_token NotifyToken;
        winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const* currentSubscribedCharacteristic{ nullptr };
        //event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;

        //void OnPropertyChanged(param::hstring const& property);
        //bool LookupBooleanProperty(param::hstring const& property);

        bool updated = false;
        int16_t power = 0;
    };
}
