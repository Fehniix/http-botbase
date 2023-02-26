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

bool GameManager::peek(void *buffer, size_t bufferSize, u64 address, std::string &error) {
	std::string attachError;
	Result rc = this->debugger->dmntchtAttach(attachError);

	if (R_FAILED(rc)) {
		error = attachError;
		return false;
	}

	if (!this->debugger->refreshed) {
		rc = this->debugger->refreshMetaData(error);

		if (R_FAILED(rc)) {
			error = "There was an error while refreshing metadata. RC: " + iths(rc);
			return false;
		}
	}

	rc = this->debugger->readMemory(buffer, bufferSize, address);

	if (R_FAILED(rc)) {
		error = "There was an error while reading memory. RC: " + iths(rc);
		return false;
	}

	rc = this->debugger->dmntchtDetach();

	if (R_FAILED(rc)) {
		error = "There was an error while detaching debugger from active process. RC: " + iths(rc);
		return false;
	}

	return true;
}

bool GameManager::poke(u64 address, u64 *data, size_t dataSize, std::string &error) {
	std::string attachError;
	Result rc = this->debugger->dmntchtAttach(attachError);

	if (R_FAILED(rc)) {
		error = attachError;
		return false;
	}

	if (!this->debugger->refreshed) {
		rc = this->debugger->refreshMetaData(error);

		if (R_FAILED(rc)) {
			error = "There was an error while refreshing metadata. RC: 0x" + iths(rc);
			return false;
		}
	}

	rc = this->debugger->writeMemory(data, dataSize, address);

	if (R_FAILED(rc)) {
		error = "There was an error while poking the supplied data buffer of size " + std::to_string(dataSize) + " to the address 0x" + iths(address) + ". RC: 0x" + iths(rc);
		return false;
	}

	rc = this->debugger->dmntchtDetach();

	if (R_FAILED(rc)) {
		error = "There was an error while detaching debugger from active process. RC: " + iths(rc);
		return false;
	}

	return true;
}