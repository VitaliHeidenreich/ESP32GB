#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>

#include <windows.h>
#include <sys\timeb.h>

#include "cpu.h"
#include "display.h"

#define MAIN_GEBUG 0

#define FPS_Rate 60

// it is just an example how to use roms
char* filename = "../games/Tetris.gb";
SDL_Event ev;

int main(int argc, char** argv)
{
    #if MAIN_GEBUG
        fDebug = fopen("games/DebugOut.txt", "w");
        if (fDebug == NULL)
        {
            printf("Error opening file!\n");
            exit(1);
        }
    #endif // MAIN_GEBUG


    prog = gb_start();

    // Pruefen, ob das Spiel vorhanden ist
    if ( gb_program_load( filename ) != 0 )
    {
        free( prog );
        printf("Failed program load!\n");
        return 1;
    }

    display_open(&disp, WIDTH, HEIGHT);

    int running = 1;
    unsigned int lastTime = 0, currentTime;

    // Test
    uint16_t val = 0;
    uint16_t DebugAddr = 0x2F0;
    val = 0x2F0 - 10;
    while( val < (DebugAddr + 10) )
    {
        if(val == DebugAddr)
            printf(">>> ");
        else
            printf("    ");
        printf("0x%04X: 0x%02X\n",val,prog->memory[ val ]);
        val ++;
    }

    while (running)
    {
        // Print a report once per second
        currentTime = SDL_GetTicks();

        if (currentTime > ( lastTime + 1000/60 ) )
        {
            lastTime = currentTime;

            gb_program_cycle( );
            //gb_ShowScreen( );
            prog->tikz = 0;
        }

        while ( SDL_PollEvent( &ev ) )
        {
            switch (ev.type)
            {
                case SDL_QUIT:
                    printf("\nSDL is closing. Please wait!\n");
                    running = 0;
                    break;
                case SDL_KEYUP: // Refresh also on KeyUp
                case SDL_KEYDOWN:
                    prog->keys = read_keys( ev );
                    break;
                default:
                    break;
            }
        }
    }

    display_close( &disp );

    return 0;
}
