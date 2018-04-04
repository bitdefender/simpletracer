#ifndef __COMMON_OF__
#define __COMMON_OF__

#ifdef DEBUG
#define PRINT(format, ...) { printf((format), ##__VA_ARGS__); }
#else
#define PRINT(format, ...)
#endif

#endif
