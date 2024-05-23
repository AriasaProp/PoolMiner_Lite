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
#include <iomanip>
#include <chrono>
#include <ctime>

#include "json.hpp"
#include "util.hpp"

#define STATE_NONE 0
#define STATE_ONSTART 1
#define STATE_RUNNING 2
#define STATE_ONSTOP 3

#define MAX_ATTEMPTS_TRY 10

extern JavaVM *global_jvm;

static JavaVMAttachArgs attachArgs {
	.version = JNI_VERSION_1_6,
	.name = "CpuWorker",
	.group = NULL
};

//static jclass consoleItem;

static jmethodID updateSpeed;
static jmethodID updateResult;
static jmethodID updateState;
static jmethodID sendMessageConsole;
//static jmethodID consoleItemConstructor;

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
  //consoleItem = env->FindClass("com/ariasaproject/poolminerlite/ConsoleItem");
  if (!m_class /*|| !consoleItem*/) return false;
	updateSpeed = env->GetMethodID (m_class, "updateSpeed", "(F)V");
	updateResult = env->GetMethodID (m_class, "updateResult", "(Z)V");
	updateState = env->GetMethodID (m_class, "updateState", "(I)V");
	sendMessageConsole = env->GetMethodID (m_class, "sendMessageConsole", "(ILjava/lang/String;Ljava/lang/String;)V");
  //consoleItemConstructor = env->GetMethodID(consoleItem, "<init>", "(ILjava/lang/String;Ljava/lang/String;)V");
  if (!updateSpeed || !updateResult || !updateState/* || !consoleItemConstructor*/) return false;
	mineRunning = false;
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

//mine data holder
struct mining_notify_data {
	std::string job_id;
  hex_array version;
  std::vector<hex_array> merkle_arr;
  hex_array ntime;
  hex_array nbit;
  bool clean;
  hex_array prev_hash;
  hex_array coinb1;
  hex_array coinb2;
};
struct mine_data_holder {
private:
	hex_array session_id;
	hex_array difficulty_;
	double difficulty;
	hex_array xnonce1;
	size_t xnonce2_size;
	std::string version = "not set";
	mining_notify_data mnd;
public:
	bool subscribed = false;
	bool authorized = false;
	
	void updateData(json::JSON d) {
		std::string _h = "handled";
		//throw error
		if (d.hasKey("error") && !d["error"].IsNull())
			_h = (std::string)d["error"];
		//valid result
		if (d.hasKey("id") && !d["id"].IsNull()) {
			//statisfy any requested result
			int id = d["id"];
			switch (id) {
				case 1: {
					//is subscribe
		      if (!d.hasKey("result") || d["result"].IsNull() || (d["result"].JSONType() != json::JSON::Class::Array)) throw std::runtime_error("hasn't valid result");
					json::JSON &res = d["result"];
					// method data extraction
					//1
					{
						std::string method = res[0][0][0];
						std::string value = res[0][0][1];
						if (method == "mining.notify") {
							session_id = convert::hexString_toBiner(value);
						}
						if (method == "mining.set_difficulty") {
							difficulty_ = convert::hexString_toBiner(value);
						}
					}
					//2
					{
						std::string method = res[0][1][0];
						std::string value = res[0][1][1];
						if (method == "mining.notify") {
							session_id = convert::hexString_toBiner(value);
						}
						if (method == "mining.set_difficulty") {
							difficulty_ = convert::hexString_toBiner(value);
						}
					}
					// xnonce1
					xnonce1 = convert::hexString_toBiner(res[1]);
					//xnonce2 size
					xnonce2_size = (int)res[2];
					subscribed = true;
				} break;
				case 2: {
					//is authorize
					if (!d.hasKey("result") || d["result"].IsNull() || !((bool)d["result"])) throw std::runtime_error("authorize is invalid");
					authorized = true;
				} break;
				default:
					_h = std::string("unhandled id ") + (std::string)d["id"];
			}
		} else if (d.hasKey("method") && d.hasKey("params")) {
			//statisfy any received method
			std::string method = d["method"];
			if (method == "mining.set_difficulty") {
				difficulty = d["params"][0];
			} else if (method == "mining.notify") {
				json::JSON params = d["params"];
				if (params.size() < 8) throw std::runtime_error("mining.notify params has not enough informations!");
		    mnd.job_id = (std::string)params[0];
		    mnd.prev_hash = convert::hexString_toBiner(params[1]);
		    
		    mnd.coinb1 = convert::hexString_toBiner(params[2]);
		    mnd.coinb2 = convert::hexString_toBiner(params[3]);
		    // merkle_arr
		    {
		  		json::JSON jm = params[4];
		  		if (jm.JSONType() != json::JSON::Class::Array) throw std::runtime_error("merkle_array params is invalid!");
		  		mnd.merkle_arr.clear();
		  		mnd.merkle_arr.reserve(jm.size());
		  		for (auto it = jm.ArrayRange().begin(); it < jm.ArrayRange().end(); ++it) {
		  			mnd.merkle_arr.push_back(convert::hexString_toBiner(*it));
		  		}
		    }
		    mnd.version = convert::hexString_toBiner(params[5]);
		    mnd.nbit = convert::hexString_toBiner(params[6]);
		    mnd.ntime = convert::hexString_toBiner(params[7]);
		    mnd.clean = params[8];
			} else if (method == "client.get_version") {
	      if (!d.hasKey("jsonrpc") && d["jsonrpc"].IsNull()) throw std::runtime_error("invalid version");
	      version = (std::string)d["jsonrpc"];
			} else {
				_h = std::string("unhandled method: ") + method;
			}
		} else {
			_h = d.dump();
		}
		if (_h != "handled") {
			JNIEnv *env;
	    if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
				env->CallVoidMethod (local_globalRef, sendMessageConsole, 4,
					env->NewStringUTF("Json Parser or Data Error"),
					env->NewStringUTF(_h.c_str())
				);
	      global_jvm->DetachCurrentThread ();
	    }
		}
	}
	
	std::string getPreMiningData() {
		std::string result;
		result += "session id: ", result += convert::hexBiner_toString(session_id);
		result += "\ndifficulty: ", result += convert::hexBiner_toString(difficulty_);
		result += "\nxnonce 1: ", result += convert::hexBiner_toString(xnonce1);
		result += "\nxnonce 2 size: ", result += xnonce2_size;
		result += "\nversion: ", result += version;
		return result;
	}
	std::string getMiningData() {
		std::string result;
		result += "job id: ", result += mnd.job_id;
		result += "\nversion: ", result += convert::hexBiner_toString(mnd.version);
		result += "\nmerkle_root: ";
		for (hex_array mr : mnd.merkle_arr)
			result += "\n  " + convert::hexBiner_toString(mr);
		result += "\nTime: " + convert::hexBiner_toString(mnd.ntime) + ", nbit: " + convert::hexBiner_toString(mnd.nbit) + ", clean: " + std::string(mnd.clean?"true":"false");
		result += "\nPrevious hash: ", result += convert::hexBiner_toString(mnd.prev_hash);
		result += "\nCoinbase 1: ", result += convert::hexBiner_toString(mnd.coinb1);
		result += "\nCoinbase 2: ", result += convert::hexBiner_toString(mnd.coinb2);
		return result;
	}
};

// 5 kBytes ~> 40 kBit
#define MAX_MESSAGE 5000
#define CONNECT_MACHINE "PoolMiner-Lite"

struct connectData {
	int sockfd = -1;
	uint32_t port;
	char *server;
	char *auth_user;
	char *auth_pass;
};

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
    //do hashing
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
    
    //try make an connection
    size_t tries = 0;
    {
	    struct hostent *host = gethostbyname (dat->server);
	    if (!host) throw std::runtime_error("host name was invalid");
	    dat->sockfd = socket (AF_INET, SOCK_STREAM, 0);
	    if (dat->sockfd == -1) throw std::runtime_error("socket has error!");
	    struct sockaddr_in server_addr {
	      .sin_family = AF_INET,
	      .sin_port = htons (dat->port),
	      .sin_addr = *((struct in_addr *)host->h_addr)
	    };
	    //try connect socket
	    do {
	      if (connect (dat->sockfd, (struct sockaddr *)&server_addr, sizeof (server_addr)) == 0) break;
	      ++tries;
	      sleep (1);
	    } while (tries < MAX_ATTEMPTS_TRY);
    	if (tries >= MAX_ATTEMPTS_TRY) throw std::runtime_error("Connection tries is always failed!");
    }
  	// try subscribe & authorize
  	char buffer[MAX_MESSAGE];
    {
    	strcpy(buffer, "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"");
    	strcat(buffer, CONNECT_MACHINE);
    	strcat(buffer, "\"]}\n{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"");
      strcat(buffer, dat->auth_user);
      strcat(buffer, "\",\"");
      strcat(buffer, dat->auth_pass);
      strcat(buffer, "\"]}\n\0");
      tries = 0;
      for (int sended = 0, length = 106+strlen(CONNECT_MACHINE)+strlen(dat->auth_user)+strlen(dat->auth_pass); (tries < MAX_ATTEMPTS_TRY) && (sended < length);) {
        int s = send (dat->sockfd, buffer + sended, length - sended, 0);
        if (s <= 0) ++tries; else sended += s;
      }
      if (tries >= MAX_ATTEMPTS_TRY) throw std::runtime_error("Sending subscribe & authorize is always failed!");
  	}
    //recv subscribe & authorize prove
    tries = 0;
    do {
	    if (recv (dat->sockfd, buffer, MAX_MESSAGE, 0) > 0) {
		  	std::string msgRcv(buffer);
		    size_t pos = 0;
      	while (((pos = msgRcv.find("\n")) != std::string::npos) && !(mdh.subscribed && mdh.authorized)) {
					json::JSON rcv = json::Parse(msgRcv.substr(0, pos));
					msgRcv.erase(0, pos+1);
					if(rcv.IsNull()) continue;
					mdh.updateData(rcv);
				}
	    }
  		sleep(1);
    } while (!(mdh.subscribed && mdh.authorized)  && (++tries < MAX_ATTEMPTS_TRY));
  	if (!mdh.subscribed || !mdh.authorized) throw std::runtime_error("Doesn't receive an subscribe or authorize message result!");
    //change state to state start running
    {
      JNIEnv *env;
	    if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
	      env->CallVoidMethod (local_globalRef, updateState, STATE_RUNNING);
				env->CallVoidMethod (local_globalRef, sendMessageConsole, 2,
					env->NewStringUTF("subscribe & authorize success"),
					env->NewStringUTF("authorize with username and pass as writen.")
				);
	      global_jvm->DetachCurrentThread ();
	    }
    }
    //loop update data from server
    tries = 0;
    {
	    bool loop = true;
	    while (loop) {
	      pthread_mutex_lock (&_mtx);
	      loop = doingjob;
	      pthread_mutex_unlock (&_mtx);
	      if (recv (dat->sockfd, buffer, MAX_MESSAGE, 0) <= 0) {
	    		if (++tries > MAX_ATTEMPTS_TRY) throw std::runtime_error("failed to receive message socket!.");
					JNIEnv *env;
				  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
						env->CallVoidMethod (local_globalRef, sendMessageConsole, 4, env->NewStringUTF("Connection Failed"), env->NewStringUTF("Try connect again after a sec!"));
					  global_jvm->DetachCurrentThread ();
				  }
					sleep (1);
	      } else {
		      if (tries) tries = 0;
			  	std::string msgRcv(buffer);
			    size_t pos = 0;
	      	while ((pos = msgRcv.find("\n")) != std::string::npos) {
						json::JSON rcv = json::Parse(msgRcv.substr(0, pos));
						msgRcv.erase(0, pos+1);
						if(rcv.IsNull()) continue;
						mdh.updateData(rcv);
						std::string _pmdata = mdh.getPreMiningData();
						std::string _mdata = mdh.getMiningData();
				    JNIEnv *env;
					  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
							env->CallVoidMethod (local_globalRef, sendMessageConsole, 0, env->NewStringUTF("Pre-Mining Data"), env->NewStringUTF(_pmdata.c_str()));
							env->CallVoidMethod (local_globalRef, sendMessageConsole, 0, env->NewStringUTF("Mining Data"), env->NewStringUTF(_mdata.c_str()));
						  global_jvm->DetachCurrentThread ();
					  }
					}
	      }
	      
	    }
    }
  } catch (const std::exception &er) {
    JNIEnv *env;
	  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
			env->CallVoidMethod (local_globalRef, sendMessageConsole, 4, env->NewStringUTF("Connection Failed"), env->NewStringUTF(er.what()));
		  global_jvm->DetachCurrentThread ();
	  }
  }
  if (dat->sockfd != -1)  {
  	close(dat->sockfd);
  	dat->sockfd = -1;
  }
  delete[] dat->server;
  delete[] dat->auth_user;
  delete[] dat->auth_pass;
  delete dat;
  //set state mining to none
  {
	  JNIEnv *env;
	  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
	    env->CallVoidMethod (local_globalRef, updateState, STATE_NONE);
	    global_jvm->DetachCurrentThread ();
	  }
  }
  pthread_mutex_lock (&_mtx);
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
    doingjob = true;
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
  if (active_worker && doingjob) {
	  pthread_mutex_lock (&_mtx);
	  doingjob = false;
	  while (active_worker > 0)
	    pthread_cond_wait (&_cond, &_mtx);
	  mineRunning = false;
	  pthread_mutex_unlock (&_mtx);
	  if (workers) delete[] workers, workers = nullptr;
  }
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


