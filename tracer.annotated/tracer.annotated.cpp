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
private:
	unsigned int __id__;
	unsigned int sz;
	std::vector<unsigned int> data;
	unsigned int refCount;
	bool isZero;
public :
	BitMap(unsigned int size) {
		__id__ = 'PMTB';
		sz = size;
		data.resize((sz + 0x1f) >> 5, 0);
		refCount = 1;
		isZero = true;
	}

	BitMap *Clone() const {
		BitMap *ret = new BitMap(sz);
		
		ret->isZero = isZero;
		for (unsigned int i = 0; i < data.size(); ++i) {
			ret->data[i] = data[i];
		}

		return ret;
	}

	bool UnionWith(BitMap *rhs) {
		if (sz != rhs->sz) {
			return false;
		}

		isZero &= rhs->IsZero();

		for (unsigned int i = 0; i < data.size(); ++i) {
			data[i] |= rhs->data[i];
		}

		return true;
	}

	void SetBit(unsigned int bit) {
		isZero = false;
		data[bit >> 5] |= (1 << (bit & 0x1F));
	}

	bool GetBit(unsigned int bit) const {
		return 0 != (data[bit >> 5] & (1 << (bit & 0x1F)));
	}

	bool IsZero() {
		if (isZero) {
			return true;
		}

		for (auto &i : data) {
			if (0 != i) {
				return false;
			}
		}

		isZero = true;
		return true;
	}

	void AddRef() {
		refCount++;
	}

	void DelRef() {
		refCount--;
		if (0 == refCount) {
			delete this;
		}
	}
} *bitMapZero;

class SplitBitMap {
private:
	unsigned int __id__;
	BitMap *bitmaps[4];
	unsigned int count;
	unsigned int refCount;
public:
	SplitBitMap() {
		__id__ = 'PMBS';
		count = 4;
		refCount = 1;
		for (int i = 0; i < 4; ++i) {
			bitmaps[i] = bitMapZero;
			bitMapZero->AddRef();
		}
	}

	SplitBitMap(BitMap *bmp, unsigned int size) {
		__id__ = 'PMBS';
		count = size;
		refCount = 1;
		for (unsigned int i = 0; i < size; ++i) {
			bitmaps[i] = bmp;
			bmp->AddRef();
		}
	}

	void SetBitmap(unsigned int idx, BitMap *newBitmap) {
		bitmaps[idx]->DelRef();
		bitmaps[idx] = newBitmap;
		bitmaps[idx]->AddRef();
	}

	SplitBitMap *Split(unsigned int fIdx, unsigned int cnt) {
		SplitBitMap *ret = new SplitBitMap();

		for (unsigned int i = 0; i < cnt; ++i) {
			ret->SetBitmap(fIdx + i, bitmaps[i]);
		}
		ret->count = cnt;
		return ret;
	}

	void Append(const SplitBitMap *rhs) {
		for (unsigned int i = 0; i < rhs->count; ++i) {
			bitmaps[i + count] = rhs->bitmaps[i];
		}

		count += rhs->count;
	}

	void Append(BitMap *rhs) {
		bitmaps[count] = rhs;
		count++;
	}

	void Prepend(BitMap *rhs) {
		for (unsigned int i = count; i > 0; --i) {
			bitmaps[i] = bitmaps[i - 1];
		}
		bitmaps[0] = rhs;
	}

	BitMap *Consolidate(unsigned int fIdx, unsigned int lIdx) {
		BitMap *ret = bitmaps[lIdx]->Clone();

		for (unsigned int i = fIdx; i < lIdx; ++i) {
			ret->UnionWith(bitmaps[i]);
		}

		return ret;
	}

	void AddRef() {
		refCount++;
	}

	void DelRef() {
		refCount--;
		if (0 == refCount) {
			for (int i = 0; i < 4; ++i) {
				bitmaps[i]->DelRef();
			}

			delete this;
		}
	}
};

class TrackingExecutor : public sym::SymbolicExecutor {
public :
	void *lastCondition[3];
	unsigned int condCount;

	TrackingExecutor(sym::SymbolicEnvironment *e) : SymbolicExecutor(e) {
		condCount = 0;
		lastCondition[0] = lastCondition[1] = lastCondition[2] = nullptr;
	}

	virtual void *CreateVariable(const char *name, rev::DWORD size) {
		BitMap *bmp = new BitMap(varCount);
		bmp->SetBit(atoi(&name[2]));
		return bmp;
	}

	virtual void *MakeConst(rev::DWORD value, rev::DWORD bits) {
		return bitMapZero;
	}

	virtual void *ExtractBits(void *expr, rev::DWORD lsb, rev::DWORD size) {
		unsigned int *id = (unsigned int *)expr;
		SplitBitMap *sbmp;
		BitMap *bmp;

		switch (*id) {
			case 'PMBS' : //split bitmap
				sbmp = (SplitBitMap *)expr;
				return sbmp->Split(lsb >> 3, size >> 3);
			case 'PMTB' : // normal bitmap
				bmp = (BitMap *)expr;
				return new SplitBitMap(bmp, size >> 3);
			default :
				DEBUG_BREAK;
		}
	}

	virtual void *ConcatBits(void *expr1, void *expr2) {
		unsigned int *id1 = (unsigned int *)expr1, *id2 = (unsigned int *)expr2;

		if (('PMBS' == *id1) && ('PMBS' == *id2)) {
			SplitBitMap *sbmp1 = (SplitBitMap *)expr1, *sbmp2 = (SplitBitMap *)expr2;

			sbmp1->Append(sbmp2);
			sbmp2->DelRef(); // maybe

			return sbmp1;
		}

		if (('PMTB' == *id1) && ('PMBS' == *id2)) {
			BitMap *bmp1 = (BitMap *)expr1;
			SplitBitMap *sbmp2 = (SplitBitMap *)expr2;

			sbmp2->Prepend(bmp1);
			return sbmp2;
		}

		if (('PMBS' == *id1) && ('PMTB' == *id2)) {
			SplitBitMap *sbmp1 = (SplitBitMap *)expr1;
			BitMap *bmp2 = (BitMap *)expr2;

			sbmp1->Append(bmp2);
			return sbmp1;
		}

		if (('PMTB' == *id1) && ('PMTB' == *id2)) {
			DEBUG_BREAK;
		}

		DEBUG_BREAK;
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
				if (env->GetFlgValue(flag, isTracked, val, lc)) {
					lastCondition[condCount] = lc;
					condCount++;
				}
			}
		}
	}

	void ResetCond() {
		condCount = 0;
	}

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

		BitMap *bRet = nullptr;
		bool needClone = false;

		for (int i = 0; i < 4; ++i) {
			rev::BOOL isTracked;
			rev::DWORD val;
			void *opVal;
			if (env->GetOperand(i, isTracked, val, opVal)) {
				if (isTracked) {
					unsigned int *id = (unsigned int *)opVal;
					BitMap *bmp = (BitMap *)opVal;
					if ('PMBS' == *id) {
						bmp = ((SplitBitMap *)opVal)->Consolidate(0, 3);
					}

					if (needClone) {
						BitMap *t = bRet->Clone();
						bRet->DelRef();
						bRet = t;
					}

					if (nullptr == bRet) {
						bmp->AddRef();
						bRet = bmp;
					} else {
						bRet->UnionWith(bmp);
					}
				}
			}
		}

		for (int i = 0; i < flagCount; ++i) {
			rev::BOOL isTracked;
			rev::BYTE val;
			void *opVal;
			if (true == env->GetFlgValue(flagList[i], isTracked, val, opVal)) {
				if (isTracked) {
					unsigned int *id = (unsigned int *)opVal;
					BitMap *bmp = (BitMap *)opVal;
					if ('PMBS' == *id) {
						bmp = ((SplitBitMap *)opVal)->Consolidate(0, 3);
					}

					if (needClone) {
						BitMap *t = bRet->Clone();
						bRet->DelRef();
						bRet = t;
					}

					if (nullptr == bRet) {
						bmp->AddRef();
						bRet = bmp;
					}
					else {
						bRet->UnionWith(bmp);
					}
				}
			}
		}

		if ((0 == (instruction->modifiers & RIVER_MODIFIER_EXT)) && (0x70 <= instruction->opCode) && (instruction->opCode < 0x80)) {
			ExecuteJCC(instruction->opCode - 0x70, instruction);
		}

		if ((nullptr != bRet) && (!bRet->IsZero())) {
			for (int i = 0; i < 4; ++i) {
				if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
					// this will leak a lot of memory
					env->SetOperand(i, bRet);
				}
			}

			for (int i = 0; i < flagCount; ++i) {
				if (flagList[i] & instruction->modFlags) {
					// whis will also leak a lot of memory
					env->SetFlgValue(flagList[i], bRet);
				}
			}
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
				bitMapZero = new BitMap(varCount);

				revEnv = NewX86RevtracerEnvironment(ctx, ctrl); //new RevSymbolicEnvironment(ctx, ctrl);
				regEnv = NewX86RegistersEnvironment(revEnv); //new OverlappedRegistersEnvironment();
				executor = new TrackingExecutor(regEnv);
				regEnv->SetExecutor(executor);

				for (unsigned int i = 0; i < varCount; ++i) {
					char vname[8];

					sprintf(vname, "s[%d]", i);

					revEnv->SetSymbolicVariable(vname, (rev::ADDR_TYPE)(&payloadBuffer[0]), 1);
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
