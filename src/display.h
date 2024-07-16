#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>

extern SDL_Window *window;
extern SDL_Renderer *renderer;

extern uint32_t *color_buffer;
extern SDL_Texture *color_buffer_texture;

extern int window_width;
extern int window_height;

bool initialize_window(void);
void destroy_window(void);

void render_color_buffer(void);
void clear_color_buffer(uint32_t color);

void draw_grid(void);
void draw_rect(int x, int y, int width, int height, uint32_t color);