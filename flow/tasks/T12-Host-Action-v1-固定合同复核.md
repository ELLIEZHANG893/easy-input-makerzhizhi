# T12 · Host Action v1 固定合同复核

> 本任务卡在已经实现 Host Action v1 的当前基线上，逐项复核同一份固定合同是否仍与代码事实一致。它不是新方案，也不改写 T02 冻结的 v1 合同；本节点只改 `flow/`。

- **背景**：T02 冻结合同时，基线尚未实现 Host Action。此后 T03—T11 已在 `main` 完成实现、测试、构建和公开。本次复核在本地练习分支上进行；该分支相对 `main@7d5f157` 只增加了灯光亮度与开机音量两项练习改动，均不触及 Host Action 的配置、事件、协议、USB／BLE 发送或状态声明文件。
- **目标**：确认固定字段与现有编号、消息容器、单通道选择和状态预算没有冲突，并记录预计涉及的语义层与不承担范围。
- **允许范围**：只读当前 Maker 仓库与既有 `flow/` 记录；只新增或修改根级 `flow/`。
- **禁止越过的边界**：不修改固件、宿主测试、构建配置或其他非 `flow/` 文件；不运行测试、构建或烧录；不访问设备；不读取其他电脑端工程；不自行设计或改写 v1 字段。
- **产出路径**：本任务卡；`flow/plan.md`；`flow/进展.md`。
- **状态**：复核完成，未发现冲突。

## 操作前文件状态

- 分支与提交：`practice/led-brightness`，`4033785c167eb15f24bbe8e79d81a8f213937c79`。
- 写入前 `git status --short` 为空：没有已跟踪或未跟踪的改动。
- 相对 `main@7d5f157` 的差异只涉及：`led_brightness.*`、`speaker_volume.*`、`led_strip_status.*`、`speaker_output.cpp`、两个 `CMakeLists.txt` 与两个对应宿主测试；没有 Host Action 相关文件。

## 逐项核对

| 固定合同条目 | 当前代码证据 | 结论 |
|---|---|---|
| 配置保存 `host_action:<canonical-lowercase-uuid>` | `components/keyboard/src/config_payload.cpp` 的 `parse_action()` 以 `kHostActionPrefix` 识别后，把完整字符串存入 `Action::host_action`；配置原文由 `main/platform/nvs_store.cpp` 的 `save_config()` 以整段 JSON 持久化 | 一致 |
| 非规范 UUID 直接拒绝，不自动转小写 | `components/keyboard/src/host_action_protocol.cpp` 的 `is_canonical_host_action_value()` 只接受总长 48、前缀精确匹配、第 9／14／19／24 位为 `-`、其余为 `0-9a-f`；没有任何大小写转换。`parse_action()` 校验失败返回 `UnknownAction` | 一致；未增加 UUID version 或 nil 限制 |
| Report ID `0x11` | `host_action_protocol.h` 的 `kHostActionV1ReportId = 0x11`；`usb_hid.cpp`、`ble_hid.cpp` 以 `static_assert` 与各自 `kReportIdAppCommand` 对齐 | 一致 |
| kind `0x05` | `kHostActionV1CommandKind = 0x05`；现有 kind 为 `0x01` 固定文字、`0x02` 热键、`0x03` 配置回执、`0x04` 状态响应（`status_hid_protocol.h` 的 `kStatusResponseCommandKind`） | 一致，无编号冲突 |
| chunk index `0`、total chunks `1`、data length `36` | `encode_host_action_v1()` 写入 `payload[1]=0`、`payload[2]=1`、`payload[3]=36` | 一致 |
| `payload[4..39]` 只放无前缀 UUID ASCII | `encode_host_action_v1()` 只复制 `action.substr(12)` 的 36 字节到 `payload[4..]` | 一致 |
| 余量保持现有归零行为 | 编码结构 `HostActionV1Report::payload{}` 值初始化为零；USB `queue_app_command_report()` 与 BLE `send_app_command_report()` 均以 `std::array<…, 63> report{}` 归零后只复制 `len` 字节 | 一致；`[40..62]` 为零 |
| 容器能承载 | 现有 App Command 容器 63 字节、头 4 字节、数据最大 59 字节；`static_assert(kHostActionV1HeaderLen + kHostActionV1UuidLen <= kHostActionV1PayloadLen)` | 4+36=40 ≤ 63，可承载 |
| `0x04` 继续用于状态响应 | `kStatusResponseCommandKind = 0x04` 未被 Host Action 复用 | 一致 |
| 按下发送一次、松开不发送 | `components/keyboard/src/keymap.cpp` 的 `event_for_action()`：`HostAction` 仅在 `Pressed` 且规范时返回 `FirmwareEventKind::HostAction`，`Released` 返回空事件 | 一致 |
| USB 与 BLE 同一内容 | `usb_hid.cpp` 与 `ble_hid.cpp` 的 `HostAction` 分支都调用同一个 `encode_host_action_v1()`，再把同一 kind／chunk／length／数据交给各自 `send_app_command_report()` | 一致 |
| 现有单通道选择、不双发 | `transport_routing.cpp` 的 `route_for_firmware_event()` 固定返回 `UsbFirst`；`app_main.cpp` 的 `dispatch_firmware_event()` 在 USB endpoint epoch 有效时只走 USB，失败记录 “BLE fallback suppressed” 后返回，USB 不可用才走 BLE | 一致；代码中 `UsbFirst` 的语义即“USB 在线只用 USB，否则 BLE，不迁移” |
| 固件不保存应用路径或名称 | 固件内只有 `host_action:` 字符串与 UUID；未发现应用路径、名称或 Bundle ID 字段 | 一致 |
| 完整实现后声明 `"host_action_v1":true` | `components/keyboard/include/keyboard/config_status.h` 与 `components/keyboard/src/config_status.cpp` 在完整、紧凑与 BLE 回退变体中输出 `"host_action_v1":true` | 一致（当前基线已实现，故已声明） |
| BLE 状态不超过 512 字节 | `config_status.h` 的 `kConfigStatusGattSafeLen = 512`；`host_test/host_action_capability_status_tests.cpp` 对各变体断言 `wire.size() <= 512` | 一致 |

**冲突门结论**：固定字段与现有编号、63 字节容器、单通道选择和 512 字节状态预算都没有直接冲突；当前基线已经完整落实该合同，因此按授权写入记录，不停止。

## 预计涉及的语义层（与当前实现的对应）

| 语义层 | 当前位置 |
|---|---|
| 配置解析与严格校验 | `config_payload.cpp` 的 `parse_action()`／`parse_binding()`，共享 `is_canonical_host_action_value()` |
| 配置持久化 | `config_state.cpp` 的 `apply_json()`；`nvs_store.cpp` 的 `save_config()` |
| Keymap 动作与固件事件 | `keymap.h` 的 `ActionKind::HostAction`、`FirmwareEventKind::HostAction`；`keymap.cpp` 的 `event_for_action()` |
| 共享协议编码 | `host_action_protocol.h/.cpp` 的 `encode_host_action_v1()` |
| 单通道选择 | `transport_routing.cpp` 的 `route_for_firmware_event()`；`app_main.cpp` 的 `dispatch_firmware_event()` |
| USB／BLE 发送适配 | `usb_hid.cpp`、`ble_hid.cpp` 的 `send_firmware_event*()` → `send_app_command_report()` |
| 能力声明与 BLE 状态预算 | `config_status.h/.cpp`；`host_action_capability_status_tests.cpp` |
| 宿主测试 | `host_action_protocol_tests.cpp`、`host_action_key_bindings_tests.cpp`、`host_action_capability_status_tests.cpp`、`transport_routing_tests.cpp` |

## 本节点不承担的范围

- 不修改、补充或重构任何 Host Action 代码；不新增 Report ID、kind、分片、释放消息或双发路径。
- 不运行宿主测试、ESP-IDF 构建或烧录；本次复核结论是静态代码对照，不替代测试、构建和实板证据（这些证据见 T09—T11）。
- 不读取、修改或设计其他电脑端工程；不保存或推断应用路径、名称、Bundle ID；不复制 App 本地的 UUID 映射。
- 不增加 UUID version 或 nil 限制，不自动规范化非规范输入。
- 不修改 GPIO、BOOT、GPIO8 共享供电、分区、USB／BLE 设备身份或硬件生命周期。
- 不改写 T02 已冻结的 v1 合同；未来不兼容变化必须使用新版本和新能力声明。

## 本节点验收自检

- [x] 写入前分支、提交和工作区状态已记录。
- [x] 每个固定字段都有可定位的代码证据。
- [x] 已确认无编号冲突、容器可承载、单通道不双发、BLE 状态预算受测试约束。
- [x] 已记录语义层与不承担范围。
- [x] 只改 `flow/`；未改代码、未测试、未构建、未烧录。
