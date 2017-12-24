#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "BC.hpp"
#include "BC45_shared.hpp"
#include "Colors.hpp"


namespace Tex {

inline void FindEndPointsBC5U(
    const float theTexelsU[],
    const float theTexelsV[],
    uint8_t &endpointU_0,
    uint8_t &endpointU_1,
    uint8_t &endpointV_0,
    uint8_t &endpointV_1)
{
    //Encoding the U and V channel by BC4 codec separately.
    FindEndPointsBC4U(theTexelsU, endpointU_0, endpointU_1);
    FindEndPointsBC4U(theTexelsV, endpointV_0, endpointV_1);
}

inline void FindEndPointsBC5S(
    const float theTexelsU[],
    const float theTexelsV[],
    int8_t &endpointU_0,
    int8_t &endpointU_1,
    int8_t &endpointV_0,
    int8_t &endpointV_1)
{
    //Encoding the U and V channel by BC4 codec separately.
    FindEndPointsBC4S(theTexelsU, endpointU_0, endpointU_1);
    FindEndPointsBC4S(theTexelsV, endpointV_0, endpointV_1);
}

void DecodeBC5U(HDRColorA *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(BC4_UNORM) == 8, "BC4_UNORM should be 8 bytes");

    auto pBCR = reinterpret_cast<const BC4_UNORM*>(pBC);
    auto pBCG = reinterpret_cast<const BC4_UNORM*>(pBC + sizeof(BC4_UNORM));

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        pColor[i] = HDRColorA(pBCR->R(i), pBCG->R(i), 0, 1.0f);
    }
}

void DecodeBC5S(HDRColorA *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(BC4_SNORM) == 8, "BC4_SNORM should be 8 bytes");

    auto pBCR = reinterpret_cast<const BC4_SNORM*>(pBC);
    auto pBCG = reinterpret_cast<const BC4_SNORM*>(pBC + sizeof(BC4_SNORM));

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        pColor[i] = HDRColorA(pBCR->R(i), pBCG->R(i), 0, 1.0f);
    }
}

void EncodeBC5U(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    UNREFERENCED_PARAMETER(flags);

    assert(pBC && pColor);
    static_assert(sizeof(BC4_UNORM) == 8, "BC4_UNORM should be 8 bytes");

    memset(pBC, 0, sizeof(BC4_UNORM) * 2);
    auto pBCR = reinterpret_cast<BC4_UNORM*>(pBC);
    auto pBCG = reinterpret_cast<BC4_UNORM*>(pBC + sizeof(BC4_UNORM));
    float theTexelsU[NUM_PIXELS_PER_BLOCK];
    float theTexelsV[NUM_PIXELS_PER_BLOCK];

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        theTexelsU[i] = pColor[i].r;
        theTexelsV[i] = pColor[i].g;
    }

    FindEndPointsBC5U(
        theTexelsU,
        theTexelsV,
        pBCR->red_0,
        pBCR->red_1,
        pBCG->red_0,
        pBCG->red_1);

    FindClosestUNORM(pBCR, theTexelsU);
    FindClosestUNORM(pBCG, theTexelsV);
}

void EncodeBC5S(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    UNREFERENCED_PARAMETER(flags);

    assert(pBC && pColor);
    static_assert(sizeof(BC4_SNORM) == 8, "BC4_SNORM should be 8 bytes");

    memset(pBC, 0, sizeof(BC4_UNORM) * 2);
    auto pBCR = reinterpret_cast<BC4_SNORM*>(pBC);
    auto pBCG = reinterpret_cast<BC4_SNORM*>(pBC + sizeof(BC4_SNORM));
    float theTexelsU[NUM_PIXELS_PER_BLOCK];
    float theTexelsV[NUM_PIXELS_PER_BLOCK];

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        theTexelsU[i] = pColor[i].r;
        theTexelsV[i] = pColor[i].g;
    }

    FindEndPointsBC5S(
        theTexelsU,
        theTexelsV,
        pBCR->red_0,
        pBCR->red_1,
        pBCG->red_0,
        pBCG->red_1);

    FindClosestSNORM(pBCR, theTexelsU);
    FindClosestSNORM(pBCG, theTexelsV);
}

}
