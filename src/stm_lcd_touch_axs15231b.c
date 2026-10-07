#include "stm_lcd_touch_axs15231b.h"
#include <stdlib.h>
#include <string.h>
struct lcd_touch_axs15231b_context {
    lcd_touch_axs15231b_config_t config;
    lcd_touch_axs15231b_point_t point;
    uint8_t count;
};
stm_err_t lcd_touch_axs15231b_create(const lcd_touch_axs15231b_config_t *config, lcd_touch_axs15231b_handle_t *out)
{
    if (!config || !out) return STM_ERR_INVALID_ARG;
    if (*out) return STM_ERR_INVALID_STATE;
    if (!config->write || !config->read || !config->x_max || !config->y_max ||
        config->swap_xy > 1u || config->mirror_x > 1u || config->mirror_y > 1u ||
        (config->reset && !config->delay_ms)) return STM_ERR_INVALID_CONFIG;
    lcd_touch_axs15231b_handle_t touch = calloc(1, sizeof(*touch));
    if (!touch) return STM_ERR_NO_MEM;
    touch->config = *config;
    *out = touch;
    return STM_OK;
}
stm_err_t lcd_touch_axs15231b_delete(lcd_touch_axs15231b_handle_t *handle)
{
    if (!handle) return STM_ERR_INVALID_ARG;
    free(*handle);
    *handle = NULL;
    return STM_OK;
}
stm_err_t lcd_touch_axs15231b_reset(lcd_touch_axs15231b_handle_t touch)
{
    if (!touch) return STM_ERR_INVALID_ARG;
    touch->count = 0;
    if (!touch->config.reset) return STM_OK;
    stm_err_t err = touch->config.reset(touch->config.io, 0);
    if (err != STM_OK) return err;
    touch->config.delay_ms(touch->config.io, 200);
    err = touch->config.reset(touch->config.io, 1);
    if (err != STM_OK) return err;
    touch->config.delay_ms(touch->config.io, 200);
    return STM_OK;
}
stm_err_t lcd_touch_axs15231b_read_data(lcd_touch_axs15231b_handle_t touch)
{
    if (!touch) return STM_ERR_INVALID_ARG;
    static const uint8_t request[11] = {0xB5,0xAB,0xA5,0x5A,0,0,0,8,0,0,0};
    uint8_t response[8] = {0};
    touch->count = 0;
    stm_err_t err = touch->config.write(touch->config.io, request, sizeof request);
    if (err != STM_OK) return err;
    err = touch->config.read(touch->config.io, response, sizeof response);
    if (err != STM_OK) return err;
    if (response[1] > 1u) return STM_ERR_VERIFY;
    if (!response[1]) return STM_OK;
    /* Response: gesture, count, event(7:6)|Xhi(3:0), Xlo, Yhi(3:0), Ylo. */
    uint16_t x = (uint16_t)(((uint16_t)(response[2] & 0x0fu) << 8) | response[3]);
    uint16_t y = (uint16_t)(((uint16_t)(response[4] & 0x0fu) << 8) | response[5]);
    if (touch->config.swap_xy) { uint16_t tmp = x; x = y; y = tmp; }
    if (x >= touch->config.x_max || y >= touch->config.y_max) return STM_OK;
    if (touch->config.mirror_x) x = (uint16_t)(touch->config.x_max - 1u - x);
    if (touch->config.mirror_y) y = (uint16_t)(touch->config.y_max - 1u - y);
    touch->point.x = x;
    touch->point.y = y;
    touch->point.id = 0;
    touch->count = 1;
    return STM_OK;
}
stm_err_t lcd_touch_axs15231b_get_data(lcd_touch_axs15231b_handle_t touch,
    lcd_touch_axs15231b_point_t *points, size_t capacity, size_t *count)
{
    if (count) *count = 0;
    if (!touch || !count || (capacity && !points)) return STM_ERR_INVALID_ARG;
    *count = touch->count && capacity ? 1u : 0u;
    if (*count) memcpy(points, &touch->point, sizeof(*points));
    return STM_OK;
}
