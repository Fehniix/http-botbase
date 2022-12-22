#pragma once

#include <iostream>
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

		u64 peekMemory(u64 address);
		Result pokeMemory(size_t varSize, u64 address, u64 value);
		MemoryInfo queryMemory(u64 address);
		Result readMemory(void *buffer, size_t bufferSize, u64 address);
		Result writeMemory(void *buffer, size_t bufferSize, u64 address);

		const DmntMemoryRegionExtents& getHeap() const;
		const DmntMemoryRegionExtents& getMain() const;
		u64 getPID();
		u64 getTitleID();
		const u8* getNsoBuildId() const;

	private:
		u64 m_tid = 0, m_pid = 0;
		DmntCheatProcessMetadata meta;
};