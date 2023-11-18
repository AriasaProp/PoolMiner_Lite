#include <cstdint>
#include <cstring>
#include <jni.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <unistd.h>

#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <netdb.h>
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

static jmethodID updateSpeed;
static jmethodID updateResult;
static jmethodID updateState;
static jmethodID sendMessageConsole;

// for mining data
static bool mineRunning;
static pthread_mutex_t _mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;
static jobject local_globalRef;
static uint32_t active_worker = 0;
static bool doingjob = false;
static uint32_t port = 80;
static uint32_t thread_use;
static pthread_t *workers = nullptr;

bool MinerService_OnLoad (JNIEnv *env) {
  jclass m_class = env->FindClass ("com/ariasaproject/poolminerlite/MinerService");
  return (m_class &&
          (updateSpeed = env->GetMethodID (m_class, "updateSpeed", "(F)V")) &&
          (updateResult = env->GetMethodID (m_class, "updateResult", "(Z)V")) &&
          (updateState = env->GetMethodID (m_class, "updateState", "(I)V")) &&
          (sendMessageConsole = env->GetMethodID (m_class, "sendMessageConsole", "(ILjava/lang/String;)V")));
  mineRunning = false;
}
void MinerService_OnUnload (JNIEnv *env) {
  env->DeleteGlobalRef (local_globalRef);
  local_globalRef = NULL;
  updateSpeed = NULL;
  updateResult = NULL;
  updateState = NULL;
  sendMessageConsole = NULL;
}

struct connectData {
  char *server;
  char *auth_user;
  char *auth_pass;
  uint32_t port;
  int sockfd;
};

size_t parserObject(char *b, size_t len, char *obj) {
	bool findedObject = false;
	size_t bracket = 0;
	char *buff = b;
	char *endbuff = buff + len;
	while ((buff < endbuff) && *buff && !(findedObject && (bracket == 0))) {
		switch (*buff) {
			case '{':
				if (!findedObject) findedObject = true;
				++bracket;
				break;
			case '}':
				--bracket;
				break;
			default:;
		}
		++buff;
	}
	size_t ob = buff - b;
	memcpy(obj, b, ob);
	memmove(b, buff, len - ob);
	return len - ob;
}

void *connectionWork (void *p) {
  connectData *dat = (connectData *)p;
  try {
    // subscribe & authorize
    char buffer[2000000], storeObj[2000]; // 2 Mbyte
    {
      strcpy (buffer, "{\"id\": 1, \"method\": \"mining.subscribe\", \"params\": []}\n");
      strcat (buffer, "{\"id\": 2, \"method\": \"mining.authorize\", \"params\": [\"");
      strcat (buffer, dat->auth_user);
      strcat (buffer, "\",\"");
      strcat (buffer, dat->auth_pass);
      strcat (buffer, "\"]}\n");
      for (int sended = 0, length = strlen (buffer), tries = 0; (tries < MAX_ATTEMPTS_TRY) && (sended < length);) {
        int s = send (dat->sockfd, buffer + sended, length - sended, 0);
        if (s < 0)
          ++tries;
        else
          sended += s;
      }
      if (tries >= MAX_ATTEMPTS_TRY) throw "Connection tries is always failed!";
    }

    bool loop;
    memset(buffer, 0, 2000000);
    size_t startBuff = 0;
    do {
      pthread_mutex_lock (&_mtx);
      loop = doingjob;
      pthread_mutex_unlock (&_mtx);
      int bytesReceived = recv (dat->sockfd, buffer+startBuff, 2000000-startBuff, 0);
      if (bytesReceived > 0) {
      	startBuff = parserObject(buffer, bytesReceived, storeObj);
        JNIEnv *env;
        if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
          env->CallVoidMethod (local_globalRef, sendMessageConsole, 0, env->NewStringUTF (storeObj));
          global_jvm->DetachCurrentThread ();
        }
      }
      sleep (1);
    } while (loop);
  } catch (const char *er) {
    JNIEnv *env;
    if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
      env->CallVoidMethod (local_globalRef, sendMessageConsole, 4, env->NewStringUTF (er));
      env->CallVoidMethod (local_globalRef, updateState, STATE_NONE);
      global_jvm->DetachCurrentThread ();
    }
  }
  close (dat->sockfd);
  delete[] dat->server;
  delete[] dat->auth_user;
  delete[] dat->auth_pass;
  delete dat;
  pthread_exit (NULL);
}

void *doWork (void *p) {
  const uint32_t start = static_cast<uint32_t> ((unsigned long)p);
  uint32_t nonce = start;
  pthread_mutex_lock (&_mtx);
  ++active_worker;
  pthread_mutex_unlock (&_mtx);
  bool loop;
  do {
    sleep (1);
    // here hashing
    pthread_mutex_lock (&_mtx);
    loop = doingjob;
    uint32_t nn = nonce + thread_use;
    pthread_mutex_unlock (&_mtx);
    if (nn < nonce) break;
    nonce = nn;
  } while (loop);
  /*
  JNIEnv *env;
  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
    std::stringstream ss;
    ss << "Native workers " << start << " have number ";
    env->CallVoidMethod (local_globalRef, sendMessageConsole, 0, env->NewStringUTF(ss.str().c_str()));
    global_jvm->DetachCurrentThread ();
  }
  */
  pthread_mutex_lock (&_mtx);
  --active_worker;
  pthread_cond_broadcast (&_cond);
  pthread_mutex_unlock (&_mtx);
  pthread_exit (NULL);
}
void *toStartBackground (void *p) {
  try {
    // check inputs parameter for mining
    connectData *dat = (connectData *)p;
    struct hostent *host = gethostbyname (data->server);
    if (!host) throw "host name was invalid";
    dat->sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if (dat->sockfd < 0) throw "socket has error!";
    struct sockaddr_in server_addr {
      .sin_family = AF_INET,
      .sin_port = htons (dat->port),
      .sin_addr = *((struct in_addr *)host->h_addr)
    };

    {
      size_t tries = 0;
      while (
          (tries < MAX_ATTEMPTS_TRY) &&
          (connect (dat->sockfd, (struct sockaddr *)&server_addr, sizeof (server_addr)) != 0)) {
        ++tries, sleep (1);
      }
      if (tries >= MAX_ATTEMPTS_TRY) throw "Connection tries is always failed!";
    }
    pthread_attr_t thread_attr;
    pthread_attr_init (&thread_attr);
    pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
    pthread_create (&connectWorker, &thread_attr, connectionWork, (void *)dat);
    pthread_attr_destroy (&thread_attr);
    // done

    pthread_mutex_lock (&_mtx);
    doingjob = true;
    active_worker = 0;
    pthread_mutex_unlock (&_mtx);
    workers = new pthread_t[thread_use];
    for (unsigned long i = 0; i < thread_use; ++i) {
      pthread_attr_t thread_attr;
      pthread_attr_init (&thread_attr);
      pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
      pthread_create (workers + i, &thread_attr, doWork, (void *)i);
      pthread_attr_destroy (&thread_attr);
    }
    JNIEnv *env;
    if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
      env->CallVoidMethod (local_globalRef, updateState, STATE_RUNNING);
      global_jvm->DetachCurrentThread ();
    }
  } catch (const char *er) {
    JNIEnv *env;
    if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
      env->CallVoidMethod (local_globalRef, sendMessageConsole, 4, env->NewStringUTF (er));
      env->CallVoidMethod (local_globalRef, updateState, STATE_NONE);
      global_jvm->DetachCurrentThread ();
    }
  }
  pthread_exit (NULL);
}

#define JNIF(R, M) extern "C" JNIEXPORT R JNICALL Java_com_ariasaproject_poolminerlite_MinerService_##M
JNIF (void, nativeStart)
(JNIEnv *env, jobject o, jobjectArray s, jintArray i) {
  jint *integers = env->GetIntArrayElements (i, nullptr);
  port = integers[0];
  thread_use = integers[1];
  env->ReleaseIntArrayElements (i, integers, JNI_ABORT);

  connectData *cd = new connectData;
  {
    jstring jserverName = (jstring)env->GetObjectArrayElement (s, 0);
    jsize len = env->GetStringUTFLength (jserverName);
    cd->server = new char[len];
    const char *serverName = env->GetStringUTFChars (jserverName, 0);
    memcpy (cd->server, serverName, len);
    env->ReleaseStringUTFChars (jserverName, serverName);
  }
  {
    jstring jauth_user = (jstring)env->GetObjectArrayElement (s, 1);
    jsize len = env->GetStringUTFLength (jauth_user);
    cd->auth_user = new char[len];
    const char *auth_user = env->GetStringUTFChars (jauth_user, 0);
    memcpy (cd->auth_user, auth_user, len);
    env->ReleaseStringUTFChars (jauth_user, auth_user);
  }
  {
    jstring jauth_pass = (jstring)env->GetObjectArrayElement (s, 2);
    jsize len = env->GetStringUTFLength (jauth_pass);
    cd->auth_pass = new char[len];
    const char *auth_pass = env->GetStringUTFChars (jauth_pass, 0);
    memcpy (cd->auth_pass, auth_pass, len);
    env->ReleaseStringUTFChars (jauth_pass, auth_pass);
  }
  if (!local_globalRef)
    local_globalRef = env->NewGlobalRef (o);
  pthread_t starting;
  pthread_attr_t thread_attr;
  pthread_attr_init (&thread_attr);
  pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
  pthread_mutex_lock (&_mtx);
  if (pthread_create (&starting, &thread_attr, toStartBackground, (void *)cd) != 0) {
    doingjob = false;
    env->CallVoidMethod (o, updateState, STATE_NONE);
  } else {
    mineRunning = true;
  }
  pthread_mutex_unlock (&_mtx);
  pthread_attr_destroy (&thread_attr);
}
JNIF (jboolean, nativeRunning)
(JNIEnv *, jobject) {
  pthread_mutex_lock (&_mtx);
  bool r = mineRunning;
  pthread_mutex_unlock (&_mtx);
  return r;
}
void *toStopBackground (void *) {
  if (!active_worker || !workers || !doingjob) pthread_exit (NULL);
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
