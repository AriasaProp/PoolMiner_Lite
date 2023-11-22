#include <cstdint>
#include <cstring>
#include <jni.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <utility>

#include "json.hpp"
#include "util.hpp"

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

//mine data holder
struct mine_data_holder {
private:
	hex_array session_id;
	hex_array difficulty;
	hex_array xnonce1;
	size_t xnonce2_size;
	void updateByMethod(std::string method, std::string value) {
		if (method == "mining.notify") {
			session_id = convert::hexString_toBiner(value);
		}
		if (method == "mining.set_difficulty") {
			difficulty = convert::hexString_toBiner(value);
		}
	}
public:
	bool subscribed = false;
	bool authorized = false;
	
	
	void updateData(json::JSON d) {
		if (!d.hasKey("id")) throw "json data doesn't has id. it's invalid.";
		if (d["id"].IsNull()) {
			
		} else {
			int id = d["id"];
			switch (id) {
				case 1: //subscribe proving
					if (d.hasKey("error") && !d["error"].IsNull())
						throw ((std::string) d["error"]).c_str();
					if (!d.hasKey("result")) throw "hasn't result";
					if (d["result"].IsNull()) throw "subscribe result is null, how?";
					// method data extraction
					updateByMethod(
						d["result"][0][0][0],
						d["result"][0][0][1]
					);
					updateByMethod(
						d["result"][0][1][0],
						d["result"][0][1][1]
					);
					// xnonce1
					xnonce1 = convert::hexString_toBiner(d["result"][1]);
					//xnonce2 size
					xnonce2_size = (int)d["result"][2];
					subscribed = true;
					break;
				case 2:
					if (d.hasKey("error") && !d["error"].IsNull())
						throw ((std::string) d["error"]).c_str();
					if (d.hasKey("result")) {
						if (d["result"].IsNull() && !((bool)d["result"])) throw "authorize is failed";
					}
					authorized = true;
					break;
				default:
					break;
			}
		}
	}
};

// for mining data
static bool mineRunning;
static pthread_mutex_t _mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;
static jobject local_globalRef;
static uint32_t active_worker = 0;
static bool doingjob = false;
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
static std::vector<std::pair<jint, char const*>> queuedMsg;
static void inline sendJavaMsg(jint lvl, const char* msg) {
	pthread_mutex_lock (&_mtx);
  queueMsg.emplace_back(lvl, msg);
  pthread_mutex_unlock (&_mtx);
	JNIEnv *env;
  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
		pthread_mutex_lock (&_mtx);
  	for (std::pair<jint,char const*> m : queueMsg) {
    	env->CallVoidMethod (local_globalRef, sendMessageConsole, m.first, env->NewStringUTF (m.second));
    	delete[] m.second;
  	}
  	m.clear();
  	pthread_mutex_unlock (&_mtx);
    global_jvm->DetachCurrentThread ();
  }
}

struct connectData {
  char *server;
  char *auth_user;
  char *auth_pass;
  uint32_t port;
  int sockfd;
};

// 5 kBytes => 40 kBit
#define MAX_MESSAGE 5000
#define CONNECT_MACHINE "PoolMiner-Lite"

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
  try {
  	mine_data_holder mdh;
    // check inputs parameter for mining
    struct hostent *host = gethostbyname (dat->server);
    if (!host) throw "host name was invalid";
    dat->sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if (dat->sockfd < 0) throw "socket has error!";
    struct sockaddr_in server_addr {
      .sin_family = AF_INET,
      .sin_port = htons (dat->port),
      .sin_addr = *((struct in_addr *)host->h_addr)
    };
    //try connect socket
    {
      size_t tries = 0;
	    do {
        if (connect (dat->sockfd, (struct sockaddr *)&server_addr, sizeof (server_addr)) == 0) break;
        ++tries;
        sleep (1);
	    } while (tries < MAX_ATTEMPTS_TRY);
      if (tries >= MAX_ATTEMPTS_TRY) throw "Connection tries is always failed!";
    }
    try {
    	// try subscribe
    	size_t start_buffer = 0;
    	char buffer[MAX_MESSAGE];
    	char message[MAX_MESSAGE];
	    char storeObj[MAX_MESSAGE];
	    
      strcpy (message, "{\"id\": 1, \"method\": \"mining.subscribe\", \"params\": [\"");
      strcat (message, CONNECT_MACHINE);
      strcat (message, "\"]}\n");
      size_t tries = 0;
      for (int sended = 0, length = strlen (message); (tries < MAX_ATTEMPTS_TRY) && (sended < length);) {
        int s = send (dat->sockfd, message + sended, length - sended, 0);
        if (s <= 0) ++tries; else sended += s;
      }
      if (tries >= MAX_ATTEMPTS_TRY) throw "Sending subscribe is always failed!";
      //recv subscribe prove
      do {
		    size_t len;
		    int bytesReceived = recv (dat->sockfd, buffer+start_buffer, MAX_MESSAGE-start_buffer, 0);
		  	char *findNewLine;
      	while ((bytesReceived > 0) && (findNewLine = strchr(buffer, '\n'))) {
      		bytesReceived += start_buffer;
    			len = findNewLine - buffer;
      		if (len > 2)
						strncpy(storeObj, buffer, len);
					bytesReceived -= len+1;
					memmove(buffer, findNewLine+1, bytesReceived);
					memset(buffer+bytesReceived, 0, MAX_MESSAGE - bytesReceived);
					json::JSON rcv = json::Parse(std::string(storeObj));
					if(rcv.IsNull()) continue;
					mdh.updateData(rcv);
					if (mdh.subscribed) break;
				}
				start_buffer = bytesReceived;
    		sleep(1);
      } while (++tries < MAX_ATTEMPTS_TRY);
      if (!mdh.subscribed) throw "Doesn't receive any subscribe message result!";
      sendJavaMsg(2, "subscribe success");
      
    	// try authorize
      strcpy (message, "{\"id\": 2, \"method\": \"mining.authorize\", \"params\": [\"");
      strcat (message, dat->auth_user);
      strcat (message, "\",\"");
      strcat (message, dat->auth_pass);
      strcat (message, "\"]}\n");
      tries = 0;
      for (int sended = 0, length = strlen (message); (tries < MAX_ATTEMPTS_TRY) && (sended < length);) {
        int s = send (dat->sockfd, message + sended, length - sended, 0);
        if (s <= 0)
          ++tries;
        else
          sended += s;
      }
      if (tries >= MAX_ATTEMPTS_TRY) throw "Sending authorize is always failed!";
      //recv authorize prove
      do {
		    size_t len;
		    int bytesReceived = recv (dat->sockfd, buffer+start_buffer, MAX_MESSAGE-start_buffer, 0);
		  	char *findNewLine;
      	while ((bytesReceived > 0) && (findNewLine = strchr(buffer, '\n'))) {
    			bytesReceived += start_buffer;
    			len = findNewLine - buffer;
      		if (len > 2)
						strncpy(storeObj, buffer, len);
					bytesReceived -= len+1;
					memmove(buffer, findNewLine+1, bytesReceived);
					memset(buffer+bytesReceived, 0, MAX_MESSAGE - bytesReceived);
					json::JSON rcv = json::Parse(std::string(storeObj));
					if(rcv.IsNull()) continue;
					mdh.updateData(rcv);
					if (mdh.authorized) break;
				}
				start_buffer = bytesReceived;
    		sleep(1);
      } while (++tries < MAX_ATTEMPTS_TRY);
      if (!mdh.authorized) throw "Doesn't receive any authorize message result!";
      sendJavaMsg(2, "authorize success");
      //state start running
      {
	      JNIEnv *env;
		    if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
		      env->CallVoidMethod (local_globalRef, updateState, STATE_RUNNING);
		      global_jvm->DetachCurrentThread ();
		    }
	    }
	    //loop update data from server
	    {
		    bool loop = true;
		    size_t len, tries = 0;
		    int bytesReceived;
		  	char *findNewLine;
		    while (loop) {
		      pthread_mutex_lock (&_mtx);
		      loop = doingjob;
		      pthread_mutex_unlock (&_mtx);
		      if ((bytesReceived = recv (dat->sockfd, buffer+start_buffer, MAX_MESSAGE-start_buffer, 0)) <= 0) {
		    		if (++tries > MAX_ATTEMPTS_TRY) throw "failed to receive message socket!.";
						sleep (1);
						continue;
		      }
		      if (tries) tries = 0;
	      	while ((bytesReceived > 0) && (findNewLine = strchr(buffer, '\n'))) {
	    			bytesReceived += start_buffer;
	    			len = findNewLine - buffer;
	      		if (len > 2)
							strncpy(storeObj, buffer, len);
						bytesReceived -= len+1;
						memmove(buffer, findNewLine+1, bytesReceived);
						memset(buffer+bytesReceived, 0, MAX_MESSAGE - bytesReceived);
		        sendJavaMsg(0, storeObj);
					}
					start_buffer = bytesReceived;
		    }
	    }
    } catch (const char *er) {
    	close(dat->sockfd);
    	throw er;
    }
  	close(dat->sockfd);
/*
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
*/
  } catch (const char *er) {
    sendJavaMsg(4, er);
  }
  delete[] dat->server;
  delete[] dat->auth_user;
  delete[] dat->auth_pass;
  delete dat;
  //set state mining to none
  pthread_mutex_lock (&_mtx);
  JNIEnv *env;
  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
		for (std::pair<jint,char const*> m : queueMsg) {
			env->CallVoidMethod (local_globalRef, sendMessageConsole, m.first, env->NewStringUTF (m.second));
			delete[] m.second;
		}
		m.clear();
    env->CallVoidMethod (local_globalRef, updateState, STATE_NONE);
    global_jvm->DetachCurrentThread ();
  }
  --active_worker;
  pthread_cond_broadcast (&_cond);
  pthread_mutex_unlock (&_mtx);
  pthread_exit (NULL);
}

#define JNIF(R, M) extern "C" JNIEXPORT R JNICALL Java_com_ariasaproject_poolminerlite_MinerService_##M

JNIF (void, nativeStart) (JNIEnv *env, jobject o, jobjectArray s, jintArray i) {
  connectData *cd = new connectData;
  {
	  jint *integers = env->GetIntArrayElements (i, nullptr);
	  cd->port = integers[0];
	  thread_use = integers[1];
	  env->ReleaseIntArrayElements (i, integers, JNI_ABORT);
	}
  {
    jstring jserverName = (jstring)env->GetObjectArrayElement (s, 0);
    cd->server = new char[env->GetStringUTFLength (jserverName)];
    const char *serverName = env->GetStringUTFChars (jserverName, JNI_FALSE);
    strcpy (cd->server, serverName);
    env->ReleaseStringUTFChars (jserverName, serverName);
  
    jstring jauth_user = (jstring)env->GetObjectArrayElement (s, 1);
    cd->auth_user = new char[env->GetStringUTFLength (jauth_user)];
    const char *auth_user = env->GetStringUTFChars (jauth_user, JNI_FALSE);
    strcpy (cd->auth_user, auth_user);
    env->ReleaseStringUTFChars (jauth_user, auth_user);
  
    jstring jauth_pass = (jstring)env->GetObjectArrayElement (s, 2);
    cd->auth_pass = new char[env->GetStringUTFLength (jauth_pass)];
    const char *auth_pass = env->GetStringUTFChars (jauth_pass, JNI_FALSE);
    strcpy (cd->auth_pass, auth_pass);
    env->ReleaseStringUTFChars (jauth_pass, auth_pass);
  }
  if (!local_globalRef)
    local_globalRef = env->NewGlobalRef (o);
  pthread_t starting;
  pthread_attr_t thread_attr;
  pthread_attr_init (&thread_attr);
  pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
  pthread_mutex_lock (&_mtx);
  active_worker = 0;
  if (pthread_create (&starting, &thread_attr, startConnect, (void *)cd) != 0) {
    doingjob = false;
    env->CallVoidMethod (o, updateState, STATE_NONE);
  } else {
    mineRunning = true;
  }
  pthread_mutex_unlock (&_mtx);
  pthread_attr_destroy (&thread_attr);
}
JNIF (jboolean, nativeRunning) (JNIEnv *, jobject) {
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
  pthread_exit (NULL);
}
JNIF (void, nativeStop) (JNIEnv *, jobject) {
  // send state for mine was stop
  pthread_t stopping;
  pthread_attr_t thread_attr;
  pthread_attr_init (&thread_attr);
  pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
  pthread_create (&stopping, &thread_attr, toStopBackground, NULL);
  pthread_attr_destroy (&thread_attr);
}
