/** \file
 * \brief Cluster Planarity tests and Cluster Planar embedding
 * for C-connected Cluster Graphs
 *
 * \author Sebastian Leipert
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/basic.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterPlanarityModule.h>
#include <ogdf/planarity/booth_lueker/PlanarPQTree.h>

namespace ogdf {
template<class E>
class ListPure;

//! C-planarity test by Cohen, Feng and Eades.
/**
 * @ingroup ga-cplanarity
 */
class OGDF_EXPORT CconnectClusterPlanar {
public:
	//aus CCCPE oder CCCP wieder entfernen
	enum class ErrorCode {
		none = 0,
		nonConnected = 1,
		nonCConnected = 2,
		nonPlanar = 3,
		nonCPlanar = 4
	};

	ErrorCode errCode() { return m_errorCode; }

	//! Constructor.
	CconnectClusterPlanar();

	//! Destructor.
	virtual ~CconnectClusterPlanar() { }

	//! Tests if a cluster graph is c-planar.
	virtual bool call(const ClusterGraph& C);

private:
	using PlanarPQTree = booth_lueker::PlanarPQTree;

	//! Recursive planarity test for clustered graph induced by \p act.
	bool planarityTest(ClusterGraph& C, const cluster act, Graph& G);

	//! Preprocessing that initializes data structures, used in call.
	bool preProcess(ClusterGraph& C, Graph& G);

	//! Prepares the planarity test for one cluster.
	bool preparation(Graph& G, const cluster C, node superSink);

	//! Performs a planarity test on a biconnected component.
	bool doTest(Graph& G, NodeArray<int>& numbering, const cluster cl, node superSink,
			EdgeArray<edge>& edgeTable);

	void prepareParallelEdges(Graph& G);

	//! Constructs the replacement wheel graphs
	void constructWheelGraph(ClusterGraph& C, Graph& G, cluster& parent, PlanarPQTree* T,
			EdgeArray<node>& outgoingTable);


	//private Members for handling parallel edges
	EdgeArray<ListPure<edge>> m_parallelEdges;
	EdgeArray<bool> m_isParallel;
	ClusterArray<PlanarPQTree*> m_clusterPQTree;
	int m_parallelCount;

	ErrorCode m_errorCode;
};

class OGDF_EXPORT CconnectClusterPlanarityModule : public ClusterPlanarityModule {
public:
	bool isClusterPlanar(const ClusterGraph& CG) override;
	bool isClusterPlanarDestructive(ClusterGraph& CG, Graph& G) override;
	bool clusterPlanarEmbed(ClusterGraph& CG, Graph& G) override;
	bool clusterPlanarEmbedClusterPlanarGraph(ClusterGraph& CG, Graph& G) override;
};

}
