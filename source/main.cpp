#include <iostream>
#include <switch.h>
#include "HTTPServer.hpp"
#include "util.hpp"
#include "RemoteLogging.hpp"
#include "EventManager.hpp"
#include "fmt/core.h"
#include "Ston.hpp"

#define TITLE_ID 0x410000000000FF15

using namespace std;

string erroredModuleName;
Result initializationErrorCode = 0;
bool debugResultCodes = false;

/**
 * @brief Does a minimal initialization of the socket module to spare on limited memory resources.
 * 
 * @return `socketInitialize` result code.
 */
int socketMinimalInit();

#if !APPLET
#define INNER_HEAP_SIZE 0x100000

#ifdef __cplusplus
extern "C" {
#endif

void serviceInit(void);
void serviceExit(void);

// Sysmodules should not use applet*.
u32 __nx_applet_type = AppletType_None;

// Newlib heap configuration function (makes malloc/free work).
void __libnx_initheap(void) {
    static u8 inner_heap[INNER_HEAP_SIZE];
    extern void* fake_heap_start;
    extern void* fake_heap_end;

    // Configure the newlib heap.
    fake_heap_start = inner_heap;
    fake_heap_end   = inner_heap + sizeof(inner_heap);
}

// Service initialization.
void __appInit(void) {
    Result rc;

    // Open a service manager session.
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    // Retrieve the current version of Horizon OS.
    if (hosversionGet() == 0) {
        rc = setsysInitialize();
        if (R_SUCCEEDED(rc)) {
            SetSysFirmwareVersion fw;
            rc = setsysGetFirmwareVersion(&fw);
            if (R_SUCCEEDED(rc))
                hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
            setsysExit();
        }
    }

    serviceInit();
}

// Service deinitialization.
void __appExit(void) {
    serviceExit();
}

#ifdef __cplusplus
}
#endif
#endif

void serviceInit(void) {
    #if !APPLET
    svcSleepThread(20e+9);
    #endif

    Result rc;

    rc = pmdmntInitialize();
    if (R_FAILED(rc)) {
        erroredModuleName = "pmdmntInitialize";
        initializationErrorCode = rc;
        fatalThrow(0x7000 + rc);
        return;
    }
        
    rc = pminfoInitialize();
    if (R_FAILED(rc)) {
        erroredModuleName = "pminfoInitialize";
        initializationErrorCode = rc;
        fatalThrow(0x8000 + rc);
        return;
    }

    rc = socketMinimalInit();
    if (R_FAILED(rc)) {
        erroredModuleName = "socketMinimalInit";
        initializationErrorCode = rc;
        fatalThrow(0x9000 + rc);
        return;
    }

    rc = pscmInitialize();
    if (R_FAILED(rc)) {
        erroredModuleName = "pscmInitialize";
        initializationErrorCode = rc;
        fatalThrow(0xA000 + rc);
        return;
    }

    rc = fsInitialize();
    if (R_FAILED(rc)) {
        erroredModuleName = "fsInitialize";
        initializationErrorCode = rc;
        fatalThrow(0xB000 + rc);
        return;
    }

    rc = fsdevMountSdmc();
    if (R_FAILED(rc)) {
        erroredModuleName = "fsdevMountSdmc";
        initializationErrorCode = rc;
        fatalThrow(0xC000 + rc);
        return;
    }

    // rc = viInitialize(ViServiceType_Default);
    // if (R_FAILED(rc)) {
    //     erroredModuleName = "viInitialize";
    //     initializationErrorCode = rc;
    //     return;
    // }

    // rc = capsscInitialize();
    // if (R_FAILED(rc)) {
    //     erroredModuleName = "capsscInitialize";
    //     initializationErrorCode = rc;
    //     return;
    // }
}

void serviceExit(void) {
    audoutExit();
    pmdmntExit();
    pminfoExit();
    socketExit();
    pscmExit();
    // capsscExit();
    // viExit();
    smExit();
}

// Custom socket initialization (thanks Antares, Behemoth and Pharynx from the ReSwitched team!)
int socketMinimalInit() {
    constexpr const SocketInitConfig sockConf = {
        .bsdsockets_version = 1,

        .tcp_tx_buf_size = 0x800,
        .tcp_rx_buf_size = 0x800,
        .tcp_tx_buf_max_size = 0x25000,
        .tcp_rx_buf_max_size = 0x25000,

        .udp_tx_buf_size = 0,
        .udp_rx_buf_size = 0,

        .sb_efficiency = 1,

        .num_bsd_sessions = 0,
        .bsd_service_type = BsdServiceType_Auto,
    };

    return socketInitialize(&sockConf);
}

void logToFile(const std::string &log) {
    FILE* logFile = fopen("/config/RemoteLogging/log.txt", "a");
    fprintf(logFile, log.c_str());
    fclose(logFile);
}

void deleteLogFile() {
    FsFileSystem *fs = fsdevGetDeviceFileSystem("sdmc");
    fsFsDeleteFile(fs, "/config/RemoteLogging/log.txt");
}

u8 rtlAttempts = 0;
u8 maxAttempts = 6;
RemoteLogging *rLog;
Mutex logMutex;

// Main program entrypoint
int main(int argc, char* argv[]) {
    #if APPLET
    serviceInit();

    PadState pad;

    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    padInitializeDefault(&pad);

    cout << "Press + to exit.\n" << endl;
    cout << ft("Errored: {}, code: {}\n", ((initializationErrorCode != 0) ? erroredModuleName : "false"), initializationErrorCode) << endl;
    #endif

    Ston::instance()->getEventManager()->initialize();
    mutexInit(&logMutex);

    string remoteLogIP = "192.168.2.20";
    rLog = Ston::instance()->getRemoteLogging();
    rLog->connect(remoteLogIP, REMOTE_PORT, true, 10, 2500);

    HTTPServer *server = new HTTPServer();
    server->startAsync(true, 10);
    
    // Main loop
    while (appletMainLoop()) {
        // logToFile(fmt::format("#appletMainLoop - rtlAttempts={}, RemoteLogging_connected={}\n", rtlAttempts, rLog->isConnected()));

        #if APPLET
        if (!server->listening() && !server->isStarting() && !testCheck) {
            // The `isStarting()` check was implemented to not allow multiple threads to be created while the server is starting.
            // This is NON-blocking! The server is started on a separate thread.
            RINFO("Starting HTTP server...\n");
            server->startAsync();
            testCheck = true;
        }
        #else
        // If not in applet mode, and thus in "prod", start listening in blocking mode within the main loop:
        // this guarantees that when the NS goes in sleep mode, the server re-opens a listening socket.
        // if (testCheck == false && false) {
        //     testCheck = true;
        // server->start();
        // }
        #endif

        #if APPLET
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
        #endif

        svcSleepThread(500'000'000);
    }

	#if APPLET
    server->stopAsync();
    cout << "Exiting..." << endl;
    #endif

    // server->stopSync();

	#if APPLET
    consoleUpdate(NULL);

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);

    serviceExit();
    #endif

    return 0;
}