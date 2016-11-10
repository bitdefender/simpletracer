#include <stdio.h>
#include <string.h>

#if defined _WIN32 || defined __CYGWIN__
#include "windirent.h"
#define PATH_SEP "\\"
#else
#include <dirent.h>
#define PATH_SEP "/"
#endif

#define MAX_PATH 4096

struct CorpusItemHeader {
	char fName[60];
	unsigned int size;
};

class CorpusWriter {
private :
	FILE *fCorpus;

public :
	CorpusWriter(const char *cName) {
		fCorpus = fopen(cName, "wt");
	}

	~CorpusWriter() {
		fclose(fCorpus);
	}

	bool AddItem(const char *fName, unsigned char *data, unsigned int size) {
		CorpusItemHeader header;
		memset(&header, 0, sizeof(header));
		strncpy(header.fName, fName, sizeof(header.fName) - 1);
		header.size = size;

		fwrite(&header, sizeof(header), 1, fCorpus);
		fwrite(data, 1, size, fCorpus);
		fflush(fCorpus);

		return true;
	}
};

bool BuildCorpus(const char *dir) {
	DIR *dpdf;
	dirent *epdf;

	dpdf = opendir(dir);
	if (dpdf != NULL) {
		CorpusWriter wr("corpus");
		
		while (epdf = readdir(dpdf)) {
			if (DT_REG == epdf->d_type) {
				char path[MAX_PATH];
				sprintf(path, "%s" PATH_SEP "%s", dir, epdf->d_name);

				printf("Adding: %s\n", path);

				FILE *f = fopen(path, "rt");

				if (nullptr == f) {
					printf("File open error!!\n");
				} else {
					fseek(f, 0L, SEEK_END);
					long sz = ftell(f);
					unsigned char *buff = new unsigned char[sz];

					fseek(f, 0, SEEK_SET);
					fread(buff, 1, sz, f);

					wr.AddItem(epdf->d_name, buff, sz);

					delete buff;
					fclose(f);
				}
			}
			// std::cout << epdf->d_name << std::endl;
		}
	}

	return true;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage %s directory\n\n", argv[0]);
		return 0;
	}

	BuildCorpus(argv[1]);
	return 0;
}
