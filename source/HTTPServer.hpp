#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include <switch.h>
#include <httplib.hpp>
#include "debugger.hpp"
#include "GameManager.hpp"
#include "EventManager.hpp"

using namespace httplib;

class HTTPServer {
	public:
		HTTPServer();
		~HTTPServer();

		void startSync();
		void stopSync();
		void startAsync(bool retry = false, int maxAttempts = 8);
		void stopAsync();
		bool listening();
	
	private:
		Server* serverInstance;
		Debugger* debugger;
		GameManager* gameManager;

		int maxAttempts = 8;
		bool retryThreadActive = false;
		bool isServerListening = false;

		int eventListeners[2] {-1, -1};
		
		/**
	 	* Represents the thread that contains the listening server. 
		*/
		Thread serverThread;

		static void connectRetryThreadFunction(void* arg);

		void consoleWokeUpHandler();
		void consoleGoingToSleepHandler();

		void initialize();
		void registerRoutes();

		void get_ping(const Request&, Response &res);
		void get_peek(const Request &req, Response &res);
		void get_poke(const Request &req, Response &res);
		void get_mainNsoBase(const Request &req, Response &res);
		void get_heapBase(const Request &req, Response &res);
		void get_meta(const Request &req, Response &res);

		void get_dmntRunning(const Request &req, Response &res);
};
#endif