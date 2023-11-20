#include <cstdint>
#include <string>

void convert::hexString_toBiner (void *, const char *, const size_t) {}
void convert::hexString_toBiner (void *, const std::string, const size_t) {}

//parsing json foward

//parsing json foward end

struct minerpool::mine_data {
	size_t x;
};
minerpool::mine_data minerpool::create_data() {
	return minerpool::minerdata();
}
bool minerpool::mine_data_update(mine_data*, const char*) {
	return false;
}
std::string minerpool::mine_data_extract(mine_data*) {
	return std::string(R"(
		nothing
		)");
}


//parsing json extras



