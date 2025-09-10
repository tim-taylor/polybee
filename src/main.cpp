#include "PolyBeeCore.h"

int main(int argc, char **argv)
{
	PolyBeeCore PolyBeeCore(argc, argv);

	PolyBeeCore.run();

	/*
	if (Params::bVis)
	{
		PolyBeeCore.runWithVis();
	}
	else
	{
		PolyBeeCore.run();
	}
	*/
}
