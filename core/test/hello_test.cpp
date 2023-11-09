#include "PoolMiner_Lite.hpp"
#include <cassert>

int main() {
    PoolMiner_Lite::Greeter greeter;
    assert(greeter.greeting().compare("Hello, World!") == 0);
    return 0;
}
