#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <float.h>
#include <stdio.h>

#include "BC.hpp"
#include "BC67_shared.hpp"
#include "Colors.hpp"


namespace Tex {

const size_t BC7_MAX_REGIONS = 3;
const size_t BC7_MAX_INDICES = 16;
const size_t BC7_NUM_CHANNELS = 4;
const size_t BC7_MAX_SHAPES = 64;


// BC67 compression (16b bits per texel)
class Block_BC7 : private CBits<16>
{
public:
    void Decode(HDRColorA* pOut) const;
    void Encode(uint32_t flags, const HDRColorA* const pIn);

private:
    struct ModeInfo
    {
        uint8_t uPartitions;
        uint8_t uPartitionBits;
        uint8_t uPBits;
        uint8_t uRotationBits;
        uint8_t uIndexModeBits;
        uint8_t uIndexPrec;
        uint8_t uIndexPrec2;
        LDRColorA RGBAPrec;
        LDRColorA RGBAPrecWithP;
    };

    struct EncodeParams
    {
        uint8_t uMode;
        LDREndPntPair aEndPts[BC7_MAX_SHAPES][BC7_MAX_REGIONS];
        LDRColorA aLDRPixels[NUM_PIXELS_PER_BLOCK];
        const HDRColorA* const aHDRPixels;

        EncodeParams(const HDRColorA* const aOriginal) : aHDRPixels(aOriginal) {}
    };

    static uint8_t Quantize(uint8_t comp, uint8_t uPrec)
    {
        assert(0 < uPrec && uPrec <= 8);
        uint8_t rnd = (uint8_t)std::min<uint16_t>(255, uint16_t(comp) + (1 << (7 - uPrec)));
        return rnd >> (8 - uPrec);
    }

    static LDRColorA Quantize(const LDRColorA& c, const LDRColorA& RGBAPrec)
    {
        LDRColorA q;
        q.r = Quantize(c.r, RGBAPrec.r);
        q.g = Quantize(c.g, RGBAPrec.g);
        q.b = Quantize(c.b, RGBAPrec.b);
        if (RGBAPrec.a)
            q.a = Quantize(c.a, RGBAPrec.a);
        else
            q.a = 255;
        return q;
    }

    static uint8_t Unquantize(uint8_t comp, size_t uPrec)
    {
        assert(0 < uPrec && uPrec <= 8);
        comp = comp << (8 - uPrec);
        return comp | (comp >> uPrec);
    }

    static LDRColorA Unquantize(const LDRColorA& c, const LDRColorA& RGBAPrec)
    {
        LDRColorA q;
        q.r = Unquantize(c.r, RGBAPrec.r);
        q.g = Unquantize(c.g, RGBAPrec.g);
        q.b = Unquantize(c.b, RGBAPrec.b);
        q.a = RGBAPrec.a > 0 ? Unquantize(c.a, RGBAPrec.a) : 255;
        return q;
    }

    void GeneratePaletteQuantized(const EncodeParams* pEP, size_t uIndexMode, const LDREndPntPair& endpts,
        LDRColorA aPalette[]) const;
    float PerturbOne(const EncodeParams* pEP, const LDRColorA colors[], size_t np, size_t uIndexMode,
        size_t ch, const LDREndPntPair &old_endpts,
        LDREndPntPair &new_endpts, float old_err, uint8_t do_b) const;
    void Exhaustive(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode,
        size_t ch, float& fOrgErr, LDREndPntPair& optEndPt) const;
    void OptimizeOne(const EncodeParams* pEP, const LDRColorA colors[], size_t np, size_t uIndexMode,
        float orig_err, const LDREndPntPair &orig_endpts, LDREndPntPair &opt_endpts) const;
    void OptimizeEndPoints(const EncodeParams* pEP, size_t uShape, size_t uIndexMode,
        const float orig_err[],
        const LDREndPntPair orig_endpts[],
        LDREndPntPair opt_endpts[]) const;
    void AssignIndices(const EncodeParams* pEP, size_t uShape, size_t uIndexMode,
        LDREndPntPair endpts[],
        size_t aIndices[], size_t aIndices2[],
        float afTotErr[]) const;
    void EmitBlock(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode,
        const LDREndPntPair aEndPts[],
        const size_t aIndex[],
        const size_t aIndex2[]);
    float Refine(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode);

    float MapColors(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode,
        const LDREndPntPair& endPts, float fMinErr) const;
    static float RoughMSE(EncodeParams* pEP, size_t uShape, size_t uIndexMode);

private:
    const static ModeInfo ms_aInfo[];
};

// BC7 compression: uPartitions, uPartitionBits, uPBits, uRotationBits, uIndexModeBits, uIndexPrec, uIndexPrec2, RGBAPrec, RGBAPrecWithP
const Block_BC7::ModeInfo Block_BC7::ms_aInfo[] =
{
    {2, 4, 6, 0, 0, 3, 0, LDRColorA(4,4,4,0), LDRColorA(5,5,5,0)},
        // Mode 0: Color only, 3 Subsets, RGBP 4441 (unique P-bit), 3-bit indecies, 16 partitions
    {1, 6, 2, 0, 0, 3, 0, LDRColorA(6,6,6,0), LDRColorA(7,7,7,0)},
        // Mode 1: Color only, 2 Subsets, RGBP 6661 (shared P-bit), 3-bit indecies, 64 partitions
    {2, 6, 0, 0, 0, 2, 0, LDRColorA(5,5,5,0), LDRColorA(5,5,5,0)},
        // Mode 2: Color only, 3 Subsets, RGB 555, 2-bit indecies, 64 partitions
    {1, 6, 4, 0, 0, 2, 0, LDRColorA(7,7,7,0), LDRColorA(8,8,8,0)},
        // Mode 3: Color only, 2 Subsets, RGBP 7771 (unique P-bit), 2-bits indecies, 64 partitions
    {0, 0, 0, 2, 1, 2, 3, LDRColorA(5,5,5,6), LDRColorA(5,5,5,6)},
        // Mode 4: Color w/ Separate Alpha, 1 Subset, RGB 555, A6, 16x2/16x3-bit indices, 2-bit rotation, 1-bit index selector
    {0, 0, 0, 2, 0, 2, 2, LDRColorA(7,7,7,8), LDRColorA(7,7,7,8)},
        // Mode 5: Color w/ Separate Alpha, 1 Subset, RGB 777, A8, 16x2/16x2-bit indices, 2-bit rotation
    {0, 0, 2, 0, 0, 4, 0, LDRColorA(7,7,7,7), LDRColorA(8,8,8,8)},
        // Mode 6: Color+Alpha, 1 Subset, RGBAP 77771 (unique P-bit), 16x4-bit indecies
    {1, 6, 4, 0, 0, 2, 0, LDRColorA(5,5,5,5), LDRColorA(6,6,6,6)}
        // Mode 7: Color+Alpha, 2 Subsets, RGBAP 55551 (unique P-bit), 2-bit indices, 64 partitions
};

float OptimizeRGBA(
    const HDRColorA* const pPoints, HDRColorA* pX, HDRColorA* pY,
    size_t cSteps, size_t cPixels, const size_t* pIndex)
{
    float fError = FLT_MAX;
    const float *pC = (3 == cSteps) ? pC3 : pC4;
    const float *pD = (3 == cSteps) ? pD3 : pD4;

    // Find Min and Max points, as starting point
    HDRColorA X(1.0f, 1.0f, 1.0f, 1.0f);
    HDRColorA Y(0.0f, 0.0f, 0.0f, 0.0f);

    for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
    {
        if (pPoints[pIndex[iPoint]].r < X.r) X.r = pPoints[pIndex[iPoint]].r;
        if (pPoints[pIndex[iPoint]].g < X.g) X.g = pPoints[pIndex[iPoint]].g;
        if (pPoints[pIndex[iPoint]].b < X.b) X.b = pPoints[pIndex[iPoint]].b;
        if (pPoints[pIndex[iPoint]].a < X.a) X.a = pPoints[pIndex[iPoint]].a;
        if (pPoints[pIndex[iPoint]].r > Y.r) Y.r = pPoints[pIndex[iPoint]].r;
        if (pPoints[pIndex[iPoint]].g > Y.g) Y.g = pPoints[pIndex[iPoint]].g;
        if (pPoints[pIndex[iPoint]].b > Y.b) Y.b = pPoints[pIndex[iPoint]].b;
        if (pPoints[pIndex[iPoint]].a > Y.a) Y.a = pPoints[pIndex[iPoint]].a;
    }

    // Diagonal axis
    HDRColorA AB = Y - X;
    float fAB = AB.Length2();

    // Single color block.. no need to root-find
    if (fAB < FLT_MIN)
    {
        *pX = X;
        *pY = Y;
        return 0.0f;
    }

    // Try all four axis directions, to determine which diagonal best fits data
    float fABInv = 1.0f / fAB;
    HDRColorA Dir = AB * fABInv;
    HDRColorA Mid = (X + Y) * 0.5f;

    float fDir[8];
    fDir[0] = fDir[1] = fDir[2] = fDir[3] = fDir[4] = fDir[5] = fDir[6] = fDir[7] = 0.0f;

    for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
    {
        HDRColorA Pt;
        Pt.r = (pPoints[pIndex[iPoint]].r - Mid.r) * Dir.r;
        Pt.g = (pPoints[pIndex[iPoint]].g - Mid.g) * Dir.g;
        Pt.b = (pPoints[pIndex[iPoint]].b - Mid.b) * Dir.b;
        Pt.a = (pPoints[pIndex[iPoint]].a - Mid.a) * Dir.a;

        float f;
        f = Pt.r + Pt.g + Pt.b + Pt.a; fDir[0] += f * f;
        f = Pt.r + Pt.g + Pt.b - Pt.a; fDir[1] += f * f;
        f = Pt.r + Pt.g - Pt.b + Pt.a; fDir[2] += f * f;
        f = Pt.r + Pt.g - Pt.b - Pt.a; fDir[3] += f * f;
        f = Pt.r - Pt.g + Pt.b + Pt.a; fDir[4] += f * f;
        f = Pt.r - Pt.g + Pt.b - Pt.a; fDir[5] += f * f;
        f = Pt.r - Pt.g - Pt.b + Pt.a; fDir[6] += f * f;
        f = Pt.r - Pt.g - Pt.b - Pt.a; fDir[7] += f * f;
    }

    float fDirMax = fDir[0];
    size_t  iDirMax = 0;

    for (size_t iDir = 1; iDir < 8; iDir++)
    {
        if (fDir[iDir] > fDirMax)
        {
            fDirMax = fDir[iDir];
            iDirMax = iDir;
        }
    }

    if (iDirMax & 4) std::swap(X.g, Y.g);
    if (iDirMax & 2) std::swap(X.b, Y.b);
    if (iDirMax & 1) std::swap(X.a, Y.a);

    // Two color block.. no need to root-find
    if (fAB < 1.0f / 4096.0f)
    {
        *pX = X;
        *pY = Y;
        return 0.0f;
    }

    // Use Newton's Method to find local minima of sum-of-squares error.
    float fSteps = (float)(cSteps - 1);

    for (size_t iIteration = 0; iIteration < 8 && fError > 0.0f; iIteration++)
    {
        // Calculate new steps
        HDRColorA pSteps[BC7_MAX_INDICES];

        for (size_t iStep = 0; iStep < cSteps; iStep++)
        {
            pSteps[iStep] = X * pC[iStep] + Y * pD[iStep];
            //LDRColorA::Interpolate(lX, lY, i, i, wcprec, waprec, aSteps[i]);
        }

        // Calculate color direction
        Dir = Y - X;
        float fLen = Dir.Length2();
        if (fLen < (1.0f / 4096.0f))
            break;

        float fScale = fSteps / fLen;
        Dir *= fScale;

        // Evaluate function, and derivatives
        float d2X = 0.0f, d2Y = 0.0f;
        HDRColorA dX(0.0f, 0.0f, 0.0f, 0.0f), dY(0.0f, 0.0f, 0.0f, 0.0f);

        for (size_t iPoint = 0; iPoint < cPixels; ++iPoint)
        {
            float fDot = HDRColorA::DotProduct((pPoints[pIndex[iPoint]] - X), Dir);
            size_t iStep;
            if (fDot <= 0.0f)
                iStep = 0;
            if (fDot >= fSteps)
                iStep = cSteps - 1;
            else
                iStep = size_t(fDot + 0.5f);

            HDRColorA Diff = pSteps[iStep] - pPoints[pIndex[iPoint]];
            float fC = pC[iStep] * (1.0f / 8.0f);
            float fD = pD[iStep] * (1.0f / 8.0f);

            d2X += fC * pC[iStep];
            dX += Diff * fC;

            d2Y += fD * pD[iStep];
            dY += Diff * fD;
        }

        // Move endpoints
        if (d2X > 0.0f)
        {
            float f = -1.0f / d2X;
            X += dX * f;
        }

        if (d2Y > 0.0f)
        {
            float f = -1.0f / d2Y;
            Y += dY * f;
        }

        if ((dX.Length2() < fEpsilon) && (dY.Length2() < fEpsilon))
            break;
    }

    *pX = X;
    *pY = Y;
    return fError;
}

float ComputeError(const LDRColorA& pixel, const LDRColorA aPalette[],
    uint8_t uIndexPrec, uint8_t uIndexPrec2, size_t* pBestIndex = nullptr,
    size_t* pBestIndex2 = nullptr)
{
    const size_t uNumIndices = size_t(1) << uIndexPrec;
    const size_t uNumIndices2 = size_t(1) << uIndexPrec2;
    float fTotalErr = 0;
    float fBestErr = FLT_MAX;

    if (pBestIndex)
        *pBestIndex = 0;
    if (pBestIndex2)
        *pBestIndex2 = 0;

    if (uIndexPrec2 == 0)
    {
        for (size_t i = 0; i < uNumIndices && fBestErr > 0; i++)
        {
            LDRColorA tpixel = aPalette[i];
            // Compute ErrorMetric
            tpixel = pixel - tpixel;
            float fErr = LDRColorA::DotProduct(tpixel, tpixel);
            if (fErr > fBestErr)	// error increased, so we're done searching
                break;
            if (fErr < fBestErr)
            {
                fBestErr = fErr;
                if (pBestIndex)
                    *pBestIndex = i;
            }
        }
        fTotalErr += fBestErr;
    }
    else
    {
        for (size_t i = 0; i < uNumIndices && fBestErr > 0; i++)
        {
            LDRColorA tpixel = aPalette[i];
            // Compute ErrorMetricRGB
            tpixel = pixel - tpixel;
            float fErr = LDRColorA::DotProduct(tpixel, tpixel);
            if (fErr > fBestErr)	// error increased, so we're done searching
                break;
            if (fErr < fBestErr)
            {
                fBestErr = fErr;
                if (pBestIndex)
                    *pBestIndex = i;
            }
        }
        fTotalErr += fBestErr;
        fBestErr = FLT_MAX;
        for (size_t i = 0; i < uNumIndices2 && fBestErr > 0; i++)
        {
            // Compute ErrorMetricAlpha
            float ea = float(pixel.a) - float(aPalette[i].a);
            float fErr = ea*ea;
            if (fErr > fBestErr)	// error increased, so we're done searching
                break;
            if (fErr < fBestErr)
            {
                fBestErr = fErr;
                if (pBestIndex2)
                    *pBestIndex2 = i;
            }
        }
        fTotalErr += fBestErr;
    }

    return fTotalErr;
}

//-------------------------------------------------------------------------------------
// BC7 Compression
//-------------------------------------------------------------------------------------
void Block_BC7::Decode(HDRColorA* pOut) const
{
    assert(pOut);

    size_t uFirst = 0;
    while (uFirst < 128 && !GetBit(uFirst)) {}
    uint8_t uMode = uint8_t(uFirst - 1);

    if (uMode < 8)
    {
        const uint8_t uPartitions = ms_aInfo[uMode].uPartitions;
        assert(uPartitions < BC7_MAX_REGIONS);

        const uint8_t uNumEndPts = (uPartitions + 1) << 1;
        const uint8_t uIndexPrec = ms_aInfo[uMode].uIndexPrec;
        const uint8_t uIndexPrec2 = ms_aInfo[uMode].uIndexPrec2;
        size_t i;
        size_t uStartBit = uMode + 1;
        uint8_t P[6];
        uint8_t uShape = GetBits(uStartBit, ms_aInfo[uMode].uPartitionBits);
        assert(uShape < BC7_MAX_SHAPES);

        uint8_t uRotation = GetBits(uStartBit, ms_aInfo[uMode].uRotationBits);
        assert(uRotation < 4);

        uint8_t uIndexMode = GetBits(uStartBit, ms_aInfo[uMode].uIndexModeBits);
        assert(uIndexMode < 2);

        LDRColorA c[BC7_MAX_REGIONS << 1];
        const LDRColorA RGBAPrec = ms_aInfo[uMode].RGBAPrec;
        const LDRColorA RGBAPrecWithP = ms_aInfo[uMode].RGBAPrecWithP;

        assert(uNumEndPts <= (BC7_MAX_REGIONS << 1));

        // Red channel
        for (i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.r > 128)
            {
#ifndef NDEBUG
                fprintf(stderr, "BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            c[i].r = GetBits(uStartBit, RGBAPrec.r);
        }

        // Green channel
        for (i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.g > 128)
            {
#ifndef NDEBUG
                fprintf(stderr, "BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            c[i].g = GetBits(uStartBit, RGBAPrec.g);
        }

        // Blue channel
        for (i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.b > 128)
            {
#ifndef NDEBUG
                fprintf(stderr, "BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            c[i].b = GetBits(uStartBit, RGBAPrec.b);
        }

        // Alpha channel
        for (i = 0; i < uNumEndPts; i++)
        {
            if (uStartBit + RGBAPrec.a > 128)
            {
#ifndef NDEBUG
                fprintf(stderr, "BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            c[i].a = RGBAPrec.a ? GetBits(uStartBit, RGBAPrec.a) : 255;
        }

        // P-bits
        assert(ms_aInfo[uMode].uPBits <= 6);
                for (i = 0; i < ms_aInfo[uMode].uPBits; i++)
        {
            if (uStartBit > 127)
            {
#ifndef NDEBUG
                fprintf(stderr, "BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            P[i] = GetBit(uStartBit);
        }

        if (ms_aInfo[uMode].uPBits)
        {
            for (i = 0; i < uNumEndPts; i++)
            {
                size_t pi = i * ms_aInfo[uMode].uPBits / uNumEndPts;
                for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
                {
                    if (RGBAPrec[ch] != RGBAPrecWithP[ch])
                    {
                        c[i][ch] = (c[i][ch] << 1) | P[pi];
                    }
                }
            }
        }

        for (i = 0; i < uNumEndPts; i++)
        {
            c[i] = Unquantize(c[i], RGBAPrecWithP);
        }

        uint8_t w1[NUM_PIXELS_PER_BLOCK], w2[NUM_PIXELS_PER_BLOCK];

        // read color indices
        for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
        {
            size_t uNumBits = IsFixUpOffset(ms_aInfo[uMode].uPartitions, uShape, i) ? uIndexPrec - 1 : uIndexPrec;
            if (uStartBit + uNumBits > 128)
            {
#ifndef NDEBUG
                fprintf(stderr, "BC7: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }
            w1[i] = GetBits(uStartBit, uNumBits);
        }

        // read alpha indices
        if (uIndexPrec2)
        {
            for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
            {
                size_t uNumBits = i ? uIndexPrec2 : uIndexPrec2 - 1;
                if (uStartBit + uNumBits > 128)
                {
#ifndef NDEBUG
                    fprintf(stderr, "BC7: Invalid block encountered during decoding\n");
#endif
                    FillWithErrorColors(pOut);
                    return;
                }
                w2[i] = GetBits(uStartBit, uNumBits);
            }
        }

        for (i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][i];
            LDRColorA outPixel;
            if (uIndexPrec2 == 0)
            {
                InterpolateLDR(c[uRegion << 1], c[(uRegion << 1) + 1], w1[i], w1[i], uIndexPrec, uIndexPrec, outPixel);
            }
            else
            {
                if (uIndexMode == 0)
                {
                    InterpolateLDR(c[uRegion << 1], c[(uRegion << 1) + 1], w1[i], w2[i], uIndexPrec, uIndexPrec2, outPixel);
                }
                else
                {
                    InterpolateLDR(c[uRegion << 1], c[(uRegion << 1) + 1], w2[i], w1[i], uIndexPrec2, uIndexPrec, outPixel);
                }
            }

            switch (uRotation)
            {
            case 1: std::swap(outPixel.r, outPixel.a); break;
            case 2: std::swap(outPixel.g, outPixel.a); break;
            case 3: std::swap(outPixel.b, outPixel.a); break;
            }

            pOut[i] = outPixel.ToHDRColorA();
        }
    }
    else
    {
#ifndef NDEBUG
        fprintf(stderr, "BC7: Reserved mode 8 encountered during decoding\n");
#endif
        // Per the BC7 format spec, we must return transparent black
        memset(pOut, 0, sizeof(HDRColorA) * NUM_PIXELS_PER_BLOCK);
    }
}

void Block_BC7::Encode(uint32_t flags, const HDRColorA* const pIn)
{
    assert(pIn);

    Block_BC7 final = *this;
    EncodeParams EP(pIn);
    float fMSEBest = FLT_MAX;

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        EP.aLDRPixels[i].r = uint8_t(std::max<float>(0.0f, std::min<float>(255.0f, pIn[i].r * 255.0f + 0.01f)));
        EP.aLDRPixels[i].g = uint8_t(std::max<float>(0.0f, std::min<float>(255.0f, pIn[i].g * 255.0f + 0.01f)));
        EP.aLDRPixels[i].b = uint8_t(std::max<float>(0.0f, std::min<float>(255.0f, pIn[i].b * 255.0f + 0.01f)));
        EP.aLDRPixels[i].a = uint8_t(std::max<float>(0.0f, std::min<float>(255.0f, pIn[i].a * 255.0f + 0.01f)));
    }

    for (EP.uMode = 0; EP.uMode < 8 && fMSEBest > 0; ++EP.uMode)
    {
        if (!(flags & BC_FLAGS_USE_3SUBSETS) && (EP.uMode == 0 || EP.uMode == 2))
        {
            // 3 subset modes tend to be used rarely and add significant compression time
            continue;
        }

        if ((flags & BC_FLAGS_FORCE_BC7_MODE6) && (EP.uMode != 6))
        {
            // Use only mode 6
            continue;
        }

        const size_t uShapes = size_t(1) << ms_aInfo[EP.uMode].uPartitionBits;
        assert(uShapes <= BC7_MAX_SHAPES);

        const size_t uNumRots = size_t(1) << ms_aInfo[EP.uMode].uRotationBits;
        const size_t uNumIdxMode = size_t(1) << ms_aInfo[EP.uMode].uIndexModeBits;
        // Number of rough cases to look at. reasonable values of this are 1, uShapes/4, and uShapes
        // uShapes/4 gets nearly all the cases; you can increase that a bit (say by 3 or 4) if you really want to squeeze the last bit out
        const size_t uItems = std::max<size_t>(1, uShapes >> 2);
        float afRoughMSE[BC7_MAX_SHAPES];
        size_t auShape[BC7_MAX_SHAPES];

        for (size_t r = 0; r < uNumRots && fMSEBest > 0; ++r)
        {
            switch (r)
            {
            case 1: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].r, EP.aLDRPixels[i].a); break;
            case 2: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].g, EP.aLDRPixels[i].a); break;
            case 3: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].b, EP.aLDRPixels[i].a); break;
            }

            for (size_t im = 0; im < uNumIdxMode && fMSEBest > 0; ++im)
            {
                // pick the best uItems shapes and refine these.
                for (size_t s = 0; s < uShapes; s++)
                {
                    afRoughMSE[s] = RoughMSE(&EP, s, im);
                    auShape[s] = s;
                }

                // Bubble up the first uItems items
                for (size_t i = 0; i < uItems; i++)
                {
                    for (size_t j = i + 1; j < uShapes; j++)
                    {
                        if (afRoughMSE[i] > afRoughMSE[j])
                        {
                            std::swap(afRoughMSE[i], afRoughMSE[j]);
                            std::swap(auShape[i], auShape[j]);
                        }
                    }
                }

                for (size_t i = 0; i < uItems && fMSEBest > 0; i++)
                {
                    float fMSE = Refine(&EP, auShape[i], r, im);
                    if (fMSE < fMSEBest)
                    {
                        final = *this;
                        fMSEBest = fMSE;
                    }
                }
            }

            switch (r)
            {
            case 1: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].r, EP.aLDRPixels[i].a); break;
            case 2: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].g, EP.aLDRPixels[i].a); break;
            case 3: for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++) std::swap(EP.aLDRPixels[i].b, EP.aLDRPixels[i].a); break;
            }
        }
    }

    *this = final;
}


//-------------------------------------------------------------------------------------
void Block_BC7::GeneratePaletteQuantized(const EncodeParams* pEP, size_t uIndexMode, const LDREndPntPair& endPts, LDRColorA aPalette[]) const
{
    assert(pEP);
    const size_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    const size_t uNumIndices = size_t(1) << uIndexPrec;
    const size_t uNumIndices2 = size_t(1) << uIndexPrec2;
    assert(uNumIndices > 0 && uNumIndices2 > 0);
    assert((uNumIndices <= BC7_MAX_INDICES) && (uNumIndices2 <= BC7_MAX_INDICES));

    LDRColorA a = Unquantize(endPts.A, ms_aInfo[pEP->uMode].RGBAPrecWithP);
    LDRColorA b = Unquantize(endPts.B, ms_aInfo[pEP->uMode].RGBAPrecWithP);
    if (uIndexPrec2 == 0)
    {
        for (size_t i = 0; i < uNumIndices; i++)
            InterpolateLDR(a, b, i, i, uIndexPrec, uIndexPrec, aPalette[i]);
    }
    else
    {
        for (size_t i = 0; i < uNumIndices; i++)
            InterpolateLDR_RGB(a, b, i, uIndexPrec, aPalette[i]);
        for (size_t i = 0; i < uNumIndices2; i++)
            InterpolateLDR_A(a, b, i, uIndexPrec2, aPalette[i]);
    }
}

float Block_BC7::PerturbOne(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode, size_t ch,
    const LDREndPntPair &oldEndPts, LDREndPntPair &newEndPts, float fOldErr, uint8_t do_b) const
{
    assert(pEP);
    const int prec = ms_aInfo[pEP->uMode].RGBAPrecWithP[ch];
    LDREndPntPair tmp_endPts = newEndPts = oldEndPts;
    float fMinErr = fOldErr;
    uint8_t* pnew_c = (do_b ? &newEndPts.B[ch] : &newEndPts.A[ch]);
    uint8_t* ptmp_c = (do_b ? &tmp_endPts.B[ch] : &tmp_endPts.A[ch]);

    // do a logarithmic search for the best error for this endpoint (which)
    for (int step = 1 << (prec - 1); step; step >>= 1)
    {
        bool bImproved = false;
        int beststep = 0;
        for (int sign = -1; sign <= 1; sign += 2)
        {
            int tmp = int(*pnew_c) + sign * step;
            if (tmp < 0 || tmp >= (1 << prec))
                continue;
            else
                *ptmp_c = (uint8_t)tmp;

            float fTotalErr = MapColors(pEP, aColors, np, uIndexMode, tmp_endPts, fMinErr);
            if (fTotalErr < fMinErr)
            {
                bImproved = true;
                fMinErr = fTotalErr;
                beststep = sign * step;
            }
        }

        // if this was an improvement, move the endpoint and continue search from there
        if (bImproved)
            *pnew_c = uint8_t(int(*pnew_c) + beststep);
    }
    return fMinErr;
}

// perturb the endpoints at least -3 to 3.
// always ensure endpoint ordering is preserved (no need to overlap the scan)
void Block_BC7::Exhaustive(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode, size_t ch,
    float& fOrgErr, LDREndPntPair& optEndPt) const
{
    assert(pEP);
    const uint8_t uPrec = ms_aInfo[pEP->uMode].RGBAPrecWithP[ch];
    LDREndPntPair tmpEndPt;
    if (fOrgErr == 0)
        return;

    int delta = 5;

    // ok figure out the range of A and B
    tmpEndPt = optEndPt;
    int alow = std::max<int>(0, int(optEndPt.A[ch]) - delta);
    int ahigh = std::min<int>((1 << uPrec) - 1, int(optEndPt.A[ch]) + delta);
    int blow = std::max<int>(0, int(optEndPt.B[ch]) - delta);
    int bhigh = std::min<int>((1 << uPrec) - 1, int(optEndPt.B[ch]) + delta);
    int amin = 0;
    int bmin = 0;

    float fBestErr = fOrgErr;
    if (optEndPt.A[ch] <= optEndPt.B[ch])
    {
        // keep a <= b
        for (int a = alow; a <= ahigh; ++a)
        {
            for (int b = std::max<int>(a, blow); b < bhigh; ++b)
            {
                tmpEndPt.A[ch] = (uint8_t)a;
                tmpEndPt.B[ch] = (uint8_t)b;

                float fErr = MapColors(pEP, aColors, np, uIndexMode, tmpEndPt, fBestErr);
                if (fErr < fBestErr)
                {
                    amin = a;
                    bmin = b;
                    fBestErr = fErr;
                }
            }
        }
    }
    else
    {
        // keep b <= a
        for (int b = blow; b < bhigh; ++b)
        {
            for (int a = std::max<int>(b, alow); a <= ahigh; ++a)
            {
                tmpEndPt.A[ch] = (uint8_t)a;
                tmpEndPt.B[ch] = (uint8_t)b;

                float fErr = MapColors(pEP, aColors, np, uIndexMode, tmpEndPt, fBestErr);
                if (fErr < fBestErr)
                {
                    amin = a;
                    bmin = b;
                    fBestErr = fErr;
                }
            }
        }
    }

    if (fBestErr < fOrgErr)
    {
        optEndPt.A[ch] = (uint8_t)amin;
        optEndPt.B[ch] = (uint8_t)bmin;
        fOrgErr = fBestErr;
    }
}

void Block_BC7::OptimizeOne(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode,
    float fOrgErr, const LDREndPntPair& org, LDREndPntPair& opt) const
{
    assert(pEP);

    float fOptErr = fOrgErr;
    opt = org;

    LDREndPntPair new_a, new_b;
    LDREndPntPair newEndPts;
    uint8_t do_b;

    // now optimize each channel separately
    for (size_t ch = 0; ch < BC7_NUM_CHANNELS; ++ch)
    {
        if (ms_aInfo[pEP->uMode].RGBAPrecWithP[ch] == 0)
            continue;

        // figure out which endpoint when perturbed gives the most improvement and start there
        // if we just alternate, we can easily end up in a local minima
        float fErr0 = PerturbOne(pEP, aColors, np, uIndexMode, ch, opt, new_a, fOptErr, 0);	// perturb endpt A
        float fErr1 = PerturbOne(pEP, aColors, np, uIndexMode, ch, opt, new_b, fOptErr, 1);	// perturb endpt B

        uint8_t& copt_a = opt.A[ch];
        uint8_t& copt_b = opt.B[ch];
        uint8_t& cnew_a = new_a.A[ch];
        uint8_t& cnew_b = new_a.B[ch];

        if (fErr0 < fErr1)
        {
            if (fErr0 >= fOptErr)
                continue;
            copt_a = cnew_a;
            fOptErr = fErr0;
            do_b = 1;		// do B next
        }
        else
        {
            if (fErr1 >= fOptErr)
                continue;
            copt_b = cnew_b;
            fOptErr = fErr1;
            do_b = 0;		// do A next
        }

        // now alternate endpoints and keep trying until there is no improvement
        for (; ; )
        {
            float fErr = PerturbOne(pEP, aColors, np, uIndexMode, ch, opt, newEndPts, fOptErr, do_b);
            if (fErr >= fOptErr)
                break;
            if (do_b == 0)
                copt_a = cnew_a;
            else
                copt_b = cnew_b;
            fOptErr = fErr;
            do_b = 1 - do_b;	// now move the other endpoint
        }
    }

    // finally, do a small exhaustive search around what we think is the global minima to be sure
    for (size_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        Exhaustive(pEP, aColors, np, uIndexMode, ch, fOptErr, opt);
}

void Block_BC7::OptimizeEndPoints(const EncodeParams* pEP, size_t uShape, size_t uIndexMode, const float afOrgErr[],
    const LDREndPntPair aOrgEndPts[], LDREndPntPair aOptEndPts[]) const
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS && uShape < BC7_MAX_SHAPES);

    LDRColorA aPixels[NUM_PIXELS_PER_BLOCK];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        // collect the pixels in the region
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
            if (g_aPartitionTable[uPartitions][uShape][i] == p)
                aPixels[np++] = pEP->aLDRPixels[i];

        OptimizeOne(pEP, aPixels, np, uIndexMode, afOrgErr[p], aOrgEndPts[p], aOptEndPts[p]);
    }
}

void Block_BC7::AssignIndices(const EncodeParams* pEP, size_t uShape, size_t uIndexMode, LDREndPntPair endPts[], size_t aIndices[], size_t aIndices2[],
    float afTotErr[]) const
{
    assert(pEP);
    assert(uShape < BC7_MAX_SHAPES);

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);

    const uint8_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    const uint8_t uNumIndices2 = 1 << uIndexPrec2;

    assert((uNumIndices <= BC7_MAX_INDICES) && (uNumIndices2 <= BC7_MAX_INDICES));

    const uint8_t uHighestIndexBit = uNumIndices >> 1;
    const uint8_t uHighestIndexBit2 = uNumIndices2 >> 1;
    LDRColorA aPalette[BC7_MAX_REGIONS][BC7_MAX_INDICES];

    // build list of possibles
    for (size_t p = 0; p <= uPartitions; p++)
    {
        GeneratePaletteQuantized(pEP, uIndexMode, endPts[p], aPalette[p]);
        afTotErr[p] = 0;
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
    {
        uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][i];
        assert(uRegion < BC7_MAX_REGIONS);
                afTotErr[uRegion] += ComputeError(pEP->aLDRPixels[i], aPalette[uRegion], uIndexPrec, uIndexPrec2, &(aIndices[i]), &(aIndices2[i]));
    }

    // swap endpoints as needed to ensure that the indices at index_positions have a 0 high-order bit
    if (uIndexPrec2 == 0)
    {
        for (size_t p = 0; p <= uPartitions; p++)
        {
            if (aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit)
            {
                std::swap(endPts[p].A, endPts[p].B);
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
                    if (g_aPartitionTable[uPartitions][uShape][i] == p)
                        aIndices[i] = uNumIndices - 1 - aIndices[i];
            }
            assert((aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit) == 0);
        }
    }
    else
    {
        for (size_t p = 0; p <= uPartitions; p++)
        {
            if (aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit)
            {
                std::swap(endPts[p].A.r, endPts[p].B.r);
                std::swap(endPts[p].A.g, endPts[p].B.g);
                std::swap(endPts[p].A.b, endPts[p].B.b);
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
                    if (g_aPartitionTable[uPartitions][uShape][i] == p)
                        aIndices[i] = uNumIndices - 1 - aIndices[i];
            }
            assert((aIndices[g_aFixUp[uPartitions][uShape][p]] & uHighestIndexBit) == 0);

            if (aIndices2[0] & uHighestIndexBit2)
            {
                std::swap(endPts[p].A.a, endPts[p].B.a);
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
                    aIndices2[i] = uNumIndices2 - 1 - aIndices2[i];
            }
            assert((aIndices2[0] & uHighestIndexBit2) == 0);
        }
    }
}

void Block_BC7::EmitBlock(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode, const LDREndPntPair aEndPts[], const size_t aIndex[], const size_t aIndex2[])
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);

    const size_t uPBits = ms_aInfo[pEP->uMode].uPBits;
    const size_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uIndexPrec2 = ms_aInfo[pEP->uMode].uIndexPrec2;
    const LDRColorA RGBAPrec = ms_aInfo[pEP->uMode].RGBAPrec;
    const LDRColorA RGBAPrecWithP = ms_aInfo[pEP->uMode].RGBAPrecWithP;
    size_t i;
    size_t uStartBit = 0;
    SetBits(uStartBit, pEP->uMode, 0);
    SetBits(uStartBit, 1, 1);
    SetBits(uStartBit, ms_aInfo[pEP->uMode].uRotationBits, static_cast<uint8_t>(uRotation));
    SetBits(uStartBit, ms_aInfo[pEP->uMode].uIndexModeBits, static_cast<uint8_t>(uIndexMode));
    SetBits(uStartBit, ms_aInfo[pEP->uMode].uPartitionBits, static_cast<uint8_t>(uShape));

    if (uPBits)
    {
        const size_t uNumEP = size_t(1 + uPartitions) << 1;
        uint8_t aPVote[BC7_MAX_REGIONS << 1] = { 0,0,0,0,0,0 };
        uint8_t aCount[BC7_MAX_REGIONS << 1] = { 0,0,0,0,0,0 };
        for (uint8_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        {
            uint8_t ep = 0;
            for (i = 0; i <= uPartitions; i++)
            {
                if (RGBAPrec[ch] == RGBAPrecWithP[ch])
                {
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].A[ch]);
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].B[ch]);
                }
                else
                {
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].A[ch] >> 1);
                    SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].B[ch] >> 1);
                    size_t idx = ep++ * uPBits / uNumEP;
                    assert(idx < (BC7_MAX_REGIONS << 1));
                                        aPVote[idx] += aEndPts[i].A[ch] & 0x01;
                    aCount[idx]++;
                    idx = ep++ * uPBits / uNumEP;
                    assert(idx < (BC7_MAX_REGIONS << 1));
                                        aPVote[idx] += aEndPts[i].B[ch] & 0x01;
                    aCount[idx]++;
                }
            }
        }

        for (i = 0; i < uPBits; i++)
        {
            SetBits(uStartBit, 1, aPVote[i] > (aCount[i] >> 1) ? 1 : 0);
        }
    }
    else
    {
        for (size_t ch = 0; ch < BC7_NUM_CHANNELS; ch++)
        {
            for (i = 0; i <= uPartitions; i++)
            {
                SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].A[ch]);
                SetBits(uStartBit, RGBAPrec[ch], aEndPts[i].B[ch]);
            }
        }
    }

    const size_t* aI1 = uIndexMode ? aIndex2 : aIndex;
    const size_t* aI2 = uIndexMode ? aIndex : aIndex2;
    for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
    {
        if (IsFixUpOffset(ms_aInfo[pEP->uMode].uPartitions, uShape, i))
            SetBits(uStartBit, uIndexPrec - 1, static_cast<uint8_t>(aI1[i]));
        else
            SetBits(uStartBit, uIndexPrec, static_cast<uint8_t>(aI1[i]));
    }
    if (uIndexPrec2)
        for (i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
            SetBits(uStartBit, i ? uIndexPrec2 : uIndexPrec2 - 1, static_cast<uint8_t>(aI2[i]));

    assert(uStartBit == 128);
}

float Block_BC7::Refine(const EncodeParams* pEP, size_t uShape, size_t uRotation, size_t uIndexMode)
{
    assert( pEP );
    assert( uShape < BC7_MAX_SHAPES );
        const LDREndPntPair* aEndPts = pEP->aEndPts[uShape];

    const size_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert( uPartitions < BC7_MAX_REGIONS );

    LDREndPntPair aOrgEndPts[BC7_MAX_REGIONS];
    LDREndPntPair aOptEndPts[BC7_MAX_REGIONS];
    size_t aOrgIdx[NUM_PIXELS_PER_BLOCK];
    size_t aOrgIdx2[NUM_PIXELS_PER_BLOCK];
    size_t aOptIdx[NUM_PIXELS_PER_BLOCK];
    size_t aOptIdx2[NUM_PIXELS_PER_BLOCK];
    float aOrgErr[BC7_MAX_REGIONS];
    float aOptErr[BC7_MAX_REGIONS];

    for(size_t p = 0; p <= uPartitions; p++)
    {
        aOrgEndPts[p].A = Quantize(aEndPts[p].A, ms_aInfo[pEP->uMode].RGBAPrecWithP);
        aOrgEndPts[p].B = Quantize(aEndPts[p].B, ms_aInfo[pEP->uMode].RGBAPrecWithP);
    }

    AssignIndices(pEP, uShape, uIndexMode, aOrgEndPts, aOrgIdx, aOrgIdx2, aOrgErr);
    OptimizeEndPoints(pEP, uShape, uIndexMode, aOrgErr, aOrgEndPts, aOptEndPts);
    AssignIndices(pEP, uShape, uIndexMode, aOptEndPts, aOptIdx, aOptIdx2, aOptErr);

    float fOrgTotErr = 0, fOptTotErr = 0;
    for(size_t p = 0; p <= uPartitions; p++)
    {
        fOrgTotErr += aOrgErr[p];
        fOptTotErr += aOptErr[p];
    }
    if(fOptTotErr < fOrgTotErr)
    {
        EmitBlock(pEP, uShape, uRotation, uIndexMode, aOptEndPts, aOptIdx, aOptIdx2);
        return fOptTotErr;
    }
    else
    {
        EmitBlock(pEP, uShape, uRotation, uIndexMode, aOrgEndPts, aOrgIdx, aOrgIdx2);
        return fOrgTotErr;
    }
}

float Block_BC7::MapColors(const EncodeParams* pEP, const LDRColorA aColors[], size_t np, size_t uIndexMode, const LDREndPntPair& endPts, float fMinErr) const
{
    assert(pEP);
    const uint8_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    LDRColorA aPalette[BC7_MAX_INDICES];
    float fTotalErr = 0;

    GeneratePaletteQuantized(pEP, uIndexMode, endPts, aPalette);
    for (size_t i = 0; i < np; ++i)
    {
        fTotalErr += ComputeError(aColors[i], aPalette, uIndexPrec, uIndexPrec2);
        if (fTotalErr > fMinErr)   // check for early exit
        {
            fTotalErr = FLT_MAX;
            break;
        }
    }

    return fTotalErr;
}

float Block_BC7::RoughMSE(EncodeParams* pEP, size_t uShape, size_t uIndexMode)
{
    assert(pEP);
    assert(uShape < BC7_MAX_SHAPES);
        LDREndPntPair* aEndPts = pEP->aEndPts[uShape];

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC7_MAX_REGIONS);

    const uint8_t uIndexPrec = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec2 : ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uIndexPrec2 = uIndexMode ? ms_aInfo[pEP->uMode].uIndexPrec : ms_aInfo[pEP->uMode].uIndexPrec2;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    const uint8_t uNumIndices2 = 1 << uIndexPrec2;
    size_t auPixIdx[NUM_PIXELS_PER_BLOCK];
    LDRColorA aPalette[BC7_MAX_REGIONS][BC7_MAX_INDICES];

    for (size_t p = 0; p <= uPartitions; p++)
    {
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
        {
            if (g_aPartitionTable[uPartitions][uShape][i] == p)
            {
                auPixIdx[np++] = i;
            }
        }

        // handle simple cases
        assert(np > 0);
        if (np == 1)
        {
            aEndPts[p].A = pEP->aLDRPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aLDRPixels[auPixIdx[0]];
            continue;
        }
        else if (np == 2)
        {
            aEndPts[p].A = pEP->aLDRPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aLDRPixels[auPixIdx[1]];
            continue;
        }

        if (uIndexPrec2 == 0)
        {
            HDRColorA epA, epB;
            OptimizeRGBA(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
            epA.Clamp(0.0f, 1.0f);
            epB.Clamp(0.0f, 1.0f);
            aEndPts[p].A = LDRColorA::FromHDRColorA(epA);
            aEndPts[p].B = LDRColorA::FromHDRColorA(epB);
        }
        else
        {
            uint8_t uMinAlpha = 255, uMaxAlpha = 0;
            for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
            {
                uMinAlpha = std::min<uint8_t>(uMinAlpha, pEP->aLDRPixels[auPixIdx[i]].a);
                uMaxAlpha = std::max<uint8_t>(uMaxAlpha, pEP->aLDRPixels[auPixIdx[i]].a);
            }

            HDRColorA epA, epB;
            OptimizeRGB(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
            epA.Clamp(0.0f, 1.0f);
            epB.Clamp(0.0f, 1.0f);
            aEndPts[p].A = LDRColorA::FromHDRColorA(epA);
            aEndPts[p].B = LDRColorA::FromHDRColorA(epB);
            aEndPts[p].A.a = uMinAlpha;
            aEndPts[p].B.a = uMaxAlpha;
        }
    }

    if (uIndexPrec2 == 0)
    {
        for (size_t p = 0; p <= uPartitions; p++)
            for (size_t i = 0; i < uNumIndices; i++)
                InterpolateLDR(aEndPts[p].A, aEndPts[p].B, i, i, uIndexPrec, uIndexPrec, aPalette[p][i]);
    }
    else
    {
        for (size_t p = 0; p <= uPartitions; p++)
        {
            for (size_t i = 0; i < uNumIndices; i++)
                InterpolateLDR_RGB(aEndPts[p].A, aEndPts[p].B, i, uIndexPrec, aPalette[p][i]);
            for (size_t i = 0; i < uNumIndices2; i++)
                InterpolateLDR_A(aEndPts[p].A, aEndPts[p].B, i, uIndexPrec2, aPalette[p][i]);
        }
    }

    float fTotalErr = 0;
    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; i++)
    {
        uint8_t uRegion = g_aPartitionTable[uPartitions][uShape][i];
        fTotalErr += ComputeError(pEP->aLDRPixels[i], aPalette[uRegion], uIndexPrec, uIndexPrec2);
    }

    return fTotalErr;
}



void DecodeBC7(HDRColorA *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(Block_BC7) == 16, "Block_BC7 should be 16 bytes");
    reinterpret_cast<const Block_BC7*>(pBC)->Decode(pColor);
}

void EncodeBC7(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    assert(pBC && pColor);
    static_assert(sizeof(Block_BC7) == 16, "Block_BC7 should be 16 bytes");
    reinterpret_cast<Block_BC7*>(pBC)->Encode(flags, pColor);
}

}
