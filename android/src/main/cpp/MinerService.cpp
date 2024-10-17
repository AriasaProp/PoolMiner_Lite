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

static struct {
	volatile bool running = false;
	volatile bool active = false;
	
	pthread_mutex_t mtx_;
	pthread_cond_t cond_;
	
	volatile jint java_state_req = -1;
	
	std::vector<std::pair<jbyte, std::string>> queued;
	
	pthread_t connection;
	pthread_t logging;
} thread_params;


void nativeStart(JNIEnv *, jobject , jobjectArray, jintArray);
jboolean nativeRunning(JNIEnv *, jobject);
void nativeStop(JNIEnv *, jobject);

bool MinerService_OnLoad (JNIEnv *env) {
  jclass m_class = env->FindClass ("com/ariasaproject/poolminerlite/MinerService");
  // consoleItem = env->FindClass("com/ariasaproject/poolminerlite/ConsoleItem");
  if (!m_class /*|| !consoleItem*/) [[unlikely]] return false;

  // Register your class' native methods.
  static const JNINativeMethod methods[] = {
    {"nativeStart", "([Ljava/lang/String;[I)V", reinterpret_cast<void*>(nativeStart)},
    {"nativeRunning", "()Z", reinterpret_cast<void*>(nativeRunning)},
    {"nativeStop", "()V", reinterpret_cast<void*>(nativeStop)},
  };
  if (env->RegisterNatives(m_class, methods, 3) != JNI_OK) [[unlikely]] return false;
  updateSpeed = env->GetMethodID (m_class, "updateSpeed", "(F)V");
  updateResult = env->GetMethodID (m_class, "updateResult", "(Z)V");
  updateState = env->GetMethodID (m_class, "updateState", "(I)V");
  sendMessageConsole = env->GetMethodID (m_class, "sendMessageConsole", "(BLjava/lang/String;)V");
  // consoleItemConstructor = env->GetMethodID(consoleItem, "<init>", "(ILjava/lang/String;Ljava/lang/String;)V");
  if (!updateSpeed || !updateResult || !updateState /* || !consoleItemConstructor*/) [[unlikely]] return false;
  
  pthread_mutex_init(&thread_params.mtx_, NULL);
  pthread_cond_init(&thread_params.cond_, NULL);
  return true;
}
void MinerService_OnUnload (JNIEnv *env) {
	
  
  pthread_mutex_destroy(&thread_params.mtx_);
  pthread_cond_destroy(&thread_params.cond_);
	
	
	jclass m_class = env->FindClass ("com/ariasaproject/poolminerlite/MinerService");
  if (m_class) {
  	env->UnregisterNatives(m_class);
  }
  updateSpeed = NULL;
  updateResult = NULL;
  updateState = NULL;
  sendMessageConsole = NULL;
}

// 5 kBytes ~> 40 kBit
#define MAX_MESSAGE 5000

struct connectData {
	uint32_t thread_use;
  struct sockaddr_in server_addr;
  char *auth_user;
  char *auth_pass;
};

static char buffer[MAX_MESSAGE];
static void *connect (void *p) {
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
	    // send subscribe & authorize
    	tries = 0;
    	miner::msg_send_subs_auth(buffer, dat->auth_user, dat->auth_pass);
      for (size_t length = strlen(buffer), s; length > 0;) {
		    s = send (sockfd, buffer, length, 0);
		    if (s <= 0) {
      		if (++tries > MAX_ATTEMPTS_TRY) throw "Sending subscribe is always failed!";
		      continue;
		    }
		    length -= s;
		    memmove(buffer, buffer + s, length);
      }
      /*
	    // rcv subscribe & authorize prove 
	    do {
        if (recv (sockfd, buffer, MAX_MESSAGE, 0) <= 0) {
          if (++tries > MAX_ATTEMPTS_TRY) throw "failed to receive message socket!.";
          sleep (1);
          continue;
        }
      } while ();
      */
	    // guest running state
	    pthread_mutex_lock (&thread_params.mtx_);
		  thread_params.java_state_req = STATE_RUNNING;
		  pthread_cond_broadcast(&thread_params.cond_);
		  pthread_mutex_unlock (&thread_params.mtx_);
  
	    // loop update data from server
	    tries = 0;
	    {
	      bool loop = true;
	      while (loop) {
	        pthread_mutex_lock (&thread_params.mtx_);
	        loop = thread_params.active;
	        pthread_mutex_unlock (&thread_params.mtx_);
	        if (recv (sockfd, buffer, MAX_MESSAGE, 0) <= 0) {
	          if (++tries > MAX_ATTEMPTS_TRY) throw "failed to receive message socket!.";
	          sleep (1);
	        } else {
	        	tries = 0;
          	std::string rp = miner::parsing(buffer);
	          pthread_mutex_lock (&thread_params.mtx_);
					  thread_params.queued.push_back({end_with, rp});
					  pthread_cond_broadcast(&thread_params.cond_);
					  pthread_mutex_unlock (&thread_params.mtx_);
  
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
  pthread_mutex_lock (&thread_params.mtx_);
  thread_params.queued.push_back({end_with, std::string(buffer)});
  thread_params.java_state_req = STATE_NONE;
  pthread_cond_broadcast(&thread_params.cond_);
  pthread_mutex_unlock (&thread_params.mtx_);
  
  miner::clear();

  pthread_join(thread_params.logging, NULL);
  pthread_mutex_lock (&thread_params.mtx_);
  thread_params.running = false;
  pthread_mutex_unlock (&thread_params.mtx_);
  pthread_exit (NULL);
}
static jobject lcl_glb;
static void *logger (void *) {
	bool loop = true;;
	jint java_state_cur = -1, java_state_set = -1;
	std::vector<std::pair<jbyte, std::string>> proc;
  JNIEnv *env;
	while (loop) {
    pthread_mutex_lock (&thread_params.mtx_);
    while (thread_params.queued.empty() || (java_state_cur == thread_params.java_state_req)) {
			pthread_cond_wait(&thread_params.cond_, &thread_params.mtx_);
    }
		proc.insert(proc.end(), thread_params.queued.begin(), thread_params.queued.end());
		thread_params.queued.clear();
		java_state_cur = thread_params.java_state_req;
    loop = thread_params.active;
    pthread_mutex_unlock (&thread_params.mtx_);
    
		if (global_jvm->AttachCurrentThread (&env, &attachArgs) != JNI_OK) [[unlikely]] continue;
		
		if (!proc.empty()) {
		  for (std::pair a : proc) {
		  	env->CallVoidMethod (lcl_glb, sendMessageConsole, a.first, env->NewStringUTF (a.second.c_str()));
		  }
		  proc.clear();
		}
    if (java_state_set != java_state_cur) {
    	java_state_set = java_state_cur;
  		env->CallVoidMethod (lcl_glb, updateState, java_state_cur);
    }
    
    global_jvm->DetachCurrentThread ();
	}
	
	env->DeleteGlobalRef (lcl_glb);
  pthread_exit (NULL);
}

//#define JNIF(R, M) extern "C" JNIEXPORT R JNICALL Java_com_ariasaproject_poolminerlite_MinerService_##M

void nativeStart(JNIEnv *env, jobject o, jobjectArray s, jintArray i) {
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
  lcl_glb = env->NewGlobalRef (o);
  pthread_mutex_lock (&thread_params.mtx_);
  thread_params.running = true;
  thread_params.active = true;
  pthread_create (&thread_params.connection, NULL, connect, (void *)cd);
  pthread_create (&thread_params.logging, NULL, logger, NULL);
  pthread_mutex_unlock (&thread_params.mtx_);
}
jboolean nativeRunning(JNIEnv *, jobject) {
  pthread_mutex_lock (&thread_params.mtx_);
  bool r = thread_params.running; 
  pthread_mutex_unlock (&thread_params.mtx_);
  return r;
}
void nativeStop(JNIEnv *, jobject) {
  // send state for mine was stop
  pthread_mutex_lock (&thread_params.mtx_);
  thread_params.active = false;
  pthread_cond_broadcast(&thread_params.cond_);
  pthread_mutex_unlock (&thread_params.mtx_);
}

