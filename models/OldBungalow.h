// Slufter3D - OldBungalow: the thatched bungalows that stood in the dunes before the park was built
// Author: Jan de Kort
// Created: 2026-11-18
// Three types, named after the colour of their doors and, with it, the number of guests: red the smallest, green,
// yellow the largest. They were alike in all but their size and that colour, so one class draws them from a plan
// that says where their parts are. The Visdiefje, the only one still standing, has a model of its own.
#pragma once

#include "Color.h"
#include "Model.h"

#include <utility>
#include <vector>

// Struct OldBungalowPlan: the measures of one type. Places along a wall are in metres from its left end as seen
// from outside, heights in metres above the floor
struct OldBungalowPlan
{
    double m_length;        // the walls, along the front
    double m_width;         // and from front to back
    Color m_door;           // the front door, which gives the type its name
    // The front, from the left: two vent slits, the door, the window
    double m_vents;         // middle of the two slits
    double m_doorMiddle;    // middle of the door
    double m_window0;       // the window, outside its frame
    double m_window1;
    double m_sill;          // its foot
    // The right end: the glazed doors to the terrace, under a cut in the thatch
    double m_glazed0;
    double m_glazed1;
    int m_leaves;           // 2->two leaves, 3->two leaves and a side light
    // The back: bedroom windows of two lights
    std::vector<std::pair<double, double>> m_backWindows;
    double m_backSill;
    // The left end: a narrow window under a cut in the thatch
    double m_endWindow0;
    double m_endWindow1;
    bool m_chimney;         // true->a brick chimney at the left end of the ridge
    bool m_ventBox;         // true->a vent low in the front wall, left of the window
    double m_frontTerrace;  // depth of the paving before door and window, round the doorstep; 0->none
    double m_endTerrace;    // depth of the paving before the glazed doors, 0->none
};

// Class OldBungalow
class OldBungalow : public Model // Base class of the bungalows of before the park: draws one from its plan
{
public:
    explicit OldBungalow(const OldBungalowPlan& plan);

    // Accessor
    std::pair<double, double> footprint(double outlineLength, double outlineWidth) const override;
    double ridgeHeight(double width) const override;

    void build(Mesh& mesh, const Site& site) const override;

private:
    OldBungalowPlan m_plan;
};
