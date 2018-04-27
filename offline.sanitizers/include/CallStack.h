#ifndef __CALLSTACK__
#define __CALLSTACK__

#include <map>
#include <stack>

/*
 * | ret addr |
 * | old ebp  |
 * .
 * .
 * | arg 1
 * | arg 0
 * | ret addr
 * | old ebp
 */

struct CallData {
	uint32_t esp;
	uint32_t offset;
	uint32_t moduleHash;

	CallData(uint32_t esp, uint32_t offset, uint32_t moduleHash)
		: esp(esp), offset(offset), moduleHash(moduleHash) {}

	CallData()
		: esp(0), offset(0), moduleHash(0) {}
};

class CallStack {
	public:
		CallStack();
		~CallStack();
		void Push(uint32_t esp, uint32_t offset, const char *moduleName);
		void Pop(struct CallData &cd);

		bool Empty() {
			return stack.empty();
		}

		void LogEntry(const struct CallData &cd);
		uint32_t GetLastCallFrame();
	private:
		std::stack<struct CallData> stack;
		std::map<uint32_t, std::string> modules;
};
#endif
