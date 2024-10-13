#ifndef MINER_CORE
#define MINER_CORE

#include <cstdint>
#include <cstdlib>
#include <string>


namespace miner {
	void init();
	
	void msg_send_subscribe(char *);
	void msg_send_auth(char*, char*, char*);
	
	std::string parsing(const char *);
	
	
	void clear();
}

#endif //MINER_CORE