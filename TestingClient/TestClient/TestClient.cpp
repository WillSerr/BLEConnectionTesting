// TestClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <time.h>
#include <chrono>

#include "BLEInterface.h"

int main()
{
    bool running = true;
    BLEInterface server;

    std::string input;

    printf("Welcome to the test client for Pedal To The Metal!\n\n");
    printf("Enter text commands:\n");
    printf("'connect' to attempt connecting to the WinrtServer.\n");
    printf("'test' to request a network test message.\n");
    printf("'stop' to exit.\n");
    printf("'bikeconnect' to attempt to connect to a bike.\n");

    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();

    float timer = 0;
    while (running) {
        end = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed_seconds = end - start;
        start = std::chrono::steady_clock::now();
        if (!server.IsBikeConnected()) {
            input.clear();
            printf("input->");
            std::cin >> input;
            printf("\n");
            if (input == "stop") {
                running = false;
            }
            if (input == "connect") {
                server.AttemptConnectionToBikeHost();
            }
            if (input == "test") {
                printf("Must connect to server first.\n");
            }
            if (input == "bikeconnect") {
                printf("Must connect to server first.\n");
            }
        }
        else {
            if (timer <= 0) {
                input.clear();
                printf("input->");
                std::cin >> input;
                printf("\n");
                server.update(elapsed_seconds.count());
                if (server.IsBikeConnected()) {
                    if (input == "stop") {
                        running = false;
                    }
                    if (input == "connect") {
                        printf("A connection is already open.\n");
                    }
                    if (input == "test") {
                        timer = 5;
                        start = std::chrono::steady_clock::now();
                        server.getNetworkTestMessage();
                    }
                    if (input == "bikeconnect") {
                        printf("\nEnter the index of the bike you wish to connect to.\n");
                        int indexInput;
                        printf("input->");
                        std::cin >> indexInput;
                        printf("\n");
                        server.update(elapsed_seconds.count());

                        printf("Requesting...\n");
                        if (server.requestConnectionToBike(indexInput)) {
                            timer = 10;
                            start = std::chrono::steady_clock::now();
                        }
                    }
                }
            }
            else{
                server.update(elapsed_seconds.count());
                timer -= elapsed_seconds.count();
            }
        }
        
    }
}
