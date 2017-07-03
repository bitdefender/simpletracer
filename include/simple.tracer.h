#ifndef __SIMPLE_TRACER__
#define __SIMPLE_TRACER__

#include "basic.observer.h"

namespace st {

class SimpleTracer;

class CustomObserver : public BasicObserver{
	public :
		SimpleTracer *st;

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

} //namespace st

#endif
