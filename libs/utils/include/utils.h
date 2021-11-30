#ifndef __UTILS_H__
#define __UTILS_H__


typedef enum LLEVEL{
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL // 5
} LOG_LEVEL;

extern LOG_LEVEL g_level;


void mylog(LOG_LEVEL level, const char *format, ...);
unsigned long long loadVMDLL(const char *cp, const char *clsQname);
int destroyVM(long handler);

#define LOGI(format, ...) \
    do { printf(format, ##__VA_ARGS__); } while(0)

#define LOGE(format, ...) \
    do { fprintf(stderr, format, ##__VA_ARGS__); } while(0)

#define LOGP(level, format, ...) mylog(level, format, ##__VA_ARGS__)

#endif /* __UTILS_H__ */