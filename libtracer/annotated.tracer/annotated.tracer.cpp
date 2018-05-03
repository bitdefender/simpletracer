#include "BinFormat.h"
#include "TextFormat.h"

#include "FileLog.h"

// annotated tracer dependencies
#include "SymbolicEnvironment/Environment.h"
#include "TrackingExecutor.h"
#include "Z3SymbolicExecutor.h"
#include "annotated.tracer.h"

#include "CommonCrossPlatform/Common.h" //MAX_PAYLOAD_BUF; MAX_PATH

#include "utils.h" //common handlers

#ifdef _WIN32
#include <Windows.h>
#define GET_LIB_HANDLER2(libname) LoadLibraryA((libname))
#else
#define GET_LIB_HANDLER2(libname) dlopen((libname), RTLD_LAZY)

#endif

// setup static values for BitMap and TrackingExecutor

int BitMap::instCount = 0;

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

typedef void __stdcall SH(void *, void *, void *);

void AddReference(void *ref) {
}

void DelReference(void *ref) {
}

namespace at {

unsigned int CustomObserver::ExecutionBegin(void *ctx, void *entryPoint) {
	printf("Process starting\n");
	at->ctrl->GetModules(mInfo, mCount);

	if (HandlePatchLibrary() < 0) {
		return EXECUTION_TERMINATE;
	}

	if (!ctxInit) {
		revEnv = NewX86RevtracerEnvironment(ctx, at->ctrl);
		regEnv = NewX86RegistersEnvironment(revEnv);

		if (at->trackingMode == TAINTED_INDEX_TRACKING) {
			executor = new TrackingExecutor(regEnv, at->varCount, aFormat);
		} else {
			executor = new Z3SymbolicExecutor(regEnv, aFormat);
		}
		executor->SetModuleData(mCount, mInfo);
		regEnv->SetExecutor(executor);
		regEnv->SetReferenceCounting(AddReference, DelReference);

		for (unsigned int i = 0; i < at->varCount; ++i) {
			char vname[8];

			sprintf(vname, "s[%d]", i);

			revEnv->SetSymbolicVariable(vname,
					(rev::ADDR_TYPE)(at->payloadBuff + i), 1);
		}

		ctxInit = true;
	}

	aFormat->WriteTestName(fileName.c_str());
	logEsp = true;

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionControl(void *ctx, void *address) {
	rev::ExecutionRegs regs;
	rev::BasicBlockInfo bbInfo;

	at->ctrl->GetLastBasicBlockInfo(ctx, &bbInfo);

	unsigned int nextSize = 2;
	struct BasicBlockPointer bbp;
	struct BasicBlockPointer bbpNext[nextSize];

	TranslateAddressToBasicBlockPointer(&bbp, (DWORD)bbInfo.address, mCount, mInfo);

	for (unsigned int i = 0; i < nextSize; ++i) {
		TranslateAddressToBasicBlockPointer(bbpNext + i,
				(DWORD)bbInfo.branchNext[i].address, mCount, mInfo);
	}

	if (logEsp) {
		ClearExecutionRegisters(&regs);
		at->ctrl->GetFirstEsp(ctx, regs.esp);
		aFormat->WriteRegisters(regs);
		logEsp = false;
	}

	at->ctrl->GetCurrentRegisters(ctx, &regs);
	struct BasicBlockMeta bbm { bbp, bbInfo.cost, bbInfo.branchType,
			bbInfo.branchInstruction, regs.esp, bbInfo.nInstructions,
			nextSize, bbpNext };
	aFormat->WriteBasicBlock(bbm);

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionEnd(void *ctx) {
	if (at->batched) {
		CorpusItemHeader header;
		if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
				(header.size == fread(at->payloadBuff, 1, header.size, stdin))) {
			std::cout << "Using " << header.fName << " as input file." << std::endl;

			aFormat->WriteTestName(header.fName);
			return EXECUTION_RESTART;
		}

		return EXECUTION_TERMINATE;
	} else {
		return EXECUTION_TERMINATE;
	}
}

unsigned int CustomObserver::TranslationError(void *ctx, void *address) {
	printf("Error issued at address %p\n", address);
	auto direction = ExecutionEnd(ctx);
	if (direction == EXECUTION_RESTART) {
		printf("Restarting after issue\n");
	}
	printf("Translation error. Exiting ...\n");
	exit(1);
	return direction;
}

CustomObserver::CustomObserver(AnnotatedTracer *at) {
	ctxInit = false;

	regEnv = nullptr;
	revEnv = nullptr;
	executor = nullptr;

	this->at = at;
}

CustomObserver::~CustomObserver() {
}

unsigned int AnnotatedTracer::ComputeVarCount() {
	if (payloadBuff == nullptr)
		return 1;

	return ReadFromFile(stdin, payloadBuff, MAX_PAYLOAD_BUF);
}

void AnnotatedTracer::SetSymbolicHandler(rev::SymbolicHandlerFunc symb) {
	this->symb = symb;
}

AnnotatedTracer::AnnotatedTracer()
	: batched(false), varCount(1), ctrl(nullptr), payloadBuff(nullptr),
	PayloadHandler(nullptr), observer(this)
{ }

AnnotatedTracer::~AnnotatedTracer()
{}

int AnnotatedTracer::Run(ez::ezOptionParser &opt) {
	uint32_t executionType = EXECUTION_INPROCESS;

	if (opt.isSet("--extern")) {
		executionType = EXECUTION_EXTERNAL;
	}

	ctrl = NewExecutionController(executionType);

	std::string fModule;
	opt.get("-p")->getString(fModule);
	std::cout << "Using payload " << fModule.c_str() << std::endl;
	if (executionType == EXECUTION_EXTERNAL)
		std::cout << "Starting " << ((executionType == EXECUTION_EXTERNAL) ? "extern" : "internal") << " tracing on module " << fModule << "\n";

	if (executionType == EXECUTION_INPROCESS) {
		LIB_T hModule = GET_LIB_HANDLER2(fModule.c_str());
		if (nullptr == hModule) {
			std::cout << "Payload not found" << std::endl;
			return 0;
		}

		payloadBuff = (unsigned char *)LOAD_PROC(hModule, "payloadBuffer");
		PayloadHandler = (PayloadHandlerFunc)LOAD_PROC(hModule, "Payload");

		if ((nullptr == payloadBuff) || (nullptr == PayloadHandler)) {
			std::cout << "Payload imports not found" << std::endl;
			return 0;
		}
	}


	if (opt.isSet("--binlog")) {
		observer.binOut = true;
	}

	if (opt.isSet("--z3")) {
		trackingMode = Z3_TRACKING;
	} else {
		trackingMode = TAINTED_INDEX_TRACKING;
	}

	std::string fName;
	opt.get("-o")->getString(fName);
	std::cout << "Writing " << (observer.binOut ? "binary" : "text") << " output to " << fName.c_str() << std::endl;

	FileLog *flog = new FileLog();
	flog->SetLogFileName(fName.c_str());
	observer.aLog = flog;

	if (observer.binOut) {
		observer.aFormat = new BinFormat(observer.aLog);
	} else {
		observer.aFormat = new TextFormat(observer.aLog);
	}

	if (opt.isSet("-m")) {
		opt.get("-m")->getString(observer.patchFile);
	}

	if (executionType == EXECUTION_INPROCESS) {
		ctrl->SetEntryPoint((void*)PayloadHandler);
	} else if (executionType == EXECUTION_EXTERNAL) {
		wchar_t ws[MAX_PATH];
		std::mbstowcs(ws, fModule.c_str(), fModule.size() + 1);
		std::cout << "Converted module name [" << fModule << "] to wstring [";
		std::wcout << std::wstring(ws) << "]\n";
		ctrl->SetPath(std::wstring(ws));
	}

	ctrl->SetExecutionFeatures(TRACER_FEATURE_SYMBOLIC);

	ctrl->SetExecutionObserver(&observer);
	ctrl->SetSymbolicHandler(symb);

	if (opt.isSet("--batch")) {
		batched = true;
		FILE *f = freopen(NULL, "rb", stdin);
		if (f == nullptr) {
			std::cout << "stdin freopen failed" << std::endl;
		}

		while (!feof(stdin)) {
			CorpusItemHeader header;
			if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
					(header.size == fread(payloadBuff, 1, header.size, stdin))) {
				std::cout << "Using " << header.fName << " as input file." << std::endl;

				observer.fileName = header.fName;
				varCount = header.size;

				ctrl->Execute();
				ctrl->WaitForTermination();
			}
		}

	} else {
		varCount = ComputeVarCount();

		observer.fileName = "stdin";

		ctrl->Execute();
		ctrl->WaitForTermination();
	}

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	return 0;
}

} // namespace at
