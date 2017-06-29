#include "BinFormat.h"
#include "TextFormat.h"

#include "FileLog.h"

#include "SymbolicEnvironment/Environment.h"

#include "TrackingExecutor.h"
#include "annotated.tracer.h"

#ifdef _WIN32
#include <Windows.h>
#define LIB_EXT ".dll"
#else
#define LIB_EXT ".so"
#endif

#define MAX_BUFF 4096

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
	((BitMap *)ref)->AddRef();
}

void DelReference(void *ref) {
	((BitMap *)ref)->DelRef();
}

void CustomObserver::TerminationNotification(void *ctx) {
	printf("Process Terminated\n");
}

unsigned int CustomObserver::GetModuleOffset(const std::string &module) const {
	const char *m = module.c_str();
	for (int i = 0; i < mCount; ++i) {
		if (0 == strcmp(mInfo[i].Name, m)) {
			return mInfo[i].ModuleBase;
		}
	}

	return 0;
}

bool CustomObserver::PatchLibrary(std::ifstream &fPatch) {
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

unsigned int CustomObserver::ExecutionBegin(void *ctx, void *address) {
	printf("Process starting\n");
	at->ctrl->GetModules(mInfo, mCount);

	if (!patchFile.empty()) {
		std::ifstream fPatch;

		fPatch.open(patchFile);

		if (!fPatch.good()) {
			std::cout << "Patch file not found" << std::endl;
			return EXECUTION_TERMINATE;
		}

		PatchLibrary(fPatch);
		fPatch.close();
	}

	if (!ctxInit) {
		at->bitMapZero = new BitMap(at->varCount, 1);

		revEnv = NewX86RevtracerEnvironment(ctx, at->ctrl); //new RevSymbolicEnvironment(ctx, at->ctrl);
		regEnv = NewX86RegistersEnvironment(revEnv); //new OverlappedRegistersEnvironment();
		//TODO: is this legit?
		executor = new TrackingExecutor(regEnv, at);
		regEnv->SetExecutor(executor);
		regEnv->SetReferenceCounting(AddReference, DelReference);

		for (unsigned int i = 0; i < at->varCount; ++i) {
			char vname[8];

			sprintf(vname, "s[%d]", i);

			revEnv->SetSymbolicVariable(vname,
					(rev::ADDR_TYPE)(&at->payloadBuff[i]), 1);
		}

		ctxInit = true;
	}

	aFormat->WriteTestName(fileName.c_str());

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionControl(void *ctx, void *address) {
	rev::BasicBlockInfo bbInfo;
	at->ctrl->GetLastBasicBlockInfo(ctx, &bbInfo);

	const char unkmod[MAX_PATH] = "???";
	unsigned int offset = (DWORD)bbInfo.address;
	int foundModule = -1;

	for (int i = 0; i < mCount; ++i) {
		if ((mInfo[i].ModuleBase <= (DWORD)bbInfo.address) && ((DWORD)bbInfo.address < mInfo[i].ModuleBase + mInfo[i].Size)) {
			offset -= mInfo[i].ModuleBase;
			foundModule = i;
			break;
		}
	}

	if (executor->condCount) {
		for (unsigned int i = 0; i < at->varCount; ++i) {
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

	aFormat->WriteBasicBlock(
			(-1 == foundModule) ? unkmod : mInfo[foundModule].Name,
			offset,
			bbInfo.cost,
			0
			);
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
		//fflush(fBlocks);
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
	fBlocks = nullptr;
	binOut = false;

	ctxInit = false;

	regEnv = nullptr;
	revEnv = nullptr;

	this->at = at;
}

CustomObserver::~CustomObserver() {
	delete aLog;
	delete aFormat;
}

unsigned int AnnotatedTracer::ComputeVarCount() {
	if (payloadBuff == nullptr)
		return 1;

	char *buff = payloadBuff;
	unsigned int bSize = MAX_BUFF;
	do {
		fgets(buff, bSize, stdin);
		while (*buff) {
			buff++;
			bSize--;
		}
	} while (!feof(stdin));

	return buff - payloadBuff - 1;
}

void AnnotatedTracer::SymbolicSetup(rev::SymbolicHandlerFunc symb) {
	varCount = ComputeVarCount();

	if (ctrl == nullptr)
		return;

	ctrl->SetExecutionFeatures(TRACER_FEATURE_SYMBOLIC);
	ctrl->SetSymbolicHandler(symb);
}

AnnotatedTracer::AnnotatedTracer()
	: observer(this)
{ }

AnnotatedTracer::~AnnotatedTracer()
{}
