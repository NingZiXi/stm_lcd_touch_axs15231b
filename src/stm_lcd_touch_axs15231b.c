/**
 * @file    stm_lcd_touch_axs15231b.c
 * @brief   实现 AXS15231B 单触点请求及响应解析。
 */
#include "stm_lcd_touch_axs15231b.h"
#include "stm_lcd_impl.h"
#include <stdlib.h>

struct lcd_touch_axs15231b_context
{
    struct stm_lcd_touch base;           // 嵌入式触点快照及变换。
    lcd_touch_axs15231b_config_t config; // 控制器复位配置。
};

static stm_err_t reset(stm_lcd_touch_handle_t base)
{
    struct lcd_touch_axs15231b_context *t = (void *)base;
    if (!t->config.reset)
        return STM_ERR_NOT_SUPPORTED;
    stm_err_t err = t->config.reset(t->config.control_context, 0);
    if (err != STM_OK)
        return err;
    t->config.delay_ms(t->config.control_context, 200);
    err = t->config.reset(t->config.control_context, 1);
    if (err != STM_OK)
        return err;
    t->config.delay_ms(t->config.control_context, 200);
    return STM_OK;
}

static stm_err_t read_data(stm_lcd_touch_handle_t base, stm_lcd_touch_point_t *point, size_t *count)
{
    static const uint8_t request[11] = {0xB5, 0xAB, 0xA5, 0x5A, 0, 0, 0, 8, 0, 0, 0};
    uint8_t response[8] = {0};
    *count = 0;
    stm_err_t err = stm_lcd_io_write(base->io, request, sizeof request);
    if (err != STM_OK)
        return err;
    err = stm_lcd_io_read(base->io, response, sizeof response);
    if (err != STM_OK)
        return err;
    if (response[1] > 1u)
        return STM_ERR_VERIFY;
    if (!response[1])
        return STM_OK;
    point->x = (uint16_t)(((uint16_t)(response[2] & 0x0fu) << 8) | response[3]);
    point->y = (uint16_t)(((uint16_t)(response[4] & 0x0fu) << 8) | response[5]);
    point->id = 0;
    *count = 1;
    return STM_OK;
}

static void destroy(stm_lcd_touch_handle_t t)
{
    free(t);
}

static const stm_lcd_touch_ops_t ops = {.reset = reset, .read_data = read_data, .destroy = destroy};

stm_err_t lcd_touch_axs15231b_create(const lcd_touch_axs15231b_config_t *c,
                                     stm_lcd_touch_handle_t *out)
{
    if (!c || !out)
        return STM_ERR_INVALID_ARG;
    if (*out)
        return STM_ERR_INVALID_STATE;
    if (!c->io || !c->io->ops || !c->io->ops->write || !c->io->ops->read ||
        (c->reset && !c->delay_ms))
        return STM_ERR_INVALID_CONFIG;
    struct lcd_touch_axs15231b_context *t = calloc(1, sizeof(*t));
    if (!t)
        return STM_ERR_NO_MEM;
    stm_err_t err = stm_lcd_touch_base_init(&t->base, &ops, c->io, &c->coordinates);
    if (err != STM_OK)
    {
        free(t);
        return err;
    }
    t->config = *c;
    *out = &t->base;
    return STM_OK;
}
