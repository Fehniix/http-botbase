#include "RemoteLogging.hpp"

const std::string months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

RemoteLogging* RemoteLogging::_instance;
Mutex RemoteLogging::instantiationMutex = 0;

RemoteLogging::RemoteLogging() {
	_instance = nullptr;
	this->connected = false;
}

RemoteLogging* RemoteLogging::instance() {
	mutexLock(&instantiationMutex);
	if (_instance == nullptr)
		_instance = new RemoteLogging();
	mutexUnlock(&instantiationMutex);
	return _instance;
}

bool RemoteLogging::connect(const std::string& ipAddress) {
	if (this->connected)
		return true;

	this->client = TCPClient(REMOTE_PORT, ipAddress);

	int connection_status = this->client.make_connection();

	if (connection_status != 0)
		return false;

	this->connected = true;
	
	return true;
}

const bool RemoteLogging::info(const std::string& msg) {
	return this->sendLog(msg, "INFO");
}

const bool RemoteLogging::debug(const std::string& msg) {
	return this->sendLog(msg, "DEBUG");
}

const bool RemoteLogging::error(const std::string& msg) {
	return this->sendLog(msg, "ERROR");
}

const bool RemoteLogging::isConnected() const {
	return this->connected;
}

std::string RemoteLogging::now() const {
	return fmt::format("{:%Y-%m-%d %H:%M:%S}", tm());
}

const bool RemoteLogging::sendLog(const std::string& msg, const std::string& level) {
	std::string formatted = fmt::format("[%s] [%s] %s\n", level, this->now(), msg);
	return this->client.send_message(formatted) == 0;
}