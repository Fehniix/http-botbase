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
	this->serverInstance.Get("/poke", [this](const Request& req, Response& res) { this->get_poke(req, res); });
	this->serverInstance.Get("/mainNsoBase", [this](const Request& req, Response& res) { this->get_mainNsoBase(req, res); });
	this->serverInstance.Get("/heapBase", [this](const Request& req, Response& res) { this->get_heapBase(req, res); });
	this->serverInstance.Get("/meta", [this](const Request& req, Response& res) { this->get_meta(req, res); });

	#if APPLET
	std::cout << "All routes were registered, debugger started." << std::endl;
	#endif

	#if NXLINK
	DEBUGMSG("[SERVER] All routes were registered, debugger started.\n");
	#endif
}

HTTPServer::~HTTPServer() {}

void HTTPServer::start() {
	std::string error;

	this->gameManager = new GameManager();
	this->debugger = new Debugger();
	this->debugger->initialize(error);

	#if NXLINK
	DEBUGMSG("[SERVER] HTTPServer started!\n");
	#endif

	flashLed();

	HTTPServer::starting = false;
	this->serverInstance.listen("0.0.0.0", 9001);
}

void HTTPServer::stop() {
	this->serverInstance.stop();

	delete this->debugger;
}

void HTTPServer::startAsync() {
	std::string error;

	this->gameManager = new GameManager();
	this->debugger = new Debugger();
	this->debugger->initialize(error);

	this->starting = true;

	#if NXLINK
	DEBUGMSG("[SERVER] HTTPServer started!\n");
	#endif

	flashLed();

	threadCreate(&this->serverThread, HTTPServer::serverThreadFunction, (void*)&this->serverInstance, NULL, 0x10000, 0x2C, -2);
	threadStart(&this->serverThread);
}

void HTTPServer::stopAsync() {
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

bool HTTPServer::listening() {
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

	if (!req.has_param("region"))
		return res.set_content(makeResponse(true, "`region` param cannot be undefined."), "application/json");

	u64 offset = parseStringToInt(req.get_param_value("offset"));
	u64 size = parseStringToInt(req.get_param_value("size"));
	std::string region = req.get_param_value("region");
	for (auto &c: region) c = tolower(c); // Convert region to lower-case.

	std::string refreshError;
	Result rc = this->debugger->refreshMetaData(refreshError);

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, refreshError), "application/json");

	json payload = nullptr;

	DmntMemoryRegionExtents extents = DmntMemoryRegionExtents();
	extents.base = 0;
	extents.size = 0;

	if (region == "heap")
		extents = this->debugger->getMeta().heap_extents;
	if (region == "main")
		extents = this->debugger->getMeta().main_nso_extents;

	unsigned short buffer[size];
	std::string __err;
	bool test = this->gameManager->peek(buffer, size, extents.base + offset, __err);

	payload = {
		{"result", "ss.str()"},
		{"debug", "Reading region #" + region + ", at offset: " + iths(offset) + ", size: " + iths(size) + ". Current extent base: " + std::to_string(extents.base) + ", debugger->readMemory RC: " + std::to_string(rc) + ", dmntError: " + __err},
		{"fancy_game_manager", test ? (*bufferToHexString(buffer, size)) : "nope, failed."}
	};

	res.set_content(makeResponse(false, "OK", make_optional(payload)), "application/json");
}

void HTTPServer::get_poke(const Request &req, Response &res) {
	if (!req.has_param("offset"))
		return res.set_content(makeResponse(true, "`offset` param cannot be undefined."), "application/json");

	if (!req.has_param("region"))
		return res.set_content(makeResponse(true, "`region` param cannot be undefined."), "application/json");
		
	if (!req.has_param("data"))
		return res.set_content(makeResponse(true, "`data` param cannot be undefined."), "application/json");

	
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

	DmntMemoryRegionExtents heapExtents = this->debugger->getHeap();

	json payload = {
		{"base", iths(heapExtents.base)}, 
		{"size", iths(heapExtents.size)},
	};

	res.set_content(makeResponse(false, "OK", payload), "application/json");
}

void HTTPServer::get_meta(const Request &req, Response &res) {
	string refreshError;
	Result rc = this->debugger->refreshMetaData(refreshError);

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, refreshError), "application/json");

	DmntCheatProcessMetadata meta = this->debugger->getMeta();

	json payload = {
		{ "heap", {
			{"base", iths(meta.heap_extents.base)},
			{"size", iths(meta.heap_extents.size)}
		} },
		{ "main", {
			{"base", iths(meta.main_nso_extents.base)},
			{"size", iths(meta.main_nso_extents.size)}
		} },
		{ "address_space", {
			{"base", iths(meta.address_space_extents.base)},
			{"size", iths(meta.address_space_extents.size)}
		} },
		{ "alias", {
			{"base", iths(meta.alias_extents.base)},
			{"size", iths(meta.alias_extents.size)}
		} },
		{ "pid", meta.process_id },
		{ "tid", iths(meta.title_id) }
	};

	res.set_content(makeResponse(false, "OK", payload), "application/json");
}

void HTTPServer::get_dmntRunning(const Request &req, Response &res) {
	res.set_content(makeResponse(false, "OK", json({"dmntRunning", this->debugger->m_dmnt})), "application/json");
}