#ifndef TEST_ROMS_H
#define TEST_ROMS_H

#include <stdint.h>

// ============================================================================
// ROM 0: TEST TECLADO
// ============================================================================
// Prueba el teclado matricial 4x4 y el buzzer
// Presiona cualquier tecla (0-F) y verás un sprite + escucharás un beep

const uint8_t ROM_TEST_KEYBOARD[] = {
    // Limpiar pantalla
    0x00, 0xE0,  // 0x200: CLS
    
    // Esperar que se presione una tecla
    0xF0, 0x0A,  // 0x202: LD V0, K - Espera tecla, resultado en V0
    0x00, 0xE0,  // 0x204: CLS - Limpia después de recibir tecla
    // Activar buzzer (10 frames)
    0x61, 0x0A,  // 0x204: LD V1, 10
    0xF1, 0x18,  // 0x206: LD ST, V1 - sound timer = 10
    
    // Calcular posición X = (tecla % 4) * 8 + 8
    0x62, 0x08,  // 0x208: LD V2, 8 (X base)
    0x64, 0x03,  // 0x20A: LD V4, 3 (máscara para módulo 4)
    0x85, 0x02,  // 0x20C: V5 = V0 AND V4 (V5 = tecla % 4)
    0x75, 0x08,  // 0x20E: V5 += 8 (multiplicar por 8 - simplificado)
    
    // Calcular posición Y = (tecla / 4) * 6 + 4
    0x63, 0x04,  // 0x210: LD V3, 4 (Y base)
    0x86, 0x06,  // 0x212: V6 = V0 SHR 1
    0x86, 0x06,  // 0x214: V6 = V6 SHR 1 (V6 = tecla / 4)
    0x76, 0x06,  // 0x216: V6 += 6
    
    // Cargar sprite en I
    0xA2, 0x40,  // 0x218: LD I, 0x240 (sprite con X)
    
    // Dibujar sprite en (V5, V6)
    0xD5, 0x68,  // 0x21A: DRW V5, V6, 8
    
    // Mostrar número de tecla en esquina
    0xF0, 0x29,  // 0x21C: LD F, V0 - Cargar fuente del dígito
    0x6A, 0x02,  // 0x21E: LD VA, 2 (X para dígito)
    0x6B, 0x02,  // 0x220: LD VB, 2 (Y para dígito)
    0xDA, 0xB5,  // 0x222: DRW VA, VB, 5 - Dibujar dígito
    
    // Delay corto
    0x6C, 0x05,  // 0x224: LD VC, 5
    0xFC, 0x15,  // 0x226: LD DT, VC
    
    // Esperar delay
    0xFD, 0x07,  // 0x228: LD VD, DT
    0x3D, 0x00,  // 0x22A: SE VD, 0
    0x12, 0x2A,  // 0x22C: JP 0x228 (repetir si no es 0)
    
    // Volver al inicio
    0x12, 0x02,  // 0x22E: JP 0x200
    
    // Padding hasta sprite
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    
    // 0x240: Sprite con X (8x8)
    0xFF,  // 11111111
    0x81,  // 10000001
    0x42,  // 01000010
    0x24,  // 00100100
    0x18,  // 00011000
    0x24,  // 00100100
    0x42,  // 01000010
    0xFF   // 11111111
};

// ============================================================================
// ROM 1: TEST BOTONES
// ============================================================================
// Dibuja 3 bloques en pantalla
// Los botones Reset/Mode/Effect funcionan

const uint8_t ROM_TEST_BUTTONS[] = {
    // Limpiar pantalla
    0x00, 0xE0,  // 0x200: CLS
    
    // Dibujar bloque 1 (izquierda)
    0x60, 0x08,  // 0x202: V0 = 8 (X)
    0x61, 0x0C,  // 0x204: V1 = 12 (Y)
    0xA2, 0x30,  // 0x206: I = 0x230 (sprite)
    0xD0, 0x18,  // 0x208: DRW V0, V1, 8
    
    // Dibujar bloque 2 (centro)
    0x60, 0x1C,  // 0x20A: V0 = 28
    0xD0, 0x18,  // 0x20C: DRW V0, V1, 8
    
    // Dibujar bloque 3 (derecha)
    0x60, 0x30,  // 0x20E: V0 = 48
    0xD0, 0x18,  // 0x210: DRW V0, V1, 8
    
    // Loop infinito con delay
    0x6E, 0x01,  // 0x212: VE = 1
    0xFE, 0x15,  // 0x214: DT = VE
    0xFF, 0x07,  // 0x216: VF = DT
    0x3F, 0x00,  // 0x218: SE VF, 0
    0x12, 0x16,  // 0x21A: JP 0x216 (esperar)
    
    0x12, 0x12,  // 0x21C: JP 0x212 (repetir)
    
    // Padding
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    
    // 0x230: Sprite - Bloque sólido
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// ============================================================================
// ROM 2: CATCH THE DOT (JUEGO)
// ============================================================================
// Controles: Tecla 4 = izquierda, Tecla 6 = derecha
// Atrapa los puntos que caen

const uint8_t ROM_GAME_CATCH[] = {
    // Inicialización
    0x00, 0xE0,  // 0x200: CLS
    0x60, 0x1C,  // 0x202: V0 = 28 (X paleta - centro)
    0x61, 0x1A,  // 0x204: V1 = 26 (Y paleta - abajo)
    0x62, 0x10,  // 0x206: V2 = 16 (X punto)
    0x63, 0x02,  // 0x208: V3 = 2  (Y punto - arriba)
    0x64, 0x00,  // 0x20A: V4 = 0  (score)
    
    // LOOP PRINCIPAL (0x20C)
    0x00, 0xE0,  // 0x20C: CLS
    
    // Verificar tecla 4 (izquierda)
    0x65, 0x04,  // 0x20E: V5 = 4
    0xE5, 0x9E,  // 0x210: SKP V5 - Saltar si tecla 4 presionada
    0x12, 0x16,  // 0x212: JP 0x216 (no presionada)
    0x70, 0xFC,  // 0x214: V0 -= 4 (mover izquierda)
    
    // Verificar tecla 6 (derecha)
    0x65, 0x06,  // 0x216: V5 = 6
    0xE5, 0x9E,  // 0x218: SKP V5
    0x12, 0x1E,  // 0x21A: JP 0x21E (no presionada)
    0x70, 0x04,  // 0x21C: V0 += 4 (mover derecha)
    
    // Mover punto hacia abajo
    0x73, 0x01,  // 0x21E: V3 += 1
    
    // Verificar si punto llegó abajo (Y >= 26)
    0x43, 0x1A,  // 0x220: SNE V3, 26
    0x12, 0x2A,  // 0x222: JP 0x22A (verificar colisión)
    0x12, 0x34,  // 0x224: JP 0x234 (continuar)
    
    // COLISIÓN - verificar si paleta atrapa punto
    // Simplificado: si X punto cerca de X paleta
    0x85, 0x00,  // 0x226: V5 = V0 (X paleta)
    0x55, 0x20,  // 0x228: SE V5, V2 (¿X paleta == X punto?)
    
    // ACIERTO - incrementar score y sonido
    0x74, 0x01,  // 0x22A: V4 += 1
    0x68, 0x0A,  // 0x22C: V8 = 10
    0xF8, 0x18,  // 0x22E: ST = V8 (buzzer 10 frames)
    0x63, 0x02,  // 0x230: V3 = 2 (reset punto)
    0x12, 0x34,  // 0x232: JP 0x234
    
    // CONTINUAR - resetear punto si llegó abajo
    0x43, 0x1C,  // 0x234: SNE V3, 28
    0x63, 0x02,  // 0x236: V3 = 2 (reset si >= 28)
    
    // Generar nueva X aleatoria para el punto
    0xC2, 0x38,  // 0x238: V2 = random() AND 0x38
    
    // Dibujar paleta
    0xA2, 0x60,  // 0x23A: I = 0x260 (sprite paleta)
    0xD0, 0x14,  // 0x23C: DRW V0, V1, 4
    
    // Dibujar punto
    0xA2, 0x64,  // 0x23E: I = 0x264 (sprite punto)
    0xD2, 0x32,  // 0x240: DRW V2, V3, 2
    
    // Delay
    0x66, 0x02,  // 0x242: V6 = 2
    0xF6, 0x15,  // 0x244: DT = V6
    0xF7, 0x07,  // 0x246: V7 = DT
    0x37, 0x00,  // 0x248: SE V7, 0
    0x12, 0x46,  // 0x24A: JP 0x246 (esperar)
    
    // Volver al loop
    0x12, 0x0C,  // 0x24C: JP 0x20C

    // Padding (18 bytes para alinear sprites en 0x260)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,

    // 0x260: Sprite PALETA (8x4)
    0xFF,  // ********
    0xFF,  // ********
    0xFF,  // ********
    0xFF,  // ********
    
    // 0x264: Sprite PUNTO (8x2)
    0x18,  //    **
    0x18   //    **
};

// ============================================================================
// LISTA DE ROMs DISPONIBLES
// ============================================================================

typedef struct {
    const char* name;
    const uint8_t* data;
    uint16_t size;
    const char* description;
} TestROM;

const TestROM TEST_ROM_LIST[] = {
    {
        "Test Teclado",
        ROM_TEST_KEYBOARD,
        sizeof(ROM_TEST_KEYBOARD),
        "Presiona teclas 0-F, verás sprite + beep"
    },
    {
        "Test Botones",
        ROM_TEST_BUTTONS,
        sizeof(ROM_TEST_BUTTONS),
        "Prueba botones Reset/Mode/Effect"
    },
    {
        "Catch The Dot",
        ROM_GAME_CATCH,
        sizeof(ROM_GAME_CATCH),
        "Juego: Teclas 4(←) y 6(→) para mover"
    }
};

const int TEST_ROM_COUNT = sizeof(TEST_ROM_LIST) / sizeof(TestROM);

#endif // TEST_ROMS_H