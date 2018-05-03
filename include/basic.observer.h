#ifndef __BASIC_OBSERVER__
#define __BASIC_OBSERVER__

#include "AbstractLog.h"

#include "Execution/Execution.h"
#include "CommonCrossPlatform/Common.h"

class BasicObserver : public ExecutionObserver {
	public :
		bool binOut;

		AbstractLog *aLog;
		AbstractFormat *aFormat;

		std::string patchFile;
		ModuleInfo *mInfo;
		int mCount;

		std::string fileName;

		bool ctxInit;
		bool logEsp;

		virtual void TerminationNotification(void *ctx);
		unsigned int GetModuleOffset(const std::string &module) const;
		bool PatchLibrary(std::ifstream &fPatch);
		virtual unsigned int ExecutionBegin(void *ctx, void *entryPoint) = 0;
		virtual unsigned int ExecutionControl(void *ctx, void *address) = 0;
		virtual unsigned int ExecutionEnd(void *ctx) = 0;
		virtual unsigned int TranslationError(void *ctx, void *address) = 0;

		BasicObserver();
		~BasicObserver();

	protected :
		int HandlePatchLibrary();
};

#endif
