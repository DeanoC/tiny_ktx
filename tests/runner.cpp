#define CATCH_CONFIG_RUNNER
#include "al2o3_catch2/catch2.hpp"
#include "utils_simple_logmanager/logmanager.h"

int main(int argc, char const *argv[]) {
	auto logger = SimpleLogManager_Alloc();

	auto ret = Catch::Session().run(argc, (char **) argv);

	SimpleLogManager_Free(logger);

	return ret;
}



