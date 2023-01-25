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
	this->serverInstance.Get("/meta", [this](const Request& req, Response& res) { this->get_meta(req, res); });

	#if APPLET
	std::cout << "All routes were registered, debugger started." << std::endl;
	#endif

	#if NXLINK
	DEBUGMSG("[SERVER] All routes were registered, debugger started.\n");
	#endif
}

HTTPServer::~HTTPServer() {}

std::string error;
void HTTPServer::start() {
	this->gm = new GameManager();
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

	if (!req.has_param("region"))
		return res.set_content(makeResponse(true, "`region` param cannot be undefined."), "application/json");

	u64 offset = parseStringToInt(req.get_param_value("offset"));
	u64 size = parseStringToInt(req.get_param_value("size"));
	int region = stoi(req.get_param_value("region"));

	std::string refreshError;
	Result rc = this->debugger->refreshMetaData(refreshError);

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, refreshError), "application/json");

	json payload = nullptr;

	// switch(region) {
	// 	case 0: // HEAP
	// 		payload = peek(this->debugger->getMeta().heap_extents.base + offset, size);
	// 		break;
	// 	case 1: // MAIN
	// 		payload = peek(this->debugger->getMeta().main_nso_extents.base + offset, size);
	// 		break;
	// 	case 2: // ABSOLUTE
	// 		payload = peek(offset, size);
	// 		break;
	// }

	// return res.set_content(makeResponse(false, "OK", make_optional(payload)), "application/json");

	optional<DmntMemoryRegionExtents> extents;
	switch(region) {
		case 0: // HEAP
			extents = this->debugger->getMeta().heap_extents;
			break;
		case 1: // MAIN
			extents = this->debugger->getMeta().main_nso_extents;
			break;
		case 2: // ABSOLUTE
			break;
	}

	unsigned short b[size];
	std::string __err;
	bool test = this->gm->peek(b, size, extents.value().base + offset, __err);

	// unsigned short buffer[size];
	
	// std::string error;
	// rc = dmntchtInitialize();
	// if (R_FAILED(rc)) {
	// 	error = "Could not initialize dmntcht, error: " + iths(rc);
	// }

	// rc = dmntchtForceOpenCheatProcess();
	// if (R_FAILED(rc)) {
	// 	error = "Could not force open cheat process (dmnt:cht, forceOpenCheatProcess): " + iths(rc);
	// 	dmntchtExit();
	// }

	// rc = this->debugger->readMemory(buffer, size, (extents.has_value() ? extents.value().base : 0) + offset);

	// std::stringstream ss;
	// ss << std::setfill('0') << std::uppercase << std::hex;

	// for (u64 i = 0; i < size; i++)
	// 	ss << std::setw(2) << buffer[i];

	payload = {
		{"result", "ss.str()"},
		{"debug", "Reading region #" + std::to_string(region) + ", at offset: " + iths(offset) + ", size: " + iths(size) + ". Current extent base: " + std::to_string(extents.value().base) + ", debugger->readMemory RC: " + std::to_string(rc) + ", dmntError: " + __err},
		{"fancy_game_manager", test ? (*bufferToHexString(b, size)) : "nope, failed."}
	};

	res.set_content(makeResponse(false, "OK", make_optional(payload)), "application/json");
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