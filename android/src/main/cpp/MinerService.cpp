#include <jni.h>
#include <pthread.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <cstdint>

#define STATE_NONE 0
#define STATE_ONSTART 1
#define STATE_RUNNING 2
#define STATE_ONSTOP 3

extern JavaVM *global_jvm;

static JavaVMAttachArgs attachArgs{
    .version = JNI_VERSION_1_6,
    .name = "CpuWorker",
    .group = NULL};

static jmethodID updateSpeed;
static jmethodID updateResult;
static jmethodID updateState;
static jmethodID sendMessageConsole;

//for mining data
static bool mineRunning;
static pthread_mutex_t _mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;
static jobject local_globalRef;
static uint32_t active_worker = 0;
static bool doingjob = false;
static uint32_t port = 80;
static uint32_t thread_use;
static pthread_t *workers = nullptr;
static pthread_attr_t thread_attr; // make attribute for detached pthread

bool MinerService_OnLoad(JNIEnv *env) {
  jclass m_class = env->FindClass ("com/ariasaproject/poolminerlite/MinerService");
  return (m_class &&
    (updateSpeed = env->GetMethodID (m_class, "updateSpeed", "(F)V")) &&
    (updateResult = env->GetMethodID (m_class, "updateResult", "(Z)V")) &&
    (updateState = env->GetMethodID (m_class, "updateState", "(I)V")) &&
    (sendMessageConsole = env->GetMethodID (m_class, "sendMessageConsole", "(ILjava/lang/String;)V"))
  );
  mineRunning = false;
}
void MinerService_OnUnload(JNIEnv *env) {
  env->DeleteGlobalRef (local_globalRef);
  local_globalRef = NULL;
  updateSpeed = NULL;
  updateResult = NULL;
  updateState = NULL;
  sendMessageConsole = NULL;
}


void *doWork(void *P) {
  uint32_t *p = (uint32_t*)P;
  uint32_t nonce = *p;
  delete p;
  pthread_mutex_lock (&_mtx);
  ++active_worker;
  pthread_mutex_unlock (&_mtx);
  for ( ;(nonce + thread_use) > nonce; nonce += thread_use) {
    pthread_mutex_lock (&_mtx);
    bool done = !doingjob;
    pthread_mutex_unlock (&_mtx);
    JNIEnv *env;
    if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
      std::string messageN = "Message from native workers. number " + std::to_string(nonce);
      env->CallVoidMethod (local_globalRef, sendMessageConsole, 0, env->NewStringUTF(messageN.c_str()));
      global_jvm->DetachCurrentThread ();
    }
    if (done) break;
    //here hashing
    sleep(1);
  }
  JNIEnv *env;
  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
    std::string messageN = "Native workers was done for id " + std::to_string(active_worker);
    env->CallVoidMethod (local_globalRef, sendMessageConsole, 0, env->NewStringUTF(messageN.c_str()));
    global_jvm->DetachCurrentThread ();
  }
  pthread_mutex_lock (&_mtx);
  --active_worker;
  pthread_cond_broadcast(&_cond);
  pthread_mutex_unlock (&_mtx);
  return 0;
}

void *prepareToStart(void *) {
  pthread_mutex_lock (&_mtx);
  doingjob = true;
  active_worker = 0;
  pthread_mutex_unlock (&_mtx);
  workers = new pthread_t[thread_use];
  for (uint32_t i = 0; i < thread_use; i++) {
    pthread_create (workers + i, &thread_attr, doWork, (void*)new uint32_t(i));
  }
  JNIEnv *env;
  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
    env->CallVoidMethod (local_globalRef, updateState, STATE_RUNNING);
    global_jvm->DetachCurrentThread ();
  }
  return 0;
}

void *cleanToStop(void *) {
  if (!active_worker || !workers || !doingjob) return 0;
  pthread_mutex_lock (&_mtx);
  doingjob = false;
  while (active_worker > 0)
    pthread_cond_wait (&_cond, &_mtx);
  mineRunning = false;
  pthread_mutex_unlock (&_mtx);
  delete[] workers;
  workers = nullptr;
  JNIEnv *env;
  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
    env->CallVoidMethod (local_globalRef, updateState, STATE_NONE);
    global_jvm->DetachCurrentThread ();
  }
  return 0;
}

#define JNIF(R, M) extern "C" JNIEXPORT R JNICALL Java_com_ariasaproject_poolminerlite_MinerService_##M

JNIF(void, nativeStart) (JNIEnv *env, jobject o, jobjectArray s, jintArray i) {
  jint* integers = env->GetIntArrayElements(i, nullptr);
  port = integers[0];
  thread_use = integers[1];
  env->ReleaseIntArrayElements(i, integers, JNI_ABORT);
  
  jstring jserverName = (jstring)env->GetObjectArrayElement(s, 0);
  const char* serverName = env->GetStringUTFChars(jserverName, 0);
  env->ReleaseStringUTFChars(jserverName, serverName);
  
  jstring jauth_user = (jstring)env->GetObjectArrayElement(s, 1);
  const char* auth_user = env->GetStringUTFChars(jauth_user, 0);
  env->ReleaseStringUTFChars(jauth_user, auth_user);
  
  jstring jauth_pass = (jstring)env->GetObjectArrayElement(s, 2);
  const char* auth_pass = env->GetStringUTFChars(jauth_pass, 0);
  env->ReleaseStringUTFChars(jauth_pass, auth_pass);
  
  if (!local_globalRef)
    local_globalRef = env->NewGlobalRef (o);
  pthread_t starting;
  pthread_attr_init (&thread_attr);
  pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
  pthread_mutex_lock (&_mtx);
  if (pthread_create (&starting, &thread_attr, prepareToStart, 0) != 0) {
    doingjob = false;
    env->CallVoidMethod (o, updateState, STATE_NONE);
  } else {
    mineRunning = true;
  }
  pthread_mutex_unlock (&_mtx);
}
JNIF(jboolean, nativeRunning) (JNIEnv *, jobject) {
  pthread_mutex_lock (&_mtx);
  bool r = mineRunning;
  pthread_mutex_unlock (&_mtx);
  return r;
}
JNIF(void, nativeStop) (JNIEnv *, jobject) {
  //send state for mine was stop
  pthread_t stopping;
  pthread_create (&stopping, &thread_attr, cleanToStop, 0);
  pthread_attr_destroy (&thread_attr);
}


