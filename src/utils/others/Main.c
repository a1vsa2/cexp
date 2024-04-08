#include "utils.h"
#include <stdio.h>
#include "jni.h"
#include <windows.h>
#include <psapi.h>

//void createVM();


//https://en.wikipedia.org/wiki/Dynamic_loading

typedef jint(JNICALL* Createjvm)(JavaVM**, void**, void*);

int destroyVM(long handler) {
    JavaVM *jvm = handler;
    jint rst = (*jvm)->DestroyJavaVM(jvm);
}

long loadVMDLL(const char *cp, const char *clsQName) {
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
        printf("not get name address\n");
        return 0;
    }
    printf("loaded jvm.dll\n");
    Createjvm fc = (Createjvm)initializer;
    JavaVM *jvm;       /* denotes a Java VM */
    JNIEnv *env;       /* pointer to native method interface */
    JavaVMInitArgs vm_args; /* JDK/JRE 6 VM initialization arguments */
    JavaVMOption options[4];
    options[0].optionString = "-Djava.class.path=d:/ztestfiles/code"; /* user classes */
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
    jclass jcls = (*env)->FindClass(env, "PostfixIncrement");    
    if ((*env)->ExceptionCheck(env) || jcls == NULL) {
        printf("error occured while find class\n");
    } else {
        jmethodID mid = (*env)->GetStaticMethodID(env, jcls, "main", "([Ljava/lang/String;)V");
        printf("mid:%d", mid == NULL);
        (*env)->CallStaticIntMethod(env, jcls, mid);
        Sleep(1000*1000);
    }
    GetLastError();
    
    FreeLibrary(jvm_dll);
    return jvm;
}

int main(int argc, char argv[]) {
    char msg[] = "message";
    mylog(DEBUG, "test log %s", msg);
    LOGI("use macro for log");
    LOGP(INFO, "macro with local function\n");
    //createVM();
    char *java_home = getenv("JAVA_HOME"); // no need to free this pointer
    printf("java home path:%s\n", java_home);
    loadVMDLL();
}

void listProcess() {
    //EnumProcesses()
}

