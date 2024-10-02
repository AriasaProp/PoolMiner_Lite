#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <jni.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

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

// for mining data
#define STATUS_MINERUNNING 1
#define STATUS_DOINGJOB 2
static int status_flags = 0;

static pthread_mutex_t _mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;
static jobject local_globalRef;
static uint32_t active_worker = 0;
static uint32_t thread_use;

int MinerService_OnLoad (JNIEnv *env) {
  jclass m_class = (*env)->FindClass (env, "com/ariasaproject/poolminerlite/MinerService");
  if (!m_class) return 0;
  updateSpeed = (*env)->GetMethodID (env, m_class, "updateSpeed", "(F)V");
  updateResult = (*env)->GetMethodID (env, m_class, "updateResult", "(Z)V");
  updateState = (*env)->GetMethodID (env, m_class, "updateState", "(I)V");
  sendMessageConsole = (*env)->GetMethodID (env, m_class, "sendMessageConsole", "(ILjava/lang/String;)V");
  if (!updateSpeed || !updateResult || !updateState) return 0;
  status_flags &= ~STATUS_MINERUNNING;
  return 1;
}
void MinerService_OnUnload (JNIEnv *env) {
  (*env)->DeleteGlobalRef (env, local_globalRef);
  local_globalRef = NULL;
  updateSpeed = NULL;
  updateResult = NULL;
  updateState = NULL;
  sendMessageConsole = NULL;
}

#define MAX_MESSAGE 8000
#define CONNECT_MACHINE "PoolMiner-Lite"

typedef struct {
	int sockfd;
  uint32_t port;
  char *server;
  char *auth;
} connectData;

static char buffer[MAX_MESSAGE];
int startConnect_connect(connectData *dat) {
	size_t tries = 0;
  struct hostent *host = gethostbyname (dat->server);
  if (!host) {
  	strcpy(buffer, "host name was invalid");
  	return 0;
  }
  if(dat->sockfd = socket (AF_INET, SOCK_STREAM, 0) == -1) { 
  	strcpy(buffer, "socket has error!");
  	return 0;
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons (dat->port);
  server_addr.sin_addr = *((struct in_addr *)host->h_addr);
  // try connect socket
  do {
    if (connect (dat->sockfd, (struct sockaddr *)&server_addr, sizeof (server_addr)) == 0) break;
    ++tries;
    sleep (1);
  } while (tries < MAX_ATTEMPTS_TRY);
  if (tries >= MAX_ATTEMPTS_TRY) {
  	strcpy(buffer, "Connection tries is always failed!");
  	return 0;
	}
	return 1;
}
// try subscribe & authorize
int startConnect_subscribe_authorize(connectData *dat) {
  size_t tries = 0;
  strcpy (buffer, "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"");
  strcat (buffer, CONNECT_MACHINE);
  strcat (buffer, "\"]}\n{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"");
  strcat (buffer, dat->auth);
  strcat (buffer, "\"]}");
  for (int length = strlen(buffer), s; (tries < MAX_ATTEMPTS_TRY) && (length > 0);) {
    s = send (dat->sockfd, buffer, length, 0);
    if (s <= 0) {
      ++tries;
      continue;
    }
    length -= s;
    memmove(buffer, buffer + s, length);
  }
  if (tries >= MAX_ATTEMPTS_TRY) {
  	strcpy(buffer, "Sending subscribe & authorize is always failed!");
  	return 0;
  }
  JNIEnv *env;
  if ((*global_jvm)->AttachCurrentThread (global_jvm, &env, &attachArgs) == JNI_OK) {
    (*env)->CallVoidMethod (env, local_globalRef, updateState, STATE_RUNNING);
    (*env)->CallVoidMethod (env, local_globalRef, sendMessageConsole, 2, (*env)->NewStringUTF (env, "subscribe & authorize success"));
    (*global_jvm)->DetachCurrentThread (global_jvm);
  } else {
  	strcpy(buffer, "Comunicate during subscribe & authorize is failed!");
  	return 0;
  }
  return 1;
}
// loop update data from server
int startConnect_loopRequest(connectData *dat) {
	size_t tries = 0;
  int loop = STATUS_DOINGJOB;
  while (loop & STATUS_DOINGJOB) {
    pthread_mutex_lock (&_mtx);
    loop = status_flags;
    pthread_mutex_unlock (&_mtx);
    JNIEnv *env;
    if (recv (dat->sockfd, buffer, MAX_MESSAGE, 0) <= 0) {
      if (++tries > MAX_ATTEMPTS_TRY) {
      	strcpy(buffer, "failed to receive message socket!.");
      	return 0;
      }
      if ((*global_jvm)->AttachCurrentThread (global_jvm, &env, &attachArgs) == JNI_OK) {
        (*env)->CallVoidMethod (env, local_globalRef, sendMessageConsole, 4, (*env)->NewStringUTF (env, "Connection Failed, Try connect again after a sec!"));
        (*global_jvm)->DetachCurrentThread (global_jvm);
      }
      sleep (1);
    } else {
      if (tries) tries = 0;
      if ((*global_jvm)->AttachCurrentThread (global_jvm, &env, &attachArgs) == JNI_OK) {
        (*env)->CallVoidMethod (env, local_globalRef, sendMessageConsole, 0, (*env)->NewStringUTF (env, buffer));
        (*global_jvm)->DetachCurrentThread (global_jvm);
      }
    }
  }
  return 1;
}


void *startConnect (void *p) {
  pthread_mutex_lock (&_mtx);
  ++active_worker;
  pthread_mutex_unlock (&_mtx);

  connectData *dat = (connectData *)p;
  dat->sockfd = -1;
  // try make an connection
  if (
	  	!startConnect_connect(dat) ||
  		!startConnect_subscribe_authorize(dat) ||
  		!startConnect_loopRequest(dat)
  	) {
	    JNIEnv *env;
	    if ((*global_jvm)->AttachCurrentThread (global_jvm, &env, &attachArgs) == JNI_OK) {
	    	memmove(buffer + 27, buffer, strlen(buffer) + 1);
	    	memcpy(buffer, "Connection Failed, because  ", 27);
	      (*env)->CallVoidMethod (env, local_globalRef, sendMessageConsole, 4, (*env)->NewStringUTF (env, _msg));
	      (*global_jvm)->DetachCurrentThread (global_jvm);
	    }
  	}
  if (dat->sockfd != -1) {
    close (dat->sockfd);
  }
  free(dat->server);
  free(dat->auth);
  free(dat);
  // set state mining to none
  {
    JNIEnv *env;
	  if ((*global_jvm)->AttachCurrentThread (global_jvm, &env, &attachArgs) == JNI_OK) {
	    (*env)->CallVoidMethod (env, local_globalRef, updateState, STATE_NONE);
		  (*global_jvm)->DetachCurrentThread (global_jvm);
	  }
  }

  pthread_mutex_lock (&_mtx);
  --active_worker;
  pthread_cond_broadcast (&_cond);
  pthread_mutex_unlock (&_mtx);
  pthread_exit (NULL);
}

#define JNIF(R, M) JNIEXPORT R JNICALL Java_com_ariasaproject_poolminerlite_MinerService_##M

JNIF (void, nativeStart)
(JNIEnv *env, jobject o, jobjectArray s, jintArray i) {
  connectData *cd = malloc(sizeof(connectData));
  {
    jint *integers = (*env)->GetIntArrayElements (env, i, nullptr);
    cd->port = integers[0];
    thread_use = integers[1];
    (*env)->ReleaseIntArrayElements (env, i, integers, JNI_ABORT);

    jstring jserverName = (jstring)(*env)->GetObjectArrayElement (env, s, 0);
    cd->server = malloc((*env)->GetStringUTFLength (env, jserverName));
    const char *serverName = (*env)->GetStringUTFChars (env, jserverName, JNI_FALSE);
    strcpy (cd->server, serverName);
    (*env)->ReleaseStringUTFChars (env, jserverName, serverName);

    jstring jauth_user = (jstring)(*env)->GetObjectArrayElement (env, s, 1);
    jstring jauth_pass = (jstring)(*env)->GetObjectArrayElement (env, s, 2);
    cd->auth = malloc((*env)->GetStringUTFLength (env, jauth_user) + 3 + (*env)->GetStringUTFLength (env, jauth_pass));
    const char *auth_user = (*env)->GetStringUTFChars (env, jauth_user, JNI_FALSE);
    const char *auth_pass = (*env)->GetStringUTFChars (env, jauth_pass, JNI_FALSE);
    strcpy (cd->auth, auth_user);
    strcat (cd->auth, "\",\"");
    strcat (cd->auth, auth_pass);
    (*env)->ReleaseStringUTFChars (env, jauth_user, auth_user);
    (*env)->ReleaseStringUTFChars (env, jauth_pass, auth_pass);
  }
  if (!local_globalRef)
    local_globalRef = (*env)->NewGlobalRef (env, o);
  pthread_t starting;
  pthread_attr_t thread_attr;
  pthread_attr_init (&thread_attr);
  pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
  pthread_mutex_lock (&_mtx);
  active_worker = 0;
  if (pthread_create (&starting, &thread_attr, startConnect, (void *)cd) != 0) {
    status_flags &= ~STATUS_DOINGJOB;
    (*env)->CallVoidMethod (env, o, updateState, STATE_NONE);
  } else {
    status_flags = STATUS_MINERUNNING | STATUS_DOINGJOB;
  }
  pthread_mutex_unlock (&_mtx);
  pthread_attr_destroy (&thread_attr);
}
JNIF (jboolean, nativeRunning)
(JNIEnv *, jobject) {
  pthread_mutex_lock (&_mtx);
  int r = status_flags;
  pthread_mutex_unlock (&_mtx);
  return status_flags & STATUS_MINERUNNING;
}
void *toStopBackground (void *) {
  if (active_worker && (status_flags & STATUS_DOINGJOB)) {
    pthread_mutex_lock (&_mtx);
    status_flags &= ~STATUS_DOINGJOB;
    while (active_worker > 0)
      pthread_cond_wait (&_cond, &_mtx);
    status_flags &= ~STATUS_MINERUNNING;
    pthread_mutex_unlock (&_mtx);
  }
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
