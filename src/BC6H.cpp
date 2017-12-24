#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <float.h>
#include <stdio.h>

#include "BC.hpp"
#include "BC67_shared.hpp"
#include "Colors.hpp"

#define ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))


namespace Tex {

const size_t BC6H_MAX_REGIONS = 2;
const size_t BC6H_MAX_INDICES = 16;
const size_t BC6H_NUM_CHANNELS = 3;
const size_t BC6H_MAX_SHAPES = 32;


// BC6H compression (16 bits per texel)
class Block_BC6H : private CBits<16>
{
public:
    void Decode(bool bSigned, HDRColorA* pOut) const;
    void Encode(bool bSigned, const HDRColorA* const pIn);

private:
    enum EField : uint8_t
    {
        NA, // N/A
        M,  // Mode
        D,  // Shape
        RW,
        RX,
        RY,
        RZ,
        GW,
        GX,
        GY,
        GZ,
        BW,
        BX,
        BY,
        BZ,
    };

    struct ModeDescriptor
    {
        EField m_eField;
        uint8_t   m_uBit;
    };

    struct ModeInfo
    {
        uint8_t uMode;
        uint8_t uPartitions;
        bool bTransformed;
        uint8_t uIndexPrec;
        LDRColorA RGBAPrec[BC6H_MAX_REGIONS][2];
    };

    struct EncodeParams
    {
        float fBestErr;
        const bool bSigned;
        uint8_t uMode;
        uint8_t uShape;
        const HDRColorA* const aHDRPixels;
        INTEndPntPair aUnqEndPts[BC6H_MAX_SHAPES][BC6H_MAX_REGIONS];
        INTColor aIPixels[NUM_PIXELS_PER_BLOCK];

        EncodeParams(const HDRColorA* const aOriginal, bool bSignedFormat) :
            fBestErr(FLT_MAX), bSigned(bSignedFormat), aHDRPixels(aOriginal)
        {
            for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
            {
                aIPixels[i].FromHDRColorA(aOriginal[i], bSigned);
            }
        }
    };

    static int Quantize(int iValue, int prec, bool bSigned);
    static int Unquantize(int comp, uint8_t uBitsPerComp, bool bSigned);
    static int FinishUnquantize(int comp, bool bSigned);

    static bool EndPointsFit(const EncodeParams* pEP, const INTEndPntPair aEndPts[]);

    void GeneratePaletteQuantized(const EncodeParams* pEP, const INTEndPntPair& endPts,
        INTColor aPalette[]) const;
    float MapColorsQuantized(const EncodeParams* pEP, const INTColor aColors[], size_t np, const INTEndPntPair &endPts) const;
    float PerturbOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, uint8_t ch,
        const INTEndPntPair& oldEndPts, INTEndPntPair& newEndPts, float fOldErr, int do_b) const;
    void OptimizeOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, float aOrgErr,
        const INTEndPntPair &aOrgEndPts, INTEndPntPair &aOptEndPts) const;
    void OptimizeEndPoints(const EncodeParams* pEP, const float aOrgErr[],
        const INTEndPntPair aOrgEndPts[],
        INTEndPntPair aOptEndPts[]) const;
    static void SwapIndices(const EncodeParams* pEP, INTEndPntPair aEndPts[],
        size_t aIndices[]);
    void AssignIndices(const EncodeParams* pEP, const INTEndPntPair aEndPts[],
        size_t aIndices[],
        float aTotErr[]) const;
    void QuantizeEndPts(const EncodeParams* pEP, INTEndPntPair* qQntEndPts) const;
    void EmitBlock(const EncodeParams* pEP, const INTEndPntPair aEndPts[],
        const size_t aIndices[]);
    void Refine(EncodeParams* pEP);

    static void GeneratePaletteUnquantized(const EncodeParams* pEP, size_t uRegion, INTColor aPalette[]);
    float MapColors(const EncodeParams* pEP, size_t uRegion, size_t np, const size_t* auIndex) const;
    float RoughMSE(EncodeParams* pEP) const;

private:
    const static ModeDescriptor ms_aDesc[][82];
    const static ModeInfo ms_aInfo[];
    const static int ms_aModeToInfo[];
};

// BC6H Compression
const Block_BC6H::ModeDescriptor Block_BC6H::ms_aDesc[14][82] =
{
    {   // Mode 1 (0x00) - 10 5 5 5
        { M, 0}, { M, 1}, {GY, 4}, {BY, 4}, {BZ, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 2 (0x01) - 7 6 6 6
        { M, 0}, { M, 1}, {GY, 5}, {GZ, 4}, {GZ, 5}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {BZ, 0}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {BY, 5}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BZ, 3}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 3 (0x02) - 11 5 4 4
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RW,10}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,10},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,10},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 4 (0x06) - 11 4 5 4
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,10},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GW,10}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,10},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {BZ, 0},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {GY, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 5 (0x0a) - 11 4 4 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,10},
        {BY, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,10},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BW,10}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {BZ, 1},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {BZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 6 (0x0e) - 9 5 5 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 7 (0x12) - 8 6 5 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {GZ, 4}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BZ, 3}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 8 (0x16) - 8 5 6 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {BZ, 0}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GY, 5}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {GZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 9 (0x1a) - 8 5 5 6
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {BY, 5}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 10 (0x1e) - 6 6 6 6
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {GZ, 4}, {BZ, 0}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GY, 5}, {BY, 5}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {GZ, 5}, {BZ, 3}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4},
    },

    {   // Mode 11 (0x03) - 10 10
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {RX, 6}, {RX, 7}, {RX, 8}, {RX, 9}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GX, 6}, {GX, 7}, {GX, 8}, {GX, 9}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BX, 6}, {BX, 7}, {BX, 8}, {BX, 9}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0},
    },

    {   // Mode 12 (0x07) - 11 9
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {RX, 6}, {RX, 7}, {RX, 8}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GX, 6}, {GX, 7}, {GX, 8}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BX, 6}, {BX, 7}, {BX, 8}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0},
    },

    {   // Mode 13 (0x0b) - 12 8
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {RX, 6}, {RX, 7}, {RW,11}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GX, 6}, {GX, 7}, {GW,11}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BX, 6}, {BX, 7}, {BW,11}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0},
    },

    {   // Mode 14 (0x0f) - 16 4
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,15},
        {RW,14}, {RW,13}, {RW,12}, {RW,11}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,15},
        {GW,14}, {GW,13}, {GW,12}, {GW,11}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,15},
        {BW,14}, {BW,13}, {BW,12}, {BW,11}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0},
    },
};

// Mode, Partitions, Transformed, IndexPrec, RGBAPrec
const Block_BC6H::ModeInfo Block_BC6H::ms_aInfo[] =
{
    {0x00, 1, true,  3, { { LDRColorA(10,10,10,0), LDRColorA( 5, 5, 5,0) }, { LDRColorA(5,5,5,0), LDRColorA(5,5,5,0) } } }, // Mode 1
    {0x01, 1, true,  3, { { LDRColorA( 7, 7, 7,0), LDRColorA( 6, 6, 6,0) }, { LDRColorA(6,6,6,0), LDRColorA(6,6,6,0) } } }, // Mode 2
    {0x02, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA( 5, 4, 4,0) }, { LDRColorA(5,4,4,0), LDRColorA(5,4,4,0) } } }, // Mode 3
    {0x06, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA( 4, 5, 4,0) }, { LDRColorA(4,5,4,0), LDRColorA(4,5,4,0) } } }, // Mode 4
    {0x0a, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA( 4, 4, 5,0) }, { LDRColorA(4,4,5,0), LDRColorA(4,4,5,0) } } }, // Mode 5
    {0x0e, 1, true,  3, { { LDRColorA( 9, 9, 9,0), LDRColorA( 5, 5, 5,0) }, { LDRColorA(5,5,5,0), LDRColorA(5,5,5,0) } } }, // Mode 6
    {0x12, 1, true,  3, { { LDRColorA( 8, 8, 8,0), LDRColorA( 6, 5, 5,0) }, { LDRColorA(6,5,5,0), LDRColorA(6,5,5,0) } } }, // Mode 7
    {0x16, 1, true,  3, { { LDRColorA( 8, 8, 8,0), LDRColorA( 5, 6, 5,0) }, { LDRColorA(5,6,5,0), LDRColorA(5,6,5,0) } } }, // Mode 8
    {0x1a, 1, true,  3, { { LDRColorA( 8, 8, 8,0), LDRColorA( 5, 5, 6,0) }, { LDRColorA(5,5,6,0), LDRColorA(5,5,6,0) } } }, // Mode 9
    {0x1e, 1, false, 3, { { LDRColorA( 6, 6, 6,0), LDRColorA( 6, 6, 6,0) }, { LDRColorA(6,6,6,0), LDRColorA(6,6,6,0) } } }, // Mode 10
    {0x03, 0, false, 4, { { LDRColorA(10,10,10,0), LDRColorA(10,10,10,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 11
    {0x07, 0, true,  4, { { LDRColorA(11,11,11,0), LDRColorA( 9, 9, 9,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 12
    {0x0b, 0, true,  4, { { LDRColorA(12,12,12,0), LDRColorA( 8, 8, 8,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 13
    {0x0f, 0, true,  4, { { LDRColorA(16,16,16,0), LDRColorA( 4, 4, 4,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 14
};

const int Block_BC6H::ms_aModeToInfo[] =
{
     0, // Mode 1   - 0x00
     1, // Mode 2   - 0x01
     2, // Mode 3   - 0x02
    10, // Mode 11  - 0x03
    -1, // Invalid  - 0x04
    -1, // Invalid  - 0x05
     3, // Mode 4   - 0x06
    11, // Mode 12  - 0x07
    -1, // Invalid  - 0x08
    -1, // Invalid  - 0x09
     4, // Mode 5   - 0x0a
    12, // Mode 13  - 0x0b
    -1, // Invalid  - 0x0c
    -1, // Invalid  - 0x0d
     5, // Mode 6   - 0x0e
    13, // Mode 14  - 0x0f
    -1, // Invalid  - 0x10
    -1, // Invalid  - 0x11
     6, // Mode 7   - 0x12
    -1, // Reserved - 0x13
    -1, // Invalid  - 0x14
    -1, // Invalid  - 0x15
     7, // Mode 8   - 0x16
    -1, // Reserved - 0x17
    -1, // Invalid  - 0x18
    -1, // Invalid  - 0x19
     8, // Mode 9   - 0x1a
    -1, // Reserved - 0x1b
    -1, // Invalid  - 0x1c
    -1, // Invalid  - 0x1d
     9, // Mode 10  - 0x1e
    -1, // Resreved - 0x1f
};

//-------------------------------------------------------------------------------------
// BC6H Compression
//-------------------------------------------------------------------------------------
void Block_BC6H::Decode(bool bSigned, HDRColorA* pOut) const
{
    assert(pOut);

    size_t uStartBit = 0;
    uint8_t uMode = GetBits(uStartBit, 2);
    if (uMode != 0x00 && uMode != 0x01)
    {
        uMode = (GetBits(uStartBit, 3) << 2) | uMode;
    }

    assert(uMode < 32);

    if (ms_aModeToInfo[uMode] >= 0)
    {
        assert(ms_aModeToInfo[uMode] < (int)ARRAYSIZE(ms_aInfo));
                const ModeDescriptor* desc = ms_aDesc[ms_aModeToInfo[uMode]];

        assert(ms_aModeToInfo[uMode] < (int)ARRAYSIZE(ms_aDesc));
                const ModeInfo& info = ms_aInfo[ms_aModeToInfo[uMode]];

        INTEndPntPair aEndPts[BC6H_MAX_REGIONS];
        memset(aEndPts, 0, BC6H_MAX_REGIONS * 2 * sizeof(INTColor));
        uint32_t uShape = 0;

        // Read header
        const size_t uHeaderBits = info.uPartitions > 0 ? 82 : 65;
        while (uStartBit < uHeaderBits)
        {
            size_t uCurBit = uStartBit;
            if (GetBit(uStartBit))
            {
                switch (desc[uCurBit].m_eField)
                {
                case D:  uShape |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RW: aEndPts[0].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RX: aEndPts[0].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RY: aEndPts[1].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RZ: aEndPts[1].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GW: aEndPts[0].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GX: aEndPts[0].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GY: aEndPts[1].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GZ: aEndPts[1].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BW: aEndPts[0].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BX: aEndPts[0].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BY: aEndPts[1].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BZ: aEndPts[1].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                default:
                {
#ifndef NDEBUG
                    fprintf(stderr, "BC6H: Invalid header bits encountered during decoding\n");
#endif
                    FillWithErrorColors(pOut);
                    return;
                }
                }
            }
        }

        assert(uShape < 64);

        // Sign extend necessary end points
        if (bSigned)
        {
            aEndPts[0].A.SignExtend(info.RGBAPrec[0][0]);
        }
        if (bSigned || info.bTransformed)
        {
            assert(info.uPartitions < BC6H_MAX_REGIONS);
                        for (size_t p = 0; p <= info.uPartitions; ++p)
            {
                if (p != 0)
                {
                    aEndPts[p].A.SignExtend(info.RGBAPrec[p][0]);
                }
                aEndPts[p].B.SignExtend(info.RGBAPrec[p][1]);
            }
        }

        // Inverse transform the end points
        if (info.bTransformed)
        {
            TransformInverse(aEndPts, info.RGBAPrec[0][0], bSigned);
        }

        // Read indices
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            size_t uNumBits = IsFixUpOffset(info.uPartitions, uShape, i) ? info.uIndexPrec - 1 : info.uIndexPrec;
            if (uStartBit + uNumBits > 128)
            {
#ifndef NDEBUG
                fprintf(stderr, "BC6H: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }
            uint8_t uIndex = GetBits(uStartBit, uNumBits);

            if (uIndex >= ((info.uPartitions > 0) ? 8 : 16))
            {
#ifndef NDEBUG
                fprintf(stderr, "BC6H: Invalid index encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            size_t uRegion = g_aPartitionTable[info.uPartitions][uShape][i];
            assert(uRegion < BC6H_MAX_REGIONS);

            // Unquantize endpoints and interpolate
            int r1 = Unquantize(aEndPts[uRegion].A.r, info.RGBAPrec[0][0].r, bSigned);
            int g1 = Unquantize(aEndPts[uRegion].A.g, info.RGBAPrec[0][0].g, bSigned);
            int b1 = Unquantize(aEndPts[uRegion].A.b, info.RGBAPrec[0][0].b, bSigned);
            int r2 = Unquantize(aEndPts[uRegion].B.r, info.RGBAPrec[0][0].r, bSigned);
            int g2 = Unquantize(aEndPts[uRegion].B.g, info.RGBAPrec[0][0].g, bSigned);
            int b2 = Unquantize(aEndPts[uRegion].B.b, info.RGBAPrec[0][0].b, bSigned);
            const int* aWeights = info.uPartitions > 0 ? g_aWeights3 : g_aWeights4;
            INTColor fc;
            fc.r = FinishUnquantize((r1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + r2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);
            fc.g = FinishUnquantize((g1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + g2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);
            fc.b = FinishUnquantize((b1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + b2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);

            pOut[i] = fc.ToHDRColorA(bSigned);
        }
    }
    else
    {
#ifndef NDEBUG
        const char* warnstr = "BC6H: Invalid mode encountered during decoding\n";
        switch (uMode)
        {
        case 0x13:  warnstr = "BC6H: Reserved mode 10011 encountered during decoding\n"; break;
        case 0x17:  warnstr = "BC6H: Reserved mode 10111 encountered during decoding\n"; break;
        case 0x1B:  warnstr = "BC6H: Reserved mode 11011 encountered during decoding\n"; break;
        case 0x1F:  warnstr = "BC6H: Reserved mode 11111 encountered during decoding\n"; break;
        }
        fprintf(stderr, warnstr);
#endif
        // Per the BC6H format spec, we must return opaque black
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            pOut[i] = HDRColorA(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}


void Block_BC6H::Encode(bool bSigned, const HDRColorA* const pIn)
{
    assert(pIn);

    EncodeParams EP(pIn, bSigned);

    for (EP.uMode = 0; EP.uMode < ARRAYSIZE(ms_aInfo) && EP.fBestErr > 0; ++EP.uMode)
    {
        const uint8_t uShapes = ms_aInfo[EP.uMode].uPartitions ? 32 : 1;
        // Number of rough cases to look at. reasonable values of this are 1, uShapes/4, and uShapes
        // uShapes/4 gets nearly all the cases; you can increase that a bit (say by 3 or 4) if you really want to squeeze the last bit out
        const size_t uItems = std::max<size_t>(1, uShapes >> 2);
        float afRoughMSE[BC6H_MAX_SHAPES];
        uint8_t auShape[BC6H_MAX_SHAPES];

        // pick the best uItems shapes and refine these.
        for (EP.uShape = 0; EP.uShape < uShapes; ++EP.uShape)
        {
            size_t uShape = EP.uShape;
            afRoughMSE[uShape] = RoughMSE(&EP);
            auShape[uShape] = static_cast<uint8_t>(uShape);
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

        for (size_t i = 0; i < uItems && EP.fBestErr > 0; i++)
        {
            EP.uShape = auShape[i];
            Refine(&EP);
        }
    }
}


//-------------------------------------------------------------------------------------
int Block_BC6H::Quantize(int iValue, int prec, bool bSigned)
{
    assert(prec > 1);	// didn't bother to make it work for 1
    int q, s = 0;
    if (bSigned)
    {
        assert(iValue >= -F16MAX && iValue <= F16MAX);
        if (iValue < 0)
        {
            s = 1;
            iValue = -iValue;
        }
        q = (prec >= 16) ? iValue : (iValue << (prec - 1)) / (F16MAX + 1);
        if (s)
            q = -q;
        assert(q > -(1 << (prec - 1)) && q < (1 << (prec - 1)));
    }
    else
    {
        assert(iValue >= 0 && iValue <= F16MAX);
        q = (prec >= 15) ? iValue : (iValue << prec) / (F16MAX + 1);
        assert(q >= 0 && q < (1 << prec));
    }

    return q;
}


int Block_BC6H::Unquantize(int comp, uint8_t uBitsPerComp, bool bSigned)
{
    int unq = 0, s = 0;
    if (bSigned)
    {
        if (uBitsPerComp >= 16)
        {
            unq = comp;
        }
        else
        {
            if (comp < 0)
            {
                s = 1;
                comp = -comp;
            }

            if (comp == 0) unq = 0;
            else if (comp >= ((1 << (uBitsPerComp - 1)) - 1)) unq = 0x7FFF;
            else unq = ((comp << 15) + 0x4000) >> (uBitsPerComp - 1);

            if (s) unq = -unq;
        }
    }
    else
    {
        if (uBitsPerComp >= 15) unq = comp;
        else if (comp == 0) unq = 0;
        else if (comp == ((1 << uBitsPerComp) - 1)) unq = 0xFFFF;
        else unq = ((comp << 16) + 0x8000) >> uBitsPerComp;
    }

    return unq;
}


int Block_BC6H::FinishUnquantize(int comp, bool bSigned)
{
    if (bSigned)
    {
        return (comp < 0) ? -(((-comp) * 31) >> 5) : (comp * 31) >> 5;  // scale the magnitude by 31/32
    }
    else
    {
        return (comp * 31) >> 6;                                        // scale the magnitude by 31/64
    }
}


//-------------------------------------------------------------------------------------
bool Block_BC6H::EndPointsFit(const EncodeParams* pEP, const INTEndPntPair aEndPts[])
{
    assert(pEP);
    const bool bTransformed = ms_aInfo[pEP->uMode].bTransformed;
    const bool bIsSigned = pEP->bSigned;
    const LDRColorA& Prec0 = ms_aInfo[pEP->uMode].RGBAPrec[0][0];
    const LDRColorA& Prec1 = ms_aInfo[pEP->uMode].RGBAPrec[0][1];
    const LDRColorA& Prec2 = ms_aInfo[pEP->uMode].RGBAPrec[1][0];
    const LDRColorA& Prec3 = ms_aInfo[pEP->uMode].RGBAPrec[1][1];

    INTColor aBits[4];
    aBits[0].r = NBits(aEndPts[0].A.r, bIsSigned);
    aBits[0].g = NBits(aEndPts[0].A.g, bIsSigned);
    aBits[0].b = NBits(aEndPts[0].A.b, bIsSigned);
    aBits[1].r = NBits(aEndPts[0].B.r, bTransformed || bIsSigned);
    aBits[1].g = NBits(aEndPts[0].B.g, bTransformed || bIsSigned);
    aBits[1].b = NBits(aEndPts[0].B.b, bTransformed || bIsSigned);
    if (aBits[0].r > Prec0.r || aBits[1].r > Prec1.r ||
        aBits[0].g > Prec0.g || aBits[1].g > Prec1.g ||
        aBits[0].b > Prec0.b || aBits[1].b > Prec1.b)
        return false;

    if (ms_aInfo[pEP->uMode].uPartitions)
    {
        aBits[2].r = NBits(aEndPts[1].A.r, bTransformed || bIsSigned);
        aBits[2].g = NBits(aEndPts[1].A.g, bTransformed || bIsSigned);
        aBits[2].b = NBits(aEndPts[1].A.b, bTransformed || bIsSigned);
        aBits[3].r = NBits(aEndPts[1].B.r, bTransformed || bIsSigned);
        aBits[3].g = NBits(aEndPts[1].B.g, bTransformed || bIsSigned);
        aBits[3].b = NBits(aEndPts[1].B.b, bTransformed || bIsSigned);

        if (aBits[2].r > Prec2.r || aBits[3].r > Prec3.r ||
            aBits[2].g > Prec2.g || aBits[3].g > Prec3.g ||
            aBits[2].b > Prec2.b || aBits[3].b > Prec3.b)
            return false;
    }

    return true;
}


void Block_BC6H::GeneratePaletteQuantized(const EncodeParams* pEP, const INTEndPntPair& endPts, INTColor aPalette[]) const
{
    assert(pEP);
    const size_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uNumIndices = size_t(1) << uIndexPrec;
    assert(uNumIndices > 0);
        const LDRColorA& Prec = ms_aInfo[pEP->uMode].RGBAPrec[0][0];

    // scale endpoints
    INTEndPntPair unqEndPts;
    unqEndPts.A.r = Unquantize(endPts.A.r, Prec.r, pEP->bSigned);
    unqEndPts.A.g = Unquantize(endPts.A.g, Prec.g, pEP->bSigned);
    unqEndPts.A.b = Unquantize(endPts.A.b, Prec.b, pEP->bSigned);
    unqEndPts.B.r = Unquantize(endPts.B.r, Prec.r, pEP->bSigned);
    unqEndPts.B.g = Unquantize(endPts.B.g, Prec.g, pEP->bSigned);
    unqEndPts.B.b = Unquantize(endPts.B.b, Prec.b, pEP->bSigned);

    // interpolate
    const int* aWeights = nullptr;
    switch (uIndexPrec)
    {
    case 3: aWeights = g_aWeights3; assert(uNumIndices <= 8);     case 4: aWeights = g_aWeights4; assert(uNumIndices <= 16);     default:
        assert(false);
        for (size_t i = 0; i < uNumIndices; ++i)
        {
            aPalette[i] = INTColor(0, 0, 0);
        }
        return;
    }

    for (size_t i = 0; i < uNumIndices; ++i)
    {
        aPalette[i].r = FinishUnquantize(
            (unqEndPts.A.r * (BC67_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.r * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT,
            pEP->bSigned);
        aPalette[i].g = FinishUnquantize(
            (unqEndPts.A.g * (BC67_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.g * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT,
            pEP->bSigned);
        aPalette[i].b = FinishUnquantize(
            (unqEndPts.A.b * (BC67_WEIGHT_MAX - aWeights[i]) + unqEndPts.B.b * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT,
            pEP->bSigned);
    }
}


// given a collection of colors and quantized endpoints, generate a palette, choose best entries, and return a single toterr
float Block_BC6H::MapColorsQuantized(const EncodeParams* pEP, const INTColor aColors[], size_t np, const INTEndPntPair &endPts) const
{
    assert(pEP);

    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    INTColor aPalette[BC6H_MAX_INDICES];
    GeneratePaletteQuantized(pEP, endPts, aPalette);

    float fTotErr = 0;
    for (size_t i = 0; i < np; ++i)
    {
        INTColor vcolors = aColors[i];

        // Compute ErrorMetricRGB
        INTColor tpal = aPalette[0];
        tpal = vcolors - tpal;
        float fBestErr = INTColor::DotProduct(tpal, tpal);

        for (int j = 1; j < uNumIndices && fBestErr > 0; ++j)
        {
            // Compute ErrorMetricRGB
            tpal = aPalette[j];
            tpal = vcolors - tpal;
            float fErr = INTColor::DotProduct(tpal, tpal);
            if (fErr > fBestErr) break;     // error increased, so we're done searching
            if (fErr < fBestErr) fBestErr = fErr;
        }
        fTotErr += fBestErr;
    }
    return fTotErr;
}


float Block_BC6H::PerturbOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, uint8_t ch,
    const INTEndPntPair& oldEndPts, INTEndPntPair& newEndPts, float fOldErr, int do_b) const
{
    assert(pEP);
    uint8_t uPrec;
    switch (ch)
    {
    case 0: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].r; break;
    case 1: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].g; break;
    case 2: uPrec = ms_aInfo[pEP->uMode].RGBAPrec[0][0].b; break;
    default: assert(false); newEndPts = oldEndPts; return FLT_MAX;
    }
    INTEndPntPair tmpEndPts;
    float fMinErr = fOldErr;
    int beststep = 0;

    // copy real endpoints so we can perturb them
    tmpEndPts = newEndPts = oldEndPts;

    // do a logarithmic search for the best error for this endpoint (which)
    for (int step = 1 << (uPrec - 1); step; step >>= 1)
    {
        bool bImproved = false;
        for (int sign = -1; sign <= 1; sign += 2)
        {
            if (do_b == 0)
            {
                tmpEndPts.A[ch] = newEndPts.A[ch] + sign * step;
                if (tmpEndPts.A[ch] < 0 || tmpEndPts.A[ch] >= (1 << uPrec))
                    continue;
            }
            else
            {
                tmpEndPts.B[ch] = newEndPts.B[ch] + sign * step;
                if (tmpEndPts.B[ch] < 0 || tmpEndPts.B[ch] >= (1 << uPrec))
                    continue;
            }

            float fErr = MapColorsQuantized(pEP, aColors, np, tmpEndPts);

            if (fErr < fMinErr)
            {
                bImproved = true;
                fMinErr = fErr;
                beststep = sign * step;
            }
        }
        // if this was an improvement, move the endpoint and continue search from there
        if (bImproved)
        {
            if (do_b == 0)
                newEndPts.A[ch] += beststep;
            else
                newEndPts.B[ch] += beststep;
        }
    }
    return fMinErr;
}


void Block_BC6H::OptimizeOne(const EncodeParams* pEP, const INTColor aColors[], size_t np, float aOrgErr,
    const INTEndPntPair &aOrgEndPts, INTEndPntPair &aOptEndPts) const
{
    assert(pEP);
    float aOptErr = aOrgErr;
    aOptEndPts.A = aOrgEndPts.A;
    aOptEndPts.B = aOrgEndPts.B;

    INTEndPntPair new_a, new_b;
    INTEndPntPair newEndPts;
    int do_b;

    // now optimize each channel separately
    for (uint8_t ch = 0; ch < BC6H_NUM_CHANNELS; ++ch)
    {
        // figure out which endpoint when perturbed gives the most improvement and start there
        // if we just alternate, we can easily end up in a local minima
        float fErr0 = PerturbOne(pEP, aColors, np, ch, aOptEndPts, new_a, aOptErr, 0);	// perturb endpt A
        float fErr1 = PerturbOne(pEP, aColors, np, ch, aOptEndPts, new_b, aOptErr, 1);	// perturb endpt B

        if (fErr0 < fErr1)
        {
            if (fErr0 >= aOptErr) continue;
            aOptEndPts.A[ch] = new_a.A[ch];
            aOptErr = fErr0;
            do_b = 1;		// do B next
        }
        else
        {
            if (fErr1 >= aOptErr) continue;
            aOptEndPts.B[ch] = new_b.B[ch];
            aOptErr = fErr1;
            do_b = 0;		// do A next
        }

        // now alternate endpoints and keep trying until there is no improvement
        for (;;)
        {
            float fErr = PerturbOne(pEP, aColors, np, ch, aOptEndPts, newEndPts, aOptErr, do_b);
            if (fErr >= aOptErr)
                break;
            if (do_b == 0)
                aOptEndPts.A[ch] = newEndPts.A[ch];
            else
                aOptEndPts.B[ch] = newEndPts.B[ch];
            aOptErr = fErr;
            do_b = 1 - do_b;	// now move the other endpoint
        }
    }
}


void Block_BC6H::OptimizeEndPoints(const EncodeParams* pEP, const float aOrgErr[], const INTEndPntPair aOrgEndPts[], INTEndPntPair aOptEndPts[]) const
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);
        INTColor aPixels[NUM_PIXELS_PER_BLOCK];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        // collect the pixels in the region
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            if (g_aPartitionTable[p][pEP->uShape][i] == p)
            {
                aPixels[np++] = pEP->aIPixels[i];
            }
        }

        OptimizeOne(pEP, aPixels, np, aOrgErr[p], aOrgEndPts[p], aOptEndPts[p]);
    }
}


// Swap endpoints as needed to ensure that the indices at fix up have a 0 high-order bit
void Block_BC6H::SwapIndices(const EncodeParams* pEP, INTEndPntPair aEndPts[], size_t aIndices[])
{
    assert(pEP);
    const size_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    const size_t uNumIndices = size_t(1) << ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uHighIndexBit = uNumIndices >> 1;

    assert(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        size_t i = g_aFixUp[uPartitions][pEP->uShape][p];
        assert(g_aPartitionTable[uPartitions][pEP->uShape][i] == p);
        if (aIndices[i] & uHighIndexBit)
        {
            // high bit is set, swap the aEndPts and indices for this region
            std::swap(aEndPts[p].A, aEndPts[p].B);

            for (size_t j = 0; j < NUM_PIXELS_PER_BLOCK; ++j)
                if (g_aPartitionTable[uPartitions][pEP->uShape][j] == p)
                    aIndices[j] = uNumIndices - 1 - aIndices[j];
        }
    }
}


// assign indices given a tile, shape, and quantized endpoints, return toterr for each region
void Block_BC6H::AssignIndices(const EncodeParams* pEP, const INTEndPntPair aEndPts[], size_t aIndices[], float aTotErr[]) const
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    const uint8_t uNumIndices = 1 << ms_aInfo[pEP->uMode].uIndexPrec;

    assert(uPartitions < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);

    // build list of possibles
    INTColor aPalette[BC6H_MAX_REGIONS][BC6H_MAX_INDICES];

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        GeneratePaletteQuantized(pEP, aEndPts[p], aPalette[p]);
        aTotErr[p] = 0;
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        const uint8_t uRegion = g_aPartitionTable[uPartitions][pEP->uShape][i];
        assert(uRegion < BC6H_MAX_REGIONS);
                float fBestErr = Norm(pEP->aIPixels[i], aPalette[uRegion][0]);
        aIndices[i] = 0;

        for (uint8_t j = 1; j < uNumIndices && fBestErr > 0; ++j)
        {
            float fErr = Norm(pEP->aIPixels[i], aPalette[uRegion][j]);
            if (fErr > fBestErr) break;	// error increased, so we're done searching
            if (fErr < fBestErr)
            {
                fBestErr = fErr;
                aIndices[i] = j;
            }
        }
        aTotErr[uRegion] += fBestErr;
    }
}


void Block_BC6H::QuantizeEndPts(const EncodeParams* pEP, INTEndPntPair* aQntEndPts) const
{
    assert(pEP && aQntEndPts);
    const INTEndPntPair* aUnqEndPts = pEP->aUnqEndPts[pEP->uShape];
    const LDRColorA& Prec = ms_aInfo[pEP->uMode].RGBAPrec[0][0];
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);

    for (size_t p = 0; p <= uPartitions; ++p)
    {
        aQntEndPts[p].A.r = Quantize(aUnqEndPts[p].A.r, Prec.r, pEP->bSigned);
        aQntEndPts[p].A.g = Quantize(aUnqEndPts[p].A.g, Prec.g, pEP->bSigned);
        aQntEndPts[p].A.b = Quantize(aUnqEndPts[p].A.b, Prec.b, pEP->bSigned);
        aQntEndPts[p].B.r = Quantize(aUnqEndPts[p].B.r, Prec.r, pEP->bSigned);
        aQntEndPts[p].B.g = Quantize(aUnqEndPts[p].B.g, Prec.g, pEP->bSigned);
        aQntEndPts[p].B.b = Quantize(aUnqEndPts[p].B.b, Prec.b, pEP->bSigned);
    }
}


void Block_BC6H::EmitBlock(const EncodeParams* pEP, const INTEndPntPair aEndPts[], const size_t aIndices[])
{
    assert(pEP);
    const uint8_t uRealMode = ms_aInfo[pEP->uMode].uMode;
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const size_t uHeaderBits = uPartitions > 0 ? 82 : 65;
    const ModeDescriptor* desc = ms_aDesc[pEP->uMode];
    size_t uStartBit = 0;

    while (uStartBit < uHeaderBits)
    {
        switch (desc[uStartBit].m_eField)
        {
        case M:  SetBit(uStartBit, uint8_t(uRealMode >> desc[uStartBit].m_uBit) & 0x01); break;
        case D:  SetBit(uStartBit, uint8_t(pEP->uShape >> desc[uStartBit].m_uBit) & 0x01); break;
        case RW: SetBit(uStartBit, uint8_t(aEndPts[0].A.r >> desc[uStartBit].m_uBit) & 0x01); break;
        case RX: SetBit(uStartBit, uint8_t(aEndPts[0].B.r >> desc[uStartBit].m_uBit) & 0x01); break;
        case RY: SetBit(uStartBit, uint8_t(aEndPts[1].A.r >> desc[uStartBit].m_uBit) & 0x01); break;
        case RZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.r >> desc[uStartBit].m_uBit) & 0x01); break;
        case GW: SetBit(uStartBit, uint8_t(aEndPts[0].A.g >> desc[uStartBit].m_uBit) & 0x01); break;
        case GX: SetBit(uStartBit, uint8_t(aEndPts[0].B.g >> desc[uStartBit].m_uBit) & 0x01); break;
        case GY: SetBit(uStartBit, uint8_t(aEndPts[1].A.g >> desc[uStartBit].m_uBit) & 0x01); break;
        case GZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.g >> desc[uStartBit].m_uBit) & 0x01); break;
        case BW: SetBit(uStartBit, uint8_t(aEndPts[0].A.b >> desc[uStartBit].m_uBit) & 0x01); break;
        case BX: SetBit(uStartBit, uint8_t(aEndPts[0].B.b >> desc[uStartBit].m_uBit) & 0x01); break;
        case BY: SetBit(uStartBit, uint8_t(aEndPts[1].A.b >> desc[uStartBit].m_uBit) & 0x01); break;
        case BZ: SetBit(uStartBit, uint8_t(aEndPts[1].B.b >> desc[uStartBit].m_uBit) & 0x01); break;
        default: assert(false);
        }
    }

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        if (IsFixUpOffset(ms_aInfo[pEP->uMode].uPartitions, pEP->uShape, i))
            SetBits(uStartBit, uIndexPrec - 1, static_cast<uint8_t>(aIndices[i]));
        else
            SetBits(uStartBit, uIndexPrec, static_cast<uint8_t>(aIndices[i]));
    }
    assert(uStartBit == 128);
}


void Block_BC6H::Refine(EncodeParams* pEP)
{
    assert(pEP);
    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);

    const bool bTransformed = ms_aInfo[pEP->uMode].bTransformed;
    float aOrgErr[BC6H_MAX_REGIONS], aOptErr[BC6H_MAX_REGIONS];
    INTEndPntPair aOrgEndPts[BC6H_MAX_REGIONS], aOptEndPts[BC6H_MAX_REGIONS];
    size_t aOrgIdx[NUM_PIXELS_PER_BLOCK], aOptIdx[NUM_PIXELS_PER_BLOCK];

    QuantizeEndPts(pEP, aOrgEndPts);
    AssignIndices(pEP, aOrgEndPts, aOrgIdx, aOrgErr);
    SwapIndices(pEP, aOrgEndPts, aOrgIdx);

    if (bTransformed) TransformForward(aOrgEndPts);
    if (EndPointsFit(pEP, aOrgEndPts))
    {
        if (bTransformed) TransformInverse(aOrgEndPts, ms_aInfo[pEP->uMode].RGBAPrec[0][0], pEP->bSigned);
        OptimizeEndPoints(pEP, aOrgErr, aOrgEndPts, aOptEndPts);
        AssignIndices(pEP, aOptEndPts, aOptIdx, aOptErr);
        SwapIndices(pEP, aOptEndPts, aOptIdx);

        float fOrgTotErr = 0.0f, fOptTotErr = 0.0f;
        for (size_t p = 0; p <= uPartitions; ++p)
        {
            fOrgTotErr += aOrgErr[p];
            fOptTotErr += aOptErr[p];
        }

        if (bTransformed) TransformForward(aOptEndPts);
        if (EndPointsFit(pEP, aOptEndPts) && fOptTotErr < fOrgTotErr && fOptTotErr < pEP->fBestErr)
        {
            pEP->fBestErr = fOptTotErr;
            EmitBlock(pEP, aOptEndPts, aOptIdx);
        }
        else if (fOrgTotErr < pEP->fBestErr)
        {
            // either it stopped fitting when we optimized it, or there was no improvement
            // so go back to the unoptimized endpoints which we know will fit
            if (bTransformed) TransformForward(aOrgEndPts);
            pEP->fBestErr = fOrgTotErr;
            EmitBlock(pEP, aOrgEndPts, aOrgIdx);
        }
    }
}


void Block_BC6H::GeneratePaletteUnquantized(const EncodeParams* pEP, size_t uRegion, INTColor aPalette[])
{
    assert(pEP);
    assert(uRegion < BC6H_MAX_REGIONS && pEP->uShape < BC6H_MAX_SHAPES);
        const INTEndPntPair& endPts = pEP->aUnqEndPts[pEP->uShape][uRegion];
    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    assert(uNumIndices > 0);

    const int* aWeights = nullptr;
    switch (uIndexPrec)
    {
    case 3: aWeights = g_aWeights3; assert(uNumIndices <= 8); break;
    case 4: aWeights = g_aWeights4; assert(uNumIndices <= 16); break;
    default:
        assert(false);
        for (size_t i = 0; i < uNumIndices; ++i)
        {
            aPalette[i] = INTColor(0, 0, 0);
        }
        return;
    }

    for (size_t i = 0; i < uNumIndices; ++i)
    {
        aPalette[i].r = (endPts.A.r * (BC67_WEIGHT_MAX - aWeights[i]) + endPts.B.r * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT;
        aPalette[i].g = (endPts.A.g * (BC67_WEIGHT_MAX - aWeights[i]) + endPts.B.g * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT;
        aPalette[i].b = (endPts.A.b * (BC67_WEIGHT_MAX - aWeights[i]) + endPts.B.b * aWeights[i] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT;
    }
}


float Block_BC6H::MapColors(const EncodeParams* pEP, size_t uRegion, size_t np, const size_t* auIndex) const
{
    assert(pEP);
    const uint8_t uIndexPrec = ms_aInfo[pEP->uMode].uIndexPrec;
    const uint8_t uNumIndices = 1 << uIndexPrec;
    INTColor aPalette[BC6H_MAX_INDICES];
    GeneratePaletteUnquantized(pEP, uRegion, aPalette);

    float fTotalErr = 0.0f;
    for (size_t i = 0; i < np; ++i)
    {
        float fBestErr = Norm(pEP->aIPixels[auIndex[i]], aPalette[0]);
        for (uint8_t j = 1; j < uNumIndices && fBestErr > 0.0f; ++j)
        {
            float fErr = Norm(pEP->aIPixels[auIndex[i]], aPalette[j]);
            if (fErr > fBestErr) break;      // error increased, so we're done searching
            if (fErr < fBestErr) fBestErr = fErr;
        }
        fTotalErr += fBestErr;
    }

    return fTotalErr;
}

float Block_BC6H::RoughMSE(EncodeParams* pEP) const
{
    assert(pEP);
    assert(pEP->uShape < BC6H_MAX_SHAPES);

    INTEndPntPair* aEndPts = pEP->aUnqEndPts[pEP->uShape];

    const uint8_t uPartitions = ms_aInfo[pEP->uMode].uPartitions;
    assert(uPartitions < BC6H_MAX_REGIONS);

    size_t auPixIdx[NUM_PIXELS_PER_BLOCK];

    float fError = 0.0f;
    for (size_t p = 0; p <= uPartitions; ++p)
    {
        size_t np = 0;
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            if (g_aPartitionTable[uPartitions][pEP->uShape][i] == p)
            {
                auPixIdx[np++] = i;
            }
        }

        // handle simple cases
        assert(np > 0);
        if (np == 1)
        {
            aEndPts[p].A = pEP->aIPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aIPixels[auPixIdx[0]];
            continue;
        }
        else if (np == 2)
        {
            aEndPts[p].A = pEP->aIPixels[auPixIdx[0]];
            aEndPts[p].B = pEP->aIPixels[auPixIdx[1]];
            continue;
        }

        HDRColorA epA, epB;
        OptimizeRGB(pEP->aHDRPixels, &epA, &epB, 4, np, auPixIdx);
        aEndPts[p].A.FromHDRColorA(epA, pEP->bSigned);
        aEndPts[p].B.FromHDRColorA(epB, pEP->bSigned);
        if (pEP->bSigned)
        {
            aEndPts[p].A.Clamp(-F16MAX, F16MAX);
            aEndPts[p].B.Clamp(-F16MAX, F16MAX);
        }
        else
        {
            aEndPts[p].A.Clamp(0, F16MAX);
            aEndPts[p].B.Clamp(0, F16MAX);
        }

        fError += MapColors(pEP, p, np, auPixIdx);
    }

    return fError;
}

//-------------------------------------------------------------------------------------
// BC6H Compression
//-------------------------------------------------------------------------------------
void DecodeBC6HU(HDRColorA *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(Block_BC6H) == 16, "Block_BC6H should be 16 bytes");
    reinterpret_cast<const Block_BC6H*>(pBC)->Decode(false, pColor);
}

void DecodeBC6HS(HDRColorA *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(Block_BC6H) == 16, "Block_BC6H should be 16 bytes");
    reinterpret_cast<const Block_BC6H*>(pBC)->Decode(true, pColor);
}

void EncodeBC6HU(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    UNREFERENCED_PARAMETER(flags);
    assert(pBC && pColor);
    static_assert(sizeof(Block_BC6H) == 16, "Block_BC6H should be 16 bytes");
    reinterpret_cast<Block_BC6H*>(pBC)->Encode(false, pColor);
}

void EncodeBC6HS(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    UNREFERENCED_PARAMETER(flags);
    assert(pBC && pColor);
    static_assert(sizeof(Block_BC6H) == 16, "Block_BC6H should be 16 bytes");
    reinterpret_cast<Block_BC6H*>(pBC)->Encode(true, pColor);
}

}
