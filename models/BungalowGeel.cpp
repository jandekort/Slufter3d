// Slufter3D - model BungalowGeel: the yellow bungalow of before the park, the largest of the three
// Author: Jan de Kort
// Created: 2026-11-18
// BungalowGeel (photos Bungalow Geel: DoosA_Tray2_029, 034, 035, 042, 043 and 045)
// The front is that of DoosA_Tray2_034, with the perspective of its wall taken out: about 7.8 m by its door leaf,
// with the slits, the mustard door under a white globe, a vent low in the wall and the window.
// In the right end the glazed doors, two leaves and a side light, in the wall under a cut in the thatch, between
// a narrow wall at the front corner and a wider one at the back (034, 035, 045); the terrace lies before them.
// The back has three windows of two lights over sills at 0.95 m (042, 043). No chimney on the Grutto; of the left end there is no photograph: the window of the green type.
#include "Models.h"
#include "OldBungalow.h"

namespace
{
    const OldBungalowPlan planGeel = {
        7.80, 5.20,
        Color(0.80f, 0.73f, 0.40f),     // mustard yellow
        1.60, 2.77, 4.45, 6.45, 0.50,
        0.95, 3.12, 3,
        { { 1.90, 3.10 }, { 4.10, 5.20 }, { 6.10, 7.20 } }, 0.95,
        3.35, 4.00,
        false, true,
        0.0, 2.40 };

    // Class BungalowGeel
    class BungalowGeel : public OldBungalow
    {
    public:
        BungalowGeel()
            :
            OldBungalow(planGeel)
        {
        }

        // Accessor
        const char* name() const override { return "BungalowGeel"; }
        const char* label() const override { return "Geel"; }
    };
}

const Model& Models::bungalowGeel()
{
    static const BungalowGeel model;
    return model;
}
