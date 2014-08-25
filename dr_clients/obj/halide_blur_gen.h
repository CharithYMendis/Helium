#ifndef HALIDE_halide_blur_gen
#define HALIDE_halide_blur_gen
#ifndef BUFFER_T_DEFINED
#define BUFFER_T_DEFINED
#include <stdint.h>
#include <stdbool.h>
typedef struct buffer_t {
    uint64_t dev;
    uint8_t* host;
    int32_t extent[4];
    int32_t stride[4];
    int32_t min[4];
    int32_t elem_size;
    bool host_dirty;
    bool dev_dirty;
} buffer_t;
#endif
#ifndef HALIDE_FUNCTION_ATTRS
#define HALIDE_FUNCTION_ATTRS
#endif
#ifdef __cplusplus
extern "C"
#endif
int halide_blur_gen(buffer_t *_p0, buffer_t *_blur_y) HALIDE_FUNCTION_ATTRS;
#endif
