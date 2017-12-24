#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "BC.hpp"
#include "BC123_shared.hpp"
#include "Colors.hpp"
#include "OptimizeAlpha.hpp"


namespace Tex {

void DecodeBC3(HDRColorA *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(Block_BC3) == 16, "Block_BC3 should be 16 bytes");

    auto pBC3 = reinterpret_cast<const Block_BC3 *>(pBC);

    // RGB part
    DecodeBC1(pColor, &pBC3->bc1, false);

    // Adaptive 3-bit alpha part
    float fAlpha[8];

    fAlpha[0] = ((float)pBC3->alpha[0]) * (1.0f / 255.0f);
    fAlpha[1] = ((float)pBC3->alpha[1]) * (1.0f / 255.0f);

    if (pBC3->alpha[0] > pBC3->alpha[1])
    {
        for (size_t i = 1; i < 7; ++i)
            fAlpha[i + 1] = (fAlpha[0] * (7 - i) + fAlpha[1] * i) * (1.0f / 7.0f);
    }
    else
    {
        for (size_t i = 1; i < 5; ++i)
            fAlpha[i + 1] = (fAlpha[0] * (5 - i) + fAlpha[1] * i) * (1.0f / 5.0f);

        fAlpha[6] = 0.0f;
        fAlpha[7] = 1.0f;
    }

    uint32_t dw = pBC3->bitmap[0] | (pBC3->bitmap[1] << 8) | (pBC3->bitmap[2] << 16);

    for (size_t i = 0; i < 8; ++i, dw >>= 3)
        pColor[i].a = fAlpha[dw & 0x7];

    dw = pBC3->bitmap[3] | (pBC3->bitmap[4] << 8) | (pBC3->bitmap[5] << 16);

    for (size_t i = 8; i < NUM_PIXELS_PER_BLOCK; ++i, dw >>= 3)
        pColor[i].a = fAlpha[dw & 0x7];
}

void EncodeBC3(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    assert(pBC && pColor);
    static_assert(sizeof(Block_BC3) == 16, "Block_BC3 should be 16 bytes");

    HDRColorA Color[NUM_PIXELS_PER_BLOCK];
    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        Color[i] = pColor[i];
    }

    auto pBC3 = reinterpret_cast<Block_BC3 *>(pBC);

    // Quantize block to A8, using Floyd Stienberg error diffusion.  This
    // increases the chance that colors will map directly to the quantized
    // axis endpoints.
    float fAlpha[NUM_PIXELS_PER_BLOCK];
    float fError[NUM_PIXELS_PER_BLOCK];

    float fMinAlpha = Color[0].a;
    float fMaxAlpha = Color[0].a;

    if (flags & BC_FLAGS_DITHER_A)
        memset(fError, 0x00, NUM_PIXELS_PER_BLOCK * sizeof(float));

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        float fAlph = Color[i].a;
        if (flags & BC_FLAGS_DITHER_A)
            fAlph += fError[i];

        fAlpha[i] = static_cast<int32_t>(fAlph * 255.0f + 0.5f) * (1.0f / 255.0f);

        if (fAlpha[i] < fMinAlpha)
            fMinAlpha = fAlpha[i];
        else if (fAlpha[i] > fMaxAlpha)
            fMaxAlpha = fAlpha[i];

        if (flags & BC_FLAGS_DITHER_A)
        {
            float fDiff = fAlph - fAlpha[i];

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

#ifdef COLOR_WEIGHTS
    if (0.0f == fMaxAlpha)
    {
        EncodeSolidBC1(&pBC3->dxt1, Color);
        pBC3->alpha[0] = 0x00;
        pBC3->alpha[1] = 0x00;
        memset(pBC3->bitmap, 0x00, 6);
    }
#endif

    // RGB part
    EncodeBC1(&pBC3->bc1, Color, false, 0.f, flags);

    // Alpha part
    if (1.0f == fMinAlpha)
    {
        pBC3->alpha[0] = 0xff;
        pBC3->alpha[1] = 0xff;
        memset(pBC3->bitmap, 0x00, 6);
        return;
    }

    // Optimize and Quantize Min and Max values
    size_t uSteps = ((0.0f == fMinAlpha) || (1.0f == fMaxAlpha)) ? 6 : 8;

    float fAlphaA, fAlphaB;
    OptimizeAlpha<false>(&fAlphaA, &fAlphaB, fAlpha, uSteps);

    uint8_t bAlphaA = (uint8_t) static_cast<int32_t>(fAlphaA * 255.0f + 0.5f);
    uint8_t bAlphaB = (uint8_t) static_cast<int32_t>(fAlphaB * 255.0f + 0.5f);

    fAlphaA = (float)bAlphaA * (1.0f / 255.0f);
    fAlphaB = (float)bAlphaB * (1.0f / 255.0f);

    // Setup block
    if ((8 == uSteps) && (bAlphaA == bAlphaB))
    {
        pBC3->alpha[0] = bAlphaA;
        pBC3->alpha[1] = bAlphaB;
        memset(pBC3->bitmap, 0x00, 6);
        return;
    }

    static const size_t pSteps6[] = { 0, 2, 3, 4, 5, 1 };
    static const size_t pSteps8[] = { 0, 2, 3, 4, 5, 6, 7, 1 };

    const size_t *pSteps;
    float fStep[8];

    if (6 == uSteps)
    {
        pBC3->alpha[0] = bAlphaA;
        pBC3->alpha[1] = bAlphaB;

        fStep[0] = fAlphaA;
        fStep[1] = fAlphaB;

        for (size_t i = 1; i < 5; ++i)
            fStep[i + 1] = (fStep[0] * (5 - i) + fStep[1] * i) * (1.0f / 5.0f);

        fStep[6] = 0.0f;
        fStep[7] = 1.0f;

        pSteps = pSteps6;
    }
    else
    {
        pBC3->alpha[0] = bAlphaB;
        pBC3->alpha[1] = bAlphaA;

        fStep[0] = fAlphaB;
        fStep[1] = fAlphaA;

        for (size_t i = 1; i < 7; ++i)
            fStep[i + 1] = (fStep[0] * (7 - i) + fStep[1] * i) * (1.0f / 7.0f);

        pSteps = pSteps8;
    }

    // Encode alpha bitmap
    float fSteps = (float)(uSteps - 1);
    float fScale = (fStep[0] != fStep[1]) ? (fSteps / (fStep[1] - fStep[0])) : 0.0f;

    if (flags & BC_FLAGS_DITHER_A)
        memset(fError, 0x00, NUM_PIXELS_PER_BLOCK * sizeof(float));

    for (size_t iSet = 0; iSet < 2; iSet++)
    {
        uint32_t dw = 0;

        size_t iMin = iSet * 8;
        size_t iLim = iMin + 8;

        for (size_t i = iMin; i < iLim; ++i)
        {
            float fAlph = Color[i].a;
            if (flags & BC_FLAGS_DITHER_A)
                fAlph += fError[i];
            float fDot = (fAlph - fStep[0]) * fScale;

            uint32_t iStep;
            if (fDot <= 0.0f)
                iStep = ((6 == uSteps) && (fAlph <= fStep[0] * 0.5f)) ? 6 : 0;
            else if (fDot >= fSteps)
                iStep = ((6 == uSteps) && (fAlph >= (fStep[1] + 1.0f) * 0.5f)) ? 7 : 1;
            else
                iStep = static_cast<uint32_t>(pSteps[static_cast<size_t>(fDot + 0.5f)]);

            dw = (iStep << 21) | (dw >> 3);

            if (flags & BC_FLAGS_DITHER_A)
            {
                float fDiff = (fAlph - fStep[iStep]);

                if (3 != (i & 3))
                    fError[i + 1] += fDiff * (7.0f / 16.0f);

                if (i < 12)
                {
                    if (i & 3)
                        fError[i + 3] += fDiff * (3.0f / 16.0f);

                    fError[i + 4] += fDiff * (5.0f / 16.0f);

                    if (3 != (i & 3))
                        fError[i + 5] += fDiff * (1.0f / 16.0f);
                }
            }
        }

        pBC3->bitmap[0 + iSet * 3] = ((uint8_t *)&dw)[0];
        pBC3->bitmap[1 + iSet * 3] = ((uint8_t *)&dw)[1];
        pBC3->bitmap[2 + iSet * 3] = ((uint8_t *)&dw)[2];
    }
}

}
