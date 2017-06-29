#include "ezOptionParser.h"

#include "simple.tracer.h"

// annotated tracer header dependencies
#include "SymbolicEnvironment/Environment.h"
#include "TrackingExecutor.h"
#include "annotated.tracer.h"

#include "Execution/Execution.h"
#include "CommonCrossPlatform/Common.h"

at::AnnotatedTracer *AT = nullptr;

void __stdcall SymbolicHandler(void *ctx, void *offset, void *addr) {
	RiverInstruction *instr = (RiverInstruction *)addr;

	if (AT != nullptr) {
		AT->observer.regEnv->SetCurrentInstruction(instr, offset);
		AT->observer.executor->Execute(instr);
	}
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
			"",
			false,
			0,
			0,
			"River generated annotated traces using taint analysis.",
			"--annotated"
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

	if (opt.isSet("--inprocess") && opt.isSet("--extern")) {
		std::cout << "Conflicting options --inprocess and --extern" << std::endl;
		return 0;
	}

	if (opt.isSet("-h")) {
		std::string usage;
		opt.getUsage(usage);
		std::cout << usage;
		return 0;
	}

	if (opt.isSet("--annotated")) {
		AT = new at::AnnotatedTracer();
		AT->SymbolicSetup(SymbolicHandler);
		return AT->Run(opt);
	} else {
		st::SimpleTracer *ST = new st::SimpleTracer();
		return ST->Run(opt);
	}
}
