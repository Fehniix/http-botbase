#include <iostream>
#include <vector>
#include "debugger.hpp"
#include "dmntcht.h"
#include "util.hpp"
#include "commands.hpp"

Debugger::Debugger() {
	std::string error;
	this->initialize(error);
}

Debugger::~Debugger() {
	if (!m_dmnt) {
		svcContinueDebugEvent(m_debugHandle, 4 | 2 | 1, 0, 0);
		svcCloseHandle(m_debugHandle);
	}
}

Result Debugger::initialize(std::string& error) {
	m_pid = 0;
	m_tid = 0;

	Result rc = pmdmntGetApplicationProcessId(&m_pid);
	if (R_FAILED(rc)) {
		error = "Could not get application process ID: " + std::to_string(rc);
		return (Result)1;
	}

	rc = pminfoGetProgramId(&m_tid, m_pid);
	if (R_FAILED(rc)) {
		error = "Could not get program ID: " + std::to_string(rc);
		return (Result)1;
	}

	if (!this->dmntRunning())
		this->m_dmnt = false;

	return (Result)0;
}

/**
 * @brief Initializes dmnt:cht and force-opens the currently active cheat process. 
 */
Result Debugger::dmntchtAttach(std::string &error) {
	Result rc = dmntchtInitialize();
	if (R_FAILED(rc)) {
		error = "Could not initialize dmntcht, error: " + iths(rc);
	}

	rc = dmntchtForceOpenCheatProcess();
	if (R_FAILED(rc)) {
		error = "Could not force open cheat process (dmnt:cht, forceOpenCheatProcess): " + iths(rc);
		dmntchtExit();
	}

	return rc;
}

/**
 * @brief Force-closes the currently opened cheat process and exits from dmnt:cht.
 */
Result Debugger::dmntchtDetach() {
	Result rc = dmntchtForceCloseCheatProcess();

	if (R_FAILED(rc))
		return rc;

	dmntchtExit();

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

Result Debugger::refreshMetaData(std::string &error) {
	Result rc;

	rc = dmntchtInitialize();
	if (R_FAILED(rc)) {
		error = "Could not initialize dmntcht, error: " + iths(rc);
		return rc;
	}

	rc = dmntchtForceOpenCheatProcess();
	if (R_FAILED(rc)) {
		error = "Could not force open cheat process (dmnt:cht, forceOpenCheatProcess): " + iths(rc);
		dmntchtExit();
		return rc;
	}

	rc = dmntchtGetCheatProcessMetadata(&this->meta);
	if (R_FAILED(rc)) 
		error = "Could not fetch process metadata, error: " + iths(rc);

	// Reset HEAP & MAIN
	this->meta.heap_extents.base = 0;
	this->meta.main_nso_extents.base = 0;

	// Traverse all pages to look for HEAP & MAIN!
	// Big thanks to EdizonSE!
	MemoryInfo memoryInfo 	= {0};
	u64 lastAddr 			= memoryInfo.addr;
	u32 mod 				= 0;

	do {
		memoryInfo = this->queryMemory(memoryInfo.addr + memoryInfo.size, rc);

		if (memoryInfo.type == MemType_Heap) {
			if (this->meta.heap_extents.base == 0)
				this->meta.heap_extents.base = memoryInfo.addr;
			this->heapEnd = memoryInfo.addr + memoryInfo.size;
		}

		if (memoryInfo.type == MemType_CodeMutable && mod == 2)
			this->mainEnd = memoryInfo.addr + memoryInfo.size;

		if (memoryInfo.type == MemType_CodeStatic && memoryInfo.perm == Perm_Rx) {
			if (mod == 1)
				this->meta.main_nso_extents.base = memoryInfo.addr;
			mod++;
		}
		
		if (R_FAILED(rc))
			break;
	} while(lastAddr < memoryInfo.addr + memoryInfo.size);

	dmntchtForceCloseCheatProcess();
	dmntchtExit();

	return rc;
}

u64 Debugger::getRunningApplicationTID() {
	return m_tid;
}

u64 Debugger::getRunningApplicationPID() {
	return m_pid;
}

Result Debugger::pause() {
	// if (m_dmnt)
		return dmntchtPauseCheatProcess();
	// else
	// 	return svcBreakDebugProcess(m_debugHandle);
}

Result Debugger::resume() {
	// if (m_dmnt)
		return dmntchtResumeCheatProcess();
	// else
	// 	return svcContinueDebugEvent(m_debugHandle, 4 | 2 | 1, 0, 0);
}

Result Debugger::readMemory(void *buffer, size_t bufferSize, u64 address) {
	// if (m_dmnt)
		return dmntchtReadCheatProcessMemory(address, buffer, bufferSize);
	// else
	// 	return svcReadDebugProcessMemory(buffer, m_debugHandle, address, bufferSize);
}

Result Debugger::writeMemory(void *buffer, size_t bufferSize, u64 address) {
	// if (m_dmnt)
		return dmntchtWriteCheatProcessMemory(address, buffer, bufferSize);
	// else
	// 	return svcWriteDebugProcessMemory(m_debugHandle, buffer, address, bufferSize);
}

MemoryInfo Debugger::queryMemory(u64 address, Result &result) {
	MemoryInfo memInfo = { 0 };
	// if (m_dmnt)
		dmntchtQueryCheatProcessMemory(&memInfo, address);
	// else {
	// 	u32 pageinfo; // ignored
	// 	svcQueryDebugProcessMemory(&memInfo, &pageinfo, m_debugHandle, address);
	// }

	return memInfo;
}

u64 Debugger::pointerAll(const s64 jumps[], size_t jumpsLength, Result &ldrDmntResult) {
	s64 finalJump = jumps[jumpsLength - 1];
	
	s64 jumpsWithoutFinal[jumpsLength - 1] = {};
	for (size_t i = 0; i < jumpsLength - 1; i++)
		jumpsWithoutFinal[i] = jumps[i];

	u64 solved = followMainPointer(jumpsWithoutFinal, jumpsLength - 1, ldrDmntResult);

	if (solved != 0)
		solved += finalJump;

	return solved;
}

/* 
* ------------
* GETTERS
* ------------
*/

/**
 * @brief Gets the currently running application's heap start & size.
 * 
 * @return const DmntMemoryRegionExtents& 
 */
const DmntMemoryRegionExtents& Debugger::getHeap() const {
	return this->meta.heap_extents;
}

const DmntMemoryRegionExtents& Debugger::getMain() const {
	return this->meta.main_nso_extents;
}

u64 Debugger::getPID() {
	return this->meta.process_id;
}

u64 Debugger::getTitleID() {
	return this->meta.title_id;
}

const u8* Debugger::getNsoBuildId() const {
	return this->meta.main_nso_build_id;
}

inline u64 Debugger::getHeapEnd() const {
	return this->heapEnd;
}

inline u64 Debugger::getMainEnd() const {
	return this->mainEnd;
}

const DmntCheatProcessMetadata& Debugger::getMeta() const {
	return this->meta;
}