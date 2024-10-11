#include "miner_core.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <iostream>

#define MAX_MSG_BUFFER 4096

char *msg_buffer = nullptr;
char *msg_buffer_end = nullptr;


void miner::init () {
	msg_buffer = new char[MAX_MSG_BUFFER]{};
	msg_buffer_end = msg_buffer + MAX_MSG_BUFFER;
}

void miner::parsing(const char *msg) {
	strcat(msg_buffer,msg);
	char *cur_msg = msg_buffer;
	do {
		//find each new line
		char *finded = cur_msg;
		do {
			if (*finded == '\n' || *finded == 0) break;
		} while (++finded < msg_buffer_end);
		try {
			if ((finded - cur_msg) < 5) throw "small object";
			//find bracket
			char *op_br = cur_msg;
			char *ed_br = finded - 1;
			do {
				if (*op_br == '{') break;
			} while (++op_br < ed_br);
			do {
				if (*ed_br == '}') break;
			} while (--ed_br > op_br);
			if ((ed_br - op_br) < 4) throw "small object";
			//branch validity
			size_t valid = 0;
			for (char *a = op_br, *b = ed_br; a <= b; ++a) {
				if (*a == '{') valid++;
				else if (*a == '}') valid--;
			}
			if (!valid) throw "branches total invalid";
			op_br++;
			ed_br--;
			
			std::cout << std::string(op_br, ed_br-op_br) << std::endl;
		} catch (const char *er) {
			//end
			std::cout << er << std::endl;
		}
		cur_msg = finded + 1;
	} while (*cur_msg);
}

void miner::clear() {
	delete[] msg_buffer;
}