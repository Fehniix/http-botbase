#ifndef EVENTMANAGER_HPP
#define EVENTMANAGER_HPP

#include <iostream>
#include <switch.h>
#include "Atmosphere/libstratosphere/include/stratosphere.hpp"
#include "fmt/core.h"
#include "fmt/chrono.h"

class EventManager {
	public:
		// Disable cloning.
		inline EventManager(EventManager *other) = delete;
		// Disable assignment.
		inline void operator=(const EventManager&) = delete;

		static EventManager* instance();

	protected:
		EventManager();
		~EventManager();

	private:
		static Mutex instantiationMutex;
		static EventManager *_instance;

		PscPmModule pscModule;
		Waiter pscModuleWaiter;
		const uint32_t dependencies[1] = {PscPmModuleId_Fs};
		alignas(ams::os::ThreadStackAlignment) u8 psc_thread_stack[0x1000];
};

#endif