#include "Ston.hpp"

Ston* Ston::_instance;
Mutex Ston::instantiationMutex = 0;

EventManager* Ston::eventManager;
RemoteLogging* Ston::remoteLogging;

Ston::Ston() {
	_instance = nullptr;
}

Ston::~Ston() {

}

Ston* Ston::instance() {
	mutexLock(&instantiationMutex);
	if (_instance == nullptr)
		_instance = new Ston();

	if (eventManager == nullptr)
		eventManager = new EventManager();

	if (remoteLogging == nullptr)
		remoteLogging = new RemoteLogging();
	mutexUnlock(&instantiationMutex);
	return _instance;
}

EventManager* Ston::getEventManager() const {
	return eventManager;
}

RemoteLogging* Ston::getRemoteLogging() const {
	return remoteLogging;
}