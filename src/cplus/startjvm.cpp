//#include "JavaVMLauncher.h"
#include "jni.h"
#include <Windows.h>
#include <string.h>
#include <iostream>

using namespace std;

typedef jint(JNICALL* JVMCREATEPROC)(JavaVM**, void**, void*);

bool LaunchJavaVM(wchar_t* jvmPath, char* jvmOpts, char* jarPath, char* startClass, char* startMethod, char* startMethodSig) {

	// JavaVM options
	cout << "Loading JavaVM options..." << endl;
	int nOptionCount = 2;
	JavaVMOption vmOptions[2];
	vmOptions[0].optionString = jvmOpts;
	vmOptions[1].optionString = strcat("-Djava.class.path=", jarPath);

	// JavaVM initial args
	cout << "Initializing JavaVM args..." << endl;
	JavaVMInitArgs vmInitArgs;
	vmInitArgs.version = JNI_VERSION_1_6;
	vmInitArgs.options = vmOptions;
	vmInitArgs.nOptions = nOptionCount;
	vmInitArgs.ignoreUnrecognized = JNI_TRUE;

	// Library handle
	cout << "Start to load JavaVM library..." << endl;
	HINSTANCE jvmDLL = LoadLibrary( jvmPath);

	// Load JavaVM library
	if (jvmDLL == NULL) {
		cout << "Failed to load JavaVM library: " + ::GetLastError() << endl;
		return false;
	}
	cout << "JavaVM library successfully loaded." << endl;

	// Initialize Java VM address
	cout << "Start to create JavaVM..." << endl;
	JVMCREATEPROC jvmProcAddress = (JVMCREATEPROC)GetProcAddress(jvmDLL, "JNI_CreateJavaVM");
	if (jvmDLL == NULL) {
		FreeLibrary(jvmDLL);
		cout << "Failed to create JavaVM: " + ::GetLastError() << endl;
		return false;
	}

	// Create Java VM
	JNIEnv* env;
	JavaVM* jvm;
	jint jvmProc = (jvmProcAddress)(&jvm, (void**)&env, &vmInitArgs);
	if (jvmProc < 0 || jvm == NULL || env == NULL) {
		FreeLibrary(jvmDLL);
		cout << "Failed to create JavaVM: " + ::GetLastError() << endl;
		return false;
	}
	cout << "JavaVM successfully created." << endl;

	// Load launcher Java class
	cout << "Start to load launcher Java class..." << endl;
	jclass mainClass = env->FindClass(startClass);
	if (env->ExceptionCheck() == JNI_TRUE || mainClass == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		FreeLibrary(jvmDLL);
		cout << "Failed to load launcher Java class." << endl;
		return false;
	}
	cout << "Launcher Java class successfully loaded." << endl;

	// Load launcher Java method
	cout << "Start to load launcher Java method..." << endl;
	jmethodID methodID = env->GetStaticMethodID(mainClass, startMethod, "([Ljava/lang/String;)V");
	if (env->ExceptionCheck() == JNI_TRUE || methodID == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		FreeLibrary(jvmDLL);
		cout << "Failed to load launcher Java method." << endl;
		return false;
	}
	cout << "Launcher Java method successfully loaded." << endl;

	// Run launcher method
	cout << "Start to run Java executable file..." << endl;
	env->CallStaticVoidMethod(mainClass, methodID, NULL);
	cout << "Java executable file successfully stopped." << endl;

	// Destroy JavaVM
	cout << "Start to destroy JavaVM..." << endl;
	jvm->DestroyJavaVM();
	cout << "JavaVM successfully destroyed." << endl;
	return true;

}