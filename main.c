#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>

#include <windows.h>
#include <sys\timeb.h>
#include <windows.h>

#include "cpu.h"
#include "display.h"

#define FPS_Rate 60

// it is just an example how to use roms
char* filename = "../games/DrMario.gb";

int main(int argc, char** argv)
{
    prog = gb_start();

    // Pruefen, ob das Spiel vorhanden ist
    if ( gb_program_load( filename ) != 0 )
    {
        free( prog );
        printf("Failed program load!\n");
        return 1;
    }

    display_open(&disp, WIDTH, HEIGHT);

    uint8_t running = 1;
    unsigned int lastTime = 0, currentTime;

    while (running)
    {
        // Print a report once per second
        currentTime = SDL_GetTicks();

        if (currentTime > ( lastTime + 1000/60 ) )
        {
            lastTime = currentTime;
            gb_program_cycle( );
            prog->tikz = 0;
            // read keys
            running = read_keys( );
        }
    }

    display_close( &disp );

    return 0;
}
