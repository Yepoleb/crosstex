#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "BC.hpp"
#include "BC123_shared.hpp"
#include "Colors.hpp"


namespace Tex {

void DecodeBC1(HDRColorA *pColor, const uint8_t *pBC)
{
    auto pBC1 = reinterpret_cast<const Block_BC1 *>(pBC);
    DecodeBC1(pColor, pBC1, true);
}

void EncodeBC1(uint8_t *pBC, const HDRColorA *pColor, float threshold, uint32_t flags)
{
    assert(pBC && pColor);

    HDRColorA Color[NUM_PIXELS_PER_BLOCK];

    if (flags & BC_FLAGS_DITHER_A)
    {
        float fError[NUM_PIXELS_PER_BLOCK];
        memset(fError, 0x00, NUM_PIXELS_PER_BLOCK * sizeof(float));

        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            HDRColorA clr = pColor[i];

            float fAlph = clr.a + fError[i];

            Color[i].r = clr.r;
            Color[i].g = clr.g;
            Color[i].b = clr.b;
            Color[i].a = (float) static_cast<int32_t>(clr.a + fError[i] + 0.5f);

            float fDiff = fAlph - Color[i].a;

            if (3 != (i & 3))
            {
                assert(i < 15);
                fError[i + 1] += fDiff * (7.0f / 16.0f);
            }

            if (i < 12)
            {
                if (i & 3)
                    fError[i + 3] += fDiff * (3.0f / 16.0f);

                fError[i + 4] += fDiff * (5.0f / 16.0f);

                if (3 != (i & 3))
                {
                    assert(i < 11);
                    fError[i + 5] += fDiff * (1.0f / 16.0f);
                }
            }
        }
    }
    else
    {
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            Color[i] = pColor[i];
        }
    }

    auto pBC1 = reinterpret_cast<Block_BC1 *>(pBC);
    EncodeBC1(pBC1, Color, true, threshold, flags);
}

void EncodeBC1(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    EncodeBC1(pBC, pColor, 0.5f, flags);
}

}
