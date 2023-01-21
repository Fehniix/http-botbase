#pragma once

#include <iostream>
#include <vector>
#include <switch.h>
#include "dmntcht.h"

class Debugger {
	public:
		bool m_dmnt = true;
		Result m_rc = 0;
		Handle m_debugHandle = 0; // no action to be taken to attach

		Debugger();
		~Debugger();

		Result initialize(std::string &error);
		Result attachToProcess();
		Result pause();
		Result resume();
		void detach();
		bool dmntRunning();

		Result refreshMetaData(std::string &error);
		u64 getRunningApplicationTID();
		u64 getRunningApplicationPID();
		
		MemoryInfo queryMemory(u64 address, Result &result);
		Result readMemory(void *buffer, size_t bufferSize, u64 address);
		Result writeMemory(void *buffer, size_t bufferSize, u64 address);
		u64 pointerAll(const s64 jumps[], size_t jumpsLength, Result &ldrDmntResult);

		const DmntMemoryRegionExtents& getHeap() const;
		const DmntMemoryRegionExtents& getMain() const;
		inline u64 getPID();
		inline u64 getTitleID();
		const u8* getNsoBuildId() const;
		inline u64 getHeapEnd() const;
		inline u64 getMainEnd() const;
		const DmntCheatProcessMetadata& getMeta() const;

	private:
		u64 m_tid = 0, m_pid = 0;
		DmntCheatProcessMetadata meta;
		
		// Keeping track of these because, apparently, it may be the case that heapStart + size != heapEnd, same for main. Odd!
		u64 heapEnd, mainEnd;
};