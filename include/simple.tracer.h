#ifndef __SIMPLE_TRACER__
#define __SIMPLE_TRACER__

#include "Logger.h"
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

enum FlowOpCode
{
	E_NEXTOP_CLOSE,
	E_NEXTOP_TASK
};



class SimpleTracer {
	public:
		ExecutionController *ctrl = NULL;
		bool batched = false;
		bool flowMode = false;

		unsigned int payloadInputSizePerTask = 0;

		typedef int(*PayloadHandlerFunc)();
		char *payloadBuff = nullptr;
		PayloadHandlerFunc PayloadHandler = nullptr;

		CustomObserver observer;

		Logger globalLog;

		void ReadFromFile(FILE* inputFile, int sizeToRead=-1);
		int Run(ez::ezOptionParser &opt);

		SimpleTracer();
		~SimpleTracer();
};

} //namespace st

#endif
