#include "ezOptionParser.h"

#include "AddressSanitizer.h"
#include "TraceParser.h"

int main(int argc, const char *argv[]) {
	ez::ezOptionParser opt;

	opt.overview = "River Sanitizer";
	opt.syntax = "river.sanitizer [OPTIONS]";

	opt.add("", 0, 0, 0, "Enable address sanitizer", "--address");
	opt.add("", 0, 0, 0, "Enable memory sanitizer", "--memory");
	opt.add("", 0, 1, 0, "Read binary input from file", "-i", "--input-file");

	opt.add("", 0, 0, 0, "Display usage instructions", "-h", "--help", "--usage");

	opt.parse(argc, argv);

	if (opt.isSet("-h")) {
		std::string usage;
		opt.getUsage(usage);
		printf("%s", usage.c_str());
		return 0;
	}

	FILE *input = stdin;

	if (opt.isSet("-i")) {
		std::string fName;

		opt.get("-i")->getString(fName);
		input = fopen(fName.c_str(), "r");

		if (input == NULL) {
			return -1;
		}
	}

	TraceParser *parser = new TraceParser();

	if (!parser->Parse(input)) {
		fclose(input);
		return -1;
	}

	AbstractSanitizer *sanitizer;
	if (opt.isSet("--address")) {
		sanitizer = new AddressSanitizer();
	} else if (opt.isSet("--memory")) {
		//handle memory sanitizer
	} else {
		printf("Error: Specify sanitization type: [--address | --memory]\n");
		fclose(input);
		return -1;
	}

	Z3Handler *handler;
   	parser->GetHandler(handler);
	((AddressSanitizer *)sanitizer)->setZ3Handler(handler);

	bool ret = true;
	int index = 0;
	while (ret) {
		struct AddressAssertion *addrAssertion;
		ret = parser->GetAddressAssertion(index++, addrAssertion);
		if (ret) {
			((AddressSanitizer *)sanitizer)->sanitize(*addrAssertion);
		}
	}

	fclose(input);

	return 0;
}

