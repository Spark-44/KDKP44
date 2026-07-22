# Run 模式左转语音别名移植设计

## 目标

将参考工程 `subject 2 5.27.1` 中新增的三个左转语音命令移植到当前工程，使 Run 模式能够通过 `0x25`、`0x2D`、`0x2E`、`0x2F` 四个输入触发完全相同的左转行驶动作。

## 修改范围

1. 在 `code/offline_voice.h` 中增加：
   - `OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_1` = `0x2D`
   - `OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_2` = `0x2E`
   - `OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_3` = `0x2F`
2. 将 `OFFLINE_VOICE_CMD_MAX` 从 `0x2C` 更新为 `0x2F`。
3. 在 `user/cpu0_main.c` 的 Run 模式语音命令分发中，将三个别名与现有 `OFFLINE_VOICE_CMD_TURN_LEFT_DRIVE` 合并到同一分支，统一调用：
   `Portion2_Fixed_Action_Start(VOICE_DRIVE_ACTION_TURN_LEFT)`。

## 不修改的行为

- 不修改左转动作的速度、转角、持续时间、航向判断或停止条件。
- 不改变 `0x04` 左转灯命令和其他路线选择命令。
- 不改变非 Run 模式下忽略语音动作命令的现有保护。
- 不调整语音串口协议、校验和或应答格式。

## 数据流

语音模块发送合法帧 → UART2解析得到命令号 → Run模式回调分发 → 四个左转命令进入同一分支 → 启动现有左转固定动作。

## 测试

先增加静态回归测试，验证三个别名的数值、最大命令号以及四个命令共用左转动作分支；确认测试在当前代码上因功能缺失而失败，再进行最小实现。实现后运行新增测试、相关语音测试和现有完整测试集，并如实报告既有失败项。
