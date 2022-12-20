#include <switch.h>
#include <stdio.h>
#include <iostream>

/**
 * @brief Whether the build's target is APPLET or SYS-MODULE.
*/
#define APPLET 1

/**
 * @brief Whether nxlink should be enabled or not.
*/
#define NXLINK 1

/**
 * @brief Sends a message with variadic parameters to stderr, likely to be caught by nxlink. Must be '\\n'-terminated, or `fflush(stderr)` right after calling this.
 */
#define VDEBUGMSG(msg, ...) 	(fprintf(stderr, msg, __VA_ARGS__))
#define DEBUGMSG(msg) 			(fprintf(stderr, msg))

#define MAX_LINE_LENGTH 344 * 32 * 2

extern u64 mainLoopSleepTime;
extern bool debugResultCodes;

int setupServerSocket();
u64 parseStringToInt(const char* arg);
s64 parseStringToSignedLong(const char* arg);
u8* parseStringToByteBuffer(const char* arg, u64* size);
std::string intToHexString(u64 number);
HidNpadButton parseStringToButton(const char* arg);
Result capsscCaptureForDebug(void *buffer, size_t buffer_size, u64 *size); //big thanks to Behemoth from the Reswitched Discord!
void flashLed(void);