# Remote Control Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port the SBUS remote receiver from `D:\ADS\source\REMOTE CONTROL\E03_uart_receiver_demo` into the current car project without stealing UART1 from voice or UART3 from GPS.

**Architecture:** Add the SeekFree SBUS receiver driver as a device module, configured to UART2 RX=P10.6/TX=P10.5. Add a small `remote_control` adapter that converts SBUS CH1/CH2 into steering angle and rear motor speed, with dead zone and failsafe. The main loop gives this adapter priority only while valid SBUS frames are active.

**Tech Stack:** TC264 C project, SeekFree UART/SBUS driver, PowerShell static checks.

---

### Task 1: Static check for the port

**Files:**
- Create: `tests/check_remote_control_port.ps1`

- [ ] **Step 1: Write the failing test**

Create a PowerShell check that requires the SBUS driver files, UART2/P10.6 configuration, UART2 RX ISR dispatch, remote-control adapter API, and main-loop integration.

- [ ] **Step 2: Run the test to verify it fails**

Run: `powershell -NoProfile -ExecutionPolicy Bypass -File tests\check_remote_control_port.ps1`

Expected: FAIL because the receiver driver and adapter do not exist yet.

### Task 2: Add SBUS receiver driver

**Files:**
- Create: `libraries/zf_device/zf_device_uart_receiver.h`
- Create: `libraries/zf_device/zf_device_uart_receiver.c`
- Modify: `libraries/zf_common/zf_common_headfile.h`
- Modify: `libraries/zf_device/zf_device_type.h`
- Modify: `libraries/zf_device/zf_device_type.c`

- [ ] **Step 1: Copy and adapt the receiver driver**

Use the source driver's SBUS parser, but configure `UART_RECEVIER_UART_INDEX` as `UART_2`, `UART_RECEVIER_TX_PIN` as `UART2_TX_P10_5`, and `UART_RECEVIER_RX_PIN` as `UART2_RX_P10_6`.

- [ ] **Step 2: Add a receiver-specific callback hook**

Expose `uart_receiver_handler` and route `set_wireless_type(RECEIVER_UART, ...)` to that callback.

### Task 3: Add current-project control adapter

**Files:**
- Create: `code/remote_control.h`
- Create: `code/remote_control.c`
- Modify: `libraries/zf_common/zf_common_headfile.h`

- [ ] **Step 1: Implement `remote_control_init()`**

Initialize SBUS via `uart_receiver_init()` and reset adapter state.

- [ ] **Step 2: Implement `remote_control_task()`**

On valid SBUS frames, initialize center offsets, apply dead zone, map CH1 to steering and CH2 to speed, and update motor/steering periodically. On disconnect or timeout, stop the rear motor and center steering target.

### Task 4: Wire into interrupts and main loop

**Files:**
- Modify: `user/isr.c`
- Modify: `user/cpu0_main.c`

- [ ] **Step 1: Connect UART2 RX ISR**

Call `uart_receiver_handler()` from `uart2_rx_isr`.

- [ ] **Step 2: Initialize and run adapter**

Call `remote_control_init()` during startup. In the main loop, call `remote_control_task()` and skip normal mode processing while it returns active.

### Task 5: Verification

**Files:**
- Test: `tests/check_remote_control_port.ps1`

- [ ] **Step 1: Run remote-control port check**

Run: `powershell -NoProfile -ExecutionPolicy Bypass -File tests\check_remote_control_port.ps1`

Expected: PASS with `Remote control UART2/SBUS port checks passed.`

- [ ] **Step 2: Check UART conflict residues**

Run: `rg "UART_RECEVIER_UART_INDEX|uart_receiver_handler|remote_control_task|GNSS_UART|OFFLINE_VOICE_UART_INDEX" -n code user libraries`

Expected: receiver uses UART2, voice remains UART1, GPS remains UART3.
