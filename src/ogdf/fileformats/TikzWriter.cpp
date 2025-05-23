/** \file
 * \brief Generator for visualizing graphs using LaTeX/TikZ.
 *
 * \author Hendrik Brueckler
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

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphList.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/Logger.h>
#include <ogdf/basic/Math.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/basic.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/basic/graphics.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/fileformats/TikzWriter.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace ogdf {

using std::string;
using std::stringstream;

static size_t NODE_ALIGNMENT = 30;
static size_t EDGE_ALIGNMENT = 30;

bool TikzWriter::draw(std::ostream& os) {
	if (!os.good() || !m_attr.has(GraphAttributes::nodeGraphics)) {
		return false;
	}

	m_nodeStyles.clear();
	m_edgeStyles.clear();

	bool uniformWidth = m_attr.isUniformForNodes<double>(&GraphAttributes::width);
	bool uniformHeight = m_attr.isUniformForNodes<double>(&GraphAttributes::height);
	bool uniformStyle = m_attr.isUniformForNodes<Shape>(&GraphAttributes::shape)
			&& m_attr.isUniform(GraphAttributes::nodeStyle);

	stringstream mainbody;
	if (m_clsAttr) {
		drawAllClusters(mainbody);
	}
	drawAllNodes(mainbody, uniformStyle, uniformWidth, uniformHeight);
	drawAllEdges(mainbody);
	wrapHeaderFooter(os, mainbody.str(), uniformStyle, uniformWidth, uniformHeight);

	return true;
}

void TikzWriter::wrapHeaderFooter(std::ostream& os, std::string tikzPic, bool uniformStyle,
		bool uniformWidth, bool uniformHeight) const {
	os << "% Generated by ogdf::TikzWriter\n"
	   << "\\documentclass{standalone}\n"
	   << "\\usepackage{tikz}\n"
	   << "\\usetikzlibrary{shapes, arrows.meta, decorations.markings}\n"
	   << "\n"
	   << "\\begin{document}\n"
	   << "\\begin{tikzpicture}%\n"
	   << "%%%%%%%%%%%%%%%%%%%%%%%%%%%\n"
	   << "%%%%%%%% TIKZ STYLES %%%%%%\n"
	   << "%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
	GraphIO::indent(os, 1) << "[yscale = -1.0,\n";
	GraphIO::indent(os, 1) << "width/.style = {minimum width = #1},\n";
	GraphIO::indent(os, 1) << "height/.style = {minimum height = #1},\n";
	GraphIO::indent(os, 1) << "size/.style = {minimum width = #1, minimum height = #1},\n";
	GraphIO::indent(os, 1)
			<< "nodelabel/.style args={#1:#2}"
			<< "{label={[text width = #1, align = center, label position = center]#2}},\n";
	GraphIO::indent(os, 1)
			<< "shiftednodelabel/.style args={#1:#2:#3:#4}"
			<< "{label={[text width = #1, xshift = #2, yshift = #3, align = center, label position = center]#4}},\n";
	GraphIO::indent(os, 1)
			<< "edgelabel/.style args={#1:#2}"
			<< "{postaction={decorate, decoration={markings,mark=at position 0.5 with \\node[{draw=none, fill = none, #1}]{#2};}}},\n";
	GraphIO::indent(os, 1) << "> = {Latex[angle=60:{" << texLength(calcArrowSize()) << "} 1]},\n";
	int num = 0;
	for (const string& nodeStyle : m_nodeStyles) {
		GraphIO::indent(os, 1) << "nodestyle" << num++ << "/.style = {" << nodeStyle << "},\n";
	}
	num = 0;
	for (const string& edgeStyle : m_edgeStyles) {
		GraphIO::indent(os, 1) << "edgestyle" << num++ << "/.style = {" << edgeStyle << "},\n";
	}
	const Graph& G = m_attr.constGraph();
	if (G.numberOfNodes() != 0 && (uniformStyle || uniformWidth || uniformHeight)) {
		node first = *G.nodes.begin();
		string globalNodeProps("");
		if (uniformStyle) {
			globalNodeProps += "nodestyle0, ";
		}
		if (uniformWidth) {
			globalNodeProps += "width = " + texLength(m_attr.width(first)) + ", ";
		}
		if (uniformHeight) {
			globalNodeProps += "height = " + texLength(m_attr.height(first)) + ", ";
		}
		GraphIO::indent(os, 1) << "every node/.append style = {" << globalNodeProps << "},\n";
	}
	GraphIO::indent(os, 1) << "]\n"
						   << tikzPic << "\\end{tikzpicture}\n"
						   << "\\end{document}\n"
						   << std::endl;
}

void TikzWriter::drawAllClusters(std::ostream& os) {
	OGDF_ASSERT(m_clsAttr != nullptr);
	os << "%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
	os << "%%%%% CLUSTER SECTION %%%%%\n";
	os << "%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";

	Queue<cluster> queue;
	queue.append(m_clsAttr->constClusterGraph().rootCluster());

	while (!queue.empty()) {
		cluster c = queue.pop();
		drawCluster(os, c);

		for (cluster child : c->children) {
			queue.append(child);
		}
	}
}

void TikzWriter::drawAllNodes(std::ostream& os, bool uniformStyle, bool uniformWidth,
		bool uniformHeight) {
	os << "%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
	os << "%%%%%% NODES SECTION %%%%%%\n";
	os << "%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
	const Graph& G(m_attr.constGraph());
	for (node v : G.nodes) {
		drawNode(os, v, uniformStyle, uniformWidth, uniformHeight);
	}
}

void TikzWriter::drawAllEdges(std::ostream& os) {
	os << "%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
	os << "%%%%%% EDGES SECTION %%%%%%\n";
	os << "%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
	const Graph& G(m_attr.constGraph());
	for (edge e : G.edges) {
		drawEdge(os, e);
	}
}

void TikzWriter::drawCluster(std::ostream& os, cluster c) {
	OGDF_ASSERT(m_clsAttr != nullptr);
	OGDF_ASSERT(c != nullptr);

	if (c == m_clsAttr->constClusterGraph().rootCluster()
			|| !m_clsAttr->has(ClusterGraphAttributes::clusterGraphics)) {
		return;
	}

	string nodeStyle = "rectangle, " + getClusterStyle(c);
	size_t nodeStyleNum =
			std::find(m_nodeStyles.begin(), m_nodeStyles.end(), nodeStyle) - m_nodeStyles.begin();
	if (nodeStyleNum == m_nodeStyles.size()) {
		m_nodeStyles.emplace_back(nodeStyle);
	}

	stringstream clusterProperties;
	clusterProperties << "nodeStyle" << nodeStyleNum << ", "
					  << "width = " << texLength(m_clsAttr->width(c)) << ", "
					  << "height = " << texLength(m_clsAttr->height(c)) << ", ";
	if (m_clsAttr->has(ClusterGraphAttributes::clusterLabel) && m_clsAttr->label(c).empty()) {
		clusterProperties << "label = {center: " << m_clsAttr->label(c) << "}, ";
	}

	string properties = clusterProperties.str().substr(0, clusterProperties.str().size() - 2);
	GraphIO::indent(os, 1) << "\\node[" << properties << "]";
	if (properties.size() > NODE_ALIGNMENT) {
		os << "\n";
		GraphIO::indent(os, 2);
	} else {
		os << string(NODE_ALIGNMENT - properties.size(), ' ');
	}

	os << "(Cluster" << c->index() << ") at (" << texLength(m_clsAttr->x(c)) << ", "
	   << texLength(m_clsAttr->y(c)) << ") {};\n";
}

void TikzWriter::drawNode(std::ostream& os, node v, bool uniformStyle, bool uniformWidth,
		bool uniformHeight) {
	OGDF_ASSERT(v != nullptr);

	stringstream nodeProperties;
	size_t nodeStyleNum = 0;
	if (!uniformStyle || m_nodeStyles.empty()) {
		string nodeStyle = getNodeShape(v) + ", " + getNodeStyle(v);
		nodeStyleNum = std::find(m_nodeStyles.begin(), m_nodeStyles.end(), nodeStyle)
				- m_nodeStyles.begin();
		if (nodeStyleNum == m_nodeStyles.size()) {
			m_nodeStyles.emplace_back(nodeStyle);
		}
	}

	if (!uniformStyle) {
		nodeProperties << "nodestyle" << nodeStyleNum << ", ";
	}
	if (!uniformWidth && !uniformHeight && m_attr.width(v) == m_attr.height(v)) {
		nodeProperties << "size = " << texLength(m_attr.width(v)) << ", ";
	} else {
		if (!uniformWidth) {
			nodeProperties << "width = " << texLength(m_attr.width(v)) << ", ";
		}
		if (!uniformHeight) {
			nodeProperties << "height = " << texLength(m_attr.height(v)) << ", ";
		}
	}
	if (m_attr.has(GraphAttributes::nodeLabel) && !m_attr.label(v).empty()) {
		nodeProperties << getNodeLabel(v) << ", ";
	}

	string properties = nodeProperties.str().substr(0, nodeProperties.str().size() - 2);
	GraphIO::indent(os, 1) << "\\node[" + properties + "]";
	if (properties.size() > NODE_ALIGNMENT) {
		os << "\n";
		GraphIO::indent(os, 2);
	} else {
		os << string(NODE_ALIGNMENT - properties.size(), ' ');
	}

	os << "(Node" << v->index() << ") at (" << texLength(m_attr.x(v)) << ", "
	   << texLength(m_attr.y(v)) << ") {};\n";
}

void TikzWriter::drawEdge(std::ostream& os, edge e) {
	OGDF_ASSERT(e != nullptr);
	OGDF_ASSERT(e->source() != nullptr && e->target() != nullptr);

	node source = e->source();
	node target = e->target();

	DPolyline edgeLine;
	List<string> bendPointStrings;
	if (m_attr.has(GraphAttributes::edgeGraphics) && !m_attr.bends(e).empty()) {
		edgeLine = m_attr.bends(e);
		for (DPoint bendPoint : m_attr.bends(e)) {
			bendPointStrings.pushBack(
					"(" + texLength(bendPoint.m_x) + ", " + texLength(bendPoint.m_y) + ")");
		}
	}
	// If no bendpoint inside source node -> snap line end to source node border
	DPoint vSize = DPoint(m_attr.width(source), m_attr.height(source));
	if (edgeLine.size() == 0
			|| !isPointCoveredByNode(*edgeLine.get(0), m_attr.point(source), vSize,
					m_attr.shape(source))) {
		bendPointStrings.pushFront("(Node" + std::to_string(source->index()) + ")");
		edgeLine.pushFront(m_attr.point(source));
	}
	// If no bendpoint inside target node -> snap line end to target node border
	vSize = DPoint(m_attr.width(target), m_attr.height(target));
	if (edgeLine.size() == 0
			|| !isPointCoveredByNode(*edgeLine.get(edgeLine.size() - 1), m_attr.point(target),
					vSize, m_attr.shape(target))) {
		bendPointStrings.pushBack("(Node" + std::to_string(target->index()) + ")");
		edgeLine.pushBack(m_attr.point(target));
	}
	DPoint midPoint = edgeLine.position(0.5);
	double midLength = edgeLine.length() * 0.5;

	string edgeStyle = getEdgeStyle(e);
	size_t edgeStyleNum =
			std::find(m_edgeStyles.begin(), m_edgeStyles.end(), edgeStyle) - m_edgeStyles.begin();
	if (edgeStyleNum == m_edgeStyles.size()) {
		m_edgeStyles.emplace_back(edgeStyle);
	}

	string path;
	string label;
	bool drawLabel = m_attr.has(GraphAttributes::edgeLabel) && !m_attr.label(e).empty();
	double lengthPassed = 0;
	OGDF_ASSERT(edgeLine.size() == bendPointStrings.size());
	// Piece together path from bendpoints and orient edge label according to slope
	for (int i = 0; i < edgeLine.size() - 1; i++) {
		path += *bendPointStrings.get(i) + " -- ";
		if (drawLabel) {
			lengthPassed += (*edgeLine.get(i + 1) - *edgeLine.get(i)).norm();
			if (lengthPassed >= midLength) {
				drawLabel = false;
				label = getEdgeLabel(e, *edgeLine.get(i + 1), midPoint);
			}
		}
	}
	path += bendPointStrings.back();
	string edgeProperties = getEdgeArrows(e) + ", edgestyle" + std::to_string(edgeStyleNum);
	if (!label.empty()) {
		edgeProperties += ", " + label;
	}

	GraphIO::indent(os, 1) << "\\path[" << edgeProperties << "]";
	if (edgeProperties.length() > EDGE_ALIGNMENT) {
		os << "\n";
		GraphIO::indent(os, 2);
	} else {
		os << string(EDGE_ALIGNMENT - edgeProperties.length(), ' ');
	}
	os << path << ";\n";
}

std::string TikzWriter::getClusterStyle(cluster c) const {
	OGDF_ASSERT(c != nullptr);

	if (!m_clsAttr->has(ClusterGraphAttributes::clusterStyle)) {
		return "draw";
	}

	string lineStyle = getLineStyle(m_clsAttr->strokeType(c), m_clsAttr->strokeWidth(c),
			m_clsAttr->strokeColor(c));
	if (m_clsAttr->fillPattern(c) != FillPattern::None) {
		return lineStyle + ", fill = " + getColorString(m_clsAttr->fillColor(c));
	}
	return lineStyle;
}

std::string TikzWriter::getNodeShape(node v) const {
	OGDF_ASSERT(v != nullptr);

	auto logPolygonWarning = [&]() {
		if (m_attr.width(v) != m_attr.height(v)) {
			Logger::slout() << "TikZ: Warning! "
							<< "Non-regular polygon node shape currently not implemented! "
							<< "Diameter of polygon in x and y direction will be max(width, height)!"
							<< std::endl;
		}
	};

	switch (m_attr.shape(v)) {
	case Shape::Rect:
		return "rectangle";
	case Shape::RoundedRect:
		return "rounded corners";
	case Shape::Ellipse:
		return "ellipse";
	case Shape::Triangle:
		return "isosceles triangle, shape border rotate = 90, isosceles triangle stretches=true";
	case Shape::InvTriangle:
		return "isosceles triangle, shape border rotate = 270, isosceles triangle stretches=true";
	case Shape::Rhomb:
		return "diamond";
	case Shape::Trapeze:
		return "trapezium, trapezium angle = 60, trapezium stretches";
	case Shape::InvTrapeze:
		return "trapezium, trapezium angle = 60, shape border rotate = 180, trapezium stretches";
	case Shape::Parallelogram:
		return "trapezium, trapezium left angle = 60, trapezium right angle = 120, trapezium stretches";
	case Shape::InvParallelogram:
		return "trapezium, trapezium left angle = 120, trapezium right angle = 60, trapezium stretches";
	// Can't support independent width and height for these shapes properly, as it would require more
	// sophisticated drawing of nodes, edges and arrowheads, which would mean less readable and editable tikz code
	case Shape::Pentagon:
		logPolygonWarning();
		return "regular polygon, regular polygon sides=5";
	case Shape::Hexagon:
		logPolygonWarning();
		return "regular polygon, regular polygon sides=6";
	case Shape::Octagon:
		logPolygonWarning();
		return "regular polygon, regular polygon sides=8";
	default:
		return "rectangle";
	}
}

std::string TikzWriter::getNodeStyle(node v) const {
	OGDF_ASSERT(v != nullptr);

	if (!m_attr.has(GraphAttributes::nodeStyle)) {
		return "draw";
	}

	string lineStyle =
			getLineStyle(m_attr.strokeType(v), m_attr.strokeWidth(v), m_attr.strokeColor(v));
	if (m_attr.fillPattern(v) != FillPattern::None) {
		return lineStyle + ", fill = " + getColorString(m_attr.fillColor(v));
	}
	return lineStyle;
}

std::string TikzWriter::getNodeLabel(node v) const {
	OGDF_ASSERT(v != nullptr);

	if (!m_attr.has(GraphAttributes::nodeLabel)) {
		return "";
	}

	stringstream ss;
	if (m_attr.has(GraphAttributes::nodeLabelPosition)
			&& (m_attr.xLabel(v) != 0.0 || m_attr.yLabel(v) != 0.0)) {
		ss << "shiftednodelabel = {" << texLength(getTextWidth(v)) << ": "
		   << texLength(m_attr.xLabel(v)) << ": " << texLength(m_attr.yLabel(v)) << ": "
		   << m_attr.label(v) << "}";
	} else {
		ss << "nodelabel = {" << texLength(getTextWidth(v)) << ": " << m_attr.label(v) << "}";
	}

	return ss.str();
}

double TikzWriter::getTextWidth(node v) const {
	// These values are not exactly computed but for most cases should assure
	// that node labels stay within the node boundary (if not too many lines)
	switch (m_attr.shape(v)) {
	case Shape::Rect:
	case Shape::RoundedRect:
		return 0.9 * m_attr.width(v);
	case Shape::Octagon:
	case Shape::Ellipse:
		return 0.8 * m_attr.width(v);
	case Shape::Rhomb:
	case Shape::Pentagon:
	case Shape::Hexagon:
		return 0.7 * m_attr.width(v);
	case Shape::Trapeze:
	case Shape::InvTrapeze:
	case Shape::Parallelogram:
	case Shape::InvParallelogram:
		return 0.55 * m_attr.width(v);
	case Shape::Triangle:
	case Shape::InvTriangle:
	default:
		return 0.35 * m_attr.width(v);
	}
}

std::string TikzWriter::getEdgeStyle(edge e) const {
	OGDF_ASSERT(e != nullptr);

	if (!m_attr.has(GraphAttributes::edgeStyle)) {
		return "draw";
	}

	return getLineStyle(m_attr.strokeType(e), m_attr.strokeWidth(e), m_attr.strokeColor(e));
}

std::string TikzWriter::getEdgeArrows(edge e) const {
	if (m_attr.has(GraphAttributes::edgeArrow)) {
		switch (m_attr.arrowType(e)) {
		case EdgeArrow::Last:
			return "->";
		case EdgeArrow::First:
			return "<-";
		case EdgeArrow::Both:
			return "<->";
		default:
			return "-";
		}
	} else if (m_attr.directed()) {
		return "->";
	}

	return "-";
}

std::string TikzWriter::getEdgeLabel(edge e, const DPoint& previousPoint,
		const DPoint& labelPoint) const {
	OGDF_ASSERT(e != nullptr);

	if (!m_attr.has(GraphAttributes::edgeLabel)) {
		return "";
	}
	string relPos;
	DPoint delta = labelPoint - previousPoint;
	double angle = atan2(-delta.m_y, delta.m_x);
	switch ((int)round(angle / (Math::pi / 4.0))) {
	case 0:
		relPos = "below";
		break;
	case 1:
		relPos = "below right";
		break;
	case 2:
		relPos = "right";
		break;
	case 3:
		relPos = "above right";
		break;
	case 4:
	case -4:
		relPos = "above";
		break;
	case -3:
		relPos = "above left";
		break;
	case -2:
		relPos = "left";
		break;
	case -1:
		relPos = "below left";
		break;
	}

	return "edgelabel={" + relPos + ": " + m_attr.label(e) + "}";
}

double TikzWriter::calcArrowSize() const {
	using std::min;
	double minSize = std::numeric_limits<double>::max();
	for (node v : m_attr.constGraph().nodes) {
		if (v->degree() != 0) {
			double degreeFrac = 1.0 / max(3, v->degree());
			double nodeSize = min(m_attr.width(v), m_attr.height(v));
			minSize = min(nodeSize * degreeFrac, minSize);
		}
	}
	for (edge e : m_attr.constGraph().edges) {
		double edgeLength = (m_attr.point(e->source()) - m_attr.point(e->target())).norm();
		minSize = min(edgeLength * 0.25, minSize);
	}
	double bboxSize = hypot(m_attr.boundingBox().width(), m_attr.boundingBox().height());
	return min(0.05 * bboxSize, minSize);
}

std::string TikzWriter::texLength(double f) const {
	std::stringstream formatter;
	formatter << std::fixed << std::setprecision(4) << f;
	std::string texf = formatter.str();
	texf.erase(texf.find_last_not_of('0') + 1, std::string::npos);
	texf.erase(texf.find_last_not_of('.') + 1, std::string::npos);
	std::string unit;
	switch (m_unit) {
	case LengthUnit::PT:
		return texf + "pt";
	case LengthUnit::MM:
		return texf + "mm";
	case LengthUnit::CM:
		return texf + "cm";
	case LengthUnit::IN:
		return texf + "in";
	case LengthUnit::EM:
		return texf + "em";
	case LengthUnit::EX:
		return texf + "ex";
	case LengthUnit::MU:
		return texf + "mu";
	default:
		return texf + "pt";
	}
}

std::string TikzWriter::getLineStyle(StrokeType strokeType, double strokeWidth,
		Color strokeColor) const {
	string lineType;

	switch (strokeType) {
	case StrokeType::None:
		lineType = "";
		break;
	case StrokeType::Solid:
		lineType = "solid";
		break;
	case StrokeType::Dash:
		lineType = "dashed";
		break;
	case StrokeType::Dot:
		lineType = "dotted";
		break;
	case StrokeType::Dashdot:
		lineType = "dash dot";
		break;
	case StrokeType::Dashdotdot:
		lineType = "dash dot dot";
		break;
	default:
		lineType = "solid";
	}

	if (!lineType.empty()) {
		return "draw = " + getColorString(strokeColor) + ", " + lineType
				+ ", line width = " + texLength(strokeWidth);
	} else {
		return "draw = none";
	}
}

std::string TikzWriter::getColorString(Color c) {
	return "{rgb, 255: red," + std::to_string(c.red()) + "; green," + std::to_string(c.green())
			+ "; blue," + std::to_string(c.blue()) + "}";
}

}
