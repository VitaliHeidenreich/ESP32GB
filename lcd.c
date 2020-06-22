#include "cpu.h"
#include "lcd.h"
#include <stdio.h>

#define LCD_STATUS_MODE         (prog->memory[0xFF41])

// Zustaende der GPU
#define H_BLANK 0
#define V_BLANK 1
#define OAM_RAM 2
#define TRANSPORT 3

#define MAX_GPU_CYCLE_TIKZ      456

#define H_BLANK_TIKZ 204
#define V_BLANK_TIKZ 4560
#define OAM_RAM_TIKZ 80
#define TRANSPORT_TIKZ 172

// LCD Control Register
#define LCD_DISPENA             (prog->memory[0xFF40] &0x80) // BIT 7
#define LCD_TILEMAPSELECT       (prog->memory[0xFF40] &0x40) // BIT 6
#define LCD_WINDOWENA           (prog->memory[0xFF40] &0x20) // BIT 5
#define LCD_BG_DATASELECTION    (prog->memory[0xFF40] &0x10) // BIT 4
#define LCD_BG_MAPSELECTION     (prog->memory[0xFF40] &0x08) // BIT 3
#define LCD_OBJ_SIZE            (prog->memory[0xFF40] &0x04) // BIT 2
#define LCD_OBJ_DISPENA         (prog->memory[0xFF40] &0x02) // BIT 1
#define LCD_BG_ENA              (prog->memory[0xFF40] &0x01) // BIT 0

// LY Coordinates and Compare
#define LY_LINE                 (prog->memory[0xFF44])
#define LY_COMP                 (prog->memory[0xFF45])

// LCD Position and Scrolling
#define SCY                     (prog->memory[0xFF42])
#define SCX                     (prog->memory[0xFF43])

#define WIN_POS_X               (prog->memory[0xFF4B]-7)
#define WIN_POS_Y               (prog->memory[0xFF4A])


// #############################################################################################################
// #############################################################################################################
// #############################################################################################################
void RenderBackground( )
{
    uint16_t tileData = 0;
    uint16_t backgroundMemory = 0;
    uint8_t  signedLocation = 0;
    uint8_t  usingWindow = 0;
    uint16_t xPosStart, yPosStart, tileAddr;

    uint8_t apix, bpix;

    // Pruefen ob Hintergrung aktiviert ist
    if( LCD_BG_ENA )  // BIT 0
    {
        // Erweiterung für Fenster
        if ( LCD_WINDOWENA ) // F
		{ // F
			if (WIN_POS_Y <= SCY) // F
				usingWindow = 1 ; // F
		} // F
		else // F
		{ // F
			usingWindow = 0 ; // F
		} // F


        // which tile data we are using?
		if ( LCD_BG_DATASELECTION ) // BIT 4
        {
            tileData = 0x8000 ; // unsigned data!
            signedLocation = 0;
        }
		else
		{
			tileData = 0x8800 ; // signed data!
			signedLocation = 1 ;
		}


		// which tile map we are using?
		if( usingWindow ) // F
        { // F
            if ( LCD_TILEMAPSELECT ) // F
				backgroundMemory = 0x9C00 ; // F
			else // F
				backgroundMemory = 0x9800 ; // F
        } // F
        else // F
        {
            if ( LCD_BG_MAPSELECTION )  // BIT 3
                backgroundMemory = 0x9C00 ;
            else
                backgroundMemory = 0x9800 ;
        }


        // Herausfuinden welche Tile Nummer durch die Verschiebung der Y-koordinate
        //    ohne der Verschiebung der X-Koordinate  als Start verwendet werden soll.
        if( usingWindow )
            yPosStart = (uint8_t)((LY_LINE-WIN_POS_Y)/8) * 32;
        else
            yPosStart = (uint8_t)((SCY+LY_LINE)/8) * 32;

        for( uint8_t pixel = 0; pixel < 160; pixel ++ )
        {
            // Herausfinden welche Tile Nummer durch die Verschiebung der X-koordinate
            //    ohne der Verschiebung der Y-Koordinate als Start verwendet werden soll.
            if( usingWindow && (pixel >= WIN_POS_X))  // F
                xPosStart = pixel - WIN_POS_X;  // F
            else  // F
                xPosStart = (pixel + SCX);
            xPosStart /= 8;

            // Ablage der Daten herausfinden
            if( signedLocation )
                // (TileNummer + 128) * 16
                tileAddr = tileData+(get_1byteSignedDataFromAddr( backgroundMemory + xPosStart + yPosStart )+128)*16;
            else
                //  TileNummer * 16
                tileAddr = tileData+get_1byteDataFromAddr( backgroundMemory + xPosStart + yPosStart )*16;

            uint16_t line = ((SCY+LY_LINE)%8) * 2;

            apix = get_1byteDataFromAddr(tileAddr + line + 0);
            bpix = get_1byteDataFromAddr(tileAddr + line + 1);

            prog->ausgabeGrafik[ LY_LINE ][ pixel ] = ( ((apix >> (7-(SCX+pixel)%8))&0x1) ) | (((bpix >> (7-(SCX+pixel)%8))&0x1) << 1);
        }
        // Debug
//        for(uint8_t x; x < 160; x++)
//            printf("%d",prog->ausgabeGrafik[ LY_LINE ][ x ]);
//        printf("\n\n");
    }
}

// #############################################################################################################
// #############################################################################################################
// #############################################################################################################
void LCD_control( uint32_t tikz )
{
    static uint32_t lastTikz = 0;
    static uint32_t gpuInitTikz = MAX_GPU_CYCLE_TIKZ;
    uint32_t vergangeneTikz;

    // lastTikz reseten wenn neuer Wert kleiner alter Wert
    if( lastTikz > tikz )
        lastTikz = 0;

    vergangeneTikz = tikz - lastTikz;

    // Runterzaehlen der gpuInitTikz um CPU tikz fuer genaues Timing
    // gpuInitTikz muss nach Ausgabe wieder auf 456 gesetzt werden
    if( gpuInitTikz < vergangeneTikz )
        gpuInitTikz = 0;
    else
        gpuInitTikz -= vergangeneTikz;

    // Speichern der aktuellen Tikz damit beim nächsten Mal die Differenz gebildet werden kann
    lastTikz = tikz;

    // While >>> H-Blank --- 0 --- ##############################################
    if( (LCD_STATUS_MODE&0x03) == H_BLANK)
    {
        debugStates( (LCD_STATUS_MODE&0x03), gpuInitTikz );

        // Check ticks
        if( gpuInitTikz <= 0 )
        {
            LY_LINE ++; // Im TRANSPORT wurde eine Zeile in den Gfx Buffer "transportiert"

            if( LY_LINE > 143 )
            {
                // Alle Zeilen zum Buffer "transportiert"
                // V-Blank  Interrupt Request setzen --> triggern der Ausgabe
                if( prog->IME & ( LCD_STATUS_MODE & (1<<4)) )
                    prog->memory[0xFF0F] |= 0x01;

                setMode( V_BLANK );
                gpuInitTikz = V_BLANK_TIKZ/10; // 4560/10 tikz für V-Blank erwartet ( /10 weil 10x V-Blanks )
            }
            else
            {
                // Noch nicht alle Zeilen "transportiert"
                gpuInitTikz = MAX_GPU_CYCLE_TIKZ; // 456 für einen neuen OAM-Transport-H_Blank Zyklus
                setMode( OAM_RAM );
            }
        }
    }
    // While >>> V-Blank --- 1 --- #################################################
    else if( (LCD_STATUS_MODE&0x03) == V_BLANK)
    {
        debugStates( (LCD_STATUS_MODE&0x03), gpuInitTikz );

        if( LY_LINE == 144 )
            setInterrupt(0);

        // Check ticks
        if( gpuInitTikz <= 0 )
        {
            LY_LINE ++;

            // Ende des Datenpransports in den Buffer --> wieder alles von vorne!!!
            if( LY_LINE > 153 )
            {
                setMode( OAM_RAM );
                gpuInitTikz = MAX_GPU_CYCLE_TIKZ;
                LY_LINE = 0;
            }
            else
            {
                gpuInitTikz = V_BLANK_TIKZ/10;
            }
        }
    }
    // While >>> OAM-RAM --- 2 --- ###################################################
    else if( (LCD_STATUS_MODE&0x03) == OAM_RAM)
    {
        debugStates( (LCD_STATUS_MODE&0x03), gpuInitTikz );

        // Check ticks
        if( gpuInitTikz <= ( MAX_GPU_CYCLE_TIKZ - OAM_RAM_TIKZ ) ) // 456 - 80 = 376
        {
            setMode( TRANSPORT );
        }
    }
    // While >>> TRANSPORT to LCD -- 3 --- ##########################################
    else if( (LCD_STATUS_MODE&0x03) == TRANSPORT)
    {
        debugStates( (LCD_STATUS_MODE&0x03), gpuInitTikz );

        // Check ticks
        if( gpuInitTikz <= ( MAX_GPU_CYCLE_TIKZ - OAM_RAM_TIKZ - TRANSPORT_TIKZ ) ) // 456 - 80 - 172 = 204
        {
            // Eine Zeile Pixel in den Ausgabespeicher schreiben
            RenderBackground( );
            setMode( H_BLANK );
        }
    }
    else
    {
        printf("UNBESTIMMTER ZUSTAND IN DER AUSGABE. ERROR 404.\n");
    }
}

// #############################################################################################################
// #############################################################################################################
// #############################################################################################################
// ----------------------------------------------------------------------------------------------------------------------
// Hilfsfunktionen ------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------
void setMode( uint8_t mode )
{
    LCD_STATUS_MODE &= 0xFC;
    LCD_STATUS_MODE |= mode;
}

// ----------------------------------------------------------------------------------------------------------------------
// DEBUG ----------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------
void debugStates( uint8_t actState, uint32_t gpuTikz )
{
//    static uint8_t lastState = 7;
//    static uint16_t lastTikz = 456;
//
//    if( actState != lastState )
//    {
//        printf("\nLCD-State: ");
//        switch (actState)
//        {
//            case 0: printf("H-Blank   "); break;
//            case 1: printf("V-Blank   ");break;
//            case 2: printf("OAM_RAM   ");break;
//            case 3: printf("TRANSPORT ");break;
//            default: break;
//        }
//        printf("--- LY_LINE = %d und GPU-Tikz = %d (%d)\n", LY_LINE, gpuTikz, lastTikz - gpuTikz);
//        lastState = actState;
//        lastTikz = gpuTikz;
//    }
}







