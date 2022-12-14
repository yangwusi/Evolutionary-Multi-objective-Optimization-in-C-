#pragma once
#include "algorithm/algorithm.h"
#include "core/individual.h"
#include "problem/problem.h"

#include <vector>

namespace emoc {

	class MOEADDYTS : public Algorithm
	{
	public:
		typedef struct
		{
			int index;
			double distance;
		}DistanceInfo;  // store euclidian distance to the index-th weight vector

		typedef enum
		{
			NEIGHBOUR,
			GLOBAL
		}NeighbourType;

		MOEADDYTS(int thread_id);
		virtual ~MOEADDYTS();

		void Solve();

	private:
		void Initialization();
		void SetNeighbours();
		void Crossover(int operator_index, Individual** parent_pop, int current_index, Individual* offspring);

		void UpdateUtility();
		void SelectCurrentSubproblem();
		void CalculateFitness(Individual** pop, int pop_num, double* fitness);
		void CreditAssignment();
		int SelectOperator();
		void UpdateBetaDis(int op, double reward);

		// use offspring to update the neighbour of current_index-th individual with specified aggregation function and return fir
		double UpdateSubproblem(Individual* offspring, int current_index);

		

	private:
		// MOEAD basic parameters
		double** lambda_;                  // weight vector
		int weight_num_;                   // the number of weight vector
		int** neighbour_;	               // neighbours of each individual
		int neighbour_num_;                // the number of neighbours
		int replace_num_;                  // the number of maximum replaced individual
		double neighbour_selectpro_;       // the probability of select neighbour scope
		NeighbourType neighbour_type_;
		double* ideal_point_;

		// MOEADDRA parameters
		int* selected_indices_;			   // the indices of selected individuals of current iteration
		int selected_size_;				   // the number of selected individuals
		double* old_obj_;				   // old Tchebycheff function value of each solution on its subproblem
		double* utility_;				   // utility for each subproblem
		double* delta_;					   // difference between new and old individuals' fitness

		// MOEADDYTS parameters
		int operator_num_;				   // the number of operator
		double* alpha_;					   // beta distribution parameter
		double* beta_;					   // beta distribution parameter
		double C;						   // beta distritbution update threshold

	};
}