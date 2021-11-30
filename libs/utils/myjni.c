#include <stdio.h>
//#include "jni.h"
#include "JNITest.h"
#include "utils.h"
#include "string.h"
#include <stdlib.h>

/**
JNIEXPORT- marks the function into the shared lib as exportable so it will be included in the
  function table, and thus JNI can find it
JNICALL – combined with JNIEXPORT, it ensures that our methods are available for the JNI framework
JNIEnv – a structure containing methods that we can use our native code to access Java elements
JavaVM – a structure that lets us manipulate a running JVM (or even start a new one) adding threads 
  to it, destroying it, etc…
 */


// gcc -I include -I d:/dev_tools/java/jdk1.8/include/ 
// -Id:/dev_tools/java/jdk1.8/include/win32 -shared myjni.c local_utils.c   -o myjni.dll
void printSomething() {
    printf("print something\n");
}
void exceptionCheck(JNIEnv *env, int lineNum) {
  if ((*env)->ExceptionCheck(env)) {
    printf("jni exception occurred, lines: %d", lineNum);
    (*env)->ExceptionDescribe(env);
    return;
  }
}

JNIEXPORT void JNICALL Java_com_xwh_jni_JNITest_sayHello
  (JNIEnv *env, jobject jobj) {
    jclass jobjcls = (*env)->GetObjectClass(env, jobj);
    exceptionCheck(env, __LINE__);
    jclass clscls = (*env)->GetObjectClass(env, jobjcls);
    jmethodID mid_getname = (*env)->GetMethodID(env, clscls, "getName", "()Ljava/lang/String;");
    exceptionCheck(env, __LINE__);
    jstring clsName = (*env)->CallObjectMethod(env, jobjcls, mid_getname);
    exceptionCheck(env, __LINE__);
    const char *cn = (*env)->GetStringUTFChars(env, clsName, JNI_FALSE);
    printf("clasName: %s\n", cn);
    (*env)->ReleaseStringUTFChars(env, clsName, cn);
    jmethodID mId = (*env)->GetMethodID(env, jobjcls, "javaSayHello", "(Ljava/lang/String;)V");
    jstring jstr = (*env)->NewStringUTF(env, "c language");
    (*env)->CallObjectMethod(env, jobj, mId, jstr);
    printSomething();
}

/*
 * Class:     com_xwh_jni_JNITest
 * Method:    getInt
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_xwh_jni_JNITest_getInt
  (JNIEnv *env, jobject jobj , jint num)
{
  // LOGI("received num from java: %d\n", num); print too slower
  printf("received num from java :%d\n", num);
  return num * num;
}

/*
 * Class:     com_xwh_jni_JNITest
 * Method:    getLong
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_xwh_jni_JNITest_getLong
  (JNIEnv *env, jobject jobj, jlong num) 
{
  LOGI("received long num from java: %d", num);
  return num + 1;
}

/*
 * Class:     com_xwh_jni_JNITest
 * Method:    getString
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_xwh_jni_JNITest_getString
  (JNIEnv *env, jobject jobj, jstring jstr) // if getString is static java method, jobject is a class object
{
  const char *ostr = (*env)->GetStringUTFChars(env, jstr, JNI_FALSE);
  LOGI("received string from java: %s\n", ostr);
  char isCopy = 0;
  //const char *str = (*env)->GetStringUTFChars(env, jstr, &isCopy);
  // int len = strlen(str);
  char amsg[] = "returned from c.";
  //char *fmsg = (char*)malloc(len + sizeof(amsg));
  //strcpy()
  //strcat(str, ",append by c.");
  // LOGI("return string is : %s", fmsg);
  jclass jcls = (*env)->GetObjectClass(env, jobj);
  (*env)->DeleteLocalRef(env, jcls);

  if ((*env)->ExceptionCheck(env)) {
    LOGI("error occurred\n");
    jclass thr = (*env)->FindClass(env, "java/lang/Exception");
    (*env)->ThrowNew(env, thr, "jni native error");
  }
  return (*env)->NewStringUTF(env, amsg);
}

/*
 * Class:     com_xwh_jni_JNITest
 * Method:    getBoolean
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_com_xwh_jni_JNITest_getBoolean
  (JNIEnv *env, jobject jobj, jboolean jbool)
{

}

/*
 * Class:     com_xwh_jni_JNITest
 * Method:    getIntArray
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL Java_com_xwh_jni_JNITest_getIntArray
  (JNIEnv *env, jobject jobj, jintArray jarr) {

}

JNIEXPORT jlong JNICALL Java_com_xwh_jni_JNITest_startJVM() {
  return 0;

}


static JNINativeMethod methods[] = {
  {"getString", "(Ljava/lang/String;)Ljava/lang/String;", (void*)&Java_com_xwh_jni_JNITest_getString}, // dynamic register with static name?
  //{"sayHello", "()V", (void*)&printSomething},
};


/*
 * Class:     com_xwh_jni_JNITest
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_xwh_jni_JNITest_registerNatives
  (JNIEnv *env, jclass jcls) {
    (*env)->RegisterNatives(env, jcls, methods, sizeof(methods) / sizeof(methods[0]));
}

const char* CLS_NAME = "com/xwh/jni/JNITest";
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) { // dynamic register, would be invoked after loadlibrary invoked
  JNIEnv *env;
  if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_8) != JNI_OK) { return JNI_ERR; }

  jclass jcls = (*env)->FindClass(env, CLS_NAME);
  if (jcls == NULL) {
    printf("class not found: %s\n", CLS_NAME);
    return JNI_ERR;
  }
  if ((*env)->RegisterNatives(env, jcls, methods, sizeof(methods) / sizeof(methods[0]))) {
    printf("not register native methods\n");
    return JNI_ERR;
  }
  return JNI_VERSION_1_8;

}