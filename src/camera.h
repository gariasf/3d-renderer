#pragma once

#include "vector.h"

#include "camera.h"

typedef struct {
    vec3_t position;
    vec3_t direction;
    vec3_t forward_velocity;
    float yaw;
} camera_t;

extern camera_t camera;