#ifndef MINER_CORE
#define MINER_CORE

#include <cstdint>
#include <cstdlib>
#include <string>


namespace miner {

	void init();
	
	void msg_send_subs_auth(char*,const char*, const char*);
	
	std::string parsing(const char *);
	
	bool subs_auth();
	
	void clear();
}

#endif //MINER_CORE