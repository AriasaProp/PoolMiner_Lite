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
		{
			size_t branch_t = 0;
			for (char *a; a < newline; ++a)
				if (*a == '{') ++branch_t;
				else if (*a == '}') --branch_t;
			}
			if (branch_t) break;
		}
		if ((newline - cur_msg) < 7) goto end_part;
		json::JSON o = json::parse(std::string(cur_msg, newline - cur_msg));
		json::JSON id = o["id"];
		if (id.IsNull()) {
			// nothing
		} else {
			switch ((int)id) {
				case 1:
					break;
				case 2:
					break;
			}
		}
		
end_part:
		cur_msg = newline + 1;
	} while (*cur_msg);
	memcpy(msg_buffer, cur_msg, MAX_MSG_BUFFER - (cur_msg - msg_buffer));
	return reparser;
}

void miner::clear() {
	delete[] msg_buffer;
	delete ld;
}


