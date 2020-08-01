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

/// Farbenzuweisung
uint8_t getGrayShadeForBG( uint8_t myColor )
{
    return ((prog->memory[0xFF47] >> (myColor*2)) &0x03 );
}

/****************************************************************************************************************
*  Render Hintergrund (statische Sachen)
****************************************************************************************************************/
void RenderBackground( )
{
    uint16_t tileData = 0;
    uint16_t backgroundMemory = 0;
    uint8_t  signedLocation = 0;
    uint8_t  usingWindow = 0;
    uint16_t xPosStart, yPosStart, tileAddr;
    uint8_t myColor = 0;

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

            myColor = ( ((apix >> (7-(SCX+pixel)%8))&0x1) ) | (((bpix >> (7-(SCX+pixel)%8))&0x1) << 1);

            prog->ausgabeGrafik[ LY_LINE ][ pixel ] = getGrayShadeForBG( myColor );
        }
    }
}

/****************************************************************************************************************
*  Render Sprites (bewegte Sachen)
****************************************************************************************************************/
void RenderSprites()
{
    int16_t yPos, xPos;
    uint16_t tLoc, attr, ySize;
    uint8_t myPixelColor = 0;

    uint8_t spriteLine, apix, bpix;

    // Sprites aktiviert?
    if( LCD_OBJ_DISPENA )
    {
        for(uint8_t sprite = 0; sprite < 40; sprite ++)
        {
            // Welche Spritegroeße wird verwendet?
            if( LCD_OBJ_SIZE )
            {
                ySize = 16;
            }
            else
            {
                ySize =  8;
            }
            // Einlesen der Eigenschaften
            yPos = prog->memory[ 0xFE00 + sprite * 4 + 0 ] - 16;
            xPos = prog->memory[ 0xFE00 + sprite * 4 + 1 ] -  8;
            tLoc = prog->memory[ 0xFE00 + sprite * 4 + 2 ];
            attr = prog->memory[ 0xFE00 + sprite * 4 + 3 ];

            // Testen ob die Zeile betroffen ist, falls nein: "dont care!"
            if (( LY_LINE >= yPos ) && ( LY_LINE < ( yPos + ySize )))
            {
                spriteLine = LY_LINE - yPos;

                // Spiegelung in y-Richtung
                if ( (attr & 0b01000000) )
                    spriteLine = ySize - spriteLine;

 				apix = prog->memory[ 0x8000 + (tLoc << 4) + spriteLine*2 + 0 ] ;
 				bpix = prog->memory[ 0x8000 + (tLoc << 4) + spriteLine*2 + 1 ] ;

                for ( int tilePixel = 0; tilePixel <= 7; tilePixel++ )
                {
					// pruefen ob in x Richtung gespiegelt werden soll
 					if ( (attr & 0b00100000) )
 						myPixelColor = ( ((apix >>    tilePixel )&0x1) ) | (((bpix >>   tilePixel )&0x1) << 1);
                    else
                        myPixelColor = ( ((apix >> (7-tilePixel))&0x1) ) | (((bpix >> (7-tilePixel))&0x1) << 1);

 					// ist die Farbe transparent? Falls nein, dann schreibe Pixel
                    if (!(myPixelColor == 0))
                    {
                        if( (LY_LINE <= 143) && ((xPos + tilePixel) <= 159) )
                        {
                            prog->ausgabeGrafik[ LY_LINE ][ xPos + tilePixel ] = myPixelColor;
                        }
                    }
                }
            }
        }
    }
}

/****************************************************************************************************************
*  Umschaltlogik zum Switchen zwischen den Modus fuer Displayausgabe
****************************************************************************************************************/
void LCD_control( uint32_t tikz )
{
    static uint32_t lastTikz = 0;
    static int gpuInitTikz = MAX_GPU_CYCLE_TIKZ;  // uint32_t ???
    uint32_t vergangeneTikz;
    // Erweiterung
    static uint8_t H_BLANK_ONCE = 0;

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
        // Nicht sicher!!!
        if(!H_BLANK_ONCE)
        {
            H_BLANK_ONCE = 1;
            setInterrupt(1);
        }

        // Check ticks
        if( gpuInitTikz <= 0 )
        {
            LY_LINE ++; // Im TRANSPORT wurde eine Zeile in den Gfx Buffer "transportiert"

            if( LY_LINE > 143 )
            {
                // Eintritt in den V-Blank --> alle Zeilen 1x durchgegangen
                setMode( V_BLANK );
                gpuInitTikz = V_BLANK_TIKZ/10; // 4560/10 tikz für V-Blank erwartet ( /10 weil 10x V-Blanks )

                H_BLANK_ONCE = 0;
            }
            else
            {
                // Noch nicht alle Zeilen "transportiert"
                gpuInitTikz = MAX_GPU_CYCLE_TIKZ; // 456 für einen neuen OAM-Transport-H_Blank Zyklus
                setMode( OAM_RAM );

                H_BLANK_ONCE = 0;
            }
        }
    }
    // While >>> V-Blank --- 1 --- #################################################
    else if( (LCD_STATUS_MODE&0x03) == V_BLANK)
    {
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
        // Check ticks
        if( gpuInitTikz <= ( MAX_GPU_CYCLE_TIKZ - OAM_RAM_TIKZ ) ) // 456 - 80 = 376
        {
            setMode( TRANSPORT );
        }
    }
    // While >>> TRANSPORT to LCD -- 3 --- ##########################################
    else if( (LCD_STATUS_MODE&0x03) == TRANSPORT)
    {
        // Check ticks
        if( gpuInitTikz <= ( MAX_GPU_CYCLE_TIKZ - OAM_RAM_TIKZ - TRANSPORT_TIKZ ) ) // 456 - 80 - 172 = 204
        {
            // Eine Zeile Pixel in den Ausgabespeicher schreiben
            RenderBackground( );
            RenderSprites();
            setMode( H_BLANK );
        }
    }
    else
    {
        printf("UNBESTIMMTER ZUSTAND IN DER AUSGABE. ERROR 404.\n");
        while(1){}
    }
}

// #######################################################################################################################
/// ----------------------------------------------------------------------------------------------------------------------
/// Hilfsfunktionen ------------------------------------------------------------------------------------------------------
/// ----------------------------------------------------------------------------------------------------------------------
void setMode( uint8_t mode )
{
    LCD_STATUS_MODE &= 0xFC;
    LCD_STATUS_MODE |= mode;
}

/// ENDE -----------------------------------------------------------------------------------------------------------------







