#include <iostream>
#include <optional>
#include <iomanip>
#include <switch.h>
#include "dmntcht.h"
#include "GameManager.hpp"
#include "util.hpp"

GameManager::GameManager() {
	this->debugger = new Debugger();
}

GameManager::~GameManager() {
	delete this->debugger;
}

/**
 * @brief Initializes dmnt:cht and force-opens the currently active cheat process. 
 */
Result GameManager::dmntchtAttach(std::string &error) {
	Result rc = dmntchtInitialize();
	if (R_FAILED(rc)) {
		error = "Could not initialize dmntcht, error: " + iths(rc);
	}

	rc = dmntchtForceOpenCheatProcess();
	if (R_FAILED(rc)) {
		error = "Could not force open cheat process (dmnt:cht, forceOpenCheatProcess): " + iths(rc);
		dmntchtExit();
	}

	return rc;
}

/**
 * @brief Force-closes the currently opened cheat process and exits from dmnt:cht.
 */
Result GameManager::dmntchtDetach() {
	Result rc = dmntchtForceCloseCheatProcess();

	if (R_FAILED(rc))
		return rc;

	dmntchtExit();

	return (Result)0;
}

bool GameManager::peek(void *buffer, size_t bufferSize, u64 address, std::string &error) {
	std::string attachError;
	Result rc = this->dmntchtAttach(attachError);

	if (R_FAILED(rc)) {
		error = attachError;
		return false;
	}

	rc = this->debugger->readMemory(buffer, bufferSize, address);

	if (R_FAILED(rc)) {
		error = "There was an error while reading memory. RC: " + iths(rc);
		return false;
	}

	rc = this->dmntchtDetach();

	if (R_FAILED(rc)) {
		error = "There was an error while detaching debugger from active process. RC: " + iths(rc);
		return false;
	}

	return true;
}