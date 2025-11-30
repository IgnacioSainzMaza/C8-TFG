#include "chip16_core.h"
#include <stdio.h>

//FONTSET
//----------------------
const uint8_t CHIP16_FONTSET[FONTSET_SIZE] = {
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

//EMULATION INIT
void chip16_init(Chip16* chip16) {
  memset(chip16, 0, sizeof(Chip16));

  //Default config
  chip16->config.debugLevel = DEBUG_NONE;
  chip16->config.intsPerSec = INSTRUCTIONS_PER_SECOND;
  chip16->config.enableSound = true;
  chip16->config.pixelColor = DEFAULT_PIXEL_COLOR;
  chip16->mode = MODE_8BIT;
  chip16->PC = ROM_LOAD_ADDRESS;
  chip16->SP = 0;
  chip16->currentEffect = EFFECT_NONE;
  chip16->colorIndex = 0;
  chip16->effectTimer = 0;

  //Fontset load
  memcpy(chip16->memory, CHIP16_FONTSET, FONTSET_SIZE);

  //Random seed
  srand(12345);

  //Timing
  chip16->lastTimerUpdate = 0;

  //Debug
  if (chip16->config.debugLevel >= DEBUG_OPCODES) {
    printf("=== CHIP 16 Init ===\n");
    printf("Modo: %s\n", chip16->mode == MODE_8BIT ? "8-bit" : "16-bit");
    printf("PC inicial: 0x%04\n", chip16->PC);
    printf("Fontset: %d bytes cargados\n", FONTSET_SIZE);
  }




}

//ROM LOAD
//---------------------------------
void chip16_load_rom(Chip16* chip, const uint8_t* rom, uint16_t size) {
  if (size > MEMORY_SIZE - ROM_LOAD_ADDRESS) {
    if (chip->config.debugLevel >= DEBUG_OPCODES) {
      printf("ERROR: ROM demasiado grande (%d bytes)\n", size);
    }
    return;
  }
  memcpy(&chip->memory[ROM_LOAD_ADDRESS], rom, size);

  if (chip->config.debugLevel >= DEBUG_OPCODES) {
    printf("ROM cargada: %d bytes en 0x%04X\n", size, ROM_LOAD_ADDRESS);
  }
}

//TIMERS UPDATE
//---------------------------------
void chip16_update_timers(Chip16* chip, uint32_t current_time_ms) {
    if (current_time_ms - chip->lastTimerUpdate >= TIMER_UPDATE_MS) {
        if (chip->delayTimer > 0) {
            chip->delayTimer--;
        }
        
        if (chip->soundTimer > 0) {
            chip->soundTimer--;
        }
        
        chip->lastTimerUpdate = current_time_ms;
    }
}

//INPUT GESTION
void chip16_set_key(Chip16* chip, uint8_t key, bool pressed) {
    if (key < KEY_COUNT) {
        chip->key[key] = pressed ? 1 : 0;
        
        if (chip->config.debugLevel >= DEBUG_VERBOSE) {
            printf("Tecla 0x%X %s\n", key, pressed ? "presionada" : "liberada");
        }
    }
}

//GRAPHIC EFFECTS
void chip16_set_effect(Chip16* chip, GraphicsEffect effect) {
    chip->currentEffect = effect;
    chip->effectTimer = 0;
    
    if (chip->config.debugLevel >= DEBUG_OPCODES) {
        printf("Efecto: %s\n", effect == EFFECT_NONE ? "Ninguno" : "Ciclo color");
    }
}

void chip16_process_effects(Chip16* chip) {
    memcpy(chip->gfxEffect, chip->gfx, CHIP16_WIDTH * CHIP16_HEIGHT);
    
    if (chip->currentEffect == EFFECT_COLOR_CYCLE) {
        chip->effectTimer++;
        if (chip->effectTimer >= COLOR_CYCLE_FRAMES) {
            chip->effectTimer = 0;
            chip->colorIndex = (chip->colorIndex + 1) % COLOR_PALETTE_SIZE;
        }
    }
}

//OPCODES

void chip16_cycle(Chip16* chip) {
    // ========== FETCH: Leer opcode de memoria ==========
    // Los opcodes son de 2 bytes (big-endian)
    chip->opcode = (chip->memory[chip->PC] << 8) | chip->memory[chip->PC + 1];
    
    // Incrementar PC (apunta a la siguiente instrucción)
    chip->PC += 2;
    
    // ========== DECODE: Extraer componentes del opcode ==========
    uint8_t x = (chip->opcode & 0x0F00) >> 8;   // Segundo nibble
    uint8_t y = (chip->opcode & 0x00F0) >> 4;   // Tercer nibble
    uint8_t n = chip->opcode & 0x000F;          // Cuarto nibble
    uint8_t kk = chip->opcode & 0x00FF;         // Byte bajo
    uint16_t nnn = chip->opcode & 0x0FFF;       // 12 bits bajos
    
    // Variables auxiliares (declaradas aquí para usar en todo el switch)
    uint16_t xPos, yPos, height, length, pattern, value, src, dst, count;
    uint8_t shift, nParams, range;
    uint32_t result, product;
    uint16_t returnValue, memValue, spriteData, activePattern, mask;
    bool found, keyPressed;
    int basePos, pixelPos, pixelX, pixelY;
    uint8_t spriteDataB;
    
    // Debug: imprimir opcode si está habilitado
    if (chip->config.debugLevel >= DEBUG_OPCODES) {
        printf("[0x%04X] Opcode: 0x%04X\n", chip->PC - 2, chip->opcode);
    }
    
    // ========== EXECUTE: Decodificar e implementar instrucción ==========
    switch (chip->opcode & 0xF000) {
        
    // ====================================================================
    // 0x0NNN: Instrucciones de Sistema
    // ====================================================================
    case 0x0000:
        switch (kk) {
            case 0xE0:  // 00E0: CLS - Limpiar pantalla
                memset(chip->gfx, 0, CHIP16_WIDTH * CHIP16_HEIGHT);
                chip->drawFlag = true;
                break;
                
            case 0xEE:  // 00EE: RET - Retornar de subrutina
                chip->SP--;
                chip->PC = chip->stack[chip->SP];
                break;
                
            default:
                if (chip->config.debugLevel >= DEBUG_OPCODES) {
                    printf("Opcode 0x0NNN desconocido: 0x%04X\n", chip->opcode);
                }
                break;
        }
        break;
    
    // ====================================================================
    // 0x1NNN: JP addr - Saltar a dirección
    // ====================================================================
    case 0x1000:
        chip->PC = nnn;
        break;
    
    // ====================================================================
    // 0x2NNN: CALL addr - Llamar subrutina
    // ====================================================================
    case 0x2000:
        chip->stack[chip->SP] = chip->PC;
        chip->SP++;
        chip->PC = nnn;
        break;
    
    // ====================================================================
    // 0x3XKK: SE Vx, byte - Saltar si Vx == KK
    // ====================================================================
    case 0x3000:
        if ((chip->V[x] & 0xFF) == kk) {
            chip->PC += 2;
        }
        break;
    
    // ====================================================================
    // 0x4XKK: SNE Vx, byte - Saltar si Vx != KK
    // ====================================================================
    case 0x4000:
        if (chip->V[x] != kk) {
            chip->PC += 2;
        }
        break;
    
    // ====================================================================
    // 0x5XYN: Instrucciones de comparación y matemáticas extendidas
    // ====================================================================
    case 0x5000:
        switch (n) {
            case 0x0:  // 5XY0: SE Vx, Vy - Saltar si Vx == Vy
                if (chip->V[x] == chip->V[y]) {
                    chip->PC += 2;
                }
                break;
            
            case 0x1:  // 5XY1: MUL Vx, Vy - Multiplicación 16 bits
                result = (uint32_t)chip->V[x] * (uint32_t)chip->V[y];
                chip->V[x] = result & 0xFFFF;
                chip->V[0xF] = (result > 0xFFFF) ? 1 : 0;  // Overflow flag
                break;
            
            case 0x2:  // 5XY2: DIV Vx, Vy - División
                if (chip->V[y] != 0) {
                    chip->V[0xF] = chip->V[x] % chip->V[y];  // Resto en VF
                    chip->V[x] = chip->V[x] / chip->V[y];    // Cociente en Vx
                } else {
                    chip->V[x] = 0xFFFF;  // Error: división por cero
                    chip->V[0xF] = 0;
                }
                break;
            
            case 0x3:  // 5XY3: VADD Vx, Vy - Suma vectorial 2D
                {
                    uint16_t nextX = (x + 1) % REGISTER_COUNT;
                    uint16_t nextY = (y + 1) % REGISTER_COUNT;
                    chip->V[x] += chip->V[y];
                    chip->V[nextX] += chip->V[nextY];
                }
                break;
            
            case 0x4:  // 5XY4: DOT Vx, Vy - Producto escalar 2D
                {
                    uint16_t nextX = (x + 1) % REGISTER_COUNT;
                    uint16_t nextY = (y + 1) % REGISTER_COUNT;
                    product = (uint32_t)chip->V[x] * (uint32_t)chip->V[y] + 
                             (uint32_t)chip->V[nextX] * (uint32_t)chip->V[nextY];
                    chip->V[x] = product & 0xFFFF;
                    chip->V[0xF] = (product >> 16) & 0xFFFF;
                }
                break;
            
            default:
                break;
        }
        break;
    
    // ====================================================================
    // 0x6XKK: LD Vx, byte - Cargar valor inmediato
    // ====================================================================
    case 0x6000:
        chip->V[x] = kk;
        break;
    
    // ====================================================================
    // 0x7XKK: ADD Vx, byte - Sumar valor inmediato
    // ====================================================================
    case 0x7000:
        chip->V[x] += kk;
        break;
    
    // ====================================================================
    // 0x8XYN: Operaciones aritméticas y lógicas entre registros
    // ====================================================================
    case 0x8000:
        switch (n) {
            case 0x0:  // 8XY0: LD Vx, Vy
                chip->V[x] = chip->V[y];
                break;
            
            case 0x1:  // 8XY1: OR Vx, Vy
                chip->V[x] |= chip->V[y];
                break;
            
            case 0x2:  // 8XY2: AND Vx, Vy
                chip->V[x] &= chip->V[y];
                break;
            
            case 0x3:  // 8XY3: XOR Vx, Vy
                chip->V[x] ^= chip->V[y];
                break;
            
            case 0x4:  // 8XY4: ADD Vx, Vy con carry
                {
                    uint32_t sum = chip->V[x] + chip->V[y];
                    chip->V[0xF] = (sum > 0xFFFF) ? 1 : 0;
                    chip->V[x] = sum & 0xFFFF;
                }
                break;
            
            case 0x5:  // 8XY5: SUB Vx, Vy con borrow
                chip->V[0xF] = (chip->V[x] > chip->V[y]) ? 1 : 0;
                chip->V[x] = (chip->V[x] - chip->V[y]) & 0xFFFF;
                break;
            
            case 0x6:  // 8XY6: SHR Vx - Shift right
                chip->V[0xF] = chip->V[x] & 0x1;
                chip->V[x] >>= 1;
                break;
            
            case 0x7:  // 8XY7: SUBN Vx, Vy (Vx = Vy - Vx)
                chip->V[0xF] = (chip->V[y] > chip->V[x]) ? 1 : 0;
                chip->V[x] = chip->V[y] - chip->V[x];
                break;
            
            case 0xE:  // 8XYE: SHL Vx - Shift left
                chip->V[0xF] = (chip->V[x] & 0x8000) >> 15;
                chip->V[x] <<= 1;
                break;
        }
        break;
    
    // ====================================================================
    // 0x9XYN: Instrucciones de manipulación de bits
    // ====================================================================
    case 0x9000:
        switch (n) {
            case 0x0:  // 9XY0: SNE Vx, Vy - Saltar si Vx != Vy
                if (chip->V[x] != chip->V[y]) {
                    chip->PC += 2;
                }
                break;
            
            case 0x1:  // 9XY1: ROR Vx, Vy - Rotación a derecha
                value = chip->V[x];
                shift = chip->V[y] & 0x0F;
                chip->V[x] = (value >> shift) | (value << (16 - shift));
                break;
            
            case 0x2:  // 9XY2: ROL Vx, Vy - Rotación a izquierda
                value = chip->V[x];
                shift = chip->V[y] & 0x0F;
                chip->V[x] = (value << shift) | (value >> (16 - shift));
                break;
            
            case 0x3:  // 9XY3: POPCNT Vx - Contar bits en 1
                value = chip->V[x];
                count = 0;
                for (int i = 0; i < 16; i++) {
                    if (value & (1 << i)) {
                        count++;
                    }
                }
                chip->V[x] = count;
                break;
        }
        break;
    
    // ====================================================================
    // 0xANNN: LD I, addr - Cargar registro I
    // ====================================================================
    case 0xA000:
        chip->I = nnn;
        break;
    
    // ====================================================================
    // 0xBNNN: JP V0, addr - Salto indexado e instrucciones de memoria
    // ====================================================================
    case 0xB000:
        switch (n) {
            case 0x0:  // BNNN: JP V0, addr - Saltar a NNN + V0
                chip->PC = nnn + (chip->V[0] & 0xFFFF);
                break;
            
            case 0x1:  // B001: MEMCPY - Copiar bloque de memoria
                count = chip->V[x];
                src = chip->I;
                dst = chip->I + count;
                
                if (dst < MEMORY_SIZE) {
                    // Manejo de solapamiento
                    if (src < dst && src + count > dst) {
                        // Copiar de atrás hacia adelante
                        for (int i = count - 1; i >= 0; i--) {
                            chip->memory[dst + i] = chip->memory[src + i];
                        }
                    } else {
                        // Copiar normal
                        for (uint16_t i = 0; i < count; i++) {
                            chip->memory[dst + i] = chip->memory[src + i];
                        }
                    }
                }
                break;
            
            case 0x2:  // B002: MEMSRCH - Buscar valor en memoria
                value = chip->V[x];
                found = false;
                
                for (int i = 0; i < 256 && (chip->I + i + 1) < MEMORY_SIZE; i += 2) {
                    memValue = (chip->memory[chip->I + i] << 8) | 
                               chip->memory[chip->I + i + 1];
                    if (memValue == value) {
                        chip->V[0xF] = i / 2;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    chip->V[0xF] = 0xFFFF;
                }
                break;
        }
        break;
    
    // ====================================================================
    // 0xCXKK: RND Vx, byte - Número aleatorio
    // ====================================================================
    case 0xC000:
        chip->V[x] = (rand() % 256) & kk;
        break;
    
    // ====================================================================
    // 0xDXYN: DRW Vx, Vy, nibble - Dibujar sprite
    // ====================================================================
    case 0xD000:
        xPos = chip->V[x] % CHIP16_WIDTH;
        yPos = chip->V[y] % CHIP16_HEIGHT;
        height = n;
        
        chip->V[0xF] = 0;  // Reset collision flag
        
        for (uint16_t row = 0; row < height; row++) {
            spriteDataB = chip->memory[chip->I + row];
            
            for (int col = 0; col < 8; col++) {
                if ((spriteDataB & (0x80 >> col)) != 0) {
                    pixelX = (xPos + col) % CHIP16_WIDTH;
                    pixelY = (yPos + row) % CHIP16_HEIGHT;
                    pixelPos = pixelX + (pixelY * CHIP16_WIDTH);
                    
                    // Detectar colisión
                    if (chip->gfx[pixelPos] == 1) {
                        chip->V[0xF] = 1;
                    }
                    
                    // XOR con píxel existente
                    chip->gfx[pixelPos] ^= 1;
                }
            }
        }
        
        chip->drawFlag = true;
        break;
    
    // ====================================================================
    // 0xEXKK: Instrucciones de teclado y aleatorios extendidos
    // ====================================================================
    case 0xE000:
        switch (kk) {
            case 0x01:  // E001: CALLP - Llamada con parámetros
                if (chip->SP + 4 <= STACK_SIZE) {
                    chip->stack[chip->SP] = chip->PC;
                    chip->stack[chip->SP + 1] = chip->V[0xD];
                    chip->stack[chip->SP + 2] = chip->V[0xE];
                    chip->stack[chip->SP + 3] = chip->V[0xF];
                    chip->SP += 4;
                    
                    nParams = y;
                    chip->V[0xD] = (chip->V[1] >> 8) & 0xFF;
                    chip->V[0xE] = chip->V[1] & 0xFF;
                    chip->V[0xF] = nParams;
                    chip->PC = chip->V[0];
                } else if (chip->config.debugLevel >= DEBUG_OPCODES) {
                    printf("ERROR: Stack overflow en CALLP\n");
                }
                break;
            
            case 0x02:  // E002: RETV - Retorno con valor
                returnValue = chip->V[y];
                if (chip->SP >= 4) {
                    chip->SP -= 4;
                    chip->V[0xF] = chip->stack[chip->SP + 3];
                    chip->V[0xE] = chip->stack[chip->SP + 2];
                    chip->V[0xD] = chip->stack[chip->SP + 1];
                    chip->PC = chip->stack[chip->SP];
                    chip->V[0] = returnValue;
                } else if (chip->config.debugLevel >= DEBUG_OPCODES) {
                    printf("ERROR: Stack underflow en RETV\n");
                }
                break;
            
            case 0x03:  // E003: RND16 - Aleatorio 16 bits
                chip->V[x] = rand() % 65536;
                break;
            
            case 0x04:  // E004: RNDR - Aleatorio en rango
                range = x + 1;
                if (chip->V[range] > 0) {
                    chip->V[x] = rand() % chip->V[range];
                } else {
                    chip->V[x] = 0;
                }
                break;
            
            case 0x9E:  // EX9E: SKP Vx - Saltar si tecla presionada
                if (chip->key[chip->V[x]] != 0) {
                    chip->PC += 2;
                }
                break;
            
            case 0xA1:  // EXA1: SKNP Vx - Saltar si tecla NO presionada
                if (chip->key[chip->V[x]] == 0) {
                    chip->PC += 2;
                }
                break;
        }
        break;
    
    // ====================================================================
    // 0xFXKK: Instrucciones variadas y gráficas extendidas
    // ====================================================================
    case 0xF000:
        switch (kk) {
            case 0x01:  // F001: DRAW16 - Dibujar sprite 16x16
                xPos = chip->V[2] % CHIP16_WIDTH;
                yPos = chip->V[3] % CHIP16_HEIGHT;
                chip->V[0xF] = 0;
                
                for (int row = 0; row < 16; row++) {
                    spriteData = (chip->memory[chip->I + row * 2] << 8) |
                                 chip->memory[chip->I + row * 2 + 1];
                    
                    for (int col = 0; col < 16; col++) {
                        if ((spriteData & (0x8000 >> col)) != 0) {
                            pixelX = (xPos + col) % CHIP16_WIDTH;
                            pixelY = (yPos + row) % CHIP16_HEIGHT;
                            pixelPos = pixelX + (pixelY * CHIP16_WIDTH);
                            
                            if (chip->gfx[pixelPos] == 1) {
                                chip->V[0xF] = 1;
                            }
                            
                            chip->gfx[pixelPos] ^= 1;
                        }
                    }
                }
                
                chip->drawFlag = true;
                break;
            
            case 0x02:  // F002: HLINE - Línea horizontal
                xPos = chip->V[2] % CHIP16_WIDTH;
                yPos = chip->V[3] % CHIP16_HEIGHT;
                length = chip->V[4];
                pattern = chip->V[5];
                
                if (length == 0 || length > CHIP16_WIDTH - xPos) {
                    length = CHIP16_WIDTH - xPos;
                }
                
                chip->V[0xF] = 0;
                basePos = xPos + (yPos * CHIP16_WIDTH);
                
                if (length <= 16) {
                    mask = 0;
                    for (uint16_t i = 0; i < length; i++) {
                        mask |= (0x8000 >> i);
                    }
                    activePattern = pattern & mask;
                    
                    for (uint16_t i = 0; i < length; i++) {
                        if ((activePattern & (0x8000 >> i)) != 0) {
                            if (chip->gfx[basePos + i] == 1) {
                                chip->V[0xF] = 1;
                            }
                            chip->gfx[basePos + i] ^= 1;
                        }
                    }
                } else {
                    for (uint16_t i = 0; i < length; i++) {
                        if ((pattern & (0x8000 >> (i % 16))) != 0) {
                            if (chip->gfx[basePos + i] == 1) {
                                chip->V[0xF] = 1;
                            }
                            chip->gfx[basePos + i] ^= 1;
                        }
                    }
                }
                
                chip->drawFlag = true;
                break;
            
            case 0x03:  // F003: VLINE - Línea vertical
                xPos = chip->V[2] % CHIP16_WIDTH;
                yPos = chip->V[3] % CHIP16_HEIGHT;
                height = chip->V[4];
                pattern = chip->V[5];
                
                if (height == 0 || height > CHIP16_HEIGHT - yPos) {
                    height = CHIP16_HEIGHT - yPos;
                }
                
                chip->V[0xF] = 0;
                
                for (uint16_t i = 0; i < height; i++) {
                    if ((pattern & (0x8000 >> (i % 16))) != 0) {
                        pixelPos = xPos + ((yPos + i) % CHIP16_HEIGHT) * CHIP16_WIDTH;
                        
                        if (chip->gfx[pixelPos] == 1) {
                            chip->V[0xF] = 1;
                        }
                        chip->gfx[pixelPos] ^= 1;
                    }
                }
                
                chip->drawFlag = true;
                break;
            
            case 0x07:  // FX07: LD Vx, DT - Leer delay timer
                chip->V[x] = chip->delayTimer;
                break;
            
            case 0x0A:  // FX0A: LD Vx, K - Esperar tecla
                keyPressed = false;
                for (int i = 0; i < KEY_COUNT; i++) {
                    if (chip->key[i]) {
                        chip->V[x] = i;
                        keyPressed = true;
                        break;
                    }
                }
                
                // Si no hay tecla, repetir instrucción
                if (!keyPressed) {
                    chip->PC -= 2;
                }
                break;
            
            case 0x15:  // FX15: LD DT, Vx - Establecer delay timer
                chip->delayTimer = chip->V[x];
                break;
            
            case 0x18:  // FX18: LD ST, Vx - Establecer sound timer
                chip->soundTimer = chip->V[x];
                break;
            
            case 0x1E:  // FX1E: ADD I, Vx
                chip->I += chip->V[x];
                break;
            
            case 0x29:  // FX29: LD F, Vx - Cargar dirección de sprite de fuente
                if (chip->mode == MODE_8BIT) {
                    uint8_t digit = chip->V[x] & 0x0F;
                    chip->I = digit * 5;
                } else {
                    uint8_t character = chip->V[x] & 0xFF;
                    if (character <= 0x0F) {
                        chip->I = character * 5;
                    } else if (character < 128 && character >= 16) {
                        chip->I = FONTSET_SIZE + ((character - 16) * 8);
                    } else {
                        chip->I = 0x0F * 5;  // Fallback
                    }
                }
                break;
            
            case 0x33:  // FX33: LD B, Vx - Almacenar BCD
                if (chip->mode == MODE_8BIT) {
                    uint8_t val = chip->V[x] & 0xFF;
                    chip->memory[chip->I] = val / 100;
                    chip->memory[chip->I + 1] = (val / 10) % 10;
                    chip->memory[chip->I + 2] = val % 10;
                } else {
                    uint16_t val = chip->V[x];
                    chip->memory[chip->I] = val / 10000;
                    chip->memory[chip->I + 1] = (val / 1000) % 10;
                    chip->memory[chip->I + 2] = (val / 100) % 10;
                    chip->memory[chip->I + 3] = (val / 10) % 10;
                    chip->memory[chip->I + 4] = val % 10;
                }
                break;
            
            case 0x55:  // FX55: LD [I], Vx - Guardar registros
                if (chip->mode == MODE_8BIT) {
                    for (int i = 0; i <= x; i++) {
                        chip->memory[chip->I + i] = chip->V[i] & 0xFF;
                    }
                } else {
                    for (int i = 0; i <= x; i++) {
                        chip->memory[chip->I + (i * 2)] = (chip->V[i] >> 8) & 0xFF;
                        chip->memory[chip->I + (i * 2) + 1] = chip->V[i] & 0xFF;
                    }
                    chip->I += (x + 1) * 2;
                }
                break;
            
            case 0x65:  // FX65: LD Vx, [I] - Cargar registros
                if (chip->mode == MODE_8BIT) {
                    for (int i = 0; i <= x; i++) {
                        chip->V[i] = chip->memory[chip->I + i];
                    }
                } else {
                    for (int i = 0; i <= x; i++) {
                        chip->V[i] = (chip->memory[chip->I + (i * 2)] << 8) |
                                     chip->memory[chip->I + (i * 2) + 1];
                    }
                    chip->I += (x + 1) * 2;
                }
                break;
        }
        break;
    
    // ====================================================================
    // Opcode desconocido
    // ====================================================================
    default:
        if (chip->config.debugLevel >= DEBUG_OPCODES) {
            printf("Opcode desconocido: 0x%04X en PC=0x%04X\n", 
                   chip->opcode, chip->PC - 2);
        }
        break;
    }
}