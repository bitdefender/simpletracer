#include "ezOptionParser.h"
#include "BinLog.h"
#include "Execution/Execution.h"
#include "CommonCrossPlatform/Common.h"
#include <stdarg.h>

#ifdef _WIN32
#include <Windows.h>
#define LIB_EXT ".dll"
#else
#define LIB_EXT ".so"
#endif

ExecutionController *ctrl = NULL;
bool writeEverythingOnASingleline = false;
bool writeLogOnFile = false;
FILE* logFile = stdout; 

static bool gIsLogEnabled = true;
void Log( const char * format, ... )
{
  if (!gIsLogEnabled)
  	return;
  
  static const int buffSz = 2048;
  static char buffer[buffSz];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer,buffSz,format, args);
  //perror (buffer);
  va_end (args);

  fprintf(logFile, "%s", buffer);
  fflush(logFile);
}


class CustomObserver : public ExecutionObserver {
public :
	FILE *fBlocks;
	bool binOut;

	BinLogWriter *blw;

	std::string patchFile;
	ModuleInfo *mInfo;
	int mCount;

	std::string fileName;

	virtual void TerminationNotification(void *ctx) {
		Log("Process Terminated\n");
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
		Log("Process starting\n");
		ctrl->GetModules(mInfo, mCount);

		if (!patchFile.empty()) {
			std::ifstream fPatch;

			fPatch.open(patchFile);
			
			if (!fPatch.good()) {
				Log("Patch file not found\n");
				return EXECUTION_TERMINATE;
			}

			PatchLibrary(fPatch);
			fPatch.close();

		}

		if (binOut) {
			blw->ExecutionBegin();
			//blw->WriteEntry((-1 == foundModule) ? unkmod : mInfo[foundModule].Name, offset, ctrl->GetLastBasicBlockCost(ctx));

		}
		else {
			fprintf(fBlocks, "## %s ##", fileName.c_str());
			fprintf(fBlocks, writeEverythingOnASingleline ? "&" : "\n");
		}

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


		if (binOut) {
			blw->WriteEntry((-1 == foundModule) ? unkmod : mInfo[foundModule].Name, offset, bbInfo.cost);
		} else {
			fprintf(fBlocks, "%-15s + %08lX (%4ld)",
				(-1 == foundModule) ? unkmod : mInfo[foundModule].Name,
				(DWORD)offset,
				bbInfo.cost
			);

			fprintf(fBlocks, writeEverythingOnASingleline ? "&" : "\n");
		}

		return EXECUTION_ADVANCE;
	}

	virtual unsigned int ExecutionEnd(void *ctx) {		
		if (binOut)
		{
			blw->ExecutionEnd();
		}
		else
		{
			fprintf(fBlocks, "\n"); // Writing a character that means for other processes the end of output from test. If this breaks someone else functionality then do this only for "--flow" option
			fflush(fBlocks);
		}
		return EXECUTION_TERMINATE;
	}

	CustomObserver() {
		fBlocks = nullptr;
		binOut = false;

		blw = nullptr;	
	}

	~CustomObserver() {
		if (nullptr != blw) {
			delete blw;
			blw = nullptr;
		}
	}
} observer;

#define MAX_BUFF 4096
typedef int(*PayloadHandlerFunc)();
char *payloadBuff = nullptr;
PayloadHandlerFunc PayloadHandler = nullptr;

struct CorpusItemHeader {
	char fName[60];
	unsigned int size;
};

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

	ctrl->Execute();
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
	if (opt.isSet("--disableLogs")) {
		gIsLogEnabled = false;
	}

	if (opt.isSet("--writeLogOnFile")) {
		logFile = fopen("log.txt", "w");
	}

	uint32_t executionType = EXECUTION_INPROCESS;

	if (opt.isSet("--inprocess") && opt.isSet("--extern")) {
		Log("Conflicting options --inprocess and --extern\n");
		return 0;
	}

	if (opt.isSet("--extern")) {
		executionType = EXECUTION_EXTERNAL;
	}

	if (executionType != EXECUTION_INPROCESS) {
		Log("Only inprocess execution supported for now! Sorry!\n");
		return 0;
	}

	ctrl = NewExecutionController(executionType);

	if (opt.isSet("-h")) {
		std::string usage;
		opt.getUsage(usage);
		Log("%s", usage.c_str());
		return 0;
	}

	std::string fModule;
	opt.get("-p")->getString(fModule);
	Log("Using payload %s\n", fModule.c_str());
	lib_t hModule = GET_LIB_HANDLER(fModule.c_str());
	if (nullptr == hModule) {
		Log("PayloadHandler not found\n");
		return 0;
	}

	payloadBuff = (char *)LOAD_PROC(hModule, "payloadBuffer");
	PayloadHandler = (PayloadHandlerFunc)LOAD_PROC(hModule, "Payload");

	if ((nullptr == payloadBuff) || (nullptr == PayloadHandler)) {
		Log("PayloadHandler imports not found\n");
		return 0;
	}		

	const bool isBinaryOutput = opt.isSet("--binlog");
	const bool isBinBuffered  = opt.isSet("--binbuffered");

	if (isBinaryOutput) {
		observer.binOut = true;
	}

	std::string fName;
	opt.get("-o")->getString(fName);
	if (strcmp(fName.c_str(), "stdout") == 0)
	{
		Log("Writing to stdout\n");
		observer.fBlocks = stdout;
	}
	else
	{
		Log("Writing %s output to %s. Is buffered ? %d\n", observer.binOut ? "binary" : "text", fName.c_str(), (int)isBinBuffered);
		FOPEN(observer.fBlocks, fName.c_str(), observer.binOut ? "wb" : "wt");
	}

	if (isBinaryOutput) {
		observer.blw = new BinLogWriter(observer.fBlocks, isBinBuffered);
	}
		
	if (opt.isSet("-m")) {
		opt.get("-m")->getString(observer.patchFile);
	}

	ctrl->SetEntryPoint((void*)PayloadHandler);
	
	ctrl->SetExecutionFeatures(0);

	ctrl->SetExecutionObserver(&observer);
	
	if (opt.isSet("--batch")) {
		freopen(NULL, "rb", stdin);
		
		while (!feof(stdin)) {
			CorpusItemHeader header;
			if ((1 == fread(&header, sizeof(header), 1, stdin)) &&
					(header.size == fread(payloadBuff, 1, header.size, stdin))) {
				Log("Using %s as input file\n", header.fName);

				observer.fileName = header.fName;

				ctrl->Execute();
				ctrl->WaitForTermination();
			}
		}

	} 
	else if (opt.isSet("--flow")) {
		// Input protocol [payload input Size  |  [task_op | payload - if taskOp == E_NEXT_OP_TASK]+ ]
		// Expecting the size of each task first then the stream of tasks
		//printf("starging \n");
		writeEverythingOnASingleline = true;

		unsigned int payloadInputSizePerTask = -1;
		fread(&payloadInputSizePerTask, sizeof(payloadInputSizePerTask), 1, stdin);				
		Log ("size of payload %d \n", payloadInputSizePerTask);

		enum FlowOpCode
		{
			E_NEXTOP_CLOSE,
			E_NEXTOP_TASK
		};

		FlowOpCode nextOp = E_NEXTOP_TASK;
		while(true)
		{
			fread(&nextOp, sizeof(char), 1, stdin);
			Log("NNext op code %d\n" , nextOp);
			
			if (nextOp == 0) {
				Log("Stopping");
				break;
			} 
			else if (nextOp == 1){
				Log("executing a new task\n");
				ReadFromFileAndExecute(stdin, payloadInputSizePerTask);
			}
			else{
				Log("invalid next op value !! probably the data stream is corrupted\n");
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

	fclose(observer.fBlocks);
	return 0;
}
