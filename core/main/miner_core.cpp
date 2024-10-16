#include "miner_core.hpp"
#include "jansson.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <iostream>

#define MAX_MSG_BUFFER 4096
#define CONNECT_MACHINE "PoolMiner-Lite"

char *msg_buffer = nullptr;
char *msg_buffer_end = nullptr;

void miner::init () {
	msg_buffer = new char[MAX_MSG_BUFFER]{};
	msg_buffer_end = msg_buffer + MAX_MSG_BUFFER;
}

void miner::msg_send_subs_auth(char *buffer, const char *user, const char *pass) {
	sprintf(buffer,"{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"%s\"]}\n{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"%s\",\"%s\"]}\n", CONNECT_MACHINE, user, pass);
}
json_error_t jet;
std::string miner::parsing(const char *msg) {
	std::string reparser = "";
	strcat(msg_buffer,msg);
	char *cur_msg = msg_buffer;
	do {
		//find each new line
		char *newline = cur_msg;
		do {
			if (*newline == '\n' || *newline == 0) break;
		} while (++newline < msg_buffer_end);
		try {
			if ((newline - cur_msg) < 7) throw "small object";
			char ln[newline - cur_msg + 1];
			strncpy(ln, cur_msg, newline - cur_msg);
			json_t *o = json_loads(ln, &jet);
			reparser += json_dumps(o, 0);
			reparser += "\n";
		} catch (const char *er) {
			//end
		}
		cur_msg = newline + 1;
	} while (*cur_msg);
	return reparser;
}

void miner::clear() {
	delete[] msg_buffer;
}