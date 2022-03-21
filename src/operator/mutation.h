#pragma once
#include "core/global.h"
#include "core/individual.h"

namespace emoc {

	void PolynomialMutation(Individual **pop, int pop_num, Global *g_GlobalSettings);
	void PolynomialMutation(Individual *ind, Global *g_GlobalSettings);

}