#ifndef __ANNOTATED_TRACER__
#define __ANNOTATED_TRACER__

#include "ezOptionParser.h"

#include "AbstractLog.h"

#include "BitMap.h"

#include "Execution/Execution.h"
#include "CommonCrossPlatform/Common.h"

#include "revtracer/revtracer.h"

namespace at {

class AnnotatedTracer;

class CustomObserver : public ExecutionObserver {
	public :
		FILE *fBlocks;
		bool binOut;

		AbstractLog *aLog;
		AbstractFormat *aFormat;

		AnnotatedTracer *at;

		std::string patchFile;
		ModuleInfo *mInfo;
		int mCount;

		std::string fileName;

		bool ctxInit;

		sym::SymbolicEnvironment *regEnv;
		sym::SymbolicEnvironment *revEnv;
		TrackingExecutor *executor;

		virtual void TerminationNotification(void *ctx);
		unsigned int GetModuleOffset(const std::string &module) const;
		bool PatchLibrary(std::ifstream &fPatch);
		virtual unsigned int ExecutionBegin(void *ctx, void *address);
		virtual unsigned int ExecutionControl(void *ctx, void *address);
		virtual unsigned int ExecutionEnd(void *ctx);
		virtual unsigned int TranslationError(void *ctx, void *address);

		CustomObserver(AnnotatedTracer *at);
		~CustomObserver();
};

struct CorpusItemHeader {
	char fName[60];
	unsigned int size;
};

class AnnotatedTracer {
	public:

		bool batched = false;

		unsigned int varCount = 1;
		ExecutionController *ctrl = NULL;

		typedef int(*PayloadFunc)();
		char *payloadBuff = nullptr;
		PayloadFunc Payload = nullptr;

		BitMap *bitMapZero;

		CustomObserver observer;

		unsigned int ComputeVarCount();

		void SymbolicSetup(rev::SymbolicHandlerFunc symb);

		AnnotatedTracer();
		~AnnotatedTracer();

		int Run(ez::ezOptionParser &opt);
};

} //namespace at


#endif
