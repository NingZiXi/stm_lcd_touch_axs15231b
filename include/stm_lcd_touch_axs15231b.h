/** @file stm_lcd_touch_axs15231b.h @brief AXS constructor; operations use stm_lcd_touch_*. */
#ifndef STM_LCD_TOUCH_AXS15231B_H
#define STM_LCD_TOUCH_AXS15231B_H
#include "stm_lcd.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define LCD_TOUCH_AXS15231B_MAX_POINTS 1u
#define LCD_TOUCH_AXS15231B_ADDR_7BIT 0x3Bu

typedef struct
{
    stm_lcd_io_handle_t io; /**< Borrowed raw I2C IO, independent write and read transactions. */
    stm_lcd_touch_config_t coordinates; /**< Swap, bounds and mirror handled by stm_lcd. */
    void (*delay_ms)(void *, uint32_t); /**< Required only when reset configured. */
    stm_err_t (*reset)(void *, int);    /**< Optional startup reset, never used by reconnect. */
    void *control_context;              /**< Borrowed reset/delay context. */
} lcd_touch_axs15231b_config_t;

stm_err_t lcd_touch_axs15231b_create(const lcd_touch_axs15231b_config_t *config,
                                     stm_lcd_touch_handle_t *out);
#ifdef __cplusplus
}
#endif
#endif
