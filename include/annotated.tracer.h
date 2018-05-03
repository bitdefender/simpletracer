#ifndef __ANNOTATED_TRACER__
#define __ANNOTATED_TRACER__

#include "ezOptionParser.h"

#include "BitMap.h"

#include "revtracer/revtracer.h"

#include "basic.observer.h"

#define Z3_TRACKING 0x1
#define TAINTED_INDEX_TRACKING 0x0

namespace at {

typedef int(*PayloadHandlerFunc)();

class AnnotatedTracer;

class CustomObserver : public BasicObserver {
	public:
		AnnotatedTracer *at;

		sym::SymbolicEnvironment *regEnv;
		sym::SymbolicEnvironment *revEnv;
		sym::SymbolicExecutor *executor;

		virtual unsigned int ExecutionBegin(void *ctx, void *entryPoint);
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

		bool batched;
		uint8_t trackingMode;
		unsigned int varCount;

		ExecutionController *ctrl;

		unsigned char *payloadBuff;
		PayloadHandlerFunc PayloadHandler;

		rev::SymbolicHandlerFunc symb;
		CustomObserver observer;

		unsigned int ComputeVarCount();
		int Run(ez::ezOptionParser &opt);
		void SetSymbolicHandler(rev::SymbolicHandlerFunc symb);

		AnnotatedTracer();
		~AnnotatedTracer();
};

} //namespace at


#endif
