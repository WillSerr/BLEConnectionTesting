#include "pch.h"
#include "IDLTesting.LiteWatcher.h"
#include "IDLTesting.LiteWatcher.g.cpp"
#include "IDLTesting.BluetoothLEDeviceDisplay.h"
#include <iostream>

using namespace winrt;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;

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

    bool LiteWatcher::SubscribeToPowerData(hstring const& Id)
    {
        if (std::get<0>(FindBluetoothLEDevice(Id)) == nullptr) {
            printf("LiteWatcher: Device not found\n\n");
            return false;
        }
        if (!UnSubscribeToPowerData()) {
            printf("LiteWatcher: failed to unsubscribe from previous Device\n\n");
            return false;
        }

        // BT_Code: BluetoothLEDevice.FromIdAsync must be called from a UI thread because it may prompt for consent.
        bluetoothLeDevice = BluetoothLEDevice::FromIdAsync(Id).get();

        //get device's services
        //GattDeviceServicesResult result = bluetoothLeDevice.GetGattServicesAsync().get(); //.get() makes this no longer Async
        GattDeviceServicesResult result = bluetoothLeDevice.GetGattServicesAsync().get();

        if (result.Status() == GattCommunicationStatus::Success)
        {
            //store all device services
            auto services = result.Services();

            //loop each services in list
            for (GattDeviceService serv : services)
            {

                //Search for Cycling Power Service
                if (serv.Uuid() == GattServiceUuids::CyclingPower())
                {
                    //get all characteristics from current service
                    GattCharacteristicsResult resultCharacteristics = serv.GetCharacteristicsAsync().get();//.get() makes this no longer Async

                    //verify if getting characteristics is success 
                    if (resultCharacteristics.Status() == GattCommunicationStatus::Success)
                    {
                        //store device services to list
                        auto characteristics = resultCharacteristics.Characteristics();

                        //loop through its characteristics
                        for (GattCharacteristic chara : characteristics)
                        {

                            //Search for Cycling Power Measurement characteristic
                            if (chara.Uuid() == GattCharacteristicUuids::CyclingPowerMeasurement())
                            {
                                //store the current characteristic
                                selectedCharacteristic = chara;

                                GattCharacteristicProperties properties = chara.CharacteristicProperties();

                                ////Probably a bette way of doing the code below
                                /*if ((properties & GattCharacteristicProperties::Notify) == GattCharacteristicProperties::Notify) {

                                }*/

                                //if selected characteristics has notify property
                                if (static_cast<uint32_t>(properties) & static_cast<uint32_t>(GattCharacteristicProperties::Notify)) //properties.HasFlag(GattCharacteristicProperties::Notify)
                                {
                                    GattCommunicationStatus status = chara.WriteClientCharacteristicConfigurationDescriptorAsync(
                                        GattClientCharacteristicConfigurationDescriptorValue::Notify).get(); //.get() makes this no longer Async

                                    if (status == GattCommunicationStatus::Success)
                                    {
                                        // Server has been informed of clients interest.
                                        printf("LiteWatcher: Subscribed to notification\n\n");
                                        std::get<0>(FindBluetoothLEDevice(Id)).NotifyOnCharacteristicChange(chara);
                                        return true;
                                    }
                                    else
                                    {
                                        printf("LiteWatcher: Failed to subscribe to notification\n\n");
                                        return false;
                                    }
                                    
                                }
                                printf("LiteWatcher: CyclingPowerMeasurement cannot notify\n\n");
                                return false;
                            }
                        }
                        printf("LiteWatcher: Could not find Cycling power measurement characteristic on device\n\n");
                        return false;
                        
                    }
                    else { 
                        printf("LiteWatcher: Failed to retrieve characteristics data\n\n");
                        return false;
                    }

                }
            }
            printf("Could not find Cycling Power Service on device\n\n");
            return false;
        }
        return false;
    }

    bool LiteWatcher::UnSubscribeToPowerData()
    {
        if (selectedCharacteristic != nullptr && bluetoothLeDevice != nullptr) {
            GattCharacteristicProperties properties = selectedCharacteristic.CharacteristicProperties();

            //if selected characteristics has notify property
            if (static_cast<uint32_t>(properties) & static_cast<uint32_t>(GattCharacteristicProperties::Notify)) //properties.HasFlag(GattCharacteristicProperties::Notify)
            {
                GattCommunicationStatus status = selectedCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                    GattClientCharacteristicConfigurationDescriptorValue::None).get(); //.get() makes this no longer Async

                if (status == GattCommunicationStatus::Success)
                {
                    // Server has been informed of clients interest.
                    printf("LiteWatcher: UnSubscribed to notification\n\n");
                    std::get<0>(FindBluetoothLEDevice(bluetoothLeDevice.BluetoothDeviceId().Id())).StopNotifyOnCharacteristicChange();
                    bluetoothLeDevice = nullptr;
                    selectedCharacteristic = nullptr;
                    return true;
                }
                else
                {
                    printf("LiteWatcher: Failed to subscribe to notification\n\n");
                    return false;
                }

            }            
            return false;
        }
        else if (selectedCharacteristic != nullptr || bluetoothLeDevice != nullptr) {
            return false;
        }
        return true;
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
        OutputDebugStringW(L"\n");

        // Protect against race condition if the task runs after the app stopped the deviceWatcher.
        if (sender == deviceWatcher)
        {
            // Make sure device isn't already present in the list.
            if (std::get<0>(FindBluetoothLEDevice(deviceInfo.Id())) == nullptr)
            {
                bool found = false;
                if (!deviceInfo.Name().empty())
                {                    
                    //BluetoothLEDevice bluetoothLeDevice = BluetoothLEDevice::FromIdAsync(device.DeviceInformation().Id()).get(); //.get() makes this no longer Async
                    BluetoothLEDevice bluetoothLeDevice = co_await BluetoothLEDevice::FromIdAsync(deviceInfo.Id()); //.get() makes this no longer Async

                    if (bluetoothLeDevice != NULL) { //If device successfully created

                        //-----Check if it is a bike trainer
                        //get device's services
                        //GattDeviceServicesResult result = bluetoothLeDevice.GetGattServicesAsync().get(); //.get() makes this no longer Async
                        GattDeviceServicesResult result = co_await bluetoothLeDevice.GetGattServicesAsync();

                        if (result.Status() == GattCommunicationStatus::Success)
                        {
                            //store all device services
                            auto services = result.Services();

                            //loop each services in list
                            for (auto serv : services)
                            {
                                //get serviceName from service UUID interface
                                hstring ServiceName = to_hstring(serv.Uuid()); //Using hstring instead of std::string for compatability with winrt

                                if (ServiceName.size() >= 9) //Redundant error checking
                                {
                                    std::string nameString = to_string(ServiceName.c_str());

                                    //-----Add bikes to the vector
                                    if (serv.Uuid() == GattServiceUuids::CyclingPower())// If Service = Cycle power Service
                                    {
                                        // If device has a friendly name display it immediately.
                                        m_knownDevices.Append(make<BluetoothLEDeviceDisplay>(deviceInfo));
                                        found = true;
                                        break;
                                    }
                                    // NOT IMPLEMENTED YET
                                    //else if (std::char_traits<char>::compare(nameString.c_str(), "{00001826", 9) == 0) //If Service = Fitness Machine service
                                    //{
                                    //    // If device has a friendly name display it immediately.
                                    //    m_knownDevices.Append(make<BluetoothLEDeviceDisplay>(deviceInfo));
                                    //    found = true;
                                    //    break;
                                    //}
                                }
                            }
                        }
                        
                    }
                    if (!found) {
                        UnknownDevices.push_back(deviceInfo);
                    }
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
        OutputDebugStringW(L"\n");

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
        OutputDebugStringW(L"\n");

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
        printf("%i devices found. Enumeration completed.\n", m_knownDevices.Size());
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
        printf("No longer watching for devices.\n");
        co_return;
    }

}
