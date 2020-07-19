
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <SDL2/SDL.h>
#include "cpu.h"
#include "main.h"

#define PIXEL_SIZE 4


#define WIDTH         160
#define HEIGHT        144

struct display_t {
    SDL_Renderer* renderer;
    SDL_Window *window;
    uint8_t width, height;
} display_t;

typedef struct display_t display;

display disp;

struct rgb {
		uint8_t r, g, b;
};


void display_open(display *disp, uint8_t width, uint8_t height);
void display_close( display* disp );
uint8_t read_keys( );


void gb_ShowScreen( );

#endif

