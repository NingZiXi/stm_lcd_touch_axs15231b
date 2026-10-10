/**
 * @file    example.c
 * @brief   演示通用触摸接入及板级配置边界。
 */
#include "stm_lcd_touch_axs15231b.h"
#include "stm_lcd_i2c_f4.h"
static stm_lcd_i2c_f4_t i2c;
static stm_lcd_touch_handle_t touch;

static void delay_ms(void *context, uint32_t ms)
{
    (void)context;
    HAL_Delay(ms);
}

extern stm_err_t board_init(stm_lcd_i2c_f4_config_t *io_config,
                            lcd_touch_axs15231b_config_t *touch_config);

int main(void)
{
    HAL_Init();
    stm_lcd_i2c_f4_config_t io_config = {.address_7bit = LCD_TOUCH_AXS15231B_ADDR_7BIT,
                                         .timeout_ms = 5};
    lcd_touch_axs15231b_config_t config = {
        .delay_ms = delay_ms,
        .coordinates = {.x_max = 170, .y_max = 560, .swap_xy = 1, .mirror_y = 1}};
    stm_err_t err = board_init(&io_config, &config);
    if (err == STM_OK)
        err = stm_lcd_i2c_f4_init(&i2c, &io_config);
    config.io = &i2c.io;
    if (err == STM_OK)
        err = lcd_touch_axs15231b_create(&config, &touch);
    if (err == STM_OK && config.reset)
        err = stm_lcd_touch_reset(touch);
    if (err != STM_OK)
    {
        (void)stm_lcd_touch_delete(&touch);
        if (i2c.io.ops)
            (void)stm_lcd_i2c_f4_deinit(&i2c);
        for (;;)
        {
            HAL_Delay(1000);
        }
    }
    for (;;)
    {
        stm_lcd_touch_point_t point;
        size_t count = 0;
        err = stm_lcd_touch_read_data(touch);
        if (err == STM_OK)
            err = stm_lcd_touch_get_data(touch, &point, 1, &count);
        // 仅在成功且 count 为 1 时使用触点；LVGL 的重试和过期由 port 管理。
        HAL_Delay(20);
    }
}
