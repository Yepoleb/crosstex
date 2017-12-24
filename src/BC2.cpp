#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "BC.hpp"
#include "BC123_shared.hpp"
#include "Colors.hpp"


namespace Tex {

void DecodeBC2(HDRColorA *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(Block_BC2) == 16, "Block_BC2 should be 16 bytes");

    auto pBC2 = reinterpret_cast<const Block_BC2 *>(pBC);

    // RGB part
    DecodeBC1(pColor, &pBC2->bc1, false);

    // 4-bit alpha part
    uint32_t dw = pBC2->bitmap[0];

    for (size_t i = 0; i < 8; ++i, dw >>= 4)
    {
        pColor[i].a = (float)(dw & 0xf) * (1.0f / 15.0f);
    }

    dw = pBC2->bitmap[1];

    for (size_t i = 8; i < NUM_PIXELS_PER_BLOCK; ++i, dw >>= 4)
    {
        pColor[i].a = (float)(dw & 0xf) * (1.0f / 15.0f);
    }
}

void EncodeBC2(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    assert(pBC && pColor);
    static_assert(sizeof(Block_BC2) == 16, "Block_BC2 should be 16 bytes");

    HDRColorA Color[NUM_PIXELS_PER_BLOCK];
    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        Color[i] = pColor[i];
    }

    auto pBC2 = reinterpret_cast<Block_BC2 *>(pBC);

    // 4-bit alpha part.  Dithered using Floyd Stienberg error diffusion.
    pBC2->bitmap[0] = 0;
    pBC2->bitmap[1] = 0;

    float fError[NUM_PIXELS_PER_BLOCK];
    if (flags & BC_FLAGS_DITHER_A)
        memset(fError, 0x00, NUM_PIXELS_PER_BLOCK * sizeof(float));

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        float fAlph = Color[i].a;
        if (flags & BC_FLAGS_DITHER_A)
            fAlph += fError[i];

        uint32_t u = (uint32_t) static_cast<int32_t>(fAlph * 15.0f + 0.5f);

        pBC2->bitmap[i >> 3] >>= 4;
        pBC2->bitmap[i >> 3] |= (u << 28);

        if (flags & BC_FLAGS_DITHER_A)
        {
            float fDiff = fAlph - (float)u * (1.0f / 15.0f);

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

    // RGB part
#ifdef COLOR_WEIGHTS
    if (!pBC2->bitmap[0] && !pBC2->bitmap[1])
    {
        EncodeSolidBC1(pBC2->dxt1, Color);
        return;
    }
#endif // COLOR_WEIGHTS

    EncodeBC1(&pBC2->bc1, Color, false, 0.f, flags);
}

}
