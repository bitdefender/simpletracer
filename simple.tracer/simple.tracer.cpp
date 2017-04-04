#include "ezOptionParser.h"

#include "AbstractLog.h"

#include "BinFormat.h"
#include "TextFormat.h"

#include "FileLog.h"

#include "Execution/Execution.h"
#include "CommonCrossPlatform/Common.h"

#ifdef _WIN32
#include <Windows.h>
#define LIB_EXT ".dll"
#define GET_LIB_HANDLER2(libname) LoadLibraryA((libname))
#else
#define LIB_EXT ".so"
#define GET_LIB_HANDLER2(libname) dlopen((libname), RTLD_LAZY)
#endif

#include "Logger.h"

ExecutionController *ctrl = NULL;
Logger gLog;

bool batched = false;
bool flowMode = false;

struct CorpusItemHeader {
	char fName[60];
	unsigned int size;
};

#define MAX_BUFF 4096
typedef int(*PayloadHandlerFunc)();
char *payloadBuff = nullptr;
PayloadHandlerFunc PayloadHandler = nullptr;

class CustomObserver : public ExecutionObserver {
public :
	bool binOut;

	AbstractLog *aLog;
	AbstractFormat *aFormat;

	std::string patchFile;
	ModuleInfo *mInfo;
	int mCount;

	std::string fileName;

	virtual void TerminationNotification(void *ctx) {
		gLog.Log("Process Terminated\n");
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
		ctrl->GetModules(mInfo, mCount);

		if (!patchFile.empty()) {
			std::ifstream fPatch;

			fPatch.open(patchFile);
			
			if (!fPatch.good()) {
				gLog.Log("Patch file not found\n");
				return EXECUTION_TERMINATE;
			}

			PatchLibrary(fPatch);
			fPatch.close();

		}

		rev::BasicBlockInfo bbInfo;
		ctrl->GetLastBasicBlockInfo(ctx, &bbInfo);

		aFormat->OnExecutionBegin(fileName.c_str());

		return EXECUTION_ADVANCE;
	}

	virtual unsigned int ExecutionControl(void *ctx, void *address) {
		rev::BasicBlockInfo bbInfo;
		ctrl->GetLastBasicBlockInfo(ctx, &bbInfo);
		
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

	virtual unsigned int ExecutionEnd(void *ctx) {
		aFormat->OnExecutionEnd();

		if (batched) {
			CorpusItemHeader header;
			if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
				(header.size == fread(payloadBuff, 1, header.size, stdin))) {
				gLog.Log("Using %s as input file\n", header.fName);

				aFormat->OnExecutionBegin(header.fName);
				return EXECUTION_RESTART;
			}

			return EXECUTION_TERMINATE;
		} 
		else if (flowMode) {
				//gLog.Log("On flow mode restart\n");
				aFormat->OnExecutionBegin(nullptr);
				return EXECUTION_RESTART;
		}
		else {
			return EXECUTION_TERMINATE;
		}
	}

	virtual unsigned int TranslationError(void *ctx, void *address) {
		printf("Error issued at address %p\n", address);
		auto direction = ExecutionEnd(ctx);
		if (direction == EXECUTION_RESTART) {
			printf("Restarting after issue\n");
		}
		printf("Translation error. Exiting ...\n");
		exit(1);
		return direction;
	}

	CustomObserver() {
		binOut = false;
	}

	~CustomObserver() {
		delete aLog;
		delete aFormat;
	}
} observer;


// Read a payload buffer from a file and execute
void ReadFromFileAndExecute(FILE* inputFile, int sizeToRead = -1)
{
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

	observer.fileName = "stdin";

	//gLog.Log("executing..\n");
	ctrl->Execute();
	//gLog.Log("waiting for termination..\n");
	ctrl->WaitForTermination();
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
		"Disable logs",
		"--disableLogs"
	);

	opt.add(
		"",
		0,
		0,
		0,
		"write Log On File",
		"--writeLogOnFile"
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
		"",
		false,
		0,
		0,
		"When using binlog, buffer everything before writing the result",
		"--binbuffered"
	);

	opt.add(
		"",
		false,
		0,
		0,
		"Use a corpus file instead of individual inputs.",
		"--batch"
	);

	opt.add(
		"",
		false,
		0,
		0,
		"Use this to create a flow of input payload- trace output. Input is fed from a different process",
		"--flow"
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

	// Don't write logs before this check
	if (!opt.isSet("--disableLogs")) {
		gLog.EnableLog();
	}

	const bool writeLogOnFile = opt.isSet("--writeLogOnFile");
	if (writeLogOnFile) {
		gLog.SetLoggingToFile("log.txt");
	}
	
	uint32_t executionType = EXECUTION_INPROCESS;

	if (opt.isSet("--inprocess") && opt.isSet("--extern")) {
		gLog.Log("Conflicting options --inprocess and --extern\n");
		return 0;
	}

	if (opt.isSet("--extern")) {
		executionType = EXECUTION_EXTERNAL;
	}

	ctrl = NewExecutionController(executionType);

	if (opt.isSet("-h")) {
		std::string usage;
		opt.getUsage(usage);
		gLog.Log("%s", usage.c_str());
		return 0;
	}

	std::string fModule;
	opt.get("-p")->getString(fModule);
	gLog.Log("Using payload %s\n", fModule.c_str());
	if (executionType == EXECUTION_EXTERNAL) {
		gLog.Log("Starting %s tracing on module %s\n", ((executionType == EXECUTION_EXTERNAL) ? "extern" : "internal"), fModule);
	}

	if (executionType == EXECUTION_INPROCESS) {
		lib_t hModule = GET_LIB_HANDLER2(fModule.c_str());
		if (nullptr == hModule) {
			gLog.Log("Payload not found\n");
			return 0;
		}

		payloadBuff = (char *)LOAD_PROC(hModule, "payloadBuffer");
		PayloadHandler = (PayloadHandlerFunc)LOAD_PROC(hModule, "Payload");

		if ((nullptr == payloadBuff) || (nullptr == PayloadHandler)) {
			gLog.Log("PayloadHandler imports not found\n");
			return 0;
		}
	}

	const bool isBinaryOutput = opt.isSet("--binlog");
	const bool isBinBuffered  = opt.isSet("--binbuffered");

	if (isBinaryOutput) {
		observer.binOut = true;
	}

	std::string fName;
	opt.get("-o")->getString(fName);
	
	// Set the file log type and format
	FileLog *flog = new FileLog();
	if (strcmp(fName.c_str(), "stdout") == 0)
	{
		gLog.Log("Writing to stdout\n");
		flog->SetExternalFile(stdout);
	}
	else
	{
		gLog.Log("Writing %s output to %s. Is buffered ? %d\n", observer.binOut ? "binary" : "text", fName.c_str(), (int)isBinBuffered);
		flog->SetLogFileName(fName.c_str());
	}
	
	observer.aLog = flog;

	if (observer.binOut) {
		observer.aFormat = new BinFormat(observer.aLog, isBinBuffered, gLog);
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
		gLog.Log("Converted module name [%s] to wstring [", fModule);
		//std::wcout << std::wstring(ws) << "]\n";
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
				gLog.Log("Using %s as input file\n", header.fName);

				observer.fileName = header.fName;

				ctrl->Execute();
				ctrl->WaitForTermination();
			}
		}

	} 
	else if (opt.isSet("--flow")) {
		flowMode = true;
		// Input protocol [payload input Size  |  [task_op | payload - if taskOp == E_NEXT_OP_TASK]+ ]
		// Expecting the size of each task first then the stream of tasks

		unsigned int payloadInputSizePerTask = -1;
		fread(&payloadInputSizePerTask, sizeof(payloadInputSizePerTask), 1, stdin);				
		gLog.Log ("size of payload %d \n", payloadInputSizePerTask);

		enum FlowOpCode
		{
			E_NEXTOP_CLOSE,
			E_NEXTOP_TASK
		};

		FlowOpCode nextOp = E_NEXTOP_TASK;
		while(true)
		{
			fread(&nextOp, sizeof(char), 1, stdin);
			gLog.Log("NNext op code %d\n" , nextOp);
			
			if (nextOp == 0) {
				gLog.Log("Stopping");
				break;
			} 
			else if (nextOp == 1){
				gLog.Log("### Executing a new task\n");
				ReadFromFileAndExecute(stdin, payloadInputSizePerTask);
				gLog.Log("###Finished executing the task\n");
			}
			else{
				gLog.Log("invalid next op value !! probably the data stream is corrupted\n");
				break;
			}			
		}
	}
	else {
		ReadFromFileAndExecute(stdin);
		ctrl->WaitForTermination();
	}

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	return 0;
}
