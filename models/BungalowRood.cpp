// Slufter3D - model BungalowRood: the red bungalow of before the park, the smallest of the three
// Author: Jan de Kort
// Created: 2026-11-18
// BungalowRood (photos Bungalow Rood: Screenshot 2026-11-18 22.39.12, 22.42.12 and 23.05.13)
// The front is that of 22.39.12, which shows it square on: 6.0 m by its door leaf. The postcard 23.05.13 shows
// the right end, two glazed leaves in the wall itself under a cut in the thatch,
// and a brick chimney at the left end of a ridge of nine or ten tiles. The end is 5.2 m in 22.42.12, as that of
// the Visdiefje. The window of the left end is that of 22.39.12; of the back there is no photograph: one window
// is assumed. No paving but the doorstep: the chairs stand in the grass.
#include "Models.h"
#include "OldBungalow.h"

namespace
{
    const OldBungalowPlan planRood = {
        6.00, 5.20,
        Color(0.74f, 0.36f, 0.24f),     // orange red
        0.72, 1.63, 3.15, 4.81, 0.48,
        1.75, 3.15, 2,
        { { 2.40, 3.60 } }, 0.95,
        2.45, 3.10,
        true, false,
        0.0, 0.0 };

    // Class BungalowRood
    class BungalowRood : public OldBungalow
    {
    public:
        BungalowRood()
            :
            OldBungalow(planRood)
        {
        }

        // Accessor
        const char* name() const override { return "BungalowRood"; }
        const char* label() const override { return "Rood"; }
    };
}

const Model& Models::bungalowRood()
{
    static const BungalowRood model;
    return model;
}
