#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "display.h"
#include "vector.h"
#include "mesh.h"
#include "array.h"

triangle_t *triangles_to_render = NULL;

vec3_t camera_position = {0, 0, 0};

// float fov_factor = 640;
// Larger fov for larger screens
float fov_factor = 1000;

bool is_running = false;

uint32_t previous_frame_ms = 0;

void setup()
{
    color_buffer = (uint32_t *)malloc(sizeof(uint32_t) * window_width * window_height);

    color_buffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        window_width,
        window_height);

    // load_cube_mesh_data();
    load_obj_file_data("./assets/cube.obj");
}

void process_input(void)
{
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type)
    {
    case SDL_QUIT:
        is_running = false;
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE)
        {
            is_running = false;
        }
        break;
    }
}

vec2_t project(vec3_t point)
{
    vec2_t projected_point = {
        .x = (fov_factor * point.x) / point.z,
        .y = (fov_factor * point.y) / point.z};

    return projected_point;
}

void update(void)
{
    // Release execution back to the CPU until we reach FRAME_TARGET_TIME to stabilise frame rate
    int delay_ms = FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_ms);
    if (delay_ms > 0 && delay_ms <= FRAME_TARGET_TIME)
    {
        SDL_Delay(delay_ms);
    }

    previous_frame_ms = SDL_GetTicks();

    triangles_to_render = NULL;

    mesh.rotation.x += 0.01;
    mesh.rotation.y += 0.01;
    mesh.rotation.z += 0.01;

    int n_faces = array_length(mesh.faces);
    for (int i = 0; i < n_faces; i++)
    {
        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a - 1];
        face_vertices[1] = mesh.vertices[mesh_face.b - 1];
        face_vertices[2] = mesh.vertices[mesh_face.c - 1];


        vec3_t transform_vertices[3];

        // Loop over the vertices of the face
        for (int j = 0; j < 3; j++)
        {
            vec3_t transformed_vertex = face_vertices[j];

            transformed_vertex = vec3_rotate_x(transformed_vertex, mesh.rotation.x);
            transformed_vertex = vec3_rotate_y(transformed_vertex, mesh.rotation.y);
            transformed_vertex = vec3_rotate_z(transformed_vertex, mesh.rotation.z);

            // Translate points away from the camera
            transformed_vertex.z += 5;

            transform_vertices[j] = transformed_vertex;
        }

        vec3_t vector_a = transform_vertices[0];
        vec3_t vector_b = transform_vertices[1];
        vec3_t vector_c = transform_vertices[2];

        vec3_t vecctor_ab = vec3_subtract(vector_b, vector_a);
        vec3_t vecctor_ac = vec3_subtract(vector_c, vector_a);

        vec3_t normal = vec3_cross(vecctor_ab, vecctor_ac);
        vec3_normalize(&normal);

        vec3_t camera_ray = vec3_subtract(camera_position, vector_a);

        float dot_normal_camera = vec3_dot(normal, camera_ray);

        if (dot_normal_camera < 0)
        {
            continue;
        }

        triangle_t projected_triangle;

        for (int j = 0; j < 3; j++)
        {
            vec2_t projected_vertex = project(transform_vertices[j]);

            // Scale and translate the projected vertex to the middle of the screen
            projected_vertex.x += (window_width / 2),
            projected_vertex.y += (window_height / 2),

            projected_triangle.vertices[j] = projected_vertex;
        }

        array_push(triangles_to_render, projected_triangle);
    }
}

void render(void)
{
    draw_grid();

    int num_triangles = array_length(triangles_to_render);
    for (int i = 0; i < num_triangles; i++)
    {
        triangle_t triangle = triangles_to_render[i];
        draw_rect(triangle.vertices[0].x, triangle.vertices[0].y, 3, 3, 0xFFFFFF00);
        draw_rect(triangle.vertices[1].x, triangle.vertices[1].y, 3, 3, 0xFFFFFF00);
        draw_rect(triangle.vertices[3].x, triangle.vertices[3].y, 3, 3, 0xFFFFFF00);

        draw_triangle(
            triangle.vertices[0].x, triangle.vertices[0].y,
            triangle.vertices[1].x, triangle.vertices[1].y,
            triangle.vertices[2].x, triangle.vertices[2].y,
            0xFF00FF00);
    }

    array_free(triangles_to_render);

    render_color_buffer();
    clear_color_buffer(0xFF000000);

    SDL_RenderPresent(renderer);
}

void free_resources(void)
{
    free(color_buffer);
    array_free(mesh.vertices);
    array_free(mesh.faces);
}

int main(void)
{
    is_running = initialize_window();

    setup();

    while (is_running)
    {
        process_input();
        update();
        render();
    }

    destroy_window();
    free_resources();

    return 0;
}
