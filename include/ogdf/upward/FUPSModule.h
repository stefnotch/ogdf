/** \file
 * \brief Declaration of Feasible Upward Planar Subgraph (FUPS) Module, an interface for subgraph computation.
 *
 * \author Hoi-Ming Wong
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
#include <ogdf/basic/Module.h>
#include <ogdf/basic/basic.h>
#include <ogdf/basic/memory.h>

namespace ogdf {
class UpwardPlanRep;
template<class E>
class List;

/**
 * \brief Interface for feasible upward planar subgraph algorithms.
 *
 */
class OGDF_EXPORT FUPSModule : public Module {
public:
	//! Initializes a feasible upward planar subgraph module.
	FUPSModule() { }

	// destruction
	virtual ~FUPSModule() { }

	/**
	 * \brief Computes a feasible upward planar subgraph of the input graph.
	 *
	 * @param UPR represents the feasible upward planar subgraph after the call. \p UPR has to be initialzed as a
	 *		  UpwardPlanRep of the input graph \p G and is modified.
	 *		  The subgraph is represented as an upward planar representation.
	 * @param delEdges is the list of deleted edges which are deleted from the input graph in order to obtain the subgraph.
	 *		  The edges are edges of the original graph of UPR.
	 * \return the status of the result.
	 */
	ReturnType call(UpwardPlanRep& UPR, List<edge>& delEdges) { return doCall(UPR, delEdges); }

	//! Computes a upward planarized representation of the input graph (shorthand for call)
	ReturnType operator()(UpwardPlanRep& UPR, List<edge>& delEdges) { return call(UPR, delEdges); }


protected:
	/**
	 * \brief Computes a feasible upward planar subgraph of the input graph.
	 *
	 * @param UPR represents the feasible upward planar subgraph after the call. \p UPR has to be initialzed as a
	 *        UpwardPlanRep of the input graph \p G and is modified.
	 *		  The subgraph is represented as an upward planar representation.
	 * @param delEdges is the list of deleted edges which are deleted from the input graph in order to obtain the subgraph.
	 *		  The edges are edges of the original graph G.
	 * \return the status of the result.
	 */
	virtual ReturnType doCall(UpwardPlanRep& UPR, List<edge>& delEdges) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
