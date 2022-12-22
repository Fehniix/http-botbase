#include <switch.h>
#include <stdio.h>
#include <httplib.hpp>
#include <iostream>
#include <optional>
#include <sstream>
#include <iomanip>

#include "HTTPServer.hpp"
#include "json.hpp"
#include "commands.hpp"
#include "util.hpp"
#include "debugger.hpp"
#include "dmntcht.h"

using namespace httplib;
using namespace std;
using json = nlohmann::json;

string makeResponse(bool isError, const string& message, const optional<json> payload = nullopt) {
	json response = {
		{"status", isError ? "error" : "okay"},
		{"message", message}
	};

	if (payload.has_value())
		response["payload"] = payload.value();
	else
		response["payload"] = nullptr;

	return response.dump();
}

bool HTTPServer::starting;

HTTPServer::HTTPServer() {
	this->starting = false;

	this->serverInstance.Get("/ping", [this](const Request& req, Response& res) { this->get_ping(req, res); });
	this->serverInstance.Get("/peek", [this](const Request& req, Response& res) { this->get_peek(req, res); });
	this->serverInstance.Get("/mainNsoBase", [this](const Request& req, Response& res) { this->get_mainNsoBase(req, res); });
	this->serverInstance.Get("/heapBase", [this](const Request& req, Response& res) { this->get_heapBase(req, res); });

	#if APPLET
	std::cout << "All routes were registered, debugger started." << std::endl;
	#endif

	#if NXLINK
	DEBUGMSG("[SERVER] All routes were registered, debugger started.\n");
	#endif
}

HTTPServer::~HTTPServer() {}

void HTTPServer::start() {
	this->debugger = new Debugger();
	this->starting = true;

	#if NXLINK
	DEBUGMSG("[SERVER] HTTPServer started!\n");
	#endif

	flashLed();

	threadCreate(&this->serverThread, HTTPServer::serverThreadFunction, (void*)&this->serverInstance, NULL, 0x10000, 0x2C, -2);
	threadStart(&this->serverThread);
}

void HTTPServer::stop() {
	this->serverInstance.stop();

	delete this->debugger;

	threadWaitForExit(&this->serverThread);
	threadClose(&this->serverThread);
}

void HTTPServer::serverThreadFunction(void* arg) {
	HTTPServer::starting = false;

	Server* serverRef = (Server*)arg;
	serverRef->listen("0.0.0.0", 9001);
}

bool HTTPServer::started() {
	return this->serverInstance.is_running();
}

bool HTTPServer::isStarting() {
	return this->starting;
}

void HTTPServer::get_ping(const Request&, Response& res) {
	res.set_content("Hello World!", "text/plain");
}

void HTTPServer::get_peek(const Request& req, Response &res) {
	if (!req.has_param("offset"))
		return res.set_content(makeResponse(true, "`offset` param cannot be undefined."), "application/json");

	if (!req.has_param("size"))
		return res.set_content(makeResponse(true, "`size` param cannot be undefined."), "application/json");

	u64 offset = parseStringToInt(req.get_param_value("offset"));
	u64 size = parseStringToInt(req.get_param_value("size"));

	std::string refreshError;
	Result rc = this->debugger->refreshMetaData(refreshError);

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, refreshError), "application/json");

	DmntMemoryRegionExtents heap = this->debugger->getHeap();
	if (heap.base + offset > heap.base + heap.size)
		return res.set_content(
			makeResponse(
				true,
				"The supplied address (" + iths(offset) + ") is outside the HEAP range! Range: [" + iths(heap.base, true) + ", " + iths(heap.base + heap.size, true) + "]"
			), 
			"application/json"
		);

	unsigned short buffer[size];
	rc = this->debugger->readMemory(buffer, size, this->debugger->getHeap().base + offset);

	std::stringstream ss;
	ss << std::setfill('0') << std::uppercase << std::hex;

	for (u64 i = 0; i < size; i++)
		ss << std::setw(2) << buffer[i];

	res.set_content(makeResponse(false, "OK", make_optional(ss.str())), "application/json");
}

void HTTPServer::get_mainNsoBase(const Request &req, Response &res) {
	string refreshError;
	Result rc = this->debugger->refreshMetaData(refreshError);

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, refreshError), "application/json");

	DmntMemoryRegionExtents mainExtents = this->debugger->getMain();
	json payload = {{"base", iths(mainExtents.base)}, {"size", iths(mainExtents.size)}};
	res.set_content(makeResponse(false, "OK", payload), "application/json");
}

void HTTPServer::get_heapBase(const Request &req, Response &res) {
	string refreshError;
	Result rc = this->debugger->refreshMetaData(refreshError);

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, refreshError), "application/json");

	DmntMemoryRegionExtents mainExtents = this->debugger->getHeap();
	json payload = {{"base", iths(mainExtents.base)}, {"size", iths(mainExtents.size)}};
	res.set_content(makeResponse(false, "OK", payload), "application/json");
}