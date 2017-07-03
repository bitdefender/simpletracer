#ifndef __ANNOTATED_TRACER__
#define __ANNOTATED_TRACER__

#include "ezOptionParser.h"

#include "BitMap.h"

#include "revtracer/revtracer.h"

#include "basic.observer.h"

namespace at {

typedef int(*PayloadHandlerFunc)();

class AnnotatedTracer;

class CustomObserver : public BasicObserver {
  public:
		AnnotatedTracer *at;

		sym::SymbolicEnvironment *regEnv;
		sym::SymbolicEnvironment *revEnv;
		TrackingExecutor *executor;

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

		bool batched;
		unsigned int varCount;

		ExecutionController *ctrl;

		char *payloadBuff;
		PayloadHandlerFunc PayloadHandler;

		BitMap *bitMapZero;

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
