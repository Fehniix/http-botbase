#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include <switch.h>
#include <httplib.hpp>
#include "debugger.hpp"
#include "GameManager.hpp"

using namespace httplib;

class HTTPServer {
	public:
		HTTPServer();
		~HTTPServer();

		void start();
		void stop();
		void startAsync();
		void stopAsync();
		bool listening();
		bool isStarting();
	
	private:
		Server serverInstance;
		Debugger* debugger;
		GameManager* gameManager;
		
		/**
	 	* Represents the thread that contains the listening server. 
		*/
		Thread serverThread;

		static bool starting;
		static void serverThreadFunction(void* arg);

		void get_ping(const Request&, Response &res);
		void get_peek(const Request &req, Response &res);
		void get_poke(const Request &req, Response &res);
		void get_mainNsoBase(const Request &req, Response &res);
		void get_heapBase(const Request &req, Response &res);
		void get_meta(const Request &req, Response &res);

		void get_dmntRunning(const Request &req, Response &res);
};
#endif