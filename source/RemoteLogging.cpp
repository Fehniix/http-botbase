#include "RemoteLogging.hpp"
#include "EventManager.hpp"
#include "Ston.hpp"

const std::string months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

RemoteLogging::RemoteLogging() {
	this->connected = false;
	this->goingToSleep = false;
	threadCreate(&this->retryThread, RemoteLogging::retryThreadFunction, (void*)this, NULL, 0x10000, 0x2C, -2);
	threadStart(&this->retryThread);
}

RemoteLogging::~RemoteLogging() {
	this->stopAutoReconnect();
	this->disconnect();
}

bool RemoteLogging::connect(
	const std::string& ipAddress, 
	u_short port, 
	bool retry, 
	int maxAttempts, 
	int retryInterval
) {
	if (this->connected)
		return true;

	this->ipAddress 	= ipAddress;
	this->port 			= port;
	this->maxAttempts 	= maxAttempts;
	this->retryInterval = retryInterval;
	this->client 		= new TCPClient(port, ipAddress);

	int connection_status = this->client->make_connection();
	if (connection_status != 0) return false;

	this->connected = true;

	if (retry)
		this->setupAutoReconnect();
	
	return true;
}

bool RemoteLogging::disconnect() {
	if (!this->connected)
		return true;

	this->client->close_connection();
	delete this->client;

	this->connected = false;

	return true;
}

void RemoteLogging::setupAutoReconnect() {
	this->retryThreadActive = true;

	EventManager* em = Ston::instance()->getEventManager();
	this->eventListeners[0] = em->addEventListener(PscPmState_ReadySleep, std::bind(&RemoteLogging::consoleGoingToSleepHandler, this));
    this->eventListeners[1] = em->addEventListener(PscPmState_Awake, std::bind(&RemoteLogging::consoleWokeUpHandler, this));
}

void RemoteLogging::stopAutoReconnect() {
	this->retryThreadActive = false;

	threadWaitForExit(&this->retryThread);
	threadClose(&this->retryThread);

	EventManager* em = Ston::instance()->getEventManager();
	em->removeEventListener(PscPmState_ReadySleep, this->eventListeners[0]);
	em->removeEventListener(PscPmState_Awake, this->eventListeners[1]);
}

void RemoteLogging::retryThreadFunction(void *args) {
	if (args == nullptr) return;

	RemoteLogging *instance 	= (RemoteLogging*)args;
	int rtlAttempts 			= 0;
	int maxAttempts 			= instance->maxAttempts;
	std::string *remoteLogIP 	= &instance->ipAddress;

	while (true) {
		if (instance->isConnected() || rtlAttempts >= maxAttempts || !instance->retryThreadActive) {
			svcSleepThread(4e+9);
			continue;
		}

		if (instance->connect(*remoteLogIP)) {
			instance->info("Connection established within retry thread.");
			rtlAttempts = 0;
			#if APPLET
			cout << "Connected to remote logging server." << endl;
			#endif
		} else {
			#if APPLET
			std::cout << ft("Attempt #{} to connect to remote logging server failed.", rtlAttempts) << std::endl;

			if (++rtlAttempts == maxAttempts)
				std::cout << ft("Could not connect to remote logging server after {} attempts.", rtlAttempts) << std::endl;
			#else
			if (++rtlAttempts == maxAttempts)
				fatalThrow(0x9999);
			#endif
		}

		svcSleepThread(2e+9);
	}
}

void RemoteLogging::consoleWokeUpHandler() {
	if (!this->retryThreadActive) return;

	threadResume(&this->retryThread);
}

void RemoteLogging::consoleGoingToSleepHandler() {
	this->disconnect();
	threadPause(&this->retryThread);
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
	time_t unixTime = time(NULL);
	struct tm* timeStruct = gmtime((const time_t *)&unixTime);

	int hours = timeStruct->tm_hour;
	int minutes = timeStruct->tm_min;
	int seconds = timeStruct->tm_sec;
	int day = timeStruct->tm_mday;
	int month = timeStruct->tm_mon;
	int year = timeStruct->tm_year +1900;

	return fmt::format("{}/{}/{} {}:{}:{}", year, months[month], day, hours, minutes, seconds);
}

const bool RemoteLogging::sendLog(const std::string& msg, const std::string& level) {
	if (!this->isConnected())
		return false;

	std::string formatted = fmt::format("[{}] [{}] {}\n", level, this->now(), msg);
	return this->client->send_message(formatted, false) == 0;
}