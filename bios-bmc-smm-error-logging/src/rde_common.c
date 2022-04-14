#include "rde_common.h"

uint64_t rdeGetUnsignedInteger(const uint8_t* bytes, uint8_t numOfBytes)
{
    uint64_t num = 0;
    for (uint8_t i = 0; i < numOfBytes; ++i)
    {
        num |= (uint64_t)(*(bytes + i)) << (i * 8);
    }
    return num;
}

uint64_t rdeGetNnint(const uint8_t* nnint)
{
    // In nnint, first byte indicate how many bytes are there.
    const uint8_t size = *nnint;
    return rdeGetUnsignedInteger(nnint + sizeof(uint8_t), size);
}

uint8_t rdeGetNnintSize(const uint8_t* nnint)
{
    // In nnint, first byte indicate how many bytes are there.
    return *nnint + sizeof(uint8_t);
}