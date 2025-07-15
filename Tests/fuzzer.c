#include "../c_polygon.h"
#include "fuzzer.h"
#include <stdio.h>
#include <crtdbg.h>



int main(void)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	#define FUZZ_FILEPATH "res/rand.ply"
	printf("-- Begin Fuzzing Checks: --\n");
	printf("\t Full Random: \n");

	for (size_t i = 0; i < 25; ++i) {
		fuzzFullRandom(FUZZ_FILEPATH, 4096);

		struct PlyScene scene;
		enum PlyResult res = PlyLoadFromDisk(FUZZ_FILEPATH, &scene, NULL);
		if (res == PLY_SUCCESS) {
			assert(00 && "sigma");
		}
		printf("res: %s\n", dbgPlyResultToString(res));
		PlyDestroyScene(&scene);
	}

	printf("\t Structured Random: \n");

	for (size_t i = 0; i < 1000; ++i) {
		fuzzStructuredRandom(FUZZ_FILEPATH, 4096);
		struct PlyScene scene;
		enum PlyResult res = PlyLoadFromDisk(FUZZ_FILEPATH, &scene, NULL);
		if (res == PLY_SUCCESS) {
			printf("sigma\n");
		}
		printf("res: %s\n", dbgPlyResultToString(res));
		PlyDestroyScene(&scene);
	}

	printf("Fuzzing Checks Complete");




	/*
	ply
format ascii 1.0
element 88a87b2175c17cbem9d1285a4e10c94a8z03bf13 179
property float 72e186a70f983942xc172a9d25a4b648ex4ce25effa12210 
property char 3d4097d3 
property uint 3e00ada2 
property float f1313ee7a288cc5afaa89916 
property uchar d364c6468d1e5dad058263a51ff937a2f1c5301648f44fbe1cf7c8cf 
property uchar b7976b15253a5939fceb8c2e72639aed 
property float a9e528a20e0812311d71a141ad58f1cbe58268fe11b7e0b15s07bebd 

element 25c2108864448c5e858b1229 71
property uchar ef1ef576053c5a00gd8865f695f3329f2fad512f7727007eeba16d3a8d94738f7ab9a6c37fcf7ea6 

element ea5c8e42 177
property double 53338adff3554ec84fb6ea6d19c657ab179cff0be8635ad9 
property float face3f8c276bef00bcebed6bbc05de6dad3bdd41fdb63a40d1c1fdc3c376f1db 
property uint f324ca7f01b8d195e74d60d9cadfadf8n70476b2ceea242a7jc0fae085203dcb87216a9a 
property double 8c1788c14bfd0a9c2ea2b141e77f1574y5022af83460e4489gebbcf1 
property int 832b82ffc88a00ed0e956a4ca2500c60jf681f7091342b7fdb5c8255fc10f0f37ac0f1be71b1293a 
property double 133be9c7 
property int b2d85178f9495a0dx5ef2f1afad6f7143n42628edb0a418afd6cd0fc86081e67a7gca274 property char f5a59c8603f92550k0709d4d 

header_end 

*/



	return EXIT_SUCCESS;
}