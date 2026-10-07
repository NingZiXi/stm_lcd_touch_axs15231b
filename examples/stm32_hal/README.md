# STM32F407 HAL 接入参考

`example.c` 使用独立 HAL I2C 发送 11 字节、接收 8 字节，默认 7 位地址 0x3B（HAL DevAddress = 0x76）。实现 `board_init()` 填入真实 I2C、引脚/上拉与可选 INT 中断管理；中断只通知任务轮询，不在 ISR 调用阻塞读。现有 F407 IOC 无此触摸的 I²C/引脚分配。示例 intentionally 返回配置错误防止误运行；根据实物标定方向与镜像。尚未实板验证。
