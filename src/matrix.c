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

mat4_t mat4_make_perspective(float fov, float aspect, float znear, float zfar) {
    // | (h/w)*1/tan(fov/2)             0              0                 0 |
    // |                  0  1/tan(fov/2)              0                 0 |
    // |                  0             0     zf/(zf-zn)  (-zf*zn)/(zf-zn) |
    // |                  0             0              1                 0 |
    mat4_t m = {{{ 0 }}};
    m.m[0][0] = aspect * (1 / tan(fov / 2));
    m.m[1][1] = 1 / tan(fov / 2);
    m.m[2][2] = zfar / (zfar - znear);
    m.m[2][3] = (-zfar * znear) / (zfar - znear);
    m.m[3][2] = 1.0;
    return m;
}


mat4_t mat4_mul_mat4(mat4_t a, mat4_t b) {
    mat4_t m;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m.m[i][j] =
                a.m[i][0] * b.m[0][j] +
                a.m[i][1] * b.m[1][j] +
                a.m[i][2] * b.m[2][j] +
                a.m[i][3] * b.m[3][j];
        }
    }

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

vec4_t mat4_mul_vec4_project(mat4_t mat_proj, vec4_t v) {
    vec4_t result = mat4_mul_vec4(mat_proj, v);

    // Perspective divide with original z-value that is now stored in w
    if (result.w != 0.0) {
        result.x /= result.w;
        result.y /= result.w;
        result.z /= result.w;
    }
    return result;
}


mat4_t mat4_look_at(vec3_t eye, vec3_t target, vec3_t up) {
    // Compute the forward (z), right (x), and up (y) vectors
    vec3_t z = vec3_sub(target, eye);
    vec3_normalize(&z);
    vec3_t x = vec3_cross(up, z);
    vec3_normalize(&x);
    vec3_t y = vec3_cross(z, x);

    // | x.x   x.y   x.z  -dot(x,eye) |
    // | y.x   y.y   y.z  -dot(y,eye) |
    // | z.x   z.y   z.z  -dot(z,eye) |
    // |   0     0     0            1 |
    mat4_t view_matrix = {{
        { x.x, x.y, x.z, -vec3_dot(x, eye) },
        { y.x, y.y, y.z, -vec3_dot(y, eye) },
        { z.x, z.y, z.z, -vec3_dot(z, eye) },
        {   0,   0,   0,                 1 }
    }};
    return view_matrix;
}
