#include "ezOptionParser.h"
#include "BinLog.h"
#include "Execution/Execution.h"
#include "CommonCrossPlatform/Common.h"
#include "revtracer/river.h"
#include "SymbolicEnvironment/Environment.h"

#ifdef _WIN32
#include <Windows.h>
#define LIB_EXT ".dll"
#else
#define LIB_EXT ".so"
extern "C" void patch__rtld_global_ro();
#endif

unsigned int varCount = 1;
ExecutionController *ctrl = NULL;

#define MAX_BUFF 4096
typedef int(*PayloadFunc)();
char *payloadBuffer = nullptr;
PayloadFunc Payload = nullptr;

class BitMap {
private :
	unsigned int sz;
	unsigned int ct;
	unsigned int rw;
	bool isZero;

	unsigned int refCount;

	std::vector<unsigned int> data;

	inline void Init(unsigned int size, unsigned int rows) {
		sz = size;
		rw = rows;
		ct = (sz + 0x1f) >> 5;
		data.resize(ct * rw, 0);

		refCount = 1;
	}
public :
	static BitMap *Union(const BitMap &b1, const BitMap &b2) {
		if ((b1.sz != b2.sz) || (b1.rw != b1.rw)) {
			return nullptr;
		}

		BitMap *ret = new BitMap(b1.sz, b1.rw);

		for (unsigned int i = 0; i < ret->rw; ++i) {
			for (unsigned int j = 0; j < ret->ct; ++j) {
				ret->data[i * ret->ct + j] = b1.data[i * ret->ct + j] | b2.data[i * ret->ct + j];
			}
		}

		printf("Union ");
		b1.Print();
		printf(", ");
		b2.Print();
		printf("=> ");
		ret->Print();
		printf("\n");

		return ret;
	}

	void Union(const BitMap &rhs) {
		if (((sz != rhs.sz) || (rw != rhs.rw)) && (rw != 1)) {
			__asm int 3;
		}

		printf("Selfunion ");
		Print();
		printf(", ");
		rhs.Print();
		printf("=> ");

		if (rw == 1) {
			for (unsigned int i = 0; i < rhs.rw; ++i) {
				for (unsigned int j = 0; j < ct; ++j) {
					data[j] |= rhs.data[i * ct + j];
				}
			}
		} else {
			for (unsigned int i = 0; i < rw; ++i) {
				for (unsigned int j = 0; j < ct; ++j) {
					data[i * ct + j] |= rhs.data[i * ct + j];
				}
			}
		}

		Print();
		printf("\n");
	}

	BitMap(unsigned int size, unsigned int rows) {
		Init(size, rows);
		isZero = true;
	}

	BitMap(const BitMap &b1, const BitMap &b2) {
		if (b1.sz != b2.sz) {
			__asm int 3;
		}

		Init(b1.sz, b1.rw + b2.rw);
		isZero = b1.isZero & b2.isZero;

		for (unsigned int i = 0; i < b1.rw; ++i) {
			for (unsigned int j = 0; j < ct; ++j) {
				data[i * ct + j] = b1.data[i * ct + j];
			}
		}

		for (unsigned int i = 0; i < b2.rw; ++i) {
			for (unsigned int j = 0; j < ct; ++j) {
				data[(b1.rw + i) * ct + j] = b2.data[i * ct + j];
			}
		}
	}

	BitMap(const BitMap &o, unsigned int c = 1) {
		Init(o.sz, c);

		for (unsigned int j = 0; j < ct; ++j) {
			unsigned int r = 0;
			for (unsigned int i = 0; i < o.rw; ++i) {
				r |= o.data[i * ct + j];
			}

			for (unsigned int i = 0; i < rw; ++i) {
				data[i * ct + j] = r;
			}
		}
	}

	BitMap(const BitMap &o, unsigned int s, unsigned int c) {
		Init(o.sz, c);

		if (o.rw != 1) {
			for (unsigned int i = 0; i < rw; ++i) {
				for (unsigned int j = 0; j < ct; ++j) {
					data[i * ct + j] = o.data[(i + s) * ct + j];
				}
			}
		} else {
			for (unsigned int i = 0; i < rw; ++i) {
				for (unsigned int j = 0; j < ct; ++j) {
					data[i * ct + j] = o.data[j];
				}
			}
		}
	}

	void SetBit(unsigned int p) {
		unsigned int m = 1 << (p & 0x1F);
		for (unsigned int i = 0; i < rw; ++i) {
			data[i * ct + (p >> 5)] |= m;
		}

		isZero = false;
	}

	bool GetBit(unsigned int p) const {
		unsigned int m = 1 << (p & 0x1F);
		unsigned int r = 0;

		for (unsigned int i = 0; i < rw; ++i) {
			r |= data[i * ct + (p >> 5)];
		}

		return 0 != (r & m);
	}

	bool IsZero() const {
		return isZero;
	}

	void Print() const {
		printf("<%08x> ", this);
		for (unsigned int i = 0; i < rw; ++i) {
			for (unsigned int j = 0; j < ct; ++j) {
				printf("%08x ", data[i * ct + j]);
			}
			printf("| ");
		}
	}
	
	void AddRef() {
		refCount++;
	}

	void DelRef() {
		refCount--;
		if (0 == refCount) {
			printf("Delete ");
			Print();
			printf("\n");
			delete this;
		}
	}
} *bitMapZero;

class TrackingExecutor : public sym::SymbolicExecutor {
public :
	BitMap *lastCondition[3];
	unsigned int condCount;

	TrackingExecutor(sym::SymbolicEnvironment *e) : SymbolicExecutor(e) {
		condCount = 0;
		lastCondition[0] = lastCondition[1] = lastCondition[2] = nullptr;
	}

	virtual void *CreateVariable(const char *name, rev::DWORD size) {
		BitMap *bmp = new BitMap(varCount, 1);
		bmp->SetBit(atoi(&name[2]));

		printf("Creating variable %s => ", name);
		bmp->Print();
		printf("\n");
		fflush(stdout);

		return bmp;
	}

	virtual void *MakeConst(rev::DWORD value, rev::DWORD bits) {
		bitMapZero->AddRef();
		return bitMapZero;
	}

	virtual void *ExtractBits(void *expr, rev::DWORD lsb, rev::DWORD size) {
		BitMap *bmp = (BitMap *)expr;

		if (nullptr == bmp) {
			return nullptr;
		}

		BitMap *ret = new BitMap(*bmp, 4 - (lsb >> 3) - (size >> 3), size >> 3);
		printf("Extract ");
		bmp->Print();
		printf(" %d %d => ", 4 - (lsb >> 3) - (size >> 3), size >> 3);
		ret->Print();
		printf("\n");
		fflush(stdout);
		return ret;
	}

	virtual void *ConcatBits(void *expr1, void *expr2) {
		BitMap *bmp1 = (BitMap *)expr1;
		BitMap *bmp2 = (BitMap *)expr2;

		if (nullptr == bmp1) {
			if (nullptr == bmp2) {
				return nullptr;
			}

			bitMapZero->AddRef();
			bmp1 = bitMapZero;
		}

		if (nullptr == bmp2) {
			bitMapZero->AddRef();
			bmp2 = bitMapZero;
		}

		BitMap *ret = new BitMap(*bmp1, *bmp2);
		printf("Concat ");
		bmp1->Print();
		printf(", ");
		bmp2->Print();
		printf("=> ");
		ret->Print();
		printf("\n");
		fflush(stdout);
		return ret;
	}

	#define OPERAND_BITMASK(idx) (0x00010000 << (idx))

	static const unsigned int flagValues[];

	void ExecuteJCC(unsigned int flag, RiverInstruction *instruction) {
		rev::BOOL isTracked;
		rev::BYTE val;
		void *lc;
		
		condCount = 0;
		
		for (int i = 0; i < 7; ++i) {
			if ((1 << i) && instruction->testFlags) {
				if (env->GetFlgValue((1 << i), isTracked, val, lc)) {
					lastCondition[condCount] = (BitMap *)lc;
					lastCondition[condCount]->AddRef();
					condCount++;
				}
			}
		}
	}

	void ResetCond() {
		condCount = 0;
	}

	struct Operands {
		bool useOp[4];
		BitMap *operands[4];
		bool useFlag[7];
		BitMap *flags[7];
	};

	virtual void Execute(RiverInstruction *instruction) {
		static const unsigned char flagList[] = {
			RIVER_SPEC_FLAG_CF,
			RIVER_SPEC_FLAG_PF,
			RIVER_SPEC_FLAG_AF,
			RIVER_SPEC_FLAG_ZF,
			RIVER_SPEC_FLAG_SF,
			RIVER_SPEC_FLAG_OF
		};

		static const int flagCount = sizeof(flagList) / sizeof(flagList[0]);

		Operands ops;
		memset(&ops, 0, sizeof(ops));
		bool trk = false;

		for (int i = 0; i < 4; ++i) {
			rev::BOOL isTracked;
			rev::DWORD val;
			void *opVal;
			if (true == (ops.useOp[i] = env->GetOperand(i, isTracked, val, opVal))) {
				if (isTracked) {
					ops.operands[i] = (BitMap *)opVal;
					ops.operands[i]->AddRef();
					trk = true;
				} else {
					bitMapZero->AddRef();
					ops.operands[i] = bitMapZero;
				}
			}
		}

		for (int i = 0; i < flagCount; ++i) {
			rev::BOOL isTracked;
			rev::BYTE val;
			void *opVal;
			if (true == (ops.useFlag[i] = env->GetFlgValue(flagList[i], isTracked, val, opVal))) {
				if (isTracked) {
					ops.flags[i] = (BitMap *)opVal;
					ops.flags[i]->AddRef();
					trk = true;
				} else {
					bitMapZero->AddRef();
					ops.operands[i] = bitMapZero;
				}
			}
		}

		if ((0 == (instruction->modifiers & RIVER_MODIFIER_EXT)) && (0x70 <= instruction->opCode) && (instruction->opCode < 0x80)) {
			ExecuteJCC(instruction->opCode - 0x70, instruction);
		}

		if (trk) {
			BitMap *ret = new BitMap(varCount, 1);

			for (int i = 0; i < 4; ++i) {
				if (ops.useOp[i]) {
					ret->Union(*ops.operands[i]);
				}
			}

			for (int i = 0; i < flagCount; ++i) {
				if (ops.useFlag[i]) {
					ret->Union(*ops.flags[i]);
				}
			}

			for (int i = 0; i < 4; ++i) {
				if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
					// this will leak a lot of memory
					ret->AddRef();
					env->SetOperand(i, ret);
				}
			}

			for (int i = 0; i < flagCount; ++i) {
				if (flagList[i] & instruction->modFlags) {
					// whis will also leak a lot of memory
					ret->AddRef();
					env->SetFlgValue(flagList[i], ret);
				}
			}

			for (int i = 0; i < 4; ++i) {
				if (ops.useOp[i]) {
					ops.operands[i]->DelRef();
				}
			}

			for (int i = 0; i < flagCount; ++i) {
				if (ops.useFlag[i]) {
					ops.flags[i]->DelRef();
				}
			}

			ret->DelRef();
		} else {
			// unset all modified operands
			for (int i = 0; i < 4; ++i) {
				if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
					// this will leak a lot of memory
					env->UnsetOperand(i);
				}
			}

			// unset all modified flags
			for (int i = 0; i < flagCount; ++i) {
				if (flagList[i] & instruction->modFlags) {
					// whis will also leak a lot of memory
					env->UnsetFlgValue(flagList[i]);
				}
			}
		}
	}
};

const unsigned int TrackingExecutor::flagValues[] = {
	RIVER_SPEC_FLAG_OF, RIVER_SPEC_FLAG_OF,
	RIVER_SPEC_FLAG_CF, RIVER_SPEC_FLAG_CF,
	RIVER_SPEC_FLAG_ZF, RIVER_SPEC_FLAG_ZF,
	RIVER_SPEC_FLAG_ZF | RIVER_SPEC_FLAG_CF, RIVER_SPEC_FLAG_ZF | RIVER_SPEC_FLAG_CF,
	RIVER_SPEC_FLAG_SF, RIVER_SPEC_FLAG_SF,
	RIVER_SPEC_FLAG_PF, RIVER_SPEC_FLAG_PF,
	RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_SF, RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_SF,
	RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_SF | RIVER_SPEC_FLAG_ZF, RIVER_SPEC_FLAG_OF | RIVER_SPEC_FLAG_SF | RIVER_SPEC_FLAG_ZF,
};

class CustomObserver : public ExecutionObserver {
public :
	FILE *fBlocks;
	bool binOut;

	BinLogWriter *blw;

	std::string patchFile;
	ModuleInfo *mInfo;
	int mCount;

	bool ctxInit;

	sym::SymbolicEnvironment *regEnv;
	sym::SymbolicEnvironment *revEnv;
	TrackingExecutor *executor;

	virtual void TerminationNotification(void *ctx) {
		printf("Process Terminated\n");
	}

	unsigned int GetModuleOffset(const std::string &module) const {
		const char *m = module.c_str();
		for (int i = 0; i < mCount; ++i) {
			if (0 == strcmp(mInfo[i].Name, m)) {
				return mInfo[i].ModuleBase;
			}
		}

		return 0;
	}

	bool PatchLibrary(std::ifstream &fPatch) {
		std::string line;
		while (std::getline(fPatch, line)) {
			bool bForce = false;
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
					bForce = true;
					nStart++;
					break;
			}

			int sep = line.find(L'+');
			int val = line.find(L'=');
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

	virtual unsigned int ExecutionBegin(void *ctx, void *address) {
		printf("Process starting\n");
		ctrl->GetModules(mInfo, mCount);

		if (!patchFile.empty()) {
			std::ifstream fPatch;

			fPatch.open(patchFile);
			
			if (!fPatch.good()) {
				std::cout << "Patch file not found" << std::endl;
				return EXECUTION_TERMINATE;
			}

			PatchLibrary(fPatch);
			fPatch.close();

			if (!ctxInit) {
				bitMapZero = new BitMap(varCount, 1);

				revEnv = NewX86RevtracerEnvironment(ctx, ctrl); //new RevSymbolicEnvironment(ctx, ctrl);
				regEnv = NewX86RegistersEnvironment(revEnv); //new OverlappedRegistersEnvironment();
				executor = new TrackingExecutor(regEnv);
				regEnv->SetExecutor(executor);

				for (unsigned int i = 0; i < varCount; ++i) {
					char vname[8];

					sprintf(vname, "s[%d]", i);

					revEnv->SetSymbolicVariable(vname, (rev::ADDR_TYPE)(&payloadBuffer[i]), 1);
				}

				ctxInit = true;
			}

		}

		return EXECUTION_ADVANCE;
	}

	virtual unsigned int ExecutionControl(void *ctx, void *address) {
		const char unkmod[MAX_PATH] = "???";
		unsigned int offset = (DWORD)address;
		int foundModule = -1;

		for (int i = 0; i < mCount; ++i) {
			if ((mInfo[i].ModuleBase <= (DWORD)address) && ((DWORD)address < mInfo[i].ModuleBase + mInfo[i].Size)) {
				offset -= mInfo[i].ModuleBase;
				foundModule = i;
				break;
			}
		}

		if (executor->condCount) {
			for (unsigned int i = 0; i < varCount; ++i) {
				bool bPrint = true;
				for (unsigned int j = 0; j < executor->condCount; ++j) {
					bPrint &= ((BitMap *)executor->lastCondition[j])->GetBit(i);
				}

				if (bPrint) {
					fprintf(fBlocks, "Using variable s[%d]\n", i);
				}
			}
		}

		for (unsigned int i = 0; i < executor->condCount; ++i) {
			executor->lastCondition[i]->DelRef();
			executor->lastCondition[i] = nullptr;
		}
		executor->condCount = 0;

		if (binOut) {
			blw->WriteEntry((-1 == foundModule) ? unkmod : mInfo[foundModule].Name, offset, ctrl->GetLastBasicBlockCost(ctx));
		} else {
			fprintf(fBlocks, "%-15s + %08lX (%4d)\n",
				(-1 == foundModule) ? unkmod : mInfo[foundModule].Name,
				(DWORD)offset,
				ctrl->GetLastBasicBlockCost(ctx)
			);
		}
		return EXECUTION_ADVANCE;
	}

	virtual unsigned int ExecutionEnd(void *ctx) {
		fflush(fBlocks);
		return EXECUTION_TERMINATE;
	}

	CustomObserver() {
		fBlocks = nullptr;
		binOut = false;

		blw = nullptr;

		ctxInit = false;

		regEnv = nullptr;
		revEnv = nullptr;
	}

	~CustomObserver() {
		if (nullptr != blw) {
			delete blw;
			blw = nullptr;
		}
	}
} observer;

void __stdcall SymbolicHandler(void *ctx, void *offset, void *addr) {
	RiverInstruction *instr = (RiverInstruction *)addr;

	observer.regEnv->SetCurrentInstruction(instr, offset);
	observer.executor->Execute(instr);
}

int main(int argc, const char *argv[]) {
	ez::ezOptionParser opt;

	opt.overview = "River simple tracer.";
	opt.syntax = "tracer.simple [OPTIONS]";
	opt.example = "tracer.simple -o<outfile>\n";

	opt.add(
		"",
		0,
		0,
		0,
		"Use inprocess execution.",
		"--inprocess"
	);

	opt.add(
		"",
		0,
		0,
		0,
		"Use extern execution.",
		"--extern"
	);

	opt.add(
		"", // Default.
		0, // Required?
		0, // Number of args expected.
		0, // Delimiter if expecting multiple args.
		"Display usage instructions.", // Help description.
		"-h",     // Flag token. 
		"--help", // Flag token.
		"--usage" // Flag token.
	);

	opt.add(
		"trace.simple.out", // Default.
		0, // Required?
		1, // Number of args expected.
		0, // Delimiter if expecting multiple args.
		"Set the trace output file.", // Help description.
		"-o",			 // Flag token.
		"--outfile"     // Flag token. 
	);

	opt.add(
		"",
		false,
		0,
		0,
		"Use binary logging instead of textual logging.",
		"--binlog"
	);

	opt.add(
		"payload" LIB_EXT,
		0,
		1,
		0,
		"Set the payload file. Only applicable for in-process tracing.",
		"-p",
		"--payload"
	);

	opt.add(
		"",
		0,
		1,
		0,
		"Set the memory patching file.",
		"-m",
		"--mem-patch"
	);

	opt.parse(argc, argv);

	uint32_t executionType = EXECUTION_INPROCESS;

	if (opt.isSet("--inprocess") && opt.isSet("--extern")) {
		std::cout << "Conflicting options --inprocess and --extern" << std::endl;
		return 0;
	}

	if (opt.isSet("--extern")) {
		executionType = EXECUTION_EXTERNAL;
	}

	if (executionType != EXECUTION_INPROCESS) {
		std::cout << "Only inprocess execution supported for now! Sorry!" << std::endl;
		return 0;
	}

	ctrl = NewExecutionController(executionType);

	if (opt.isSet("-h")) {
		std::string usage;
		opt.getUsage(usage);
		std::cout << usage;
		return 0;
	}

	std::string fModule;
	opt.get("-p")->getString(fModule);
	std::cout << "Using payload " << fModule << std::endl;
	lib_t hModule = GET_LIB_HANDLER(fModule.c_str());
	if (nullptr == hModule) {
		std::cout << "Payload not found" << std::endl;
		return 0;
	}

	payloadBuffer = (char *)LOAD_PROC(hModule, "payloadBuffer");
	Payload = (PayloadFunc)LOAD_PROC(hModule, "Payload");

	if ((nullptr == payloadBuffer) || (nullptr == Payload)) {
		std::cout << "Payload imports not found" << std::endl;
		return 0;
	}

	if (opt.isSet("--binlog")) {
		observer.binOut = true;
	}


	std::string fName;
	opt.get("-o")->getString(fName);
	std::cout << "Writing " << (observer.binOut ? "binary" : "text") << " output to " << fName << std::endl;
	
	char *buff = payloadBuffer;
	unsigned int bSize = MAX_BUFF;
	do {
		fgets(buff, bSize, stdin);
		while (*buff) {
			buff++;
			bSize--;
		}
	} while (!feof(stdin));

	varCount = buff - payloadBuffer - 1;
	FOPEN(observer.fBlocks, fName.c_str(), observer.binOut ? "wb" : "wt");

	if (observer.binOut) {
		observer.blw = new BinLogWriter(observer.fBlocks);
	}
		
	if (opt.isSet("-m")) {
		opt.get("-m")->getString(observer.patchFile);
	}
#ifdef __linux__
	patch__rtld_global_ro();
#endif

	ctrl->SetEntryPoint((void*)Payload);
	
	ctrl->SetExecutionFeatures(TRACER_FEATURE_SYMBOLIC);

	ctrl->SetExecutionObserver(&observer);
	ctrl->SetSymbolicHandler(SymbolicHandler);
	
	ctrl->Execute();

	ctrl->WaitForTermination();

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	fclose(observer.fBlocks);
	return 0;
}
