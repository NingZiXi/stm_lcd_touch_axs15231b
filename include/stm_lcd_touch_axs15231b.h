#ifndef STM_LCD_TOUCH_AXS15231B_H
#define STM_LCD_TOUCH_AXS15231B_H
#include <stddef.h>
#include <stdint.h>
#include "stm_err.h"
#ifdef __cplusplus
extern "C" {
#endif
#define LCD_TOUCH_AXS15231B_MAX_POINTS 1u
#define LCD_TOUCH_AXS15231B_ADDR_7BIT 0x3Bu
typedef struct lcd_touch_axs15231b_context *lcd_touch_axs15231b_handle_t;
typedef struct { uint16_t x, y; uint8_t id; } lcd_touch_axs15231b_point_t;
typedef struct {
    /* Two separate synchronous I2C transactions to 0x3B (7-bit), no register address. */
    stm_err_t (*write)(void *io, const uint8_t *data, size_t bytes);
    stm_err_t (*read)(void *io, uint8_t *data, size_t bytes);
    void (*delay_ms)(void *io, uint32_t ms); /* required with reset */
    stm_err_t (*reset)(void *io, int high); /* optional, active low */
    void *io;
    uint16_t x_max, y_max; /* transformed logical dimensions, exclusive */
    uint8_t swap_xy, mirror_x, mirror_y;
} lcd_touch_axs15231b_config_t;
stm_err_t lcd_touch_axs15231b_create(const lcd_touch_axs15231b_config_t *config, lcd_touch_axs15231b_handle_t *out);
stm_err_t lcd_touch_axs15231b_delete(lcd_touch_axs15231b_handle_t *handle);
stm_err_t lcd_touch_axs15231b_reset(lcd_touch_axs15231b_handle_t touch);
stm_err_t lcd_touch_axs15231b_read_data(lcd_touch_axs15231b_handle_t touch);
stm_err_t lcd_touch_axs15231b_get_data(lcd_touch_axs15231b_handle_t touch,
                                        lcd_touch_axs15231b_point_t *points, size_t capacity, size_t *count);
#ifdef __cplusplus
}
#endif
#endif
