# stm_lcd_touch_axs15231b

可移植 AXS15231B 触摸协议实现，只依赖 `stm_lcd` / `stm_common`，不依赖 HAL/LVGL。

`lcd_touch_axs15231b_create(&config, &touch)` 返回 `stm_lcd_touch_handle_t`。配置包含通用 IO、逻辑坐标变换 `.coordinates`、可选 reset/delay/control_context。后续使用 `stm_lcd_touch_reset/read_data/get_data/delete`，移除旧操作 API。

保留 11 字节写请求 → STOP → 独立 8 字节读响应；只解析一个触点，response[1]>1 返回 VERIFY。不改为寄存器读取。交换/边界/镜像只由公共框架处理一次；get_data 为非消费快照、不产生 I2C。错误清除快照，不代表既有异常帧问题已修复。

IO 及硬件由调用方持有；port 借用期间不能删除/复位。离线恢复由 port 探测，不重置可能与屏幕共享的 reset。构造失败保持空输出；可选 reset 缺失返回 NOT_SUPPORTED。

HAL I2C 适配在 `stm_lcd/adapters/stm32f4`，使用 raw Master_Transmit/Receive。CMake 优先复用 stm_lcd target/本地源码，缺失时自动获取固定提交，详见下方依赖说明。测试覆盖请求、解析、通信失败、畸形帧、逻辑变换、越界与重复快照读取。

## CMake 与依赖

```cmake
add_subdirectory(Lib/stm_lcd_touch_axs15231b)
target_link_libraries(your_firmware PRIVATE stm_lcd_touch_axs15231b)
```

独立依赖回归（含本地/离线/固定拉取及 C11/C++17，默认从本地 `stm_lcd` Git checkout 获取，不访问网络；真实网络检查可显式指定 `--lcd-repository`）：

```sh
python tests/test_dependency.py --lcd-source /absolute/path/to/stm_lcd --common-source /absolute/path/to/stm_common
```

独立接入只需添加本组件并链接其 target；不必手动下载 `stm_lcd`。两个 AXS 组件共用依赖，先添加任意一个均可，不重复获取。

`stm_lcd` 解析顺序：已有 target（含 alias） → `STM_LCD_SOURCE_DIR` → `FETCHCONTENT_SOURCE_DIR_STM_LCD` → 同级 `../stm_lcd` 源码 → 自动下载。默认固定已验证开发提交 `da713eecea80c5abb59973a3b003015800011b4a`，不跟随 `main`，不代表已创建正式版本。下载源码位于构建目录的 `_deps/stm_lcd-src`。

| 选项 | 默认值 | 用途 |
| --- | --- | --- |
| `STM_LCD_FETCH` | `ON` | 缺失时允许获取；`OFF` 仍可复用已有 target 或本地源码 |
| `STM_LCD_SOURCE_DIR` | 空 | 显式指定本地框架目录；无效时明确报错，不回退网络 |
| `FETCHCONTENT_SOURCE_DIR_STM_LCD` | 空 | 框架离线源码覆盖，同样可在关闭下载时使用 |
| `STM_LCD_GIT_REPOSITORY` | GitHub `NingZiXi/stm_lcd.git` | 可信镜像，例如 Gitee `https://gitee.com/nzxhg/stm_lcd.git` |

`stm_common` 保持优先复用 target/同级源码，缺失时固定获取 v1.0.0 提交；可使用 `STM_COMMON_FETCH=OFF`、`STM_COMMON_GIT_REPOSITORY` 和 `FETCHCONTENT_SOURCE_DIR_STM_COMMON`。完全离线请同时提供两个依赖的 target/本地源码并关闭下载。`FETCHCONTENT_FULLY_DISCONNECTED=ON` 且框架首次缺失时会明确报错。

```cmake
# 可选：使用可信镜像；两个 AXS 组件共享这些选项。
set(STM_LCD_GIT_REPOSITORY "https://gitee.com/nzxhg/stm_lcd.git" CACHE STRING "")
# 或离线使用本地框架，并禁止下载：
# set(STM_LCD_SOURCE_DIR "D:/components/stm_lcd" CACHE PATH "")
# set(STM_LCD_FETCH OFF CACHE BOOL "")
```

## 验证与许可

2026-10-10，在 STM32F407 + AXS15231B 消费工程以 21 MHz SPI、170×560 原生竖屏、RGB565、两个 16 行普通 SRAM 缓冲运行，用户确认显示与触摸正常。该结论仅覆盖上述配置，未作为独立长时间 soak 验收，也不宣称既有触摸畸形帧问题已修复。当前为未发布开发提交，未创建新 tag 或 Release。

自有源码沿用维护者原 MIT，见 [LICENSE](LICENSE)。
