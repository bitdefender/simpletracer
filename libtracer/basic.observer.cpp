#include <stdio.h>
#include <string.h>

#include <string>
#include <fstream>
#include <iostream>

#include "basic.observer.h"

void BasicObserver::TerminationNotification(void *ctx) {
}

unsigned int BasicObserver::GetModuleOffset(const std::string &module) const {
	const char *m = module.c_str();
	for (int i = 0; i < mCount; ++i) {
		if (0 == strcmp(mInfo[i].Name, m)) {
			return mInfo[i].ModuleBase;
		}
	}

	return 0;
}

bool BasicObserver::PatchLibrary(std::ifstream &fPatch) {
	std::string line;
	while (std::getline(fPatch, line)) {
		int nStart = 0;

		// l-trim equivalent
		while ((line[nStart] == ' ') || (line[nStart] == '\t')) {
			nStart++;
		}

		switch (line[nStart]) {
			case '#' : // comment line
			case '\n' : // empty line
				continue;
			case '!' : // force patch line
				nStart++;
				break;
		}

		unsigned int sep = line.find(L'+');
		unsigned int val = line.find(L'=');
		if ((std::string::npos == sep) || (std::string::npos == val)) {
			return false;
		}

		unsigned int module = GetModuleOffset(line.substr(0, sep));

		if (0 == module) {
			return false;
		}

		unsigned long offset = std::stoul(line.substr(sep + 1, val), nullptr, 16);
		unsigned long value = std::stoul(line.substr(val + 1), nullptr, 16);


		// TODO: move this to the controller
		*(unsigned int *)(module + offset) = value;
	}

	return true;
}

int BasicObserver::HandlePatchLibrary() {
	if (!patchFile.empty()) {
		std::ifstream fPatch;

		fPatch.open(patchFile);

		if (!fPatch.good()) {
			std::cerr << "Patch file not found" << std::endl;
			return -1;
		}

		PatchLibrary(fPatch);
		fPatch.close();

	}

	return 0;
}

BasicObserver::BasicObserver()
	: binOut(false), aLog(nullptr), aFormat(nullptr),
  mInfo(nullptr), mCount(0), ctxInit(false)
{ }

BasicObserver::~BasicObserver() {
	delete aLog;
	delete aFormat;
}
