#ifndef LCD_H
#define LCD_H

#pragma once
#include "lcd.h"

void LCD_control( uint32_t tikz );
//void getTile( uint16_t addr, uint16_t tileNum, gameboy *prog );
void RenderBackground( );
void RenderSprites();


// Hilfsfunktionen
void setMode( uint8_t mode );
void debugStates( uint8_t actState, uint32_t tikz );

#endif
