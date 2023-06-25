#ifndef REMOTELOGGING_HPP
#define REMOTELOGGING_HPP

#include <iostream>
#include "Socket.hpp"
#include "fmt/core.h"
#include "fmt/chrono.h"

#define REMOTE_PORT 9002
#define MAX_ATTEMPTS 8
#define RETRY_INTERVAL_MS 1000

class RemoteLogging {
	public:
		RemoteLogging();
		~RemoteLogging();

		/**
		 * @brief Connects to the remote logging server given its IP address. Default port is 9002.
		 *  
		 * @return `true` if connection was successful or previously established, `false` otherwise. 
		 */
		bool connect(const std::string& ipAddress, u_short port = REMOTE_PORT, bool retry = false, int maxAttempts = MAX_ATTEMPTS, int retryInterval = RETRY_INTERVAL_MS);

		/**
		 * @brief Disconnects from the remote logging server.
		 * 
		 * @return `true` if disconnection was successful, `false` otherwise.
		 */
		bool disconnect();

		/**
		 * @brief Stops the internal auto-reconnect thread.
		 */
		void stopAutoReconnect();

		const bool info(const std::string& msg);
		const bool debug(const std::string& msg);
		const bool error(const std::string& msg);
		const bool isConnected() const;

	protected:
		TCPClient *client;
		bool connected;
		bool goingToSleep;
		bool retryThreadActive = false;

		std::string ipAddress;
		u_short port;
		int maxAttempts;
		int retryInterval;

		std::string now() const;
		const bool sendLog(const std::string& msg, const std::string& level);
		static void retryThreadFunction(void *args);

		void consoleWokeUpHandler();
		void consoleGoingToSleepHandler();

		void setupAutoReconnect();
	private:
		int eventListeners[2] {-1, -1};

		Thread retryThread;
};

#endif