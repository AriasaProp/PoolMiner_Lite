#include "miner_core.hpp"
#include "json.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <iostream>

#define MAX_MSG_BUFFER 4096
#define CONNECT_MACHINE "PoolMiner-Lite"

char *msg_buffer = nullptr;
char *msg_buffer_end = nullptr;

miner::data *ld;

void miner::init () {
	msg_buffer = new char[MAX_MSG_BUFFER]{};
	msg_buffer_end = msg_buffer + MAX_MSG_BUFFER;
	ld = new miner::data;
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
		if ((newline - cur_msg) < 7) goto end_part;
		//json::JSON o = json::Parse(std::string(cur_msg, newline - cur_msg));
		reparser += std::string(cur_msg, newline - cur_msg);
		reparser += "\n";
end_part:
		cur_msg = newline + 1;
	} while (*cur_msg);
	return reparser;
}

void miner::clear() {
	delete[] msg_buffer;
	delete ld;
}


