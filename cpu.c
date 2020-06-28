#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include "display.h"
#include "bios.h"
#include "lcd.h"

// Define Timers
#define TIMER_DEVIDER_REGISTER_FF04  (prog->memory[0xFF04])
#define TIMER_COUNTER_FF05           (prog->memory[0xFF05])
#define TIMER_INIT_LOAD_FF06         (prog->memory[0xFF06])
#define TIMER_CONTROL_REGISTER_FF07  (prog->memory[0xFF07])

// Interrupt registers
#define IME                    (prog->IME)
// Interrupt enable registers
#define V_BLANK_IR_ENABLED     (prog->memory[0xFFFF] & (1<<0))
#define LCD_STAT_IR_ENABLED    (prog->memory[0xFFFF] & (1<<1))
#define TIMER_IR_ENABLED       (prog->memory[0xFFFF] & (1<<2))
#define SERIAL_IR_ENABLED      (prog->memory[0xFFFF] & (1<<3))
#define JOYPAD_IR_ENABLED      (prog->memory[0xFFFF] & (1<<4))
// Interrupt flags registers
#define V_BLANK_IR_SET         (prog->memory[0xFF0F] & (1<<0))
#define LCD_STAT_IR_SET        (prog->memory[0xFF0F] & (1<<1))
#define TIMER_IR_SET           (prog->memory[0xFF0F] & (1<<2))
#define SERIAL_IR_SET          (prog->memory[0xFF0F] & (1<<3))
#define JOYPAD_IR_SET          (prog->memory[0xFF0F] & (1<<4))


#define CPU_SPEED_GAMEBOY             4194304

// ####################################################################################
// Initialisierung
gameboy* gb_start( )
{
    gameboy* p = malloc(sizeof(gameboy));

    p->pc           = 0x0100;
    p->sp           = 0xfffe;
    p->reg.af       = 0x01b0;
    p->reg.bc       = 0x0013;
    p->reg.de       = 0x00d8;
    p->reg.hl       = 0x014d;
    // p->ticks        = 0x0000;
    // Nur zum Spaß --> Gameboy Logo Zeigen
    p->bios_done    = 0;
    // alle Tasten als nicht gedrueckt initialisieren
    p->keys = 0xFF;

    // Buttons initialize to not pressed (high!)
    p->memory[0xFF00] = 0b00001111;

    p->memory[0xFF10] = 0x80 ;
    p->memory[0xFF11] = 0xBF ;
    p->memory[0xFF12] = 0xF3 ;
    p->memory[0xFF14] = 0xBF ;
    p->memory[0xFF16] = 0x3F ;
    p->memory[0xFF17] = 0x00 ;
    p->memory[0xFF19] = 0xBF ;
    p->memory[0xFF1A] = 0x7F ;
    p->memory[0xFF1B] = 0xFF ;
    p->memory[0xFF1C] = 0x9F ;
    p->memory[0xFF1E] = 0xBF ;
    p->memory[0xFF20] = 0xFF ;
    p->memory[0xFF21] = 0x00 ;
    p->memory[0xFF22] = 0x00 ;
    p->memory[0xFF23] = 0xBF ;
    p->memory[0xFF24] = 0x77 ;
    p->memory[0xFF25] = 0xF3 ;
    p->memory[0xFF26] = 0xF1 ;
    p->memory[0xFF40] = 0x91 ;
    // Nicht sicher
    p->memory[0xFF41] = 0x02 ;

    p->memory[0xFF42] = 0x00 ;
    p->memory[0xFF43] = 0x00 ;
    p->memory[0xFF45] = 0x00 ;
    p->memory[0xFF47] = 0xE4 ; // !!!
    p->memory[0xFF48] = 0xFF ;
    p->memory[0xFF49] = 0xFF ;
    p->memory[0xFF4A] = 0x00 ;
    p->memory[0xFF4B] = 0x00 ;

    // Timer
    p->deviderVariable = 0x00;
    p->memory[0xFF05]  = 0x00;
    p->memory[0xFF06]  = 0x00;
    p->memory[0xFF07]  = 0x00;
    p->timer           = 0x00;
    p->clockSpeedCycles= CPU_SPEED_GAMEBOY/4096;
    p->tikz = 0;
    p->displayMode = 4;

    // Interrupts
    p->IR_req = 0;
    p->memory[0xFF0F] = 0x00 ;

    p->memory[0xFFFF] = 0x00 ;
    p->memory[0xFFFF] |= (1<<4);

    return p;
}

// ####################################################################################
// Lade Cartrige in den Speicher
uint8_t gb_program_load( char* filename )
{
    printf("ROM is loading please wait... ");

    // Datei öffnen und binär lesen
    FILE* f = fopen(filename, "rb");

    if (f == NULL)
    {
        printf("could not find \"%s\"\n", filename);
        return 1;
    }

    // Dateigröße bestimmen
    fseek(f, 0, SEEK_END);
    uint32_t size = ftell(f);
    rewind(f);

    if ( size > (MEM_SIZE) )
    {
        printf("Not enough memory: %i", size);
        return 1;
    }

    // Speichern der Datei im Buffer
    fread(prog->memory, 1, 0x200000, f);

    load_bios( prog );

    printf("DONE! %d.%d kB added. (0x%02X)\n", ((size)/1024), ((size) % 1024),size );

    return 0;
}

// ####################################################################################
// ####################################################################################
// Herzstueck des Gameboys
void gb_program_cycle( )
{
    while( prog->tikz < MAX_TICKS )
    {
        gb_opcode_fetch( );
        gb_opcode_exec( ); // HALT wird im Exec abgefangen

        gb_update_timer( prog->tikz );
        LCD_control( prog->tikz );
        gb_interrupts( );
    }
    // Displayausgabe
    gb_ShowScreen();
}

/**************************************************************************************
*  Timer and Divider Registers
*       FF04 - DIV - Divider Register (R/W) --> Incremented at rate of 16384Hz
*       FF05 - TIMA - Timer counter (R/W) --> Incremented by a clock frequency specified
*           by the TAC register ($FF07). When the value overflows (gets bigger than FFh)
*           then it will be reset to the value specified in TMA (FF06), and an
*           interrupt will be requested, as described below.
*       FF06 - TMA - Timer Modulo (R/W) --> When the TIMA overflows, this data will be loaded.
*       FF07 - TAC - Timer Control (R/W)
*            Bit 2    - Timer Stop  (0=Stop, 1=Start)
*            Bits 1-0 - Input Clock Select
*                00:   4096 Hz    (~4194 Hz SGB)
*                01: 262144 Hz  (~268400 Hz SGB)
*                10:  65536 Hz   (~67110 Hz SGB)
*                11:  16384 Hz   (~16780 Hz SGB)
*       INT 50 - Timer Interrupt
**************************************************************************************/
// Nicht sicher ob es so passt!
void gb_update_timer( uint16_t cycles )
{
    // Do only if timer is started bit2 = 1
    if ( TIMER_CONTROL_REGISTER_FF07 & 0x04 )
    {
        TIMER_COUNTER_FF05 += cycles;
        if( TIMER_COUNTER_FF05 > 0xFF )
        {
            TIMER_COUNTER_FF05 = TIMER_INIT_LOAD_FF06;
            // Set Timer interrupt INT 50
            prog->memory[ 0xff0f ] |= (1 << 2);
        }
    }

    // DeviderRegister The Devider Is always Counting!
    prog->deviderVariable += cycles;
    if( prog->deviderVariable >= CPU_SPEED_GAMEBOY/16384 )
    {
        prog->deviderVariable = 0;
        TIMER_DEVIDER_REGISTER_FF04 ++;
    }
}

/**************************************************************************************
*  Interrupt handling function
*     Bit 0: V-Blank  Interrupt Request (INT 40h)  (1=Request)
*     Bit 1: LCD STAT Interrupt Request (INT 48h)  (1=Request)
*     Bit 2: Timer    Interrupt Request (INT 50h)  (1=Request)
*     Bit 3: Serial   Interrupt Request (INT 58h)  (1=Request)
*     Bit 4: Joypad   Interrupt Request (INT 60h)  (1=Request)
**************************************************************************************/
void gb_interrupts( )
{
    static uint16_t programCounterBeforeIR = 0;
    static uint8_t  waitForFinishActIR = 0;

    // Einlesen der Tasten, aber noch kein Schreiben auf 0xFF00
    gb_keys();

    // Pruefen ob bereits ein Interrupt ausgeführt wird, falls ja dann warte bis
    //    Ausfuehrung beendet worden ist. Dazu wird auf den Steck gelegten PC vor der
    //    Interruptausfuehrung gelegten Wert gewartet.
    if( waitForFinishActIR && (programCounterBeforeIR == PC) )
    {
        waitForFinishActIR = 0;
    }

    // Nur wenn Interrupt Master Enable Flag gesetzt ist
    if( IME && !waitForFinishActIR ) //
    {
        // highest priority (0) have to be executed first if requested
        // First V-Blank interrupt because of the highest priority
        if( V_BLANK_IR_ENABLED && V_BLANK_IR_SET )
        {
            programCounterBeforeIR = safePcAndUnsetIr( 0 );
            waitForFinishActIR = 1;
            // Trigger INT 40h
            PC = 0x40;
        }

        else if( LCD_STAT_IR_ENABLED && LCD_STAT_IR_SET )
        {
            programCounterBeforeIR = safePcAndUnsetIr( 1 );
            waitForFinishActIR = 1;
            // Trigger INT 40h
            PC = 0x48;
        }

        else if( TIMER_IR_ENABLED && TIMER_IR_SET )
        {
            programCounterBeforeIR = safePcAndUnsetIr( 2 );
            waitForFinishActIR = 1;
            // Trigger INT 40h
            PC = 0x50;
        }
        // Wird nicht unbedingt benötigt
        else if( SERIAL_IR_ENABLED && SERIAL_IR_SET )
        {
            // TBD
        }
        // Fast niemaden interessiert diese Funktion :-) !!!
        else if( JOYPAD_IR_ENABLED && JOYPAD_IR_SET )
        {
            programCounterBeforeIR = safePcAndUnsetIr( 4 );
            waitForFinishActIR = 1;
            // Trigger INT 60h
            PC = 0x60;
        }
        else
        {
            // nop: wenn nichts angefragt ist
        }
    }
}

// SubR fuer die IR Registersteuerung; Sperren der IRs und retten des PCs
uint16_t safePcAndUnsetIr(uint8_t IrNum)
{
    // Disable both IME and the IR Request
    IME = 0; // Nicht sicher !!!
    prog->memory[0xFF0F] &= ~(1 << IrNum);
    prog->halt = 0;
    push_to_stack( PC );
    return PC;
}

void gb_keys()
{
    static uint16_t lastState = 0;
    if( prog->keys != lastState )
    {
        //JOYPAD_IR_SET
        prog->memory[0xFF0F] |= (1<<4);
        lastState = prog->keys;
    }
}

/************************************************************************************
* Umwaldeln der Tasten in gb format
*    Bit 7 - Not used
*    Bit 6 - Not used
*    Bit 5 - P15 Select Button Keys      (0=Select)
*    Bit 4 - P14 Select Direction Keys   (0=Select)
*    Bit 3 - P13 Input Down  or Start    (0=Pressed) (Read Only)
*    Bit 2 - P12 Input Up    or Select   (0=Pressed) (Read Only)
*    Bit 1 - P11 Input Left  or Button B (0=Pressed) (Read Only)
*    Bit 0 - P10 Input Right or Button A (0=Pressed) (Read Only)
************************************************************************************/
uint8_t getGbButtonState( uint8_t iVal )
{
    uint8_t iRet = 0;
    // Wenn Pfeiltasten angefordert werden:
    if( iVal & ( 1 << 4 ) )
        iRet = ((  prog->keys  ) & 0x0F) | 0b11010000;
    // Wenn Funktionstasten angefragt werden:
    else
        iRet = (( prog->keys >> 4 ) & 0x0F) | 0b11100000;

    //printf("Bekommen: 0x%02X Gesendet: 0x%02X und Rohwert: 0x%02X \n",iVal,iRet,prog->keys);

    return iRet;
}


/*************************************************************************************
*   ##################################################################################
*   ##################################################################################
*
*   Hilfsfunktionen des Gameboys
*   Lesen und schreiben auf den Speicher uns sonstiges fuer die Op Abarbeitung
*
*   ##################################################################################
*   ##################################################################################
*************************************************************************************/
// Lese 1 Byte direkt von der nächsten Stelle oder aus der Adresse
uint8_t get_1byteData(  )
{
    return ( prog->memory[ prog->pc + 1 ] );
}

uint8_t get_1byteDataFromAddr(  uint16_t addr )
{
    if( addr == 0xFF00 )
    {
        // FF00 enthaelt die Infos ueber die Tasten -> dabei ist 0 gedrueckt
        return getGbButtonState( prog->memory[0xFF00] );
    }
    else
    {
        // return whats in memory :-)
        return ( prog->memory[ addr ] );
    }
}

/*************************************************************************************
*   Schreibe ein Byte ins RAM, kein Schreiben auf das ROM erlauben
*************************************************************************************/
void write_1byteData( uint16_t addr, uint8_t data )
{
    // Kein Schreiben auf das ROM erlaubt
    if ( (addr < 0x8000) || (( addr >= 0xFEA0 ) && ( addr < 0xFEFF)) )
    {
        // nop
    }
    // writing to ECHO ram also writes in RAM
    else if ( ( addr >= 0xE000 ) && (addr < 0xFE00) )
    {
        prog->memory[ addr ] = data;
        write_1byteData(addr-0x2000, data) ;
    }
    // nicht sicher wie das funktioniert!!!
    else if( addr == 0xFF07 )
    {
       // https://gbdev.gg8.se/wiki/articles/Timer_and_Divider_Registers
       switch( data & 0x03 )
        {
            case 0: prog->clockSpeedCycles = CPU_SPEED_GAMEBOY/4096; break ;//1024
            case 1: prog->clockSpeedCycles = CPU_SPEED_GAMEBOY/268400; break;//16
            case 2: prog->clockSpeedCycles = CPU_SPEED_GAMEBOY/65536; break ;//64
            case 3: prog->clockSpeedCycles = CPU_SPEED_GAMEBOY/16384; break ;//256
            default: break ;
        }
    }
    // Writing any value to register 0xff04 resets it to 0x00
    else if( addr == 0xFF04 )
    {
        prog->memory[ 0xFF04 ] = 0x00;
    }
    else
    {
       prog->memory[ addr ] = data;
    }
}

/*************************************************************************************
*   Schreibe zwei Bytes ins RAM --> 2x Aufruf der Funktion: "write_1byteData"
*************************************************************************************/
void write_2byteData( uint16_t addr, uint16_t data )
{
    write_1byteData( addr, data & 0xff);
    write_1byteData( addr + 1, ((data >> 8) & 0xff) );
}

// ---
uint16_t get_2byteData( )
{
    uint16_t msb = prog->memory[ prog->pc + 2 ] & 0xff;
    uint16_t lsb = prog->memory[ prog->pc + 1 ] & 0xff;
    return ( msb << 8 ) | ( lsb );
}

// ---
uint16_t get_2byteDataFromAddr( uint16_t addr )
{
    return ((prog->memory[ addr + 1 ] << 8) & 0xff00) + (prog->memory[ addr ] & 0xff);
}

// ---
int get_1byteSignedData( )
{
    if( (prog->memory[ prog->pc + 1 ] & 0b10000000) == 0 )
        return (uint8_t)( prog->memory[ prog->pc + 1 ] & 0x7F );
    else
        return (uint8_t)(~(prog->memory[ prog->pc + 1 ]-0x01))*-1;
}

// ---
int get_1byteSignedDataFromAddr( uint16_t addr )
{
    if( (prog->memory[ addr ] & 0b10000000) == 0 )
        return (uint8_t)( prog->memory[ addr ] & 0x7F );
    else
        return (uint8_t)(~(prog->memory[ addr ]-0x01))*-1;
}

// TESTED!
void push_to_stack( uint16_t data )
{
        prog->memory[ prog->sp ] = ((data >> 8) & 0xff);
        prog->sp--;
        prog->memory[ prog->sp ] = (data & 0xff);
        prog->sp--;

        //print stack
//        uint16_t i = prog->sp + 1;
//        while( i <= 0xCFFF )
//        {
//            printf(" --- 0x %02X %02X\n", prog->memory[ i+1 ], prog->memory[ i ]);
//            i +=2;
//        }
}
// TESTED!
uint16_t read_from_stack( )
{
    uint16_t iRet = 0x0;
    prog->sp ++;
    iRet  = (prog->memory[ prog->sp ] & 0xff);
    prog->sp ++;
    iRet |= ((prog->memory[ prog->sp ] << 8) & 0xff00);
    return iRet;
}

// Interrupt Flag (R/W)
//  Bit 0: V-Blank  Interrupt Request (INT 40h)  (1=Request)
//  Bit 1: LCD STAT Interrupt Request (INT 48h)  (1=Request)
//  Bit 2: Timer    Interrupt Request (INT 50h)  (1=Request)
//  Bit 3: Serial   Interrupt Request (INT 58h)  (1=Request)
//  Bit 4: Joypad   Interrupt Request (INT 60h)  (1=Request)
void setInterrupt( uint8_t ISR )
{
    prog->memory[0xFF0F] |= (1<<ISR);
}
void resetInterrupt( uint8_t ISR )
{
    prog->memory[0xFF0F] &= ~(1<<ISR);
}

// HILFSFUNKTIONEN :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void setFlags( uint8_t Z, uint8_t N, uint8_t H, uint8_t C )
{
    // Zero flag handling
    if( Z == 1 )
        prog->reg.f |= 0b10000000;
    else if( Z == 0 )
        prog->reg.f &= 0b01111111;
    else if ( Z == 2 )
    {
        if(  prog->reg.f & 0b10000000 )
            prog->reg.f &= 0b01111111;
        else
            prog->reg.f |= 0b10000000;
    }
    else
    {
        // nop
    }

    // Negative flag handling
    if( N == 1 )
        prog->reg.f |= 0b01000000;
    else if( N == 0 )
        prog->reg.f &= 0b10111111;
    else if ( N == 2 )
    {
        if(  prog->reg.f & 0b01000000 )
            prog->reg.f &= 0b10111111;
        else
            prog->reg.f |= 0b01000000;
    }
    else
    {
        // nop
    }

    // Half carry flag handling
    if( H == 1 )
        prog->reg.f |= 0b00100000;
    else if( H == 0 )
        prog->reg.f &= 0b11011111;
    else if ( H == 2 )
    {
        if(  prog->reg.f & 0b00100000 )
            prog->reg.f &= 0b11011111;
        else
            prog->reg.f |= 0b00100000;
    }
    else
    {
        // nop
    }

    // Carry flag handling
    if( C == 1 )
        prog->reg.f |= 0b00010000;
    else if( C == 0 )
        prog->reg.f &= 0b11101111;
    else if ( C == 2 )
    {
        if(  prog->reg.f & 0b00010000 )
            prog->reg.f &= 0b11101111;
        else
            prog->reg.f |= 0b00010000;
    }
    else
    {
        // nop
    }
}

uint8_t getZeroFlag(  )
{
    return (( prog->reg.f & 0x80 ) >> 7);
}

uint8_t getNegativeFlag(  )
{
    return (( prog->reg.f & 0x40 ) >> 6);
}

uint8_t getHalfCarryFlag(  )
{
    return (( prog->reg.f & 0x20 ) >> 5);
}

uint8_t getCarryFlag(  )
{
    return (( prog->reg.f & 0x10 ) >> 4);
}

int get_signed_8(uint8_t data)
{
    int iRet = 0;
    if( data & 0x80 )
    {
        //negative Zahl
        iRet = (~(data - 1)&0xff) * (-1);
    }
    else
    {
        //positive Zahl
        iRet = data & 0xff;
    }
    return iRet;
}

void setFlags_for_Add_1Byte(  uint8_t oldVal, uint8_t valToAdd )
{
    setFlags( 0, 0, 0, 0 );
    // Zero flag
    if( ((oldVal+valToAdd)&0xff) == 0x0 )
        setFlags( 1, 3, 3, 3 );
    // Half carry flag
    if( (( oldVal & 0xf) + (valToAdd & 0xf) ) & 0x10 )
        setFlags( 3, 3, 1, 3 );
    // Carry flag
    if( (oldVal + valToAdd) > 0xff )
        setFlags( 3, 3, 3, 1 );
}

void setFlags_for_Add_2Byte(  uint16_t oldVal, uint16_t valToAdd )
{
    uint16_t result = oldVal + valToAdd;
    setFlags( 3, 0, 0, 0 );

    if (result & 0x10000)
        setFlags( 3, 0, 3, 1 );
    if ((oldVal ^ valToAdd ^ (result & 0xFFFF)) & 0x1000)
        setFlags( 3, 0, 1, 3 );
}

uint8_t setFlags_for_Adc_1Byte(  uint8_t oldVal, uint8_t valToAdd )
{
    uint8_t iRet = 0;

    if( getCarryFlag(prog) )
        iRet = iRet + 1;

    setFlags( 0, 0, 0, 0 );
    // Zero flag
    if( ((oldVal+valToAdd+iRet)&0xff) == 0x0 )
        setFlags( 1, 3, 3, 3 );
    // Half carry flag
    if( (( oldVal & 0xf) + ((valToAdd+iRet) & 0xf) ) & 0x10 )
        setFlags( 3, 3, 1, 3 );
    // Carry flag
    if( (oldVal + valToAdd + iRet) > 0xff )
        setFlags( 3, 3, 3, 1 );

    return (iRet + valToAdd);
}

// to check FAIL
void setFlags_for_Sub_1Byte(  uint8_t oldVal, uint8_t valToSub )
{
    setFlags( 0, 1, 0, 0 );
    // Zero flag
    if( oldVal == valToSub )
        setFlags( 1, 3, 3, 3 );
    // Half Carry flag !!!
    if (((oldVal - valToSub) & 0xF) > (oldVal & 0xF))
        setFlags( 3, 3, 1, 3 );
    // Carry flag
    if( oldVal < valToSub )
        setFlags( 3, 3, 3, 1 );
}

void setFlags_for_CP(  uint8_t oldVal, uint8_t valToSub )
{
    // Z N HC C
    setFlags( 0, 1, 0, 0 );
    // Zero flag
    if( oldVal == valToSub )
        setFlags( 1, 3, 3, 3 );
    // Half Carry flag !!!
    if (((oldVal - valToSub) & 0xF) > (oldVal & 0xF))
    {
        setFlags( 3, 3, 1, 3 );
    }
    // Carry flag
    if( oldVal < valToSub )
        setFlags( 3, 3, 3, 1 );
}

// to check
uint8_t setFlags_for_Sbc_1Byte(  uint8_t oldVal, uint8_t valToSub )
{
    uint8_t iRet = 0;

    if( getCarryFlag(prog) )
        iRet = 1;

    setFlags( 0, 1, 0, 0 );
    // Zero flag
    if( ((oldVal-valToSub-iRet)&0xff) == 0x0 )
        setFlags( 1, 3, 3, 3 );
    // Half carry flag
    if( (( oldVal & 0xf) - ((valToSub+iRet) & 0xf) ) & 0x10 )
        setFlags( 3, 3, 1, 3 );
    // Carry flag
    if( oldVal < (valToSub+iRet) )
        setFlags( 3, 3, 3, 1 );

    return (iRet + valToSub);
}

// to check
void setFlags_for_And_1Byte(  uint8_t one, uint8_t two )
{
    setFlags( 0, 0, 1, 0 );
    // Zero flag
    if( ((one & two)& 0xff) == 0x0 )
        setFlags( 1, 3, 3, 3 );
}

// to check
void setFlags_for_Xor_1Byte(  uint8_t one, uint8_t two )
{
    // Zero flag
    if( ((one ^ two)& 0xff) == 0x00 )
        setFlags( 1, 0, 0, 0 );
    else
        setFlags( 0, 0, 0, 0 );

}

// to check
void setFlags_for_Or_1Byte(  uint8_t one, uint8_t two )
{
    setFlags( 0, 0, 0, 0 );
    // Zero flag
    one |= two;
    if( one == 0x00 )
        setFlags( 1, 3, 3, 3 );
}

void setFlags_for_Inc_1Byte(  uint8_t oldVal )
{
    setFlags( 0, 0, 0, 3 );
    // Zero flag
    if( ((oldVal+1)&0xff) == 0x0 )
        setFlags( 1, 3, 3, 3 );
    // Half carry flag
    if( (( oldVal & 0xf) + 0x1 ) & 0x10 )
        setFlags( 3, 3, 1, 3 );
}

void setFlags_for_Dec_1Byte(  uint8_t oldVal )
{
    setFlags( 0, 1, 0, 3 );
    // Zero flag
    if( ((oldVal-1)&0xff) == 0x0 )
        setFlags( 1, 3, 3, 3 );
    // Half Carry flag
    if( (( oldVal & 0xf) - 0x1 ) & 0x10 )
        setFlags( 3, 3, 1, 3 );
}

void setFlagsForAdd2Byte( uint16_t a, uint16_t b)
{
    setFlags(0,0,0,0);
    if( ((a & 0xFFF) + (b & 0xFFF))>0xFFF )
        setFlags(3,3,1,3);
    if( (a+b) > 0xFFFF )
        setFlags(3,3,3,1);
}

void do_DAA(  )
{
    // Korrektur für Subtraktion
    if( getNegativeFlag( prog ) )
    {
        if( getHalfCarryFlag( prog ) )
        {
            prog->reg.a = (prog->reg.a - 0x6) & 0xFF;
        }
        if( getCarryFlag( prog ) )
        {
            prog->reg.a = prog->reg.a - 0x60;
        }
    }
    // Korrektur für Addition
    else
    {
        if( ((prog->reg.a & 0x0f) > 0x9) || getHalfCarryFlag( prog ) )
        {
            prog->reg.a = prog->reg.a + 0x6;
        }
        if( (((prog->reg.a & 0xf0)>>4) > 0x9) || getCarryFlag( prog ) )
        {
            prog->reg.a = prog->reg.a + 0x60;
            setFlags( 3,3,3,1);
        }
    }
    if((prog->reg.a & 0xff) == 0x0)
        setFlags( 1,3,3,3);

    setFlags( 3,3,0,3);
}

void BIT(uint8_t bit, uint8_t val)
{
    setFlags( 0, 0, 1, 3);
    // write bit x to flag Z
    if( val & (1 << bit) )
        setFlags( 1, 0, 1, 3);
}

// ToDo: TEST
void RL(uint8_t *val)
{
    uint8_t oldval = *val;
    *val = ((*val << 1) & 0xFE) | (getCarryFlag( prog ) & 0x01);
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x80 )
        setFlags(3,3,3,1);
    if( *val == 0 )
        setFlags(1,3,3,3);
}

// ToDo: TEST
void RR(uint8_t *val)
{
    uint8_t oldval = *val;
    *val = ((*val >> 1) & 0x7F) | ((getCarryFlag( prog ) & 0x01) << 7);
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x01 )
        setFlags(3,3,3,1);
    if( *val == 0 )
        setFlags(1,3,3,3);
}

// ToDo: TEST
void SLA(uint8_t *val)
{
    uint8_t oldval = *val;
    *val = ((*val << 1) & 0xFE);
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x80 )
        setFlags(3,3,3,1);
    if( *val == 0 )
        setFlags(1,3,3,3);
}

// ToDo: TEST
void SRA(uint8_t *val)
{
    uint8_t oldval = *val;
    uint8_t msb = *val & 0x80;
    *val = ((*val >> 1) & 0x7F) | msb;
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x01 )
        setFlags(3,3,3,1);
    if( *val == 0 )
        setFlags(1,3,3,3);
}

// ToDo: TEST
void SRL(uint8_t *val)
{
    uint8_t oldval = *val;
    *val = ((*val >> 1) & 0x7F);
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x01 )
        setFlags(3,3,3,1);
    if( *val == 0 )
        setFlags(1,3,3,3);
}

// Tested!
void SWAP(uint8_t *val)
{
    *val = (((*val & 0xF0) >> 4) | ((*val & 0x0F) << 4));
    setFlags(0,0,0,0);
    if(*val == 0)
        setFlags(1,0,0,0);
}

// ToDo: TEST
void RLA(uint8_t *val)
{
    uint8_t oldval = *val;
    *val = ((*val << 1) & 0xFE) | (getCarryFlag( prog ) & 0x01);
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x80 )
        setFlags(3,3,3,1);
}

void RLCA(uint8_t *val)
{
    uint8_t oldval = *val;
    *val = ((*val << 1) & 0xFE) | ((*val & 0x80)>>7);
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x80 )
        setFlags(3,3,3,1);
}

void RRA(uint8_t *val)
{

    uint8_t oldval = *val;
    *val = (((oldval >> 1)) | ( getCarryFlag( prog ) << 7 ));
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x01 )
        setFlags(3,3,3,1);
}

void RRCA(uint8_t *val)
{
    uint8_t oldval = *val;
    *val = (*val >> 1) | ((*val & 0x01)<<7);
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x01 )
        setFlags(3,3,3,1);
}

void SET_fnx(uint8_t bit, uint8_t *val)
{
    *val |=  (1 << bit);
}

void RES(uint8_t bit, uint8_t *val)
{
    *val &= ~(1 << bit);
}





void RLC(uint8_t *val)
{
    uint8_t oldval = *val;
    *val = ((*val << 1) & 0xFE) | ((*val & 0x80)>>7);
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x80 )
        setFlags(3,3,3,1);
    if( *val == 0 )
        setFlags(1,3,3,3);
}

void RRC(uint8_t *val)
{
    uint8_t oldval = *val;
    *val = (*val >> 1) | ((*val & 0x01)<<7);
    // Set flags
    setFlags(0,0,0,0);
    if( oldval & 0x01 )
        setFlags(3,3,3,1);
    if( *val == 0 )
        setFlags(1,3,3,3);
}





// ####################################################################################
// Testfunktionen
void printBin(uint8_t val)
{
    unsigned i;
    for (i = 1 << 7; i > 0; i = i / 2)
        (val & i)? printf("1"): printf("0");
    fflush(stdout);
    printFlags( prog );
}
void printFlags( )
{
    printf(" Flags: %d %d %d %d (ZNHC)\n", getZeroFlag( prog ), getNegativeFlag( prog ), getHalfCarryFlag(prog), getCarryFlag(prog));
}
void RLCA_Test( gameboy *prog )
{
    uint8_t val = 0b10001001;
    printf("RCLA Test:\n");
    printBin(val);
    RLCA(&val);
    printBin(val);
    RLCA(&val);
    printBin(val);
    printf("\n");
}
void SWAP_Test( gameboy *prog )
{
    uint8_t val = 0b11001001;
    printf("SWAP Test:\n");
    printBin(val);
    SWAP(&val);
    printBin(val);
    SWAP(&val);
    printBin(val);
    printf("\n");
}
void RRCA_Test( gameboy *prog )
{
    uint8_t val = 0b10001001;
    printf("RRLA Test:\n");
    printBin(val);
    RRCA(&val);
    printBin(val);
    RRCA(&val);
    printBin(val);
    printf("\n");
}
void RLA_Test( gameboy *prog )
{
    uint8_t val = 0b10001001;
    printf("RLA Test:\n");
    printBin(val);
    RLA(&val);
    printBin(val);
    RLA(&val);
    printBin(val);
    printf("\n");
}
void RRA_Test( gameboy *prog )
{
    uint8_t val = 0b10001001;
    printf("RRA Test:\n");
    printBin(val);
    RRA(&val);
    printBin(val);
    RRA(&val);
    printBin(val);
    printf("\n");
}
int get_1byteSignedData_Test( uint8_t val )
{
    if( (val & 0b10000000) == 0 )
        return (uint8_t)( val & 0x7F );
    else
        return (uint8_t)(~(val-0x01))*-1;

}
