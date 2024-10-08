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
#include <utility>
#include <vector>

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

// static jclass consoleItem;

static jmethodID updateSpeed;
static jmethodID updateResult;
static jmethodID updateState;
static jmethodID sendMessageConsole;
// static jmethodID consoleItemConstructor;

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
  // consoleItem = env->FindClass("com/ariasaproject/poolminerlite/ConsoleItem");
  if (!m_class /*|| !consoleItem*/) return false;
  updateSpeed = env->GetMethodID (m_class, "updateSpeed", "(F)V");
  updateResult = env->GetMethodID (m_class, "updateResult", "(Z)V");
  updateState = env->GetMethodID (m_class, "updateState", "(I)V");
  sendMessageConsole = env->GetMethodID (m_class, "sendMessageConsole", "(ILjava/lang/String;)V");
  // consoleItemConstructor = env->GetMethodID(consoleItem, "<init>", "(ILjava/lang/String;Ljava/lang/String;)V");
  if (!updateSpeed || !updateResult || !updateState /* || !consoleItemConstructor*/) return false;
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

// mine data holder
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
  hex_array xnonce2;
};
struct mine_data_holder {
private:
	hex_array session_id;
	hex_array difficulty_;
	double difficulty;
	hex_array xnonce1;
	std::string xnonce2_size = "not set";
	std::string version = "not set";
	mining_notify_data mnd;
public:
  std::vector<std::string> json_list_a;
	bool subscribed = false;
	bool authorized = false;
	
	void updateData(json::JSON d) {
		std::string _h;
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
							session_id = value;
						}
						if (method == "mining.set_difficulty") {
							difficulty_ = value;
						}
					}
					//2
					{
						std::string method = res[0][1][0];
						std::string value = res[0][1][1];
						if (method == "mining.notify") {
							session_id = value;
						}
						if (method == "mining.set_difficulty") {
							difficulty_ = value;
						}
					}
					// xnonce1
					xnonce1 = res[1];
					//xnonce2 size
					xnonce2_size = (std::string)res[2];
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
		    mnd.prev_hash = params[1];
		    
		    mnd.coinb1 = params[2];
		    mnd.coinb2 = params[3];
		    // merkle_arr
		    {
		  		json::JSON jm = params[4];
		  		if (jm.JSONType() != json::JSON::Class::Array) throw std::runtime_error("merkle_array params is invalid!");
		  		mnd.merkle_arr.clear();
		  		mnd.merkle_arr.reserve(jm.size());
		  		for (auto it = jm.ArrayRange().begin(); it < jm.ArrayRange().end(); ++it) {
		  			mnd.merkle_arr.push_back((std::string)*it);
		  		}
		    }
		    mnd.version = params[5];
		    mnd.nbit = params[6];
		    mnd.ntime = params[7];
		    mnd.clean = params[8];
			} else if ((method == "client.get_version") && d.hasKey("jsonrpc") && d["jsonrpc"].IsNull()) {
	      version = (std::string)d["jsonrpc"];
			} else {
				_h = std::string("unhandled method: ") + method;
			}
		} else {
			_h = d.dump(1, " ");
		}
		
		if (_h.empty()) {
			json_list_a.push_back(_h);
		}
	}
/*
	void doJob() {
		make header
		
        // Increment extranonce2
        HexArray xnonce2 = this._xnonce2;
        String xnonce2_str = xnonce2.getStr();
        for (int i = 0; i < xnonce2.getLength() && (0 == (++xnonce2.refHex()[i])); i++)
            ;

        // Assemble block header
        HexArray work_data = new HexArray(this._notify.version);
        work_data.append(this._notify.prev_hash);
        work_data.append(this._merkle_loot, 0, 32);
        work_data.append(this._notify.ntime);
        work_data.append(this._notify.nbit);
        work_data.append(new byte[] {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, (byte) 0x80});
        work_data.append(new byte[40]);
        work_data.append(new byte[] {(byte) 0x80, 0x02, 0x00, 0x00});
        return new StratumMiningWork(
                work_data,
                diff2target(this._difficulty / 65536.0),
                this._notify.job_id,
                xnonce2_str);
    // Increment extranonce2
    
    // Assemble block header
    std::string header = mnd.version;
    std::string prev = mnd.prev_hash;
    header += prev;
	}
		*/
};

// 5 kBytes ~> 40 kBit
#define MAX_MESSAGE 5000
#define CONNECT_MACHINE "PoolMiner-Lite"

struct connectData {
  struct sockaddr_in server_addr;
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
    // do hashing
  } while (loop);
  pthread_mutex_lock (&_mtx);
  --active_worker;
  pthread_cond_broadcast (&_cond);
  pthread_mutex_unlock (&_mtx);
  pthread_exit (NULL);
}
char buffer[MAX_MESSAGE];
void *startConnect (void *p) {
  pthread_mutex_lock (&_mtx);
  ++active_worker;
  pthread_mutex_unlock (&_mtx);

  connectData *dat = (connectData *)p;
  mine_data_holder mdh;
	//make socket
  try {
	  int sockfd = socket (AF_INET, SOCK_STREAM, 0);
	  if (sockfd == -1) throw std::runtime_error ("socket has error!");
	  try {
	    // check inputs parameter for mining
	    size_t tries = 0;
      // try connect socket
      while (connect (sockfd, (struct sockaddr *)&dat->server_addr, sizeof (dat->server_addr)) != 0) {
      	if (++tries >= MAX_ATTEMPTS_TRY) throw std::runtime_error ("Connection tries is always failed!");
        sleep (1);
      }
	    // try subscribe & authorize
	    {
	      JNIEnv *env;
	      if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
	        env->CallVoidMethod (local_globalRef, sendMessageConsole, 2, env->NewStringUTF ("start sending"));
	        env->CallVoidMethod (local_globalRef, sendMessageConsole, 2, env->NewStringUTF (dat->auth_user));
	        env->CallVoidMethod (local_globalRef, sendMessageConsole, 2, env->NewStringUTF (dat->auth_pass));
	        global_jvm->DetachCurrentThread ();
	      }
	    }
    	snprintf(buffer, MAX_MESSAGE,"{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"%s\"]}\n{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"%s\",\"%s\"]}", CONNECT_MACHINE, dat->auth_user, dat->auth_pass);
      tries = 0;
      for (size_t length = strlen(buffer), s; length > 0;) {
		    s = send (sockfd, buffer, length, 0);
		    if (s <= 0) {
      		if (++tries >= MAX_ATTEMPTS_TRY) throw std::runtime_error ("Sending subscribe & authorize is always failed!");
		      continue;
		    }
		    length -= s;
		    memmove(buffer, buffer + s, length);
      }
	    {
	      JNIEnv *env;
	      if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
	        env->CallVoidMethod (local_globalRef, sendMessageConsole, 2, env->NewStringUTF ("Sended"));
	        global_jvm->DetachCurrentThread ();
	      }
	    }
	    // recv subscribe & authorize prove
	    tries = 0;
	    do {
	      if (recv (sockfd, buffer, MAX_MESSAGE, 0) > 0) {
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
	    if (!mdh.subscribed || !mdh.authorized) throw std::runtime_error ("Doesn't receive an subscribe or authorize message result!");
	    // change state to state start running
	    {
	      JNIEnv *env;
	      if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
	        env->CallVoidMethod (local_globalRef, updateState, STATE_RUNNING);
	        env->CallVoidMethod (local_globalRef, sendMessageConsole, 2, env->NewStringUTF ("subscribe & authorize success"));
	        global_jvm->DetachCurrentThread ();
	      }
	    }
	    // loop update data from server
	    tries = 0;
	    {
	      bool loop = true;
	      while (loop) {
	        pthread_mutex_lock (&_mtx);
	        loop = doingjob;
	        pthread_mutex_unlock (&_mtx);
	        if (recv (sockfd, buffer, MAX_MESSAGE, 0) <= 0) {
	          if (++tries > MAX_ATTEMPTS_TRY) throw std::runtime_error ("failed to receive message socket!.");
	          JNIEnv *env;
	          if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
	            env->CallVoidMethod (local_globalRef, sendMessageConsole, 4, env->NewStringUTF ("Connection Failed, Try connect again after a sec!"));
	            global_jvm->DetachCurrentThread ();
	          }
	          sleep (1);
	        } else {
	          if (tries) tries = 0;
	          std::string msgRcv (buffer);
	          size_t pos = 0;
	          while ((pos = msgRcv.find ("\n")) != std::string::npos) {
	            json::JSON rcv = json::Parse (msgRcv.substr (0, pos));
	            msgRcv.erase (0, pos + 1);
	            if (rcv.IsNull ()) continue;
	            mdh.updateData (rcv);
	          }
	        }
	      }
	    }
	  } catch (const std::exception &er) {
    	close (sockfd);
	  	throw er;
	  }
    close (sockfd);
  } catch (const std::exception &er) {
    JNIEnv *env;
    if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
    	std::string _msg = "Connection Failed, because ";
    	_msg += er.what();
      env->CallVoidMethod (local_globalRef, sendMessageConsole, 4, env->NewStringUTF (_msg.c_str ()));
      global_jvm->DetachCurrentThread ();
    }
  }
  
  delete[] dat->auth_user;
  delete[] dat->auth_pass;
  delete dat;
  // set state mining to none
  {
    JNIEnv *env;
	  if (global_jvm->AttachCurrentThread (&env, &attachArgs) == JNI_OK) {
	    env->CallVoidMethod (local_globalRef, updateState, STATE_NONE);
			if (!mdh.json_list_a.empty()) {
				strcpy(buffer, "Result: \n");
				for (std::string erl : mdh.json_list_a) {
					strcat(buffer, erl.c_str());
					strcat(buffer, "\n");
				}
				env->CallVoidMethod (local_globalRef, sendMessageConsole, 0, env->NewStringUTF(buffer));
	  		mdh.json_list_a.clear();
			}
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

JNIF (void, nativeStart)
(JNIEnv *env, jobject o, jobjectArray s, jintArray i) {
  connectData *cd = new connectData;
  {
  	cd->server_addr.sin_family = AF_INET;
    jint *integers = env->GetIntArrayElements (i, NULL);
	  cd->server_addr.sin_port = htons (integers[0]);
    thread_use = integers[1];
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
JNIF (jboolean, nativeRunning)
(JNIEnv *, jobject) {
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
