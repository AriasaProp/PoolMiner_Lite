#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <jni.h>
#include <netdb.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include "miner_core.hpp"

#define STATE_NONE 0
#define STATE_ONSTART 1
#define STATE_RUNNING 2
#define STATE_ONSTOP 3

#define MAX_ATTEMPTS_TRY 10

extern JavaVM *global_jvm;

static JavaVMAttachArgs attachArgs{
    .version = JNI_VERSION_1_6,
    .name = "CpuWorker",
    .group = NULL};

// static jclass consoleItem;

static jmethodID updateSpeed;
static jmethodID updateResult;
static jmethodID updateState;
static jmethodID sendMessageConsole;
// static jmethodID consoleItemConstructor;

static jobject local_globalRef;

bool MinerService_OnLoad (JNIEnv *env) {
  jclass m_class = env->FindClass ("com/ariasaproject/poolminerlite/MinerService");
  // consoleItem = env->FindClass("com/ariasaproject/poolminerlite/ConsoleItem");
  if (!m_class /*|| !consoleItem*/) return false;
  updateSpeed = env->GetMethodID (m_class, "updateSpeed", "(F)V");
  updateResult = env->GetMethodID (m_class, "updateResult", "(Z)V");
  updateState = env->GetMethodID (m_class, "updateState", "(I)V");
  sendMessageConsole = env->GetMethodID (m_class, "sendMessageConsole", "(BLjava/lang/String;)V");
  // consoleItemConstructor = env->GetMethodID(consoleItem, "<init>", "(ILjava/lang/String;Ljava/lang/String;)V");
  if (!updateSpeed || !updateResult || !updateState /* || !consoleItemConstructor*/) return false;
  return true;
}
void MinerService_OnUnload (JNIEnv *env) {
  env->DeleteGlobalRef (local_globalRef);
  local_globalRef = NULL;
  updateSpeed = NULL;
  updateResult = NULL;
  updateState = NULL;
  sendMessageConsole = NULL;
}

// 5 kBytes ~> 40 kBit
#define MAX_MESSAGE 5000

// for mining global data
static struct {
	bool req_stop;
	uint32_t active_worker;
} gp;
static pthread_mutex_t _mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;

struct connectData {
	uint32_t thread_use;
  struct sockaddr_in server_addr;
  char *auth_user;
  char *auth_pass;
};

char buffer[MAX_MESSAGE];
void *startConnect (void *p) {
  pthread_mutex_lock (&_mtx);
  gp.req_stop = false;
  gp.active_worker = 1;
  pthread_mutex_unlock (&_mtx);
  
  miner::init();

  connectData *dat = (connectData *)p;
	//make socket
	char end_with = 2;
  try {
	  int sockfd = socket (AF_INET, SOCK_STREAM, 0);
	  if (sockfd == -1) throw "socket has error!";
	  try {
	    // check inputs parameter for mining
	    size_t tries = 0;
      // try connect socket
      while (connect (sockfd, (struct sockaddr *)&dat->server_addr, sizeof (dat->server_addr)) != 0) {
      	if (tries++ >= MAX_ATTEMPTS_TRY) throw "Connection tries is always failed!";
        sleep (1);
      }
	    // send subscribe
    	tries = 0;
    	miner::msg_send_subscribe(buffer);
      for (size_t length = strlen(buffer), s; length > 0;) {
		    s = send (sockfd, buffer, length, 0);
		    if (s <= 0) {
      		if (++tries > MAX_ATTEMPTS_TRY) throw "Sending subscribe & authorize is always failed!";
		      continue;
		    }
		    length -= s;
		    memmove(buffer, buffer + s, length);
      }
      
	    // rcv subscribe prove 
	    
	    // try authorize
	    tries = 0;
    	miner::msg_send_auth(buffer, dat->auth_user, dat->auth_pass);
      for (size_t length = strlen(buffer), s; length > 0;) {
		    s = send (sockfd, buffer, length, 0);
		    if (s <= 0) {
      		if (++tries > MAX_ATTEMPTS_TRY) throw "Sending subscribe & authorize is always failed!";
		      continue;
		    }
		    length -= s;
		    memmove(buffer, buffer + s, length);
      }
	    
	    // rcv authorize prove 

	    // loop update data from server
	    tries = 0;
	    {
	      bool loop = true;
	      while (loop) {
	        pthread_mutex_lock (&_mtx);
	        if (gp.req_stop) loop = false;
	        pthread_mutex_unlock (&_mtx);
	        if (recv (sockfd, buffer, MAX_MESSAGE, 0) <= 0) {
	          if (++tries > MAX_ATTEMPTS_TRY) throw "failed to receive message socket!.";
	          sleep (1);
	        } else {
	        	tries = 0;
            JNIEnv *env;
	          if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
	          	std::string rp = miner::parsing(buffer);
	            env->CallVoidMethod (local_globalRef, sendMessageConsole, 0, env->NewStringUTF (rp.c_str()));
	            global_jvm->DetachCurrentThread ();
	          }
	        }
	      }
	    }
    	strcpy(buffer, "ended succesfully");
    	close (sockfd);
	  } catch (const char *er) {
    	close (sockfd);
	  	throw er;
	  }
  } catch (const char *er) {
  	strcpy(buffer, "Connection Failed, because ");
  	strcat(buffer, er);
  	end_with = 4;
  }
  
  delete[] dat->auth_user;
  delete[] dat->auth_pass;
  delete dat;
  // set state mining to none
  {
    JNIEnv *env;
	  while (global_jvm->AttachCurrentThread (&env, &attachArgs) != JNI_OK) {
	  	(void)env;
	  }
    env->CallVoidMethod (local_globalRef, sendMessageConsole, end_with, env->NewStringUTF (buffer));
    env->CallVoidMethod (local_globalRef, updateState, STATE_NONE);
	  global_jvm->DetachCurrentThread ();
  }
  
  miner::clear();

  pthread_mutex_lock (&_mtx);
  --gp.active_worker;
  pthread_cond_broadcast (&_cond);
  pthread_mutex_unlock (&_mtx);
  pthread_exit (NULL);
}

#define JNIF(R, M) extern "C" JNIEXPORT R JNICALL Java_com_ariasaproject_poolminerlite_MinerService_##M

JNIF (void, nativeStart)
(JNIEnv *env, jobject o, jobjectArray s, jintArray i) {
  connectData *cd = new connectData;
  {
  	cd->server_addr.sin_family = AF_INET;
    jint *integers = env->GetIntArrayElements (i, NULL);
	  cd->server_addr.sin_port = htons (integers[0]);
    cd->thread_use = integers[1];
    env->ReleaseIntArrayElements (i, integers, JNI_ABORT);

    jstring jserverName = (jstring)env->GetObjectArrayElement (s, 0);
    const char *serverName = env->GetStringUTFChars (jserverName, JNI_FALSE);
    struct hostent *host = gethostbyname (serverName);
	  if (!host) {
      env->CallVoidMethod (o, sendMessageConsole, 4, env->NewStringUTF ("host name was invalid"));
    	env->CallVoidMethod (o, updateState, STATE_NONE);
	  	return;
	  }
	  cd->server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    env->ReleaseStringUTFChars (jserverName, serverName);

    jstring jauth_user = (jstring)env->GetObjectArrayElement (s, 1);
    cd->auth_user = new char[env->GetStringUTFLength (jauth_user)];
    const char *auth_user = env->GetStringUTFChars (jauth_user, JNI_FALSE);
    strcpy(cd->auth_user, auth_user);
    env->ReleaseStringUTFChars (jauth_user, auth_user);
    
    jstring jauth_pass = (jstring)env->GetObjectArrayElement (s, 2);
    cd->auth_pass = new char[env->GetStringUTFLength (jauth_pass)];
    const char *auth_pass = env->GetStringUTFChars (jauth_pass, JNI_FALSE);
    strcpy(cd->auth_pass, auth_pass);
    env->ReleaseStringUTFChars (jauth_pass, auth_pass);
  }
  if (!local_globalRef)
    local_globalRef = env->NewGlobalRef (o);
  pthread_t starting;
  pthread_attr_t thread_attr;
  pthread_attr_init (&thread_attr);
  pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
  pthread_mutex_lock (&_mtx);
  if (pthread_create (&starting, &thread_attr, startConnect, (void *)cd) != 0) {
    env->CallVoidMethod (o, updateState, STATE_NONE);
  } else {
    env->CallVoidMethod (o, updateState, STATE_RUNNING);
  }
  pthread_mutex_unlock (&_mtx);
  pthread_attr_destroy (&thread_attr);
}
JNIF (jboolean, nativeRunning)
(JNIEnv *, jobject) {
  pthread_mutex_lock (&_mtx);
  bool r = gp.active_worker > 0;
  pthread_mutex_unlock (&_mtx);
  return r;
}
void *toStopBackground (void *) {
  pthread_mutex_lock (&_mtx);
  if (gp.active_worker) {
    gp.req_stop = true;
    while (gp.active_worker > 0)
      pthread_cond_wait (&_cond, &_mtx);
  }
  pthread_mutex_unlock (&_mtx);
  pthread_exit (NULL);
}
JNIF (void, nativeStop)
(JNIEnv *, jobject) {
  // send state for mine was stop
  pthread_t stopping;
  pthread_attr_t thread_attr;
  pthread_attr_init (&thread_attr);
  pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
  pthread_create (&stopping, &thread_attr, toStopBackground, NULL);
  pthread_attr_destroy (&thread_attr);
}

