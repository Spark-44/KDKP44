# 后轮编码器更换与代码修改记录

当前工程位置：

```text
D:\ADS\source\subject 2 5.27.1\kart_508-kmy
```

注意：实际修改的工程目录是 `kart_508-kmy` 子工程，不是外层目录 `D:\ADS\source\subject 2 5.27.1`。

## 1. 后轮编码器初始化方式

文件：

```text
code/peripheral.c
code/peripheral.h
```

后轮编码器由原来的正交编码器模式，改成了“脉冲 + 方向”模式。

当前初始化：

```c
encoder_dir_init(ENCODER_QUADDEC, ENCODER_QUADDEC_A, ENCODER_QUADDEC_B);
```

当前宏定义：

```c
#define ENCODER_QUADDEC   (TIM2_ENCODER)
#define ENCODER_QUADDEC_A (TIM2_ENCODER_CH1_P33_7)
#define ENCODER_QUADDEC_B (TIM2_ENCODER_CH2_P33_6)
```

当前接线对应：

```text
P33.7 = 脉冲输入
P33.6 = 方向输入
GND   = 共地
VCC   = 按编码器规格供电
```

使用的逐飞库函数：

```c
encoder_dir_init()
encoder_get_count()
```

## 2. 累计脉冲与累计距离

文件：

```text
code/rear_motor/rear_motor.c
code/rear_motor/rear_motor.h
```

新增累计脉冲变量：

```c
static int32 total_encoder_pulses = 0;
```

每次 10ms 编码器采样后累计：

```c
total_encoder_pulses += (int32)encoder_10ms;
```

新增接口：

```c
int32 rear_motor_get_total_encoder_pulses(void);
float rear_motor_get_total_distance_m(void);
void rear_motor_clear_odometer(void);
```

## 3. 距离标定参数

文件：

```text
code/rear_motor/rear_motor.h
```

当前后轮参数：

```c
#define REAR_WHEEL_DIAMETER_M        0.24f
#define REAR_ODOMETER_PULSES_PER_WHEEL_REV  2122.0f
#define REAR_DISTANCE_PER_PULSE_M    (REAR_WHEEL_CIRCUM_M / REAR_ODOMETER_PULSES_PER_WHEEL_REV)
```

含义：

```text
轮胎直径：0.24 m
轮胎一圈距离：约 0.754 m
实测一圈脉冲：2122
每个脉冲距离：0.754 / 2122
```

累计距离计算：

```c
return (float)total_encoder_pulses * REAR_DISTANCE_PER_PULSE_M;
```

## 4. 编码器反馈方向取反

因为之前出现“目标正速度，实际速度为负”的问题，所以增加了反馈方向取反。

文件：

```text
code/rear_motor/rear_motor.h
code/rear_motor/rear_motor.c
```

方向宏：

```c
#define REAR_ENCODER_FEEDBACK_DIRECTION (-1)
```

编码器增量计算：

```c
encoder_10ms = (int16)((int32)REAR_ENCODER_FEEDBACK_DIRECTION * (int32)calculate_delta(current_count, last_encoder_count));
```

影响：

```text
encoder_10ms 符号取反
totalPulse 符号随之取反
dist 符号随之取反
actual_mps 速度反馈方向随之取反
```

## 5. 速度计算方式

文件：

```text
code/rear_motor/rear_motor.c
code/rear_motor/rear_motor.h
```

速度从原来的 10ms 瞬时值，改成 100ms 平均值，降低抖动。

当前速度计算：

```c
actual_mps = (float)encoder_100ms * REAR_DISTANCE_PER_PULSE_M * REAR_SPEED_CALIBRATION_FACTOR / 0.1f;
```

其中：

```c
#define REAR_SPEED_CALIBRATION_FACTOR 0.7557f
```

这个速度校正系数来自一次实测：

```text
真实速度：1.5 m/s
串口平均速度：约 1.9849 m/s
校正系数：1.5 / 1.9849 ≈ 0.7557
```

注意：`REAR_SPEED_CALIBRATION_FACTOR` 只校正速度，不校正累计距离 `dist`。

## 6. PID 目标脉冲换算

文件：

```text
code/rear_motor/rear_motor.c
```

PID 的目标脉冲换算已改成使用标定后的每脉冲距离：

```c
float target_pulses = target_mps * 0.1f / REAR_DISTANCE_PER_PULSE_M;
```

不再使用旧的：

```c
REAR_EFFECTIVE_PPR = 1024 * 1.6
```

这样速度、距离、PID 控制使用同一套距离标定。

## 7. 串口输出内容

文件：

```text
user/cpu0_main.c
```

普通后轮遥测输出：

```text
[REAR-SPEED] target=... actual=... speed=...mps pwm=... enc10=... enc100=... totalPulse=... dist=...
```

字段含义：

```text
target     当前后轮目标速度
actual     当前后轮实际速度
speed      明确标出的实际速度，单位 m/s
pwm        当前后轮 PWM
enc10      10ms 编码器脉冲增量
enc100     100ms 编码器脉冲累计
totalPulse 总累计脉冲
dist       编码器累计距离
```

record 空闲诊断输出：

```text
[ENC-DIAG] enc10=... enc100=... totalPulse=... dist=... actual=... pwm=...
```

这个输出用于手动转轮子时观察编码器脉冲和距离。

## 8. record 空闲编码器诊断

文件：

```text
user/cpu0_main.c
```

在以下条件满足时，每 10ms 输出编码器诊断：

```text
main_mode == Guandao_Portion2_Recode
remote_control_is_active() == 0
conrtol_mode == IDLE
```

作用：

```text
record 模式下没有遥控控制、没有运行路线时，可以手动转轮子观察 enc10、totalPulse、dist。
```

## 9. 遥控速度平滑处理

文件：

```text
code/remote_control.c
```

CH2 死区加大：

```c
#define REMOTE_CONTROL_CHANNEL_DEAD_ZONE (50)
```

原因：之前 CH2 中位附近会出现 `targetSpeed=-0.03`，导致松手后仍有小 PWM。

新增速度斜坡：

```c
#define REMOTE_CONTROL_SPEED_RAMP_STEP_MPS (0.03f)
static float remote_command_speed_mps = 0.0f;
```

新增函数：

```c
remote_control_ramp_speed_command()
```

作用：

```text
防止遥控器目标速度从 0 瞬间跳到 1m/s。
每 10ms 目标速度最多变化 0.03m/s。
等效加速度限制约 3m/s²。
```

遥控周期控制中现在使用：

```c
remote_command_speed_mps = remote_control_ramp_speed_command(remote_control_limit_speed_command(remote_target_speed_mps));
rear_motor_set_target_mps(remote_command_speed_mps);
```

## 10. 遥控速度上限保护

文件：

```text
code/rear_motor/rear_motor.c
code/rear_motor/rear_motor.h
code/remote_control.c
```

新增接口：

```c
void rear_motor_set_speed_limit_mps(float limit_mps);
void rear_motor_clear_speed_limit(void);
```

遥控 record 模式下启用 1m/s 限速：

```c
rear_motor_set_speed_limit_mps(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS);
```

如果超过速度限制，会去掉同方向前馈，并限制同方向 PWM：

```text
正向超速：去掉正向前馈，正向 PWM 限制为 0
反向超速：去掉反向前馈，反向 PWM 限制为 0
```

## 11. 屏幕调试显示

文件：

```text
code/peripheral.c
```

增加了后轮编码器相关显示，例如：

```text
Enc100
```

用于屏幕上查看后轮 100ms 编码器脉冲。

## 12. 验证脚本

新增或修改的相关检查脚本：

```text
tests/check_rear_encoder_direction_mode.ps1
tests/check_rear_encoder_odometer.ps1
tests/check_rear_motor_serial_telemetry.ps1
tests/check_record_idle_encoder_diag.ps1
tests/check_remote_speed_governor.ps1
tests/check_remote_control_port.ps1
```

这些脚本主要检查：

```text
后轮是否使用 encoder_dir_init
P33.7 是否作为脉冲输入
P33.6 是否作为方向输入
是否存在 totalPulse
是否存在 dist
是否存在 speed=mps
是否使用 2122 脉冲/圈
是否编码器反馈取反
是否使用 0.7557 速度校正
遥控是否使用 CH2 死区 50
遥控是否使用速度斜坡 0.03m/s
遥控限速是否开启
```

## 13. 当前后轮编码器总状态

```text
编码器类型：脉冲 + 方向
逐飞库函数：encoder_dir_init + encoder_get_count
脉冲脚：P33.7
方向脚：P33.6
一圈脉冲标定：2122
轮胎直径：0.24m
累计距离：按 2122 脉冲/圈计算
速度反馈：100ms 平均 + 0.7557 校正
反馈方向：已取反
串口显示：speed / enc10 / enc100 / totalPulse / dist
遥控速度：CH2 死区 50 + 速度斜坡 0.03m/s 每 10ms
```

