#include "PolyBeeCore.h"
#include "PolyBeeEvolve.h"
#include "Params.h"

int main(int argc, char **argv)
{
	PolyBeeCore polyBeeCore(argc, argv);

	if (Params::bEvolve)
	{
		// run optimization
		PolyBeeEvolve optimizer(polyBeeCore);
		optimizer.evolve();
	}
	else
	{
		// run normal simulation
		polyBeeCore.run();
	}
}
