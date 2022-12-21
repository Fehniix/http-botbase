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

string makeResponse(bool isError, const string& message, const optional<string> payload = nullopt) {
	json response = {
		{"status", isError ? "error" : "okay"},
		{"message", message},
		{"payload", payload.value_or(nullptr)}
	};

	return response.dump();
}

HTTPServer::HTTPServer() {
	// this->debugger = new Debugger();
	this->started = false;

	this->serverInstance.Get("/ping", [this](const Request& req, Response& res) { this->get_ping(req, res); });
	this->serverInstance.Get("/peek", [this](const Request& req, Response& res) { this->get_peek(req, res); });
	this->serverInstance.Get("/mainNsoBase", [this](const Request& req, Response& res) { this->get_mainNsoBase(req, res); });

	#if APPLET
	std::cout << "All routes were registered, debugger started." << std::endl;
	#endif

	#if NXLINK
	DEBUGMSG("[SERVER] All routes were registered, debugger started.\n");
	#endif
}

HTTPServer::~HTTPServer() {}

void HTTPServer::start() {
	this->started = true;

	#if NXLINK
	DEBUGMSG("[SERVER] HTTPServer started!\n");
	#endif

	flashLed();

	threadCreate(&this->serverThread, HTTPServer::serverThreadFunction, (void*)&this->serverInstance, NULL, 0x10000, 0x2C, -2);
	threadStart(&this->serverThread);
}

void HTTPServer::stop() {
	this->serverInstance.stop();

	this->debugger->detach();
	delete this->debugger;

	threadWaitForExit(&this->serverThread);
	threadClose(&this->serverThread);
}

void HTTPServer::serverThreadFunction(void* arg) {
	Server* serverRef = (Server*)arg;
	serverRef->listen("0.0.0.0", 9001);
}

void HTTPServer::get_ping(const Request&, Response& res) {
	res.set_content("Hello World!", "text/plain");
}

void HTTPServer::get_peek(const Request& req, Response &res) {
	// Result ldrDmntResult;
	// MetaData meta = getMetaData(ldrDmntResult);

	// if (ldrDmntResult != 0)
	// 	return res.set_content(makeResponse(true, "`ldrDmntInitialize` failed with error code: " + to_string(ldrDmntResult), make_optional(to_string(meta.main_nso_base))), "application/json");

	if (!req.has_param("offset"))
		return res.set_content(makeResponse(true, "`offset` param cannot be undefined."), "application/json");

	if (!req.has_param("size"))
		return res.set_content(makeResponse(true, "`size` param cannot be undefined."), "application/json");

	u64 offset = parseStringToInt(req.get_param_value("offset").c_str());
	u64 size = parseStringToInt(req.get_param_value("size").c_str());

	DmntCheatProcessMetadata _meta;
	Result rc = dmntchtGetCheatProcessMetadata(&_meta);

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, "Could not fetch process metadata, error: " + iths(rc)), "application/json");

	u8 buffer[size];
	rc = this->debugger->readMemory(buffer, size, offset);

	std::stringstream ss;
	ss << std::setfill('0') << std::uppercase << std::hex;

	for (u64 i = 0; i < size; i++)
		ss << std::setw(sizeof(u8)*2) << int(buffer[i]);

	// string peekResult = peek(meta.heap_base + offset, size);
	res.set_content(
		makeResponse(
			false, 
			"OK", 
			make_optional(ss.str() + " " + std::to_string(_meta.main_nso_extents.base))
		), 
		"application/json"
	);
}

void HTTPServer::get_mainNsoBase(const Request& req, Response& res) {
	Result rc = dmntchtInitialize();

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, "Could not initialize dmntcht, error: " + iths(rc)), "application/json");

	rc = dmntchtForceOpenCheatProcess();

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, "Could not force open cheat process (dmnt:cht, forceOpenCheatProcess): " + iths(rc)), "application/json");

	DmntCheatProcessMetadata _meta;
	rc = dmntchtGetCheatProcessMetadata(&_meta);
	dmntchtExit();

	if (R_FAILED(rc))
		return res.set_content(makeResponse(true, "Could not fetch process metadata, error: " + iths(rc)) + ", meta: " + iths(_meta.main_nso_extents.base), "application/json");
	
	json payload = {{"base", iths(_meta.main_nso_extents.base)}, {"size", _meta.main_nso_extents.size}};
	res.set_content(makeResponse(false, "OK", payload.dump()), "application/json");
}