// PythonStuff.cpp
// Copyright 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "PythonStuff.h"

#include "Area.h"
#include "Point.h"
#include "AreaDxf.h"

#if _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#ifdef __GNUG__
#pragma implementation
#endif

#include <boost/progress.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/wrapper.hpp>
#include <boost/python/call.hpp>

#include "clipper.hpp"
using namespace clipper;


namespace bp = boost::python;

boost::python::list getVertices(const CCurve& curve) {
	boost::python::list vlist;
	BOOST_FOREACH(const CVertex& vertex, curve.m_vertices) {
		vlist.append(vertex);
    }
	return vlist;
}

boost::python::list getCurves(const CArea& area) {
	boost::python::list clist;
	BOOST_FOREACH(const CCurve& curve, area.m_curves) {
		clist.append(curve);
    }
	return clist;
}

static void print_curve(const CCurve& c)
{
	unsigned int nvertices = c.m_vertices.size();
	printf("number of vertices = %d\n", nvertices);
	int i = 0;
	for(std::list<CVertex>::const_iterator It = c.m_vertices.begin(); It != c.m_vertices.end(); It++, i++)
	{
		const CVertex& vertex = *It;
		printf("vertex %d type = %d, x = %g, y = %g", i+1, vertex.m_type, vertex.m_p.x / CArea::m_units, vertex.m_p.y / CArea::m_units);
		if(vertex.m_type)printf(", xc = %g, yc = %g", vertex.m_c.x / CArea::m_units, vertex.m_c.y / CArea::m_units);
		printf("\n");
	}
}

static void print_area(const CArea &a)
{
	for(std::list<CCurve>::const_iterator It = a.m_curves.begin(); It != a.m_curves.end(); It++)
	{
		const CCurve& curve = *It;
		print_curve(curve);
	}
}

static unsigned int num_vertices(const CCurve& curve)
{
	return curve.m_vertices.size();
}

static CVertex FirstVertex(const CCurve& curve)
{
	return curve.m_vertices.front();
}

static CVertex LastVertex(const CCurve& curve)
{
	return curve.m_vertices.back();
}

static void set_units(double units)
{
	CArea::m_units = units;
}

static double get_units()
{
	return CArea::m_units;
}

static bool holes_linked()
{
	return CArea::HolesLinked();
}

static CArea AreaFromDxf(const char* filepath)
{
	CArea area;
	AreaDxfRead dxf(&area, filepath);
	dxf.DoRead();
	return area;
}

static void append_point(CCurve& c, const Point& p)
{
	c.m_vertices.push_back(CVertex(p));
}

boost::python::list MakePocketToolpath(const CArea& a, double tool_radius, double extra_offset, double stepover, bool from_center, bool use_zig_zag, double zig_angle)
{
	std::list<CCurve> toolpath;

	CAreaPocketParams params(tool_radius, extra_offset, stepover, from_center, use_zig_zag ? ZigZagPocketMode : SpiralPocketMode, zig_angle);
	a.SplitAndMakePocketToolpath(toolpath, params);

	boost::python::list clist;
	BOOST_FOREACH(const CCurve& c, toolpath) {
		clist.append(c);
    }
	return clist;
}

boost::python::list SplitArea(const CArea& a)
{
	std::list<CArea> areas;
	a.Split(areas);

	boost::python::list alist;
	BOOST_FOREACH(const CArea& a, areas) {
		alist.append(a);
    }
	return alist;
}

void dxfArea(CArea& area, const char* str)
{
	area = CArea();
}

boost::python::list getCurveSpans(const CCurve& c)
{
	boost::python::list span_list;
	const Point *prev_p = NULL;

	for(std::list<CVertex>::const_iterator VIt = c.m_vertices.begin(); VIt != c.m_vertices.end(); VIt++)
	{
		const CVertex& vertex = *VIt;

		if(prev_p)
		{
			span_list.append(Span(*prev_p, vertex));
		}
		prev_p = &(vertex.m_p);
	}

	return span_list;
}

Span getFirstCurveSpan(const CCurve& c)
{
	if(c.m_vertices.size() < 2)return Span();

	std::list<CVertex>::const_iterator VIt = c.m_vertices.begin();
	const Point &p = (*VIt).m_p;
	VIt++;
	return Span(p, *VIt, true);
}

Span getLastCurveSpan(const CCurve& c)
{
	if(c.m_vertices.size() < 2)return Span();

	std::list<CVertex>::const_reverse_iterator VIt = c.m_vertices.rbegin();
	const CVertex &v = (*VIt);
	VIt++;

	return Span((*VIt).m_p, v, c.m_vertices.size() == 2);
}

bp::tuple TangentialArc(const Point &p0, const Point &p1, const Point &v0)
{
  Point c;
  int dir;
  tangential_arc(p0, p1, v0, c, dir);

  return bp::make_tuple(c, dir);
}

BOOST_PYTHON_MODULE(area) {
	bp::class_<Point>("Point") 
        .def(bp::init<double, double>())
        .def(bp::init<Point>())
        .def(bp::other<double>() * bp::self)
        .def(bp::self * bp::other<double>())
        .def(bp::self / bp::other<double>())
        .def(bp::self * bp::other<Point>())
        .def(bp::self - bp::other<Point>())
        .def(bp::self + bp::other<Point>())
        .def(bp::self ^ bp::other<Point>())
        .def(bp::self == bp::other<Point>())
        .def(bp::self != bp::other<Point>())
        .def(-bp::self)
        .def(~bp::self)
        .def("dist", &Point::dist)
        .def("length", &Point::length)
        .def("normalize", &Point::normalize)
		.def("Rotate", static_cast< void (Point::*)(double, double) >(&Point::Rotate))
		.def("Rotate", static_cast< void (Point::*)(double) >(&Point::Rotate))
        .def_readwrite("x", &Point::x)
        .def_readwrite("y", &Point::y)
    ;

	bp::class_<CVertex>("Vertex") 
        .def(bp::init<CVertex>())
        .def(bp::init<int, Point, Point>())
        .def(bp::init<Point>())
        .def(bp::init<int, Point, Point, int>())
        .def_readwrite("type", &CVertex::m_type)
        .def_readwrite("p", &CVertex::m_p)
        .def_readwrite("c", &CVertex::m_c)
        .def_readwrite("user_data", &CVertex::m_user_data)
    ;

	bp::class_<Span>("Span") 
        .def(bp::init<Span>())
        .def(bp::init<Point, CVertex, bool>())
		.def("NearestPoint", static_cast< Point (Span::*)(const Point& p)const >(&Span::NearestPoint))
		.def("NearestPoint", static_cast< Point (Span::*)(const Span& p, double *d)const >(&Span::NearestPoint))
		.def("GetBox", &Span::GetBox)
		.def("IncludedAngle", &Span::IncludedAngle)
		.def("GetArea", &Span::GetArea)
		.def("On", &Span::On)
		.def("MidPerim", &Span::MidPerim)
		.def("MidParam", &Span::MidParam)
		.def("Length", &Span::Length)
		.def("GetVector", &Span::GetVector)
        .def_readwrite("p", &Span::m_p)
		.def_readwrite("v", &Span::m_v)
    ;

	bp::class_<CCurve>("Curve") 
        .def(bp::init<CCurve>())
        .def("getVertices", &getVertices)
        .def("append",&CCurve::append)
        .def("append",&append_point)
        .def("text", &print_curve)
		.def("NearestPoint", static_cast< Point (CCurve::*)(const Point& p)const >(&CCurve::NearestPoint))
		.def("Reverse", &CCurve::Reverse)
		.def("getNumVertices", &num_vertices)
		.def("FirstVertex", &FirstVertex)
		.def("LastVertex", &LastVertex)
		.def("GetArea", &CCurve::GetArea)
		.def("IsClockwise", &CCurve::IsClockwise)
		.def("IsClosed", &CCurve::IsClosed)
        .def("ChangeStart",&CCurve::ChangeStart)
        .def("ChangeEnd",&CCurve::ChangeEnd)
        .def("Offset",&CCurve::Offset)
        .def("OffsetForward",&CCurve::OffsetForward)
        .def("GetSpans",&getCurveSpans)
        .def("GetFirstSpan",&getFirstCurveSpan)
        .def("GetLastSpan",&getLastCurveSpan)
        .def("Break",&CCurve::Break)
        .def("Perim",&CCurve::Perim)
        .def("PerimToPoint",&CCurve::PerimToPoint)
        .def("PointToPerim",&CCurve::PointToPerim)
		.def("FitArcs",&CCurve::FitArcs)
        .def("UnFitArcs",&CCurve::UnFitArcs)
    ;

	bp::class_<CBox2D>("Box") 
        .def(bp::init<CBox2D>())
		.def("MinX", &CBox2D::MinX)
		.def("MaxX", &CBox2D::MaxX)
		.def("MinY", &CBox2D::MinY)
		.def("MaxY", &CBox2D::MaxY)
    ;

	bp::class_<CArea>("Area") 
        .def(bp::init<CArea>())
        .def("getCurves", &getCurves)
        .def("append",&CArea::append)
        .def("Subtract",&CArea::Subtract)
        .def("Intersect",&CArea::Intersect)
        .def("Union",&CArea::Union)
        .def("Offset",&CArea::Offset)
        .def("FitArcs",&CArea::FitArcs)
        .def("text", &print_area)
		.def("num_curves", &CArea::num_curves)
		.def("NearestPoint", &CArea::NearestPoint)
		.def("GetBox", &CArea::GetBox)
		.def("Reorder", &CArea::Reorder)
		.def("MakePocketToolpath", &MakePocketToolpath)
		.def("Split", &SplitArea);
    ;

    bp::def("set_units", set_units);
    bp::def("get_units", get_units);
    bp::def("holes_linked", holes_linked);
    bp::def("AreaFromDxf", AreaFromDxf);
    bp::def("TangentialArc", TangentialArc);
}
