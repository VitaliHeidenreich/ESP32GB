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
                SDL_SetRenderDrawColor( disp->renderer, 255,255,255,255);
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
uint8_t read_keys( )
{
    static uint8_t iRet = 0xFF;
    static uint16_t lastState = 0;

    SDL_Event ev;

    while( SDL_PollEvent( &ev ) )
		{
			switch (ev.type)
            {
                case SDL_QUIT:
                    printf("\nSDL is closing. Please wait!\n");
                    return 0;
                    break;

                case SDL_KEYUP:
                    switch(ev.key.keysym.sym)
                    {
                    case SDLK_a:
                        iRet |=  (1<<1);
                        break;
                    case SDLK_s:
                        iRet |=  (1<<3);
                        break;
                    case SDLK_w:
                        iRet |=  (1<<2);
                        break;
                    case SDLK_d:
                        iRet |=  (1<<0);
                        break;

                    case SDLK_g:
                        iRet |=  (1<<4);
                        break;
                    case SDLK_h:
                        iRet |=  (1<<5);
                        break;
                    case SDLK_j:
                        iRet |=  (1<<6);
                        break;
                    case SDLK_k:
                        iRet |=  (1<<7);
                        break;
                    }
                    break;

                case SDL_KEYDOWN:
                    switch(ev.key.keysym.sym)
                    {
                    case SDLK_a:
                        iRet &= ~(1<<1);
                        break;
                    case SDLK_s:
                        iRet &= ~(1<<3);
                        break;
                    case SDLK_w:
                        iRet &= ~(1<<2);
                        break;
                    case SDLK_d:
                        iRet &=  ~(1<<0);
                        break;

                    case SDLK_g:
                        iRet &=  ~(1<<4);
                        break;
                    case SDLK_h:
                        iRet &=  ~(1<<5);
                        break;
                    case SDLK_j:
                        iRet &=  ~(1<<6);
                        break;
                    case SDLK_k:
                        iRet &=  ~(1<<7);
                        break;
                    }
                    break;

                default:
                    break;
            }
		}
		//printf("Button Val: ");printBinary(iRet);
    prog->keys = iRet;

    if( prog->keys != lastState )
    {
        //JOYPAD_IR_SET
        prog->memory[0xFF0F] |= (1<<4);
        lastState = prog->keys;
    }

    return 1;
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

