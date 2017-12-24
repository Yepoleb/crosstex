#include <stddef.h>

#include "BC.hpp"


namespace Tex {

template <bool bRange> void OptimizeAlpha(float *pX, float *pY, const float *pPoints, size_t cSteps)
{
    static const float pC6[] = { 5.0f / 5.0f, 4.0f / 5.0f, 3.0f / 5.0f, 2.0f / 5.0f, 1.0f / 5.0f, 0.0f / 5.0f };
    static const float pD6[] = { 0.0f / 5.0f, 1.0f / 5.0f, 2.0f / 5.0f, 3.0f / 5.0f, 4.0f / 5.0f, 5.0f / 5.0f };
    static const float pC8[] = { 7.0f / 7.0f, 6.0f / 7.0f, 5.0f / 7.0f, 4.0f / 7.0f, 3.0f / 7.0f, 2.0f / 7.0f, 1.0f / 7.0f, 0.0f / 7.0f };
    static const float pD8[] = { 0.0f / 7.0f, 1.0f / 7.0f, 2.0f / 7.0f, 3.0f / 7.0f, 4.0f / 7.0f, 5.0f / 7.0f, 6.0f / 7.0f, 7.0f / 7.0f };

    const float *pC = (6 == cSteps) ? pC6 : pC8;
    const float *pD = (6 == cSteps) ? pD6 : pD8;

    const float MAX_VALUE = 1.0f;
    const float MIN_VALUE = (bRange) ? -1.0f : 0.0f;

    // Find Min and Max points, as starting point
    float fX = MAX_VALUE;
    float fY = MIN_VALUE;

    if (8 == cSteps)
    {
        for (size_t iPoint = 0; iPoint < NUM_PIXELS_PER_BLOCK; iPoint++)
        {
            if (pPoints[iPoint] < fX)
                fX = pPoints[iPoint];

            if (pPoints[iPoint] > fY)
                fY = pPoints[iPoint];
        }
    }
    else
    {
        for (size_t iPoint = 0; iPoint < NUM_PIXELS_PER_BLOCK; iPoint++)
        {
            if (pPoints[iPoint] < fX && pPoints[iPoint] > MIN_VALUE)
                fX = pPoints[iPoint];

            if (pPoints[iPoint] > fY && pPoints[iPoint] < MAX_VALUE)
                fY = pPoints[iPoint];
        }

        if (fX == fY)
        {
            fY = MAX_VALUE;
        }
    }

    // Use Newton's Method to find local minima of sum-of-squares error.
    float fSteps = (float)(cSteps - 1);

    for (size_t iIteration = 0; iIteration < 8; iIteration++)
    {
        float fScale;

        if ((fY - fX) < (1.0f / 256.0f))
            break;

        fScale = fSteps / (fY - fX);

        // Calculate new steps
        float pSteps[8];

        for (size_t iStep = 0; iStep < cSteps; iStep++)
            pSteps[iStep] = pC[iStep] * fX + pD[iStep] * fY;

        if (6 == cSteps)
        {
            pSteps[6] = MIN_VALUE;
            pSteps[7] = MAX_VALUE;
        }

        // Evaluate function, and derivatives
        float dX = 0.0f;
        float dY = 0.0f;
        float d2X = 0.0f;
        float d2Y = 0.0f;

        for (size_t iPoint = 0; iPoint < NUM_PIXELS_PER_BLOCK; iPoint++)
        {
            float fDot = (pPoints[iPoint] - fX) * fScale;

            size_t iStep;

            if (fDot <= 0.0f)
                iStep = ((6 == cSteps) && (pPoints[iPoint] <= fX * 0.5f)) ? 6 : 0;
            else if (fDot >= fSteps)
                iStep = ((6 == cSteps) && (pPoints[iPoint] >= (fY + 1.0f) * 0.5f)) ? 7 : (cSteps - 1);
            else
                iStep = static_cast<int32_t>(fDot + 0.5f);


            if (iStep < cSteps)
            {
                // D3DX had this computation backwards (pPoints[iPoint] - pSteps[iStep])
                // this fix improves RMS of the alpha component
                float fDiff = pSteps[iStep] - pPoints[iPoint];

                dX += pC[iStep] * fDiff;
                d2X += pC[iStep] * pC[iStep];

                dY += pD[iStep] * fDiff;
                d2Y += pD[iStep] * pD[iStep];
            }
        }

        // Move endpoints
        if (d2X > 0.0f)
            fX -= dX / d2X;

        if (d2Y > 0.0f)
            fY -= dY / d2Y;

        if (fX > fY)
        {
            float f = fX; fX = fY; fY = f;
        }

        if ((dX * dX < (1.0f / 64.0f)) && (dY * dY < (1.0f / 64.0f)))
            break;
    }

    *pX = (fX < MIN_VALUE) ? MIN_VALUE : (fX > MAX_VALUE) ? MAX_VALUE : fX;
    *pY = (fY < MIN_VALUE) ? MIN_VALUE : (fY > MAX_VALUE) ? MAX_VALUE : fY;
}

}
