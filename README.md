# stm_lcd_touch_axs15231b

可移植 AXS15231B 触摸协议实现，只依赖 `stm_lcd` / `stm_common`，不依赖 HAL/LVGL。

`lcd_touch_axs15231b_create(&config, &touch)` 返回 `stm_lcd_touch_handle_t`。配置包含通用 IO、逻辑坐标变换 `.coordinates`、可选 reset/delay/control_context。后续使用 `stm_lcd_touch_reset/read_data/get_data/delete`，移除旧操作 API。

保留 11 字节写请求 → STOP → 独立 8 字节读响应；只解析一个触点，response[1]>1 返回 VERIFY。不改为寄存器读取。交换/边界/镜像只由公共框架处理一次；get_data 为非消费快照、不产生 I2C。错误清除快照，不代表既有异常帧问题已修复。

IO 及硬件由调用方持有；port 借用期间不能删除/复位。离线恢复由 port 探测，不重置可能与屏幕共享的 reset。构造失败保持空输出；可选 reset 缺失返回 NOT_SUPPORTED。

HAL I2C 适配在 `stm_lcd/adapters/stm32f4`，使用 raw Master_Transmit/Receive。CMake 只复用 stm_lcd target 或同级源码，缺失报错。测试覆盖请求、解析、通信失败、畸形帧、逻辑变换、越界与重复快照读取。

## 验证与许可

2026-10-10，在 STM32F407 + AXS15231B 消费工程以 21 MHz SPI、170×560 原生竖屏、RGB565、两个 16 行普通 SRAM 缓冲运行，用户确认显示与触摸正常。该结论仅覆盖上述配置，未作为独立长时间 soak 验收，也不宣称既有触摸畸形帧问题已修复。当前为未发布开发提交，未创建新 tag 或 Release。

自有源码沿用维护者原 MIT，见 [LICENSE](LICENSE)。 必要第三方许可正文保留于 [LICENSE-ESPRESSIF](LICENSE-ESPRESSIF)。
