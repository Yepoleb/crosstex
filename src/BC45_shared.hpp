#include <stdint.h>
#include <stddef.h>

#include "BC.hpp"


namespace Tex {

// BC4U/BC5U
struct BC4_UNORM
{
    float R(size_t uOffset) const
    {
        size_t uIndex = GetIndex(uOffset);
        return DecodeFromIndex(uIndex);
    }

    float DecodeFromIndex(size_t uIndex) const
    {
        if (uIndex == 0)
            return red_0 / 255.0f;
        if (uIndex == 1)
            return red_1 / 255.0f;
        float fred_0 = red_0 / 255.0f;
        float fred_1 = red_1 / 255.0f;
        if (red_0 > red_1)
        {
            uIndex -= 1;
            return (fred_0 * (7 - uIndex) + fred_1 * uIndex) / 7.0f;
        }
        else
        {
            if (uIndex == 6)
                return 0.0f;
            if (uIndex == 7)
                return 1.0f;
            uIndex -= 1;
            return (fred_0 * (5 - uIndex) + fred_1 * uIndex) / 5.0f;
        }
    }

    size_t GetIndex(size_t uOffset) const
    {
        return (size_t)((data >> (3 * uOffset + 16)) & 0x07);
    }

    void SetIndex(size_t uOffset, size_t uIndex)
    {
        data &= ~((uint64_t)0x07 << (3 * uOffset + 16));
        data |= ((uint64_t)uIndex << (3 * uOffset + 16));
    }

    union
    {
        struct
        {
            uint8_t red_0;
            uint8_t red_1;
            uint8_t indices[6];
        };
        uint64_t data;
    };
};

// BC4S/BC5S
struct BC4_SNORM
{
    float R(size_t uOffset) const
    {
        size_t uIndex = GetIndex(uOffset);
        return DecodeFromIndex(uIndex);
    }

    float DecodeFromIndex(size_t uIndex) const
    {
        int8_t sred_0 = (red_0 == -128) ? -127 : red_0;
        int8_t sred_1 = (red_1 == -128) ? -127 : red_1;

        if (uIndex == 0)
            return sred_0 / 127.0f;
        if (uIndex == 1)
            return sred_1 / 127.0f;
        float fred_0 = sred_0 / 127.0f;
        float fred_1 = sred_1 / 127.0f;
        if (red_0 > red_1)
        {
            uIndex -= 1;
            return (fred_0 * (7 - uIndex) + fred_1 * uIndex) / 7.0f;
        }
        else
        {
            if (uIndex == 6)
                return -1.0f;
            if (uIndex == 7)
                return 1.0f;
            uIndex -= 1;
            return (fred_0 * (5 - uIndex) + fred_1 * uIndex) / 5.0f;
        }
    }

    size_t GetIndex(size_t uOffset) const
    {
        return (size_t)((data >> (3 * uOffset + 16)) & 0x07);
    }

    void SetIndex(size_t uOffset, size_t uIndex)
    {
        data &= ~((uint64_t)0x07 << (3 * uOffset + 16));
        data |= ((uint64_t)uIndex << (3 * uOffset + 16));
    }

    union
    {
        struct
        {
            int8_t red_0;
            int8_t red_1;
            uint8_t indices[6];
        };
        uint64_t data;
    };
};

void FindEndPointsBC4U(const float* theTexelsU, uint8_t &endpointU_0, uint8_t &endpointU_1);
void FindEndPointsBC4S(const float* theTexelsU, int8_t &endpointU_0, int8_t &endpointU_1);
void FindClosestUNORM(BC4_UNORM* pBC, const float* theTexelsU);
void FindClosestSNORM(BC4_SNORM* pBC, const float* theTexelsU);

}
