/**
 * @file    stm_lcd_touch_axs15231b.h
 * @brief   提供 AXS15231B 通用触摸构造接口。
 */
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
    stm_lcd_io_handle_t io;             // 借用的原始 I2C IO，写读为独立事务。
    stm_lcd_touch_config_t coordinates; // 交换轴、边界检查与镜像由 stm_lcd 处理。
    void (*delay_ms)(void *, uint32_t); // 配置复位时必须提供。
    stm_err_t (*reset)(void *, int);    // 可选启动复位，不用于掉线重连。
    void *control_context;              // 借用的复位与延时上下文。
} lcd_touch_axs15231b_config_t;

/**
 * @brief 创建 AXS15231B 触摸，只复制配置、借用 IO，不执行硬件初始化。
 *
 * @param config 有效配置及持续有效的 IO、控制上下文
 * @param out 输出通用触摸句柄地址；初始必须为 NULL
 * @return STM_OK；配置、内存或所有权错误时失败；空输出保持 NULL，非空输出不覆盖。
 */
stm_err_t lcd_touch_axs15231b_create(const lcd_touch_axs15231b_config_t *config,
                                     stm_lcd_touch_handle_t *out);
#ifdef __cplusplus
}
#endif
#endif
