// Slufter3D - the visit: world, visitor and picture together
// Author: Jan de Kort
// Created: 2026-11-16
#include "Game.h"

#include "Model.h"

#include <algorithm>
#include <cstdio>

namespace
{
    const double pi = 3.14159265358979323846;
}

Game::Game(const WorldData& data)
    :
    m_world(data)
    , m_walker()
    , m_scene()
    , m_maxStep(0.02)
    , m_startPitch(-10.0 * pi / 180.0)
{
    m_scene.build(m_world);
    reset();
}

void Game::reset()
{
    // Where the start lies on something, the visitor starts just above it, at the height where landing there
    // ends, so that he is never inside it
    const WorldData& data = m_world.getData();
    const double east = data.m_startEast;
    const double north = data.m_startNorth;
    const double altitude = m_world.obstacle(east, north) ? m_world.clearanceAt(east, north) : 0.0;
    m_walker.place(m_world, east, north, data.m_startHeading * pi / 180.0, altitude);
    m_walker.look(0.0, m_startPitch);
}

bool Game::goTo(int houseNumber)
{
    const House* house = m_world.findHouse(houseNumber);
    if (house == nullptr)
    {
        return false;
    }
    double east = 0.0;
    double north = 0.0;
    double heading = 0.0;
    m_world.viewpoint(*house, east, north, heading);
    m_walker.place(m_world, east, north, heading);
    return true;
}

void Game::place(double east, double north, double headingDegrees)
{
    m_walker.place(m_world, east, north, headingDegrees * pi / 180.0);
}

void Game::step(double elapsed)
{
    // Short steps, so that a slow frame cannot carry the visitor through a wall
    double left = clamp(elapsed, 0.0, 1.0);
    while (left > 0.0)
    {
        const double dt = std::min(left, m_maxStep);
        m_walker.step(m_world, dt);
        left -= dt;
    }
}

std::string Game::hudString() const
{
    const WorldData& data = m_world.getData();
    const double east = m_walker.getEast();
    const double north = m_walker.getNorth();
    const House* house = m_world.houseInView(east, north, m_walker.getHeading());
    double heading = std::fmod(m_walker.getHeading() * 180.0 / pi, 360.0);
    if (heading < 0.0)
    {
        heading += 360.0;
    }
    std::string label = "0";
    if (house != nullptr)
    {
        label = (house->m_number == 0 && !house->m_name.empty()) ? house->m_name : std::to_string(house->m_number);
    }
    char text[256];
    std::snprintf(text, sizeof(text), "%.0f|%.0f|%s|%s|%s|%.0f|%.0f|%.1f|%.0f|%s|%.0f",
        3.6 * m_walker.getSpeed(), 3.6 * m_walker.flightSpeed(),
        label.c_str(),
        house != nullptr ? house->m_type.c_str() : "",
        house != nullptr ? house->m_model->label() : "",
        data.m_originEast + east, data.m_originNorth + north,
        m_world.getTerrain().height(east, north), heading,
        World::surfaceName(m_world.surfaceAt(east, north)),
        m_walker.getAltitude());
    return std::string(text);
}
