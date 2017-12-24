#include <stdint.h>
#include <stddef.h>
#include <float.h>

#include "BC.hpp"
#include "BC67_shared.hpp"
#include "Colors.hpp"


namespace Tex {

void InterpolateLDR_RGB(const LDRColorA& c0, const LDRColorA& c1, size_t wc, size_t wcprec, LDRColorA& out)
{
    const int* aWeights = nullptr;
    switch (wcprec)
    {
    case 2: aWeights = g_aWeights2; assert(wc < 4); break;
    case 3: aWeights = g_aWeights3; assert(wc < 8); break;
    case 4: aWeights = g_aWeights4; assert(wc < 16); break;
    default: assert(false); out.r = out.g = out.b = 0; return;
    }
    out.r = uint8_t((uint32_t(c0.r) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.r) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
    out.g = uint8_t((uint32_t(c0.g) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.g) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
    out.b = uint8_t((uint32_t(c0.b) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.b) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
}

void InterpolateLDR_A(
    const LDRColorA& c0, const LDRColorA& c1, size_t wa, size_t waprec,
    LDRColorA& out)
{
    const int* aWeights = nullptr;
    switch (waprec)
    {
    case 2: aWeights = g_aWeights2; assert(wa < 4); break;
    case 3: aWeights = g_aWeights3; assert(wa < 8); break;
    case 4: aWeights = g_aWeights4; assert(wa < 16); break;
    default: assert(false); out.a = 0; return;
    }
    out.a = uint8_t((uint32_t(c0.a) * uint32_t(BC67_WEIGHT_MAX - aWeights[wa]) + uint32_t(c1.a) * uint32_t(aWeights[wa]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
}

void InterpolateLDR(
    const LDRColorA& c0, const LDRColorA& c1, size_t wc, size_t wa,
    size_t wcprec, size_t waprec, LDRColorA& out)
{
    InterpolateLDR_RGB(c0, c1, wc, wcprec, out);
    InterpolateLDR_A(c0, c1, wa, waprec, out);
}

float OptimizeRGB(
    const HDRColorA* const pPoints, HDRColorA* pX, HDRColorA* pY,
    size_t cSteps, size_t cPixels, const size_t* pIndex)
{
    float fError = FLT_MAX;
    const float *pC = (3 == cSteps) ? pC3 : pC4;
    const float *pD = (3 == cSteps) ? pD3 : pD4;

    // Find Min and Max points, as starting point
    HDRColorA X(1.0f, 1.0f, 1.0f, 0.0f);
    HDRColorA Y(0.0f, 0.0f, 0.0f, 0.0f);

    for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
    {
        if (pPoints[pIndex[iPoint]].r < X.r) X.r = pPoints[pIndex[iPoint]].r;
        if (pPoints[pIndex[iPoint]].g < X.g) X.g = pPoints[pIndex[iPoint]].g;
        if (pPoints[pIndex[iPoint]].b < X.b) X.b = pPoints[pIndex[iPoint]].b;
        if (pPoints[pIndex[iPoint]].r > Y.r) Y.r = pPoints[pIndex[iPoint]].r;
        if (pPoints[pIndex[iPoint]].g > Y.g) Y.g = pPoints[pIndex[iPoint]].g;
        if (pPoints[pIndex[iPoint]].b > Y.b) Y.b = pPoints[pIndex[iPoint]].b;
    }

    // Diagonal axis
    HDRColorA AB;
    AB.r = Y.r - X.r;
    AB.g = Y.g - X.g;
    AB.b = Y.b - X.b;

    float fAB = AB.r * AB.r + AB.g * AB.g + AB.b * AB.b;

    // Single color block.. no need to root-find
    if (fAB < FLT_MIN)
    {
        pX->r = X.r; pX->g = X.g; pX->b = X.b;
        pY->r = Y.r; pY->g = Y.g; pY->b = Y.b;
        return 0.0f;
    }

    // Try all four axis directions, to determine which diagonal best fits data
    float fABInv = 1.0f / fAB;

    HDRColorA Dir;
    Dir.r = AB.r * fABInv;
    Dir.g = AB.g * fABInv;
    Dir.b = AB.b * fABInv;

    HDRColorA Mid;
    Mid.r = (X.r + Y.r) * 0.5f;
    Mid.g = (X.g + Y.g) * 0.5f;
    Mid.b = (X.b + Y.b) * 0.5f;

    float fDir[4];
    fDir[0] = fDir[1] = fDir[2] = fDir[3] = 0.0f;

    for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
    {
        HDRColorA Pt;
        Pt.r = (pPoints[pIndex[iPoint]].r - Mid.r) * Dir.r;
        Pt.g = (pPoints[pIndex[iPoint]].g - Mid.g) * Dir.g;
        Pt.b = (pPoints[pIndex[iPoint]].b - Mid.b) * Dir.b;

        float f;
        f = Pt.r + Pt.g + Pt.b; fDir[0] += f * f;
        f = Pt.r + Pt.g - Pt.b; fDir[1] += f * f;
        f = Pt.r - Pt.g + Pt.b; fDir[2] += f * f;
        f = Pt.r - Pt.g - Pt.b; fDir[3] += f * f;
    }

    float fDirMax = fDir[0];
    size_t  iDirMax = 0;

    for (size_t iDir = 1; iDir < 4; iDir++)
    {
        if (fDir[iDir] > fDirMax)
        {
            fDirMax = fDir[iDir];
            iDirMax = iDir;
        }
    }

    if (iDirMax & 2) std::swap(X.g, Y.g);
    if (iDirMax & 1) std::swap(X.b, Y.b);

    // Two color block.. no need to root-find
    if (fAB < 1.0f / 4096.0f)
    {
        pX->r = X.r; pX->g = X.g; pX->b = X.b;
        pY->r = Y.r; pY->g = Y.g; pY->b = Y.b;
        return 0.0f;
    }

    // Use Newton's Method to find local minima of sum-of-squares error.
    float fSteps = (float)(cSteps - 1);

    for (size_t iIteration = 0; iIteration < 8; iIteration++)
    {
        // Calculate new steps
        HDRColorA pSteps[4] = {};

        for (size_t iStep = 0; iStep < cSteps; iStep++)
        {
            pSteps[iStep].r = X.r * pC[iStep] + Y.r * pD[iStep];
            pSteps[iStep].g = X.g * pC[iStep] + Y.g * pD[iStep];
            pSteps[iStep].b = X.b * pC[iStep] + Y.b * pD[iStep];
        }

        // Calculate color direction
        Dir.r = Y.r - X.r;
        Dir.g = Y.g - X.g;
        Dir.b = Y.b - X.b;

        float fLen = (Dir.r * Dir.r + Dir.g * Dir.g + Dir.b * Dir.b);

        if (fLen < (1.0f / 4096.0f))
            break;

        float fScale = fSteps / fLen;

        Dir.r *= fScale;
        Dir.g *= fScale;
        Dir.b *= fScale;

        // Evaluate function, and derivatives
        float d2X = 0.0f, d2Y = 0.0f;
        HDRColorA dX(0.0f, 0.0f, 0.0f, 0.0f), dY(0.0f, 0.0f, 0.0f, 0.0f);

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            float fDot = (pPoints[pIndex[iPoint]].r - X.r) * Dir.r +
                (pPoints[pIndex[iPoint]].g - X.g) * Dir.g +
                (pPoints[pIndex[iPoint]].b - X.b) * Dir.b;

            size_t iStep;
            if (fDot <= 0.0f)
                iStep = 0;
            if (fDot >= fSteps)
                iStep = cSteps - 1;
            else
                iStep = size_t(fDot + 0.5f);

            HDRColorA Diff;
            Diff.r = pSteps[iStep].r - pPoints[pIndex[iPoint]].r;
            Diff.g = pSteps[iStep].g - pPoints[pIndex[iPoint]].g;
            Diff.b = pSteps[iStep].b - pPoints[pIndex[iPoint]].b;

            float fC = pC[iStep] * (1.0f / 8.0f);
            float fD = pD[iStep] * (1.0f / 8.0f);

            d2X += fC * pC[iStep];
            dX.r += fC * Diff.r;
            dX.g += fC * Diff.g;
            dX.b += fC * Diff.b;

            d2Y += fD * pD[iStep];
            dY.r += fD * Diff.r;
            dY.g += fD * Diff.g;
            dY.b += fD * Diff.b;
        }

        // Move endpoints
        if (d2X > 0.0f)
        {
            float f = -1.0f / d2X;

            X.r += dX.r * f;
            X.g += dX.g * f;
            X.b += dX.b * f;
        }

        if (d2Y > 0.0f)
        {
            float f = -1.0f / d2Y;

            Y.r += dY.r * f;
            Y.g += dY.g * f;
            Y.b += dY.b * f;
        }

        if ((dX.r * dX.r < fEpsilon) && (dX.g * dX.g < fEpsilon) && (dX.b * dX.b < fEpsilon) &&
            (dY.r * dY.r < fEpsilon) && (dY.g * dY.g < fEpsilon) && (dY.b * dY.b < fEpsilon))
        {
            break;
        }
    }

    pX->r = X.r; pX->g = X.g; pX->b = X.b;
    pY->r = Y.r; pY->g = Y.g; pY->b = Y.b;
    return fError;
}

void FillWithErrorColors(HDRColorA* pOut)
{
    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
#ifndef NDEBUG
        // Use Magenta in debug as a highly-visible error color
        pOut[i] = HDRColorA(1.0f, 0.0f, 1.0f, 1.0f);
#else
        // In production use, default to black
        pOut[i] = HDRColorA(0.0f, 0.0f, 0.0f, 1.0f);
#endif
    }
}

}
