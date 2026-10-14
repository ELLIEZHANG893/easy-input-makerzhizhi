# T13 · Host Action v1 纯逻辑验证与测试缺口补齐

> 依据 T02 冻结合同与 T12 复核，在已有等价实现的基线上先验证、再只补测试缺口。本节点只涉及纯逻辑层宿主测试与 `flow/`，不改生产代码，不接 USB／BLE，不构建固件，不烧录。

- **背景**：当前基线已实现 Host Action v1（见 T03—T12）。合同要求的纯逻辑行为均有实现，但部分拒绝路径缺少直接测试保护。
- **目标**：确认以下纯逻辑行为成立并有测试约束——合法小写标准 UUID 被接受；大写、长度、连字符或字符错误被直接拒绝且不自动转小写；配置层保留完整 `host_action:<UUID>`；按下生成一次 Host Action、松开不生成；编码为 `0x11`／`0x05`／`0`／`1`／`36`，线上不带前缀且不占用 `0x04`。
- **允许范围**：只读实现；只修改 `host_test/` 中与上述行为直接相关的宿主测试；记录到 `flow/`。
- **禁止越过的边界**：不重写或重构 `components/keyboard/src/` 的已有实现；不修改 `main/`、USB／BLE 适配、构建配置、分区或硬件相关代码；不运行 ESP-IDF 构建；不访问设备、不烧录。
- **产出路径**：本任务卡；`host_test/host_action_protocol_tests.cpp`；`host_test/config_payload_tests.cpp`；`host_test/keymap_tests.cpp`；`flow/plan.md`；`flow/进展.md`。
- **状态**：完成。

## 操作前状态

- 分支与提交：`practice/led-brightness`，`966ff58`（T12 记录提交）；写入前 `git status --short` 为空。
- 合同来源：`flow/tasks/T02-Host-Action-v1-固定兼容协议.md`（冻结合同）与 `flow/tasks/T12-Host-Action-v1-固定合同复核.md`（与当前代码逐项对照，无冲突）。

## 先验证：现有实现与测试覆盖

修改前运行 6 个定向测试（`config_payload_tests`、`config_state_tests`、`host_action_capability_status_tests`、`host_action_key_bindings_tests`、`host_action_protocol_tests`、`keymap_tests`）：**6/6 通过**。

| 要求 | 实现位置 | 修改前已有测试 | 缺口 |
|---|---|---|---|
| 合法小写标准 UUID 被接受 | `host_action_protocol.cpp` 的 `is_canonical_host_action_value()` | `host_action_protocol_tests` 的 `accepts_canonical_lowercase_uuid_without_semantic_uuid_restrictions`；`config_payload_tests` 的 `parses_canonical_host_actions_and_preserves_the_full_value` | 无 |
| 大写被拒绝且不转小写 | 同上（仅接受 `0-9a-f`，无大小写转换） | 协议层与配置层各有一个大写用例 | 前缀大写 `HOST_ACTION:` 未测 |
| 长度错误被拒绝 | 同上（总长必须为 48） | 只测了“少一位” | “多一位”、仅前缀（零长度 UUID）未测 |
| 连字符错误被拒绝 | 同上（第 9／14／19／24 位必须为 `-`） | 测了连字符错位 | 正确位置换成非连字符（如 `_`）未测 |
| 字符错误被拒绝 | 同上 | 测了 `g` | 无 |
| 配置层保留完整前缀 | `config_payload.cpp` 的 `parse_action()` | `config_payload_tests`、`host_action_key_bindings_tests`（KEY1—KEY8） | 无 |
| 按下生成一次、松开不生成 | `keymap.cpp` 的 `event_for_action()` | `keymap_tests` 的 `host_actions_emit_one_event_on_press_and_none_on_release`；`host_action_key_bindings_tests` | 不规范值进入 Keymap 时“按下也不生成”未测 |
| 编码 `0x11`／`0x05`／`0`／`1`／`36`、无前缀、`[40..62]` 为零、不占 `0x04` | `host_action_protocol.cpp` 的 `encode_host_action_v1()` | `host_action_protocol_tests` 的 `encodes_the_frozen_v1_report_without_the_config_prefix`（含 `static_assert` 区分 `0x04`） | 无 |

结论：实现满足全部要求，**不需要改生产代码**；只补拒绝路径的测试缺口。

## 补齐的测试（共 +34 行，仅 `host_test/`）

- `host_test/host_action_protocol_tests.cpp`：在 `rejects_noncanonical_uuid_and_leaves_output_untouched` 的拒绝列表中新增 4 例——多一位、仅前缀、正确位置换成 `_`、前缀大写 `HOST_ACTION:`；每例同时断言编码失败且输出缓冲区保持不变。
- `host_test/config_payload_tests.cpp`：在 `rejects_noncanonical_host_action_uuids_without_normalizing` 中新增 2 例——多一位、前缀大写；断言 `ConfigParseStatus::UnknownAction`。
- `host_test/keymap_tests.cpp`：新增 `noncanonical_host_action_values_never_emit_an_event`——大写、过长、错误分隔符三种值即使直接放进 `Action`，按下与松开都只得到 `FirmwareEventKind::None`；同时补 `#include <array>`。

## 测试证据

- **定向测试（补齐后）**：上述 6 个测试 **6/6 通过**。
- **全部宿主测试**：全新配置的构建目录，CTest 发现 62、执行 62、通过 62，0 失败，编译 0 警告。
- **测试有效性（变异检查，在仓库外临时副本中进行，未进入仓库）**：故意改坏两处实现——① 去掉 `event_for_action()` 的规范性检查；② 让 `is_canonical_host_action_value()` 接受过长输入并忽略前缀大小写。
  - 使用**修改前**的测试：`config_payload_tests` 与 `keymap_tests` 仍然通过（未能发现），只有 `host_action_protocol_tests` 因既有前缀用例失败。
  - 使用**补齐后**的测试：`config_payload_tests`、`host_action_protocol_tests`、`keymap_tests` 全部失败（成功发现）。
  - 临时副本已删除。

## 未验证项

- 未运行 ESP-IDF 5.5.5 构建：本节点只改宿主测试，测试文件不参与固件编译；如需固件构建证据，需在可访问乐鑫组件仓库的环境中运行。
- 未接 USB／BLE、未烧录、未在实板上观察 Host Action 发送；USB／BLE 发送路径沿用既有实现，本节点未改动也未重新验证。
- 未与 EasyInput App 联调；App 侧 UUID 到本机应用的映射不在固件范围内。

## 本节点不承担的范围

- 不修改任何生产代码（`components/keyboard/src/`、`components/keyboard/include/`、`main/`、`features/`）。
- 不改写 T02 冻结的 v1 合同，不新增 Report ID、kind、分片、释放消息或双发路径。
- 不增加 UUID version 或 nil 限制，不自动规范化非规范输入。
- 不修改 GPIO、BOOT、GPIO8 共享供电、分区、USB／BLE 设备身份或硬件生命周期。

## 验收自检

- [x] 先读取合同（T02、T12），合同存在且与固定字段无冲突。
- [x] 先运行定向测试验证已有实现，再判断缺口。
- [x] 只在纯逻辑层补最小宿主测试；生产代码零改动。
- [x] 定向测试与全部宿主测试通过；变异检查证明新增用例有效。
- [x] 未接 USB／BLE、未构建固件、未烧录；未验证项已列出。
