// Slufter3D - the park and the land around it: ground, houses, roads, water
// Author: Jan de Kort
// Created: 2026-11-16
#include "World.h"

#include "Check.h"
#include "Models.h"

#include <algorithm>
#include <sstream>

namespace
{
    const double pi = 3.14159265358979323846;

    bool inTriangle(double px, double py, const float* t)
    {
        const double d1 = (px - t[2])*(t[1] - t[3]) - (t[0] - t[2])*(py - t[3]);
        const double d2 = (px - t[4])*(t[3] - t[5]) - (t[2] - t[4])*(py - t[5]);
        const double d3 = (px - t[0])*(t[5] - t[1]) - (t[4] - t[0])*(py - t[1]);
        const bool negative = d1 < 0.0 || d2 < 0.0 || d3 < 0.0;
        const bool positive = d1 > 0.0 || d2 > 0.0 || d3 > 0.0;
        return !(negative && positive);
    }
}

// Struct House
std::string House::debugString() const
{
    std::stringstream ss;
    ss << "House " << (m_name.empty() ? std::to_string(m_number) : m_name) << " type " << m_type << " model "
        << (m_model != nullptr ? m_model->name() : "none") << " at (" << m_east << ", " << m_north << ") "
        << m_site.m_length << " x " << m_site.m_width << " m";
    return ss.str();
}

// Class World
World::World(const WorldData& data)
    :
    m_data(data)
    , m_terrain(data)
    , m_reception(data, m_terrain)
    , m_tower()
    , m_everyHouse()
    , m_houses()
    , m_era(WorldData::today())
    , m_roadStart()
    , m_roadOrder()
    , m_wallMargin(0.35)
    , m_edgeMargin(30.0)
    , m_semiStagger(1.22)
    , m_clearance(12.0)
    , m_towerHeadroom(0.5)
    , m_viewDistance(14.0)
    , m_viewRange(90.0)
    , m_viewAngle(0.45)
{
    m_houses.reserve(data.m_houses.size());
    std::transform(data.m_houses.begin(), data.m_houses.end(), std::back_inserter(m_houses),
        [&](const WorldData::HouseRecord& record)
        {
            House house;
            house.m_number = record.m_number;
            house.m_type = record.m_type;
            house.m_name = record.m_name;
            house.m_model = Models::byName(record.m_model);
            require(house.m_model != nullptr, "World file names a model that does not exist");
            house.m_east = record.m_east;
            house.m_north = record.m_north;
            house.m_semi = record.m_semi;
            house.m_pitch = record.m_pitch;
            house.m_eras = record.m_eras;
            // The long axis lies a quarter turn anticlockwise of the front
            const double front = record.m_front * pi / 180.0;
            const double along = front - 0.5 * pi;
            house.m_site.m_axisLength = Vec3(std::sin(along), 0.0, -std::cos(along));
            house.m_site.m_axisWidth = Vec3(std::sin(front), 0.0, -std::cos(front));
            if (record.m_outline)
            {
                house.m_site.m_length = record.m_length;
                house.m_site.m_width = record.m_width;
            }
            else
            {
                const auto [length, width] = house.m_model->footprint(record.m_length, record.m_width);
                house.m_site.m_length = length;
                house.m_site.m_width = width;
            }
            house.m_site.m_floor = Vec3(record.m_east, record.m_floor, -record.m_north);
            house.m_site.m_ground = &m_terrain;
            return house;
        });
    joinSemiDetached(data);
    m_everyHouse = m_houses;
    setEra(m_era);
    indexRoads();
}

void World::setEra(int era)
{
    if (era < 0 || era >= WorldData::eraCount())
    {
        return;
    }
    m_era = era;
    m_houses.clear();
    std::copy_if(m_everyHouse.begin(), m_everyHouse.end(), std::back_inserter(m_houses),
        [&](const House& house) { return (house.m_eras & (1u << era)) != 0; });
}

// The two halves of a block lie a quarter of the block from its middle. Their models have their own
// length, so each half is moved along the block until the two meet in the middle again.
void World::joinSemiDetached(const WorldData& data)
{
    std::vector<House> joined = m_houses;
    for (size_t k = 0; k < m_houses.size(); ++k)
    {
        const House& house = m_houses[k];
        const House* other = data.m_houses[k].m_semiWith > 0 ? findHouse(data.m_houses[k].m_semiWith) : nullptr;
        if (other == nullptr)
        {
            continue;
        }
        const double middleEast = 0.5 * (house.m_east + other->m_east);
        const double middleNorth = 0.5 * (house.m_north + other->m_north);
        const double apart = std::hypot(house.m_east - other->m_east, house.m_north - other->m_north);
        const double away = 0.5 * house.m_site.m_length / std::max(1.0e-6, 0.5 * apart);
        House& moved = joined[k];
        moved.m_east = middleEast + (house.m_east - middleEast) * away;
        moved.m_north = middleNorth + (house.m_north - middleNorth) * away;
        if (house.m_model == &Models::comfort4C())
        {
            // The two halves of a 4C block are staggered: the half along the axis stands forward of the other, as
            // in 55 of the 58 blocks of the BGT. The model is the forward half, its outer gable along the axis; the
            // other half is its mirror image.
            const Vec3 fromMiddle(house.m_east - middleEast, 0.0, -(house.m_north - middleNorth));
            const double side = dot(fromMiddle, house.m_site.m_axisLength) > 0.0 ? 0.5 : -0.5;
            moved.m_east += side * m_semiStagger * house.m_site.m_axisWidth.x;
            moved.m_north -= side * m_semiStagger * house.m_site.m_axisWidth.z;
            if (side < 0.0)
            {
                moved.m_site.m_axisLength = -house.m_site.m_axisLength;
                moved.m_site.m_mirrored = true;
            }
        }
        moved.m_site.m_floor = Vec3(moved.m_east, house.m_site.m_floor.y, -moved.m_north);
    }
    m_houses = joined;
}

// Every road triangle lies within one tile of the finest grid, so a tile leads straight to its triangles
void World::indexRoads()
{
    const TerrainLevel& level = m_terrain.getLevel(0);
    const size_t tiles = (level.getNx() - 1) * (level.getNz() - 1);
    const size_t count = m_data.m_roadKinds.size();
    std::vector<size_t> tileOf(count);
    m_roadStart.assign(tiles + 1, 0);
    for (size_t k = 0; k < count; ++k)
    {
        const float* t = m_data.m_roadCorners.data() + 6*k;
        tileOf[k] = level.tileOf((t[0] + t[2] + t[4]) / 3.0, (t[1] + t[3] + t[5]) / 3.0);
        ++m_roadStart[tileOf[k] + 1];
    }
    for (size_t tile = 0; tile < tiles; ++tile)
    {
        m_roadStart[tile + 1] += m_roadStart[tile];
    }
    std::vector<size_t> next(m_roadStart.begin(), m_roadStart.end() - 1);
    m_roadOrder.resize(count);
    for (size_t k = 0; k < count; ++k)
    {
        m_roadOrder[next[tileOf[k]]++] = k;
    }
}

const House* World::findHouse(int number) const
{
    const auto it = std::find_if(m_houses.begin(), m_houses.end(),
        [&](const House& house)
        {
            return house.m_number == number;
        });
    return (number > 0 && it != m_houses.end()) ? &*it : nullptr;
}

const House* World::nearestHouse(double east, double north) const
{
    const auto it = std::min_element(m_houses.begin(), m_houses.end(),
        [&](const House& a, const House& b)
        {
            return std::hypot(a.m_east - east, a.m_north - north) < std::hypot(b.m_east - east, b.m_north - north);
        });
    return it != m_houses.end() ? &*it : nullptr;
}

const House* World::houseInView(double east, double north, double heading) const
{
    const House* best = nullptr;
    double bestScore = 0.0;
    for (const House& house : m_houses)
    {
        const double distance = std::hypot(house.m_east - east, house.m_north - north);
        double angle = std::atan2(house.m_east - east, house.m_north - north) - heading;
        angle = std::fabs(std::remainder(angle, 2.0 * pi));
        if (distance < m_viewRange && angle < m_viewAngle)
        {
            const double score = distance * (1.0 + 4.0 * angle); // near and straight ahead wins
            if (best == nullptr || score < bestScore)
            {
                best = &house;
                bestScore = score;
            }
        }
    }
    return best != nullptr ? best : nearestHouse(east, north);
}

bool World::insideHouse(const House& house, double east, double north, double margin) const
{
    const Vec3 offset(east - house.m_east, 0.0, -north - house.m_site.m_floor.z);
    return house.m_model->covers(house.m_site.m_length, house.m_site.m_width,
        dot(offset, house.m_site.m_axisLength), dot(offset, house.m_site.m_axisWidth), margin);
}

bool World::obstacle(double east, double north) const
{
    if (!m_terrain.contains(east, north, m_edgeMargin))
    {
        return true;
    }
    return inTower(east, north) || (hasReception() && m_reception.contains(east, north, m_wallMargin))
        || std::any_of(m_houses.begin(), m_houses.end(),
            [&](const House& house)
            {
                return insideHouse(house, east, north, m_wallMargin);
            });
}

bool World::inTower(double east, double north) const
{
    const double reach = m_tower.getFootprint() + m_wallMargin;
    return std::fabs(east - m_data.m_towerEast) < reach && std::fabs(north - m_data.m_towerNorth) < reach;
}

double World::clearanceAt(double east, double north) const
{
    return inTower(east, north) ? m_tower.getHeight() + m_towerHeadroom : m_clearance;
}

bool World::standable(double east, double north) const
{
    return !m_terrain.waterAt(east, north) && !m_terrain.seaAt(east, north);
}

World::Surface World::surfaceAt(double east, double north) const
{
    if (m_terrain.seaAt(east, north))
    {
        return Surface::Sea;
    }
    if (m_terrain.waterAt(east, north))
    {
        return Surface::Water;
    }
    const TerrainLevel& level = m_terrain.getLevel(0);
    if (level.contains(east, north))
    {
        const size_t tile = level.tileOf(east, north);
        for (size_t at = m_roadStart[tile]; at < m_roadStart[tile + 1]; ++at)
        {
            const size_t k = m_roadOrder[at];
            if ((m_data.m_roadEras[k] & (1u << m_era)) != 0 && inTriangle(east, north, m_data.m_roadCorners.data() + 6*k))
            {
                switch (m_data.m_roadKinds[k])
                {
                case 0: return Surface::Asphalt;
                case 1: return Surface::Bricks;
                case 2: return Surface::Shells;
                default: return Surface::Sand;
                }
            }
        }
    }
    return Surface::Dunes;
}

const char* World::surfaceName(Surface surface)
{
    switch (surface)
    {
    case Surface::Asphalt: return "asphalt";
    case Surface::Bricks:  return "bricks";
    case Surface::Shells:  return "shells";
    case Surface::Sand:    return "sand";
    case Surface::Water:   return "water";
    case Surface::Sea:     return "sea";
    case Surface::Dunes:   return "dunes";
    default: require(false, "Unknown surface"); return "";
    }
}

void World::viewpoint(const House& house, double& east, double& north, double& heading) const
{
    // Try the front first, then the other sides, nearest first
    const Vec3 directions[] = { house.m_site.m_axisWidth, house.m_site.m_axisLength, -house.m_site.m_axisLength,
        -house.m_site.m_axisWidth };
    const double reach[] = { 0.5 * house.m_site.m_width, 0.5 * house.m_site.m_length, 0.5 * house.m_site.m_length,
        0.5 * house.m_site.m_width };
    const double steps[] = { 1.0, 0.7, 1.4, 0.45 };
    for (const double step : steps)
    {
        for (size_t k = 0; k < 4; ++k)
        {
            const double distance = reach[k] + step * m_viewDistance;
            const double e = house.m_east + directions[k].x * distance;
            const double n = house.m_north - directions[k].z * distance;
            if (!blocked(e, n))
            {
                east = e;
                north = n;
                heading = std::atan2(-directions[k].x, directions[k].z);
                return;
            }
        }
    }
    east = house.m_east + directions[0].x * (reach[0] + m_viewDistance);
    north = house.m_north - directions[0].z * (reach[0] + m_viewDistance);
    heading = std::atan2(-directions[0].x, directions[0].z);
}
