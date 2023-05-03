
#pragma once

#include "WinsockHandling.h"


/**
 *
 */

class BLEInterface 
{

public:
	/** Implement this for initialization of instances of the system */
	BLEInterface();

	/** Implement this for deinitialization of instances of the system */
	~BLEInterface();

	bool AttemptConnectionToBikeHost();

	void getAvailableBikes();
	void getNetworkTestMessage();
	bool requestConnectionToBike(int index);

	int GetSmoothPower();

	int GetInstantPower();

	void update(float deltaTime);

	bool IsBikeConnected();
	bool isAwaitingServerResponse();
private:

	WinsockHandling winsockHandler;

#pragma region UINavigation

	float selectProgress;

	float delayBeforeSelectStart;
	float delayBeforeSelectTimer;

	float timeReqForSelect;
	float timeReqForSelectTimer;

	int requiredPowerToSelect;

	float timeSincePedalledOverThreshold;

public:
	void restartPedalSelectAction(float timeRequired = 4.f, float delayBeforeStart = 2.f, int requiredPower = 50);


	void cancelPedalSelectAction();


	float getPedalSelectProgress();


	float getTimeSincePedalledOverThreshold();


	void forceProgressPedalSelectAction(float deltaTime);
#pragma endregion
};
