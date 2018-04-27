#include <stdint.h>
#include <string.h>

#include "crc32.h"
#include "CallStack.h"
#include "Common.h"

#include "CommonCrossPlatform/Common.h"

CallStack::CallStack()
{}

CallStack::~CallStack() {}

void CallStack::LogEntry(const struct CallData &cd) {
	printf("#%zu\t0x%08X at %s\n",
			stack.size(),
			cd.offset,
			modules[cd.moduleHash].c_str());
}

void CallStack::Push(uint32_t esp, uint32_t offset, const char *moduleName) {
	uint32_t moduleHash = crc32(0,
			(unsigned char *)moduleName,
			strlen(moduleName));
	struct CallData cd(esp, offset, moduleHash);

	if (!modules.count(moduleHash)) {
		modules[moduleHash] = std::string(moduleName);
	}
	stack.push(cd);
}

void CallStack::Pop(struct CallData &cd) {
	if (Empty()) {
		return;
	}

	cd = stack.top();
	LogEntry(cd);
	stack.pop();
}

uint32_t CallStack::GetLastCallFrame() {
	if (Empty())
		return 0xffffffff;
	auto cd = stack.top();
	return cd.esp;
}
