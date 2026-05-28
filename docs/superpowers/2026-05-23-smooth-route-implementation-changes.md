# 惯导平滑路线规划 —— 详细变更记录

> 变更日期：2026-05-23  
> 关联计划：`docs/superpowers/plans/2026-05-23-smooth-guandao-route.md`  
> 目标：在科目一惯导自动驾驶阶段引入路线平滑规划，消除车辆频繁左右打死的抖动现象，保留原始打点数据。

---

## 一、变更文件总览

| 序号 | 文件 | 变更类型 | 说明 |
|------|------|----------|------|
| 1 | `code/guandao.h` | 修改 | 在 `guandao_state` 结构体中新增平滑路线相关字段 |
| 2 | `code/guandao.c` | 修改 | 核心逻辑：新增平滑路线生成函数、统一路线访问、修改追踪逻辑 |
| 3 | `user/cpu0_main.c` | 修改 | 增强串口调试输出，便于观察平滑路线是否生效 |
| 4 | `docs/superpowers/plans/2026-05-23-smooth-guandao-route.md` | 新增 | 设计计划文档 |

---

## 二、`guandao.h` 变更详情

### 2.1 新增字段

在 `guandao_state` 结构体中新增以下字段，用于存储平滑后的规划路线：

```c
state_t planned_map[MAX_LENGTH_INDEX];    // 平滑规划后的路线点数组
int16 planned_length;                      // 规划后的路线点数量
uint8 plan_ready;                          // 平滑路线是否已生成 (0=未生成, 1=已生成)
```

### 2.2 新增函数声明

```c
void guandao_build_smooth_plan(guandao_state * state);   // 生成平滑规划路线
```

### 2.3 变更原因

- **保留原始数据**：`recode_map[]` 仍然保存推车打点的原始坐标，Flash 读写只涉及 `recode_map[]` 和 `length_index`，新增字段不会影响已有 Flash 数据格式。
- **非侵入式**：现有代码中所有直接访问 `recode_map[]` 的地方，都被统一封装成 `guandao_route_point()` 调用，自动判断使用原始路线还是规划路线。

---

## 三、`guandao.c` 核心变更详情

### 3.1 新增辅助函数 —— 统一路线访问

#### `guandao_route_length()`

```c
static int16 guandao_route_length(guandao_state *state)
{
    if(state->plan_ready && state->planned_length > 0) return state->planned_length;
    return state->length_index;
}
```

- 当 `plan_ready == 1` 且 `planned_length > 0` 时，返回规划路线的长度。
- 否则返回原始打点路线的长度。

#### `guandao_route_point()`

```c
static state_t guandao_route_point(guandao_state *state, int index)
{
    int16 route_length = guandao_route_length(state);
    if(route_length <= 0) return state->current_state;
    if(index < 0) index = 0;
    if(index >= route_length) index = route_length - 1;
    if(state->plan_ready && state->planned_length > 0) return state->planned_map[index];
    return state->recode_map[index];
}
```

- 统一获取路线上的某个点。
- 如果规划路线已就绪，返回 `planned_map[index]`；否则返回 `recode_map[index]`。
- 自动做越界保护。

### 3.2 平滑路线生成函数 —— `guandao_build_smooth_plan()`

这是本次变更的核心算法，使用 **Catmull-Rom 样条曲线插值** 生成平滑路径。

#### 算法原理

Catmull-Rom 样条通过相邻 4 个点（`p0, p1, p2, p3`）计算中间平滑曲线。每一段原始线段（`p1` 到 `p2`）之间均匀插入多个新点，使路线更密更平滑。

#### 关键代码

```c
void guandao_build_smooth_plan(guandao_state * state)
{
    int16 source_length = guandao_clamp_length(state->length_index);
    state->planned_length = 0;
    state->plan_ready = 0;

    if(source_length <= 0) return;
    if(source_length < 3)    // 点太少时无法插值，直接复制原始点
    {
        for(int i = 0; i < source_length; i++)
            state->planned_map[i] = state->recode_map[i];
        state->planned_length = source_length;
        state->plan_ready = 1;
        return;
    }

    // 计算每段原始线段插入的样本数 (1~6 之间)
    int samples = (MAX_LENGTH_INDEX - 1) / (source_length - 1);
    if(samples < 1) samples = 1;
    if(samples > 6) samples = 6;

    for(int i = 0; i < source_length - 1 && state->planned_length < MAX_LENGTH_INDEX - 1; i++)
    {
        // 取相邻 4 个点用于 Catmull-Rom
        state_t p0 = state->recode_map[(i > 0) ? i - 1 : i];
        state_t p1 = state->recode_map[i];
        state_t p2 = state->recode_map[i + 1];
        state_t p3 = state->recode_map[(i + 2 < source_length) ? i + 2 : i + 1];

        for(int j = 0; j < samples && state->planned_length < MAX_LENGTH_INDEX - 1; j++)
        {
            float t = (float)j / (float)samples;
            float t2 = t * t;
            float t3 = t2 * t;

            state_t out;
            // Catmull-Rom 公式 (x, y 分量)
            out.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t +
                     (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                     (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
            out.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t +
                     (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                     (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
            // 角度线性插值
            out.theta = p1.theta + (p2.theta - p1.theta) * t;

            state->planned_map[state->planned_length] = out;
            state->planned_length++;
        }
    }

    // 终点必须精确复制原始最后一个点
    state->planned_map[state->planned_length] = state->recode_map[source_length - 1];
    state->planned_length++;
    state->plan_ready = 1;
}
```

#### 参数说明

| 参数 | 含义 |
|------|------|
| `samples` | 每段原始线段之间插入的新点数。根据原始点数自适应计算，范围 1~6。 |
| `t` | 插值参数，`0` 对应 `p1`，`1` 对应 `p2`。 |
| `p0/p3` | 边界处理：首尾段外延到第一个点 / 最后一个点，确保曲线自然。 |
| `out.theta` | 角度线性插值，保证平滑过渡。 |

### 3.3 初始化相关修改

#### `guandao_state_init()`

新增字段初始化：

```c
e->length_index = 0;
e->planned_length = 0;
e->plan_ready = 0;
```

#### `portion_1_reset()`

重置时也清空规划状态：

```c
INS.current_point_index = 0;
INS.planned_length = 0;
INS.plan_ready = 0;
```

### 3.4 `portion_1()` 入口修改

在科目一自动驾驶启动阶段（首次进入 `portion_1_state_flag == 0` 时），先调用平滑规划生成：

```c
if(portion1_state_flag == 0)
{
    // ... 倒车点处理逻辑保留不变 ...

    guandao_build_smooth_plan(&INS);
    INS.current_point_index = 0;
    if(INS.planned_length > 1)
    {
        int end_index = INS.planned_length - 1;
        if(end_index > GUANDAO_START_SEARCH_POINTS) end_index = GUANDAO_START_SEARCH_POINTS;
        INS.current_point_index = guandao_find_closest_index(&INS, 1, end_index);
    }
    portion1_state_flag = 1;
}
```

- 倒车点判断逻辑保持原样。
- 在判定完倒车点后，先生成平滑规划路线。
- 然后在规划路线中查找最近点作为起点。

### 3.5 `guandao_find_closest_index()` 修改

修改后统一通过 `guandao_route_point()` 访问路线点，支持平滑路线：

```c
static int guandao_find_closest_index(guandao_state *state, int start_index, int end_index)
{
    int best_index = start_index;
    float best_distance = 0.0f;
    int16 route_length = guandao_route_length(state);
    // ... 越界保护 ...
    best_distance = get_distance(state->current_state, guandao_route_point(state, start_index));
    for(int i = start_index + 1; i <= end_index; i++)
    {
        float distance = get_distance(state->current_state, guandao_route_point(state, i));
        if(distance + 0.05f < best_distance)
        {
            best_distance = distance;
            best_index = i;
        }
    }
    return best_index;
}
```

### 3.6 `pursuit_contral_mode()` 修改

纯追踪主函数中所有访问路线的地方已改为统一接口：

| 修改位置 | 原代码 | 修改后 |
|----------|--------|--------|
| 路线长度 | `INS.length_index` | `guandao_route_length(state)` |
| 当前目标点 | `state->recode_map[state->current_point_index]` | `guandao_route_point(state, state->current_point_index)` |
| 搜索终点 | `state->current_point_index + GUANDAO_TRACE_SEARCH_POINTS`（上限 `state->length_index - 1`） | 上限改为 `route_length - 1` |
| 到达终点判断 | `state->current_point_index >= state->length_index` | `state->current_point_index >= route_length` |
| 到终点距离 | `get_distance(state->current_state, state->recode_map[state->length_index - 1])` | `get_distance(state->current_state, guandao_route_point(state, route_length - 1))` |
| 末段减速阈值 | `state->current_point_index >= state->length_index - 30` | `state->current_point_index >= route_length - 30` |

### 3.7 `pursuit_midhandle()` 修改

中间处理函数同样改为统一路线访问：

| 修改位置 | 原代码 | 修改后 |
|----------|--------|--------|
| 路线长度 | `state->length_index` | `guandao_route_length(state)` |
| 预瞄点索引 | `preview_spets + state->current_point_index`（上限 `state->length_index - 1`） | 上限改为 `route_length - 1` |
| 预瞄点获取 | `state->recode_map[preview_index]` | `guandao_route_point(state, preview_index)` |
| 到终点距离 | `get_distance(state->current_state, state->recode_map[state->length_index - 1])` | `get_distance(state->current_state, guandao_route_point(state, route_length - 1))` |
| 末段停车判断 | `state->current_point_index >= state->length_index - 30` | `state->current_point_index >= route_length - 30` |

### 3.8 `azimuth_adjust()` 修改

航向修正函数改为统一路线访问：

| 修改位置 | 原代码 | 修改后 |
|----------|--------|--------|
| 路线长度 | `state->length_index` | `guandao_route_length(state)` |
| 预瞄点索引 | `preview_spets + state->current_point_index`（上限 `state->length_index - 1`） | 上限改为 `route_length - 1` |
| 预瞄点获取 | `state->recode_map[preview_index]` | `guandao_route_point(state, preview_index)` |

### 3.9 未修改的保留逻辑

以下逻辑**保持原样**，不受影响：

- **倒车点判定**：`portion_1()` 中的 `daoche_point_length` 判定和 `daoche_point_flag` 处理不变。
- **终点停车**：末段减速和停车判断逻辑不变，只是改走规划路线计算终点距离。
- **Flash 存储**：`flash.c` 中只保存 `recode_map[]` 和 `length_index`，不会读写 `planned_map[]`。
- **遥控代码**：完全未碰 `RemteControl.c` / `RemteControl.h`。
- **PWM/速度输出接口**：`out_v_l`、`out_v_r`、`out_servo` 的赋值方式不变。

---

## 四、`user/cpu0_main.c` 串口调试增强详情

### 4.1 缓冲区扩容

```c
// 原代码
char line[220];

// 修改后
static char line[320];
```

- 原因：新 `AUTO` 行字段更多，220 字节可能溢出。
- `static` 修饰：避免在栈上分配大数组，改为静态存储区。

### 4.2 `AUTO` 行新增字段

原 `AUTO` 行格式：

```
AUTO,t=...,idx=...,len=...,D100=...,A10=...,reason=...,x100=...,y100=...,yaw10=...,vl10=...,vr10=...,servo10=...,tgt100=...,act100=...,pwm=...,enc10=...,enc100=...
```

新 `AUTO` 行格式：

```
AUTO,t=...,idx=...,rlen=...,plen=...,ready=...,D100=...,A10=...,fd100=...,reason=...,pth100=...,pv=...,x100=...,y100=...,yaw10=...,vl10=...,vr10=...,servo10=...,tgt100=...,act100=...,pwm=...,enc10=...,enc100=...
```

### 4.3 新增字段说明

| 字段 | 含义 | 单位/缩放 |
|------|------|-----------|
| `idx` | 当前追踪的路线点索引 | 整数，对应 `planned_map[]` 或 `recode_map[]` |
| `rlen` | 原始打点数量 | `INS.length_index` |
| `plen` | 平滑规划后的点数 | `INS.planned_length` |
| `ready` | 平滑路线是否已生成 | `1`=已启用规划路线，`0`=未启用 |
| `D100` | 到当前目标点距离 | ×100，例如 `D100=45` 表示 0.45m |
| `A10` | 车头与目标点方向角度差 | ×10，例如 `A10=123` 表示 12.3° |
| `fd100` | 到终点距离 | ×100 |
| `reason` | 追踪状态原因码 | `0`=正常，`1`=空/末尾，`2`=切点，`4`=到达终点，`5`=最近点推进 |
| `pth100` | 追踪切点阈值 | ×100 |
| `pv` | 预瞄步数 | `preview_spets` |
| `x100/y100` | 当前惯导坐标 | ×100 |
| `yaw10` | 当前 IMU 航向角 | ×10 |
| `vl10/vr10` | 惯导输出的左右轮目标速度 | ×10 |
| `servo10` | 转向输出 | ×10 |
| `tgt100` | 后轮目标速度 | ×100 |
| `act100` | 后轮实际速度 | ×100 |
| `pwm` | 后轮 PWM | 整数 |
| `enc10/enc100` | 编码器统计 | 10ms/100ms |

### 4.4 串口使用示例

打开串口助手（115200 波特率），运行自动驾驶后你会看到：

```
AUTO,t=123456,idx=5,rlen=50,plen=248,ready=1,D100=45,A10=12,fd100=5230,reason=0,pth100=30,pv=5,...
```

**关键判断指标**：

- 如果 `ready=1` 且 `plen > rlen`，说明平滑规划已生效。
- 如果 `ready=0`，说明可能打点太少（<3）或路线生成失败。
- `fd100` 逐渐减小到接近 0 时，车辆应该开始减速并停车。

---

## 五、验证与静态检查

### 5.1 静态检查项

| 检查项 | 结果 |
|--------|------|
| `guandao.h` 中新增字段声明 | ✅ `planned_map[]`、`planned_length`、`plan_ready`、`guandao_build_smooth_plan()` |
| `guandao.c` 中新增 `guandao_route_length()` | ✅ 已添加，返回规划路线或原始路线长度 |
| `guandao.c` 中新增 `guandao_route_point()` | ✅ 已添加，统一访问路线点 |
| `guandao.c` 中 `guandao_build_smooth_plan()` 实现 | ✅ Catmull-Rom 样条插值，边界保护齐全 |
| 初始化函数清零新字段 | ✅ `guandao_state_init()` 和 `portion_1_reset()` |
| `portion_1()` 调用平滑规划 | ✅ 在首次进入时调用 `guandao_build_smooth_plan()` |
| 所有 `recode_map[...]` 追踪访问已替换 | ✅ 通过 `guandao_route_point()` 统一访问 |
| 所有 `length_index` 长度检查已替换 | ✅ 通过 `guandao_route_length()` 统一获取 |
| `cpu0_main.c` 缓冲区改为 `static char line[320]` | ✅ 避免栈溢出 |
| `cpu0_main.c` AUTO 行加入新字段 | ✅ `plen`、`ready`、`fd100`、`pth100`、`pv` 等 |

### 5.2 尚未完成的验证

| 检查项 | 状态 | 说明 |
|--------|------|------|
| ADS 编译通过 | ⏳ 待确认 | 当前环境没有 ADS 编译命令行入口，需用户在 ADS IDE 中 Build |
| 实车测试 | ⏳ 待确认 | 需下载到 TC264 开发板验证 |

---

## 六、调试指南

### 6.1 如何确认平滑规划已生效

1. 推车打点完成后，切换到自动驾驶模式。
2. 打开串口助手，观察 `AUTO` 行输出。
3. 检查 `ready` 字段：
   - `ready=1` → 平滑规划已生成，正在使用 `planned_map[]`。
   - `ready=0` → 平滑规划未生成，仍在使用原始 `recode_map[]`。

### 6.2 `ready=0` 的常见原因

- 原始点数 < 3，无法做 Catmull-Rom 插值，函数直接复制原始点并设置 `ready=1`。但如果 `source_length <= 0`，则直接返回。
- `MAX_LENGTH_INDEX` 已满，导致 `planned_length` 达到上限。

### 6.3 如何判断效果

| 指标 | 改善前（原始路线） | 改善后（平滑路线） |
|------|-------------------|-------------------|
| `A10`（角度差）波动 | 频繁大幅跳变 | 变化更连续、更平缓 |
| `servo10`（转向输出） | 频繁左右摆动 | 变化更连续，很少剧烈跳变 |
| `idx` 推进节奏 | 每点距离远，跳跃大 | 点更密集（`plen` 更大），推进更平滑 |
| 行驶感受 | 左右打死、不顺畅 | 弧线过渡、更柔和 |

### 6.4 若发现异常

- 如果 `plen` 明显小于 `rlen`（例如原始 50 点，规划后只有 60 点），说明 `samples` 可能只有 1。可检查 `MAX_LENGTH_INDEX` 是否足够大。
- 如果 `ready=1` 但车辆仍然剧烈摆动，可能是预瞄参数 `preview_spets` 或切点阈值 `persuit_threshold` 需要微调。
- 如果终点停车不准（`fd100` 未降到 0 就停，或过了终点才停），检查 `route_length - 30` 的末段判断是否在规划路线上正确计算。

---

## 七、附录：Catmull-Rom 公式推导（简要）

对于一维值，给定相邻 4 个点 `p0, p1, p2, p3`，在 `p1` 到 `p2` 之间的插值公式为：

```
out = 0.5 * ( (2*p1)
            + (-p0 + p2) * t
            + (2*p0 - 5*p1 + 4*p2 - p3) * t^2
            + (-p0 + 3*p1 - 3*p2 + p3) * t^3 )
```

其中 `t ∈ [0, 1]`。分别对 `x` 和 `y` 分量计算，即可得到平滑曲线上的一点。

---

## 八、后续建议

1. **ADS 编译**：在 ADS 中 Build 工程，确认编译通过。
2. **实车测试**：先在一个简单路线（例如直线+一个弯道）上测试，观察串口输出中 `A10` 和 `servo10` 的波动是否减小。
3. **参数调优**：如果仍然不够平滑，可以调整：
   - `samples` 的最大值（当前限制为 6，可以增大以生成更密的路径）。
   - `preview_spets`（预瞄步数，增大后看更远，减少抖动）。
4. **GPS 校验**：若 `gnss_flag` 启用，确保平滑路线不破坏 GPS 远距离校验逻辑。
