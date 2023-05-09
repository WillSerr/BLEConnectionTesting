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
    }

    void BluetoothLEDeviceDisplay::NotifyOnCharacteristicChange(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const& sender)
    {
        if (currentSubscribedCharacteristic == nullptr) {
            
            //TESTING: does pairing remove the responsiveness problems
            printf("Attempting pair...\n");
            Windows::Devices::Enumeration::DevicePairingResult result = m_deviceInformation.Pairing().PairAsync().get();
            Windows::Devices::Enumeration::DevicePairingResultStatus status = result.Status();
            if (status == Windows::Devices::Enumeration::DevicePairingResultStatus::Paired) {
                printf("Pairing successfull\n");
            }
            else {                
                printf("Pairing unsuccessfull.");
                if (status == Windows::Devices::Enumeration::DevicePairingResultStatus::AlreadyPaired) {
                    printf(" Already Paired.");
                }
                switch (status)
                {
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::NotReadyToPair:
                    printf(" NotReadyToPair.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::NotPaired:
                    printf(" NotPaired.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::ConnectionRejected:
                    printf(" ConnectionRejected.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::TooManyConnections:
                    printf(" TooManyConnections.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::HardwareFailure:
                    printf(" HardwareFailure.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::AuthenticationTimeout:
                    printf(" AuthenticationTimeout.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::AuthenticationNotAllowed:
                    printf(" AuthenticationNotAllowed.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::AuthenticationFailure:
                    printf(" AuthenticationFailure.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::NoSupportedProfiles:
                    printf(" NoSupportedProfiles.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::ProtectionLevelCouldNotBeMet:
                    printf(" ProtectionLevelCouldNotBeMet.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::AccessDenied:
                    printf(" AccessDenied.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::InvalidCeremonyData:
                    printf(" InvalidCeremonyData.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::PairingCanceled:
                    printf(" PairingCanceled.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::OperationAlreadyInProgress:
                    printf(" OperationAlreadyInProgress.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::RequiredHandlerNotRegistered:
                    printf(" RequiredHandlerNotRegistered.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::RejectedByHandler:
                    printf(" RejectedByHandler.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::RemoteDeviceHasAssociation:
                    printf(" RemoteDeviceHasAssociation.");
                    break;
                case winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::Failed:
                    printf(" Failed.");
                    break;
                default:
                    break;
                }
                printf("\n");
            }
            

            currentSubscribedCharacteristic = sender;
            //NotifyToken = sender.ValueChanged({ this, &BluetoothLEDeviceDisplay::characteristicNotification }); //As per microsoft docs https://learn.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/handle-events
        }
    }

    void BluetoothLEDeviceDisplay::StopNotifyOnCharacteristicChange()
    {
        if (currentSubscribedCharacteristic != nullptr) {

            //TESTING: does pairing remove the responsiveness problems
            printf("Attempting pair...\n");
            Windows::Devices::Enumeration::DeviceUnpairingResult result = m_deviceInformation.Pairing().UnpairAsync().get();
            Windows::Devices::Enumeration::DeviceUnpairingResultStatus status = result.Status();
            if (status == Windows::Devices::Enumeration::DeviceUnpairingResultStatus::Unpaired) {
                printf("Pairing successfull\n");
            }
            else {
                printf("Pairing unsuccessfull.");
                if (status == Windows::Devices::Enumeration::DeviceUnpairingResultStatus::AlreadyUnpaired) {
                    printf(" Already Unpaired.");
                }
                printf("\n");
            }


            //currentSubscribedCharacteristic.ValueChanged(NotifyToken);
            currentSubscribedCharacteristic = nullptr;
        }
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
            uint16_t newPower = dataReader.ReadUInt16();
            power = newPower;
            printf("Instantaneous Power:  %u \t", power);           

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

        }
        else {
            printf("The notification data is invalid");
        }
        printf("\n");

        co_return;// fire_and_forget();
    }
}
