#include "pch.h"
#include "IDLTesting.LiteWatcher.h"
#include "IDLTesting.LiteWatcher.g.cpp"
#include "IDLTesting.BluetoothLEDeviceDisplay.h"
#include <iostream>

using namespace winrt;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation;

namespace winrt::IDLTesting::implementation
{
    LiteWatcher::LiteWatcher() {
        std::cout << "IM ALIVE!\n\n";
    }
    LiteWatcher::~LiteWatcher() {
        
        if (deviceWatcher != nullptr)
        {
            StopBleDeviceWatcher();
            std::cout << "Enumration Stopped\n";
        }
        std::cout << "IM DEAD!\n\n";
    }

    void LiteWatcher::EnumerateButton_Click()
    {
        if (deviceWatcher == nullptr)
        {
            StartBleDeviceWatcher();
            std::cout << "Enumration Started\n";
        }
        else
        {
            StopBleDeviceWatcher();
            std::cout << "Enumration Stopped\n";
        }
    }
    void LiteWatcher::PairButton_Click()
    {
        throw hresult_not_implemented();
    }
    bool LiteWatcher::Not(bool value)
    {
        throw hresult_not_implemented();
    }
    void LiteWatcher::StartBleDeviceWatcher()
    {
        // Additional properties we would like about the device.
       // Property strings are documented here https://msdn.microsoft.com/en-us/library/windows/desktop/ff521659(v=vs.85).aspx
        auto requestedProperties = single_threaded_vector<hstring>({ L"System.Devices.Aep.DeviceAddress", L"System.Devices.Aep.IsConnected", L"System.Devices.Aep.Bluetooth.Le.IsConnectable" });

        // BT_Code: Example showing paired and non-paired in a single query.
        hstring aqsAllBluetoothLEDevices = L"(System.Devices.Aep.ProtocolId:=\"{bb7bb05e-5972-42b5-94fc-76eaa7084d49}\")";

        deviceWatcher =
            Windows::Devices::Enumeration::DeviceInformation::CreateWatcher(
                aqsAllBluetoothLEDevices,
                requestedProperties,
                DeviceInformationKind::AssociationEndpoint);

        // Register event handlers before starting the watcher.
        deviceWatcherAddedToken = deviceWatcher.Added({ get_weak(), &LiteWatcher::DeviceWatcher_Added });
        deviceWatcherUpdatedToken = deviceWatcher.Updated({ get_weak(), &LiteWatcher::DeviceWatcher_Updated });
        deviceWatcherRemovedToken = deviceWatcher.Removed({ get_weak(), &LiteWatcher::DeviceWatcher_Removed });
        deviceWatcherEnumerationCompletedToken = deviceWatcher.EnumerationCompleted({ get_weak(), &LiteWatcher::DeviceWatcher_EnumerationCompleted });
        deviceWatcherStoppedToken = deviceWatcher.Stopped({ get_weak(), &LiteWatcher::DeviceWatcher_Stopped });

        // Start over with an empty collection.
        m_knownDevices.Clear();

        // Start the watcher. Active enumeration is limited to approximately 30 seconds.
        // This limits power usage and reduces interference with other Bluetooth activities.
        // To monitor for the presence of Bluetooth LE devices for an extended period,
        // use the BluetoothLEAdvertisementWatcher runtime class. See the BluetoothAdvertisement
        // sample for an example.
        deviceWatcher.Start();
    }

    void LiteWatcher::StopBleDeviceWatcher()
    {
        if (deviceWatcher != nullptr)
        {
            // Unregister the event handlers.
            deviceWatcher.Added(deviceWatcherAddedToken);
            deviceWatcher.Updated(deviceWatcherUpdatedToken);
            deviceWatcher.Removed(deviceWatcherRemovedToken);
            deviceWatcher.EnumerationCompleted(deviceWatcherEnumerationCompletedToken);
            deviceWatcher.Stopped(deviceWatcherStoppedToken);

            // Stop the watcher.
            deviceWatcher.Stop();
            deviceWatcher = nullptr;
        }
    }

    std::tuple<winrt::IDLTesting::BluetoothLEDeviceDisplay, uint32_t> LiteWatcher::FindBluetoothLEDevice(hstring const& id)
    {
        uint32_t size = m_knownDevices.Size();
        for (uint32_t index = 0; index < size; index++)
        {
            auto bleDeviceDisplay = m_knownDevices.GetAt(index).as<IDLTesting::BluetoothLEDeviceDisplay>();
            if (bleDeviceDisplay.Id() == id)
            {
                return { bleDeviceDisplay, index };
            }
        }
        return { nullptr, 0 - 1U };
    }

    std::vector<Windows::Devices::Enumeration::DeviceInformation>::iterator LiteWatcher::FindUnknownDevices(hstring const& id)
    {
        return std::find_if(UnknownDevices.begin(), UnknownDevices.end(), [&](auto&& bleDeviceInfo)
            {
                return bleDeviceInfo.Id() == id;
            });
    }

    fire_and_forget LiteWatcher::DeviceWatcher_Added(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformation deviceInfo)
    {
        OutputDebugStringW((L"Added " + deviceInfo.Id() + deviceInfo.Name()).c_str());

        // Protect against race condition if the task runs after the app stopped the deviceWatcher.
        if (sender == deviceWatcher)
        {
            // Make sure device isn't already present in the list.
            if (std::get<0>(FindBluetoothLEDevice(deviceInfo.Id())) == nullptr)
            {
                if (!deviceInfo.Name().empty())
                {
                    // If device has a friendly name display it immediately.
                    m_knownDevices.Append(make<BluetoothLEDeviceDisplay>(deviceInfo));
                }
                else
                {
                    // Add it to a list in case the name gets updated later. 
                    UnknownDevices.push_back(deviceInfo);
                }
            }
        }
        co_return;
    }

    fire_and_forget LiteWatcher::DeviceWatcher_Updated(DeviceWatcher sender, DeviceInformationUpdate deviceInfoUpdate)
    {

        OutputDebugStringW((L"Updated " + deviceInfoUpdate.Id()).c_str());

        // Protect against race condition if the task runs after the app stopped the deviceWatcher.
        if (sender == deviceWatcher)
        {
            IDLTesting::BluetoothLEDeviceDisplay bleDeviceDisplay = std::get<0>(FindBluetoothLEDevice(deviceInfoUpdate.Id()));
            if (bleDeviceDisplay != nullptr)
            {
                // Device is already being displayed - update UX.
                bleDeviceDisplay.Update(deviceInfoUpdate);
                co_return;
            }

            auto deviceInfo = FindUnknownDevices(deviceInfoUpdate.Id());
            if (deviceInfo != UnknownDevices.end())
            {
                deviceInfo->Update(deviceInfoUpdate);
                // If device has been updated with a friendly name it's no longer unknown.
                if (!deviceInfo->Name().empty())
                {
                    m_knownDevices.Append(make<BluetoothLEDeviceDisplay>(*deviceInfo));
                    UnknownDevices.erase(deviceInfo);
                }
            }
        }
        co_return;
    }

    fire_and_forget LiteWatcher::DeviceWatcher_Removed(DeviceWatcher sender, DeviceInformationUpdate deviceInfoUpdate)
    {

        OutputDebugStringW((L"Removed " + deviceInfoUpdate.Id()).c_str());

        // Protect against race condition if the task runs after the app stopped the deviceWatcher.
        if (sender == deviceWatcher)
        {
            // Find the corresponding DeviceInformation in the collection and remove it.
            auto [bleDeviceDisplay, index] = FindBluetoothLEDevice(deviceInfoUpdate.Id());
            if (bleDeviceDisplay != nullptr)
            {
                m_knownDevices.RemoveAt(index);
            }

            auto deviceInfo = FindUnknownDevices(deviceInfoUpdate.Id());
            if (deviceInfo != UnknownDevices.end())
            {
                UnknownDevices.erase(deviceInfo);
            }
        }
        co_return;
    }

    fire_and_forget LiteWatcher::DeviceWatcher_EnumerationCompleted(DeviceWatcher sender, IInspectable const&)
    {

        // Protect against race condition if the task runs after the app stopped the deviceWatcher.
        //if (sender == deviceWatcher)
        //{
        //    rootPage.NotifyUser(to_hstring(m_knownDevices.Size()) + L" devices found. Enumeration completed.",
        //        NotifyType::StatusMessage);
        //}
        co_return;
    }

    fire_and_forget LiteWatcher::DeviceWatcher_Stopped(DeviceWatcher sender, IInspectable const&)
    {

        //// Protect against race condition if the task runs after the app stopped the deviceWatcher.
        //if (sender == deviceWatcher)
        //{
        //    rootPage.NotifyUser(L"No longer watching for devices.",
        //        sender.Status() == DeviceWatcherStatus::Aborted ? NotifyType::ErrorMessage : NotifyType::StatusMessage);
        //}
        co_return;
    }

}
