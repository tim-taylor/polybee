// This file is from the FastEMD library by Ofir Pele
// available at https://github.com/ofirpele/FastEMD
//
// It is distributed with polybee under the conditions of the licence
// of the FastEMD package, which is as follows:
/*
IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.

This license is for all code in this distribution except: demo_FastEMD4/deltaE2000.hpp which is a
C++ version of Garuv Sharma's Matlab code which can be found here:
http://www.ece.rochester.edu/~gsharma/ciede2000/dataNprograms/deltaE2000.m

By downloading, copying, installing or using the software you agree to this license.  If you do not
agree to this license, do not download, install, copy or use the software.

LICENSE CONDITIONS

Copyright (c) 2009-2012, Ofir Pele
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of the The Hebrew University of Jerusalem nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _LEMON_THRESHOLDED_EMD_H_
#define _LEMON_THRESHOLDED_EMD_H_

//#include "OP_utils.hpp"
//#include "tictoc.hpp"
#include <cassert>

#include <vector>
using namespace std;

// https://lemon.cs.elte.hu/trac/lemon/wiki/Downloads
#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/network_simplex.h>
#include <lemon/cost_scaling.h>
#include <lemon/capacity_scaling.h>
#include <lemon/cycle_canceling.h>
using namespace lemon;

int lemon_thresholded_emd(const std::vector<int>& P,
			  const std::vector<int>& Q,
			  const std::vector< std::vector<size_t> >& C_inds,
			  const std::vector< std::vector<int> >& C_vals,
			  int thresh) {

	//------------------------------------------------------------
	typedef int numType;
	typedef lemon::SmartDigraph t_graph;
	typedef t_graph::Arc t_arc;
	typedef t_graph::Node t_node;
	typedef t_graph::NodeMap<numType> t_nodes_map;
	typedef t_graph::ArcMap<numType> t_arcs_map;
	//typedef NetworkSimplex<t_graph,numType> t_solver; // fast?
	typedef CostScaling<t_graph,numType> t_solver; // fastest?
	//typedef CapacityScaling<t_graph,numType> t_solver;
	//typedef CycleCanceling<t_graph,numType> t_solver;
	//------------------------------------------------------------


	//------------------------------------------------------------
	t_graph g;
	t_nodes_map nodes_supply(g);
	t_arcs_map arcs_costs(g);
	t_arcs_map arcs_capacity_lower_bounds(g);
	t_arcs_map arcs_capacity_upper_bounds(g);
	//------------------------------------------------------------


	//------------------------------------------------------------
	size_t total_neighbors_num= 0;
	for (size_t i=0; i<P.size(); ++i) {
		total_neighbors_num+= C_inds[i].size();
		//mc_assert_eq(C_vals[i].size(),C_inds[i].size());
		assert(C_vals[i].size()==C_inds[i].size());
	}
	//------------------------------------------------------------


	//------------------------------------------------------------
	// sources + targets + transhipment
	const size_t TRANSHIPMENT= P.size()+Q.size();
	vector<t_node> nodes(P.size()+Q.size() + 1);
	size_t nodes_i= 0;
	for (size_t i=0; i<P.size(); ++i) {
		nodes[nodes_i]= g.addNode();
		nodes_supply[nodes[nodes_i]]= P[i];
		++nodes_i;
	}
	for (size_t i=0; i<Q.size(); ++i) {
		nodes[nodes_i]= g.addNode();
		nodes_supply[nodes[nodes_i]]= -Q[i];
		++nodes_i;
	}
	nodes[nodes_i]= g.addNode(); // transhipment
	nodes_supply[nodes[nodes_i]]= 0;
	//------------------------------------------------------------


	//------------------------------------------------------------
	vector<t_arc> arcs(P.size() + Q.size() + total_neighbors_num);
	size_t arcs_i= 0;
	// from sources to transhipment
	for (size_t i=0; i<P.size(); ++i) {
		arcs[arcs_i]= g.addArc(nodes[i], nodes[TRANSHIPMENT]);
		arcs_costs[arcs[arcs_i]]= thresh;
		arcs_capacity_lower_bounds[arcs[arcs_i]]= 0;
		// can be also std::numeric_limits<numType>::infinity();
		arcs_capacity_upper_bounds[arcs[arcs_i]]= P[i];
		++arcs_i;
	}
	// from transhipment to targets
	for (size_t i=0; i<Q.size(); ++i) {
		arcs[arcs_i]= g.addArc(nodes[TRANSHIPMENT], nodes[P.size()+i]);
		arcs_costs[arcs[arcs_i]]= 0;
		arcs_capacity_lower_bounds[arcs[arcs_i]]= 0;
		// can be also std::numeric_limits<numType>::infinity();
		arcs_capacity_upper_bounds[arcs[arcs_i]]= Q[i];
		++arcs_i;
	}
	// from sources to targets
	for (size_t i=0; i<P.size(); ++i){
		for (size_t ji=0; ji<C_inds[i].size(); ++ji) {
			size_t j= C_inds[i][ji];
			arcs[arcs_i]= g.addArc(nodes[i], nodes[P.size()+j]);
			arcs_costs[arcs[arcs_i]]= C_vals[i][ji];
			arcs_capacity_lower_bounds[arcs[arcs_i]]= 0;
			arcs_capacity_upper_bounds[arcs[arcs_i]]= std::min(P[i],Q[j]);
			++arcs_i;
		} // ji
	} // i

	//mc_assert_eq(arcs_i,arcs.size());
	assert(arcs_i==arcs.size());

	//------------------------------------------------------------



	//------------------------------------------------------------
	t_solver mcf_solver(g);

	mcf_solver.lowerMap(arcs_capacity_lower_bounds);
	mcf_solver.upperMap(arcs_capacity_upper_bounds);
	mcf_solver.supplyMap(nodes_supply);
	mcf_solver.costMap(arcs_costs);

//	tictoc t;
//	t.tic();
	//mcf_solver.run(t_solver::ALTERING_LIST);
	mcf_solver.run();
//	t.toc();
	//cout << "solver time== " <<t.totalTimeSec() << endl;

	return mcf_solver.totalCost();
	//------------------------------------------------------------

}
#endif
