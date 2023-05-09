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
        UnSubscribeToPowerData();
        if (deviceWatcher != nullptr)
        {
            StopBleDeviceWatcher();
            std::cout << "Enumration Stopped\n";
        }
        std::cout << "IM DEAD!\n\n";
    }


    bool LiteWatcher::BikeConnected()
    {
        return bluetoothLeDeviceDisplay != nullptr;
    }

    bool LiteWatcher::BikeUpdated()
    {        
        if (updated) {
            updated = false;
            return true;
        }
        else {
            return false;
        }
        //return bluetoothLeDeviceDisplay.Updated();
    }

    int16_t LiteWatcher::BikePower()
    {
        return power;
        //return bluetoothLeDeviceDisplay.Power();
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

    bool LiteWatcher::SubscribeToPowerData(hstring const& Id)
    {
        if (std::get<0>(FindBluetoothLEDevice(Id)) == nullptr) {    //If valid ID
            printf("LiteWatcher: Device not found\n\n");
            return false;
        }
        if (!UnSubscribeToPowerData()) {    //Remove previous subscription
            printf("LiteWatcher: failed to unsubscribe from previous Device\n\n");
            return false;
        }


        auto newBluetoothLeDevice = BluetoothLEDevice::FromIdAsync(Id).get();      //No non-async alternative

        //get device's services
        GattDeviceServicesResult result = newBluetoothLeDevice.GetGattServicesAsync().get();    //No non-async alternative, not inherently threadsafe

        if (result.Status() == GattCommunicationStatus::Success)
        {
            printf("GetService Successfull\n");

            //store all device services
            auto services = result.Services();

            bool foundService = false;
            //loop each services in list searching for Cycling Power Service
            for (GattDeviceService serv : services)
            {
                if (serv.Uuid() == GattServiceUuids::CyclingPower())
                {

                    printf("GetCyclingPowerService Successfull\n");

                    //get all characteristics from current service
                    GattCharacteristicsResult resultCharacteristics = serv.GetCharacteristicsAsync().get();

                    //verify if getting characteristics was successfull
                    if (resultCharacteristics.Status() == GattCommunicationStatus::Success)
                    {

                        printf("GetCharacteristics Successfull\n");
                        //store device services to list
                        auto characteristics = resultCharacteristics.Characteristics();
                        
                        bool foundCharacteristic = false;                        

                        //loop through its characteristics searching for Cycling Power Measurement characteristi
                        for (GattCharacteristic chara : characteristics)
                        {
                            if (chara.Uuid() == GattCharacteristicUuids::CyclingPowerMeasurement())
                            {
                                printf("GetCyclingPowerMeasurementCharacteristics Successfull\n");                                
                                
                                GattCharacteristicProperties properties = chara.CharacteristicProperties();

                                if ((properties & GattCharacteristicProperties::Notify) == GattCharacteristicProperties::Notify)
                                {

                                    printf("Subscribing to notification\n");
                                    GattCommunicationStatus status = chara.WriteClientCharacteristicConfigurationDescriptorAsync(
                                        GattClientCharacteristicConfigurationDescriptorValue::Notify).get();
                                    
                                    printf("Checking if subscription was successfull\n");   //For identifying if the program is hanging on WritingCharDescriptor
                                    if (status == GattCommunicationStatus::Success)
                                    {
                                        // Server has been informed of clients interest.
                                        printf("LiteWatcher: Subscribed to notification\n\n");

                                        bluetoothLeDeviceDisplay = std::get<0>(FindBluetoothLEDevice(Id));
                                        bluetoothLeDeviceDisplay.NotifyOnCharacteristicChange(chara);                                        
                                        selectedCharacteristic = chara;
                                        bluetoothLeDevice = newBluetoothLeDevice;

                                        NotifyToken = selectedCharacteristic.ValueChanged({ this, &LiteWatcher::characteristicNotification }); //As per microsoft docs https://learn.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/handle-events

                                        return true;

                                        foundCharacteristic = true;
                                    }
                                    else
                                    {
                                        printf("LiteWatcher: Failed to subscribe to notification\n\n");
                                        foundCharacteristic = false;
                                    }
                                    
                                }
                                else {
                                    printf("LiteWatcher: CyclingPowerMeasurement cannot notify\n\n");                                    
                                }
                            }
                        }

                        if (!foundCharacteristic) {
                            printf("LiteWatcher: Could not find Cycling power measurement characteristic on device\n\n");
                        }
                        
                    }
                    else { 
                        printf("LiteWatcher: Failed to retrieve characteristics data\n");
                        printf("Status Message: %i\n\n", resultCharacteristics.Status());                        
                    }

                }
            }
            if (!foundService) {
                printf("Could not find Cycling Power Service on device\n\n");
            }
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
                    bluetoothLeDeviceDisplay.StopNotifyOnCharacteristicChange();
                    bluetoothLeDeviceDisplay = nullptr;
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
        if (selectedCharacteristic == nullptr && bluetoothLeDevice == nullptr) {
            return true;
        }
        else {
            printf("LiteWatcher: Unknown Error");
            return false;
        }
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
                if (FindUnknownDevices(deviceInfo.Id()) == UnknownDevices.end()) 
                {
                    if (!deviceInfo.Name().empty())
                    {
                        m_knownDevices.Append(make<BluetoothLEDeviceDisplay>(deviceInfo));
                        
                    }
                    else
                    {
                        // Add it to a list in case the name gets updated later. 
                        UnknownDevices.push_back(deviceInfo);
                    }
                }                
            }
        }
        co_return;
    }

    fire_and_forget LiteWatcher::DeviceWatcher_Updated(DeviceWatcher sender, DeviceInformationUpdate deviceInfoUpdate)
    {
        // Protect against race condition if the task runs after the app stopped the deviceWatcher.
        if (sender == deviceWatcher)
        {
            IDLTesting::BluetoothLEDeviceDisplay bleDeviceDisplay = std::get<0>(FindBluetoothLEDevice(deviceInfoUpdate.Id()));
            if (bleDeviceDisplay != nullptr)
            {
                // Device is already known, update info
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
        if (sender == deviceWatcher)
        {
            printf("%i devices found. Enumeration completed.\n", m_knownDevices.Size());
        }
        co_return;
    }

    fire_and_forget LiteWatcher::DeviceWatcher_Stopped(DeviceWatcher sender, IInspectable const&)
    {
        // Protect against race condition if the task runs after the app stopped the deviceWatcher.
        if (sender == deviceWatcher)
        {
            printf("No longer watching for devices.\n");
        }
        co_return;
    }

    fire_and_forget LiteWatcher::characteristicNotification(GattCharacteristic sender, GattValueChangedEventArgs args)
    {
        //args.CharacteristicValue
        auto dataReader = Windows::Storage::Streams::DataReader::FromBuffer(args.CharacteristicValue());
        dataReader.ByteOrder(Windows::Storage::Streams::ByteOrder::LittleEndian);

        //printf("Notification: ");       //Show that notification has been received
        unsigned int bufflen = dataReader.UnconsumedBufferLength(); //get the size of the notification in bytes
        //printf("byte count: %u ", bufflen);
        //if (bufflen >= 4) {
            updated = true;

            uint8_t flags[2] = { 11,11 };
            dataReader.ReadBytes(flags);
            //printf("Flags:  %x %x \t", flags[0], flags[1]);
            uint16_t newPower = dataReader.ReadUInt16();
            power = newPower;
            //printf("Instantaneous Power:  %u \t", power);

            //if ((flags[0]>>7) & 1) {
            //    printf("Pedal Power Balance in 0.5%s:  %u \t", dataReader.ReadByte()); // For double crank systems
            //}
            //if ((flags[0] >> 5) & 1) {
            //    printf("Accumulated Torque :  %u \t", dataReader.ReadUInt16());
            //}
            //if ((flags[0] >> 3) & 1) {
            //    printf("Cumulative Wheel Revolutions :  %u \t", dataReader.ReadUInt32());
            //    printf("Last Wheel Event Time :  %u \t", dataReader.ReadUInt16());                
            //}
            //if ((flags[0] >> 2) & 1) {
            //    printf("Cumulative Crank Revolutions :  %u \t", dataReader.ReadUInt16());
            //    printf("Last Crank Event Time :  %u \t", dataReader.ReadUInt16());
            //}
            //etc, there are a lot of these. None of them are usefull but this approach can be copied if expanding into FTMS

        //}
        //else {
        //    printf("The notification data is invalid");
        //}
        //printf("\n");

        co_return;// fire_and_forget();
    }

}
