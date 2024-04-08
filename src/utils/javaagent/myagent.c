
#include <stdio.h>
#include <stdlib.h>
#include <jvmti.h>

#include <string.h>

// JNIEXPORT- marks the function into the shared lib as exportable so it will be included in the function table, 
//              and thus JNI can find it
// JNICALL – combined with JNIEXPORT, it ensures that our methods are available for the JNI framework
// JNIEnv – a structure containing methods that we can use our native code to access Java elements
// JavaVM – a structure that lets us manipulate a running JVM (or even start a new one) adding threads 
//          to it, destroying it, etc...

// ############## https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html
// ############## https://docs.oracle.com/javase/8/docs/platform/jvmti/jvmti.html
// ############## .../share/demo/jvmti/heapTracker

#define _STRING(s) #s
#define STRING(s) _STRING(s)
#ifdef _WIN32
#define snprintf(buffer, count, format, ...) _snprintf_s(buffer, count, _TRUNCATE, format, ##__VA_ARGS__)
#endif

typedef struct {
    /* JVMTI Environment */
    jvmtiEnv      *jvmti;
    /* State of the VM flags */
    jboolean       vmStarted;
    jboolean       vmInitialized;
    jboolean       vmDead;
    /* Options */
    int            maxDump;
    /* Data access Lock */
    jrawMonitorID  lock;
    /* Counter on classes where BCI has been applied */
    jint           ccount;
    
} GlobalAgentData;

static GlobalAgentData *gdata;


void fatal_error(const char * format, ...) {
    va_list ap;
    va_start(ap, format);
    (void)vfprintf(stderr, format, ap);
    (void)fflush(stderr);
    va_end(ap);
    exit(3);
}

void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if ( errnum != JVMTI_ERROR_NONE ) {
        char *errnum_str;
        errnum_str = NULL;
        (void)(*jvmti)->GetErrorName(jvmti, errnum, &errnum_str);
        fatal_error("ERROR: JVMTI: %d(%s): %s\n", errnum,
                (errnum_str==NULL?"Unknown":errnum_str),
                (str==NULL?"":str));
    }
}

/* Enter a critical section by doing a JVMTI Raw Monitor Enter */
static void
enterCriticalSection(jvmtiEnv *jvmti)
{
    jvmtiError error;

    error = (*jvmti)->RawMonitorEnter(jvmti, gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot enter with raw monitor");
}

/* Exit a critical section by doing a JVMTI Raw Monitor Exit */
static void
exitCriticalSection(jvmtiEnv *jvmti)
{
    jvmtiError error;

    error = (*jvmti)->RawMonitorExit(jvmti, gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot exit with raw monitor");
}

/*  Callback method **/

static void JNICALL
cbVMStart(jvmtiEnv *jvmti, JNIEnv *env) {
    // register native methods
    // static JNINativeMethod methods[1];
    printf("VM Start event.\n");
    // jclass kclass;
    // kclass = (*env)->FindClass(env, "");
    // jmethodID jmid = (*env)->GetMethodID(env, kclass, "getName", "()Ljava/lang/String;");
    // jstring jclsName = (*env)->CallObjectMethod(env, kclass, jmid);
    // char* clsname = (*env)->GetStringUTFChars(env, jclsName, JNI_FALSE);
    // (*env)->ReleaseStringUTFChars(env, jclsName, clsname);
}

void cb_VMInit(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread) {
    
    jvmtiThreadInfo ti;
    memset(&ti, 0, sizeof(ti));
    (*jvmti_env)->GetThreadInfo(jvmti_env, thread, &ti);
    printf("VM Init, %s\n", ti.name);
}

void cb_VMDeath(jvmtiEnv *jvmti_env, JNIEnv *jni_env) {
    printf("VM Death\n");
}

void cb_Exception(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method, 
    jlocation location, jobject exception, jmethodID catch_method, jlocation catch_location) {

    printf("Exception occurred:\n");
}
void cb_ObjectAlloc(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, 
    jobject object, jclass object_klass, jlong size){

}

/* Must set can_redefine_classes */
static void JNICALL
cbClassFileLoadHook(jvmtiEnv *jvmti, JNIEnv* env,
                jclass class_being_redefined, jobject loader,
                const char* name, jobject protection_domain,
                jint class_data_len, const unsigned char* class_data,
                jint* new_class_data_len, unsigned char** new_class_data)
{
    printf("class file load hook %s\n", name);
}

/*  Callback method end **/

JNIEXPORT jint JNICALL 
Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("enter agent_onload\n"); // VM is not initialized yet
    static GlobalAgentData data;
    jvmtiEnv              *jvmti;
    jvmtiError             error;
    jint                   res;

    jvmtiCapabilities      capabilities;
    jvmtiEventCallbacks    callbacks;


    /* Setup initial global agent data area
     *   Use of static/extern data should be handled carefully here.
     *   We need to make sure that we are able to cleanup after ourselves
     *     so anything allocated in this library needs to be freed in
     *     the Agent_OnUnload() function.
     */
    (void)memset((void*)&data, 0, sizeof(data));
    gdata = &data;

    /* First thing we need to do is get the jvmtiEnv* or JVMTI environment */
    res = (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK) {
        /* This means that the VM was unable to obtain this version of the
         *   JVMTI interface, this is a fatal error.
         */
        fatal_error("ERROR: Unable to access JVMTI Version 1_2 (0x%x),"
                " is your JDK a 5.0 or newer version?"
                " JNIEnv's GetEnv() returned %d\n",
               JVMTI_VERSION_1_2, res);
    }

    /* Here we save the jvmtiEnv* for Agent_OnUnload(). */
    gdata->jvmti = jvmti;

    /* Parse any options supplied on java command line */
    printf("agent option: %s\n", options);

    /* Immediately after getting the jvmtiEnv* we need to ask for the
     *   capabilities this agent will need.
     */
    (void)memset(&capabilities,0, sizeof(capabilities));
    capabilities.can_generate_all_class_hook_events = 1;
    capabilities.can_tag_objects  = 1;
    capabilities.can_redefine_classes = 1;
    error = (*jvmti)->AddCapabilities(jvmti, &capabilities);
    check_jvmti_error(jvmti, error, "Unable to get necessary JVMTI capabilities.");

    /* Next we need to provide the pointers to the callback functions to
     *   to this jvmtiEnv*
     */
    (void)memset(&callbacks,0, sizeof(callbacks));

    callbacks.VMStart           = &cbVMStart; /* JVMTI_EVENT_VM_START */    
    callbacks.VMInit            = &cb_VMInit; /* JVMTI_EVENT_VM_INIT */
    callbacks.VMDeath           = &cb_VMDeath; /* JVMTI_EVENT_VM_DEATH */  
    //callbacks.ClassFileLoadHook = &cb_ClassFileLoadHook; 
    error = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, (jint)sizeof(callbacks));
    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks");

    /* At first the only initial events we are interested in are VM
     *   initialization, VM death, and Class File Loads.
     *   Once the VM is initialized we will request more events.
     */
    error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                          JVMTI_EVENT_VM_START, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");
    error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                          JVMTI_EVENT_VM_INIT, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");
    error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                          JVMTI_EVENT_VM_DEATH, (jthread)NULL);
    // check_jvmti_error(jvmti, error, "Cannot set event notification");
    // error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
    //                       JVMTI_EVENT_OBJECT_FREE, (jthread)NULL);
    // check_jvmti_error(jvmti, error, "Cannot set event notification");
    // error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
    //                       JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
    // check_jvmti_error(jvmti, error, "Cannot set event notification");
    error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
                          JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");

    /* Here we create a raw monitor for our use in this agent to
     *   protect critical sections of code.
     */
    error = (*jvmti)->CreateRawMonitor(jvmti, "agent data", &(gdata->lock));
    check_jvmti_error(jvmti, error, "Cannot create raw monitor");

    /* We return JNI_OK to signify success */
    return JNI_OK;
}


JNIEXPORT jint JNICALL 
Agent_OnAttach(JavaVM* vm, char *options, void *reserved) {
    printf("enter Agent_OnAttach\n");
    return JNI_OK;
}

JNIEXPORT void JNICALL 
Agent_OnUnload(JavaVM *vm) {
    printf("enter Agent_OnUnload\n");
}