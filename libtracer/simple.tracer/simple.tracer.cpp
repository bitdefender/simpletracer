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
	st->globalLog.Log("Process starting\n");
	st->ctrl->GetModules(mInfo, mCount);

	if (HandlePatchLibrary() < 0) {
		return EXECUTION_TERMINATE;
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
			bbInfo.branchType,
			bbInfo.branchInstruction
			);

	return EXECUTION_ADVANCE;
}

unsigned int CustomObserver::ExecutionEnd(void *ctx) {
	if (st->batched) {
		CorpusItemHeader header;
		if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
				(header.size == fread(st->payloadBuff, 1, header.size, stdin))) {
			st->globalLog.Log("Using %s as input file.\n", header.fName);

			aFormat->WriteTestName(header.fName);
			return EXECUTION_RESTART;
		}

		return EXECUTION_TERMINATE;
	} else if (st->flowMode) {
		st->globalLog.Log("On flow mode restart\n");

		FlowOpCode nextOp = E_NEXTOP_TASK;
		fread(&nextOp, sizeof(char), 1, stdin);
		st->globalLog.Log("NNext op code %d\n" , nextOp);

		if (nextOp == 0) {
			st->globalLog.Log("Stopping\n");
			st->flowMode = false;
			return EXECUTION_TERMINATE;
		} else if (nextOp == 1){
			st->globalLog.Log("### Executing a new task\n");
			st->ReadFromFile(stdin, st->payloadInputSizePerTask);
			st->globalLog.Log("###Finished executing the task\n");
			aFormat->OnExecutionBegin(nullptr);
			return EXECUTION_RESTART;
		}

		st->globalLog.Log("invalid next op value !! probably the data stream is corrupted\n");
		return EXECUTION_TERMINATE;

	} else {
		return EXECUTION_TERMINATE;
	}
}

unsigned int CustomObserver::TranslationError(void *ctx, void *address) {
	st->globalLog.Log("Error issued at address %p\n", address);
	auto direction = ExecutionEnd(ctx);
	if (direction == EXECUTION_RESTART) {
		st->globalLog.Log("Restarting after issue\n");
	}
	st->globalLog.Log("Translation error. Exiting ...\n");
	exit(1);
	return direction;
}

CustomObserver::CustomObserver(SimpleTracer *st) {
	this->st = st;
}

CustomObserver::~CustomObserver() {}

// Read a payload buffer from a file and execute
void SimpleTracer::ReadFromFile(FILE* inputFile, int sizeToRead) {
	const bool readUntilEOF = sizeToRead == -1;
	char *buff = payloadBuff;
	unsigned int bSize = MAX_BUFF;
	do {
		if (readUntilEOF)
		{
			fgets(buff, bSize, inputFile);

			while (*buff) {
				buff++;
				bSize--;
			}
		}
		else
		{
			fread(buff, sizeof(char), sizeToRead, inputFile);
		}
	} while (!feof(inputFile) && readUntilEOF);
}

int SimpleTracer::Run( ez::ezOptionParser &opt) {
	uint32_t executionType = EXECUTION_INPROCESS;

	// Don't write logs before this check
	if (!opt.isSet("--disableLogs")) {
		globalLog.EnableLog();
	}

	const bool writeLogOnFile = opt.isSet("--writeLogOnFile");
	if (writeLogOnFile) {
		globalLog.SetLoggingToFile("log.txt");
	}

	if (opt.isSet("--extern")) {
		executionType = EXECUTION_EXTERNAL;
	}

	ctrl = NewExecutionController(executionType);

	std::string fModule;
	opt.get("-p")->getString(fModule);
	globalLog.Log("Using payload %s\n", fModule.c_str());
	if (executionType == EXECUTION_EXTERNAL)
		globalLog.Log("Starting %s tracing on module %s\n",
				((executionType == EXECUTION_EXTERNAL) ? "extern" : "internal"),
				fModule);

	if (executionType == EXECUTION_INPROCESS) {
		LIB_T hModule = GET_LIB_HANDLER2(fModule.c_str());
		if (nullptr == hModule) {
			std::cerr << "Payload not found" << std::endl;
			return 0;
		}

		payloadBuff = (char *)LOAD_PROC(hModule, "payloadBuffer");
		PayloadHandler = (PayloadHandlerFunc)LOAD_PROC(hModule, "Payload");

		if ((nullptr == payloadBuff) || (nullptr == PayloadHandler)) {
			std::cerr << "Payload imports not found" << std::endl;
			return 0;
		}
	}

	if (opt.isSet("--binlog")) {
		observer.binOut = true;
	}

	const bool isBinBuffered  = opt.isSet("--binbuffered");

	std::string fName;
	opt.get("-o")->getString(fName);
	globalLog.Log("Writing %s output to %s\n",
			(observer.binOut ? "binary" : "text"), fName.c_str());

	FileLog *flog = new FileLog();
	flog->SetLogFileName(fName.c_str());
	observer.aLog = flog;

	if (observer.binOut) {
		observer.aFormat = new BinFormat(observer.aLog, isBinBuffered);
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
		ctrl->SetPath(std::wstring(ws));
	}

	ctrl->SetExecutionFeatures(0);

	ctrl->SetExecutionObserver(&observer);

	if (opt.isSet("--batch")) {
		batched = true;
		FILE *f = freopen(NULL, "rb", stdin);
		if (f == nullptr) {
			std::cerr << "Stdin reopen failed" << std::endl;
		}

		while (!feof(stdin)) {
			CorpusItemHeader header;
			if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
					(header.size == fread(payloadBuff, 1, header.size, stdin))) {
				globalLog.Log("Using %s as input file.\n", header.fName);

				observer.fileName = header.fName;

				ctrl->Execute();
				ctrl->WaitForTermination();
			}
		}

	} else if (opt.isSet("--flow")) {
		flowMode = true;
		freopen(NULL, "rb", stdin);
		// Input protocol [payload input Size  |  [task_op | payload - if taskOp == E_NEXT_OP_TASK]+ ]
		// Expecting the size of each task first then the stream of tasks

		fread(&payloadInputSizePerTask, sizeof(unsigned int), 1, stdin);
		globalLog.Log ("size of payload %u \n", payloadInputSizePerTask);

		// flowMode may be modified in ExecutionEnd
		while (!feof(stdin) && flowMode) {
			FlowOpCode nextOp = E_NEXTOP_TASK;
			fread(&nextOp, sizeof(char), 1, stdin);
			globalLog.Log("NNext op code %d\n" , nextOp);

			if (nextOp == 0) {
				globalLog.Log("Stopping\n");
				break;
			}
			else if (nextOp == 1){
				globalLog.Log("### Executing a new task\n");

				ReadFromFile(stdin, payloadInputSizePerTask);

				observer.fileName = "stdin";

				ctrl->Execute();
				ctrl->WaitForTermination();

				globalLog.Log("###Finished executing the task\n");
			}
			else{
				globalLog.Log("invalid next op value !! probably the data stream is corrupted\n");
				break;
			}
		}
	} else {
		ReadFromFile(stdin);
		ctrl->Execute();
		ctrl->WaitForTermination();
	}

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	return 0;
}

SimpleTracer::SimpleTracer() :
  observer(this), globalLog() {}

SimpleTracer::~SimpleTracer() {}

} //namespace st
