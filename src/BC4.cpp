#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "BC.hpp"
#include "BC45_shared.hpp"
#include "Colors.hpp"


namespace Tex {

void DecodeBC4U(HDRColorA *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(BC4_UNORM) == 8, "BC4_UNORM should be 8 bytes");

    auto pBC4 = reinterpret_cast<const BC4_UNORM*>(pBC);

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        pColor[i] = HDRColorA(pBC4->R(i), 0.0f, 0.0f, 1.0f);
    }
}

void DecodeBC4S(HDRColorA *pColor, const uint8_t *pBC)
{
    assert(pColor && pBC);
    static_assert(sizeof(BC4_SNORM) == 8, "BC4_SNORM should be 8 bytes");

    auto pBC4 = reinterpret_cast<const BC4_SNORM*>(pBC);

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        pColor[i] = HDRColorA(pBC4->R(i), 0.0f, 0.0f, 1.0f);
    }
}

void EncodeBC4U(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    UNREFERENCED_PARAMETER(flags);

    assert(pBC && pColor);
    static_assert(sizeof(BC4_UNORM) == 8, "BC4_UNORM should be 8 bytes");

    memset(pBC, 0, sizeof(BC4_UNORM));
    auto pBC4 = reinterpret_cast<BC4_UNORM*>(pBC);
    float theTexelsU[NUM_PIXELS_PER_BLOCK];

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        theTexelsU[i] = pColor[i].r;
    }

    FindEndPointsBC4U(theTexelsU, pBC4->red_0, pBC4->red_1);
    FindClosestUNORM(pBC4, theTexelsU);
}

void EncodeBC4S(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags)
{
    UNREFERENCED_PARAMETER(flags);

    assert(pBC && pColor);
    static_assert(sizeof(BC4_SNORM) == 8, "BC4_SNORM should be 8 bytes");

    memset(pBC, 0, sizeof(BC4_UNORM));
    auto pBC4 = reinterpret_cast<BC4_SNORM*>(pBC);
    float theTexelsU[NUM_PIXELS_PER_BLOCK];

    for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
    {
        theTexelsU[i] = pColor[i].r;
    }

    FindEndPointsBC4S(theTexelsU, pBC4->red_0, pBC4->red_1);
    FindClosestSNORM(pBC4, theTexelsU);
}

}
