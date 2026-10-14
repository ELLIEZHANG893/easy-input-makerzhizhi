# T16 · Host Action v1 能力声明与状态字节预算核验

> 枚举当前工程真正会发布的配置状态与 BLE 状态变体，确认 `"host_action_v1":true` 已进入所有需要的能力声明路径，并逐变体测量最终 UTF-8 字节数、证明不超过 512。本节点只改宿主测试与 `flow/`；不改生产代码、协议、HID／GATT、设备身份、硬件或分区，不构建、不烧录。

- **背景**：T07 已把能力字段加入状态构造器；已有 `host_action_capability_status_tests` 只测 6 个变体，且固件字符串只有 6 个字符，没有覆盖诊断、普通阶段回退链、带扬声器启动摘要的电量视图和真实固件版本字符串。
- **目标**：已有等价实现就先验证再补缺口；超限时才按仓库既有字段语义最小收缩。
- **允许范围**：只读实现；修改 `host_test/host_action_capability_status_tests.cpp` 与 `host_test/CMakeLists.txt`（仅给该测试加仓库根路径定义）；记录到 `flow/`。
- **产出路径**：本任务卡；上述 2 个文件；`flow/plan.md`；`flow/进展.md`。
- **状态**：完成（已存在等价实现；未超限，未做任何收缩；生产代码零改动）。

## 操作前状态

- 分支与提交：`practice/led-brightness`，`cad7021`（T15）；写入前 `git status --short` 为空。

## 能力字段的发出点（实现未改动）

| 发出点 | 位置 | 覆盖的视图 |
|---|---|---|
| `append_sync_core()` | `components/keyboard/src/config_status.cpp` | 全部紧凑视图：普通阶段回退、诊断、电量、配置确认 |
| `build_full_status_json()` | 同上 | 普通阶段完整视图 |
| `build_speaker_probe_status_json()` | 同上 | 扬声器诊断固件的 `spk_probe` 视图 |
| `kConfigStatusFallbackJson` | `components/keyboard/include/keyboard/config_status.h` | BLE 超限时的固定兜底 |

四处都写布尔 `"host_action_v1":true`，且位于 `capabilities` 对象内。状态响应 kind `0x04` 与 Host Action kind `0x05` 分工不变（测试中以 `static_assert` 固定）。

## 实际发布路径

- `main/app_main.cpp` `publish_config_status()` 按阶段给构造器不同输入：
  - 普通阶段（`boot`、`initializing` 等）：带实时音频，不带电源和诊断；
  - `push`／`platform` 以及 USB 状态请求：强制配置确认视图；
  - `diag`：电源 + 板级诊断，不带音频；
  - `battery`：电源 + 电量明细（有采样时）+ 产品固件的扬声器启动摘要；
  - `spk_probe`：仅扬声器诊断固件。
  - 构造结果超过 512 时改用确认视图，仍超就不发布。
- BLE：`publish_status_json()` 超过 512 时换成固定兜底；电量视图追加短 `ble` 连接信息，其余视图追加详细 `ble` 信息，**只有追加后仍不超过 512 才追加**，否则原样发布。
- USB：状态响应（kind `0x04`）携带同一份 JSON，不追加 `ble`；上限 `kStatusResponseMaxJsonLen == 512`。

## 测量方法

新增 `real_domain_published_views_are_bounded_with_one_capability`：

- 固定取值的字段用代码里的真实取值：`firmware` 从 `main/app_main.cpp` 读取全部 `kFirmwareVersion` 字面量（最长 30 字符）；`target_platform` 为 `macos`／`windows`；`hotkey_mode` 为 `toggle`；音频来源 `kAudioTransport`；板名 `kBoardName`。
- 开放或运行时字段取最坏值：PTT 快捷键（来自用户配置）用 32 个 `a`、16 个 `"`、40 个 `\`；音频主机、错误信息用需转义的引号；计数器取 32 位最大值；`ble` 信息取最坏值。
- 按上面的阶段输入组合遍历，每个结果都断言：`"host_action_v1":true` 恰好出现一次且在 `capabilities` 内；构造结果和最终发布结果都 ≤ 512；`ble` 信息只追加、不改写能力对象。并断言下列 15 个变体都实际出现过。

## 实际发布变体与最终字节数（最坏值）

| 变体 | 内容 | 构造 | 最终 | 余量 | 是否有大小检查 |
|---|---|---|---|---|---|
| 配置确认（BLE） | 指纹 + 快捷键 + 既有能力 | 504 | 504 | 8 | 无（兜底层） |
| USB 状态（确认视图） | 同上 | 504 | 504 | 8 | 无（兜底层） |
| USB 状态（扬声器诊断） | `spk_probe` | 512 | 512 | 0 | 无 |
| 普通：紧凑音频 | `audio` | 511 | 511 | 1 | 有 |
| 普通：紧凑音频 + 错误 | `audio+err` | 501 | 501 | 11 | 有 |
| 普通：音频 + 既有能力 | `audio+legacy+usbm` | 512 | 512 | 0 | 有 |
| 普通：指纹 + 音频 | `fp+audio` | 510 | 510 | 2 | 有 |
| 普通：指纹 + 音频 + 错误 | `fp+audio+err` | 512 | 512 | 0 | 有 |
| 普通：最终兜底（有指纹） | `fp+fpd+legacy+usbm` | 500 | 500 | 12 | 无 |
| 普通：最终兜底（无指纹） | `legacy+usbm` | 357 | 357 | 155 | 无 |
| 诊断：精简诊断 | `power+diag+legacy+usbm` | 512 | 512 | 0 | 有 |
| 诊断：最终兜底 | `legacy+usbm` | 349 | 349 | 163 | 无 |
| 电量：最终兜底 | `fp+power+legacy` | 486 | 486 | 26 | 无（`ble` 按需追加） |
| 扬声器诊断（BLE） | `spk_probe` | 512 | 512 | 0 | 无 |
| BLE 固定兜底 | `status_too_large` | 241 | 241 | 271 | 编译期 |

- **最大最终字节数：512**（上限 512）。等于 512 的变体中，除 `spk_probe` 外都是“先测大小、不超才返回”的分支，按构造不可能超限；`spk_probe` 在全部 32 位指标取最大值时恰好 512，余量 0。
- 原有测试（短固件字符串）的 6 个变体继续通过，最大 512。

## 超限判断与收缩

- 没有任何实际发布变体超过 512，**未做任何收缩**，未改上限。
- 附带观察：用真实固件字符串时，电量视图从不包含电量明细、扬声器启动摘要和电源周期历史；诊断视图从不包含按键与编码器明细；普通阶段常落到只含音频的紧凑视图，旧的能力布尔值会被省略。这些省略是仓库既有回退顺序的结果（电量视图为 `ble` 预留最坏 80 字节）。用“固件字符串缩短 22 字节”等效模拟去掉 `,"host_action_v1":true`，结果形态完全相同，所以这些省略**不是新增字段造成的**。电源当前事实在所有电量视图中保留（诊断视图的最终兜底按既有设计不含电源）；扬声器完整指标保留在 `spk_probe` 视图中。

## 实际修改

- `host_test/host_action_capability_status_tests.cpp` +354 行：新增上述测试，以及判别视图形态的辅助函数、真实取值快照构造函数；用 `static_assert` 固定 `0x04`／`0x05` 分工、USB 状态上限等于 512、预算本身等于 512。
- `host_test/CMakeLists.txt` +4 行：仅给该测试加 `EASY_INPUT_REPO_ROOT`，用于读取固件版本字面量。
- 生产代码、`main/`、`components/`、协议、HID 描述符、GATT、设备身份、硬件配置、分区零改动。

## 测试证据

- **定向测试**：`ble_status_wire_tests`、`config_status_tests`、`firmware_source_contract_tests`、`host_action_capability_status_tests`、`host_action_key_bindings_tests`、`host_action_protocol_tests`、`status_hid_protocol_tests` 共 7/7 通过。
- **全部宿主测试**：全新构建目录，CTest 62/62 通过，0 失败，编译 0 警告。
- **变异检查**（仓库外临时副本，已删除；每次还原后 `touch` 再构建）：

| 编号 | 故意引入的错误 | 修改前测试 | 补齐后测试 |
|---|---|---|---|
| M1 | 共享核心去掉能力字段 | 发现 | 发现 |
| M2 | 能力值改成字符串 `"true"` | 发现 | 发现 |
| M3 | 把上限改成 600 | 编译期已拦截 | 编译期已拦截 |
| M4 | 诊断最终兜底保留明细（可能超限） | 未发现 | 发现 |
| M5 | 能力对象多写一段字段导致确认视图超限 | 发现 | 发现 |
| M6 | BLE 固定兜底重复写入能力 | 发现 | 发现 |
| M7 | 固件版本字符串变长 | 未发现 | 发现 |
| M8 | 能力字段写到 `capabilities` 对象外 | 发现 | 发现 |

## 未验证项

- 未运行 ESP-IDF 构建，未在实板上读取 GATT 或 USB 状态响应，最终字节数是宿主侧构造结果，不等于线上抓包。
- 未与 EasyInput App 联调，未验证 App 是否据此显示 Host Action 能力。
- “等效去掉字段”的比较用缩短固件字符串模拟，是字节等效推算，不是真实删掉字段后的构建。
- 电量明细、扬声器启动摘要、诊断明细在真实固件字符串下不会发布，这是既有行为，本节点只记录、未调整；是否需要调整回退顺序由后续决定。
- `spk_probe` 视图最坏余量为 0，以后在该视图中加任何字段都会超限。

## 本节点不承担的范围

- 不新增能力字段或声明路径，不改变字段顺序和回退顺序，不提高 512 上限。
- 不修改协议、HID 描述符、GATT、设备身份、GPIO、BOOT、GPIO8 共享供电或分区。

## 验收自检

- [x] 枚举真实发布路径与变体（15 个），每个都有字节数。
- [x] 每个变体恰好一个布尔 `"host_action_v1":true`，且位于 `capabilities` 内。
- [x] 最大最终字节数 512 ≤ 512；未超限，未收缩。
- [x] `0x04`／`0x05` 分工以 `static_assert` 固定。
- [x] 定向 7/7、完整 62/62 通过；变异检查 4/8 → 7/8，另 1 项编译期拦截。
- [x] 生产与硬件相关文件零差异；未构建、未烧录。
