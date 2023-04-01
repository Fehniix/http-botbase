#include "EventManager.hpp"

EventManager* EventManager::_instance;
Mutex EventManager::instantiationMutex = 0;

EventManager::EventManager() {
	_instance = nullptr;
}

EventManager* EventManager::instance() {
	mutexLock(&instantiationMutex);
	if (_instance == nullptr)
		_instance = new EventManager();
	mutexUnlock(&instantiationMutex);
	return _instance;
}