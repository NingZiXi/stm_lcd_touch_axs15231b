# stm_lcd_touch_axs15231b：Agent 工作指南

## 范围与阅读顺序

本文件约束本组件目录及子目录；与上级指令合并使用，存在更深层 AGENTS.md 时遵循其局部约束。README 面向使用者，本文件用于 Agent 的接入、修改和验证。

先检查消费工程构建方式、已存在的 target、外设初始化及用户未提交改动，再读本组件 README/CMake/公开头；不覆盖已有业务代码。

## 接入与不可破坏的约束

- 先读 `include/stm_lcd_touch_axs15231b.h`、`src/stm_lcd_touch_axs15231b.c`、`examples/stm32_hal/example.c`；再读公共 touch/IO 接口。
- 使用原始 I2C IO，7 位地址 0x3B 不提前左移；F4 适配负责 HAL 地址左移。
- 必须保留 11 字节写请求 → STOP → 独立 8 字节读响应；不能改成 HAL_I2C_Mem_Read 或寄存器事务。
- 只解析一个点，response[1]>1 返回 VERIFY，不能假称异常帧问题已修复或顺便增加多点支持。
- 构造只分配并复制配置；初始输出为空；reset 可选，配置 reset 时 delay 必需。IO 及上下文由应用持有。
- 原始坐标交由 stm_lcd 统一 swap → 边界 → mirror，不能再在驱动或应用变换。get_data 不读总线、不消费快照。
- 驱动不依赖 HAL/LVGL，不实现离线重试/过期策略；由 port 处理，不新增后台任务。
- 不在掉线恢复时自动复位，触摸复位可能与面板共享。port 借用期间不能复位或删除。
- 两 AXS 共用固定 stm_lcd v1.0.0 提交 `c359e54a657be38aec90c797ea19ee3d492d9284`，依赖选项/优先级以 CMake 和 README 为准，不追踪 main。
- 主机测试需同级 stm_common/stm_lcd，保留报文、失败、越界、方向与非消费快照，以及 C/C++ 消费编译。

## 编码与注释

- 中文交流；C11；Allman 括号、4 空格缩进，按本目录 `.clang-format` 格式化，只处理自有代码。
- `.c` / `.h` 文件头仅有 `@file` 和一句 `@brief`，不写 author/date/version/copyright。
- 头文件结构体外的公开函数声明使用 Doxygen：brief、真实参数语义、关键返回值；void 不虚构返回值。
- struct/enum 可在上方写一句说明；成员和枚举项仅在同行右侧用简短 `//`，不使用 `/**< */`，不把字段注释写在上方。
- C 实现不写函数 Doxygen；函数前至多一句 `//`。复杂约束最多 1–3 行，不写教学块、历史或章节分隔线。
- 不改维护者 MIT LICENSE；保留现有第三方来源/许可说明，不把第三方内容直接改署名。

## 验证与交付

从本组件根目录执行，需要主机 C11/C++17 编译器、CMake 3.22+、Ninja：

```sh
cmake -S tests -B build/tests -G Ninja
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

没有同级依赖时先准备源码，不能删测试或伪造空 target 来通过。主机测试不用 MCU 交叉编译器。测试包含 C/C++ 头文件消费；改生命周期/协议时增加对应失败路径测试。

接入消费工程还须使用其工具链编译固件，并运行可用的真实 LVGL / HAL 回归。仅文档注释整理时校验非注释 C token 不变。报告实际依赖版本/提交、修改文件、执行命令、测试/编译结果及未验证项；编译成功不等于实板通过。

未经用户明确授权不烧录、擦写存储、推送、发布版本、修改远程仓库或自动升级依赖。缺少接线、共享复位、目标器件或构建配置时先询问，不猜测。不要修改消费者的 CubeMX 生成文件、HAL 或供应商 LVGL；必要配置列出操作步骤。

## 依赖回归

在本组件根目录执行以下命令，需要 Python 3 和 Git。测试还包含固定提交拉取和双组件解析，默认本地 Git 仓库必须包含 README 标注的固定 stm_lcd v1.0.0 提交，普通无 .git 的拷贝不能替代：

```sh
python tests/test_dependency.py --lcd-source /absolute/path/to/stm_lcd --common-source /absolute/path/to/stm_common
```

可用 `--build-dir <新目录>` 保留日志，`--lcd-repository <可信仓库>` 显式选择拉取来源；不擅自改固定提交来让测试通过。
