#include "ezOptionParser.h"
#include "BinLog.h"
#include "Execution/Execution.h"
#include "CommonCrossPlatform/Common.h"

#ifdef _WIN32
#include <Windows.h>
#define LIB_EXT ".dll"
#else
#define LIB_EXT ".so"
#endif

ExecutionController *ctrl = NULL;

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

		}

		if (binOut) {
			//blw->WriteEntry((-1 == foundModule) ? unkmod : mInfo[foundModule].Name, offset, ctrl->GetLastBasicBlockCost(ctx));

		}
		else {
			fprintf(fBlocks, "## %s ##\n", fileName.c_str());
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
			fprintf(fBlocks, "%-15s + %08lX (%4ld)\n",
				(-1 == foundModule) ? unkmod : mInfo[foundModule].Name,
				(DWORD)offset,
				bbInfo.cost
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
		"",
		false,
		0,
		0,
		"Use a corpus file instead of individual inputs.",
		"--batch"
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
		std::cout << "PayloadHandler not found" << std::endl;
		return 0;
	}

	payloadBuff = (char *)LOAD_PROC(hModule, "payloadBuffer");
	PayloadHandler = (PayloadHandlerFunc)LOAD_PROC(hModule, "Payload");

	if ((nullptr == payloadBuff) || (nullptr == PayloadHandler)) {
		std::cout << "PayloadHandler imports not found" << std::endl;
		return 0;
	}

	if (opt.isSet("--binlog")) {
		observer.binOut = true;
	}


	std::string fName;
	opt.get("-o")->getString(fName);
	std::cout << "Writing " << (observer.binOut ? "binary" : "text") << " output to " << fName << std::endl;
	FOPEN(observer.fBlocks, fName.c_str(), observer.binOut ? "wb" : "wt");

	if (observer.binOut) {
		observer.blw = new BinLogWriter(observer.fBlocks);
	}
		
	if (opt.isSet("-m")) {
		opt.get("-m")->getString(observer.patchFile);
	}

	ctrl->SetEntryPoint((void*)PayloadHandler);
	
	ctrl->SetExecutionFeatures(0);

	ctrl->SetExecutionObserver(&observer);

	ctrl->Startup();
	
	if (opt.isSet("--batch")) {
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

	fclose(observer.fBlocks);
	return 0;
}
