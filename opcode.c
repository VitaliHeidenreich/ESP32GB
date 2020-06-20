/**
*  Author: Vitali Heidenreich
*  Sources used: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM [11.03.2020]
*                http://devernay.free.fr/hacks/chip8/chip8def.htm
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "cpu.h"
#include "main.h"
// only for debugger
#include <windows.h>

// Flagsposition
#define ZERO        0b10000000
#define NEGATIVE    0b01000000
#define HALFCARRY   0b00100000
#define CARRY       0b00010000

// Zum Einstellen der Flags
#define UNMOD       3
#define NOT         2
#define SET         1
#define UNSET       0

// Lets debug!
#define DEBUG_OPM   0
#define DEBUG_STEP  0

// Zyklen
const unsigned char InstructionTicks[256] = {
	2, 6, 4, 4, 2, 2, 4, 4, 10, 4, 4, 4, 2, 2, 4, 4, // 0x0_
	2, 6, 4, 4, 2, 2, 4, 4,  4, 4, 4, 4, 2, 2, 4, 4, // 0x1_
	0, 6, 4, 4, 2, 2, 4, 2,  0, 4, 4, 4, 2, 2, 4, 2, // 0x2_
	4, 6, 4, 4, 6, 6, 6, 2,  0, 4, 4, 4, 2, 2, 4, 2, // 0x3_
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x4_
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x5_
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x6_
	4, 4, 4, 4, 4, 4, 2, 4,  2, 2, 2, 2, 2, 2, 4, 2, // 0x7_
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x8_
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x9_
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0xa_
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0xb_
	0, 6, 0, 6, 0, 8, 4, 8,  0, 2, 0, 0, 0, 6, 4, 8, // 0xc_
	0, 6, 0, 0, 0, 8, 4, 8,  0, 8, 0, 0, 0, 0, 4, 8, // 0xd_
	6, 6, 4, 0, 0, 8, 4, 8,  8, 2, 8, 0, 0, 0, 4, 8, // 0xe_
	6, 6, 4, 2, 0, 8, 4, 8,  6, 4, 8, 2, 0, 0, 4, 8  // 0xf_
};


const unsigned char InstructionTicks_0xCB[256] = {
	8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x0_
	8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x1_
	8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x2_
	8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x3_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x4_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x5_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x6_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x7_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x8_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x9_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xa_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xb_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xc_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xd_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xe_
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8  // 0xf_
};


/*Wait for a key press*/
uint8_t status_key_press( uint8_t* pressed_key )
{
    uint8_t iRet = 0;

    for( int i = 0; i < 0xf; i ++ )
    {
        if( prog->keys[ i ] )
        {
            iRet = 1;
            *pressed_key = i;
        }
    }
    return iRet;
}


void check_interrupts(  )
{
  // Interrupts
  if (prog->IME == 0)
    return;
}


void gb_opcode_exec( )
{
    #if DEBUG_STEP
        char myChar;
        static uint32_t stepNum = 0;
        static uint16_t stopOnPC = 0x022f;
    #endif // DEBUG_STEP
    uint16_t myVal = 0;
    int mySignVal = 0;

    // Dauer des auszuführenden Codes in Zyklen
    if(prog->opcode != 0xCB)
        prog->tikz += InstructionTicks[ prog->opcode ];

    // Warten auf Event (Eingabe, Interrupt)
    if( prog->halt )
    {
        //printf("0x10! Programm gestoppt, Eingabe erforderlich!\n");
        return;
    }

    // CPU Anweisungen
    switch( prog->opcode )
    {
    case 0x00:
        // nop
        INCREMENT( 1 );
        break;

    case 0x01:
        // LD BC,u16
        REG_BC = get_2byteData(  );
        INCREMENT( 3 );
        break;

    case 0x02:
        // LD (BC),A
        write_1byteData( REG_BC, REG_A );
        INCREMENT( 1 );
        break;

    case 0x03:
        // INC BC
        REG_BC ++;
        INCREMENT( 1 );
        break;
    case 0x04:
        // INC B
        setFlags_for_Inc_1Byte( REG_B );
        REG_B ++;
        INCREMENT( 1 );
        break;

    case 0x05:
        // DEC B
        setFlags_for_Dec_1Byte( REG_B );
        REG_B --;
        INCREMENT( 1 );
        break;

    case 0x06:
        // LD B,u8
        REG_B = get_1byteData(  );
        INCREMENT( 2 );
        break;

    case 0x07:
        // RLCA
        RLCA(&REG_A );
        INCREMENT( 1 );
        break;

    case 0x08:
        // LD (u16),SP
        write_2byteData( get_2byteData( ), SP );
        INCREMENT( 3 );
        break;

    case 0x09:
        // ADD HL,BC
        setFlags_for_Add_2Byte( REG_HL, REG_BC );
        REG_HL = REG_HL + REG_BC;
        INCREMENT( 1 );
        break;

    case 0x0A:
        // LD A,(BC)
        REG_A = get_1byteDataFromAddr( REG_BC );
        INCREMENT( 1 );
        break;

    case 0x0B:
        // DEC BC
        REG_BC --;
        INCREMENT( 1 );
        break;

    case 0x0C:
        // INC C
        setFlags_for_Inc_1Byte(  REG_C );
        REG_C ++;
        INCREMENT( 1 );
        break;

    case 0x0D:
        //DEC C
        setFlags_for_Dec_1Byte(  REG_C );
        REG_C --;
        INCREMENT( 1 );
        break;

    case 0x0E:
        // LD C,u8
        REG_C = get_1byteData(  );
        INCREMENT( 2 );
        break;

    case 0x0F:
        // RRCA
        RRCA(&REG_A );
        INCREMENT( 1 );
        break;

    case 0x10:
        // STOP
        prog->stop = 1;
        INCREMENT( 2 );
        break;

    case 0x11:
        // LD DE,u16
        REG_DE = get_2byteData(  );
        INCREMENT( 3 );
        break;

    case 0x12:
        // LD (DE),A
        write_1byteData( REG_DE, REG_A);
        INCREMENT( 1 );
        break;

    case 0x13:
        // INC DE
        REG_DE ++;
        INCREMENT( 1 );
        break;

    case 0x14:
        // INC D
        setFlags_for_Inc_1Byte( REG_D );
        REG_D ++;
        INCREMENT( 1 );
        break;

    case 0x15:
        // DEC D
        setFlags_for_Dec_1Byte(  REG_D );
        REG_D --;
        INCREMENT( 1 );
        break;

    case 0x16:
        // LD D,u8
        REG_D = get_1byteData(  );
        INCREMENT( 2 );
        break;

    case 0x17:
        // RLA
        RLA(&REG_A );
        INCREMENT( 1 );
        break;

    case 0x18:
        // JR i8
        mySignVal = get_1byteSignedData();
        INCREMENT( 2 );
        PC = PC + mySignVal;
        break;

    case 0x19:
        // ADD HL,DE !!!
        setFlags_for_Add_2Byte( REG_HL, REG_DE );
        REG_HL = REG_HL + REG_DE;
        INCREMENT( 1 );
        break;

    case 0x1A:
        // LD A,(DE)
        REG_A = get_1byteDataFromAddr( REG_DE );
        INCREMENT( 1 );
        break;

    case 0x1B:
        // DEC DE
        REG_DE --;
        INCREMENT( 1 );
        break;

    case 0x1C:
        // INC E
        setFlags_for_Inc_1Byte( REG_E );
        REG_E ++;
        INCREMENT( 1 );
        break;

    case 0x1D:
        // DEC E
        setFlags_for_Dec_1Byte( REG_E );
        REG_E --;
        INCREMENT( 1 );
        break;

    case 0x1E:
        // LD E, u8
        REG_E = get_1byteData( );
        INCREMENT( 2 );
        break;

    case 0x1F: // OK
        // RRA
        RRA(&REG_A );
        INCREMENT(1);
        break;

    case 0x20:
        // JR NZ,i8 !!!
        mySignVal = get_1byteSignedData();
        INCREMENT( 2 );
        if( !getZeroFlag( ) )
            PC = PC + mySignVal;
        break;

    case 0x21:
        // LD HL,u16
        REG_HL = get_2byteData( );
        INCREMENT( 3 );
        break;

    case 0x22: // !!!
        // LD (HL+),A
        write_1byteData( REG_HL, REG_A );
        REG_HL ++;
        INCREMENT( 1 );
        break;

    case 0x23:
        // INC HL
        REG_HL ++;
        INCREMENT( 1 );
        break;

    case 0x24:
        // INC H
        setFlags_for_Inc_1Byte( REG_H );
        REG_H ++;
        INCREMENT( 1 );
        break;

    case 0x25:
        // DEC H
        setFlags_for_Dec_1Byte( REG_H );
        REG_H --;
        INCREMENT( 1 );
        break;

    case 0x26:
        // LD H,u8
        REG_H = get_1byteData(  );
        INCREMENT( 2 );
        break;

    case 0x27:
        // DAA !!!
        do_DAA(  );
        INCREMENT( 1 );
        break;

    case 0x28:
        // JR Z,i8
        mySignVal = get_1byteSignedData( );
        INCREMENT( 2 );
        if( getZeroFlag( ) )
            PC = PC + mySignVal;
        break;

    case 0x29:
        // ADD HL,HL
        setFlags_for_Add_2Byte( REG_HL, REG_HL );
        REG_HL = REG_HL + REG_HL;
        INCREMENT( 1 );
        break;

    case 0x2A:
        // LD A,(HL+)
        REG_A = get_1byteDataFromAddr( REG_HL );
        REG_HL++;
        INCREMENT( 1 );
        break;

    case 0x2B:
        // DEC HL
        REG_HL --;
        INCREMENT( 1 );
        break;

    case 0x2C:
        // INC L
        setFlags_for_Inc_1Byte( REG_L );
        REG_L ++;
        INCREMENT( 1 );
        break;

    case 0x2D:
        // DEC L
        setFlags_for_Dec_1Byte(  REG_L );
        REG_L --;
        INCREMENT( 1 );
        break;

    case 0x2E:
        // LD L,u8
        REG_L = get_1byteData(  );
        INCREMENT( 2 );
        break;

    case 0x2F:
        // CPL
        REG_A=~REG_A;
        setFlags(UNMOD,SET,SET,UNMOD );
        INCREMENT( 1 );
        break;

    case 0x30:
        // JR NC,i8
        mySignVal = get_1byteSignedData();
        INCREMENT( 2 );
        if( !getCarryFlag( ) )
            PC = PC + mySignVal;
        break;

    case 0x31:
        // LD SP,u16
        SP = get_2byteData(  );
        INCREMENT( 3 );
        break;

    case 0x32:
        // LD (HL-),A
        write_1byteData( REG_HL, REG_A);
        REG_HL--;
        INCREMENT( 1 );
        break;

    case 0x33:
        // INC HL
        SP ++;
        INCREMENT( 1 );
        break;

    case 0x34:
        // INC H
        setFlags_for_Inc_1Byte( prog->memory[ REG_HL ] );
        prog->memory[ REG_HL ] ++;
        INCREMENT( 1 );
        break;

    case 0x35:
        // INC H
        setFlags_for_Dec_1Byte( prog->memory[ REG_HL ] );
        prog->memory[ REG_HL ] --;
        INCREMENT( 1 );
        break;

    case 0x36:
        // LD (HL),u8
        write_1byteData(  REG_HL, get_1byteData(  ));
        INCREMENT( 2 );
        break;

    case 0x37:
        // SCF
        setFlags(UNMOD,UNSET,UNSET,SET );
        INCREMENT( 1 );
        break;

    case 0x38:
        // JR C,i8 !!!
        mySignVal = get_1byteSignedData();
        INCREMENT( 2 );
        if( getCarryFlag( ) )
            PC = PC + mySignVal;
        break;

    case 0x39:
        // ADD HL,SP
        setFlags_for_Add_2Byte( REG_HL,SP);
        REG_HL = REG_HL + prog->sp;
        INCREMENT( 1 );
        break;

    case 0x3A:
        // LD A,(HL-)
        REG_A = get_1byteDataFromAddr( REG_HL );
        REG_HL--;
        INCREMENT( 1 );
        break;

    case 0x3B:
        // DEC SP
        SP --;
        INCREMENT( 1 );
        break;

    case 0x3C:
        // INC A
        setFlags_for_Inc_1Byte( REG_A );
        REG_A ++;
        INCREMENT( 1 );
        break;

    case 0x3D:
        // DEC A
        setFlags_for_Dec_1Byte( REG_A );
        REG_A --;
        INCREMENT( 1 );
        break;

    case 0x3E:
        // LD A,u8
        REG_A = get_1byteData( );
        INCREMENT( 2 );
        break;

    case 0x3F:
        // CCF Flips the carry flag, and clears the N and H flags
        setFlags( UNMOD, UNSET, UNSET, NOT );
        INCREMENT( 1 );
        break;

    case 0x40:
        // LD B,B
        REG_B = REG_B;
        INCREMENT( 1 );
        break;

    case 0x41:
        // LD B,C
        REG_B = REG_C;
        INCREMENT( 1 );
        break;

    case 0x42:
        // LD B,D
        REG_B = REG_D;
        INCREMENT( 1 );
        break;

    case 0x43:
        // LD B,E
        REG_B = REG_E;
        INCREMENT( 1 );
        break;

    case 0x44:
        // LD B,H
        REG_B = REG_H;
        INCREMENT( 1 );
        break;

    case 0x45:
        // LD B,L
        REG_B = REG_L;
        INCREMENT( 1 );
        break;

    case 0x46:
        // LD B,(HL)
        REG_B = get_1byteDataFromAddr( REG_HL );
        INCREMENT( 1 );
        break;

    case 0x47:
        // LD B,Ap->memory
        REG_B = REG_A;
        INCREMENT( 1 );
        break;

    case 0x48:
        // LD C,B
        REG_C = REG_B;
        INCREMENT( 1 );
        break;

    case 0x49:
        // LD C,C
        REG_C = REG_C;
        INCREMENT( 1 );
        break;

    case 0x4A:
        // LD C,D
        REG_C = REG_D;
        INCREMENT( 1 );
        break;

    case 0x4B:
        // LD C,E
        REG_C = REG_E;
        INCREMENT( 1 );
        break;

    case 0x4C:
        // LD C,H
        REG_C = REG_H;
        INCREMENT( 1 );
        break;

    case 0x4D:
        // LD C,L
        REG_C = REG_L;
        INCREMENT( 1 );
        break;

    case 0x4E:
        // LD C,(HL)
        REG_C = get_1byteDataFromAddr( REG_HL );
        INCREMENT( 1 );
        break;

    case 0x4F:
        // LD C,A
        REG_C = REG_A;
        INCREMENT( 1 );
        break;

    case 0x50:
        // LD D,B
        REG_D = REG_B;
        INCREMENT( 1 );
        break;

    case 0x51:
        // LD D,C
        REG_D = REG_C;
        INCREMENT( 1 );
        break;

    case 0x52:
        // LD D,D
        REG_D = REG_D;
        INCREMENT( 1 );
        break;

    case 0x53:
        // LD D,E
        REG_D = REG_E;
        INCREMENT( 1 );
        break;

    case 0x54:
        // LD D,H
        REG_D = REG_H;
        INCREMENT( 1 );
        break;

    case 0x55:
        // LD D,L
        REG_D = REG_L;
        INCREMENT( 1 );
        break;

    case 0x56:
        // LD D,(HL)
        REG_D = get_1byteDataFromAddr( REG_HL );
        INCREMENT( 1 );
        break;

    case 0x57:
        // LD D,A
        REG_D = REG_A;
        INCREMENT( 1 );
        break;

    case 0x58:
        // LD E,B
        REG_E = REG_B;
        INCREMENT( 1 );
        break;

    case 0x59:
        // LD E,C
        REG_E = REG_C;
        INCREMENT( 1 );
        break;

    case 0x5A:
        // LD E,D
        REG_E = REG_D;
        INCREMENT( 1 );
        break;

    case 0x5B:
        // LD E,E
        REG_E = REG_E;
        INCREMENT( 1 );
        break;

    case 0x5C:
        // LD E,H
        REG_E = REG_H;
        INCREMENT( 1 );
        break;

    case 0x5D:
        // LD E,L
        REG_E = REG_L;
        INCREMENT( 1 );
        break;

    case 0x5E:
        // LD E,(HL)
        REG_E = get_1byteDataFromAddr( REG_HL );
        INCREMENT( 1 );
        break;

    case 0x5F:
        // LD E,A
        REG_E = REG_A;
        INCREMENT( 1 );
        break;

    case 0x60:
        // LD H,B
        REG_H = REG_B;
        INCREMENT( 1 );
        break;

    case 0x61:
        // LD H,C
        REG_H = REG_C;
        INCREMENT( 1 );
        break;

    case 0x62:
        // LD H,D
        REG_H = REG_D;
        INCREMENT( 1 );
        break;

    case 0x63:
        // LD H,E
        REG_H = REG_E;
        INCREMENT( 1 );
        break;

    case 0x64:
        // LD H,H
        REG_H = REG_H;
        INCREMENT( 1 );
        break;

    case 0x65:
        // LD H,L
        REG_H = REG_L;
        INCREMENT( 1 );
        break;

    case 0x66:
        // LD H,(HL)
        REG_H = get_1byteDataFromAddr( REG_HL );
        INCREMENT( 1 );
        break;

    case 0x67:
        // LD H,A
        REG_H = REG_A;
        INCREMENT( 1 );
        break;

    case 0x68:
        // LD L,B
        REG_L = REG_B;
        INCREMENT( 1 );
        break;

    case 0x69:
        // LD L,C
        REG_L = REG_C;
        INCREMENT( 1 );
        break;

    case 0x6A:
        // LD L,D
        REG_L = REG_D;
        INCREMENT( 1 );
        break;

    case 0x6B:
        // LD L,E
        REG_L = REG_E;
        INCREMENT( 1 );
        break;

    case 0x6C:
        // LD L,H
        REG_L = REG_H;
        INCREMENT( 1 );
        break;

    case 0x6D:
        // LD L,L
        REG_L = REG_L;
        INCREMENT( 1 );
        break;

    case 0x6E:
        // LD L,(HL)
        REG_L = get_1byteDataFromAddr( REG_HL );
        INCREMENT( 1 );
        break;

    case 0x6F:
        // LD L,A
        REG_L = REG_A;
        INCREMENT( 1 );
        break;

    case 0x70:
        // LD (HL),B
        write_1byteData( REG_HL, REG_B );
        INCREMENT( 1 );
        break;

    case 0x71:
        // LD (HL),C
        write_1byteData(  REG_HL, REG_C );
        INCREMENT( 1 );
        break;

    case 0x72:
        // LD (HL),D
        write_1byteData(  REG_HL, REG_D );
        INCREMENT( 1 );
        break;

    case 0x73:
        // LD (HL),E
        write_1byteData(  REG_HL, REG_E );
        INCREMENT( 1 );
        break;

    case 0x74:
        // LD (HL),H
        write_1byteData(  REG_HL, REG_H );
        INCREMENT( 1 );
        break;

    case 0x75:
        // LD (HL),L
        write_1byteData(  REG_HL, REG_L );
        INCREMENT( 1 );
        break;

    case 0x76:
        // HALT !!!
        prog->halt = 1;
        //printf("\n ---------- HALT !!!! --------------- \n");
        INCREMENT( 1 );
        break;

    case 0x77:
        // LD (HL),A
        write_1byteData( REG_HL, REG_A );
        INCREMENT( 1 );
        break;

    case 0x78:
        // LD A,B
        REG_A = REG_B;
        INCREMENT( 1 );
        break;

    case 0x79:
        // LD A,C
        REG_A = REG_C;
        INCREMENT( 1 );
        break;

    case 0x7A:
        // LD A,D
        REG_A = REG_D;
        INCREMENT( 1 );
        break;

    case 0x7B:
        // LD A,E
        REG_A = REG_E;
        INCREMENT( 1 );
        break;

    case 0x7C:
        // LD A,H
        REG_A = REG_H;
        INCREMENT( 1 );
        break;

    case 0x7D:
        // LD A,L
        REG_A = REG_L;
        INCREMENT( 1 );
        break;
    case 0x7E:
        // LD A,(HL)
        REG_A = get_1byteDataFromAddr( REG_HL );
        INCREMENT( 1 );
        break;

    case 0x7F:
        // LD A,A
        REG_A = REG_A;
        INCREMENT( 1 );
        break;

    case 0x80:
        // ADD A,B
        setFlags_for_Add_1Byte( REG_A, REG_B );
        REG_A = REG_A + REG_B;
        INCREMENT( 1 );
        break;

    case 0x81:
        // ADD A,C
        setFlags_for_Add_1Byte(  REG_A, REG_C );
        REG_A = REG_A + REG_C;
        INCREMENT( 1 );
        break;

    case 0x82:
        // ADD A,D
        setFlags_for_Add_1Byte(  REG_A, REG_D );
        REG_A = REG_A + REG_D;
        INCREMENT( 1 );
        break;

    case 0x83:
        // ADD A,E
        setFlags_for_Add_1Byte(  REG_A, REG_E );
        REG_A = REG_A + REG_E;
        INCREMENT( 1 );
        break;

    case 0x84:
        // ADD A,H
        setFlags_for_Add_1Byte(  REG_A, REG_H );
        REG_A = REG_A + REG_H;
        INCREMENT( 1 );
        break;

    case 0x85:
        // ADD A,L
        setFlags_for_Add_1Byte(  REG_A, REG_L );
        REG_A = REG_A + REG_L;
        INCREMENT( 1 );
        break;

    case 0x86:
        // ADD A,(HL)
        setFlags_for_Add_1Byte(  REG_A, get_1byteDataFromAddr( REG_HL ) );
        REG_A = REG_A + get_1byteDataFromAddr( REG_HL ) ;
        INCREMENT( 1 );
        break;

    case 0x87:
        // ADD A,A
        setFlags_for_Add_1Byte( REG_A, REG_A );
        REG_A = REG_A + REG_A;
        INCREMENT( 1 );
        break;

    case 0x88:
        // ADC A,B
        REG_A = REG_A + setFlags_for_Adc_1Byte( REG_A, REG_B );
        INCREMENT( 1 );
        break;

    case 0x89:
        // ADD A,C
        REG_A = REG_A + setFlags_for_Adc_1Byte( REG_A, REG_C );
        INCREMENT( 1 );
        break;

    case 0x8A:
        // ADD A,D
        REG_A = REG_A + setFlags_for_Adc_1Byte(  REG_A, REG_D );
        INCREMENT( 1 );
        break;

    case 0x8B:
        // ADD A,E
        REG_A = REG_A + setFlags_for_Adc_1Byte(  REG_A, REG_E );
        INCREMENT( 1 );
        break;

    case 0x8C:
        // ADD A,H
        REG_A = REG_A + setFlags_for_Adc_1Byte(  REG_A, REG_H );
        INCREMENT( 1 );
        break;

    case 0x8D:
        // ADD A,L
        REG_A = REG_A + setFlags_for_Adc_1Byte(  REG_A, REG_L );
        INCREMENT( 1 );
        break;

    case 0x8E:
        // ADD A,(HL)
        REG_A = REG_A + setFlags_for_Adc_1Byte(  REG_A, get_1byteDataFromAddr(  REG_HL ) );
        INCREMENT( 1 );
        break;

    case 0x8F:
        // ADD A,A
        REG_A = REG_A + setFlags_for_Adc_1Byte( REG_A, REG_A );
        INCREMENT( 1 );
        break;

    case 0x90:
        // SUB A,B
        setFlags_for_Sub_1Byte(  REG_A, REG_B );
        REG_A = REG_A - REG_B;
        INCREMENT(1);
        break;

    case 0x91:
        // SUB A,C
        setFlags_for_Sub_1Byte(  REG_A, REG_C );
        REG_A = REG_A - REG_C;
        INCREMENT(1);
        break;

    case 0x92:
        // SUB A,D
        setFlags_for_Sub_1Byte(  REG_A, REG_D );
        REG_A = REG_A - REG_D;
        INCREMENT(1);
        break;

    case 0x93:
        // SUB A,E
        setFlags_for_Sub_1Byte(  REG_A, REG_E );
        REG_A = REG_A - REG_E;
        INCREMENT(1);
        break;

    case 0x94:
        // SUB A,H
        setFlags_for_Sub_1Byte(  REG_A, REG_H );
        REG_A = REG_A - REG_H;
        INCREMENT(1);
        break;

    case 0x95:
        // SUB A,L
        setFlags_for_Sub_1Byte(  REG_A, REG_L );
        REG_A = REG_A - REG_L;
        INCREMENT(1);
        break;

    case 0x96:
        // SUB A,(HL)
        setFlags_for_Sub_1Byte(  REG_A, get_1byteDataFromAddr(  REG_HL ) );
        REG_A = REG_A - get_1byteDataFromAddr(  REG_HL );
        INCREMENT(1);
        break;

    case 0x97:
        // SUB A,A
        setFlags_for_Sub_1Byte(  REG_A, REG_A );
        REG_A = REG_A - REG_A;
        INCREMENT(1);
        break;

    case 0x98:
        // SBC A,B
        REG_A = REG_A - setFlags_for_Sbc_1Byte(  REG_A, REG_B );
        INCREMENT(1);
        break;

    case 0x99:
        // SBC A,C
        REG_A = REG_A - setFlags_for_Sbc_1Byte(  REG_A, REG_C );
        INCREMENT(1);
        break;

    case 0x9A:
        // SBC A,D
        REG_A = REG_A - setFlags_for_Sbc_1Byte(  REG_A, REG_D );
        INCREMENT(1);
        break;

    case 0x9B:
        // SBC A,E
        REG_A = REG_A - setFlags_for_Sbc_1Byte(  REG_A, REG_E );
        INCREMENT(1);
        break;

    case 0x9C:
        // SBC A,H
        REG_A = REG_A - setFlags_for_Sbc_1Byte(  REG_A, REG_H );
        INCREMENT(1);
        break;

    case 0x9D:
        // SBC A,L
        REG_A = REG_A - setFlags_for_Sbc_1Byte(  REG_A, REG_L );
        INCREMENT(1);
        break;

    case 0x9E:
        // SBC A,(HL)
        REG_A = REG_A - setFlags_for_Sbc_1Byte(  REG_A, get_1byteDataFromAddr(  REG_HL ) );
        INCREMENT(1);
        break;

    case 0x9F:
        // SBC A,A
        REG_A = REG_A - setFlags_for_Sbc_1Byte(  REG_A, REG_A );
        INCREMENT(1);
        break;

    case 0xA0:
        // AND A,B
        setFlags_for_And_1Byte( REG_A, REG_B );
        REG_A = REG_A & REG_B;
        INCREMENT(1);
        break;

    case 0xA1:
        // AND A,C
        setFlags_for_And_1Byte(  REG_A, REG_C );
        REG_A = REG_A & REG_C;
        INCREMENT(1);
        break;

    case 0xA2:
        // AND A,D
        setFlags_for_And_1Byte(  REG_A, REG_D );
        REG_A = REG_A & REG_D;
        INCREMENT(1);
        break;

    case 0xA3:
        // AND A,E
        setFlags_for_And_1Byte(  REG_A, REG_E );
        REG_A = REG_A & REG_E;
        INCREMENT(1);
        break;

    case 0xA4:
        // AND A,H
        setFlags_for_And_1Byte( REG_A, REG_H );
        REG_A = REG_A & REG_H;
        INCREMENT(1);
        break;

    case 0xA5:
        // AND A,L
        setFlags_for_And_1Byte( REG_A, REG_L );
        REG_A = REG_A & REG_L;
        INCREMENT(1);
        break;

    case 0xA6:
        // AND A,(HL)
        setFlags_for_And_1Byte( REG_A, get_1byteDataFromAddr(  REG_HL ) );
        REG_A = REG_A & get_1byteDataFromAddr(  REG_HL );
        INCREMENT(1);
        break;

    case 0xA7:
        // AND A,A
        setFlags_for_And_1Byte( REG_A, REG_A );
        REG_A = REG_A & REG_A;
        INCREMENT(1);
        break;

    case 0xA8:
        // XOR A,B
        setFlags_for_Xor_1Byte( REG_A, REG_B );
        REG_A = REG_A ^ REG_B;
        INCREMENT(1);
        break;

    case 0xA9:
        // XOR A,C
        setFlags_for_Xor_1Byte(  REG_A, REG_C );
        REG_A = REG_A ^ REG_C;
        INCREMENT(1);
        break;

    case 0xAA:
        // XOR A,D
        setFlags_for_Xor_1Byte( REG_A, REG_D );
        REG_A = REG_A ^ REG_D;
        INCREMENT(1);
        break;

    case 0xAB:
        // XOR A,E
        setFlags_for_Xor_1Byte( REG_A, REG_E );
        REG_A = REG_A ^ REG_E;
        INCREMENT(1);
        break;

    case 0xAC:
        // XOR A,H
        setFlags_for_Xor_1Byte( REG_A, REG_H );
        REG_A = REG_A ^ REG_H;
        INCREMENT(1);
        break;

    case 0xAD:
        // XOR A,L
        setFlags_for_Xor_1Byte(  REG_A, REG_L );
        REG_A = REG_A ^ REG_L;
        INCREMENT(1);
        break;

    case 0xAE:
        // XOR A,(HL)
        setFlags_for_Xor_1Byte( REG_A, get_1byteDataFromAddr(  REG_HL ) );
        REG_A = REG_A ^ get_1byteDataFromAddr(  REG_HL );
        INCREMENT(1);
        break;

    case 0xAF:
        // XOR A,A
        setFlags_for_Xor_1Byte( REG_A, REG_A );
        REG_A = REG_A ^ REG_A;
        INCREMENT(1);
        break;

    case 0xB0:
        // OR A,B
        setFlags_for_Or_1Byte( REG_A, REG_B );
        REG_A = REG_A | REG_B;
        INCREMENT( 1 );
        break;

    case 0xB1:
        // OR A,C
        setFlags_for_Or_1Byte( REG_A, REG_C );
        REG_A = REG_A | REG_C;
        INCREMENT( 1 );
        break;

    case 0xB2:
        // OR A,D
        setFlags_for_Or_1Byte( REG_A, REG_D );
        REG_A = REG_A | REG_D;
        INCREMENT( 1 );
        break;

    case 0xB3:
        // OR A,E
        setFlags_for_Or_1Byte( REG_A, REG_E );
        REG_A = REG_A | REG_E;
        INCREMENT( 1 );
        break;

    case 0xB4:
        // OR A,H
        setFlags_for_Or_1Byte( REG_A, REG_H );
        REG_A = REG_A | REG_H;
        INCREMENT( 1 );
        break;

    case 0xB5:
        // OR A,L
        setFlags_for_Or_1Byte( REG_A, REG_L );
        REG_A = REG_A | REG_L;
        INCREMENT( 1 );
        break;

    case 0xB6:
        // OR A,(HL)
        setFlags_for_Or_1Byte(  REG_A, get_1byteDataFromAddr(  REG_HL ) );
        REG_A = REG_A | get_1byteDataFromAddr( REG_HL );
        INCREMENT( 1 );
        break;

    case 0xB7:
        // OR A,A
        setFlags_for_Or_1Byte( REG_A, REG_A );
        REG_A = REG_A | REG_A;
        INCREMENT( 1 );
        break;

    case 0xB8:
        // CP A,B
        setFlags_for_Sub_1Byte( REG_A, REG_B );
        INCREMENT( 1 );
        break;

    case 0xB9:
        // CP A,C
        setFlags_for_Sub_1Byte( REG_A, REG_C );
        INCREMENT( 1 );
        break;
    case 0xBA:

        // CP A,D
        setFlags_for_Sub_1Byte( REG_A, REG_D );
        INCREMENT( 1 );
        break;

    case 0xBB:
        // CP A,E
        setFlags_for_Sub_1Byte( REG_A, REG_E );
        INCREMENT( 1 );
        break;

    case 0xBC:
        // CP A,H
        setFlags_for_Sub_1Byte( REG_A, REG_H );
        INCREMENT( 1 );
        break;

    case 0xBD:
        // CP A,L
        setFlags_for_Sub_1Byte( REG_A, REG_L );
        INCREMENT( 1 );
        break;

    case 0xBE:
        // CP A,(HL)
        setFlags_for_Sub_1Byte(  REG_A, get_1byteDataFromAddr(  REG_HL ) );
        INCREMENT( 1 );
        break;

    case 0xBF:
        // CP A,A
        setFlags_for_Sub_1Byte(  REG_A, REG_A );
        INCREMENT( 1 );
        break;

    case 0xC0: // !!!
        // RET NZ
        INCREMENT( 1 );
        if ( !getZeroFlag( ) )
            PC = read_from_stack( );
        break;

    case 0xC1:
        // POP BC
        INCREMENT( 1 );
        REG_BC = read_from_stack( );
        break;

    case 0xC2:
        // JP NZ,u16
        myVal = get_2byteData( );
        INCREMENT( 3 );
        if( !getZeroFlag( ) )
            PC = myVal;
        break;

    case 0xC3: // !!!
        // JP u16
        myVal = get_2byteData( );
        INCREMENT( 3 );
        PC = myVal;
        break;

    case 0xC4:
        // CALL NZ,u16
        myVal = get_2byteData( );
        INCREMENT( 3 );
        if( !getZeroFlag( ) )
        {
            push_to_stack( PC );
            PC = myVal;
        }
        break;

    case 0xC5:
        // PUSH BC
        INCREMENT( 1 );
        push_to_stack( REG_BC );
        break;

    case 0xC6:
        // ADD A,u8
        setFlags_for_Add_1Byte(  REG_A, get_1byteData(  ) );
        REG_A = REG_A + get_1byteData(  );
        INCREMENT( 2 );
        break;

//    case 0xC7:
//        // RST 00h
//        INCREMENT( 1 );
//        push_to_stack( PC );
//        PC = 0x00;
//        break;

    case 0xC8:
        // RET NZ
        INCREMENT( 1 );
        if ( getZeroFlag( ) )
            PC = read_from_stack( );
        break;

    case 0xC9:
        // RET
        myVal = read_from_stack( );
        INCREMENT( 1 );
        PC = myVal;
        break;

    case 0xCA:
        // JP Z,u16
        myVal = get_2byteData( );
        INCREMENT( 3 );
        if( getZeroFlag( ) )
            PC = myVal;
        break;

    case 0xCB:
        // PREFIX CB
        INCREMENT( 1 );
        prog->opcode = prog->memory[ PC ];
        prog->tikz += InstructionTicks_0xCB[ prog->opcode ];
        gb_exec_prefix( );
        break;

    case 0xCC:
        // CALL Z,u16
        myVal = get_2byteData(  );
        INCREMENT( 3 );
        if( getZeroFlag( ))
            PC = myVal;
        break;

    case 0xCD: // CALL !!!
        // CALL u16
        myVal = get_2byteData( );
        INCREMENT( 3 );
        push_to_stack( PC );
        PC = myVal;
        break;

    case 0xCF:
        // RST 08h
        INCREMENT( 1 );
        push_to_stack( PC );
        PC = 0x08;
        break;

    case 0xD0:
        // RET NC
        INCREMENT( 1 );
        if ( !getCarryFlag(  ) )
            PC = read_from_stack(  );
        break;

    case 0xD1:
        // POP DE
        REG_DE = read_from_stack(  );
        INCREMENT( 1 );
        break;

    case 0xD2:
        // JP NC,u16
        myVal = get_2byteData(  );
        INCREMENT( 3 );
        if( !getCarryFlag(  ) )
            PC = myVal;
        break;

    case 0xD4:
        // CALL NC,u16
        myVal = get_2byteData(  );
        INCREMENT( 3 );
        if( !getCarryFlag(  ) ){
            push_to_stack( PC );
            PC = myVal;
        }
        break;

    case 0xD5:
        // PUSH DE
        push_to_stack( REG_DE );
        INCREMENT( 1 );
        break;

    case 0xD6:
        // SUB A,u8
        setFlags_for_Sub_1Byte(  REG_A, get_1byteData( ) );
        REG_A = REG_A - get_1byteData( );
        INCREMENT( 2 );
        break;

//    case 0xD7:
//        // RST 10h
//        INCREMENT( 1 );
//        push_to_stack( PC );
//        PC = 0x10;
//        break;

    case 0xD8:
        // RET C
        INCREMENT( 1 );
        if ( getCarryFlag( ) )
            PC = read_from_stack( );
        break;

    case 0xD9:
        // RETI
        PC = read_from_stack( );
        prog->IME = 1;
        break;

    case 0xDA:
        // JP C,u16
        myVal = get_2byteData( );
        INCREMENT( 3 );
        if( getCarryFlag( ) )
            PC = myVal;
        break;

    case 0xDC:
        // CALL C,u16
        myVal = get_2byteData(  );
        INCREMENT( 3 );
        if( getCarryFlag( ) ){
            push_to_stack( PC );
            PC = myVal;
        }
        break;

    case 0xDE:
        // SBC A,C
        REG_A = REG_A - setFlags_for_Sbc_1Byte(  REG_A, get_1byteData() );
        INCREMENT(1);
        break;

    case 0xDF:
        // RST 38h
        INCREMENT( 1 );
        push_to_stack( PC );
        prog->pc = 0x18;
        break;

    case 0xE0:
        // LD (FF00+u8),A
        write_1byteData( (0xFF00 + get_1byteData( )), REG_A );
        INCREMENT( 2 );
        break;

    case 0xE1:
        // POP HL
        REG_HL = read_from_stack(  );
        INCREMENT( 1 );
        break;

    case 0xE2:
        // LD (FF00+C),A
        write_1byteData( 0xFF00 + REG_C, REG_A);
        INCREMENT( 1 );
        break;

    case 0xE5:
        // PUSH HL
        INCREMENT( 1 );
        push_to_stack( REG_HL );
        break;

    case 0xE6:
        // AND A,u8
        setFlags_for_And_1Byte( REG_A, get_1byteData( ) );
        REG_A = REG_A & get_1byteData( );
        INCREMENT( 2 );
        break;

//    case 0xE7:
//        // RST 20h
//        INCREMENT( 1 );
//        push_to_stack( PC );
//        PC = 0x20;
//        break;

    case 0xE8: // !!!
        // ADD SP,i8
        setFlagsForAdd2Byte(SP, get_1byteSignedData());
        SP = SP + get_1byteSignedData();
        INCREMENT( 2 );
        break;

    case 0xE9:
        // JP HL
        INCREMENT( 1 );
        PC = REG_HL;
        break;

    case 0xEA:
        // LD (u16),A !!!
        write_1byteData( get_2byteData( ), REG_A );
        INCREMENT( 3 );
        break;

    case 0xEF:
        // RST 28h !!!
        INCREMENT( 1 );
        push_to_stack( PC );
        PC = 0x0028;
        break;

    case 0xF0:
        // LD A,(FF00+u8)
        REG_A = get_1byteDataFromAddr( 0xFF00 + get_1byteData( ) );
        INCREMENT( 2 );
        break;

    case 0xF1:
        // POP AF
        REG_AF = read_from_stack( );
        INCREMENT( 1 );
        break;

    case 0xF2:
        // LD A,(FF00+C)
        REG_A = get_1byteDataFromAddr( 0xFF00 + REG_C );
        INCREMENT( 1 );
        break;

    case 0xF3:
        // DI
        prog->IME = 0;
        INCREMENT( 1 );
        break;

    case 0xF5:
        // PUSH AF
        push_to_stack( REG_AF );
        INCREMENT( 1 );
        break;

    case 0xF6:
        // AND A,u8
        setFlags_for_Or_1Byte(  REG_A, get_1byteData(  ) );
        REG_A = REG_A | get_1byteData(  );
        INCREMENT( 2 );
        break;

    case 0xF7:
        // RST 30h
        INCREMENT( 1 );
        push_to_stack( PC );
        PC = 0x30;
        break;

    case 0xF8:
        // LD HL,SP+i8
        mySignVal = get_1byteSignedData();
        REG_HL = SP + mySignVal;
        setFlags(0,0,0,0);
        if (((SP ^ mySignVal ^ REG_HL) & 0x100) == 0x100)
            setFlags(0,0,3,1);
        if (((SP ^ mySignVal ^ REG_HL) & 0x10) == 0x10)
            setFlags(0,0,1,0);
        INCREMENT( 2 );
        break;

    case 0xF9:
        //LD SP,HL
        SP = REG_HL;
        INCREMENT( 1 );
        break;

    case 0xFA:
        //LD A,(u16)
        REG_A = get_1byteDataFromAddr(  get_2byteData( ) );
        INCREMENT( 3 );
        break;

    case 0xFB:
        // EI
        prog->IME = 1;
        INCREMENT( 1 );
        break;

    case 0xFE:
        // CP A,u8
        setFlags_for_CP( REG_A, get_1byteData(  ) );
        INCREMENT( 2 );
        break;

    case 0xFF: // !!!
        // RST 38h
        INCREMENT( 1 );
        push_to_stack( PC );
        prog->pc = 0x38;
        break;

    default:
        printf("Funktion nicht unterstuetzt: 0x%02X @ addr.:%d\n",prog->opcode,PC);
        while(1){}
        break;
    }

    #if (DEBUG_OPM==1)
        printf("AF=0x%04X, BC=0x%04X, DE=0x%04X, HL=0x%04X, SP=0x%04X, PC:=0x%04X\n",REG_AF,REG_BC,REG_DE,REG_HL,SP,PC);
    #endif // DEBUG_OPM

    #if DEBUG_STEP
        if( PC == stopOnPC )
        {
            stepNum = 0;
            printf("--- Stoped on PC: 0x%04X ---\n",prog->pc);
        }
        if( (!stepNum)  )
        {
            printf("   -->Menue: Trigger(i),BC(s),+10(z),+100(h),+1000(t),+65000(x): ");
            myChar = getchar();
            if( myChar == 'i' )
            {
                int i;
                scanf("%x", &i); stopOnPC = (uint16_t)i;
                printf("-->Val setted to 0x%04X", stopOnPC);
                Sleep(1000);
                stepNum = 1000000;
            }
            else
            {
                //if (myChar == 's') REG_BC = 0x0101;
                if( myChar == 'z' ) stepNum = 10;
                if( myChar == 'h' ) stepNum = 100;
                if( myChar == 't' ) stepNum = 1000;
                if( myChar == 'x' ) stepNum = 65535;
            }

            // Warte auf Zeilenumbruch
            while('\n'!=getchar());
        }
        else
            stepNum --;
    #endif // DEBUG_STEP
}

// Sonderfunktionen des Gameboys
void gb_exec_prefix( )
{
    uint8_t myHelperVal = 0;

    switch( prog->opcode & 0xff )
    {
    case 0x00:
        // RLC B
        RLC(&REG_B);
        break;

    case 0x01:
        // RLC C
        RLC(&REG_C);
        break;

    case 0x02:
        // RLC D
        RLC(&REG_D);
        break;

    case 0x03:
        // RLC E
        RLC(&REG_E);
        break;

    case 0x04:
        // RLC H
        RLC(&REG_H);
        break;

    case 0x05:
        // RLC L
        RLC(&REG_L);
        break;

    case 0x06:
        // RLC (HL)
        myHelperVal = get_1byteDataFromAddr(REG_HL);
        RLC(&myHelperVal);
        write_1byteData(REG_HL, myHelperVal);
        break;

    case 0x07:
        // RLC A
        RLC(&REG_A);
        break;

    case 0x08:
        // RRC B
        RRC(&REG_B);
        break;

    case 0x09:
        // RRC C
        RRC(&REG_C);
        break;

    case 0x0A:
        // RRC D
        RRC(&REG_D);
        break;

    case 0x0B:
        // RRC E
        RRC(&REG_E);
        break;

    case 0x0C:
        // RRC H
        RRC(&REG_H);
        break;

    case 0x0D:
        // RRC L
        RRC(&REG_L);
        break;

    case 0x0E:
        // RRC (HL)
        myHelperVal = get_1byteDataFromAddr(REG_HL);
        RRC(&myHelperVal);
        write_1byteData(REG_HL, myHelperVal);
        break;

    case 0x0F:
        // RRC A
        RRC(&REG_A);
        break;

    case 0x10:
        // RL B
        RL(&REG_B );
        break;

    case 0x11:
        // RL C
        RL(&REG_C );
        break;

    case 0x12:
        // RL D
        RL(&REG_D );
        break;

    case 0x13:
        // RL E
        RL(&REG_E );
        break;

    case 0x14:
        // RL H
        RL(&REG_H );
        break;

    case 0x15:
        // RL L
        RL(&REG_L );
        break;

    case 0x16:
        // RL (HL)
        myHelperVal = get_1byteDataFromAddr(  REG_HL);
        RL(&myHelperVal );
        write_1byteData(  REG_HL, myHelperVal);
        break;

    case 0x17:
        // RL A
        RL(&REG_A );
        break;

    case 0x18:
        // RR B
        RR(&REG_B );
        break;

    case 0x19:
        // RR C
        RR(&REG_C );
        break;

    case 0x1A:
        // RR D
        RR(&REG_D );
        break;

    case 0x1B:
        // RR E
        RR(&REG_E );
        break;

    case 0x1C:
        // RR H
        RR(&REG_H );
        break;

    case 0x1D:
        // RR L
        RR(&REG_L );
        break;

    case 0x1E:
        // RR (HL)
        myHelperVal = get_1byteDataFromAddr(  REG_HL);
        RR(&myHelperVal );
        write_1byteData(  REG_HL, myHelperVal);
        break;

    case 0x1F:
        // RR A
        RR(&REG_A );
        break;

    case 0x20:
        // SLA B
        SLA(&REG_B );
        break;

    case 0x21:
        // SLA C
        SLA(&REG_C );
        break;

    case 0x22:
        // SLA D
        SLA(&REG_D );
        break;

    case 0x23:
        // SLA E
        SLA(&REG_E );
        break;

    case 0x24:
        // SLA H
        SLA(&REG_H );
        break;

    case 0x25:
        // SLA L
        SLA(&REG_L );
        break;

    case 0x26:
        // SLA (HL)
        myHelperVal = get_1byteDataFromAddr(  REG_HL);
        SLA(&myHelperVal );
        write_1byteData(  REG_HL, myHelperVal);
        break;

    case 0x27:
        // SLA A
        SLA(&REG_A );
        break;

    case 0x28:
        // SRA B
        SRA(&REG_B );
        break;

    case 0x29:
        // SRA C
        SRA(&REG_C );
        break;

    case 0x2A:
        // SRA D
        SRA(&REG_D );
        break;

    case 0x2B:
        // SRA E
        SRA(&REG_E );
        break;

    case 0x2C:
        // SRA H
        SRA(&REG_H );
        break;

    case 0x2D:
        // SRA L
        SRA(&REG_L );
        break;

    case 0x2E:
        // SRA (HL)
        myHelperVal = get_1byteDataFromAddr(  REG_HL);
        SRA(&myHelperVal );
        write_1byteData(  REG_HL, myHelperVal);
        break;

    case 0x2F:
        // SRA A
        SRA(&REG_A );
        break;

    case 0x30:
        // SWAP B
        SWAP(&REG_B );
        break;

    case 0x31:
        // SWAP C
        SWAP(&REG_C );
        break;

    case 0x32:
        // SWAP D
        SWAP(&REG_D );
        break;

    case 0x33:
        // SWAP E
        SWAP(&REG_E );
        break;

    case 0x34:
        // SWAP H
        SWAP(&REG_H );
        break;

    case 0x35:
        // SWAP L
        SWAP(&REG_L );
        break;

    case 0x36:
        // SWAP (HL)
        myHelperVal = get_1byteDataFromAddr(  REG_HL );
        SWAP(&myHelperVal );
        write_1byteData(  REG_HL, myHelperVal);
        break;

    case 0x37:
        // SWAP A
        SWAP(&REG_A );
        break;

    case 0x38:
        // SRL B
        SRL(&REG_B );
        break;

    case 0x39:
        // SRL C
        SRL(&REG_C );
        break;

    case 0x3A:
        // SRL D
        SRL(&REG_D );
        break;

    case 0x3B:getchar();
        // SRL E
        SRL(&REG_E );
        break;

    case 0x3C:
        // SRL H
        SRL(&REG_H );
        break;

    case 0x3D:
        // SRL L
        SRL(&REG_L );
        break;

    case 0x3E:
        // SRL (HL)
        myHelperVal = get_1byteDataFromAddr(  REG_HL);
        SRL(&myHelperVal );
        write_1byteData(  REG_HL, myHelperVal);
        break;

    case 0x3F:
        // SRL A
        SRL(&REG_A );
        break;

    case 0x40:
        // BIT 0,B
        BIT(0,REG_B);
        break;

    case 0x41:
        // BIT 0,C
        BIT(0,REG_C);
        break;

    case 0x42:
        // BIT 0,D
        BIT(0,REG_D);
        break;

    case 0x43:
        // BIT 0,E
        BIT(0,REG_E);
        break;

    case 0x44:
        // BIT 0,H
        BIT(0,REG_H);
        break;

    case 0x45:
        // BIT 0,L
        BIT(0,REG_L);
        break;

    case 0x46: // To test !!! !!! !!!
        // BIT 0,(HL)
        BIT(0,get_1byteData(REG_HL));
        break;

    case 0x47:
        // BIT 0,A
        BIT(0,REG_A);
        break;

    case 0x48:
        // BIT 1,B
        BIT(1,REG_B);
        break;

    case 0x49:
        // BIT 1,C
        BIT(1,REG_C);
        break;

    case 0x4A:
        // BIT 1,D
        BIT(1,REG_D);
        break;

    case 0x4B:
        // BIT 1,E
        BIT(1,REG_E);
        break;

    case 0x4C:
        // BIT 1,H
        BIT(1,REG_H);
        break;

    case 0x4D:
        // BIT 1,L
        BIT(1,REG_L);
        break;

    case 0x4E: // To test !!! !!! !!!
        // BIT 1,(HL)
        BIT(1,get_1byteData(REG_HL));
        break;

    case 0x4F:
        // BIT 1,A
        BIT(1,REG_A);
        break;

    case 0x50:
        // BIT 2,B
        BIT(2,REG_B);
        break;

    case 0x51:
        // BIT 2,C
        BIT(2,REG_C);
        break;

    case 0x52:
        // BIT 2,D
        BIT(2,REG_D);
        break;

    case 0x53:
        // BIT 2,E
        BIT(2,REG_E);
        break;

    case 0x54:
        // BIT 2,H
        BIT(2,REG_H);
        break;

    case 0x55:
        // BIT 2,L
        BIT(2,REG_L);
        break;

    case 0x56: // To test !!! !!! !!!
        // BIT 2,(HL)
        BIT(2,get_1byteData(REG_HL));
        break;

    case 0x57:
        // BIT 2,A
        BIT(2,REG_A);
        break;

    case 0x58:
        // BIT 3,B
        BIT(3,REG_B);
        break;

    case 0x59:
        // BIT 3,C
        BIT(3,REG_C);
        break;

    case 0x5A:
        // BIT 3,D
        BIT(3,REG_D);
        break;

    case 0x5B:
        // BIT 3,E
        BIT(3,REG_E);
        break;

    case 0x5C:
        // BIT 3,H
        BIT(3,REG_H);
        break;

    case 0x5D:
        // BIT 3,L
        BIT(3,REG_L);
        break;

    case 0x5E: // To test !!! !!! !!!
        // BIT 3,(HL)
        BIT(3,get_1byteData(REG_HL));
        break;

    case 0x5F:
        // BIT 3,A
        BIT(3,REG_A);
        break;

    case 0x60:
        // BIT 4,B
        BIT(4,REG_B);
        break;

    case 0x61:
        // BIT 4,C
        BIT(4,REG_C);
        break;

    case 0x62:
        // BIT 4,D
        BIT(4,REG_D);
        break;

    case 0x63:
        // BIT 4,E
        BIT(4,REG_E);
        break;

    case 0x64:
        // BIT 4,H
        BIT(4,REG_H);
        break;

    case 0x65:
        // BIT 4,L
        BIT(4,REG_L);
        break;

    case 0x66: // To test !!! !!! !!!
        // BIT 4,(HL)
        BIT(4,get_1byteData(REG_HL));
        break;

    case 0x67:
        // BIT 4,A
        BIT(4,REG_A);
        break;

    case 0x68:
        // BIT 5,B
        BIT(5,REG_B);
        break;

    case 0x69:
        // BIT 5,C
        BIT(5,REG_C);
        break;

    case 0x6A:
        // BIT 5,D
        BIT(5,REG_D);
        break;

    case 0x6B:
        // BIT 5,E
        BIT(5,REG_E);
        break;

    case 0x6C:
        // BIT 5,H
        BIT(5,REG_H);
        break;

    case 0x6D:
        // BIT 5,L
        BIT(5,REG_L);
        break;

    case 0x6E: // To test !!! !!! !!!
        // BIT 5,(HL)
        BIT(5,get_1byteData(REG_HL));
        break;

    case 0x6F:
        // BIT 5,A
        BIT(5,REG_A);
        break;

    case 0x70:
        // BIT 6,B
        BIT(6,REG_B);
        break;

    case 0x71:
        // BIT 6,C
        BIT(6,REG_C);
        break;

    case 0x72:
        // BIT 6,D
        BIT(6,REG_D);
        break;

    case 0x73:
        // BIT 6,E
        BIT(6,REG_E);
        break;

    case 0x74:
        // BIT 6,H
        BIT(6,REG_H);
        break;

    case 0x75:
        // BIT 6,L
        BIT(6,REG_L);
        break;

    case 0x76: // To test !!! !!! !!!
        // BIT 6,(HL)
        BIT(6,get_1byteData(REG_HL));
        break;

    case 0x77:
        // BIT 6,A
        BIT(6,REG_A);
        break;

    case 0x78:
        // BIT 7,B
        BIT(7,REG_B);
        break;

    case 0x79:
        // BIT 7,C
        BIT(7,REG_C);
        break;

    case 0x7A:
        // BIT 7,D
        BIT(7,REG_D);
        break;

    case 0x7B:
        // BIT 7,E
        BIT(7,REG_E);
        break;

    case 0x7C:
        // BIT 7,H
        BIT(7,REG_H);
        break;

    case 0x7D:
        // BIT 7,L
        BIT(7,REG_L);
        break;

    case 0x7E: // To test !!! !!! !!!
        // BIT 7,(HL)
        BIT(7,get_1byteData(REG_HL));
        break;

    case 0x7F:
        // BIT 7,A
        BIT(7,REG_A);
        break;

    case 0x80:
        // RES 0,B
        RES(0,&REG_B);
        break;

    case 0x81:
        // RES 0,C
        RES(0,&REG_C);
        break;

    case 0x82:
        // RES 0,D
        RES(0,&REG_D);
        break;

    case 0x83:
        // RES 0,E
        RES(0,&REG_E);
        break;

    case 0x84:
        // RES 0,H
        RES(0,&REG_H);
        break;

    case 0x85:
        // RES 0,L
        RES(0,&REG_L);
        break;

    case 0x86: // To test !!! !!! !!!
        // RES 0,(HL)
        myHelperVal = get_1byteData(REG_HL);
        RES(0,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0x87:
        // RES 0,A
        RES(0,&REG_A);
        break;

    case 0x88:
        // RES 1,B
        RES(1,&REG_B);
        break;

    case 0x89:
        // RES 1,C
        RES(1,&REG_C);
        break;

    case 0x8A:
        // RES 1,D
        RES(1,&REG_D);
        break;

    case 0x8B:
        // RES 1,E
        RES(1,&REG_E);
        break;

    case 0x8C:
        // RES 1,H
        RES(1,&REG_H);
        break;

    case 0x8D:
        // RES 1,L
        RES(1,&REG_L);
        break;

    case 0x8E: // To test !!! !!! !!!
        // RES 1,(HL)
        myHelperVal = get_1byteData(REG_HL);
        RES(1,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0x8F:
        // RES 1,A
        RES(1,&REG_A);
        break;

    case 0x90:
        // RES 2,B
        RES(2,&REG_B);
        break;

    case 0x91:
        // RES 2,C
        RES(2,&REG_C);
        break;

    case 0x92:
        // RES 2,D
        RES(2,&REG_D);
        break;

    case 0x93:
        // RES 2,E
        RES(2,&REG_E);
        break;

    case 0x94:
        // RES 2,H
        RES(2,&REG_H);
        break;

    case 0x95:
        // RES 2,L
        RES(2,&REG_L);
        break;

    case 0x96: // To test !!! !!! !!!
        // RES 2,(HL)
        myHelperVal = get_1byteData(REG_HL);
        RES(2,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0x97:
        // RES 2,A
        RES(2,&REG_A);
        break;

    case 0x98:
        // RES 3,B
        RES(3,&REG_B);
        break;

    case 0x99:
        // RES 3,C
        RES(3,&REG_C);
        break;

    case 0x9A:
        // RES 3,D
        RES(3,&REG_D);
        break;

    case 0x9B:
        // RES 3,E
        RES(3,&REG_E);
        break;

    case 0x9C:
        // RES 3,H
        RES(3,&REG_H);
        break;

    case 0x9D:
        // RES 3,L
        RES(3,&REG_L);
        break;

    case 0x9E: // To test !!! !!! !!!
        // RES 3,(HL)
        myHelperVal = get_1byteData(REG_HL);
        RES(3,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0x9F:
        // RES 3,A
        RES(3,&REG_A);
        break;

    case 0xA0:
        // RES 4,B
        RES(4,&REG_B);
        break;

    case 0xA1:
        // RES 4,C
        RES(4,&REG_C);
        break;

    case 0xA2:
        // RES 4,D
        RES(4,&REG_D);
        break;

    case 0xA3:
        // RES 4,E
        RES(4,&REG_E);
        break;

    case 0xA4:
        // RES 4,H
        RES(4,&REG_H);
        break;

    case 0xA5:
        // RES 4,L
        RES(4,&REG_L);
        break;

    case 0xA6: // To test !!! !!! !!!
        // RES 4,(HL)
        myHelperVal = get_1byteData(REG_HL);
        RES(4,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xA7:
        // RES 4,A
        RES(4,&REG_A);
        break;

    case 0xA8:
        // RES 5,B
        RES(5,&REG_B);
        break;

    case 0xA9:
        // RES 5,C
        RES(5,&REG_C);
        break;

    case 0xAA:
        // RES 5,D
        RES(5,&REG_D);
        break;

    case 0xAB:
        // RES 5,E
        RES(5,&REG_E);
        break;

    case 0xAC:
        // RES 5,H
        RES(5,&REG_H);
        break;

    case 0xAD:
        // RES 5,L
        RES(5,&REG_L);
        break;

    case 0xAE: // To test !!! !!! !!!
        // RES 5,(HL)
        myHelperVal = get_1byteData(REG_HL);
        RES(5,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xAF:
        // RES 5,A
        RES(5,&REG_A);
        break;

    case 0xB0:
        // RES 6,B
        RES(6,&REG_B);
        break;

    case 0xB1:
        // RES 6,C
        RES(6,&REG_C);
        break;

    case 0xB2:
        // RES 6,D
        RES(6,&REG_D);
        break;

    case 0xB3:
        // RES 6,E
        RES(6,&REG_E);
        break;

    case 0xB4:
        // RES 6,H
        RES(6,&REG_H);
        break;

    case 0xB5:
        // RES 6,L
        RES(6,&REG_L);
        break;

    case 0xB6: // To test !!! !!! !!!
        // RES 6,(HL)
        myHelperVal = get_1byteData(REG_HL);
        RES(6,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xB7:
        // RES 6,A
        RES(6,&REG_A);
        break;

    case 0xB8:
        // RES 7,B
        RES(7,&REG_B);
        break;

    case 0xB9:
        // RES 7,C
        RES(7,&REG_C);
        break;

    case 0xBA:
        // RES 7,D
        RES(7,&REG_D);
        break;

    case 0xBB:
        // RES 7,E
        RES(7,&REG_E);
        break;

    case 0xBC:
        // RES 7,H
        RES(7,&REG_H);
        break;

    case 0xBD:
        // RES 7,L
        RES(7,&REG_L);
        break;

    case 0xBE: // To test !!! !!! !!!
        // RES 7,(HL)
        myHelperVal = get_1byteData(REG_HL);
        RES(7,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xBF:
        // RES 7,A
        RES(7,&REG_A);
        break;

    case 0xC0:
        // SET 0,B
        SET_fnx(0,&REG_B);
        break;

    case 0xC1:
        // SET 0,C
        SET_fnx(0,&REG_C);
        break;

    case 0xC2:
        // SET 0,D
        SET_fnx(0,&REG_D);
        break;

    case 0xC3:
        // SET 0,E
        SET_fnx(0,&REG_E);
        break;

    case 0xC4:
        // SET 0,H
        SET_fnx(0,&REG_H);
        break;

    case 0xC5:
        // SET 0,L
        SET_fnx(0,&REG_L);
        break;

    case 0xC6: // To test !!! !!! !!!
        // SET 0,(HL)
        myHelperVal = get_1byteData(REG_HL);
        SET_fnx(0,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xC7:
        // SET 0,A
        SET_fnx(0,&REG_A);
        break;

    case 0xC8:
        // SET 1,B
        SET_fnx(1,&REG_B);
        break;

    case 0xC9:
        // SET 1,C
        SET_fnx(1,&REG_C);
        break;

    case 0xCA:
        // SET 1,D
        SET_fnx(1,&REG_D);
        break;

    case 0xCB:
        // SET 1,E
        SET_fnx(1,&REG_E);
        break;

    case 0xCC:
        // SET 1,H
        SET_fnx(1,&REG_H);
        break;

    case 0xCD:
        // SET 1,L
        SET_fnx(1,&REG_L);
        break;

    case 0xCE: // To test !!! !!! !!!
        // SET 1,(HL)
        myHelperVal = get_1byteData(REG_HL);
        SET_fnx(1,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xCF:
        // SET 1,A
        SET_fnx(1,&REG_A);
        break;

    case 0xD0:
        // SET 2,B
        SET_fnx(2,&REG_B);
        break;

    case 0xD1:
        // SET 2,C
        SET_fnx(2,&REG_C);
        break;

    case 0xD2:
        // SET 2,D
        SET_fnx(2,&REG_D);
        break;

    case 0xD3:
        // SET 2,E
        SET_fnx(2,&REG_E);
        break;

    case 0xD4:
        // SET 2,H
        SET_fnx(2,&REG_H);
        break;

    case 0xD5:
        // SET 2,L
        SET_fnx(2,&REG_L);
        break;

    case 0xD6: // To test !!! !!! !!!
        // SET 2,(HL)
        myHelperVal = get_1byteData(REG_HL);
        SET_fnx(2,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xD7:
        // SET 2,A
        SET_fnx(2,&REG_A);
        break;

    case 0xD8:
        // SET 3,B
        SET_fnx(3,&REG_B);
        break;

    case 0xD9:
        // SET 3,C
        SET_fnx(3,&REG_C);
        break;

    case 0xDA:
        // SET 3,D
        SET_fnx(3,&REG_D);
        break;

    case 0xDB:
        // SET 3,E
        SET_fnx(3,&REG_E);
        break;

    case 0xDC:
        // SET 3,H
        SET_fnx(3,&REG_H);
        break;

    case 0xDD:
        // SET 3,L
        SET_fnx(3,&REG_L);
        break;

    case 0xDE: // To test !!! !!! !!!
        // SET 3,(HL)
        myHelperVal = get_1byteData(REG_HL);
        SET_fnx(3,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xDF:
        // SET 3,A
        SET_fnx(3,&REG_A);
        break;


    case 0xE0:
        // SET 4,B
        SET_fnx(4,&REG_B);
        break;

    case 0xE1:
        // SET 4,C
        SET_fnx(4,&REG_C);
        break;

    case 0xE2:
        // SET 4,D
        SET_fnx(4,&REG_D);
        break;

    case 0xE3:
        // SET 4,E
        SET_fnx(4,&REG_E);
        break;

    case 0xE4:
        // SET 4,H
        SET_fnx(4,&REG_H);
        break;

    case 0xE5:
        // SET 4,L
        SET_fnx(4,&REG_L);
        break;

    case 0xE6: // To test !!! !!! !!!
        // SET 4,(HL)
        myHelperVal = get_1byteData(REG_HL);
        SET_fnx(4,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xE7:
        // SET 4,A
        SET_fnx(4,&REG_A);
        break;

    case 0xE8:
        // SET 5,B
        SET_fnx(5,&REG_B);
        break;

    case 0xE9:
        // SET 5,C
        SET_fnx(5,&REG_C);
        break;

    case 0xEA:
        // SET 5,D
        SET_fnx(5,&REG_D);
        break;

    case 0xEB:
        // SET 5,E
        SET_fnx(5,&REG_E);
        break;

    case 0xEC:
        // SET 5,H
        SET_fnx(5,&REG_H);
        break;

    case 0xED:
        // SET 5,L
        SET_fnx(5,&REG_L);
        break;

    case 0xEE: // To test !!! !!! !!!
        // SET 5,(HL)
        myHelperVal = get_1byteData(REG_HL);
        SET_fnx(5,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xEF:
        // SET 5,A
        SET_fnx(5,&REG_A);
        break;

    case 0xF0:
        // SET 6,B
        SET_fnx(6,&REG_B);
        break;

    case 0xF1:
        // SET 6,C
        SET_fnx(6,&REG_C);
        break;

    case 0xF2:
        // SET 6,D
        SET_fnx(6,&REG_D);
        break;

    case 0xF3:
        // SET 6,E
        SET_fnx(6,&REG_E);
        break;

    case 0xF4:
        // SET 6,H
        SET_fnx(6,&REG_H);
        break;

    case 0xF5:
        // SET 6,L
        SET_fnx(6,&REG_L);
        break;

    case 0xF6: // To test !!! !!! !!!
        // SET 6,(HL)
        myHelperVal = get_1byteData(REG_HL);
        SET_fnx(6,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xF7:
        // SET 6,A
        SET_fnx(6,&REG_A);
        break;

    case 0xF8:
        // SET 7,B
        SET_fnx(7,&REG_B);
        break;

    case 0xF9:
        // SET 7,C
        SET_fnx(7,&REG_C);
        break;

    case 0xFA:
        // SET 7,D
        SET_fnx(7,&REG_D);
        break;

    case 0xFB:
        // SET 7,E
        SET_fnx(7,&REG_E);
        break;

    case 0xFC:
        // SET 7,H
        SET_fnx(7,&REG_H);
        break;

    case 0xFD:
        // SET 7,L
        SET_fnx(7,&REG_L);
        break;

    case 0xFE: // To test !!! !!! !!!
        // SET 7,(HL)
        myHelperVal = get_1byteData(REG_HL);
        SET_fnx(7,&myHelperVal);
        write_1byteData( REG_HL, myHelperVal );
        break;

    case 0xFF:
        // SET 7,A
        SET_fnx(7,&REG_A);
        break;

    default:
        printf("Sonderfunktion nicht unterstuetzt: 0x%02X at addr.:%d\n",prog->opcode,PC);
        while(1){  }
        break;
    }
    // All 0xCB functions takes additional 1 byte
    INCREMENT( 1 );
}


//INFO
// Flag: F: Z N H C 0 0 0 0

// Read op code pointed by pc
void gb_opcode_fetch( )
{
    prog->opcode = prog->memory[ PC ];
    #if (DEBUG_OPM==1)
        printf("After exec: 0x%02X @ 0x%04X: ",prog->opcode,PC);
    #endif //DEBUG_OPM
}

