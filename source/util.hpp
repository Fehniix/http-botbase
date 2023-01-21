#include <switch.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>

/**
 * @brief Whether the build's target is APPLET or SYS-MODULE.
*/
#define APPLET 0

/**
 * @brief Whether nxlink should be enabled or not.
*/
#define NXLINK 1

/**
 * @brief Sends a message with variadic parameters to stderr, likely to be caught by nxlink. Must be '\\n'-terminated, or `fflush(stderr)` right after calling this.
 */
#define VDEBUGMSG(msg, ...) 	(fprintf(stderr, msg, __VA_ARGS__))
#define DEBUGMSG(msg) 			(fprintf(stderr, msg))
#define RETURN_IF_FAIL(rc)		if (R_FAILED(rc)) return rc;

#define MAX_LINE_LENGTH 344 * 32 * 2

extern u64 mainLoopSleepTime;
extern bool debugResultCodes;

int setupServerSocket();
u64 parseStringToInt(const char* arg);
/**
 * Please bear in mind that this overload creates an `std::string` pointer, copies the - already copied arg! - given `str` object and calls the `const std::string*` overload! For long strings, the overhead could be massive.
 */
u64 parseStringToInt(std::string str);
u64 parseStringToInt(const std::string* str);
s64 parseStringToSignedLong(const char* arg);
u8* parseStringToByteBuffer(const char* arg, u64* size);

/**
 * @brief "Int To Hex String", converts an integer to a hexadecimal string.
 */
std::string iths(u64 number, bool showBase = false);

/**
 * @brief Converts the supplied number array to a hexadecimal string.
 */
template<typename T>
std::string* bufferToHexString(T* buffer, size_t bufferSize) {
	std::stringstream ss;
	ss << std::setfill('0') << std::uppercase << std::hex;

	for (u64 i = 0; i < bufferSize; i++)
		ss << std::setw(2) << buffer[i];

    std::string* str = new std::string(ss.str());
    return str;
};

HidNpadButton parseStringToButton(const char* arg);
Result capsscCaptureForDebug(void *buffer, size_t buffer_size, u64 *size); //big thanks to Behemoth from the Reswitched Discord!
void flashLed(void);