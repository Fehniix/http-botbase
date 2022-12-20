#include <switch.h>
#include <httplib.hpp>
#include "debugger.hpp"

using namespace httplib;

class HTTPServer {
	public:
		HTTPServer();
		~HTTPServer();

		void start();
		void stop();

		bool started;
	
	private:
		Server serverInstance;
		Debugger* debugger;
		
		/**
	 	* Represents the thread that contains the listening server. 
		*/
		Thread serverThread;

		static void serverThreadFunction(void* arg);

		void get_ping(const Request&, Response& res);
		void get_peek(const Request& req, Response& res);
		void get_mainNsoBase(const Request& req, Response& res);
};