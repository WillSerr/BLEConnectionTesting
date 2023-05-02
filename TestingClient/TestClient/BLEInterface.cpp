
#include "BLEInterface.h"

BLEInterface::BLEInterface()
{
}

BLEInterface::~BLEInterface()
{
}

int BLEInterface::GetSmoothPower()
{
	return winsockHandler.connectedBike.getSmoothPower();
}

int BLEInterface::GetInstantPower()
{
	return winsockHandler.connectedBike.getUnsmoothedPower();
}

void BLEInterface::update(float deltaTime)
{
	if (winsockHandler.connectedBike.isConnected()) {
		winsockHandler.update();
		if (selectProgress < 1.f) {	//Select hasn't been performed
			if (delayBeforeSelectTimer < delayBeforeSelectStart) { //Grace period before selection starts
				delayBeforeSelectTimer += deltaTime;
			}
			else if (winsockHandler.connectedBike.getUnsmoothedPower() >= requiredPowerToSelect) {	//User is pedalling to select
				timeReqForSelectTimer += deltaTime;
				timeSincePedalledOverThreshold = 0;

				selectProgress = timeReqForSelectTimer / timeReqForSelect;
				if (selectProgress > 1.f) {
					selectProgress = 1.f;
				}
			}
			else {	//User is not peddaling to select when prompted
				timeSincePedalledOverThreshold += deltaTime;
				timeReqForSelectTimer = 0;

				selectProgress = 0;
			}
		}
	}
}

bool BLEInterface::IsBikeConnected()
{
	return winsockHandler.connectedBike.isConnected();
}

void BLEInterface::restartPedalSelectAction(float timeRequired, float delayBeforeStart, int requiredPower)
{
	selectProgress = 0.f;
	timeSincePedalledOverThreshold = 0.f;

	timeReqForSelect = timeRequired;
	timeReqForSelectTimer = 0.f;

	delayBeforeSelectStart = delayBeforeStart;
	delayBeforeSelectTimer = 0.f;

	requiredPowerToSelect = requiredPower;
}

void BLEInterface::cancelPedalSelectAction()
{
	selectProgress = 1.f;
	timeSincePedalledOverThreshold = 0.f;
	timeReqForSelectTimer = timeReqForSelect;
	delayBeforeSelectTimer = delayBeforeSelectStart;
}

float BLEInterface::getPedalSelectProgress()
{
	return selectProgress;
}

float BLEInterface::getTimeSincePedalledOverThreshold()
{
	return timeSincePedalledOverThreshold;
}

void BLEInterface::forceProgressPedalSelectAction(float deltaTime)
{
	if (selectProgress < 1.f) {	//Select hasn't been performed
		if (delayBeforeSelectTimer < delayBeforeSelectStart) { //Grace period before selection starts
			delayBeforeSelectTimer += deltaTime;
		}
		else {	//User is pedalling to select
			timeReqForSelectTimer += deltaTime;
			timeSincePedalledOverThreshold = 0;

			selectProgress = timeReqForSelectTimer / timeReqForSelect;
			if (selectProgress > 1.f) {
				selectProgress = 1.f;
			}
		}
	}
}

bool BLEInterface::AttemptConnectionToBikeHost() {
	winsockHandler.StartClient();
	return winsockHandler.connectedBike.isConnected();
}

void BLEInterface::getAvailableBikes()
{
	winsockHandler.requestAvailableBikeMsg();
}

void BLEInterface::getNetworkTestMessage()
{
	winsockHandler.requestNetworkTestMsg();
}

bool BLEInterface::requestConnectionToBike(int index)
{
	return winsockHandler.requestConnectionToBike(index);
}
