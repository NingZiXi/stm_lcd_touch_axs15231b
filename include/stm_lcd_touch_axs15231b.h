/**
 * @file stm_lcd_touch_axs15231b.h
 *
 * @brief AXS15231B 触摸接口，统一错误码与不透明句柄。
 */
#ifndef STM_LCD_TOUCH_AXS15231B_H
#define STM_LCD_TOUCH_AXS15231B_H
#include <stddef.h>
#include <stdint.h>
#include "stm_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * 当前协议解析只支持一个触点。
 */
#define LCD_TOUCH_AXS15231B_MAX_POINTS 1u

/**
 * 7 位 I2C 地址；
 * HAL DevAddress 由板级左移一位。
 */
#define LCD_TOUCH_AXS15231B_ADDR_7BIT 0x3Bu

typedef struct lcd_touch_axs15231b_context *lcd_touch_axs15231b_handle_t;

typedef struct
{
    uint16_t x, y; /**< 变换后的逻辑像素坐标，范围 [0,x_max) / [0,y_max)。 */
    uint8_t id;    /**< 当前单点解析固定为 0，不提取硬件触点 ID。 */
} lcd_touch_axs15231b_point_t;

typedef struct
{
    stm_err_t (*write)(void *io,
                       const uint8_t *data,
                       size_t bytes); /**< 同步写请求，bytes 为字节数，当前为 11 字节。 */
    stm_err_t (*read)(void *io,
                      uint8_t *data,
                      size_t bytes); /**< 独立同步读事务，当前为 8 字节； 不是寄存器 Mem_Read。 */
    void (*delay_ms)(void *io, uint32_t ms); /**< 配置 reset 时必需； 毫秒阻塞延时。 */
    stm_err_t (*reset)(void *io, int high);  /**< 可选，设置低有效 RST 电平，high 为 0/1。 */
    void *io;                            /**< 借用上下文，允许 NULL； 板级保存 I2C 句柄及地址。 */
    uint16_t x_max, y_max;               /**< 非零的变换后逻辑尺寸，上限不包含。 */
    uint8_t swap_xy, mirror_x, mirror_y; /**< 仅允许 0/1； 先交换轴并检查边界，再镜像。 */
} lcd_touch_axs15231b_config_t;

/**
 * @brief 创建触摸控制对象，不访问硬件或接管板级资源。
 *
 * @param config 配置被复制；
 *     回调及 io 资源在实例存续期保持有效。
 * @param out 输出句柄地址；
 *     *out 必须为 NULL，失败保持原值。
 *
 * @return STM_OK、STM_ERR_INVALID_ARG、STM_ERR_INVALID_STATE、
 *     STM_ERR_INVALID_CONFIG 或 STM_ERR_NO_MEM。
 *
 * @note 使用 calloc/free；
 *     无 HAL、日志或 RTOS 依赖，应用串行化访问，不从 ISR 调用。
 */
stm_err_t lcd_touch_axs15231b_create(const lcd_touch_axs15231b_config_t *config,
                                     lcd_touch_axs15231b_handle_t *out);

/**
 * @brief 释放控制对象并清空句柄，不关闭 I2C 或释放 GPIO。
 *
 * @param handle 句柄地址；
 *     *handle 为 NULL 时也成功，handle 本身不能为 NULL。
 *
 * @return STM_OK 或 STM_ERR_INVALID_ARG。
 *
 * @note 删除前停止并发访问；
 *     其他句柄别名不会自动清空，删除后禁止使用。
 */
stm_err_t lcd_touch_axs15231b_delete(lcd_touch_axs15231b_handle_t *handle);

/**
 * @brief 清空触点缓存，并执行可选复位：低 200ms、高 200ms。
 *
 * @param touch 有效触摸句柄。
 *
 * @return STM_OK、STM_ERR_INVALID_ARG 或 reset 回调原样返回的错误。
 *
 * @note 未提供 reset 时仅清缓存。
 *     共享 LCD RST 的情况由板级协调，勿在显示初始化后重复复位。
 */
stm_err_t lcd_touch_axs15231b_reset(lcd_touch_axs15231b_handle_t touch);

/**
 * @brief 写入 11 字节请求，再以独立 I2C 事务读取并解析 8 字节响应。
 *
 * @param touch 有效触摸句柄。
 *
 * @return STM_OK、STM_ERR_INVALID_ARG、STM_ERR_VERIFY 或通信回调原样返回的错误。
 *
 * @note count 大于 1 返回 VERIFY；
 *     通信失败/畸形帧清缓存，零点帧释放。
 *     交换轴后越界的点被丢弃，返回 STM_OK 且缓存为空；
 *     镜像使用 size-1-value。
 *     每次读取前清缓存，必须检查返回值；
 *     不校验芯片 ID，不自动重试。
 *     板级保证写请求和读响应之间没有其他共享总线事务插入；
 *     无内置锁或异步 DMA 接口。
 */
stm_err_t lcd_touch_axs15231b_read_data(lcd_touch_axs15231b_handle_t touch);

/**
 * @brief 复制本地缓存的触点，不访问 I2C，不消费或清除缓存。
 *
 * @param touch 有效触摸句柄。
 * @param points 调用者提供的数组，capacity 非零时不能为 NULL。
 * @param capacity 数组容量，以触点数量计；
 *     为 0 时允许 points 为 NULL。
 * @param count 必需，返回实际复制的点数（0 或 1）；
 *     失败时若非空则置 0。
 *
 * @return STM_OK 或 STM_ERR_INVALID_ARG。
 *
 * @note capacity 为 0 时 *count 为 0，不是查询缓存数量的接口；
 *     输出数据在返回前复制完成。
 */
stm_err_t lcd_touch_axs15231b_get_data(lcd_touch_axs15231b_handle_t touch,
                                       lcd_touch_axs15231b_point_t *points,
                                       size_t capacity,
                                       size_t *count);

#ifdef __cplusplus
}
#endif
#endif
