#ifndef STON_HPP
#define STON_HPP

#include <iostream>
#include "Socket.hpp"
#include "EventManager.hpp"
#include "RemoteLogging.hpp"
#include "fmt/core.h"
#include "fmt/chrono.h"

class Ston {
	public:
		// Disable cloning.
		inline Ston(Ston *other) = delete;
		// Disable assignment.
		inline void operator=(const Ston&) = delete;

		static Ston* instance();

		EventManager* getEventManager() const;
		RemoteLogging* getRemoteLogging() const;

	protected:
		Ston();
		~Ston();

	private:
		static Mutex instantiationMutex;
		static Ston *_instance;

		static EventManager* eventManager;
		static RemoteLogging* remoteLogging;
};
#endif