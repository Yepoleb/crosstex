#include <stdint.h>
#include <stddef.h>
#include <cmath>

#include "BC.hpp"
#include "BC45_shared.hpp"
#include "Colors.hpp"
#include "OptimizeAlpha.hpp"


namespace Tex {

const size_t BLOCK_LEN = 4;
// length of each block in texel

const size_t BLOCK_SIZE = (BLOCK_LEN * BLOCK_LEN);
// total texels in a 4x4 block.

//-------------------------------------------------------------------------------------
// Convert a floating point value to an 8-bit SNORM
//-------------------------------------------------------------------------------------
void inline FloatToSNorm(float fVal, int8_t *piSNorm)
{
    const uint32_t dwMostNeg = (1 << (8 * sizeof(int8_t) - 1));

    if (std::isnan(fVal))
        fVal = 0;
    else
        if (fVal > 1)
            fVal = 1;    // Clamp to 1
        else
            if (fVal < -1)
                fVal = -1;    // Clamp to -1

    fVal = fVal * (int8_t)(dwMostNeg - 1);

    if (fVal >= 0)
        fVal += .5f;
    else
        fVal -= .5f;

    *piSNorm = (int8_t)(fVal);
}


//------------------------------------------------------------------------------
void FindEndPointsBC4U(const float* theTexelsU, uint8_t &endpointU_0, uint8_t &endpointU_1)
{
    // The boundary of codec for signed/unsigned format
    const float MIN_NORM = 0.f;
    const float MAX_NORM = 1.f;

    // Find max/min of input texels
    float fBlockMax = theTexelsU[0];
    float fBlockMin = theTexelsU[0];
    for (size_t i = 0; i < BLOCK_SIZE; ++i)
    {
        if (theTexelsU[i] < fBlockMin)
        {
            fBlockMin = theTexelsU[i];
        }
        else if (theTexelsU[i] > fBlockMax)
        {
            fBlockMax = theTexelsU[i];
        }
    }

    //  If there are boundary values in input texels, should use 4 interpolated color values to guarantee
    //  the exact code of the boundary values.
    bool bUsing4BlockCodec = (MIN_NORM == fBlockMin || MAX_NORM == fBlockMax);

    // Using Optimize
    float fStart, fEnd;

    if (!bUsing4BlockCodec)
    {
        // 6 interpolated color values
        OptimizeAlpha<false>(&fStart, &fEnd, theTexelsU, 8);

        uint8_t iStart = static_cast<uint8_t>(fStart * 255.0f);
        uint8_t iEnd = static_cast<uint8_t>(fEnd * 255.0f);

        endpointU_0 = iEnd;
        endpointU_1 = iStart;
    }
    else
    {
        // 4 interpolated color values
        OptimizeAlpha<false>(&fStart, &fEnd, theTexelsU, 6);

        uint8_t iStart = static_cast<uint8_t>(fStart * 255.0f);
        uint8_t iEnd = static_cast<uint8_t>(fEnd * 255.0f);

        endpointU_1 = iEnd;
        endpointU_0 = iStart;
    }
}

void FindEndPointsBC4S(const float* theTexelsU, int8_t &endpointU_0, int8_t &endpointU_1)
{
    // The boundary of codec for signed/unsigned format
    const float MIN_NORM = -1.f;
    const float MAX_NORM = 1.f;

    // Find max/min of input texels
    float fBlockMax = theTexelsU[0];
    float fBlockMin = theTexelsU[0];
    for (size_t i = 0; i < BLOCK_SIZE; ++i)
    {
        if (theTexelsU[i] < fBlockMin)
        {
            fBlockMin = theTexelsU[i];
        }
        else if (theTexelsU[i] > fBlockMax)
        {
            fBlockMax = theTexelsU[i];
        }
    }

    //  If there are boundary values in input texels, should use 4 interpolated color values to guarantee
    //  the exact code of the boundary values.
    bool bUsing4BlockCodec = (MIN_NORM == fBlockMin || MAX_NORM == fBlockMax);

    // Using Optimize
    float fStart, fEnd;

    if (!bUsing4BlockCodec)
    {
        // 6 interpolated color values
        OptimizeAlpha<true>(&fStart, &fEnd, theTexelsU, 8);

        int8_t iStart, iEnd;
        FloatToSNorm(fStart, &iStart);
        FloatToSNorm(fEnd, &iEnd);

        endpointU_0 = iEnd;
        endpointU_1 = iStart;
    }
    else
    {
        // 4 interpolated color values
        OptimizeAlpha<true>(&fStart, &fEnd, theTexelsU, 6);

        int8_t iStart, iEnd;
        FloatToSNorm(fStart, &iStart);
        FloatToSNorm(fEnd, &iEnd);

        endpointU_1 = iEnd;
        endpointU_0 = iStart;
    }
}

//------------------------------------------------------------------------------
void FindClosestUNORM(BC4_UNORM* pBC, const float* theTexelsU)
{
    float rGradient[8];
    for (size_t i = 0; i < 8; ++i)
    {
        rGradient[i] = pBC->DecodeFromIndex(i);
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        size_t uBestIndex = 0;
        float fBestDelta = 100000;
        for (size_t uIndex = 0; uIndex < 8; uIndex++)
        {
            float fCurrentDelta = fabsf(rGradient[uIndex] - theTexelsU[i]);
            if (fCurrentDelta < fBestDelta)
            {
                uBestIndex = uIndex;
                fBestDelta = fCurrentDelta;
            }
        }
        pBC->SetIndex(i, uBestIndex);
    }
}

void FindClosestSNORM(BC4_SNORM* pBC, const float* theTexelsU)
{
    float rGradient[8];
    for (size_t i = 0; i < 8; ++i)
    {
        rGradient[i] = pBC->DecodeFromIndex(i);
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        size_t uBestIndex = 0;
        float fBestDelta = 100000;
        for (size_t uIndex = 0; uIndex < 8; uIndex++)
        {
            float fCurrentDelta = fabsf(rGradient[uIndex] - theTexelsU[i]);
            if (fCurrentDelta < fBestDelta)
            {
                uBestIndex = uIndex;
                fBestDelta = fCurrentDelta;
            }
        }
        pBC->SetIndex(i, uBestIndex);
    }
}

}
