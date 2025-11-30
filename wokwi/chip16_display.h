#ifndef CHIP16_DISPLAY_H
#define CHIP16_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "chip16_config.h"
#include "chip16_core.h"

// Colores RGB565 (compatibles con Adafruit)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW  0xFFE0

// Dirty rectangle para optimización
typedef struct {
    uint16_t x_min;
    uint16_t y_min;
    uint16_t x_max;
    uint16_t y_max;
    bool dirty;
} DirtyRect;

// Estructura del display
typedef struct {
    bool initialized;
    uint16_t width;
    uint16_t height;
    uint8_t lastGfx[CHIP16_WIDTH * CHIP16_HEIGHT];
} Display;

// Declaraciones de funciones (exportadas desde C++)
#ifdef __cplusplus
extern "C" {
#endif

void chip16_display_init(Display* display);
void chip16_display_cleanup(Display* display);
void chip16_display_fill_screen(uint16_t color);
void chip16_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void chip16_display_render(Display* display, Chip16* chip);
uint16_t chip16_display_get_color(Chip16* chip);

// Funciones auxiliares
// void chip16_display_set_rotation(uint8_t rotation);
// void chip16_display_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
// void chip16_display_write_command(uint8_t cmd);
// void chip16_display_write_data(uint8_t data);
// void chip16_display_write_data16(uint16_t data);
// void chip16_display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
// void chip16_display_reset_dirty_rect(DirtyRect* rect);
// void chip16_display_update_dirty_rect(DirtyRect* rect, uint16_t x, uint16_t y);
uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif

#endif
