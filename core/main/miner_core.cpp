#include "miner_core.hpp"

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

void miner::msg_send_subscribe(char *buffer) {
	sprintf(buffer,"{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"%s\"]}\n", CONNECT_MACHINE);
}
void miner::msg_send_auth(char *buffer, const char *user, const char *pass) {
	sprintf(buffer,"{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"%s\",\"%s\"]}\n", user, pass);
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
		try {
			if ((newline - cur_msg) < 7) throw "small object";
			//find bracket
			char *op_br = cur_msg;
			char *ed_br = newline - 1;
			do {
				if (*op_br == '{') break;
			} while (++op_br < ed_br);
			do {
				if (*ed_br == '}') break;
			} while (--ed_br > op_br);
			if ((ed_br - op_br) < 6) throw "small object";
			{
				//branch validity
				size_t valid = 0;
				char *a = ++op_br; 
				do {
					if (*a == '{') ++valid;
					else if (*a == '}') --valid;
				} while (++a < ed_br);
				if (valid) throw "branches total invalid";
			}
			
			do {
				char *koma = op_br;
				do {
					if (*koma == ',') break;
				} while (++koma < ed_br);
				
				if ((koma - op_br) > 5) {
					reparser += std::string(op_br, koma-op_br);
					reparser += "\n";
				}
				op_br = koma + 1;
			} while(op_br < ed_br);
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