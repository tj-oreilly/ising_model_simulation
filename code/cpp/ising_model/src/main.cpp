#include <ctime>
#include <chrono>
#include <iostream>
#include <string>
#include "spin_grid.h"

int main()
{
	uint64_t seed = std::time(nullptr); //Use time as seed
	SpinGrid sp(10, 10, seed);

	auto clock = std::chrono::system_clock::now();
	int64_t start = clock.time_since_epoch() / std::chrono::microseconds(1);

	for (int i = 0; i < 1000000; ++i)
		sp.Iterate();

	clock = std::chrono::system_clock::now();
	int64_t end = clock.time_since_epoch() / std::chrono::microseconds(1);

	std::string out = "Time taken: " + std::to_string((end - start) / 1000000.0);
	std::cout << out << std::endl;

	return 0;
}