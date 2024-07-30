#include <math.h>
#include "matrix.h"

mat4_t mat4_identity(void) {
    mat4_t m = {{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
    }};

    return m;
}

mat4_t mat4_make_scale(float sx, float sy, float sz) {
    mat4_t m = {{
        {sx, 0, 0, 0},
        {0, sy, 0, 0},
        {0, 0, sz, 0},
        {0, 0, 0, 1},
    }};

    return m;
}

mat4_t mat4_make_translation(float tx, float ty, float tz) {
    mat4_t m = {{
        {1, 0, 0, tx},
        {0, 1, 0, ty},
        {0, 0, 1, tz},
        {0, 0, 0, 1},
    }};

    return m;
}

mat4_t mat4_make_rotation_z(float angle) {
    float cosAlpha = cos(angle);
    float sinAlpha = sin(angle);

    mat4_t m = {{
        {cosAlpha, -sinAlpha, 0, 0},
        {sinAlpha, cosAlpha,  0, 0},
        {0,        0,         1, 0},
        {0,        0,         0, 1}
    }};

    return m;
}
mat4_t mat4_make_rotation_y(float angle) {
    float cosAlpha = cos(angle);
    float sinAlpha = sin(angle);

    mat4_t m = {{
        {cosAlpha,  0, sinAlpha, 0},
        {0,         1, 0,        0},
        {-sinAlpha, 0, cosAlpha, 0},
        {0,         0, 0,        1}
    }};

    return m;
}
mat4_t mat4_make_rotation_x(float angle) {
    float cosAlpha = cos(angle);
    float sinAlpha = sin(angle);

    mat4_t m = {{
        {1, 0,        0,         0},
        {0, cosAlpha, -sinAlpha, 0},
        {0, sinAlpha, cosAlpha,  0},
        {0, 0,        0,         1}
    }};

    return m;
}

vec4_t mat4_mul_vec4(mat4_t m, vec4_t v) {
    vec4_t result = {
        .x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w,
        .y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w,
        .z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w,
        .w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w
    };

    return result;
}