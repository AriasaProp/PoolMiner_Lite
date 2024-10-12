#ifndef MINER_CORE
#define MINER_CORE

#include <cstdint>
#include <cstdlib>
#include <string>


namespace miner {
	void init();
	
	std::string parsing(const char *);
	
	
	void clear();
}

#endif //MINER_CORE