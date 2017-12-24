#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cassert>
#include <algorithm>

#define SIGN_EXTEND(x,nb) ((((x)&(1<<((nb)-1)))?((~0)<<(nb)):0)|(x))


namespace Tex {

const uint16_t F16S_MASK = 0x8000;   // f16 sign mask
const uint16_t F16E_MASK = 0x7c00;   // f16 exponent mask
const uint16_t F16M_MASK = 0x03FF;   // f16 mantissa mask
const uint16_t F16EM_MASK = 0x7fff;   // f16 exp & mantissa mask
const uint16_t F16MAX = 0x7bff;   // MAXFLT bit pattern for XMHALF

const uint32_t F32E_MASK = 0x7f800000;


class HDRColorA
{
public:
    float r, g, b, a;

    HDRColorA() = default;
    HDRColorA(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {}

    HDRColorA operator+(const HDRColorA& c) const
    {
        return HDRColorA(r + c.r, g + c.g, b + c.b, a + c.a);
    }

    HDRColorA operator-(const HDRColorA& c) const
    {
        return HDRColorA(r - c.r, g - c.g, b - c.b, a - c.a);
    }

    HDRColorA operator*(const HDRColorA& c) const
    {
        return HDRColorA(r * c.r, g * c.g, b * c.b, a * c.a);
    }

    HDRColorA operator/(const HDRColorA& c) const
    {
        return HDRColorA(r / c.r, g / c.g, b / c.b, a / c.a);
    }

    HDRColorA operator+(float n) const
    {
        return HDRColorA(r + n, g + n, b + n, a + n);
    }

    HDRColorA operator-(float n) const
    {
        return HDRColorA(r - n, g - n, b - n, a - n);
    }

    HDRColorA operator*(float n) const
    {
        return HDRColorA(r * n, g * n, b * n, a * n);
    }

    HDRColorA operator/(float n) const
    {
        return HDRColorA(r / n, g / n, b / n, a / n);
    }

    HDRColorA& operator+=(const HDRColorA& c)
    {
        r += c.r;
        g += c.g;
        b += c.b;
        a += c.a;
        return *this;
    }

    HDRColorA& operator-=(const HDRColorA& c)
    {
        r -= c.r;
        g -= c.g;
        b -= c.b;
        a -= c.a;
        return *this;
    }

    HDRColorA& operator*=(const HDRColorA& c)
    {
        r *= c.r;
        g *= c.g;
        b *= c.b;
        a *= c.a;
        return *this;
    }

    HDRColorA& operator/=(const HDRColorA& c)
    {
        r /= c.r;
        g /= c.g;
        b /= c.b;
        a /= c.a;
        return *this;
    }

    HDRColorA& operator+=(float n)
    {
        r += n;
        g += n;
        b += n;
        a += n;
        return *this;
    }

    HDRColorA& operator-=(float n)
    {
        r -= n;
        g -= n;
        b -= n;
        a -= n;
        return *this;
    }

    HDRColorA& operator*=(float n)
    {
        r *= n;
        g *= n;
        b *= n;
        a *= n;
        return *this;
    }

    HDRColorA& operator/=(float n)
    {
        r /= n;
        g /= n;
        b /= n;
        a /= n;
        return *this;
    }

    const float& operator[](size_t i) const
    {
        switch (i)
        {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        case 3: return a;
        default: assert(false); return r;
        }
    }

    float& operator[](size_t i)
    {
        switch (i)
        {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        case 3: return a;
        default: assert(false); return r;
        }
    }

    HDRColorA& Clamp(float vmin, float vmax)
    {
        r = std::min<float>(vmax, std::max<float>(vmin, r));
        g = std::min<float>(vmax, std::max<float>(vmin, g));
        b = std::min<float>(vmax, std::max<float>(vmin, b));
        a = std::min<float>(vmax, std::max<float>(vmin, a));
        return *this;
    }

    static HDRColorA Lerp(const HDRColorA& c1, const HDRColorA& c2, float ratio)
    {
        HDRColorA c_out;

        c_out.r = c1.r + (float)(ratio * (c2.r - c1.r));
        c_out.g = c1.g + (float)(ratio * (c2.g - c1.g));
        c_out.b = c1.b + (float)(ratio * (c2.b - c1.b));
        c_out.a = c1.a + (float)(ratio * (c2.a - c1.a));
        return c_out;
    }

    static float DotProduct(const HDRColorA& c1, const HDRColorA& c2)
    {
        return c1.r * c2.r + c1.g * c2.g + c1.b * c2.b + c1.a * c2.a;
    }

    float Length2()
    {
        return DotProduct(*this, *this);
    }

    void Decode565(const uint16_t w565)
    {
        r = (float)((w565 >> 11) & 31) * (1.0f / 31.0f);
        g = (float)((w565 >> 5) & 63) * (1.0f / 63.0f);
        b = (float)((w565 >> 0) & 31) * (1.0f / 31.0f);
        a = 1.0f;
    }

    uint16_t Encode565()
    {
        HDRColorA c(*this);
        c.Clamp(0.0f, 1.0f);

        uint16_t w;
        w = (uint16_t)((static_cast<int32_t>(c.r * 31.0f + 0.5f) << 11) |
            (static_cast<int32_t>(c.g * 63.0f + 0.5f) << 5) |
            (static_cast<int32_t>(c.b * 31.0f + 0.5f) << 0));

        return w;
    }
};


class LDRColorA
{
public:
    uint8_t r, g, b, a;

    LDRColorA() = default;
    LDRColorA(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_) : r(r_), g(g_), b(b_), a(a_) {}

    LDRColorA operator+(const LDRColorA& c) const
    {
        return LDRColorA(r + c.r, g + c.g, b + c.b, a + c.a);
    }

    LDRColorA operator-(const LDRColorA& c) const
    {
        return LDRColorA(r - c.r, g - c.g, b - c.b, a - c.a);
    }

    LDRColorA operator*(const LDRColorA& c) const
    {
        return LDRColorA(r * c.r, g * c.g, b * c.b, a * c.a);
    }

    LDRColorA operator/(const LDRColorA& c) const
    {
        return LDRColorA(r / c.r, g / c.g, b / c.b, a / c.a);
    }

    LDRColorA operator+(uint8_t n) const
    {
        return LDRColorA(r + n, g + n, b + n, a + n);
    }

    LDRColorA operator-(uint8_t n) const
    {
        return LDRColorA(r - n, g - n, b - n, a - n);
    }

    LDRColorA operator*(uint8_t n) const
    {
        return LDRColorA(r * n, g * n, b * n, a * n);
    }

    LDRColorA operator/(uint8_t n) const
    {
        return LDRColorA(r / n, g / n, b / n, a / n);
    }

    LDRColorA& operator+=(const LDRColorA& c)
    {
        r += c.r;
        g += c.g;
        b += c.b;
        a += c.a;
        return *this;
    }

    LDRColorA& operator-=(const LDRColorA& c)
    {
        r -= c.r;
        g -= c.g;
        b -= c.b;
        a -= c.a;
        return *this;
    }

    LDRColorA& operator*=(const LDRColorA& c)
    {
        r *= c.r;
        g *= c.g;
        b *= c.b;
        a *= c.a;
        return *this;
    }

    LDRColorA& operator/=(const LDRColorA& c)
    {
        r /= c.r;
        g /= c.g;
        b /= c.b;
        a /= c.a;
        return *this;
    }

    LDRColorA& operator+=(uint8_t n)
    {
        r += n;
        g += n;
        b += n;
        a += n;
        return *this;
    }

    LDRColorA& operator-=(uint8_t n)
    {
        r -= n;
        g -= n;
        b -= n;
        a -= n;
        return *this;
    }

    LDRColorA& operator*=(uint8_t n)
    {
        r *= n;
        g *= n;
        b *= n;
        a *= n;
        return *this;
    }

    LDRColorA& operator/=(uint8_t n)
    {
        r /= n;
        g /= n;
        b /= n;
        a /= n;
        return *this;
    }

    const uint8_t& operator[](size_t i) const
    {
        switch (i)
        {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        case 3: return a;
        default: assert(false); return r;
        }
    }

    uint8_t& operator[](size_t i)
    {
        switch (i)
        {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        case 3: return a;
        default: assert(false); return r;
        }
    }

    LDRColorA& Clamp(uint8_t vmin, uint8_t vmax)
    {
        r = std::min<uint8_t>(vmax, std::max<uint8_t>(vmin, r));
        g = std::min<uint8_t>(vmax, std::max<uint8_t>(vmin, g));
        b = std::min<uint8_t>(vmax, std::max<uint8_t>(vmin, b));
        a = std::min<uint8_t>(vmax, std::max<uint8_t>(vmin, a));
        return *this;
    }

    static LDRColorA Lerp(const LDRColorA& c1, const LDRColorA& c2, float ratio)
    {
        LDRColorA c_out;

        c_out.r = c1.r + (uint8_t)(ratio * (c2.r - c1.r));
        c_out.g = c1.g + (uint8_t)(ratio * (c2.g - c1.g));
        c_out.b = c1.b + (uint8_t)(ratio * (c2.b - c1.b));
        c_out.a = c1.a + (uint8_t)(ratio * (c2.a - c1.a));
        return c_out;
    }

    static float DotProduct(const LDRColorA& c1, const LDRColorA& c2)
    {
        return c1.r * c2.r + c1.g * c2.g + c1.b * c2.b + c1.a * c2.a;
    }

    static LDRColorA FromHDRColorA(const HDRColorA& c_hdr)
    {
        LDRColorA c_ldr;
        c_ldr.r = (c_hdr.r * 255.0f + 0.01f);
        c_ldr.g = (c_hdr.g * 255.0f + 0.01f);
        c_ldr.b = (c_hdr.b * 255.0f + 0.01f);
        c_ldr.a = (c_hdr.a * 255.0f + 0.01f);
        return c_ldr;
    }

    HDRColorA ToHDRColorA() const
    {
        HDRColorA c_hdr;
        c_hdr.r = r * (1.0f / 255.0f);
        c_hdr.g = g * (1.0f / 255.0f);
        c_hdr.b = b * (1.0f / 255.0f);
        c_hdr.a = a * (1.0f / 255.0f);
        return c_hdr;
    }
};


class INTColor
{
public:
    int r, g, b, pad;

    INTColor() = default;
    INTColor(int r_, int g_, int b_) : r(r_), g(g_), b(b_) {}
    INTColor(const INTColor& c) : r(c.r), g(c.g), b(c.b) {}

    INTColor operator+(const INTColor& c) const
    {
        return INTColor(r + c.r, g + c.g, b + c.b);
    }

    INTColor operator-(const INTColor& c) const
    {
        return INTColor(r - c.r, g - c.g, b - c.b);
    }

    INTColor operator*(const INTColor& c) const
    {
        return INTColor(r * c.r, g * c.g, b * c.b);
    }

    INTColor operator/(const INTColor& c) const
    {
        return INTColor(r / c.r, g / c.g, b / c.b);
    }

    INTColor operator+(int n) const
    {
        return INTColor(r + n, g + n, b + n);
    }

    INTColor operator-(int n) const
    {
        return INTColor(r - n, g - n, b - n);
    }

    INTColor operator*(int n) const
    {
        return INTColor(r * n, g * n, b * n);
    }

    INTColor operator/(int n) const
    {
        return INTColor(r / n, g / n, b / n);
    }

    INTColor& operator+=(const INTColor& c)
    {
        r += c.r;
        g += c.g;
        b += c.b;
        return *this;
    }

    INTColor& operator-=(const INTColor& c)
    {
        r -= c.r;
        g -= c.g;
        b -= c.b;
        return *this;
    }

    INTColor& operator*=(const INTColor& c)
    {
        r *= c.r;
        g *= c.g;
        b *= c.b;
        return *this;
    }

    INTColor& operator/=(const INTColor& c)
    {
        r /= c.r;
        g /= c.g;
        b /= c.b;
        return *this;
    }

    INTColor& operator&=(const INTColor& c)
    {
        r &= c.r;
        g &= c.g;
        b &= c.b;
        return *this;
    }

    INTColor& operator+=(int n)
    {
        r += n;
        g += n;
        b += n;
        return *this;
    }

    INTColor& operator-=(int n)
    {
        r -= n;
        g -= n;
        b -= n;
        return *this;
    }

    INTColor& operator*=(int n)
    {
        r *= n;
        g *= n;
        b *= n;
        return *this;
    }

    INTColor& operator/=(int n)
    {
        r /= n;
        g /= n;
        b /= n;
        return *this;
    }

    const int& operator[](size_t i) const
    {
        switch (i)
        {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        default: assert(false); return r;
        }
    }

    int& operator[](size_t i)
    {
        switch (i)
        {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        default: assert(false); return r;
        }
    }

    INTColor& Clamp(int vmin, int vmax)
    {
        r = std::min<int>(vmax, std::max<int>(vmin, r));
        g = std::min<int>(vmax, std::max<int>(vmin, g));
        b = std::min<int>(vmax, std::max<int>(vmin, b));
        return *this;
    }

    static INTColor Lerp(const INTColor& c1, const INTColor& c2, float ratio)
    {
        INTColor c_out;

        c_out.r = c1.r + (int)(ratio * (c2.r - c1.r));
        c_out.g = c1.g + (int)(ratio * (c2.g - c1.g));
        c_out.b = c1.b + (int)(ratio * (c2.b - c1.b));
        return c_out;
    }

    static float DotProduct(const INTColor& c1, const INTColor& c2)
    {
        return c1.r * c2.r + c1.g * c2.g + c1.b * c2.b;
    }

    HDRColorA ToHDRColorA(bool bSigned) const
    {
        HDRColorA c_hdr;

        c_hdr.r = INT2Float(r, bSigned);
        c_hdr.g = INT2Float(r, bSigned);
        c_hdr.b = INT2Float(r, bSigned);
        c_hdr.a = 1.0f;
        return c_hdr;
    }

    static INTColor FromHDRColorA(const HDRColorA& c_hdr, bool bSigned)
    {
        INTColor c_int;

        c_int.r = FloatToINT(c_hdr.r, bSigned);
        c_int.g = FloatToINT(c_hdr.g, bSigned);
        c_int.b = FloatToINT(c_hdr.b, bSigned);
    }

    void SignExtend(const LDRColorA& prec)
    {
        r = SIGN_EXTEND(r, prec.r);
        g = SIGN_EXTEND(g, prec.g);
        b = SIGN_EXTEND(b, prec.b);
    }

    static int FloatToINT(float f, bool bSigned)
    {
        uint32_t f32 = *reinterpret_cast<uint32_t*>(&f);
        uint16_t f16 = (f32 >> 16) & F16S_MASK;
        f16 |= (((f32 & F32E_MASK) - 0x38000000) >> 13) & F16E_MASK;
        f16 |= (f32 >> 13) & F16M_MASK;

        int out;
        if (bSigned)
        {
            int s = f16 & F16S_MASK;
            f16 &= F16EM_MASK;
            if (f16 > F16MAX) {
                out = F16MAX;
            } else {
                out = f16;
            }
            out = s ? -out : out;
        }
        else
        {
            if (f16 & F16S_MASK) {
                out = 0;
            } else {
                out = f16;
            }
        }
        return out;
    }

    static float INT2Float(int input, bool bSigned)
    {
        uint16_t out;
        if (bSigned)
        {
            int s = 0;
            if (input < 0)
            {
                s = F16S_MASK;
                input = -input;
            }
            out = uint16_t(s | input);
        }
        else
        {
            assert(input >= 0 && input <= F16MAX);
            out = (uint16_t)input;
        }

        uint32_t f32 = (out & F16S_MASK) << 16;
        f32 |= ((out & F16E_MASK) + 0x1C000) << 13;
        f32 |= (out & F16M_MASK) << 13;
        return *reinterpret_cast<float*>(&f32);
    }
};

static_assert(sizeof(LDRColorA) == 4, "Unexpected packing");
static_assert(sizeof(INTColor) == 16, "Unexpected packing");

struct LDREndPntPair
{
    LDRColorA A;
    LDRColorA B;
};

struct INTEndPntPair
{
    INTColor A;
    INTColor B;
};

}
