# T14 · Host Action v1 八键共用解析与旧动作回归

> 在已实现 Host Action v1 的基线上，确认 KEY1—KEY8 共用同一配置解析，并用宿主测试证明八键 Host Action 与复制、粘贴、快捷键、固定文字共存且互不干扰。本节点只改宿主测试与 `flow/`；不改生产代码、USB／BLE、能力声明、硬件配置、分区或设备身份，不构建固件，不烧录。

- **背景**：T04 已为 KEY1—KEY8 增加 Host Action 参数化测试；旧动作的回归测试分散在 `config_payload_tests`、`keymap_tests` 中，且大多只针对固定按键（如复制在 KEY6、粘贴在 KEY7）。
- **目标**：已有能力用测试证明，缺失时只做最小补齐，不重写架构。
- **允许范围**：只读实现；只修改 `host_test/host_action_key_bindings_tests.cpp`；记录到 `flow/`。
- **产出路径**：本任务卡；`host_test/host_action_key_bindings_tests.cpp`；`flow/plan.md`；`flow/进展.md`。
- **状态**：完成。

## 操作前状态

- 分支与提交：`practice/led-brightness`，`d56974d`（T13）；写入前 `git status --short` 为空。
- 基线定向测试（`config_payload_tests`、`host_action_key_bindings_tests`、`keymap_tests`）：3/3 通过。

## 共用解析证据（实现未改动）

- `components/keyboard/src/config_payload.cpp` 的 `parse_config_payload()` 用同一个 `key_inputs` 数组（`KEY1`→`InputId::Key1` … `KEY8`→`InputId::Key8`）循环，对每个键调用同一个 `parse_binding()` → `parse_action()`；没有按键专属分支。
- Host Action 识别、规范 UUID 校验、复制／粘贴命名动作、`{"hotkey":…}`、`{"text":…}` 都在同一个 `parse_action()` 中处理，八键完全共用。
- 结论：能力已存在，不需要补实现。

## 修改前已有覆盖

| 能力 | 已有测试 |
|---|---|
| KEY1—KEY8 每键保留完整 `host_action:<UUID>`、按下一次、松开不重复 | `host_action_key_bindings_tests` 的 `every_main_key_preserves_and_emits_one_host_action_per_press_cycle` |
| 复制／粘贴／快捷键解析 | `config_payload_tests`（固定按键：复制 KEY6、粘贴 KEY7、快捷键 KEY2／KEY4／KEY8 等） |
| 固定文字解析 | `config_payload_tests`（KEY1） |
| 单个动作的按下／松开事件语义 | `keymap_tests`（与具体按键无关） |
| 默认 Keymap 内容 | `keymap_tests` 逐键核对默认动作 |

缺口：没有一项测试让**每个**主按键都经过“配置解析 → Keymap → 事件”完整链路去验证旧动作，也没有在同一份配置中混合 Host Action 与旧动作的测试；默认 Keymap 不含 Host Action 只是间接由逐键默认值断言保证。

## 实际修改（仅 1 个测试文件，+159 行）

`host_test/host_action_key_bindings_tests.cpp` 新增：

1. **`every_main_key_mixes_host_action_with_legacy_actions`**：五种按下动作（Host Action、复制 `"copy"`、粘贴 `"paste"`、快捷键 `{"hotkey":"Ctrl+Tab"}`、固定文字 `{"text":"hello"}`）在 KEY1—KEY8 上轮转 5 轮，每一轮八个键各分配一种动作并写进**同一份配置**。共 40 个“按键 × 动作”组合，五种动作在每个键上都出现过。每个组合都断言：
   - 解析：动作类型正确；Host Action 保留完整 `host_action:<UUID>`；旧动作的 `host_action` 字段为空；快捷键与固定文字原值保留。
   - 事件：Host Action 按下得到一次 `HostAction`、松开为 `None`；复制按下／松开为 `HidKeyDown`／`HidKeyUp` 且值为 `Meta+C`；粘贴为 `Meta+V`；快捷键为原值的按下／松开；固定文字按下一次 `FixedText`、松开为 `None`；旧动作的事件永远不会是 `HostAction`。
2. **`default_keymap_contains_no_host_action`**：遍历 `DefaultKeymap()` 的全部输入（含编码器），断言没有 `ActionKind::HostAction`，`host_action` 字段全为空。

生产代码、`main/`、USB／BLE、能力声明、硬件配置、分区、设备身份零改动。

## 八键证据

- 原有：KEY1—KEY8 逐键单独绑定 Host Action，每键解析保留完整前缀，按下周期内恰好产生 1 次 `HostAction`，其余七键为 `Disabled`。
- 新增：KEY1—KEY8 每个键在 5 轮中依次承担五种动作；每一轮都在同一份配置里同时含有 Host Action 与旧动作，全部按预期解析和产生事件。

## 旧动作回归

- 复制、粘贴、快捷键、固定文字在 KEY1—KEY8 上全部通过“解析 → Keymap → 事件”断言（macOS 默认平台）。
- 原有 `config_payload_tests` 与 `keymap_tests` 继续通过，未被修改。

## 测试证据

- **定向测试**：`config_payload_tests`、`config_state_tests`、`host_action_capability_status_tests`、`host_action_key_bindings_tests`、`host_action_protocol_tests`、`keymap_tests` 共 6/6 通过。
- **全部宿主测试**：全新构建目录，CTest 发现 62、执行 62、通过 62，0 失败，编译 0 警告。
- **变异检查**（仓库外临时副本，已删除）：
  - 固定文字松开时重复发送、粘贴误发复制组合键、默认 Keymap 混入 Host Action 三种错误，新测试**都能发现**。
  - 原有 `keymap_tests` 对这三种错误**也都能发现**；因此本次新增测试不是修补漏洞，而是把“每个主按键 × 每种动作、同一配置内共存”的证据写成直接断言，并为默认 Keymap 增加一条与具体默认值无关的防护。
- **示例 UUID 范围**：`123e4567-e89b-12d3-a456-426614174000` 只出现在 `host_test/` 的 6 个文件；`components/`、`main/`、`features/`、`diagnostics/` 中出现 0 次。

## 未验证项

- 未测试 Windows 平台下复制／粘贴的 `Ctrl+C`／`Ctrl+V` 映射（本次轮转使用默认 macOS；已有 `config_payload_tests` 覆盖平台修饰键）。
- 未运行 ESP-IDF 5.5.5 构建（只改宿主测试，测试文件不参与固件编译）。
- 未接 USB／BLE、未烧录、未在实板上逐键按压验证；未与 EasyInput App 联调。
- 编码器左右旋转与按压的 Host Action 绑定不在本节点范围（合同只针对八个实体主按键）。

## 本节点不承担的范围

- 不重写配置解析、Keymap 或事件架构；不修改生产代码。
- 不修改 USB／BLE 发送、能力声明、GPIO、BOOT、GPIO8 共享供电、分区或设备身份。
- 不把测试示例 UUID 写入默认 Keymap、生产配置或任何非测试文件。

## 验收自检

- [x] 确认八键共用 `parse_binding()`／`parse_action()`。
- [x] KEY1—KEY8 Host Action 与四种旧动作均有逐键测试证据。
- [x] 只改 1 个宿主测试文件；生产与硬件相关文件零差异。
- [x] 定向 6/6、完整宿主 62/62 通过；变异检查结论如实记录。
- [x] 示例 UUID 仅在宿主测试；默认 Keymap 不含 Host Action。
- [x] 未构建固件、未接 USB／BLE、未烧录；未验证项已列出。
