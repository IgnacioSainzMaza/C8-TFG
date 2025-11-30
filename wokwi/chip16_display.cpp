#include "chip16_display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// Instancia global del display Adafruit
static Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ============================================================================
// FUNCIONES EXPORTADAS A C
// ============================================================================

extern "C" {

void chip16_display_init(Display* display) {
    printf("=== Inicializando ILI9341 con Adafruit ===");
    
   
    // Iniciar display
    tft.begin();
    printf("Display ILI9341 inicializado");
    
    // Configurar rotación (landscape)
    tft.setRotation(1);
    printf("Rotación configurada");
    
    // Limpiar pantalla
    tft.fillScreen(ILI9341_BLACK);
    printf("Pantalla limpiada");
 
    // Configurar estructura
    display->initialized = true;
    display->width = SCREEN_WIDTH;
    display->height = SCREEN_HEIGHT;
    memset(display->lastGfx, 0, CHIP16_WIDTH * CHIP16_HEIGHT);
    
    printf("=== Display listo ===");
}

void chip16_display_cleanup(Display* display) {
    display->initialized = false;
    printf("Display limpiado");
}

void chip16_display_fill_screen(uint16_t color) {
    tft.fillScreen(color);
}

void chip16_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    tft.fillRect(x, y, w, h, color);
}

uint16_t chip16_display_get_color(Chip16* chip) {
    if (chip->currentEffect == EFFECT_COLOR_CYCLE) {
        return COLOR_PALETTE[chip->colorIndex];
    }
    return chip->config.pixelColor;
}

void chip16_display_render(Display* display, Chip16* chip) {
    if (!chip->drawFlag) {
        return;
    }
    static uint32_t renderCount = 0;
    renderCount++;

    // Procesar efectos
    chip16_process_effects(chip);

    // Obtener color
    uint16_t pixelColor = chip16_display_get_color(chip);

    // Detectar si toda la pantalla está en negro (CLS)
    bool allBlack = true;
    for (int i = 0; i < CHIP16_WIDTH * CHIP16_HEIGHT; i++) {
        if (chip->gfxEffect[i] != 0) {
            allBlack = false;
            break;
        }
    }

    // Si toda la pantalla está negra, hacer un fillScreen rápido
    if (allBlack) {
        tft.fillScreen(ILI9341_BLACK);
        memset(display->lastGfx, 0, CHIP16_WIDTH * CHIP16_HEIGHT);
        chip->drawFlag = false;
        return;
    }

    uint32_t pixelsChanged = 0;

    for (int cy = 0; cy < CHIP16_HEIGHT; cy++) {
        for (int cx = 0; cx < CHIP16_WIDTH; cx++) {
            int idx = cx + (cy * CHIP16_WIDTH);

            // Solo dibujar si el píxel cambió desde el último frame
            if (chip->gfxEffect[idx] != display->lastGfx[idx]) {
                uint16_t color = chip->gfxEffect[idx] ? pixelColor : ILI9341_BLACK;
                uint16_t screenX = cx * PIXEL_SCALE;
                uint16_t screenY = cy * PIXEL_SCALE;

                tft.fillRect(screenX, screenY, PIXEL_SCALE, PIXEL_SCALE, color);

                // Actualizar cache
                display->lastGfx[idx] = chip->gfxEffect[idx];
                pixelsChanged++;
            }
        }
    }

    chip->drawFlag = false;


}

void chip16_display_set_rotation(uint8_t rotation) {
    tft.setRotation(rotation);
}

void chip16_display_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Con Adafruit no necesitamos esto directamente
    // La librería lo maneja internamente
}

void chip16_display_write_command(uint8_t cmd) {
    // No usado con Adafruit
}

void chip16_display_write_data(uint8_t data) {
    // No usado con Adafruit
}

void chip16_display_write_data16(uint16_t data) {
    // No usado con Adafruit
}

void chip16_display_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    tft.drawPixel(x, y, color);
}

void chip16_display_reset_dirty_rect(DirtyRect* rect) {
    rect->x_min = SCREEN_WIDTH;
    rect->y_min = SCREEN_HEIGHT;
    rect->x_max = 0;
    rect->y_max = 0;
    rect->dirty = false;
}

void chip16_display_update_dirty_rect(DirtyRect* rect, uint16_t x, uint16_t y) {
    rect->dirty = true;
    if (x < rect->x_min) rect->x_min = x;
    if (y < rect->y_min) rect->y_min = y;
    if (x + PIXEL_SCALE > rect->x_max) rect->x_max = x + PIXEL_SCALE;
    if (y + PIXEL_SCALE > rect->y_max) rect->y_max = y + PIXEL_SCALE;
}

uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

} // extern "C"