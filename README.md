# stm_lcd_touch_axs15231b：AXS15231B I²C 触摸

匹配 AgentDock 同型号触摸，独立于屏幕 SPI 组件。7 位 I²C 地址 **0x3B**，STM32 HAL 的 `DevAddress` 为 `0x3B << 1`；先向设备写 11 字节 `B5 AB A5 5A 00 00 00 08 00 00 00`，然后以**独立 I²C 读事务**取 8 字节，不是 `HAL_I2C_Mem_Read`。当前协议只解一个点，其他点数作为 `STM_ERR_VERIFY` 清除本帧状态；零点帧立即释放。帧含 gesture/count/X/Y；与 Espressif 源码一样从 [2..5] 提取 12 位坐标，不依赖 C 位域布局。

## API 与坐标

```c
lcd_touch_axs15231b_handle_t touch = NULL;
lcd_touch_axs15231b_config_t cfg = {
    .write = board_write, .read = board_read, .io = &board,
    .x_max = 560, .y_max = 170, .mirror_x = 1, .mirror_y = 1,
};
stm_err_t err = lcd_touch_axs15231b_create(&cfg, &touch);
if (err == STM_OK) err = lcd_touch_axs15231b_reset(touch);
if (err == STM_OK) err = lcd_touch_axs15231b_read_data(touch);
lcd_touch_axs15231b_point_t p; size_t n = 0;
if (err == STM_OK) err = lcd_touch_axs15231b_get_data(touch, &p, 1, &n);
lcd_touch_axs15231b_delete(&touch);
```

`x_max/y_max` 为变换后逻辑尺寸，不含边界值。先交换轴、检查坐标、再镜像，镜像公式 `size - 1 - value`，避免原例中 `size - value` 的越界端点。AgentDock 的配置是 raw X 对应 560、raw Y 对应 170，且两轴镜像；LVGL 的旋转、swap 与本模组实际安装方向仍要在实板校准，不能重复变换。触摸 INT、RST 和 SPI 屏幕背光均由板级持有，复位若配置则 `delay_ms` 必需；复位低 200ms、高 200ms。两笔 I²C 事务需在板级整体加锁，不能让共享总线的别的事务插入；不得在中断中执行阻塞 HAL，DMA/缓存一致性由板级负责。

`create` 复制配置、不访问硬件，要求输出空；`delete` 清空句柄。通信失败与畸形点数清空缓存，回调错误原样传递，零点帧清空。`get_data` 将最多 `capacity` 个缓存点复制并返回实际点数；`capacity=0` 可传空数组。`STM_ERR_INVALID_ARG`、`INVALID_CONFIG`、`INVALID_STATE`、`NO_MEM`、`VERIFY` 与 `stm_common` 一致。无 RTOS、RTT、HAL 依赖；板级适配参见 [`examples/stm32_hal/`](examples/stm32_hal/README.md)。

## 构建

```cmake
add_subdirectory(Lib/stm_lcd_touch_axs15231b)
target_link_libraries(your_firmware PRIVATE stm_lcd_touch_axs15231b)
```

自动复用 `stm_common` target 或同级源码，缺失时默认固定提交下载；离线可设 `STM_COMMON_FETCH=OFF`。主机测试：`cmake -S tests -B build && cmake --build build && ctest --test-dir build --output-on-failure`。已覆盖基本帧、传输故障和错误路径；**协议参考 Espressif `esp_lcd_axs15231b`，保留 Apache-2.0 授权见 `NOTICE` / `LICENSE-ESPRESSIF`；尚未对 F407 实板验证**。

