#include "EventManager.hpp"
#include "RemoteLogging.hpp"

PscPmModule EventManager::pscModule;
Waiter EventManager::pscModuleWaiter;
bool EventManager::internalThreadRunning = false;
std::vector<std::function<void()>> EventManager::eventListenerMap[6];

EventManager::EventManager() {}

EventManager::~EventManager() {
	this->internalThreadRunning = false;

	pscPmModuleFinalize(&this->pscModule);
	pscPmModuleClose(&this->pscModule);
	eventClose(&this->pscModule.event);

	svcCancelSynchronization(this->listenerThread.handle);
	threadWaitForExit(&this->listenerThread);
	threadClose(&this->listenerThread);
}

Result EventManager::initialize() {
	Result rc = pscmGetPmModule(&this->pscModule, PscPmModuleId(126), this->dependencies, sizeof(this->dependencies) / sizeof(uint32_t), true);
	
	if (R_FAILED(rc))
		return rc;

	for (u8 i = 0; i < 6; i++)
		this->eventListenerMap[i] = std::vector<std::function<void()>>();

	this->pscModuleWaiter = waiterForEvent(&this->pscModule.event);
	this->internalThreadRunning = true;
	threadCreate(&this->listenerThread, EventManager::listenerThreadFunction, nullptr, this->pscThreadStack, sizeof(this->pscThreadStack), 0x2C, -2);
	return threadStart(&this->listenerThread);
}

int EventManager::addEventListener(PscPmState state, std::function<void()> listener) {
	if (state < 0 || state > 5)
		return -1;

	if (this->eventListenerMap[state].size() >= MAX_LISTENERS)
		return -2;

	this->eventListenerMap[state].push_back(listener);

	return this->eventListenerMap[state].size() - 1;
}

bool EventManager::removeEventListener(PscPmState state, long unsigned int id) {
	if (state < 0 || state > 5)
		return false;

	if (id < 0 || id >= this->eventListenerMap[state].size())
		return false;

	this->eventListenerMap[state].erase(this->eventListenerMap[state].begin() + id);
	return true;
}

void EventManager::clearAllListeners() {
	for (u8 i = 0; i < 6; i++)
		this->eventListenerMap[i].clear();
}

void EventManager::listenerThreadFunction(void* args) {
	do {
		if (R_FAILED(waitSingle(EventManager::pscModuleWaiter, UINT64_MAX)))
			continue;
		
		PscPmState pscState;
		u32 out_flags;

		if (R_FAILED(pscPmModuleGetRequest(&EventManager::pscModule, &pscState, &out_flags)))
			continue;

		// Guard on invalid state
		if (pscState < 0 || pscState > 5)
			continue;

		for (std::vector<std::function<void()>>::iterator callback = EventManager::eventListenerMap[pscState].begin(); 
			callback != EventManager::eventListenerMap[pscState].end();
			callback++
		)
			if (*callback != nullptr)
				(*callback)();

		pscPmModuleAcknowledge(&EventManager::pscModule, pscState);
	} while (EventManager::internalThreadRunning);
}