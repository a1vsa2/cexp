#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <psapi.h>
#include "jni.h"


// log
LOG_LEVEL g_level = INFO;

void mylog(LOG_LEVEL level, const char *format, ...) {

    if (level >= g_level) {
        va_list aptr;
        // initializing argument to the list pointer;
        va_start(aptr, format);
        vprintf(format, aptr); // vsprintf()
        va_end(aptr);
    }
}


typedef jint(JNICALL* Createjvm)(JavaVM**, void**, void*);
/**
 * TODO: which jdk version should start 
 */
unsigned long long loadVMDLL(const char *cp, const char *clsQname) {
    DWORD pid = GetCurrentProcessId();
    printf("start load jvm.dll, current process id: %d\n", pid);
    char *java_home = getenv("JAVA_HOME"); // no need to free this pointer
    if (java_home == NULL) {
        printf("can't get java home\n");
        return 0;
    }
    char last_char = java_home[strlen(java_home) - 1];
    char *jvmdll_rpath = "jre/bin/server/jvm.dll";
    if (last_char != '\\' || last_char != '/') {
        jvmdll_rpath = "/jre/bin/server/jvm.dll";
    }
    char *jvmdll_path = (char*)malloc(strlen(java_home) + strlen(jvmdll_rpath) + 1);
    //strcpy(jvmdll_path, java_home);
    memcpy(jvmdll_path, java_home, strlen(java_home));
    memcpy(jvmdll_path + strlen(java_home), jvmdll_rpath, strlen(jvmdll_rpath) + 1);
    //HMODULE jvm_dll = LoadLibrary(TEXT("d:/dev_tools/java/jdk1.8/jre/bin/server/jvm.dll"));
    //printf("jvm path:%s\n", jvmdll_path);
    HMODULE jvm_dll = LoadLibrary(jvmdll_path);
    free(jvmdll_path);
    if (jvm_dll == NULL) {
        printf("not load jvm.dll\n");
        GetLastError();
        return 0;
    }
    FARPROC initializer = GetProcAddress(jvm_dll, "JNI_CreateJavaVM");
    if (initializer == NULL) {
        FreeLibrary(jvm_dll);
        GetLastError();
        printf("not get lib proc address\n");
        return 0;
    }
    printf("loaded jvm.dll\n");
    Createjvm fc = (Createjvm)initializer;
    JavaVM *jvm;       /* denotes a Java VM */
    JNIEnv *env;       /* pointer to native method interface */
    JavaVMInitArgs vm_args; /* JDK/JRE 6 VM initialization arguments */
    JavaVMOption options[4];
    char cpArgPrefix[] = "-Djava.class.path=";
    char *cpArg = (char*)malloc(strlen(cp) + strlen(cpArgPrefix) + 1);
    memcpy(cpArg, cpArgPrefix, strlen(cpArgPrefix));
    memcpy(cpArg, cp, strlen(cp) + 1); // append '0'
    //options[0].optionString = "-Djava.class.path=d:/ztestfiles/code"; /* user classes */
    options[0].optionString = cpArg;
    options[1].optionString = "-Djava.compiler=NONE";           /* disable JIT */
    options[2].optionString = "-verbose:jni, class";                   /* print JNI-related messages */
    options[3].optionString = "-Djava.library.path=";  /* set native library path */
    vm_args.version = JNI_VERSION_1_6;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = JNI_TRUE;
    jint jrst = fc(&jvm, (void**)&env, &vm_args);
    if (jrst < 0 || jvm == NULL || env == NULL) {
        GetLastError();
        printf("failed to create jvm\n");
        FreeLibrary(jvm_dll);
        return 0;
    } else {
        printf("created jvm successful: %d\n", jrst);
    }
    jclass jcls = (*env)->FindClass(env, clsQname);    
    if ((*env)->ExceptionCheck(env) || jcls == NULL) {
        printf("error occured while find class %s\n", clsQname);
    } else {
        jmethodID mid = (*env)->GetStaticMethodID(env, jcls, "main", "([Ljava/lang/String;)V");
        printf("mid:%d", mid == NULL);
        (*env)->CallStaticIntMethod(env, jcls, mid);
        Sleep(1000*1000);
    }
    GetLastError();
    
    FreeLibrary(jvm_dll);
    return (UINT_PTR)jvm;
}