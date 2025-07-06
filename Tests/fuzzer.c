#include "fuzzer.h"
#include "../c_polygon.h"
#include <stdio.h>
#include <crtdbg.h>



int main(void)
{

	for (size_t i = 0; i < 250; ++i) {
		fuzzFullRandom("tests/res/rand.ply", 4096);

		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		struct PlyScene scene;
		enum PlyResult res = PlyLoadFromDisk("Tests/res/rand.ply", &scene);
		printf("res: %s\n", dbgPlyResultToString(res));
		PlyDestroyScene(&scene);
	}

	for (size_t i = 0; i < 250; ++i) {
		fuzzStructuredRandom("tests/res/rand.ply", 4096);

		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		struct PlyScene scene;
		enum PlyResult res = PlyLoadFromDisk("Tests/res/rand.ply", &scene);
		printf("res: %s\n", dbgPlyResultToString(res));
		PlyDestroyScene(&scene);
	}










	return EXIT_SUCCESS;
}