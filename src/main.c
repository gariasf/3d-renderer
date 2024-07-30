#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "display.h"
#include "vector.h"
#include "mesh.h"
#include "array.h"
#include "matrix.h"

enum cull_method {
    CULL_NONE,
    CULL_BACKFACE
} cull_method;

enum render_method {
    RENDER_WIRE,
    RENDER_WIRE_VERTEX,
    RENDER_FILL_TRIANGLE,
    RENDER_FILL_TRIANGLE_WIRE
} render_method;

triangle_t *triangles_to_render = NULL;

vec3_t camera_position = { .x = 0, .y = 0, .z = 0 };

// float fov_factor = 640;
// Larger fov for larger screens
float fov_factor = 1000;

bool is_running = false;

uint32_t previous_frame_ms = 0;

void setup()
{
    render_method = RENDER_WIRE;
    cull_method = CULL_BACKFACE;

    color_buffer = (uint32_t *)malloc(sizeof(uint32_t) * window_width * window_height);

    color_buffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        window_width,
        window_height);

    load_cube_mesh_data();
    // load_obj_file_data("./assets/cube.obj");
}

void process_input(void)
{
    SDL_Event event;
       SDL_PollEvent(&event);
       switch (event.type) {
           case SDL_QUIT:
               is_running = false;
               break;
           case SDL_KEYDOWN:
               if (event.key.keysym.sym == SDLK_ESCAPE)
                   is_running = false;
               if (event.key.keysym.sym == SDLK_1)
                   render_method = RENDER_WIRE_VERTEX;
               if (event.key.keysym.sym == SDLK_2)
                   render_method = RENDER_WIRE;
               if (event.key.keysym.sym == SDLK_3)
                   render_method = RENDER_FILL_TRIANGLE;
               if (event.key.keysym.sym == SDLK_4)
                   render_method = RENDER_FILL_TRIANGLE_WIRE;
               if (event.key.keysym.sym == SDLK_c)
                   cull_method = CULL_BACKFACE;
               if (event.key.keysym.sym == SDLK_d)
                   cull_method = CULL_NONE;
               break;
       }
}

vec2_t project(vec3_t point)
{
    vec2_t projected_point = {
        .x = (fov_factor * point.x) / point.z,
        .y = (fov_factor * point.y) / point.z
    };

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
    mesh.scale.x += 0.002;
    mesh.scale.y += 0.001;
    mesh.translation.x += 0.01;
    mesh.translation.z = 5.0;

    mat4_t scale_matrix = mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.y);
    mat4_t translation_matrix = mat4_make_translation(mesh.translation.x, mesh.translation.y, mesh.translation.z);
    mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
    mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
    mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);
    int n_faces = array_length(mesh.faces);
    for (int i = 0; i < n_faces; i++)
    {
        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a - 1];
        face_vertices[1] = mesh.vertices[mesh_face.b - 1];
        face_vertices[2] = mesh.vertices[mesh_face.c - 1];


        vec4_t transformed_vertices[3];

        // Loop over the vertices of the face
        for (int j = 0; j < 3; j++)
        {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            mat4_t world_matrix = mat4_identity();
            world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
            world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

            transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);

            transformed_vertices[j] = transformed_vertex;
        }

        // Backface culling test to see if the current face should be projected
        if (cull_method == CULL_BACKFACE) {
            vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /*   A   */
            vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]); /*  / \  */
            vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]); /* C---B */

            vec3_t vector_ab = vec3_sub(vector_b, vector_a);
            vec3_t vector_ac = vec3_sub(vector_c, vector_a);
            vec3_normalize(&vector_ab);
            vec3_normalize(&vector_ac);

            // Compute the face normal (using cross product to find perpendicular)
            vec3_t normal = vec3_cross(vector_ab, vector_ac);
            vec3_normalize(&normal);

            // Find the vector between vertex A in the triangle and the camera origin
            vec3_t camera_ray = vec3_sub(camera_position, vector_a);

            // Calculate how aligned the camera ray is with the face normal (using dot product)
            float dot_normal_camera = vec3_dot(normal, camera_ray);

            // Bypass the triangles that are looking away from the camera
            if (dot_normal_camera < 0) {
                continue;
            }
        }

        vec2_t projected_vertices[3];

        // Loop all three vertices to perform projection
        for (int j = 0; j < 3; j++) {
            // Project the current vertex
            projected_vertices[j] = project(vec3_from_vec4(transformed_vertices[j]));

            // Scale and translate the projected points to the middle of the screen
            projected_vertices[j].x += (window_width / 2);
            projected_vertices[j].y += (window_height / 2);

        }

        float avg_depth = (transformed_vertices[0].z + transformed_vertices[1].z + transformed_vertices[2].z) / 3;

        triangle_t projected_triangle = {
            .vertices = {
                {projected_vertices[0].x, projected_vertices[0].y},
                {projected_vertices[1].x, projected_vertices[1].y},
                {projected_vertices[2].x, projected_vertices[2].y}
            },
            .color = mesh_face.color,
            .avg_depth = avg_depth
        };

        array_push(triangles_to_render, projected_triangle);
    }

    int num_triangles = array_length(triangles_to_render);

    for (int i = 0; i < num_triangles; i++) {
        for (int j = i; j < num_triangles; j++) {
            if(triangles_to_render[i].avg_depth < triangles_to_render[j].avg_depth) {
                triangle_t temp = triangles_to_render[i];
                triangles_to_render[i] = triangles_to_render[j];
                triangles_to_render[j] = temp;
            }
        }
    }
}

void render(void)
{
    SDL_RenderClear(renderer);

    draw_grid();

    int num_triangles = array_length(triangles_to_render);
    for (int i = 0; i < num_triangles; i++)
    {
       triangle_t triangle = triangles_to_render[i];

        if (render_method == RENDER_FILL_TRIANGLE || render_method == RENDER_FILL_TRIANGLE_WIRE) {
            draw_filled_triangle(
                triangle.vertices[0].x, triangle.vertices[0].y, // vertex A
                triangle.vertices[1].x, triangle.vertices[1].y, // vertex B
                triangle.vertices[2].x, triangle.vertices[2].y, // vertex C
                triangle.color
            );
        }

        if (render_method == RENDER_WIRE || render_method == RENDER_WIRE_VERTEX || render_method == RENDER_FILL_TRIANGLE_WIRE) {
            draw_triangle(
                triangle.vertices[0].x, triangle.vertices[0].y, // vertex A
                triangle.vertices[1].x, triangle.vertices[1].y, // vertex B
                triangle.vertices[2].x, triangle.vertices[2].y, // vertex C
                0xFFFFFFFF
            );
        }

        if (render_method == RENDER_WIRE_VERTEX) {
            draw_rect(triangle.vertices[0].x - 3, triangle.vertices[0].y - 3, 6, 6, 0xFFFF0000); // vertex A
            draw_rect(triangle.vertices[1].x - 3, triangle.vertices[1].y - 3, 6, 6, 0xFFFF0000); // vertex B
            draw_rect(triangle.vertices[2].x - 3, triangle.vertices[2].y - 3, 6, 6, 0xFFFF0000); // vertex C
        }
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
