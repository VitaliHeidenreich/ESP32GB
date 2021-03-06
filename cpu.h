

#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/// Speichergr��e
#define     MEM_SIZE        0x10000
#define     ROM_MEM_SIZE    0x4000
#define     ROM_BANK_NUMB   4

/// Lets debug!
#define     DEBUG_OPM       0
#define     DEBUG_STEP      0


/// Zum Einstellen der Flags
#define     UNMOD           3
#define     NOT             2
#define     SET             1
#define     UNSET           0

/*************************************************************
* simplify register handling
*************************************************************/
#define     INCREMENT(x) (prog->pc = prog->pc + (x));

#define     CARRY_V 0xf

#define     REG_A   (prog->reg.a)
#define     REG_F   (prog->reg.f)
#define     REG_AF  (prog->reg.af)

#define     REG_C   (prog->reg.c)
#define     REG_B   (prog->reg.b)
#define     REG_BC  (prog->reg.bc)

#define     REG_D   (prog->reg.d)
#define     REG_E   (prog->reg.e)
#define     REG_DE  (prog->reg.de)

#define     REG_L   (prog->reg.l)
#define     REG_H   (prog->reg.h)
#define     REG_HL  (prog->reg.hl)


#define     PC      (prog->pc)
#define     SP      (prog->sp)
///***********************************************************

#define     MAX_TICKS     70224//69905

struct registers_t {
	struct {
		union {
			struct {
				unsigned char f;
				unsigned char a;
			};
			unsigned short af;
		};
	};

	struct {
		union {
			struct {
				unsigned char c;
				unsigned char b;
			};
			unsigned short bc;
		};
	};

	struct {
		union {
			struct {
				unsigned char e;
				unsigned char d;
			};
			unsigned short de;
		};
	};

	struct {
		union {
			struct {
				unsigned char l;
				unsigned char h;
			};
			unsigned short hl;
		};
	};

	unsigned short sp;
	unsigned short pc;
} registers_t;
typedef struct registers_t registers;

struct gameboy_t{

    // Speicher fuer den gelesenen Befehl aus dem ROM
    uint8_t opcode;
    // Programm pointer --> von Neumann Architektur
    uint16_t pc;
    // Stackpointer fuer ISR und Co
    uint16_t  sp;
    // Hauptspeicher
    uint8_t  memory[MEM_SIZE];
    // Rom-Banks
    uint8_t  rom_bank_mem[ROM_BANK_NUMB][ROM_MEM_SIZE];
    // Tasten --> Input 6 Buttons
    uint16_t  keys;
    // Struktur der Register AF BC DE HL
    registers reg;
    // Interrupt Master Enable Flag (Write Only)
    uint8_t IME;
    uint8_t IR_req;
    // F�r Grafikausgabe 160 Spalten und 144 Zeilen
    uint8_t ausgabeGrafik[144][160];

    // Timer
    uint16_t deviderVariable;
    uint16_t timer;
    uint8_t stop;
    uint8_t halt;
    uint16_t clockSpeedCycles;
    uint8_t timerEnabled;
    // Display
    uint8_t displayMode;
    // Speicher f�r die Zyklenzahl
    uint32_t tikz;
    // ROM Bank Management
    uint16_t rombanknumber;
    uint16_t rambank;
    uint8_t rambankenabled;
    uint8_t ram_mode_selected;
} gameboy_t;
typedef struct gameboy_t gameboy;

// ####################################################################################
// ####################################################################################
gameboy* prog;
// ####################################################################################
// ####################################################################################

// ####################################################################################
// ####################################################################################
// Hauptfunktionen des Gameboys
gameboy*    gb_start();
uint8_t     gb_program_load( char* filename);
// Hauptschleife CPU
void        gb_program_cycle( );
void        gb_opcode_fetch( );
void        gb_opcode_exec( );
// interrupts und co
void        gb_update_timer( uint16_t cycles );
void        gb_update_gfx( uint16_t cycles );
void        gb_interrupts( );
// Prefix funktionen --> Erweiterung zu z80
void        gb_exec_prefix( );
// ####################################################################################
void setInterrupt(uint8_t ISR);
void resetInterrupt(uint8_t ISR);
uint16_t safePcAndUnsetIr(uint8_t IrNum);
// ####################################################################################
// Hilfsfunktionen des Gameboys
uint16_t get_2byteData(  );
uint16_t get_2byteDataFromAddr( uint16_t addr );

int get_1byteSignedData( );
int get_1byteSignedData_Test( uint8_t val );
int get_1byteSignedDataFromAddr( uint16_t addr );

uint8_t get_1byteData( );
uint8_t get_1byteDataFromAddr( uint16_t addr );

void write_1byteData( uint16_t addr, uint8_t data );
void write_2byteData( uint16_t addr, uint16_t data );
// PUSH TO STACK AND READ FROM STACK
void push_to_stack( uint16_t data );
uint16_t read_from_stack( );

// SET AND GET FLAGS
void setFlags( uint8_t Z, uint8_t N, uint8_t H, uint8_t C );
void setFlags_for_Add_1Byte( uint8_t oldVal, uint8_t valToAdd );
void setFlags_for_Add_2Byte(  uint16_t oldVal, uint16_t valToAdd );
void ADC( uint8_t *val, uint8_t valToAdd );
void setFlags_for_Sub_1Byte( uint8_t oldVal, uint8_t valToSub );
void SBC( uint8_t *val, uint8_t valToSub );
void setFlags_for_And_1Byte(  uint8_t one, uint8_t two );
void setFlags_for_Xor_1Byte(  uint8_t one, uint8_t two );
void setFlags_for_Or_1Byte(  uint8_t one, uint8_t two );
void setFlags_for_Inc_1Byte(  uint8_t oldVal );
void setFlags_for_Dec_1Byte(  uint8_t oldVal );
void setFlagsForAdd2Byte( uint16_t a, uint16_t b);
void setFlags_for_CP(  uint8_t a, uint8_t b );

uint8_t getZeroFlag( );
uint8_t getNegativeFlag( );
uint8_t getHalfCarryFlag( );
uint8_t getCarryFlag( );

void setZeroFlag( uint8_t f );
void setNegativeFlag( uint8_t f );
void setHalfCarryFlag( uint8_t f );
void setCarryFlag( uint8_t f );

// GET SIGNED DATA
int get_signed_8(uint8_t data);

// DAA
void do_DAA( );

// Erweiterte Funktionen des GB
void BIT(uint8_t bit, uint8_t val);
void RL(uint8_t *val);
void RR(uint8_t *val);
void SLA(uint8_t *val);
void SRA(uint8_t *val);
void SRL(uint8_t *val);
void SWAP(uint8_t *val);
void RLA(uint8_t *val);
void RLCA(uint8_t *val);

void RRA(uint8_t *val);
void RRCA(uint8_t *val);

void RLC(uint8_t *val);
void RRC(uint8_t *val);


// Testfunktionen
void printBin(uint8_t val);
void printFlags( );
void RLCA_Test( );
void RRCA_Test( );
void SWAP_Test( );
void RRA_Test( );
void RLA_Test( );

// Delete and set bit
void SET_fnx(uint8_t bit, uint8_t *val);
void RES(uint8_t bit, uint8_t *val);

void printBinary(uint8_t val);

#endif
