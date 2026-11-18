// Slufter3D - model BungalowGroen: the green bungalow of before the park, the middle one of the three
// Author: Jan de Kort
// Created: 2026-11-18
// BungalowGroen (photos Bungalow Groen: D16T1_047, D16T2_032, D16T2_033 and D16T2_039)
// The front is that of D16T2_032 and D16T2_039, about 6.4 m by its door leaf: slits, the olive door under a white
// globe, the window; grey paving along it from the doorstep and round the right corner. D16T2_039 shows the left
// end with its narrow window under an arched cut in the thatch, drawn here as a square cut, and the brick chimney
// at the left end of the ridge. Of the right end and the back there is no photograph: the glazed doors of the red
// type and two windows are assumed.
#include "Models.h"
#include "OldBungalow.h"

namespace
{
    const OldBungalowPlan planGroen = {
        6.40, 5.20,
        Color(0.50f, 0.55f, 0.22f),     // olive green
        0.90, 2.02, 3.97, 5.63, 0.55,
        1.75, 3.15, 2,
        { { 1.30, 2.40 }, { 4.00, 5.10 } }, 0.95,
        3.35, 4.00,
        true, false,
        2.60, 1.60 };

    // Class BungalowGroen
    class BungalowGroen : public OldBungalow
    {
    public:
        BungalowGroen()
            :
            OldBungalow(planGroen)
        {
        }

        // Accessor
        const char* name() const override { return "BungalowGroen"; }
        const char* label() const override { return "Groen"; }
    };
}

const Model& Models::bungalowGroen()
{
    static const BungalowGroen model;
    return model;
}
