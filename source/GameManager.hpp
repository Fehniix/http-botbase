#pragma once

#include <iostream>
#include <iomanip>
#include <optional>
#include "debugger.hpp"

class GameManager {
	public:
		GameManager();
		~GameManager();

		bool peek(void *buffer, size_t bufferSize, u64 address, std::string &error);

	private:
		Debugger *debugger;

		Result dmntchtAttach(std::string &error);
		Result dmntchtDetach();
};