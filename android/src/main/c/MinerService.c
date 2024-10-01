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
  if (!m_class) return false;
  updateSpeed = (*env)->GetMethodID (env, m_class, "updateSpeed", "(F)V");
  updateResult = (*env)->GetMethodID (env, m_class, "updateResult", "(Z)V");
  updateState = (*env)->GetMethodID (env, m_class, "updateState", "(I)V");
  sendMessageConsole = (*env)->GetMethodID (env, m_class, "sendMessageConsole", "(ILjava/lang/String;)V");
  if (!updateSpeed || !updateResult || !updateState) return false;
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

#define MAX_MESSAGE 5000
#define CONNECT_MACHINE "PoolMiner-Lite"

typedef struct {
  int sockfd = -1;
  uint32_t port;
  char *server;
  char *auth_user;
  char *auth_pass;
} connectData;

void *doWork (void *p) {
  const uint32_t start = static_cast<uint32_t> ((unsigned long)p);
  uint32_t nonce = start;
  pthread_mutex_lock (&_mtx);
  ++active_worker;
  pthread_mutex_unlock (&_mtx);
  int loop;
  do {
    sleep (1);
    // here hashing
    pthread_mutex_lock (&_mtx);
    loop = status_flags;
    uint32_t nn = nonce + thread_use;
    pthread_mutex_unlock (&_mtx);
    if (nn < nonce) break;
    nonce = nn;
    // do hashing
  } while (loop & STATUS_DOINGJOB);
  pthread_mutex_lock (&_mtx);
  --active_worker;
  pthread_cond_broadcast (&_cond);
  pthread_mutex_unlock (&_mtx);
  pthread_exit (NULL);
}
void *startConnect (void *p) {
  pthread_mutex_lock (&_mtx);
  ++active_worker;
  pthread_mutex_unlock (&_mtx);

  connectData *dat = (connectData *)p;
  //mine_data_holder mdh;
  try {
    // check inputs parameter for mining

    // try make an connection
    size_t tries = 0;
    {
      struct hostent *host = gethostbyname (dat->server);
      if (!host) throw ("host name was invalid");
      dat->sockfd = socket (AF_INET, SOCK_STREAM, 0);
      if (dat->sockfd == -1) throw ("socket has error!");
      struct sockaddr_in server_addr {
        .sin_family = AF_INET,
        .sin_port = htons (dat->port),
        .sin_addr = *((struct in_addr *)host->h_addr)
      };
      // try connect socket
      do {
        if (connect (dat->sockfd, (struct sockaddr *)&server_addr, sizeof (server_addr)) == 0) break;
        ++tries;
        sleep (1);
      } while (tries < MAX_ATTEMPTS_TRY);
      if (tries >= MAX_ATTEMPTS_TRY) throw ("Connection tries is always failed!");
    }
    // try subscribe & authorize
    char buffer[MAX_MESSAGE];
    {
      strcpy (buffer, "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"");
      strcat (buffer, CONNECT_MACHINE);
      strcat (buffer, "\"]}\n{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"");
      strcat (buffer, dat->auth_user);
      strcat (buffer, "\",\"");
      strcat (buffer, dat->auth_pass);
      strcat (buffer, "\"]}\n\0");
      tries = 0;
      for (int sended = 0, length = 106 + strlen (CONNECT_MACHINE) + strlen (dat->auth_user) + strlen (dat->auth_pass); (tries < MAX_ATTEMPTS_TRY) && (sended < length);) {
        int s = send (dat->sockfd, buffer + sended, length - sended, 0);
        if (s <= 0)
          ++tries;
        else
          sended += s;
      }
      if (tries >= MAX_ATTEMPTS_TRY) throw ("Sending subscribe & authorize is always failed!");
    }
    // recv subscribe & authorize prove
    /*
    tries = 0;
    do {
      if (recv (dat->sockfd, buffer, MAX_MESSAGE, 0) > 0) {
        std::string msgRcv (buffer);
        size_t pos = 0;
        while (((pos = msgRcv.find ("\n")) != std::string::npos) && !(mdh.subscribed && mdh.authorized)) {
          json::JSON rcv = json::Parse (msgRcv.substr (0, pos));
          msgRcv.erase (0, pos + 1);
          if (rcv.IsNull ()) continue;
          mdh.updateData (rcv);
        }
      }
      sleep (1);
    } while (!(mdh.subscribed && mdh.authorized) && (++tries < MAX_ATTEMPTS_TRY));
    if (!mdh.subscribed || !mdh.authorized) throw ("Doesn't receive an subscribe or authorize message result!");
    */
    // change state to state start running
    {
      JNIEnv *env;
      if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
        (*env)->CallVoidMethod (env, local_globalRef, updateState, STATE_RUNNING);
        (*env)->CallVoidMethod (env, local_globalRef, sendMessageConsole, 2, (*env)->NewStringUTF (env, "subscribe & authorize success"));
        global_jvm->DetachCurrentThread ();
      }
    }
    // loop update data from server
    tries = 0;
    {
      int loop = STATUS_DOINGJOB;
      while (loop & STATUS_DOINGJOB) {
        pthread_mutex_lock (&_mtx);
        loop = status_flags;
        pthread_mutex_unlock (&_mtx);
        if (recv (dat->sockfd, buffer, MAX_MESSAGE, 0) <= 0) {
          if (++tries > MAX_ATTEMPTS_TRY) throw ("failed to receive message socket!.");
          JNIEnv *env;
          if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
            (*env)->CallVoidMethod (env, local_globalRef, sendMessageConsole, 4, (*env)->NewStringUTF (env, "Connection Failed, Try connect again after a sec!"));
            global_jvm->DetachCurrentThread ();
          }
          sleep (1);
        } else {
          if (tries) tries = 0;
          JNIEnv *env;
          if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
            (*env)->CallVoidMethod (env, local_globalRef, sendMessageConsole, 0, (*env)->NewStringUTF (env, buffer));
            global_jvm->DetachCurrentThread ();
          }
        }
      }
    }
  } catch (const char *er) {
    JNIEnv *env;
    if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
    	char _msg[MAX_MESSAGE];
    	strcpy(_msg, "Connection Failed, because ");
    	strcat(_msg, er);
      (*env)->CallVoidMethod (env, local_globalRef, sendMessageConsole, 4, (*env)->NewStringUTF (env, _msg));
      global_jvm->DetachCurrentThread ();
    }
  }
  if (dat->sockfd != -1) {
    close (dat->sockfd);
    dat->sockfd = -1;
  }
  free(dat->server);
  free(dat->auth_user);
  free(dat->auth_pass);
  free(dat);
  // set state mining to none
  {
    JNIEnv *env;
	  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
	    (*env)->CallVoidMethod (env, local_globalRef, updateState, STATE_NONE);
		  global_jvm->DetachCurrentThread ();
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
    cd->auth_user = malloc((*env)->GetStringUTFLength (env, jauth_user));
    const char *auth_user = (*env)->GetStringUTFChars (env, jauth_user, JNI_FALSE);
    strcpy (cd->auth_user, auth_user);
    (*env)->ReleaseStringUTFChars (env, jauth_user, auth_user);

    jstring jauth_pass = (jstring)(*env)->GetObjectArrayElement (env, s, 2);
    cd->auth_pass = malloc((*env)->GetStringUTFLength (env, jauth_pass));
    const char *auth_pass = (*env)->GetStringUTFChars (env, jauth_pass, JNI_FALSE);
    strcpy (cd->auth_pass, auth_pass);
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
