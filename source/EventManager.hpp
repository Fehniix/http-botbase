#ifndef EVENTMANAGER_HPP
#define EVENTMANAGER_HPP

#include <iostream>
#include <functional>
#include <switch.h>
#include <vector>
#include "fmt/core.h"
#include "fmt/chrono.h"

#define MAX_LISTENERS 16

class EventManager {
	public:
		EventManager();
		~EventManager();

		// Initializes the `EventManager`.
		Result initialize();

		/**
		 * @brief As for right now, this method calls the supplied callback whenever the NS is about to go to sleep.
		 */
		int addEventListener(PscPmState state, std::function<void()> callback);

		/**
		 * @brief Removes the listener with the given ID.
		 * 
		 * @return true If the listener was removed successfully.
		 * @return false If the listener was not found, or if the state is invalid.
		 */
		bool removeEventListener(PscPmState state, long unsigned int id);

		void clearAllListeners();

	protected:
		Thread listenerThread;

		static void listenerThreadFunction(void* args);

	private:
		static PscPmModule pscModule;
		static Waiter pscModuleWaiter;
		// Use the FS module for reliability-purposes.
		const uint32_t dependencies[1] = {PscPmModuleId_Fs};
		alignas(4096) u8 pscThreadStack[0x1000];
		static bool internalThreadRunning;

		static std::vector<std::function<void()>> eventListenerMap[];
};

#endif