#ifndef TYPES_H
#define TYPES_H
#include <inttypes.h>

#define MAX(a, b) ({typeof(a) _a = a; typeof(b) _b = b; _a > _b ? _a : _b;})
#define MIN(a, b) ({typeof(a) _a = a; typeof(b) _b = b; _a < _b ? _a : _b;})
#define CLAMP(a, b, val) ({\
    typeof(a) _min = MIN(a, b);\
    typeof(b) _max = MAX(a, b);\
    typeof(val) _val = val;\
    _val > _max ? _max : _val < _min ? _min : _val;\
})
#define ABS(v) ({typeof(v) _v = v; _v > 0 ? _v : -_v;})
#define SGN(v) (v > 0 ? 1 : 0)

#define color32(r, g, b, a) ((color32) {.rgba = {r, g, b, a}})

typedef union color32 {
    uint32_t i;
    uint8_t rgba[4];
    struct {
        uint8_t r, g, b, a;
    };
} color32;

#endif