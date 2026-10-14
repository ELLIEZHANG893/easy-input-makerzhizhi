# T15 · Host Action v1 接入现有 App Command 发送链路核验

> 依据 T02 冻结合同与 T12—T14 记录，确认 Host Action 已经最小接入工程现有的 App Command 发送链路，并补齐“共享编码、事件分发、路由”三处可测证据。本节点只改宿主测试与 `flow/`；不改生产代码、能力声明、HID 描述符／GATT、设备身份、硬件或分区，不烧录。

- **背景**：T06 已把 Host Action 接入 `app_main.cpp`、`usb_hid.cpp`、`ble_hid.cpp`。已有源码合同测试检查了“各自调用一次共享编码”“USB 失败不迁移 BLE”等要点，但没有约束两端参数顺序是否一致、数据是否来自编码结果、实体按键是否真的进入分发器、Host Action 是否误入按住型键盘通道。
- **目标**：接入已存在就先验证，只补最小测试；队列、路由、函数和文件名以当前代码为准。
- **允许范围**：只读实现；只修改 `host_test/firmware_source_contract_tests.cpp`、`host_test/host_action_protocol_tests.cpp`、`host_test/transport_routing_tests.cpp`；记录到 `flow/`。
- **产出路径**：本任务卡；上述 3 个宿主测试；`flow/plan.md`；`flow/进展.md`；`flow/踩坑记录.md`。
- **状态**：完成（核验 + 补测试；生产代码零改动）。

## 操作前状态

- 分支与提交：`practice/led-brightness`，`7de660f`（T14）；写入前 `git status --short` 为空。
- 合同：`0x11`／`0x05`／chunk `0`／total `1`／length `36`；`payload[4..39]` 为无前缀 UUID，`[40..62]` 为零；`0x04` 仅用于状态响应（T02、T12）。

## 调用链（当前代码）

| 步 | 位置 | 行为 |
|---|---|---|
| 1 | `main/app_main.cpp` `handle_input_event()` | 取 `keymap().action_for(event.input)` → `event_for_action()` → `dispatch_firmware_event(app, event.input, firmware_event)` |
| 2 | `components/keyboard/src/keymap.cpp` `event_for_action()` | 仅在按下且值规范时产出 `FirmwareEventKind::HostAction`，松开为空事件 |
| 3 | `main/app_main.cpp` `dispatch_firmware_event()` | `HidKeyDown／HidKeyUp` 走按住型键盘通道（`KeyboardTransportLatch`）；Host Action 不进该分支，改调 `route_for_firmware_event()` |
| 4 | `components/keyboard/src/transport_routing.cpp` `route_for_firmware_event()` | 与固定文字、App 命令相同，固定 `UsbFirst` |
| 5 | `dispatch_firmware_event()` 单通道选择 | USB 端点 epoch 有效 → 只调用 `usb.send_firmware_event_for_epoch()`，失败记录 “BLE fallback suppressed” 后返回；USB 不可用 → 只调用 `ble.send_firmware_event()` |
| 6 | `usb_hid.cpp` `send_firmware_event_for_epoch()`／`ble_hid.cpp` `send_firmware_event_for_owner()` 的 `HostAction` 分支 | 调用同一个 `encode_host_action_v1()`，检查 Report ID 与 `kReportIdAppCommand` 一致，再把 `payload[0]`、`[1]`、`[2]`、`payload.data()+4`、`payload[3]` 交给 `send_app_command_report()` |
| 7 | USB `send_app_command_report()` → `queue_app_command_report()` → `push_app_command_report_locked()`；BLE `send_app_command_report()` | 都用归零的 63 字节 `report{}`，按 kind／chunk／total／len 写头，再复制 `len` 字节；USB 进入现有报告队列，BLE 走现有 App Command 输入特征 |

结论：接入已经完整存在，符合“复用同一编码、单通道、不双发、不占 `0x04`”。不需要新增队列、路由或函数。

## 修改前证据缺口

| 关注点 | 修改前已有 | 缺口 |
|---|---|---|
| 共享编码 | 每端恰好 1 次 `encode_host_action_v1`；出现 `payload[0..3]`；不出现 `0x05`、`36` | 未约束参数**顺序**；未约束数据指针来自编码结果；未证明头／数据拆分能还原同一容器 |
| USB／BLE 同内容 | 两端各自满足上面的条件 | 未直接比较两端 `HostAction` 分支 |
| 事件分发 | USB 失败后 `return`、BLE 在其后 | 未约束实体按键进入分发器；未约束 Host Action 不进按住型键盘通道；未约束先选路再发送；USB 失败块内再加一次 BLE 调用不会被发现 |
| 路由 | Host Action 在 BLE 连接／未连接时均为 `UsbFirst` | 未约束与固定文字、App 命令同路由，未约束与动作值无关 |
| 不占 `0x04` | 编码层 `static_assert`；适配层必须引用 `payload[0]`（变异 M7 已能发现） | 无实质缺口；补一条显式断言 |

## 实际修改（3 个宿主测试，+173 行）

1. `host_test/host_action_protocol_tests.cpp` 新增 `adapter_argument_split_rebuilds_the_identical_container`：用适配层同样的参数拆分（`payload[0]`、`[1]`、`[2]`、`payload.data()+4`、`payload[3]`）和同样的归零 63 字节构造方式重建容器，断言与编码结果逐字节相等，且 kind 为 `0x05`、不等于状态 kind `0x04`。这是**模型测试**，证明拆分方式本身无损；它不链接真实适配层代码。
2. `host_test/transport_routing_tests.cpp` 新增 `host_actions_share_the_existing_single_channel_route`：BLE 连接与否两种情况下，Host Action 的路由与固定文字、App 命令完全相同，且对不同值（含空值）不变。
3. `host_test/firmware_source_contract_tests.cpp` 新增 `host_action_reaches_app_command_chain_with_identical_usb_ble_content`（并补 `#include <array>`）：
   - 实体按键路径：`action_for` → `event_for_action` → `dispatch_firmware_event` 顺序存在；
   - 分发器：按住型键盘分支不提及 Host Action；`route_for_firmware_event` 在任何发送调用之前；分发器内 USB 发送调用恰好 1 处、BLE 发送调用恰好 2 处（`BleFirst` 分支与 USB 不可用分支，互斥）；
   - USB 与 BLE 的 `HostAction` 分支去空白后，除最后一个归属参数（`expected_epoch`／`expected_owner`）外**完全一致**；
   - 参数顺序固定为 kind、chunk、total、`payload.data()+kHostActionV1HeaderLen`、length；不出现 `event.value.data()`、`kStatusResponseCommandKind`、`0x04`；
   - 两端 App Command 构造器的头字段写入顺序一致。

生产代码、`main/`、`components/`、能力声明、HID 描述符、GATT、设备身份、硬件配置、分区零改动。

## 测试证据

- **定向测试**：`config_payload_tests`、`firmware_source_contract_tests`、`host_action_capability_status_tests`、`host_action_key_bindings_tests`、`host_action_protocol_tests`、`keymap_tests`、`transport_routing_tests` 共 7/7 通过。
- **全部宿主测试**：全新构建目录，CTest 发现 62、执行 62、通过 62，0 失败，编译 0 警告。
- **变异检查**（仓库外临时副本，已删除）：

| 编号 | 故意引入的错误 | 修改前测试 | 补齐后测试 |
|---|---|---|---|
| M1 | BLE 端交换 chunk 与 total 参数 | 未发现 | 发现 |
| M2 | USB 端改发配置字符串字节而非编码结果 | 未发现 | 发现 |
| M3 | USB 失败块内追加一次 BLE 发送（双发） | 未发现 | 发现 |
| M4 | 路由在 BLE 连接时给 Host Action 返回 `BleFirst` | 发现 | 发现 |
| M5 | Host Action 被并入按住型键盘通道 | 未发现 | 发现 |
| M6 | BLE 构造器头字段写入错位 | 未发现 | 发现 |
| M7 | USB 端 kind 改用状态 kind `0x04` | 发现 | 发现 |
| M8 | 实体按键路径不再调用分发器 | 未发现 | 发现 |

  修改前 2/8，补齐后 8/8。`adapter_argument_split_rebuilds_the_identical_container` 是模型测试，不在上述变异中单独起作用；真实适配层的约束来自源码合同测试。
- **示例 UUID 范围**：只出现在 `host_test/`；`components/`、`main/`、`features/`、`diagnostics/` 中 0 次。

## 未验证项

- 未运行 ESP-IDF 5.5.5 构建：本节点只改宿主测试，不参与固件编译；当前云端环境无法访问乐鑫组件仓库，固件构建证据仍以 T06 记录为准。
- 源码合同测试是文本级约束，不等于运行时证据；未在实板上抓取 USB 报告或 BLE 通知，未验证线上字节。
- 未烧录，未与 EasyInput App 联调；未验证 App 收到后能否打开对应应用。
- 未测试 USB 队列满、端点重挂载、BLE 断连等运行时竞态下的 Host Action 行为（已有通用队列与 epoch 测试覆盖，但未针对 Host Action 单独验证）。
- 编码器旋转与按压绑定 Host Action 不在本节点范围。

## 本节点不承担的范围

- 不新增队列、路由、函数或文件；不改 `dispatch_firmware_event()`、适配层或 App Command 构造器。
- 不增加能力声明；不修改 HID 描述符、GATT、设备身份、GPIO、BOOT、GPIO8 共享供电或分区。
- 不新增 Report ID、kind、分片、释放消息或双发；不依赖 App 去重。

## 验收自检

- [x] 先读 T02／T06／T12—T14 与当前代码，确认接入已存在。
- [x] 调用链逐步有代码位置。
- [x] 共享编码、事件分发、路由三处各有新增可测证据。
- [x] 定向 7/7、完整宿主 62/62 通过；变异检查 2/8 → 8/8 如实记录。
- [x] 只改 3 个宿主测试；生产与硬件相关文件零差异；未构建固件、未烧录。
