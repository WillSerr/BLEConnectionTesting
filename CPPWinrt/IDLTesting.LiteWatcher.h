#pragma once
#include "IDLTesting.LiteWatcher.g.h"


namespace winrt::IDLTesting::implementation
{
    struct LiteWatcher : LiteWatcherT<LiteWatcher>
    {
        LiteWatcher();
        ~LiteWatcher();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> KnownDevices() {
            return m_knownDevices;
        }
       
        bool BikeConnected();
        bool BikeUpdated();
        int16_t BikePower();

        void EnumerateButton_Click();

        bool SubscribeToPowerData(hstring const& Id);
        bool UnSubscribeToPowerData();

    private:
        Windows::Foundation::Collections::IObservableVector<Windows::Foundation::IInspectable> m_knownDevices = single_threaded_observable_vector<Windows::Foundation::IInspectable>();
        std::vector<Windows::Devices::Enumeration::DeviceInformation> UnknownDevices;
        Windows::Devices::Enumeration::DeviceWatcher deviceWatcher{ nullptr };
        event_token deviceWatcherAddedToken;
        event_token deviceWatcherUpdatedToken;
        event_token deviceWatcherRemovedToken;
        event_token deviceWatcherEnumerationCompletedToken;
        event_token deviceWatcherStoppedToken;

        Windows::Devices::Bluetooth::BluetoothLEDevice bluetoothLeDevice{ nullptr };
        winrt::IDLTesting::BluetoothLEDeviceDisplay bluetoothLeDeviceDisplay{ nullptr };
        Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic selectedCharacteristic{ nullptr };
        void StartBleDeviceWatcher();
        void StopBleDeviceWatcher();
        std::tuple<winrt::IDLTesting::BluetoothLEDeviceDisplay, uint32_t> FindBluetoothLEDevice(hstring const& id);
        std::vector<Windows::Devices::Enumeration::DeviceInformation>::iterator FindUnknownDevices(hstring const& id);

        fire_and_forget DeviceWatcher_Added(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformation deviceInfo);
        fire_and_forget DeviceWatcher_Updated(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate);
        fire_and_forget DeviceWatcher_Removed(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate);
        fire_and_forget DeviceWatcher_EnumerationCompleted(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Foundation::IInspectable const&);
        fire_and_forget DeviceWatcher_Stopped(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Foundation::IInspectable const&);

        fire_and_forget characteristicNotification(Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic sender,
            Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs args);
        event_token NotifyToken;

        bool updated = false;
        int16_t power = 0;
    };
}
namespace winrt::IDLTesting::factory_implementation
{
    struct LiteWatcher : LiteWatcherT<LiteWatcher, implementation::LiteWatcher>
    {
    };
}
