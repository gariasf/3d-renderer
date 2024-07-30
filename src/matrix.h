#pragma once

#include "vector.h"

typedef struct {
    float m[4][4];
} mat4_t;

mat4_t mat4_indentity(void);
mat4_t mat4_make_scale(float sx, float sy, float sz);
mat4_t mat4_make_translation(float tx, float ty, float tz);
mat4_t mat4_make_rotation_z(float angle);
mat4_t mat4_make_rotation_y(float angle);
mat4_t mat4_make_rotation_x(float angle);
vec4_t mat4_mul_vec4(mat4_t m, vec4_t v);
