# 计划（plan）——契约

> 初始化空白模板。计划经确认后再填写和执行。

## 里程碑

- [x] 完成烧录前只读审计：云端与本机烧录副本源码树一致、工作区干净、`git diff --check` 通过；`main` 以来 27 个文件全部可解释（灯光、音量两项练习 + 测试 + flow）；引脚、BOOT／电源、目标／分区／有效配置、USB／BLE 身份与 HID／GATT、Host Action 合同、隐私均 PASS；托管依赖与构建产物对得上锁文件和元数据；`SAFE_TO_REQUEST_FLASH=YES`；未访问设备、未烧录。
- [x] 完成软件侧总验收：重新发现并执行全部宿主测试 62/62（相对公开基线 60 多出的两项来自灯光与音量练习），四类覆盖（协议、八键、单通道路由、能力状态）有测试函数证据；ESP-IDF v5.5.5 / esp32s3 / v2 默认构建通过，0 警告，App 1,640,832 bytes，factory 分区余量 48%；未烧录、未运行 App。
- [x] 完成 Host Action v1 能力声明与状态字节预算核验：确认 4 个发出点均写布尔 `"host_action_v1":true`；按真实取值枚举 15 个实际发布变体（配置确认、USB 状态、普通阶段回退、诊断、电量、扬声器诊断、BLE 兜底），最大最终 512 字节，均不超限，未收缩；`0x04`／`0x05` 分工以 `static_assert` 固定；只改宿主测试，定向 7/7、完整 62/62 通过；未构建、未烧录。
- [x] 完成 Host Action v1 接入现有 App Command 发送链路核验：确认按键 → `event_for_action()` → `dispatch_firmware_event()` → `route_for_firmware_event()` → USB 或 BLE 单通道 → 同一 `encode_host_action_v1()` → `send_app_command_report()` 已完整存在；补齐共享编码、事件分发、路由三处宿主测试，生产代码零改动；定向 7/7、完整宿主 62/62 通过，变异检查 2/8 → 8/8；未构建固件、未烧录。
- [x] 完成 Host Action v1 八键共用解析与旧动作回归：确认 KEY1—KEY8 共用 `parse_binding()`；新增五种动作在八键上轮转的混合配置测试与默认 Keymap 不含 Host Action 的测试，生产代码零改动；定向 6/6、完整宿主 62/62 通过；示例 UUID 仅在宿主测试；未构建固件、未接 USB／BLE、未烧录。
- [x] 完成 Host Action v1 纯逻辑验证与测试缺口补齐：已有实现满足合同，生产代码零改动；补齐过长、仅前缀、错误分隔符、前缀大写及 Keymap 层不规范值不生成事件的宿主测试；定向 6/6、完整宿主 62/62 通过，变异检查证明新增用例有效；未构建固件、未接 USB／BLE、未烧录。
- [x] 完成 Host Action v1 固定合同复核：在已实现的基线上逐项对照配置格式、规范小写 UUID 拒绝策略、`0x11`／`0x05`／`0`／`1`／`36` 字节布局、`[40..62]` 归零余量、`0x04` 状态响应、按下一次、USB／BLE 同内容单通道、能力声明与 512 字节 BLE 状态预算，未发现冲突；本节点只更新 `flow/`，未改代码、未测试、未构建、未烧录。
- [x] 建立 Host Action v1 完整基线与学员起点：固化三层边界，验证 Maker `main`，创建只移除 Host Action 的课程分支与两个固定标签，并在隔离现场完成生产公共代码同步验证；不引入 OTA、不推送、不烧录。
- [x] 完成 Host Action v1 烧录前只读修改范围审计：可靠基线为 `main@34087cd`，写入前 20 个 unstaged tracked、12 个 untracked 与 2 个 ignored 生成目录均逐文件／目录归类；八项禁止边界全部 PASS，无无关或无法解释项，`SAFE_TO_REQUEST_FLASH = YES`，但未访问设备或授予烧录。
- [x] 完成 Host Action v1 软件侧总验收：重新配置后 CTest 实际发现／执行／通过 59/59/59、0 失败；ESP-IDF v5.5.5 对默认 `v2`／`esp32s3` 构建通过，应用镜像 `0x190470` 字节、App 分区剩余 `0x16fb90` 字节（48%）；未烧录、未运行 App 或真机验证。
- [x] 完成 Host Action v1 第 08 步能力声明：完整、紧凑、speaker probe、确认、battery、BLE fallback 与最终发布变体都只包含一次布尔 `host_action_v1: true`；状态预算回归完成，定向 6/6、完整宿主 59/59 通过，最大最终 BLE 状态 512/512 字节；未运行 ESP-IDF 构建或实板验证。
- [x] 完成 Host Action v1 Vendor HID App Command 发送接入：USB／BLE 共用共享编码，沿用 USB 优先且失败不迁移的单通道路由；定向 8/8、完整宿主 58/58、ESP-IDF 5.5.5 默认构建通过，未改描述符、GATT、能力声明或硬件边界。
- [x] 完成 Host Action v1 的 KEY1—KEY8 配置覆盖：确认八键共用 `parse_binding()`，仅新增参数化宿主测试；定向 5/5、完整宿主 58/58 通过，生产逻辑和 USB／BLE 均无本轮差异。
- [x] 完成 Host Action v1 纯逻辑节点：严格解析完整配置值、Keymap 按下／松开事件、共享 `0x11`／`0x05` 编码和失败优先宿主测试；定向 4/4、完整宿主 57/57 通过，未修改 `main/` 或 USB／BLE 适配。
- [x] 核对并冻结 Host Action v1 兼容协议：现有 App Command 编号、63 字节容器和 USB 优先的单通道路由均无冲突；本节点只更新 `flow/`，未实现代码、未运行测试或构建、未烧录。
- [x] 从干净 `main` 基线完成 Host Action 课程复跑的工程结构与 EasyInput V2.0 板型只读核对；本节点只更新 `flow/`，未改代码、未构建、未访问设备。
- [x] 补齐 EasyInput App 已使用的编码器原生文字选择能力：兼容 `cursor + text_caret_select` 与旧动作名，完成手势、标准键盘报告、队列背压和 USB/BLE owner 保护；仅含本功能的暂存快照完整宿主测试 56/56、ESP-IDF 5.5.5 默认构建通过。

## 任务拆解

| 任务 | 负责角色 / 工具 | 输入 | 产出路径 | 验收标准 |
|---|---|---|---|---|
| T01 · 工程结构与板型只读核对 | 开发 Agent／`project-flow-cy`／`easyinput-board-cy` | 干净 `main` 基线、Maker 项目规则、EasyInput V2.0 板级合同 | `flow/tasks/T01-工程结构与板型只读核对.md` | 指明工程分层、按键链路入口和保护边界；无非 `flow/` 差异 |
| T02 · Host Action v1 固定兼容协议 | 开发 Agent／`project-flow-cy` | 使用者冻结的 Host Action v1 合同、当前 Maker App Command 与状态协议 | `flow/tasks/T02-Host-Action-v1-固定兼容协议.md` | 逐项原样固定合同、完整改动层与不承担范围；确认无编号或容量冲突；本节点无非 `flow/` 差异 |
| T03 · Host Action v1 纯逻辑实现 | 固件开发 Agent／`project-flow-cy` | T02 冻结合同、现有配置／Keymap／宿主测试基线 | `flow/tasks/T03-Host-Action-v1-纯逻辑实现.md` | 先红测后实现；定向与完整宿主测试通过；`main/`、USB、BLE 无差异 |
| T04 · Host Action v1 八键配置覆盖 | 固件开发 Agent／`project-flow-cy` | T02 合同、T03 纯逻辑实现、KEY1—KEY8 通用解析链路 | `flow/tasks/T04-Host-Action-v1-八键配置覆盖.md` | 八键逐项解析并保留完整前缀；每次按下仅一个 Host Action、松开为 `None`；旧动作回归通过；示例 UUID 仅在宿主测试 |
| T05 · 编码器原生文字选择 | 固件开发 Agent | EasyInput App 现有配置合同、当前 Maker 源码 | `flow/tasks/T05-编码器原生文字选择兼容.md` | 软件侧合同全部通过；App 重新同步与实板文字选择分别提供后续证据 |
| T06 · Host Action v1 Vendor HID 发送接入 | 固件开发 Agent／`project-flow-cy`／`esp-idf-cy` | T02 冻结合同、T03 共享编码、T04 八键事件、现有 App Command 队列与路由 | `flow/tasks/T06-Host-Action-v1-Vendor-HID-发送接入.md` | USB／BLE 共用共享编码；固定 payload 与单通道不双发证据通过；完整宿主测试与 ESP-IDF 5.5.5 构建通过；禁止范围无差异 |
| T08 · Host Action v1 能力声明与 BLE 状态预算 | 固件开发 Agent／`project-flow-cy` | T02 冻结合同、T06 发送接入、现有状态 JSON 与 BLE 最终发布路径 | `flow/tasks/T08-Host-Action-v1-能力声明与BLE状态预算.md` | 所有发布状态只含一次布尔能力；speaker 与 power/cycle 预算规则通过；最终 UTF-8 状态不超过 512 字节；完整宿主测试通过；不提前做 App／真机验证 |
| T09 · Host Action v1 软件侧总验收 | 固件开发 Agent／`project-flow-cy`／`esp-idf-cy` | T02／T03／T04／T06／T08 交接、当前工作区、CTest 实际清单、ESP-IDF v5.5.5 | `flow/tasks/T09-Host-Action-v1-软件侧总验收.md` | 重新发现／执行／通过的宿主测试数量一致且 0 失败；默认 `v2`／`esp32s3` 构建通过；记录产物、空间、警告、边界和未验证项 |
| T10 · Host Action v1 烧录前只读修改范围审计 | 审计 Agent／`project-flow-cy`／`easyinput-board-cy` | `main@34087cd` 课程起点、全部 staged／unstaged／untracked／ignored 内容、冻结合同和 T09 总验收 | `flow/tasks/T10-Host-Action-v1-烧录前只读修改范围审计.md` | 每项变化有合理归属；硬件、身份、协议、隐私和生成物八项边界全部 PASS；只更新 `flow/`，不访问设备或烧录 |
| T11 · Host Action v1 课程分支与公共基线同步 | 维护 Agent／`project-flow-cy`／`git-workflow-cy`／`course-design-cy`／`easyinput-board-cy`／`esp-idf-cy` | 已确认的三层边界、Maker 完整基线、产品侧进入 OTA 前的公共固件事实 | `flow/tasks/T11-Host-Action-v1-课程分支与公共基线同步.md` | `main` 与 starter 仅有课程目标差异；BLE 修复保留、OTA 排除；两边宿主测试和 ESP-IDF 5.5.5 构建通过；分支与固定标签可追溯；不推送、不烧录 |
| T12 · Host Action v1 固定合同复核 | 审计 Agent／`project-flow-cy` | T02 冻结合同、当前已实现基线与 T03—T11 记录 | `flow/tasks/T12-Host-Action-v1-固定合同复核.md` | 每个固定字段有代码证据；无编号或容器冲突；单通道不双发；BLE 状态 ≤512 字节有测试约束；只改 `flow/` |
| T13 · Host Action v1 纯逻辑验证与测试缺口补齐 | 固件开发 Agent／`project-flow-cy` | T02 合同、T12 复核、现有纯逻辑实现与宿主测试 | `flow/tasks/T13-Host-Action-v1-纯逻辑验证与测试缺口补齐.md` | 先验证后补缺；只改 `host_test/`；定向与完整宿主测试通过；新增用例经变异检查有效；不接 USB／BLE、不烧录 |
| T14 · Host Action v1 八键共用解析与旧动作回归 | 固件开发 Agent／`project-flow-cy` | T02 合同、T04 八键测试、现有配置解析与旧动作测试 | `flow/tasks/T14-Host-Action-v1-八键共用解析与旧动作回归.md` | 八键共用解析有代码证据；八键 × 五种动作逐键断言；旧动作回归通过；示例 UUID 不进生产；不改 USB／BLE、能力声明、硬件、分区或身份 |
| T15 · Host Action v1 接入现有 App Command 发送链路核验 | 固件开发 Agent／`project-flow-cy` | T02 合同、T06 接入记录、T12—T14、现有分发与适配层 | `flow/tasks/T15-Host-Action-v1-App-Command发送链路接入核验.md` | 调用链逐步有代码位置；USB／BLE 分支内容一致且参数顺序固定；Host Action 不进按住型键盘通道、先选路后单通道发送；路由与固定文字／App 命令一致；不增加能力声明，不改 HID／GATT、身份、硬件或分区 |
| T16 · Host Action v1 能力声明与状态字节预算核验 | 固件开发 Agent／`project-flow-cy` | T02 合同、T07 能力声明、`publish_config_status()` 与 BLE／USB 状态发布路径 | `flow/tasks/T16-Host-Action-v1-能力声明与状态字节预算核验.md` | 每个实际发布变体恰好一个布尔能力字段且在 `capabilities` 内；逐变体最终字节数 ≤ 512 有测试输出；未提高上限、未删必要信息；不改协议、HID／GATT、身份、硬件或分区 |
| T17 · 软件侧总验收 | 固件开发 Agent／`project-flow-cy`／`esp-idf-cy` | 全部宿主测试、仓库版本化声明（`dependencies.lock`、`esp_hid` 版本锁、根 `CMakeLists.txt`） | `flow/tasks/T17-软件侧总验收.md` | 发现数 = 执行数 = 通过数且失败 0；四类覆盖可定位；按仓库声明完成默认构建并记录工具版本、产物、分区余量、警告；测试／构建／烧录／真实功能四类证据分开 |
| T18 · 烧录前只读审计 | 固件开发 Agent／`project-flow-cy`／`easyinput-board-cy` | 课程锚点、`main..HEAD` 真实差异、两份副本的 Git 状态与忽略内容、板级合同 | `flow/tasks/T18-烧录前只读审计.md` | 每处变化可解释；禁止项全为 PASS；生成物与托管依赖对得上锁文件和构建元数据；不改文件、不访问设备 |

## 实时进展 / 交接棒

见 `flow/进展.md` 顶部；本文件只维护已经确认的计划。
