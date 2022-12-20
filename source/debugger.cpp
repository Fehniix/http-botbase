#include <iostream>
#include "debugger.hpp"
#include "dmntcht.h"
#include "util.hpp"

Debugger::Debugger() {
    std::string error;
    this->initialize(error);
}

Debugger::~Debugger() {
    if (!m_dmnt) {
        svcContinueDebugEvent(m_debugHandle, 4 | 2 | 1, 0, 0);
        svcCloseHandle(m_debugHandle);
    }

    pminfoExit();
    pmdmntExit();
    dmntchtExit(); // need to check if dmntcht is runnnig
}

Result Debugger::initialize(std::string& error) {
    Result rc;

    if (this->dmntRunning()) {
        Result rc = dmntchtInitialize();

        if (R_FAILED(rc)) {
            error = "dmntchtInitialize failed: " + std::to_string(rc);
            return (Result)1;
        }
    } else {
        error = "dmnt not running.";
        return (Result)1;
    }

    m_pid = 0;
    m_tid = 0;

    rc = pmdmntGetApplicationProcessId(&m_pid);
    if (R_FAILED(rc)) {
        error = "Could not get application process ID: " + std::to_string(rc);
        return (Result)1;
    }

    rc = pminfoGetProgramId(&m_tid, m_pid);
    if (R_FAILED(rc)) {
        error = "Could not get program ID: " + std::to_string(rc);
        return (Result)1;
    }

    error = "all is well?";
    return (Result)0;
}

Result Debugger::attachToProcess() {
    if (this->m_debugHandle == 0 && (envIsSyscallHinted(0x60) == 1)) {
        this->m_rc = svcDebugActiveProcess(&this->m_debugHandle, this->m_pid);
        if ((int)this->m_rc == 0)
            this->m_dmnt = false;
        return m_rc;
    }
    return 1;
  // return dmntchtForceOpenCheatProcess();
}

void Debugger::detach() {
    if (!m_dmnt) {
        svcContinueDebugEvent(m_debugHandle, 4 | 2 | 1, 0, 0);
        svcCloseHandle(m_debugHandle);
        m_dmnt = true;
    }
}

bool Debugger::dmntRunning() {
    // Get all process ids
    u64 process_ids[0x50];
    s32 num_process_ids;

    Result rc = svcGetProcessList(&num_process_ids, process_ids, sizeof process_ids);  // need to double check
    if (R_FAILED(rc))
        return false;

    // Look for dmnt or dmntgen2 titleID
    u64 titleID;
    for (s32 i = 0; i < num_process_ids; ++i)
        if (R_SUCCEEDED(pminfoGetProgramId(&titleID, process_ids[i])))
            if (titleID == 0x010000000000000D)
                return true;

    return false;
};

u64 Debugger::getRunningApplicationTID() {
    return m_tid;
}

u64 Debugger::getRunningApplicationPID() {
    return m_pid;
}

u64 Debugger::peekMemory(u64 address) {
    u64 out;

    if (m_dmnt)
        dmntchtReadCheatProcessMemory(address, &out, sizeof(u64));
    else
        svcReadDebugProcessMemory(&out, m_debugHandle, address, sizeof(u64));
    
    return out;
}

Result Debugger::pokeMemory(size_t varSize, u64 address, u64 value) {
    if (m_dmnt)
        return dmntchtWriteCheatProcessMemory(address, &value, varSize);
    else
        return svcWriteDebugProcessMemory(m_debugHandle, &value, address, varSize);
}

Result Debugger::pause() {
    if (m_dmnt)
        return dmntchtPauseCheatProcess();
    else
        return svcBreakDebugProcess(m_debugHandle);
}

Result Debugger::resume() {
    if (m_dmnt)
        return dmntchtResumeCheatProcess();
    else
        return svcContinueDebugEvent(m_debugHandle, 4 | 2 | 1, 0, 0);
}

Result Debugger::readMemory(void *buffer, size_t bufferSize, u64 address) {
    if (m_dmnt)
        return dmntchtReadCheatProcessMemory(address, buffer, bufferSize);
    else
        return svcReadDebugProcessMemory(buffer, m_debugHandle, address, bufferSize);
}

Result Debugger::writeMemory(void *buffer, size_t bufferSize, u64 address) {
    if (m_dmnt)
        return dmntchtWriteCheatProcessMemory(address, buffer, bufferSize);
    else
        return svcWriteDebugProcessMemory(m_debugHandle, buffer, address, bufferSize);
}

MemoryInfo Debugger::queryMemory(u64 address) {
    MemoryInfo memInfo = { 0 };
    if (m_dmnt)
        dmntchtQueryCheatProcessMemory(&memInfo, address);
    else {
        u32 pageinfo; // ignored
        svcQueryDebugProcessMemory(&memInfo, &pageinfo, m_debugHandle, address);
    }

    return memInfo;
}