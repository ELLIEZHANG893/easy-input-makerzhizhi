#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "keyboard/audio_stub.h"
#include "keyboard/ble_status_wire.h"
#include "keyboard/board_pins.h"
#include "keyboard/config_status.h"
#include "keyboard/host_action_protocol.h"
#include "keyboard/status_hid_protocol.h"

namespace {

struct PublishedVariant {
  const char* name;
  std::string base;
  std::string wire;
};

std::size_t count_occurrences(const std::string& value,
                              const std::string& needle) {
  std::size_t count = 0;
  std::size_t offset = 0;
  while ((offset = value.find(needle, offset)) != std::string::npos) {
    ++count;
    offset += needle.size();
  }
  return count;
}

void assert_host_action_v1_capability(const std::string& json) {
  constexpr char kCapability[] = R"("host_action_v1":true)";
  assert(count_occurrences(json, kCapability) == 1);
  assert(json.find(R"("host_action_v1":"true")") == std::string::npos);
  assert(json.find(R"("host_action_v1":false)") == std::string::npos);
  const auto capabilities_begin = json.find(R"("capabilities":{)");
  const auto capabilities_end = json.find('}', capabilities_begin);
  const auto capability = json.find(kCapability, capabilities_begin);
  assert(capabilities_begin != std::string::npos);
  assert(capabilities_end != std::string::npos);
  assert(capability != std::string::npos);
  assert(capability < capabilities_end);
}

ai_keyboard::BleStatusWireSnapshot worst_case_battery_wire() {
  return {
      true,
      true,
      std::numeric_limits<std::uint16_t>::max(),
      std::numeric_limits<std::uint16_t>::max(),
      std::numeric_limits<std::uint16_t>::max(),
  };
}

ai_keyboard::BleDetailedStatusWireSnapshot worst_case_detailed_wire() {
  const auto max = std::numeric_limits<std::uint32_t>::max();
  return {
      true,
      true,
      std::numeric_limits<std::uint16_t>::max(),
      true,
      std::numeric_limits<std::uint16_t>::max(),
      std::numeric_limits<std::uint16_t>::max(),
      std::numeric_limits<std::uint16_t>::max(),
      std::numeric_limits<std::int32_t>::min(),
      max,
      max,
      max,
      max,
      max,
      max,
      max,
      max,
      max,
      max,
      max,
  };
}

ai_keyboard::ConfigStatusSnapshot full_snapshot() {
  ai_keyboard::ConfigStatusSnapshot snapshot;
  snapshot.firmware = "0.4.47";
  snapshot.phase = "status";
  snapshot.status = "cached";
  snapshot.bytes = 128;
  snapshot.crc16 = 4660;
  snapshot.ptt_hotkey = "F12";
  snapshot.edit_ptt_hotkey = "F14";
  snapshot.hotkey_mode = "toggle";
  snapshot.saved = true;
  snapshot.battery_mv = 3800;
  snapshot.battery_percent = 55;
  snapshot.target_platform = "macos";
  return snapshot;
}

ai_keyboard::ConfigStatusSnapshot compact_snapshot() {
  auto snapshot = full_snapshot();
  snapshot.firmware = "0.4.47-idf-v2-host-action-capability";
  snapshot.status = std::string(64, 's');
  snapshot.audio.enabled = true;
  snapshot.audio.source = "wifi_udp";
  snapshot.audio.microphone_source = "keyboard";
  snapshot.audio.capture = "mic_streaming";
  snapshot.audio.host = "192.168.255.255";
  snapshot.audio.port = 65535;
  snapshot.audio.control_state = "ready loops=4294967295 hb=4294967295";
  snapshot.audio.last_error = std::string(240, 'e');
  snapshot.audio.sent_packets = std::numeric_limits<std::uint32_t>::max();
  snapshot.audio.sent_bytes = std::numeric_limits<std::uint32_t>::max();
  snapshot.audio.session_id = std::numeric_limits<std::uint64_t>::max();
  snapshot.power.mode = "awake";
  snapshot.power.inactive_ms = std::numeric_limits<std::uint32_t>::max();
  snapshot.power.deep_sleep_block = std::string(64, 'd');
  snapshot.power.last_wake = std::string(64, 'w');
  snapshot.power.cycle_seq = std::numeric_limits<std::uint32_t>::max();
  snapshot.power.cycle_inactive_ms = std::numeric_limits<std::uint32_t>::max();
  snapshot.power.cycle_deep_sleep = true;
  snapshot.power.cycle_wake = std::string(64, 'c');
  return snapshot;
}

ai_keyboard::ConfigStatusSnapshot battery_snapshot() {
  ai_keyboard::ConfigStatusSnapshot snapshot;
  snapshot.firmware = "0.4.47";
  snapshot.phase = "battery";
  snapshot.status = "fresh";
  snapshot.bytes = 128;
  snapshot.crc16 = 4660;
  snapshot.saved = true;
  snapshot.battery_mv = 3800;
  snapshot.battery_percent = 55;
  snapshot.target_platform = "macos";
  snapshot.battery = {3790, "battery", 24, true};
  snapshot.power.mode = "awake";
  snapshot.power.inactive_ms = 600000;
  snapshot.power.deep_sleep_block = "keyboard_mic";
  snapshot.power.last_wake = "deep_key";
  snapshot.power.cycle_seq = 8;
  snapshot.power.cycle_inactive_ms = 420000;
  snapshot.power.cycle_deep_sleep = true;
  snapshot.power.cycle_wake = "deep_key";
  return snapshot;
}

ai_keyboard::ConfigStatusSnapshot speaker_probe_snapshot(
    ai_keyboard::SpeakerProbeSnapshot* speaker) {
  const auto max = std::numeric_limits<std::uint32_t>::max();
  speaker->present = true;
  speaker->version = ai_keyboard::kSpeakerProbeStatusVersion;
  speaker->generation = max;
  speaker->stage = ai_keyboard::SpeakerProbeStage::Done;
  speaker->result = ai_keyboard::SpeakerProbeResult::Cancelled;
  speaker->error = ai_keyboard::SpeakerProbeError::Unknown;
  speaker->raw_error = std::numeric_limits<std::int32_t>::min();
  speaker->microphone_generation = max;
  speaker->first_submit_us = max;
  speaker->decode_total_us = max;
  speaker->decode_max_us = max;
  speaker->decoded_frames = max;
  speaker->decoded_pcm_bytes = max;
  speaker->stack_high_water_bytes = max;
  speaker->heap_begin_free = max;
  speaker->heap_terminal_free = max;
  speaker->heap_largest_block = max;
  speaker->heap_minimum_free = max;
  speaker->decoded_abs_peak = max;
  speaker->decoded_rms_permille = max;

  ai_keyboard::ConfigStatusSnapshot snapshot;
  snapshot.firmware = std::string(40, 'f');
  snapshot.phase = "spk_probe";
  snapshot.status = "cancelled";
  snapshot.bytes = std::numeric_limits<std::uint16_t>::max();
  snapshot.crc16 = std::numeric_limits<std::uint16_t>::max();
  snapshot.saved = true;
  snapshot.target_platform = "windows";
  snapshot.speaker = speaker;
  return snapshot;
}

void every_published_variant_has_one_boolean_capability_and_is_bounded() {
  const auto detailed = worst_case_detailed_wire();
  const auto full = ai_keyboard::build_config_status_json(full_snapshot());
  assert(full.find(R"("compact":true)") == std::string::npos);

  const auto compact = ai_keyboard::build_config_status_json(compact_snapshot());
  assert(compact.find(R"("compact":true)") != std::string::npos);

  ai_keyboard::SpeakerProbeSnapshot speaker;
  const auto speaker_probe = ai_keyboard::build_config_status_json(
      speaker_probe_snapshot(&speaker));
  assert(speaker_probe.find(R"("phase":"spk_probe")") != std::string::npos);

  const auto confirmation = ai_keyboard::build_config_confirmation_status_json(
      compact_snapshot());
  assert(confirmation.find(R"("bytes":128)") != std::string::npos);
  assert(confirmation.find(R"("audio")") == std::string::npos);

  const auto battery = ai_keyboard::build_config_status_json(battery_snapshot());
  assert(battery.find(R"("phase":"battery")") != std::string::npos);
  const auto battery_wire = ai_keyboard::append_ble_status_wire_json(
      battery, worst_case_battery_wire());
  assert(battery_wire.find(R"("ble":{)") != std::string::npos);

  const std::string fallback = ai_keyboard::kConfigStatusFallbackJson;
  std::vector<PublishedVariant> variants{
      {"full", full,
       ai_keyboard::append_ble_detailed_status_wire_json(full, detailed)},
      {"compact", compact,
       ai_keyboard::append_ble_detailed_status_wire_json(compact, detailed)},
      {"speaker_probe", speaker_probe,
       ai_keyboard::append_ble_detailed_status_wire_json(speaker_probe, detailed)},
      {"confirmation", confirmation,
       ai_keyboard::append_ble_detailed_status_wire_json(confirmation, detailed)},
      {"battery", battery, battery_wire},
      {"ble_fallback", fallback,
       ai_keyboard::append_ble_detailed_status_wire_json(fallback, detailed)},
  };

  std::size_t maximum = 0;
  for (const auto& variant : variants) {
    assert_host_action_v1_capability(variant.base);
    assert_host_action_v1_capability(variant.wire);
    assert(variant.wire.size() <= ai_keyboard::kConfigStatusGattSafeLen);
    maximum = std::max(maximum, variant.wire.size());
    std::cout << "STATUS_BYTES " << variant.name
              << " base=" << variant.base.size()
              << " final=" << variant.wire.size()
              << " remaining="
              << (ai_keyboard::kConfigStatusGattSafeLen - variant.wire.size())
              << '\n';
  }
  std::cout << "STATUS_MAX final=" << maximum
            << " limit=" << ai_keyboard::kConfigStatusGattSafeLen
            << " remaining="
            << (ai_keyboard::kConfigStatusGattSafeLen - maximum) << '\n';
}

// ---------------------------------------------------------------------------
// Real-domain enumeration of every status view the firmware publishes.
//
// publish_config_status() (main/app_main.cpp) feeds each phase a different
// snapshot shape; the BLE transport then appends one optional overlay that is
// only inserted when the complete JSON still fits in 512 bytes, and the USB
// status response (command kind 0x04) carries the base JSON unchanged.
// Fields with a fixed domain in the firmware use that domain:
//   firmware        every kFirmwareVersion literal in main/app_main.cpp
//   target_platform "macos" / "windows"      hotkey_mode "hold" / "toggle"
//   audio source    kAudioTransport           diag board  kBoardName
// Open-ended or runtime fields (PTT hotkeys from config, audio host, status,
// counters, diagnostic strings) use worst-case escapes and maxima.
// ---------------------------------------------------------------------------

std::vector<std::string> firmware_versions_from_source() {
  std::ifstream input(std::string(EASY_INPUT_REPO_ROOT) + "/main/app_main.cpp");
  assert(input.good());
  std::ostringstream contents;
  contents << input.rdbuf();
  const auto source = contents.str();
  std::vector<std::string> versions;
  const std::string marker = "constexpr const char* kFirmwareVersion";
  std::size_t offset = 0;
  while ((offset = source.find(marker, offset)) != std::string::npos) {
    const auto open = source.find('"', offset);
    const auto close = source.find('"', open + 1);
    assert(open != std::string::npos && close != std::string::npos);
    versions.push_back(source.substr(open + 1, close - open - 1));
    offset = close;
  }
  assert(versions.size() >= 2);
  return versions;
}

bool contains(const std::string& json, const char* needle) {
  return json.find(needle) != std::string::npos;
}

std::string view_shape(const std::string& json) {
  if (contains(json, R"("status":"status_too_large")")) {
    return "ble_fallback";
  }
  if (contains(json, R"("phase":"spk_probe")") && contains(json, R"("mu":)")) {
    return "spk_probe";
  }
  if (!contains(json, R"("compact":true)") &&
      contains(json, R"("hotkey_mode":)") &&
      json.find(R"("bytes":)") < json.find(R"("target_platform":)")) {
    return contains(json, R"("audio":{)") ? "full+audio" : "full";
  }
  std::string shape;
  const auto flag = [&](const char* needle, const char* name) {
    if (contains(json, needle)) {
      shape += shape.empty() ? "" : "+";
      shape += name;
    }
  };
  flag(R"("bytes":)", "fp");
  flag(R"("ptt_hotkey":)", "fpd");
  flag(R"("battery_raw_mv":)", "bat");
  flag(R"("audio":{)", "audio");
  flag(R"("last_error":)", "err");
  flag(R"("power":{)", "power");
  flag(R"("cycle_seq":)", "cycle");
  flag(R"("diag":{)", "diag");
  flag(R"("keys":)", "diagd");
  flag(R"("semantic_actions":)", "legacy");
  flag(R"("usb_management_v1":)", "usbm");
  flag(R"("spk":{)", "spk");
  return shape.empty() ? "core" : shape;
}

struct VariantBytes {
  std::size_t base = 0;
  std::size_t final = 0;
};

class VariantLedger {
 public:
  void record(const std::string& group,
              const std::string& base,
              const std::string& wire) {
    assert_host_action_v1_capability(base);
    assert_host_action_v1_capability(wire);
    assert(base.size() <= ai_keyboard::kConfigStatusGattSafeLen);
    assert(wire.size() <= ai_keyboard::kConfigStatusGattSafeLen);
    // Overlays only append; they never rewrite the capabilities object.
    assert(wire.compare(0, base.size() - 1, base, 0, base.size() - 1) == 0);
    auto& bytes = by_variant_[group + ":" + view_shape(base)];
    bytes.base = std::max(bytes.base, base.size());
    bytes.final = std::max(bytes.final, wire.size());
    maximum_ = std::max(maximum_, wire.size());
  }

  bool reached(const std::string& variant) const {
    return by_variant_.count(variant) != 0;
  }

  void print() const {
    for (const auto& [name, bytes] : by_variant_) {
      std::cout << "REAL_STATUS_BYTES " << name << " base=" << bytes.base
                << " final=" << bytes.final << " remaining="
                << (ai_keyboard::kConfigStatusGattSafeLen - bytes.final)
                << '\n';
    }
    std::cout << "REAL_STATUS_MAX final=" << maximum_
              << " limit=" << ai_keyboard::kConfigStatusGattSafeLen
              << " variants=" << by_variant_.size() << '\n';
  }

 private:
  std::map<std::string, VariantBytes> by_variant_;
  std::size_t maximum_ = 0;
};

ai_keyboard::AudioStatusSnapshot runtime_audio(int scale,
                                               bool max_counters,
                                               bool control_state) {
  const auto counter =
      max_counters ? std::numeric_limits<std::uint32_t>::max() : 7U;
  ai_keyboard::AudioStatusSnapshot audio;
  audio.enabled = scale > 0;
  audio.source = ai_keyboard::kAudioTransport;
  audio.microphone_source = "keyboard";
  audio.transport = ai_keyboard::kAudioTransport;
  audio.capture = std::string(static_cast<std::size_t>(scale) * 8, 'c');
  audio.host = std::string(static_cast<std::size_t>(scale) * 8, '"');
  audio.port = scale > 0 ? 65535 : 0;
  audio.mic_channel = ai_keyboard::kAudioMicrophoneChannel;
  audio.speaker_channel = ai_keyboard::kAudioSpeakerChannel;
  audio.sent_packets = counter;
  audio.sent_bytes = counter;
  audio.last_rms_milli = counter;
  audio.peak_rms_milli = counter;
  audio.send_errors = counter;
  audio.read_errors = counter;
  audio.recovery_count = counter;
  audio.session_generation = counter;
  audio.session_id = max_counters ? std::numeric_limits<std::uint64_t>::max() : 7U;
  audio.stream_phase = std::string(static_cast<std::size_t>(scale) * 6, 'p');
  audio.stop_reason = std::string(static_cast<std::size_t>(scale) * 6, 'r');
  audio.control_state =
      control_state ? std::string(static_cast<std::size_t>(scale) * 14, 'k')
                    : std::string();
  audio.last_error = std::string(static_cast<std::size_t>(scale) * 12, '"');
  return audio;
}

ai_keyboard::PowerDiagnosticsSnapshot runtime_power(int scale, bool cycle) {
  const auto max = std::numeric_limits<std::uint32_t>::max();
  ai_keyboard::PowerDiagnosticsSnapshot power;
  power.mode = "awake";
  power.inactive_ms = scale > 1 ? max : 1000U;
  power.deep_sleep_block = std::string(static_cast<std::size_t>(scale) * 8, 'd');
  power.last_wake = std::string(static_cast<std::size_t>(scale) * 4, 'w');
  power.wake_edges = max;
  power.usb = true;
  if (cycle) {
    power.cycle_seq = scale > 1 ? max : 3U;
    power.cycle_inactive_ms = scale > 1 ? max : 1000U;
    power.cycle_deep_sleep = true;
    power.cycle_wake = std::string(static_cast<std::size_t>(scale) * 4, 'c');
  }
  return power;
}

ai_keyboard::BoardDiagnosticsSnapshot runtime_diagnostics(int scale) {
  const auto max = std::numeric_limits<std::uint32_t>::max();
  ai_keyboard::BoardDiagnosticsSnapshot diagnostics;
  diagnostics.board = ai_keyboard::kBoardName;
  diagnostics.keys = std::string(static_cast<std::size_t>(scale) * 4, 'k');
  diagnostics.encoder = std::string(static_cast<std::size_t>(scale) * 4, 'e');
  diagnostics.last_input = std::string(static_cast<std::size_t>(scale) * 8, 'l');
  diagnostics.input_drops = scale > 1 ? max : 1U;
  diagnostics.encoder_drops = scale > 1 ? max : 1U;
  return diagnostics;
}

void real_domain_published_views_are_bounded_with_one_capability() {
  // Kind ownership is unchanged: 0x04 carries status JSON, 0x05 Host Action.
  static_assert(ai_keyboard::kStatusResponseCommandKind == 0x04);
  static_assert(ai_keyboard::kHostActionV1CommandKind == 0x05);
  static_assert(ai_keyboard::kStatusResponseMaxJsonLen ==
                ai_keyboard::kConfigStatusGattSafeLen);
  // The budget is a fixed contract, not a tunable: raising it must fail here.
  static_assert(ai_keyboard::kConfigStatusGattSafeLen == 512);

  const auto detailed = worst_case_detailed_wire();
  const auto battery_wire = worst_case_battery_wire();
  ai_keyboard::SpeakerProbeSnapshot speaker;
  const auto probe_base = speaker_probe_snapshot(&speaker);

  const std::vector<std::string> hotkeys{
      "F12", std::string(32, 'a'), std::string(16, '"'), std::string(40, '\\')};
  VariantLedger ledger;

  for (const auto& firmware : firmware_versions_from_source()) {
    for (const auto& hotkey : hotkeys) {
      for (const char* platform : {"macos", "windows"}) {
        ai_keyboard::ConfigStatusSnapshot core;
        core.firmware = firmware;
        core.ptt_hotkey = hotkey;
        core.edit_ptt_hotkey = hotkey;
        core.hotkey_mode = "toggle";
        core.saved = true;
        core.bytes = 2048;
        core.crc16 = 65535;
        core.battery_mv = 65535;
        core.battery_percent = 100;
        core.target_platform = platform;

        // Config confirmation: BLE "push"/"platform", and the USB 0x04
        // status response (forced confirmation view, no BLE overlay).
        for (const char* phase : {"push", "platform", "status"}) {
          auto snapshot = core;
          snapshot.phase = phase;
          snapshot.status = "fixed_text_too_large";
          const auto json =
              ai_keyboard::build_config_confirmation_status_json(snapshot);
          ledger.record("confirm", json,
                        ai_keyboard::append_ble_detailed_status_wire_json(
                            json, detailed));
          ledger.record("usb_status", json, json);
        }

        // Ordinary phases always carry live audio status, never power/diag.
        for (const char* phase : {"boot", "initializing"}) {
          for (const std::size_t bytes : {std::size_t{0}, std::size_t{2048}}) {
            for (int scale = 0; scale <= 3; ++scale) {
              for (const bool max_counters : {false, true}) {
                for (const bool control : {false, true}) {
                  auto snapshot = core;
                  snapshot.phase = phase;
                  snapshot.status = "no_stored_config";
                  snapshot.bytes = bytes;
                  snapshot.audio = runtime_audio(scale, max_counters, control);
                  const auto json = ai_keyboard::build_config_status_json(snapshot);
                  ledger.record("normal", json,
                                ai_keyboard::append_ble_detailed_status_wire_json(
                                    json, detailed));
                }
              }
            }
          }
        }

        // Diagnostic long press: board + power, no audio, no fingerprint.
        for (int scale = 1; scale <= 3; ++scale) {
          for (const bool cycle : {false, true}) {
            auto snapshot = core;
            snapshot.phase = "diag";
            snapshot.status = std::string(119, 's');
            snapshot.bytes = 0;
            snapshot.crc16 = 0;
            snapshot.power = runtime_power(scale, cycle);
            snapshot.diagnostics = runtime_diagnostics(scale);
            const auto json = ai_keyboard::build_config_status_json(snapshot);
            ledger.record("diag", json,
                          ai_keyboard::append_ble_detailed_status_wire_json(
                              json, detailed));
          }
        }

        // GATT battery refresh: power always, battery detail when sampled,
        // compact speaker boot core when the product build recorded one.
        for (int scale = 0; scale <= 3; ++scale) {
          for (const bool cycle : {false, true}) {
            for (const bool sampled : {false, true}) {
              for (const bool boot_probe : {false, true}) {
                auto snapshot = core;
                snapshot.phase = "battery";
                snapshot.status = "cached";
                snapshot.bytes = scale == 0 ? 0 : 2048;
                snapshot.crc16 = scale == 0 ? 0 : 65535;
                snapshot.battery_mv = scale == 0 ? 3800 : 65535;
                snapshot.power = runtime_power(scale, cycle);
                if (sampled) {
                  snapshot.battery = {
                      static_cast<std::uint16_t>(scale == 0 ? 3790 : 65535),
                      scale == 0 ? "battery" : "discharging",
                      scale == 0 ? 24U : std::numeric_limits<std::uint32_t>::max(),
                      true};
                }
                snapshot.speaker = boot_probe ? &speaker : nullptr;
                const auto json = ai_keyboard::build_config_status_json(snapshot);
                ledger.record("battery", json,
                              ai_keyboard::append_ble_status_wire_json(
                                  json, battery_wire));
              }
            }
          }
        }

        // Speaker diagnostic builds publish one probe view on USB and BLE.
        auto probe = probe_base;
        probe.firmware = firmware;
        probe.target_platform = platform;
        const auto probe_json = ai_keyboard::build_config_status_json(probe);
        ledger.record("spk_probe", probe_json,
                      ai_keyboard::append_ble_detailed_status_wire_json(
                          probe_json, detailed));
        ledger.record("usb_status", probe_json, probe_json);
      }
    }
  }

  const std::string fallback = ai_keyboard::kConfigStatusFallbackJson;
  ledger.record("ble_fallback", fallback,
                ai_keyboard::append_ble_detailed_status_wire_json(fallback, detailed));

  ledger.print();

  // Every view the firmware actually publishes under real field domains was
  // exercised. Builder branches that keep battery detail, the speaker boot
  // core, power-cycle history, diag details or the full/compact ordinary view
  // are not reachable with the real firmware strings: their worst-case size
  // (including the BLE reserve) already exceeds 512 bytes, so the builder
  // falls back by the repository's existing optional-field order. Those
  // branches still share the same capability emitters, which the
  // short-firmware variants in the test above cover.
  for (const char* variant : {
           "confirm:fp+fpd+legacy+usbm",
           "usb_status:fp+fpd+legacy+usbm",
           "usb_status:spk_probe",
           "normal:audio",
           "normal:audio+err",
           "normal:audio+legacy+usbm",
           "normal:fp+audio",
           "normal:fp+audio+err",
           "normal:legacy+usbm",
           "normal:fp+fpd+legacy+usbm",
           "diag:power+diag+legacy+usbm",
           "diag:legacy+usbm",
           "battery:fp+power+legacy",
           "spk_probe:spk_probe",
           "ble_fallback:ble_fallback",
  }) {
    if (!ledger.reached(variant)) {
      std::cerr << "variant not reached: " << variant << '\n';
      assert(false);
    }
  }
}

}  // namespace

int main() {
  every_published_variant_has_one_boolean_capability_and_is_bounded();
  real_domain_published_views_are_bounded_with_one_capability();
  return 0;
}
