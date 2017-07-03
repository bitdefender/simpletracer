#include "ezOptionParser.h"

#include "BinFormat.h"
#include "TextFormat.h"

#include "FileLog.h"

#include "simple.tracer.h"

#ifdef _WIN32
#include <Windows.h>
#define GET_LIB_HANDLER2(libname) LoadLibraryA((libname))
#else
#define GET_LIB_HANDLER2(libname) dlopen((libname), RTLD_LAZY)
#endif


#define MAX_BUFF 4096

namespace st {

unsigned int CustomObserver::ExecutionBegin(void *ctx, void *address) {
	printf("Process starting\n");
	st->ctrl->GetModules(mInfo, mCount);

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

	rev::BasicBlockInfo bbInfo;
	st->ctrl->GetLastBasicBlockInfo(ctx, &bbInfo);

	aFormat->WriteTestName(fileName.c_str());

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionControl(void *ctx, void *address) {
	rev::BasicBlockInfo bbInfo;
	st->ctrl->GetLastBasicBlockInfo(ctx, &bbInfo);

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


	aFormat->WriteBasicBlock(
			(-1 == foundModule) ? unkmod : mInfo[foundModule].Name,
			offset,
			bbInfo.cost,
			0
			);

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionEnd(void *ctx) {
	if (st->batched) {
		CorpusItemHeader header;
		if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
				(header.size == fread(st->payloadBuff, 1, header.size, stdin))) {
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

CustomObserver::CustomObserver(SimpleTracer *st) {
  this->st = st;
}

CustomObserver::~CustomObserver() {}


int SimpleTracer::Run( ez::ezOptionParser &opt) {
	uint32_t executionType = EXECUTION_INPROCESS;

	if (opt.isSet("--extern")) {
		executionType = EXECUTION_EXTERNAL;
	}

	ctrl = NewExecutionController(executionType);

	std::string fModule;
	opt.get("-p")->getString(fModule);
	std::cout << "Using payload " << fModule << std::endl;
	if (executionType == EXECUTION_EXTERNAL)
		std::cout << "Starting " << ((executionType == EXECUTION_EXTERNAL) ? "extern" : "internal") << " tracing on module " << fModule << "\n";

	if (executionType == EXECUTION_INPROCESS) {
		LIB_T hModule = GET_LIB_HANDLER2(fModule.c_str());
		if (nullptr == hModule) {
			std::cout << "Payload not found" << std::endl;
			return 0;
		}

		payloadBuff = (char *)LOAD_PROC(hModule, "payloadBuffer");
		PayloadHandler = (PayloadHandlerFunc)LOAD_PROC(hModule, "Payload");

		if ((nullptr == payloadBuff) || (nullptr == PayloadHandler)) {
			std::cout << "Payload imports not found" << std::endl;
			return 0;
		}
	}

	if (opt.isSet("--binlog")) {
		observer.binOut = true;
	}

	std::string fName;
	opt.get("-o")->getString(fName);
	std::cout << "Writing " << (observer.binOut ? "binary" : "text") << " output to " << fName << std::endl;

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
		wchar_t ws[4096];
		std::mbstowcs(ws, fModule.c_str(), fModule.size() + 1);
		std::cout << "Converted module name [" << fModule << "] to wstring [";
		std::wcout << std::wstring(ws) << "]\n";
		ctrl->SetPath(std::wstring(ws));
	}

	ctrl->SetExecutionFeatures(0);

	ctrl->SetExecutionObserver(&observer);

	if (opt.isSet("--batch")) {
		batched = true;
		freopen(NULL, "rb", stdin);

		while (!feof(stdin)) {
			CorpusItemHeader header;
			if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
					(header.size == fread(payloadBuff, 1, header.size, stdin))) {
				std::cout << "Using " << header.fName << " as input file." << std::endl;

				observer.fileName = header.fName;

				ctrl->Execute();
				ctrl->WaitForTermination();
			}
		}

	} else {
		char *buff = payloadBuff;
		unsigned int bSize = MAX_BUFF;
		do {
			fgets(buff, bSize, stdin);
			while (*buff) {
				buff++;
				bSize--;
			}
		} while (!feof(stdin));

		observer.fileName = "stdin";

		ctrl->Execute();
		ctrl->WaitForTermination();
	}

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	return 0;
}

SimpleTracer::SimpleTracer() :
  observer(this) {}

SimpleTracer::~SimpleTracer() {}

} //namespace st
