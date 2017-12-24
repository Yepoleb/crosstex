#pragma once
#include <stdint.h>

#include "BC.hpp"
#include "Colors.hpp"


namespace Tex {

#pragma pack(push,1)
// BC1/DXT1 compression (4 bits per texel)
struct Block_BC1
{
    uint16_t    rgb[2]; // 565 colors
    uint32_t    bitmap; // 2bpp rgb bitmap
};

// BC2/DXT2/3 compression (8 bits per texel)
struct Block_BC2
{
    uint32_t    bitmap[2];  // 4bpp alpha bitmap
    Block_BC1    bc1;        // BC1 rgb data
};

// BC3/DXT4/5 compression (8 bits per texel)
struct Block_BC3
{
    uint8_t     alpha[2];   // alpha values
    uint8_t     bitmap[6];  // 3bpp alpha bitmap
    Block_BC1    bc1;        // BC1 rgb data
};
#pragma pack(pop)

void DecodeBC1(HDRColorA *pColor, const Block_BC1 *pBC, bool isbc1);
void EncodeBC1(Block_BC1 *pBC, const HDRColorA *pColor, bool bColorKey, float threshold, uint32_t flags);
#ifdef COLOR_WEIGHTS
void EncodeSolidBC1(Block_BC1 *pBC, const HDRColorA *pColor);
#endif

}
