#include <ogdf/basic/Graph.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/planarity/PlanRep.h>
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>

#include <iostream>
#include <string>

using namespace ogdf;

int main() {
	Graph G;
	if (!GraphIO::read(G, "input.gml")) {
		std::cerr << "Could not load input.gml" << std::endl;
		return 1;
	}

	auto PL = new PlanarSubgraphFast<int>;
	PL->runs(10);

	List<edge> delEdges;
	PL->call(G, delEdges);

	for (edge& eDel : delEdges) {
		std::cout << eDel << std::endl;
	}

	// GraphIO::write(G, "done", GraphIO::writeGML);
	// std::cout << "wrote to output.gml" << std::endl;
	return 0;
}
