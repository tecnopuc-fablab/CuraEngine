// Copyright (c) 2024 UltiMaker
// CuraEngine is released under the terms of the AGPLv3 or higher.

#include "geometry/mixed_lines_set.h"

#include <numeric>

#include "geometry/open_polyline.h"
#include "geometry/polygon.h"
#include "geometry/shape.h"


namespace cura
{

Shape MixedLinesSet::offset(coord_t distance, ClipperLib::JoinType join_type, double miter_limit) const
{
    if (distance == 0)
    {
        // Return a shape that contains only actual polygons
        Shape result;

        for (const PolylinePtr& line : (*this))
        {
            if (const std::shared_ptr<const Polygon> polygon = dynamic_pointer_cast<const Polygon>(line))
            {
                result.push_back(*polygon);
            }
        }

        return result;
    }
    else
    {
        Shape polygons;
        ClipperLib::ClipperOffset clipper(miter_limit, 10.0);

        for (const PolylinePtr& line : (*this))
        {
            if (const std::shared_ptr<const Polygon> polygon = dynamic_pointer_cast<const Polygon>(line))
            {
                // Union all polygons first and add them later
                polygons.push_back(*polygon);
            }
            else
            {
                ClipperLib::EndType end_type;

                if (line->addClosingSegment())
                {
                    end_type = ClipperLib::etClosedLine;
                }
                else if (join_type == ClipperLib::jtMiter)
                {
                    end_type = ClipperLib::etOpenSquare;
                }
                else
                {
                    end_type = ClipperLib::etOpenRound;
                }

                clipper.AddPath(line->getPoints(), join_type, end_type);
            }
        }

        if (! polygons.empty())
        {
            polygons = polygons.unionPolygons();

            for (const Polygon& polygon : polygons)
            {
                clipper.AddPath(polygon.getPoints(), join_type, ClipperLib::etClosedPolygon);
            }
        }

        clipper.MiterLimit = miter_limit;

        ClipperLib::Paths result;
        clipper.Execute(result, static_cast<double>(distance));
        return Shape(std::move(result));
    }
}

void MixedLinesSet::push_back(const OpenPolyline& line)
{
    std::vector<PolylinePtr>::push_back(std::make_shared<OpenPolyline>(line));
}

void MixedLinesSet::push_back(OpenPolyline&& line)
{
    std::vector<PolylinePtr>::push_back(std::make_shared<OpenPolyline>(std::move(line)));
}

void MixedLinesSet::push_back(ClosedPolyline&& line)
{
    std::vector<PolylinePtr>::push_back(std::make_shared<ClosedPolyline>(std::move(line)));
}

void MixedLinesSet::push_back(const Polygon& line)
{
    std::vector<PolylinePtr>::push_back(std::make_shared<Polygon>(std::move(line)));
}

void MixedLinesSet::push_back(const std::shared_ptr<OpenPolyline>& line)
{
    std::vector<PolylinePtr>::push_back(line);
}

void MixedLinesSet::push_back(const PolylinePtr& line)
{
    std::vector<PolylinePtr>::push_back(line);
}

void MixedLinesSet::push_back(LinesSet<OpenPolyline>&& lines_set)
{
    reserve(size() + lines_set.size());
    for (OpenPolyline& line : lines_set)
    {
        push_back(std::move(line));
    }
}

void MixedLinesSet::push_back(const LinesSet<OpenPolyline>& lines_set)
{
    reserve(size() + lines_set.size());
    for (const OpenPolyline& line : lines_set)
    {
        push_back(line);
    }
}

void MixedLinesSet::push_back(LinesSet<ClosedPolyline>&& lines_set)
{
    reserve(size() + lines_set.size());
    for (ClosedPolyline& line : lines_set)
    {
        push_back(std::move(line));
    }
}

void MixedLinesSet::push_back(const LinesSet<Polygon>& lines_set)
{
    reserve(size() + lines_set.size());
    for (const Polygon& line : lines_set)
    {
        push_back(line);
    }
}

void MixedLinesSet::push_back(const Shape& shape)
{
    push_back(static_cast<const LinesSet<Polygon>&>(shape));
}

coord_t MixedLinesSet::length() const
{
    return std::accumulate(
        begin(),
        end(),
        coord_t(0),
        [](coord_t value, const PolylinePtr& line)
        {
            return value + line->length();
        });
}

} // namespace cura
