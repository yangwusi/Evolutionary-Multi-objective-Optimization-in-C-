#include <ctime>
#include <iostream>
#include <memory>
#include <algorithm>
#include <vector>

#include "core/global.h"
#include "core/individual.h"
#include "problem/zdt.h"
#include "problem/dtlz.h"
#include "algorithms/moead/moead.h"
#include "algorithms/moead_de/moead_de.h"
#include "algorithms/nsga2/nsga2.h"
#include "algorithms/ibea/ibea.h"
#include "algorithms/spea2/spea2.h"
#include "algorithms/smsemoa/smsemoa.h"
#include "algorithms/moead_dra/moead_dra.h"
#include "algorithms/moead_frrmab/moead_frrmab.h"
#include "algorithms/hype/hype.h"
#include "metric/hv.h"
#include "metric/igd.h"
#include "metric/gd.h"
#include "metric/spacing.h"
#include "random/random.h"


using emoc::g_GlobalSettings;

int main()
{		
	double igd_sum = 0;
	// initilize some bases for random number
	randomize();
	for (int i = 0; i < 1; ++i)
	{
		// run time recording
		clock_t start, end;
		start = clock();

		// algorithm main entity
		g_GlobalSettings = new emoc::Global("moeaddra", "zdt4", 200, 30, 2, 200000);
		g_GlobalSettings->Start();

		end = clock();
		double time = (double)(end - start) / CLOCKS_PER_SEC;

		double igd = emoc::CalculateIGD(g_GlobalSettings->parent_population_.data(), g_GlobalSettings->population_num_);
		double gd = emoc::CalculateGD(g_GlobalSettings->parent_population_.data(), g_GlobalSettings->population_num_);
		double hv = emoc::CalculateHV(g_GlobalSettings->parent_population_.data(), g_GlobalSettings->population_num_);
		double spacing = emoc::CalculateSpacing(g_GlobalSettings->parent_population_.data(), g_GlobalSettings->population_num_);

		printf("igd : %f\n", igd);
		printf("hv : %f\n", hv);
		printf("gd : %f\n", gd);
		printf("spacing : %f\n", spacing);
		printf("runtime : %fs\n", time);

		delete g_GlobalSettings;
	}

	// printf("mean igd : %f\n", igd_sum/10.0);



	getchar();
	return 0;
}