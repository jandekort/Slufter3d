// Slufter3D - the visit: world, visitor and picture together
// Author: Jan de Kort
// Created: 2026-11-16
#pragma once

#include "Scene.h"
#include "Walker.h"
#include "World.h"

#include <string>

// Class Game
class Game
{
public:
    explicit Game(const WorldData& data);

    // Mutator
    void reset();                       // back to the start: on the lookout tower, looking east, landed
    void setControls(const Controls& controls) { m_walker.setControls(controls); }
    void setPace(Walker::Pace pace) { m_walker.setPace(pace); }
    void look(double turn, double lift) { m_walker.look(turn, lift); }
    void rise(double notches) { m_walker.rise(notches); }
    void land() { m_walker.land(); }
    bool goTo(int houseNumber);         // false->no such house
    void place(double east, double north, double headingDegrees); // metres from the origin, on the ground
    void step(double elapsed);

    // Accessor
    const World& getWorld() const { return m_world; }
    const Walker& getWalker() const { return m_walker; }
    const Scene& getScene() const { return m_scene; }
    // "speed km/h|set speed km/h|number|type|model of the house looked at|east RD|north RD|ground m NAP|heading|
    // surface|altitude m"; number is the name of the house when it has no number, and 0 when it has neither
    std::string hudString() const;

private:
    World m_world;
    Walker m_walker;
    Scene m_scene;
    double m_maxStep;   // longest time step taken at once [s]
    double m_startPitch; // the visit starts looking this far up, down when negative [rad]
};
