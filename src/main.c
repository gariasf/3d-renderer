#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "display.h"
#include "upng.h"
#include "vector.h"
#include "mesh.h"
#include "array.h"
#include "matrix.h"
#include "light.h"
#include "texture.h"
#include "triangle.h"
#include "camera.h"

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

enum cull_method {
    CULL_NONE,
    CULL_BACKFACE
} cull_method;

enum render_method {
    RENDER_WIRE,
    RENDER_WIRE_VERTEX,
    RENDER_FILL_TRIANGLE,
    RENDER_FILL_TRIANGLE_WIRE,
    RENDER_TEXTURED,
    RENDER_TEXTURED_WIRE
} render_method;

triangle_t *triangles_to_render = NULL;

mat4_t world_matrix;
mat4_t projection_matrix;
mat4_t view_matrix;

bool is_running = false;

uint32_t previous_frame_ms = 0;

void setup()
{
    render_method = RENDER_WIRE;
    cull_method = CULL_BACKFACE;

    color_buffer = (uint32_t *)malloc(sizeof(uint32_t) * window_width * window_height);

    color_buffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        window_width,
        window_height);

    float fov = M_PI/1.8;
    float aspect = (float)window_height / (float)window_width;
    float znear = 0.1;
    float zfar = 100.0;

    projection_matrix = mat4_make_perspective(
        fov,
        aspect,
        znear,
        zfar
    );

    // load_cube_mesh_data();
    load_obj_file_data("./assets/f22.obj");

    load_png_texture_data("./assets/f22.png");
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
               if (event.key.keysym.sym == SDLK_5)
                   render_method = RENDER_TEXTURED;
               if (event.key.keysym.sym == SDLK_6)
                   render_method = RENDER_TEXTURED_WIRE;
               if (event.key.keysym.sym == SDLK_c)
                   cull_method = CULL_BACKFACE;
               if (event.key.keysym.sym == SDLK_d)
                   cull_method = CULL_NONE;
               break;
       }
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

    mesh.rotation.x -= 0.01;
    mesh.rotation.y -= 0.01;
    mesh.rotation.z -= 0.01;
    //mesh.scale.x += 0.002;
    //mesh.scale.y += 0.001;
    //mesh.translation.x += 0.01;
    mesh.translation.z = 5.0;

    // Change camera position
    camera.position.x += 0.008;
    camera.position.y += 0.008;

    // Create view matrix (looking at a hardcoded target point)
    vec3_t target = {0, 0, 5.0};
    vec3_t up = {0, 1, 0};
    view_matrix = mat4_look_at(
        camera.position,
        target,
        up 
    );


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
        face_vertices[0] = mesh.vertices[mesh_face.a];
        face_vertices[1] = mesh.vertices[mesh_face.b];
        face_vertices[2] = mesh.vertices[mesh_face.c];


        vec4_t transformed_vertices[3];

        // Loop over the vertices of the face
        for (int j = 0; j < 3; j++)
        {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            world_matrix = mat4_identity();
            world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
            world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

            transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);
            
            transformed_vertex = mat4_mul_vec4(view_matrix, transformed_vertex);

            transformed_vertices[j] = transformed_vertex;
        }

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
        vec3_t origin = {0, 0, 0};
        vec3_t camera_ray = vec3_sub(origin, vector_a);

        // Calculate how aligned the camera ray is with the face normal (using dot product)
        float dot_normal_camera = vec3_dot(normal, camera_ray);

        // Backface culling test to see if the current face should be projected
        if (cull_method == CULL_BACKFACE) {
            // Bypass the triangles that are looking away from the camera
            if (dot_normal_camera < 0) {
                continue;
            }
        }

        vec4_t projected_vertices[3];

        // Loop all three vertices to perform projection
        for (int j = 0; j < 3; j++) {
            // Project the current vertex
            projected_vertices[j] = mat4_mul_vec4_project(projection_matrix, transformed_vertices[j]);

            // Scale into the viewport
            projected_vertices[j].x *= (window_width / 2);
            projected_vertices[j].y *= (window_height / 2);

            // Invert y values to account for flipped screen y coords.
            projected_vertices[j].y *= -1;

            // Translate the projected points to the middle of the screen
            projected_vertices[j].x += (window_width / 2);
            projected_vertices[j].y += (window_height / 2);
        }

        float avg_depth = (transformed_vertices[0].z + transformed_vertices[1].z + transformed_vertices[2].z) / 3;

        float light_intensity_factor = -vec3_dot(normal, light.direction);

        uint32_t triangle_color = light_apply_intensity(mesh_face.color, light_intensity_factor);

        triangle_t projected_triangle = {
            .vertices = {
                { projected_vertices[0].x, projected_vertices[0].y, projected_vertices[0].z,  projected_vertices[0].w },
                { projected_vertices[1].x, projected_vertices[1].y, projected_vertices[1].z,  projected_vertices[1].w },
                { projected_vertices[2].x, projected_vertices[2].y, projected_vertices[2].z,  projected_vertices[2].w }
            },
            .texcoords = {
                { mesh_face.a_uv.u, mesh_face.a_uv.v },
                { mesh_face.b_uv.u, mesh_face.b_uv.v },
                { mesh_face.c_uv.u, mesh_face.c_uv.v },
            },
            .color = triangle_color,
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

        if (render_method == RENDER_WIRE || render_method == RENDER_WIRE_VERTEX || render_method == RENDER_FILL_TRIANGLE_WIRE || render_method == RENDER_TEXTURED_WIRE) {
            draw_triangle(
                triangle.vertices[0].x, triangle.vertices[0].y, // vertex A
                triangle.vertices[1].x, triangle.vertices[1].y, // vertex B
                triangle.vertices[2].x, triangle.vertices[2].y, // vertex C
                0xFFFFFFFF
            );
        }

        if(render_method == RENDER_TEXTURED || render_method == RENDER_TEXTURED_WIRE) {
            draw_textured_triangle(
                triangle.vertices[0].x, triangle.vertices[0].y, triangle.vertices[0].z, triangle.vertices[0].w, triangle.texcoords[0].u, triangle.texcoords[0].v, // vertex A
                triangle.vertices[1].x, triangle.vertices[1].y, triangle.vertices[1].z, triangle.vertices[1].w, triangle.texcoords[1].u, triangle.texcoords[1].v, // vertex B
                triangle.vertices[2].x, triangle.vertices[2].y, triangle.vertices[2].z, triangle.vertices[2].w, triangle.texcoords[2].u, triangle.texcoords[2].v, // vertex C
                mesh_texture
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
    upng_free(png_texture);
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
