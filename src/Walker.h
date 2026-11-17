// Slufter3D - the visitor whose eyes the world is seen through: walking, or flying above the ground
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Math.h"

class World;

// Struct Controls
struct Controls
{
    double m_forward = 0.0; // +1->ahead, -1->back
    double m_strafe = 0.0;  // +1->to the right
    double m_turn = 0.0;    // +1->clockwise
    double m_climb = 0.0;   // +1->up, -1->down
};

// Class Walker
// On the ground the visitor walks and walls stop him. He can also rise above the ground: the altitude is
// the height of his feet above it. Flying goes forward the way he faces, not the way he looks, and the
// higher he is, the faster. Coming down over a roof or over water he stays hanging above it, over the lookout
// tower just above its railings.
class Walker
{
public:
    enum class Pace
    {
        Walk,  // 5 km/h
        Cycle, // 20 km/h
        Drive  // 50 km/h
    };

    Walker();

    // Mutator
    // altitude->of the feet above the ground, where he is held until told otherwise; 0->on the ground
    void place(const World& world, double east, double north, double heading, double altitude = 0.0);
    void setControls(const Controls& controls) { m_controls = controls; }
    void setPace(Pace pace) { m_pace = pace; }
    void look(double turn, double lift);  // adds to heading and pitch [rad]
    void rise(double notches, double roof = 0.0); // +1->one step higher, -1->one lower; steps grow with the altitude;
                                                  // roof: top of what stands under him, 0 on the ground
    void land() { m_targetAltitude = 0.0; }
    void step(const World& world, double dt);

    // Accessor
    Pace getPace() const { return m_pace; }
    double paceSpeed() const;            // set speed on the ground [m/s]
    double flightSpeed() const;          // set speed at this altitude [m/s]
    double getSpeed() const { return m_speed; } // speed over the ground in the last step [m/s]
    double getEast() const { return m_east; }
    double getNorth() const { return m_north; }
    double getHeading() const { return m_heading; } // [rad], 0->north, clockwise
    double getPitch() const { return m_pitch; }
    double getEyeHeight() const { return m_eyeHeight; }
    double getAltitude() const { return m_altitude; } // of the feet above the ground [m]
    bool isFlying() const { return m_altitude > m_footing; }
    Vec3 eye() const;
    Vec3 gaze() const;                   // unit vector the eyes look along

private:
    Controls m_controls;
    Pace m_pace;
    double m_east;
    double m_north;
    double m_heading;
    double m_pitch;
    double m_ground;          // smoothed height of the ground under the feet [m NAP]
    double m_altitude;        // feet above the ground [m]
    double m_targetAltitude;  // where the altitude is heading [m]
    double m_speed;
    double m_walkSpeed;       // [m/s]
    double m_cycleSpeed;      // [m/s]
    double m_driveSpeed;      // [m/s]
    double m_turnRate;        // [rad/s]
    double m_eyeHeight;       // above the feet [m]
    double m_groundRate;      // how fast the eyes follow the ground when standing [1/s]
    double m_groundReach;     // distance over which they catch up with it when moving [m]
    double m_pitchLimit;      // [rad]
    double m_footing;         // below this altitude the visitor is on the ground [m]
    double m_maxAltitude;     // [m]
    double m_riseFactor;      // one notch multiplies the altitude by this
    double m_firstRise;       // the first notch from the ground takes him here [m]
    double m_altitudeRate;    // how fast the altitude follows its target [1/s]
    double m_climbRate;       // of the climb control, per second, as a part of the altitude
    double m_speedPerHeight;  // flying speed per metre of altitude [1/s]
    double m_glide;           // the eyes follow the ground this many metres later per metre of altitude
    double m_hover;           // lowest altitude over water [m]
    double m_keep;            // gliding may cost at most this part of the altitude
};
