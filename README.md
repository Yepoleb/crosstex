# crosstex

Cross-platform port of the DirectXTex block compression codecs.

Supported codecs: BC1, BC2, BC3, BC4U, BC4S, BC5U, BC5S, BC6HU, BC6HS, BC7

## Usage

```c++
#include "crosstex/BC.hpp"

constexpr size_t BLOCK_SIZE_BC1 = 8;

uint8_t block_compressed[BLOCK_SIZE_BC1];
Tex::HDRColorA block_decompressed[16];

EncodeBC1(block_compressed, block_decompressed, BC_FLAGS_NONE);
DecodeBC1(block_decompressed, block_compressed);
```

## Building

    mkdir build
    cd build
    cmake ..
    make

## Installing

    make install

## License

[MIT](LICENSE)
