#pragma once

#define READINGSTRIDE 5

class ConnectedBike
{
public:
	ConnectedBike();
	~ConnectedBike();

	bool isConnected();

	/// <summary> </summary>
	/// <returns>the power value smoothed over the last few readings</returns>
	int getSmoothPower();

	/// <summary> </summary>
	/// <returns>The latest power reading recieved</returns>
	int getUnsmoothedPower();

	void AddInstantPowerReading(int PowerReading);
	void setAsConnected();
	void setAsDisconnected();
	void update(float dt);
private:
	bool connected = false;

	int powerReadings[READINGSTRIDE] = { 0 };
	int nextReadingIndex = 0;

	int currentPowerValue = 0;
	int latestPowerValue = 0;
};