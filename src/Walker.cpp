// Slufter3D - the visitor whose eyes the world is seen through: walking, or flying above the ground
// Author: Jan de Kort
// Created: 2026-11-16
#include "Walker.h"

#include "Check.h"
#include "World.h"

#include <algorithm>

Walker::Walker()
    :
    m_controls()
    , m_pace(Pace::Cycle)
    , m_east(0.0)
    , m_north(0.0)
    , m_heading(0.0)
    , m_pitch(0.0)
    , m_ground(0.0)
    , m_altitude(0.0)
    , m_targetAltitude(0.0)
    , m_speed(0.0)
    , m_walkSpeed(5.0 / 3.6)
    , m_cycleSpeed(20.0 / 3.6)
    , m_driveSpeed(50.0 / 3.6)
    , m_turnRate(1.6)
    , m_eyeHeight(1.7)
    , m_groundRate(16.0)
    , m_groundReach(0.25)
    , m_pitchLimit(1.5)
    , m_footing(0.3)
    , m_maxAltitude(300.0)
    , m_riseFactor(1.3)
    , m_firstRise(3.0)
    , m_altitudeRate(2.5)
    , m_climbRate(0.5)
    , m_speedPerHeight(0.4)
    , m_glide(1.5)
    , m_hover(2.0)
    , m_keep(0.7)
{
}

void Walker::place(const World& world, double east, double north, double heading, double altitude /* = 0.0 */)
{
    m_east = east;
    m_north = north;
    m_heading = heading;
    m_pitch = 0.0;
    m_speed = 0.0;
    m_altitude = altitude;
    m_targetAltitude = altitude;
    m_ground = world.getTerrain().height(east, north);
}

void Walker::look(double turn, double lift)
{
    m_heading += turn;
    m_pitch = clamp(m_pitch + lift, -m_pitchLimit, m_pitchLimit);
}

void Walker::rise(double notches)
{
    if (notches > 0.0)
    {
        // From the ground the first notch goes to the first height; every further one multiplies
        const bool fromGround = m_targetAltitude < m_firstRise;
        const double base = fromGround ? m_firstRise : m_targetAltitude;
        m_targetAltitude = base * std::pow(m_riseFactor, fromGround ? notches - 1.0 : notches);
        m_targetAltitude = std::max(m_targetAltitude, std::min(m_firstRise, base));
    }
    else if (notches < 0.0)
    {
        m_targetAltitude *= std::pow(m_riseFactor, notches);
        if (m_targetAltitude < m_firstRise / m_riseFactor)
        {
            m_targetAltitude = 0.0;     // the last notch lands
        }
    }
    m_targetAltitude = std::min(m_targetAltitude, m_maxAltitude);
}

double Walker::paceSpeed() const
{
    switch (m_pace)
    {
    case Pace::Walk:  return m_walkSpeed;
    case Pace::Cycle: return m_cycleSpeed;
    case Pace::Drive: return m_driveSpeed;
    default: require(false, "Unknown pace"); return 0.0;
    }
}

double Walker::flightSpeed() const
{
    return std::max(paceSpeed(), m_speedPerHeight * m_altitude);
}

void Walker::step(const World& world, double dt)
{
    if (!(dt > 0.0))
    {
        return;
    }
    m_heading += clamp(m_controls.m_turn, -1.0, 1.0) * m_turnRate * dt;

    const double forward = clamp(m_controls.m_forward, -1.0, 1.0);
    const double strafe = clamp(m_controls.m_strafe, -1.0, 1.0);
    const double amount = std::hypot(forward, strafe);
    const double scale = amount > 1.0 ? 1.0 / amount : 1.0; // diagonal is no faster
    const double distance = flightSpeed() * dt * scale;
    const double stepEast = distance * (forward * std::sin(m_heading) + strafe * std::cos(m_heading));
    const double stepNorth = distance * (forward * std::cos(m_heading) - strafe * std::sin(m_heading));

    // On the ground walls and water stop the visitor, below the roofs walls only, unless he is above what stands
    // there (the lookout tower), above the roofs the edge of the world; and only the part of the step that runs
    // into them is lost
    const bool walking = !isFlying();
    const bool low = m_altitude < world.getClearance();
    const auto free = [&](double east, double north)
        {
            if (walking)
            {
                return !world.blocked(east, north);
            }
            if (low)
            {
                return !world.obstacle(east, north) || m_altitude >= world.clearanceAt(east, north);
            }
            return world.getTerrain().contains(east, north, 2.0 * m_maxAltitude);
        };
    const double fromEast = m_east;
    const double fromNorth = m_north;
    if (free(m_east + stepEast, m_north + stepNorth))
    {
        m_east += stepEast;
        m_north += stepNorth;
    }
    else if (free(m_east + stepEast, m_north))
    {
        m_east += stepEast;
    }
    else if (free(m_east, m_north + stepNorth))
    {
        m_north += stepNorth;
    }
    m_speed = std::hypot(m_east - fromEast, m_north - fromNorth) / dt;

    // Up and down: the control moves the target, the altitude follows it. Over a roof, the lookout tower or
    // water there is no landing: the visitor stays above it.
    const double climb = clamp(m_controls.m_climb, -1.0, 1.0);
    if (climb != 0.0)
    {
        m_targetAltitude += climb * m_climbRate * std::max(m_firstRise, m_targetAltitude) * dt;
        m_targetAltitude = clamp(m_targetAltitude, 0.0, m_maxAltitude);
    }
    double target = m_targetAltitude;
    if (world.obstacle(m_east, m_north))
    {
        target = std::max(target, world.clearanceAt(m_east, m_north));
    }
    else if (!world.standable(m_east, m_north))
    {
        target = std::max(target, m_hover);
    }
    m_altitude += (target - m_altitude) * (1.0 - std::exp(-m_altitudeRate * dt));
    if (target == 0.0 && m_altitude < 0.02)
    {
        m_altitude = 0.0;
    }

    // The feet follow the ground, the eyes a little more smoothly; high up they glide over the relief
    const double ground = world.getTerrain().height(m_east, m_north);
    const double reach = m_groundReach + m_glide * m_altitude;
    const double rate = std::max(m_groundRate / (1.0 + m_altitude), m_speed / reach); // the faster, the quicker
    m_ground += (ground - m_ground) * (1.0 - std::exp(-rate * dt));
    if (isFlying())
    {
        m_ground = std::max(m_ground, ground - m_keep * m_altitude); // never gliding into a rising dune
    }
}

Vec3 Walker::eye() const
{
    return Vec3(m_east, m_ground + m_altitude + m_eyeHeight, -m_north);
}

Vec3 Walker::gaze() const
{
    return Vec3(std::sin(m_heading) * std::cos(m_pitch), std::sin(m_pitch), -std::cos(m_heading) * std::cos(m_pitch));
}
