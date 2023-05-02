#include "ConnectedBike.h"

ConnectedBike::ConnectedBike()
{
}

ConnectedBike::~ConnectedBike()
{
}

bool ConnectedBike::isConnected()
{
	return connected;
}

int ConnectedBike::getSmoothPower()
{
	int total = 0;
	for (int reading : powerReadings) {
		total += reading;
	}
	return (total / READINGSTRIDE);
}

int ConnectedBike::getUnsmoothedPower()
{
	int lastReadingIndex = (nextReadingIndex - 1) % READINGSTRIDE;
	return powerReadings[lastReadingIndex];
}

void ConnectedBike::AddInstantPowerReading(int PowerReading)
{
	//Add power to the list of past READINGSTRIDE readings
	powerReadings[nextReadingIndex] = PowerReading;
	nextReadingIndex = (nextReadingIndex + 1) % READINGSTRIDE;

}

void ConnectedBike::setAsConnected()
{
	connected = true;
}

void ConnectedBike::setAsDisconnected()
{
	connected = false;
}

void ConnectedBike::update(float dt)
{
}
