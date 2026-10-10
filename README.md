# stm_lcd_touch_axs15231b

[![version 1.0.0](https://img.shields.io/badge/version-1.0.0-5364b5?style=flat-square)](https://github.com/NingZiXi/stm_lcd_touch_axs15231b/releases/tag/v1.0.0)

可移植 AXS15231B 单触点协议驱动，只依赖 `stm_lcd` / `stm_common`，不依赖 HAL 或 LVGL。构造返回 `stm_lcd_touch_handle_t`，后续通过公共接口读取快照。

## 🤖 让 Agent 帮助接入

复制以下 Prompt，将 `<...>` 替换为实际需求：

> 将 [stm32-hal-lib](https://github.com/NingZiXi/stm32-hal-lib) 中的 `stm_lcd_touch_axs15231b` 接入当前工程，实现 `<功能>`。硬件：`<MCU/器件型号、外设与引脚、屏幕尺寸与方向（按需填写）>`。先读组件 AGENTS.md 和 README，核对配置，保留已有代码；缺失信息先询问，不猜接线。完成后说明版本、编译结果和未验证项；未经确认不烧录或擦写。

💡 可使用 [Gitee 镜像](https://gitee.com/nzxhg/stm32-hal-lib)。需要业务入口和日志时，参考 [skills/README.md](https://github.com/NingZiXi/stm32-hal-lib/blob/main/skills/README.md)（仅适用于已有 CubeMX1 CMake 工程）；编译通过不代表实板验证通过。

## 目录与职责

- `include/stm_lcd_touch_axs15231b.h`：7 位地址、配置与构造函数。
- `src/`：请求写入、响应读取、单触点解析，不负责 LVGL 采样策略。
- `examples/stm32_hal/`：I2C 与板级初始化占位示例。
- `tests/`：请求协议、异常帧、快照与 C/C++ 头文件回归。

HAL 原始 I2C 适配在 `stm_lcd/adapters/stm32f4`。引脚、供电、复位协调由应用负责；采样、过期释放、掉线探测由 port 负责，不增加第二套轮询状态机。

## 最小使用过程

以下构造函数要求原始 I2C IO 已初始化且已协调硬件复位；IO 地址使用未左移的 `LCD_TOUCH_AXS15231B_ADDR_7BIT`（0x3B）。方向配置仅为当前 170×560 模组示例。

```c
#include "stm_lcd_touch_axs15231b.h"

stm_err_t create_touch(stm_lcd_io_handle_t io, stm_lcd_touch_handle_t *touch)
{
    lcd_touch_axs15231b_config_t config =
    {
        .io = io,
        .coordinates =
        {
            .x_max = 170,
            .y_max = 560,
            .swap_xy = 1,
            .mirror_x = 0,
            .mirror_y = 1,
        },
    };
    return lcd_touch_axs15231b_create(&config, touch);
}

stm_err_t sample_touch(stm_lcd_touch_handle_t touch,
                       stm_lcd_touch_point_t *point, size_t *count)
{
    *count = 0;
    stm_err_t err = stm_lcd_touch_read_data(touch);
    if (err != STM_OK)
    {
        return err;
    }
    return stm_lcd_touch_get_data(touch, point, 1, count);
}
```

调用前 `*touch` 必须为 NULL；采样片段要求 point/count 非空。仅在返回 STM_OK 且 `count == 1` 时使用触点，否则按释放处理。完整 HAL 接入见 [example.c](examples/stm32_hal/example.c) 和[示例说明](examples/stm32_hal/README.md)。LVGL 应用将句柄交给 port，不再同时调用自建采样循环。

## 协议、配置与快照

- 保留 **11 字节写请求 → STOP → 独立 8 字节读响应**；不能改为 `HAL_I2C_Mem_Read` 或重复起始的寄存器读取。
- 当前只解析一个触点；`response[1] > 1` 返回 STM_ERR_VERIFY，无触点返回空快照。
- `coordinates.x_max/y_max` 为非零、不包含端点的逻辑边界；布尔变换依次为 swap → 边界检查 → mirror，只由公共框架处理一次。
- `read_data` 访问总线并更新快照；`get_data` 不访问总线、不消费快照。失败、零触点及越界清空快照。
- 可选 reset 为低有效电平回调；配置 reset 时必须提供 delay_ms，控制上下文须持续有效。构造不执行复位或读取。
- 借用 IO，删除不销毁硬件；port 借用期间不能复位或删除触摸。释放时先删除 port，再删除触摸，最后清除 IO。

## 返回值与排查

| 结果 | 处理 |
| --- | --- |
| STM_OK、count 为零 | 无有效触点，不使用历史坐标作为按下 |
| STM_ERR_VERIFY | 畸形/不支持的帧，释放输入；不能据此直接判断总线离线 |
| STM_ERR_IO / STM_ERR_TIMEOUT | 检查供电、地址、上拉、I2C 配置及超时；由 port 处理掉线重试 |
| STM_ERR_INVALID_CONFIG | 检查 IO 写读能力、逻辑边界、布尔方向、reset/delay 配对 |
| STM_ERR_INVALID_STATE | 检查对象借用、访问重入或删除顺序 |
| STM_ERR_NOT_SUPPORTED | 可选 reset 缺失，不代表读取不可用 |

方向异常先检查交换后的逻辑边界及镜像端点，不在上层重复变换。掉线恢复只探测 IO，不自动触发可能影响屏幕的共享复位。本轮不宣称异常帧问题已修复，也不增加多点处理。

## CMake 与依赖


```cmake
add_subdirectory(Lib/stm_lcd_touch_axs15231b)
target_link_libraries(your_firmware PRIVATE stm_lcd_touch_axs15231b)
```

独立依赖回归包含本地/离线/固定拉取及 C11/C++17，默认从本地 `stm_lcd` Git checkout 获取且不访问网络。该 checkout 必须包含上述固定提交；普通源码拷贝可另外用 `--lcd-repository <本地Git镜像>` 提供提交来源。真实网络检查须显式指定远程 `--lcd-repository`：

```sh
python tests/test_dependency.py --lcd-source /absolute/path/to/stm_lcd --common-source /absolute/path/to/stm_common
```

独立接入只需添加本组件并链接其 target；不必手动下载 `stm_lcd`。两个 AXS 组件共用依赖，先添加任意一个均可，不重复获取。测试加 `--peer-source <另一AXS组件目录>` 可覆盖双组件添加顺序，加 `--build-dir <目录>` 可保留日志。

`stm_lcd` 解析顺序：已有 target（含 alias） → `STM_LCD_SOURCE_DIR` → `FETCHCONTENT_SOURCE_DIR_STM_LCD` → 同级 `../stm_lcd` 源码 → 自动下载。默认固定框架 `v1.0.0` 提交 `c359e54a657be38aec90c797ea19ee3d492d9284`，不跟随 `main`。下载源码位于构建目录的 `_deps/stm_lcd-src`。

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


## 主机测试

需要 CMake 3.22+、Ninja、支持 C11/C++17 的主机编译器。从**组件根目录**执行：

```sh
cmake -S tests -B build/tests -G Ninja
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

测试时需将 `stm_common` 和 `stm_lcd` 放在本组件同级（`stm_lcd` 自身只需同级 `stm_common`），或按测试 CMake 的要求准备依赖。嵌入式交叉编译器不用于运行主机测试。可显式传入 `-DCMAKE_C_COMPILER=<主机C编译器>` 和 `-DCMAKE_CXX_COMPILER=<主机C++编译器>`。

覆盖写请求/独立读响应、零触点、多点异常、通信失败、越界、方向端点及非消费快照。

## 验证范围与许可

2026-10-10，消费工程在 STM32F407 + AXS15231B、21 MHz SPI、170×560 原生竖屏、RGB565、两个 16 行普通 SRAM 缓冲的配置下完成显示和触摸验收。当前文档整理不改变这一运行配置。

该结果不等于其他模组、其他 MCU 或长时间稳定性验收；既有触摸畸形帧问题不能标记为已修复。当前正式版本为 `v1.0.0`，统一接口不兼容重构前 API；接入时以该版本公开头文件为准，不将旧版本的接口或验收结论套用到本版本。

维护者的 MIT 许可证保持原内容，见 [LICENSE](LICENSE)。源码中已有的第三方来源及许可说明保持保留。Agent 工作约束见 [AGENTS.md](AGENTS.md)。
