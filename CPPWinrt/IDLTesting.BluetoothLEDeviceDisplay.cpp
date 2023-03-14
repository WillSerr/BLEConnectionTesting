#include "pch.h"
#include <bluetoothleapis.h>
#include "IDLTesting.BluetoothLEDeviceDisplay.h"
#include "IDLTesting.BluetoothLEDeviceDisplay.g.cpp"



namespace winrt::IDLTesting::implementation
{
    BluetoothLEDeviceDisplay::BluetoothLEDeviceDisplay(Windows::Devices::Enumeration::DeviceInformation const& deviceInfoIn)
        : m_deviceInformation(deviceInfoIn)
    {
        updated = false;
        power = 0;
    }

    BluetoothLEDeviceDisplay::~BluetoothLEDeviceDisplay()
    {
    }

    bool BluetoothLEDeviceDisplay::Updated()
    {
        return updated;
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

    void BluetoothLEDeviceDisplay::NotifyOnCharacteristicChange(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const& sender)
    {
        currentSubscribedCharacteristic = &sender;
        NotifyToken = sender.ValueChanged({ get_weak(), &BluetoothLEDeviceDisplay::characteristicNotification });
    }

    fire_and_forget BluetoothLEDeviceDisplay::characteristicNotification(GattCharacteristic sender, GattValueChangedEventArgs args)
    {
        
        //args.CharacteristicValue
        auto dataReader = Windows::Storage::Streams::DataReader::FromBuffer(args.CharacteristicValue());
        dataReader.ByteOrder(Windows::Storage::Streams::ByteOrder::LittleEndian);

        printf("Notification: ");       //Show that notification has been received
        unsigned int bufflen = dataReader.UnconsumedBufferLength(); //get the size of the notification in bytes
        printf("byte count: %u ", bufflen);
        if (bufflen >= 4) {
            updated = true;

            uint8_t flags[2] = { 11,11 };
            dataReader.ReadBytes(flags);
            printf("Flags:  %x %x \t", flags[0], flags[1]);
            power = dataReader.ReadUInt16();
            printf("Instantaneous Power:  %u \t", power);

           

            if ((flags[0]>>7) & 1) {
                printf("Pedal Power Balance in 0.5%s:  %u \t", dataReader.ReadByte()); // For double crank systems
            }
            if ((flags[0] >> 5) & 1) {
                printf("Accumulated Torque :  %u \t", dataReader.ReadUInt16());
            }
            if ((flags[0] >> 3) & 1) {
                printf("Cumulative Wheel Revolutions :  %u \t", dataReader.ReadUInt32());
                printf("Last Wheel Event Time :  %u \t", dataReader.ReadUInt16());                
            }
            if ((flags[0] >> 2) & 1) {
                printf("Cumulative Crank Revolutions :  %u \t", dataReader.ReadUInt16());
                printf("Last Crank Event Time :  %u \t", dataReader.ReadUInt16());
            }
            //etc, there are a lot of these and I'm pretty sure none of them are usefull

        }
        else {
            printf("The notification data is invalid");
        }
        printf("\n");

        //Dump the raw data into console
        /*printf("byte count: %u \tData - [", bufflen);
        while (dataReader.UnconsumedBufferLength() > 0) {
            dataReader.ByteOrder(Windows::Storage::Streams::ByteOrder::BigEndian);
            uint8_t flags = 0;
            flags = dataReader.ReadByte();
            printf("%x,", flags);
        }
        printf("]\n");*/

        return fire_and_forget();
    }

    //bool BluetoothLEDeviceDisplay::LookupBooleanProperty(param::hstring const& property)
    //{
    //    auto value = m_deviceInformation.Properties().TryLookup(property);
    //    return value && unbox_value<bool>(value);
    //}

}
