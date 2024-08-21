#ifndef TYPES_H
#define TYPES_H
#include <inttypes.h>

#define color32(r, g, b, a) ((color32) {.rgba = {r, g, b, a}})

typedef union color32 {
    uint32_t i;
    uint8_t rgba[4];
    struct {
        uint8_t r, g, b, a;
    };
} color32;

#endif