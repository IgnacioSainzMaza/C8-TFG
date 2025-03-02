#ifndef CHIP8
#define CHIP8

#include <stdint.h>
#define MEMORY 65536
// Variable a la que se asocia el opcode que se ejecuta en cada paso de instrucción.
uint32_t opcode;
// Variable que almacena la memoria de la máquina.
uint8_t memory[(MEMORY)];
// Array que almacena los 16 registros de 8 bits de propósito general con los que cuenta la arquitectura.
uint16_t v[16];
// Variable que almacena el valor del registro I
uint16_t I;
// Variable que almacena el valor del Program Counter
uint16_t PC;
// Variable que almacena la pantalla para su dibujo. 
uint8_t gfx[128*64];
// Array que incluye la fuente en formato de sprites para su dibujo en pantalla (hexadecimal).
uint8_t fontset[80]={
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
// Variable asociada al timer de delay
uint16_t delay_timer;
// Variable asociada al timer de sonido
uint16_t sound_timer;
// Array que almacena la pila de la arquitectura para almacenar subrutinas y funciones.
uint16_t stack[16];
// Variable en que se aloja el Stack Pointer (puntero de pila).
uint16_t sp;
// Almacena el teclado hexadecimal que se emula, con disposición original de 123C-456D-789E-A0BF
uint8_t keyboard[16];

uint_fast8_t drawflag;

#endif