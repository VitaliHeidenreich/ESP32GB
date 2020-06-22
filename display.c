#include <SDL2/SDL.h>
#include "display.h"
#include <stdlib.h>
#include <stdio.h>




void display_open(display *disp, uint8_t width, uint8_t height)
{
    printf("Initialisierung des Renderers gestartet!\n");

    disp->width = width, disp->height = height;

    // Nur Videoausgabe soll verwendet werden.
    if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
    {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
    }
    else
    {
        disp->window = SDL_CreateWindow("Gameboy",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     width*PIXEL_SIZE,
                                     height*PIXEL_SIZE,
                                     SDL_WINDOW_SHOWN);


        if ( disp->window == NULL )
        {
            printf("SDL_Create Error: %s\n", SDL_GetError());
        }
        else
        {
            disp->renderer = SDL_CreateRenderer(disp->window, -1,
                                                    SDL_RENDERER_ACCELERATED);

            if ( disp->renderer ==  NULL )
            {
                SDL_DestroyWindow(disp->window);
                printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
            }
            else
            {
                SDL_SetRenderDrawColor( disp->renderer, 0,0,0,255);
                SDL_RenderClear( disp->renderer );
                printf("Renderer erfolgreich angelegt!\n");
            }

        }

    }

}

// Freigeben der Display Resourcen
void display_close( display* disp )
{
    free(disp);
    SDL_Quit( );
}

// Einlesen der Tasten über SDL2
uint8_t read_keys( SDL_Event ev )
{
    uint8_t button_codes[ ] = {
            SDL_SCANCODE_A, SDL_SCANCODE_W, SDL_SCANCODE_D, SDL_SCANCODE_S, // A W D S
            SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_J, SDL_SCANCODE_K, // G H J K
    };

    uint16_t iRet = 0;

    for( int i = 0; i < sizeof(button_codes) / sizeof(uint8_t); i++ )
    {
        if( ev.key.keysym.scancode == button_codes[ i ] )
        {
            if(ev.type == SDL_KEYDOWN)
            {
                iRet = iRet | ( 1 << i );
            }
        }
    }
    return iRet;
}

// Anpasten der gelesenen Tasten an das Gameboy Layout
void convert_keys(gameboy* prog, uint8_t key_byte )
{
    // do something
}

uint16_t GBC_colors[ 4 ] = {10,70,140,200};

// Ausgabe an die Anzeige
void gb_ShowScreen( )
{
    //printf("Print!\n");
    // display disp;
    SDL_Rect srcrect;
    SDL_RenderClear( disp.renderer );

    uint8_t color;

    for (uint8_t y=0; y < 144; y++)
    {
        for (uint8_t x=0; x < 160; x++)
        {
            srcrect.x = x*PIXEL_SIZE;
            srcrect.y = y*PIXEL_SIZE;
            srcrect.w = (x+1)*PIXEL_SIZE;
            srcrect.h = (y+1)*PIXEL_SIZE;

            color = GBC_colors[ prog->ausgabeGrafik[y][x] ];
            SDL_SetRenderDrawColor( disp.renderer, color,color,color,200);
            SDL_RenderFillRect(disp.renderer, &srcrect);
        }
    }

    SDL_RenderPresent( disp.renderer );
}

