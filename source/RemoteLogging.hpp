#ifndef REMOTELOGGING_HPP
#define REMOTELOGGING_HPP

#include <iostream>
#include "Socket.hpp"
#include "fmt/core.h"
#include "fmt/chrono.h"

#define REMOTE_PORT 9002

class RemoteLogging {
	public:
		// Disable cloning.
		inline RemoteLogging(RemoteLogging *other) = delete;
		// Disable assignment.
		inline void operator=(const RemoteLogging&) = delete;

		static RemoteLogging* instance();

		/**
		 * @brief Connects to the remote logging server given its IP address. Default port is 9002.
		 *  
		 * @return `true` if connection was successful or previously established, `false` otherwise. 
		 */
		bool connect(const std::string& ipAddress);

		const bool info(const std::string& msg);
		const bool debug(const std::string& msg);
		const bool error(const std::string& msg);
		const bool isConnected() const;

	protected:
		TCPClient client;
		bool connected;

		RemoteLogging();
		~RemoteLogging();

		std::string now() const;
		const bool sendLog(const std::string& msg, const std::string& level);
	private:
		static Mutex instantiationMutex;
		static RemoteLogging *_instance;
};

#endif