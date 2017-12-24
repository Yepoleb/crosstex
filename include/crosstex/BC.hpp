#pragma once
#include <stdint.h>
#include <stddef.h>

#include "Colors.hpp"

#define UNREFERENCED_PARAMETER(name) do { (void)(name); } while (0)


namespace Tex
{
//-------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------

const size_t NUM_PIXELS_PER_BLOCK = 16;

enum BC_FLAGS
{
    BC_FLAGS_NONE               = 0x0,
    BC_FLAGS_DITHER_RGB         = 0x10000,  // Enables dithering for RGB colors for BC1-3
    BC_FLAGS_DITHER_A           = 0x20000,  // Enables dithering for Alpha channel for BC1-3
    BC_FLAGS_UNIFORM            = 0x40000,  // By default, uses perceptual weighting for BC1-3; this flag makes it a uniform weighting
    BC_FLAGS_USE_3SUBSETS       = 0x80000,  // By default, BC7 skips mode 0 & 2; this flag adds those modes back
    BC_FLAGS_FORCE_BC7_MODE6    = 0x100000, // BC7 should only use mode 6; skip other modes
};

//-------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------

typedef void (*BC_DECODE)(HDRColorA *pColor, const uint8_t *pBC);
typedef void (*BC_ENCODE)(uint8_t *pDXT, const HDRColorA *pColor, uint32_t flags);

void DecodeBC1(HDRColorA *pColor, const uint8_t *pBC);
void DecodeBC2(HDRColorA *pColor, const uint8_t *pBC);
void DecodeBC3(HDRColorA *pColor, const uint8_t *pBC);
void DecodeBC4U(HDRColorA *pColor, const uint8_t *pBC);
void DecodeBC4S(HDRColorA *pColor, const uint8_t *pBC);
void DecodeBC5U(HDRColorA *pColor, const uint8_t *pBC);
void DecodeBC5S(HDRColorA *pColor, const uint8_t *pBC);
void DecodeBC6HU(HDRColorA *pColor, const uint8_t *pBC);
void DecodeBC6HS(HDRColorA *pColor, const uint8_t *pBC);
void DecodeBC7(HDRColorA *pColor, const uint8_t *pBC);

void EncodeBC1(uint8_t *pBC, const HDRColorA *pColor, float threshold, uint32_t flags);
// BC1 requires one additional parameter, so it doesn't match signature of BC_ENCODE above

void EncodeBC2(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags);
void EncodeBC3(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags);
void EncodeBC4U(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags);
void EncodeBC4S(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags);
void EncodeBC5U(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags);
void EncodeBC5S(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags);
void EncodeBC6HU(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags);
void EncodeBC6HS(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags);
void EncodeBC7(uint8_t *pBC, const HDRColorA *pColor, uint32_t flags);

}; // namespace
