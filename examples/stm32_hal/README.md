# STM32F4 触摸接入示例

[example.c](example.c) 演示 board_init → I2C IO 适配 → 构造通用 touch → 可选复位 → 20ms 读取快照。仅在返回成功且 count 为 1 时使用坐标，否则按释放处理。

应用实现 example.c 中声明的 `board_init`，提供时钟、I2C、SDA/SCL、上拉及复位协调。地址 0x3B 为未左移 7 位值；HAL 适配负责左移。示例方向为 170×560、swap_xy=1、mirror_x=0、mirror_y=1，其他模组需实际验证。

CMake 链接 `stm_lcd_touch_axs15231b` 与 `stm_lcd_i2c_f4`，适配要求真实 `stm32_hal` target。原始写请求与读响应之间保留 STOP，不替换为寄存器读取。

示例是板级占位程序，不是可直接烧录的完整项目。LVGL 场景直接把 touch 交给 port，其内部接管采样、过期释放与离线重试，不保留此示例的并行采样循环。共享复位由板级在接入前协调，掉线恢复不自动复位。

初始化失败删除触摸后再清除 IO，不销毁借用 HAL 资源。完整约束见[组件 README](../../README.md)；未经确认不烧录或擦写。
