#ifndef CHIP64_H
#define CHIP64_H

#include <stdint.h>
#include <stdbool.h>
#include "config64.h"

// Definición del conjunto de fuentes en formato de sprites hexadecimales
static const uint8_t chip64_fontset[FONTSET_SIZE] = {
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


typedef struct {
    uint16_t opcode;              // Opcode actual
    uint16_t SP;                  // Stack Pointer
    uint16_t PC;                  // Program Counter

    uint8_t memory[MEMORY_SIZE];  // Memoria del sistema

    uint64_t V[REGISTER_COUNT];    // Registros V0-V31

    uint64_t I;                   // Registro índice

    uint8_t gfx[DISPLAY_WIDTH * DISPLAY_HEIGHT];         // Framebuffer principal
    uint8_t gfx2Buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];  // Buffer para efectos
    Color16 palette[MAX_COLORS];

    uint8_t delayTimer;           // Timer de retardo
    uint8_t soundTimer;           // Timer de sonido
    uint16_t stack[STACK_SIZE];   // Pila para guardar direcciones de retorno
    
    uint8_t key[KEY_COUNT];       // Estado del teclado
    bool drawFlag;                // Bandera para indicar si hay que actualizar la pantalla
    Config64 config;                // Configuración del emulador
    EmuMode mode; 

    GraphicsEffects currentEffect; // Efecto gráfico actual
    uint8_t effectTimer; // Temporizador para efectos gráficos
    uint8_t colorIndex; // Índice del color actual en el ciclo de colores
} Chip64;



/**
 * @brief Inicializa el emulador CHIP-64
 * 
 * - Resetea registros, memoria y display
 * - Carga fontset en memoria (0x000-0x04F)
 * - Carga paleta de colores por defecto
 * - Establece PC en 0x200 (punto de entrada estándar)
 * - Configura modo inicial (MODE_8BIT por compatibilidad)
 * 
 * @param chip64 Puntero a la estructura del emulador
 */
void chip64Init(Chip64* chip64);

/**
 * @brief Carga una ROM desde un archivo
 * 
 * Lee el archivo y lo carga en memoria a partir de 0x200.
 * Soporta ROMs de hasta 65024 bytes (64KB - 512 bytes reservados).
 * 
 * @param chip64 Puntero a la estructura del emulador
 * @param filename Ruta del archivo ROM
 * @return true si la carga fue exitosa, false en caso contrario
 */
bool chip64LoadROM(Chip64* chip64, const char* filename);

/**
 * @brief Ejecuta un ciclo de instrucción (fetch-decode-execute)
 * 
 * Todas las instrucciones están adaptadas para:
 * - Trabajar con registros de 64 bits (con máscaras según modo)
 * - Acceder a 64KB de memoria
 * - Dibujar en display de 128×64
 * - Usar los 32 registros disponibles
 * - Soportar sprites de 8×N, 16×16 y 32×32
 * 
 * @param chip64 Puntero a la estructura del emulador
 */
void chip64Cycle(Chip64* chip64);

/**
 * @brief Actualiza los temporizadores (delay y sound)
 * 
 * Debe llamarse a 60Hz para mantener el timing correcto.
 * 
 * @param chip64 Puntero a la estructura del emulador
 */
void chip64UpdateTimers(Chip64* chip64);


/**
 * @brief Establece el estado de una tecla
 * 
 * @param chip64 Puntero a la estructura del emulador
 * @param key Código de la tecla (0x0-0xF)
 * @param value 1 = presionada, 0 = liberada
 */
void chip64SetKey(Chip64* chip64, uint8_t key, uint8_t value);

/**
 * @brief Cambia el modo de emulación
 * 
 * Permite cambiar entre 8-bit (CHIP-8), 16-bit (CHIP-16) y 64-bit (CHIP-64).
 * 
 * Cambios automáticos según modo:
 * - MODE_8BIT:  Display 64×32, registros enmascaran a 8 bits
 * - MODE_16BIT: Display 64×32, registros enmascaran a 16 bits
 * - MODE_64BIT: Display 128×64, registros usan 64 bits completos
 * 
 * @param chip64 Puntero a la estructura del emulador
 * @param mode Modo a establecer
 */
void chip64SetMode(Chip64* chip64, EmuMode mode);

/**
 * @brief Activa/desactiva modo de color
 * 
 * @param chip64 Puntero a la estructura del emulador
 * @param enable true = 16 colores, false = monocromo (compatibilidad)
 */
void chip64SetColorMode(Chip64* chip64, bool enable);

/**
 * @brief Cambia la paleta de colores activa
 * 
 * @param chip64 Puntero a la estructura del emulador
 * @param newPalette Array de 16 colores en formato RGB565
 */
void chip64SetPalette(Chip64* chip64, const Color16* newPalette);

/**
 * @brief Establece el efecto gráfico activo
 * 
 * Los efectos se procesan en el buffer secundario (gfx2Buffer).
 * Esto permite mantener el framebuffer primario intacto.
 * 
 * Efectos disponibles:
 * - EFFECT_NONE: Sin efectos
 * - EFFECT_COLOR_CYCLE: Cicla a través de COLOR_PALETTE_RGBA
 * - EFFECT_FADE: Fade in/out (futuro)
 * - EFFECT_SHAKE: Screen shake (futuro)
 * 
 * @param chip64 Puntero a la estructura del emulador
 * @param effect Tipo de efecto a activar
 */
void chip64SetEffect(Chip64* chip64, GraphicsEffects effect);

/**
 * @brief Procesa los efectos gráficos activos
 * 
 * Esta función:
 * 1. Copia gfx → gfx2Buffer
 * 2. Aplica el efecto actual sobre gfx2Buffer
 * 3. El renderizador luego usa gfx2Buffer para mostrar
 * 
 * Debe llamarse antes de renderizar si hay efectos activos.
 * 
 * @param chip64 Puntero a la estructura del emulador
 */
void chip64ProcessEffects(Chip64* chip64);
/**
 * @brief Obtiene información de estado del emulador
 * 
 * Formato: "PC=0xXXXX I=0xXXXX SP=X Mode=Xbit Res=WxH"
 * 
 * @param chip64 Puntero a la estructura del emulador
 * @param buffer Buffer donde escribir la información
 * @param bufferSize Tamaño del buffer
 */
void chip64GetStatus(Chip64* chip64, char* buffer, size_t bufferSize);

/**
 * @brief Obtiene el ancho efectivo del display según el modo
 * 
 * @param chip64 Puntero a la estructura del emulador
 * @return Ancho en píxeles (64 o 128)
 */
uint16_t chip64GetDisplayWidth(Chip64* chip64);

/**
 * @brief Obtiene la altura efectiva del display según el modo
 * 
 * @param chip64 Puntero a la estructura del emulador
 * @return Altura en píxeles (32 o 64)
 */
uint16_t chip64GetDisplayHeight(Chip64* chip64);

#endif // CHIP64_H