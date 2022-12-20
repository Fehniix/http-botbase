#include <iostream>
#include <switch.h>
#include <httplib.hpp>
#include "HTTPServer.hpp"
#include "util.hpp"

#define TITLE_ID 0x410000000000FF15

using namespace std;

string erroredModuleName;
Result initializationErrorCode = 0;

#if !APPLET
#define INNER_HEAP_SIZE 0x00C00000

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
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

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

    #if NXLINK
    nxlinkStdioForDebug();
    svcSleepThread(2e+9);
    DEBUGMSG("[NXLINK] Session started!\n");
    #endif
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
    Result rc;

    rc = pmdmntInitialize();
    if (R_FAILED(rc)) {
        erroredModuleName = "pmdmntInitialize";
        initializationErrorCode = rc;
        return;
    }
        
    rc = pminfoInitialize();
    if (R_FAILED(rc)) {
        erroredModuleName = "pminfoInitialize";
        initializationErrorCode = rc;
        return;
    }

    rc = socketInitializeDefault();
    if (R_FAILED(rc)) {
        erroredModuleName = "socketInitializeDefault";
        initializationErrorCode = rc;
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
    // capsscExit();
    // viExit();
    smExit();
}

// Main program entrypoint
int main(int argc, char* argv[]) {
    #if APPLET
    serviceInit();
    #endif

    #if APPLET
    PadState pad;

    consoleInit(NULL);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    padInitializeDefault(&pad);

    cout << "Press + to exit.\n" << endl;
    cout << "Errored: " << ((initializationErrorCode != 0) ? erroredModuleName : "false") << ", code: " << initializationErrorCode << endl;
    #endif

	HTTPServer *server = new HTTPServer();
    
    // Main loop
    while (appletMainLoop()) {
        if (!server->started)
            // This is NON-blocking! The server is started on a separate thread. 
            server->start();

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
    cout << "Exiting..." << endl;
    #endif

	server->stop();

	#if APPLET
    consoleUpdate(NULL);

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);

    serviceExit();
    #endif

    return 0;
}