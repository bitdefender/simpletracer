#ifndef __SIMPLE_TRACER__
#define __SIMPLE_TRACER__

#include "AbstractLog.h"

#include "Execution/Execution.h"
#include "CommonCrossPlatform/Common.h"

#ifdef _WIN32
#define LIB_EXT ".dll"
#else
#define LIB_EXT ".so"
#endif

class SimpleTracer;

class CustomObserver : public ExecutionObserver {
	public :
		bool binOut;

		AbstractLog *aLog;
		AbstractFormat *aFormat;

		std::string patchFile;
		ModuleInfo *mInfo;
		int mCount;

		std::string fileName;

		SimpleTracer *st;

		virtual void TerminationNotification(void *ctx);
		unsigned int GetModuleOffset(const std::string &module) const;
		bool PatchLibrary(std::ifstream &fPatch);
		virtual unsigned int ExecutionBegin(void *ctx, void *address);
		virtual unsigned int ExecutionControl(void *ctx, void *address);
		virtual unsigned int ExecutionEnd(void *ctx);
		virtual unsigned int TranslationError(void *ctx, void *address);

		CustomObserver(SimpleTracer *st);
		~CustomObserver();
};

struct CorpusItemHeader {
	char fName[60];
	unsigned int size;
};

class SimpleTracer {
	public:
		ExecutionController *ctrl = NULL;
		bool batched = false;

		typedef int(*PayloadHandlerFunc)();
		char *payloadBuff = nullptr;
		PayloadHandlerFunc PayloadHandler = nullptr;

		CustomObserver observer;

		int Run(ez::ezOptionParser &opt);

    SimpleTracer();
    ~SimpleTracer();
};

#endif
