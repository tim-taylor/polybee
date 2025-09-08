#include "PolyBee.h"

int main(int argc, char **argv)
{
	PolyBee PolyBee(argc, argv);

	if (Params::bVis)
	{
		PolyBee.runWithVis();
	}
	else
	{
		PolyBee.run();
	}
}
