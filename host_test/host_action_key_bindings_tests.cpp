#include <array>
#include <cassert>
#include <cstddef>
#include <string>

#include "keyboard/config_payload.h"
#include "keyboard/keymap.h"

namespace {

struct KeyBinding {
  const char* name;
  ai_keyboard::InputId input;
};

constexpr std::array<KeyBinding, 8> kMainKeys{{
    {"KEY1", ai_keyboard::InputId::Key1},
    {"KEY2", ai_keyboard::InputId::Key2},
    {"KEY3", ai_keyboard::InputId::Key3},
    {"KEY4", ai_keyboard::InputId::Key4},
    {"KEY5", ai_keyboard::InputId::Key5},
    {"KEY6", ai_keyboard::InputId::Key6},
    {"KEY7", ai_keyboard::InputId::Key7},
    {"KEY8", ai_keyboard::InputId::Key8},
}};

std::string payload_with_host_action_at(std::size_t target,
                                        const std::string& host_action) {
  std::string payload =
      R"({"schema":"ai_keyboard.v1","profiles":[{"id":"default","keys":{)";
  for (std::size_t index = 0; index < kMainKeys.size(); ++index) {
    if (index != 0) {
      payload += ',';
    }
    payload += '"';
    payload += kMainKeys[index].name;
    payload += R"(":{"press":")";
    payload += index == target ? host_action : "disabled";
    payload += R"("})";
  }
  payload +=
      R"(},"encoder":{"left":"disabled","right":"disabled","press":"disabled"}}]})";
  return payload;
}

void every_main_key_preserves_and_emits_one_host_action_per_press_cycle() {
  const std::string configured_action =
      "host_action:123e4567-e89b-12d3-a456-426614174000";

  for (std::size_t target = 0; target < kMainKeys.size(); ++target) {
    const auto result = ai_keyboard::parse_config_payload(
        payload_with_host_action_at(target, configured_action));
    assert(result.status == ai_keyboard::ConfigParseStatus::Ok);

    for (std::size_t index = 0; index < kMainKeys.size(); ++index) {
      const auto& action =
          result.config.keymap.action_for(kMainKeys[index].input);
      if (index == target) {
        assert(action.kind == ai_keyboard::ActionKind::HostAction);
        assert(action.host_action == configured_action);
        assert(action.host_action.rfind("host_action:", 0) == 0);
      } else {
        assert(action.kind == ai_keyboard::ActionKind::Disabled);
        assert(action.host_action.empty());
      }
    }

    const auto& action =
        result.config.keymap.action_for(kMainKeys[target].input);
    const std::array<ai_keyboard::InputPhase, 2> cycle{{
        ai_keyboard::InputPhase::Pressed,
        ai_keyboard::InputPhase::Released,
    }};
    std::size_t host_action_events = 0;
    for (const auto phase : cycle) {
      const auto event = ai_keyboard::event_for_action(
          action, phase, "RightMeta", "RightOption");
      if (event.kind == ai_keyboard::FirmwareEventKind::HostAction) {
        ++host_action_events;
        assert(phase == ai_keyboard::InputPhase::Pressed);
        assert(event.value == configured_action);
      } else {
        assert(phase == ai_keyboard::InputPhase::Released);
        assert(event.kind == ai_keyboard::FirmwareEventKind::None);
        assert(event.value.empty());
      }
    }
    assert(host_action_events == 1);
  }
}

// Five representative press actions. Rotating them across KEY1-KEY8 means every
// key is exercised with every action, and Host Action always coexists with the
// legacy actions in the same profile through the one shared parse_binding().
enum class RotatedAction { HostAction, Copy, Paste, Hotkey, FixedText };
constexpr std::array<RotatedAction, 5> kRotation{{
    RotatedAction::HostAction,
    RotatedAction::Copy,
    RotatedAction::Paste,
    RotatedAction::Hotkey,
    RotatedAction::FixedText,
}};
constexpr const char* kTestHostAction =
    "host_action:123e4567-e89b-12d3-a456-426614174000";
constexpr const char* kTestHotkey = "Ctrl+Tab";
constexpr const char* kTestFixedText = "hello";

RotatedAction rotated_action_for(std::size_t key_index, std::size_t shift) {
  return kRotation[(key_index + shift) % kRotation.size()];
}

std::string press_json(RotatedAction action) {
  switch (action) {
    case RotatedAction::HostAction:
      return std::string("\"") + kTestHostAction + "\"";
    case RotatedAction::Copy:
      return R"("copy")";
    case RotatedAction::Paste:
      return R"("paste")";
    case RotatedAction::Hotkey:
      return std::string(R"({"hotkey":")") + kTestHotkey + R"("})";
    case RotatedAction::FixedText:
      return std::string(R"({"text":")") + kTestFixedText + R"("})";
  }
  return R"("disabled")";
}

std::string rotated_payload(std::size_t shift) {
  std::string payload =
      R"({"schema":"ai_keyboard.v1","profiles":[{"id":"default","keys":{)";
  for (std::size_t index = 0; index < kMainKeys.size(); ++index) {
    if (index != 0) {
      payload += ',';
    }
    payload += '"';
    payload += kMainKeys[index].name;
    payload += R"(":{"press":)";
    payload += press_json(rotated_action_for(index, shift));
    payload += '}';
  }
  payload +=
      R"(},"encoder":{"left":"disabled","right":"disabled","press":"disabled"}}]})";
  return payload;
}

void expect_parsed_action(const ai_keyboard::Action& action,
                          RotatedAction expected) {
  switch (expected) {
    case RotatedAction::HostAction:
      assert(action.kind == ai_keyboard::ActionKind::HostAction);
      assert(action.host_action == kTestHostAction);
      break;
    case RotatedAction::Copy:
      assert(action.kind == ai_keyboard::ActionKind::Copy);
      assert(action.host_action.empty());
      break;
    case RotatedAction::Paste:
      assert(action.kind == ai_keyboard::ActionKind::Paste);
      assert(action.host_action.empty());
      break;
    case RotatedAction::Hotkey:
      assert(action.kind == ai_keyboard::ActionKind::Hotkey);
      assert(action.hotkey == kTestHotkey);
      assert(action.host_action.empty());
      break;
    case RotatedAction::FixedText:
      assert(action.kind == ai_keyboard::ActionKind::FixedText);
      assert(action.text == kTestFixedText);
      assert(action.host_action.empty());
      break;
  }
}

void expect_press_release_events(const ai_keyboard::Action& action,
                                 RotatedAction expected) {
  // Default platform (macOS) maps semantic copy/paste to Meta+C / Meta+V.
  const auto down = ai_keyboard::event_for_action(
      action, ai_keyboard::InputPhase::Pressed, "RightMeta", "RightOption");
  const auto up = ai_keyboard::event_for_action(
      action, ai_keyboard::InputPhase::Released, "RightMeta", "RightOption");
  switch (expected) {
    case RotatedAction::HostAction:
      assert(down.kind == ai_keyboard::FirmwareEventKind::HostAction);
      assert(down.value == kTestHostAction);
      assert(up.kind == ai_keyboard::FirmwareEventKind::None);
      assert(up.value.empty());
      break;
    case RotatedAction::Copy:
      assert(down.kind == ai_keyboard::FirmwareEventKind::HidKeyDown);
      assert(down.value == "Meta+C");
      assert(up.kind == ai_keyboard::FirmwareEventKind::HidKeyUp);
      assert(up.value == "Meta+C");
      break;
    case RotatedAction::Paste:
      assert(down.kind == ai_keyboard::FirmwareEventKind::HidKeyDown);
      assert(down.value == "Meta+V");
      assert(up.kind == ai_keyboard::FirmwareEventKind::HidKeyUp);
      assert(up.value == "Meta+V");
      break;
    case RotatedAction::Hotkey:
      assert(down.kind == ai_keyboard::FirmwareEventKind::HidKeyDown);
      assert(down.value == kTestHotkey);
      assert(up.kind == ai_keyboard::FirmwareEventKind::HidKeyUp);
      assert(up.value == kTestHotkey);
      break;
    case RotatedAction::FixedText:
      assert(down.kind == ai_keyboard::FirmwareEventKind::FixedText);
      assert(down.value == kTestFixedText);
      assert(up.kind == ai_keyboard::FirmwareEventKind::None);
      assert(up.value.empty());
      break;
  }
  // Host Action must never leak into a legacy action's events.
  if (expected != RotatedAction::HostAction) {
    assert(down.kind != ai_keyboard::FirmwareEventKind::HostAction);
    assert(up.kind != ai_keyboard::FirmwareEventKind::HostAction);
  }
}

void every_main_key_mixes_host_action_with_legacy_actions() {
  for (std::size_t shift = 0; shift < kRotation.size(); ++shift) {
    const auto result =
        ai_keyboard::parse_config_payload(rotated_payload(shift));
    assert(result.status == ai_keyboard::ConfigParseStatus::Ok);
    for (std::size_t index = 0; index < kMainKeys.size(); ++index) {
      const auto expected = rotated_action_for(index, shift);
      const auto& action =
          result.config.keymap.action_for(kMainKeys[index].input);
      expect_parsed_action(action, expected);
      expect_press_release_events(action, expected);
    }
  }
}

void default_keymap_contains_no_host_action() {
  // Test UUIDs live only in host tests; the shipped default keymap must not
  // carry any Host Action binding or identifier.
  const auto keymap = ai_keyboard::DefaultKeymap();
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(ai_keyboard::InputId::Count);
       ++index) {
    const auto& action =
        keymap.action_for(static_cast<ai_keyboard::InputId>(index));
    assert(action.kind != ai_keyboard::ActionKind::HostAction);
    assert(action.host_action.empty());
  }
}

}  // namespace

int main() {
  every_main_key_preserves_and_emits_one_host_action_per_press_cycle();
  every_main_key_mixes_host_action_with_legacy_actions();
  default_keymap_contains_no_host_action();
  return 0;
}
