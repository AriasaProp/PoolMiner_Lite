#include "miner_core.hpp"
#include "json.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>

#define MAX_MSG_BUFFER 4096
#define CONNECT_MACHINE "PoolMiner-Lite"

char *msg_buffer = nullptr;
char *msg_buffer_end = nullptr;

struct hex_ {
	char *value;
	hex_(const int &v) {
		value = new char[sizeof(v)];
		memcpy(value, v, sizeof(v));
	}
	hex_(const std::string &v) {
		size_t l = v.size();
		l = l/2 + (l&1);
		value = new char[l];
		// big-endian i guest
		const char *c = v.c_str();
		char *vl = value;
		do {
			char a = *c;
			*vl = (a <= '9') ? a - '0' : a - 'a' + 10;
			if (!*(++c)) break;
			char b = *c;
			*vl |= ((b <= '9') ? b - '0' : b - 'a' + 10) << 4;
			++vl;
		} while (*(++c));
	}
	~hex_() {
		delete[] value;
	}
};

static struct {
	bool subs, auth;
	
	std::unordered_map<std::string, hex_> session;
	std::unordered_map<std::string, hex_> requipment;
	std::unordered_map<std::string, hex_> job;
	
	void init() {
		subs = false, auth = false;
		session.clear();
		requipment.clear();
		job.clear();
	}
	void clear() {
		subs = false, auth = false;
		session.clear();
		requipment.clear();
		job.clear();
		
	}
} data_mine;

void miner::init () {
	msg_buffer = new char[MAX_MSG_BUFFER]{};
	msg_buffer_end = msg_buffer + MAX_MSG_BUFFER;
	data_mine.init();
}

void miner::msg_send_subs_auth(char *buffer, const char *user, const char *pass) {
	sprintf(buffer,"{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"%s\"]}\n{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"%s\",\"%s\"]}\n", CONNECT_MACHINE, user, pass);
}



std::string miner::parsing(const char *msg) {
	std::stringstream reparser;
	strcat(msg_buffer,msg);
	char *cur_msg = msg_buffer;
	do {
		//find each new line
		char *newline = cur_msg;
		do {
			if (*newline == '\n' || *newline == 0) break;
		} while (++newline < msg_buffer_end);
		size_t branch_t = 0;
		for (char *a; a < newline; ++a) {
			if (*a == '{') ++branch_t;
			else if (*a == '}') --branch_t;
		}
		if ((branch_t == 0) && ((newline - cur_msg) > 7)) {
			json::JSON o = json::parse(std::string(cur_msg, newline - cur_msg));
			json::JSON id = o["id"];
			if (id.IsNull()) {
				std::string m = o["method"];
				json::JSON p = o["params"];
				if (m == "mining.set_difficulty") {
					data_mine.requipment[m] = hex_((int)p[0]);
				} else if (m == "mining.notify") {
					reparser << "job:\n";
					reparser << " " << p[0] << "\n"; 
					reparser << " " << p[1] << "\n"; 
					reparser << " " << p[2] << "\n"; 
					reparser << " " << p[3] << "\n"; 
					reparser << " merkle root:\n"; 
					for (size_t i = 0, j = p[4].size(); i < j; ++i) {
						reparser << "   " << p[4][i] << "\n"; 
					}
					reparser << " " << p[5] << "\n"; 
					reparser << " " << p[6] << "\n"; 
					reparser << " " << p[7] << "\n"; 
					reparser << " " << ((bool)p[8]?"true":"false") << "\n";
				} else {
					reparser << "method: " << m << "\n";
					reparser << "params: " << (std::string)p << "\n";
				}
			} else {
				json::JSON er = o["error"];
				json::JSON res = o["result"];
				switch ((int)id) {
					case 1:
						if (!er.IsNull()) {
							reparser << "error on subs: ";
							reparser << (std::string)er;
							reparser << "\n";
							break;
						}
						data_mine.subs = true;
						data_mine.session[(std::string)res[0][0][0]] = hex_((std::string)res[0][0][1]);
						data_mine.session[(std::string)res[0][1][0]] = hex_((std::string)res[0][1][1]);
						data_mine.session["job.id"] = hex_((std::string)res[1]);
						data_mine.session["protocol"] = hex_((std::string)res[2]);
						break;
					case 2:
						if (!(bool)res) {
							reparser << "auth is invalid" << "\n";
						}
						if (!er.IsNull()) {
							reparser << "error on auth: " << (std::string)er << "\n";
						}
						data_mine.auth = (bool)res & er.IsNull();
						break;
				}
			}
		}
		cur_msg = newline + 1;
	} while (*cur_msg);
	memcpy(msg_buffer, cur_msg, MAX_MSG_BUFFER - (cur_msg - msg_buffer));
	return reparser.str();
}

bool miner::subs_auth() {
	return data_mine.subs & data_mine.auth;
}

void miner::clear() {
	delete[] msg_buffer;
	data_mine.clear();
}


