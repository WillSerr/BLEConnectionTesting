#include "pch.h"

#include "IDLTesting.LiteWatcher.h"
#include "WinsockHelper.h"
#include <iostream>

using namespace winrt;

int main()
{
    init_apartment();

    printf("Welcome to the intermediary program for Pedal To The Metal!\n\n");

    auto watcher = make<IDLTesting::implementation::LiteWatcher>();
    
    auto devList = watcher.KnownDevices();

    printf("This program only scans for new devices for 30 seconds\n\n\n");

    WinsockHelper winsockHelper;
    winsockHelper.devList = &devList;
    winsockHelper.deviceWatcher = &watcher;

    watcher.EnumerateButton_Click(); //Start device enumeration

    while (true) { 
        
        if (winsockHelper.getClientCount() > 0) { //IF the game is connected
            winsockHelper.HandleIncomingEvents();

            if (winsockHelper.getClientCount() == 0) {  //The game client has closed
                break;
            }
            
            if (winsockHelper.bikeIDToConnect != "NULL") {
                if (!watcher.SubscribeToPowerData(to_hstring(winsockHelper.bikeIDToConnect.c_str()))) {
                    winsockHelper.sendErrorMessage(WinsockHelper::FailedToConnectToDevice);
                }
                else {
                    winsockHelper.sendErrorMessage(WinsockHelper::NoError);
                }
                winsockHelper.bikeIDToConnect = "NULL";
            }
            
            if (watcher.BikeConnected()) {
                if (watcher.BikeUpdated())
                {
                    winsockHelper.sendPowerMessage(watcher.BikePower());
                }
            }           
        }
        else {
            winsockHelper.PollForConnection(); //Try connecting to the Game client
        }
    }
}
