#include "keyboard/led_brightness.h"

namespace ai_keyboard {

std::uint8_t clamp_led_brightness_percent(int percent) {
  if (percent <= 0) {
    return 0;
  }
  if (percent >= kMaxLedBrightnessPercent) {
    return kMaxLedBrightnessPercent;
  }
  return static_cast<std::uint8_t>(percent);
}

std::uint8_t scale_led_channel(std::uint8_t value, int percent) {
  const unsigned clamped = clamp_led_brightness_percent(percent);
  const unsigned half = kMaxLedBrightnessPercent / 2U;
  return static_cast<std::uint8_t>(
      (static_cast<unsigned>(value) * clamped + half) /
      kMaxLedBrightnessPercent);
}

}  // namespace ai_keyboard
